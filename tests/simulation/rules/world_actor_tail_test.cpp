#include "ark/simulation/actors/rules/world_actor_tail.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool v, const char *message) {
    ++checks;
    if (!v)
        throw std::runtime_error(message);
}
WorldActorTailResult tail(bool consuming, const RescueWorldState &source,
                          const WorldActorTailInput &input) {
    if (!consuming)
        return prepare_world_actor_tail(source, input);
    auto disposable = source;
    return prepare_world_actor_tail_consuming(std::move(disposable), input);
}
RescueWorldState fixture(ActorKind kind = ActorKind::human) {
    RescueWorldState s;
    s.map = {4, 4, std::vector<LegacyMapCell>(16)};
    for (auto &cell : s.map.cells)
        cell.legacy_state = 4;
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.occupants = {{1}, {1}, {2}};
    s.facilities.emplace(3, f);
    s.map = *bind_facility_map(s.map, {{f.placement, 3}}).map;
    BattleActorRecord a;
    a.id = {1};
    a.kind = kind;
    a.control.state = 14;
    a.control.flags = 2U;
    a.control.action = 11;
    a.control.action_counter = 9;
    a.control.alternate_counter = 20;
    a.capacity = 100;
    a.hp = {0, 10, 10, 10, false, 0};
    a.position = {150, 30, 150};
    s.ai.battle.actors.emplace(a.id, a);
    (kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order).push_back(a.id);
    s.ai.contexts.emplace(a.id, RewardActorContext{{1, 1}, true, {}, {}, true});
    s.actors.emplace(a.id, RescueActorContext{});
    s.actors.at(a.id).binding = ArrivalBinding{{1, 1}, {3}, 33};
    s.actors.at(a.id).journey = FacilityDeparture{};
    s.actors.at(a.id).journey->route.steps = {{1, 1}, {2, 1}};
    return s;
}
WorldActorTailInput input(const RescueWorldState &s) {
    return {
        {1}, {s.map, std::vector<int>(16, 0), std::vector<std::uint32_t>(16, 0), {0, 4, 0, 4}}, {}};
}
void source_order(bool consuming) {
    auto s = fixture();
    auto i = input(s);
    s.ai.contexts.at({1}).cell = {0, 0};
    s.ai.contexts.at({1}).inside_town = true; // Deliberately stale ax, not the L/M predicate.
    auto r = tail(consuming, s, i);
    check(r.candidate && r.candidate->state.actors.at({1}).town_updates == 0 &&
              r.candidate->state.actors.at({1}).outside_updates == 1 &&
              r.candidate->state.ai.contexts.at({1}).cell == Position{1, 1} &&
              r.candidate->state.ai.contexts.at({1}).inside_town && r.candidate->queried_area,
          "old-s town predicate precedes physics, s/t refresh but ax remains old");
    s = fixture();
    s.ai.battle.actors.at({1}).physics_pause = 1;
    s.ai.battle.actors.at({1}).vertical_velocity = 3;
    r = tail(consuming, s, input(s));
    check(r.candidate && !r.candidate->queried_area &&
              r.candidate->state.ai.battle.actors.at({1}).physics_pause == 0 &&
              r.candidate->state.ai.battle.actors.at({1}).position.height == 30 &&
              r.candidate->state.actors.at({1}).town_updates == 1,
          "P old1 decrements to0 yet skips gravity/K, no duplicated L update");
    s = fixture();
    s.actors.at({1}).town_updates = std::numeric_limits<int>::max() - 1;
    r = tail(consuming, s, input(s));
    check(r.candidate && r.candidate->state.actors.at({1}).town_updates == 0,
          "world L preserves modulo INTMAX behavior");
    check(s.actors.at({1}).town_updates == std::numeric_limits<int>::max() - 1 &&
              s.ai.battle.actors.at({1}).position.height == 30 &&
              r.candidate->state.map.cells.size() == 16 &&
              r.candidate->state.facilities.at(3).occupants.size() == 3,
          "tail keeps source baseline and returns the complete independent world");
    r.candidate->state.map.cells.front().legacy_state = 99;
    check(s.map.cells.front().legacy_state == 4,
          "transferred candidate map storage is independent of the source world");
}
void thresholds(bool consuming) {
    for (unsigned flags : {2U, 2U | 512U, 2U | 1024U, 2U | 16U})
        for (int count = 0; count <= 100; ++count) {
            auto s = fixture();
            auto i = input(s);
            i.spawn_cells = {{1, 1}};
            s.ai.battle.actors.at({1}).control.flags = flags;
            s.actors.at({1}).spawn_updates = count;
            const auto r = tail(consuming, s, i);
            const int threshold = flags & (512U | 1024U) ? 30 : 100;
            check(r.candidate && r.candidate->delete_instance == (count + 1 >= threshold) &&
                      r.candidate->state.actors.at({1}).spawn_updates == count + 1 &&
                      r.candidate->state.ai.battle.actors.count({1}) == 1 &&
                      r.candidate->state.facilities.at(3).occupants.size() == 3,
                  "spawn timeout is a return request, not premature erase/release");
        }
    for (int count = 0; count <= 60; ++count) {
        auto s = fixture();
        s.ai.battle.actors.at({1}).control.state = 0;
        s.actors.at({1}).journey.reset();
        s.actors.at({1}).no_path_updates = count;
        const auto r = tail(consuming, s, input(s));
        check(r.candidate && r.candidate->delete_instance == (count + 1 >= 60),
              "empty G threshold60 preserved on actual owner");
    }
    auto s = fixture();
    s.ai.battle.actors.at({1}).control.flags = 64U;
    s.actors.at({1}).blocked_updates = 199;
    auto r = tail(consuming, s, input(s));
    check(r.candidate && r.candidate->cleaned_up &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 19 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action == 11 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action_counter == 9 &&
              r.candidate->state.actors.at({1}).journey &&
              r.candidate->state.facilities.at(3).occupants == std::vector<CharacterId>{{1}, {2}},
          "ab200 r removes first occupation but keeps k/l and G, not n0/O");
    s = fixture(ActorKind::monster);
    r = tail(consuming, s, input(s));
    check(r.candidate && r.candidate->delete_instance &&
              r.candidate->deletion_reason == ActorDeletionReason::unbound_monster,
          "monster missing db deletes after earlier timeout guards");
    s.ai.battle.actors.at({1}).control.state = 3;
    r = tail(consuming, s, input(s));
    check(r.candidate && !r.candidate->delete_instance,
          "monster death3 exempt from missing-db deletion");
}
void bad_area_and_release(bool consuming) {
    auto s = fixture();
    s.ai.contexts.at({1}).move_area = false;
    s.ai.battle.actors.at({1}).control.action = 7;
    s.actors.at({1}).bad_area_updates = 29;
    auto r = tail(consuming, s, input(s));
    check(r.candidate && r.candidate->cleaned_up && !r.candidate->delete_instance &&
              !r.candidate->state.actors.at({1}).journey &&
              (r.candidate->state.ai.battle.actors.at({1}).control.flags & 32768U) &&
              r.candidate->state.ai.battle.actors.at({1}).hp.target == 1 &&
              r.candidate->state.ai.battle.actors.at({1}).hp.displayed == 1 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action == 0,
          "aI30 O->r->escape32768->g(action7)==0 d1->n0 executes in order");
    s = fixture();
    auto removed = prepare_world_actor_remove(s, {1}, true);
    check(removed.candidate && removed.candidate->state.ai.battle.actors.empty() &&
              removed.candidate->state.ai.human_order.empty() &&
              removed.candidate->state.ai.retired_actors.count({1}) &&
              removed.candidate->state.ai.contexts.count({1}) &&
              removed.candidate->state.facilities.at(3).occupants ==
                  std::vector<CharacterId>{{1}, {2}},
          "human d erase releases first q then retires actual object/context, no eager GC");
    removed = prepare_world_actor_remove(s, {1}, false);
    check(removed.candidate && removed.candidate->state.facilities.at(3).occupants.size() == 3,
          "c true erase has no added d-style occupation release");
    s = fixture();
    auto i = input(s);
    i.facts.map.cells[0].legacy_state = 3;
    check(!tail(consuming, s, i).candidate && s.actors.at({1}).town_updates == 0,
          "disagreeing map authorities reject before any tail mutation");
    i = input(s);
    s.ai.battle.actors.at({1}).position.x = std::numeric_limits<float>::infinity();
    check(!tail(consuming, s, i).candidate && s.actors.at({1}).town_updates == 0,
          "late invalid physics rolls back old-cell counters");
}
void ground_path_retention(bool consuming) {
    auto s = fixture();
    auto &ctx = s.actors.at({1});
    ctx.journey.reset();
    ctx.unbound_route = LegacyPathResult{};
    ctx.unbound_route->steps = {{1, 1}, {2, 1}};
    ctx.destination = Position{2, 1};
    ctx.path_pending = true;
    s.ai.battle.actors.at({1}).control.state = 0;
    ctx.no_path_updates = 59;
    auto r = tail(consuming, s, input(s));
    check(r.candidate && !r.candidate->delete_instance &&
              r.candidate->state.actors.at({1}).no_path_updates == 0 &&
              r.candidate->state.actors.at({1}).unbound_route,
          "real ground G is nonempty without a facility binding, not empty-path deletion60");
    s.ai.contexts.at({1}).move_area = false;
    ctx.bad_area_updates = 29;
    s.ai.battle.actors.at({1}).control.action = 7;
    r = tail(consuming, s, input(s));
    check(r.candidate && r.candidate->cleaned_up &&
              !r.candidate->state.actors.at({1}).unbound_route &&
              r.candidate->state.actors.at({1}).destination == Position{2, 1},
          "bad-area O() clears actual ground G, r still retains old O destination");
}
void projected_facing_order(bool consuming) {
    auto s = fixture();
    s.ai.battle.actors.at({1}).control.flags = 64U;
    s.actors.at({1}).blocked_updates = 199;
    auto i = input(s);
    int calls{};
    i.facing_after_projection = [&](const BattleActorRecord &actor) -> std::optional<int> {
        ++calls;
        check(actor.control.state == 14 && actor.position.height < 30,
              "direction reads actual physics result before retention changes state");
        return 2;
    };
    const auto result = tail(consuming, s, i);
    check(result.candidate && calls == 1 && result.candidate->cleaned_up &&
              result.candidate->state.ai.battle.actors.at({1}).control.state == 19 &&
              result.candidate->state.ai.battle.actors.at({1}).control.facing == 2,
          "projected direction survives subsequent actual r cleanup");
    check(result.candidate->projected_actor &&
              result.candidate->projected_actor->position.height > 0 &&
              result.candidate->state.ai.battle.actors.at({1}).position.height == 0,
          "tail retains actual pre-r projection separately from cleaned current n");
    for (const int state : {4, 20}) {
        auto excluded = fixture();
        excluded.ai.battle.actors.at({1}).control.state = state;
        excluded.ai.battle.actors.at({1}).control.facing = 3;
        auto excluded_input = input(excluded);
        excluded_input.facing_after_projection =
            [](const BattleActorRecord &) -> std::optional<int> { return {}; };
        const auto kept = tail(consuming, excluded, excluded_input);
        check(kept.candidate && kept.candidate->state.ai.battle.actors.at({1}).control.facing == 3,
              "state4/20 skips direction callback and preserves source facing");
    }
    for (const int direction : {-1, 4}) {
        i.facing_after_projection = [direction](const BattleActorRecord &) {
            return std::optional<int>{direction};
        };
        check(!tail(consuming, s, i).candidate && s.actors.at({1}).blocked_updates == 199 &&
                  s.facilities.at(3).occupants.size() == 3,
              "invalid direction rolls back physics and later occupancy cleanup");
    }
    i.facing_after_projection = [](const BattleActorRecord &) -> std::optional<int> { return {}; };
    check(!tail(consuming, s, i).candidate && s.actors.at({1}).town_updates == 0,
          "missing projected metadata refuses without partial old-cell counters");
    i.facing_after_projection = [](const BattleActorRecord &) -> std::optional<int> {
        throw std::runtime_error("explicit callback failure");
    };
    const auto throwing = tail(consuming, s, i);
    check(!throwing.candidate && throwing.error == RescueWorldError::preparation_failed &&
              s.facilities.at(3).occupants.size() == 3 && s.actors.at({1}).town_updates == 0,
          "throwing facing consumer refuses without publishing physics or cleanup");
}
void transfer_boundaries() {
    auto s = fixture();
    auto i = input(s);
    i.facts.map.cells.front().legacy_state = 3;
    const auto mismatch = prepare_world_actor_tail_consuming(std::move(s), i);
    check(!mismatch.candidate && mismatch.error == RescueWorldError::invalid_input &&
              s.map.cells.size() == 16 && s.ai.battle.actors.count({1}) == 1 &&
              s.actors.at({1}).town_updates == 0,
          "map rejection precedes even the rvalue world transfer");
    i = input(s);
    i.actor = {99};
    const auto stale = prepare_world_actor_tail_consuming(std::move(s), i);
    check(!stale.candidate && stale.error == RescueWorldError::stale_actor &&
              s.map.cells.size() == 16 && s.ai.battle.actors.count({1}) == 1,
          "stale identity rejects before transferring private storage");

    s.ai.human_order.clear();
    s.ai.battle.actors.at({1}).legacy_id = -1;
    s.ai.battle.actors.at({1}).control.flags = 64U;
    s.actors.at({1}).blocked_updates = 199;
    const auto detached_input = input(s);
    const auto copied = prepare_world_detached_actor_tail(s, detached_input);
    auto disposable = s;
    const auto consumed =
        prepare_world_detached_actor_tail_consuming(std::move(disposable), detached_input);
    check(copied.candidate && consumed.candidate && copied.candidate->cleaned_up &&
              consumed.candidate->cleaned_up && consumed.candidate->state.ai.human_order.empty() &&
              consumed.candidate->state.ai.monster_order.empty() &&
              consumed.candidate->state.ai.battle.actors.at({1}).control.state ==
                  copied.candidate->state.ai.battle.actors.at({1}).control.state &&
              consumed.candidate->state.facilities.at(3).occupants ==
                  copied.candidate->state.facilities.at(3).occupants &&
              consumed.candidate->projected_actor &&
              consumed.candidate->projected_actor->control.state == 14 &&
              s.facilities.at(3).occupants.size() == 3,
          "detached consuming cleanup preserves no-roster identity and full pre-cleanup audit");
}
} // namespace
int main() {
    try {
        transfer_boundaries();
        for (const bool consuming : {false, true}) {
            try {
                source_order(consuming);
                thresholds(consuming);
                bad_area_and_release(consuming);
                ground_path_retention(consuming);
                projected_facing_order(consuming);
            } catch (const std::exception &e) {
                throw std::runtime_error(
                    std::string(consuming ? "consuming tail: " : "const tail: ") + e.what());
            }
        }
        std::cout << checks << " world tail checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
