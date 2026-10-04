#include "dungeon_village_reference/world_actor_tail.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool v, const char *message) {
    ++checks;
    if (!v)
        throw std::runtime_error(message);
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
void source_order() {
    auto s = fixture();
    auto i = input(s);
    s.ai.contexts.at({1}).cell = {0, 0};
    s.ai.contexts.at({1}).inside_town = true; // Deliberately stale ax, not the L/M predicate.
    auto r = prepare_world_actor_tail(s, i);
    check(r.candidate && r.candidate->state.actors.at({1}).town_updates == 0 &&
              r.candidate->state.actors.at({1}).outside_updates == 1 &&
              r.candidate->state.ai.contexts.at({1}).cell == Position{1, 1} &&
              r.candidate->state.ai.contexts.at({1}).inside_town && r.candidate->queried_area,
          "old-s town predicate precedes physics, s/t refresh but ax remains old");
    s = fixture();
    s.ai.battle.actors.at({1}).physics_pause = 1;
    s.ai.battle.actors.at({1}).vertical_velocity = 3;
    r = prepare_world_actor_tail(s, input(s));
    check(r.candidate && !r.candidate->queried_area &&
              r.candidate->state.ai.battle.actors.at({1}).physics_pause == 0 &&
              r.candidate->state.ai.battle.actors.at({1}).position.height == 30 &&
              r.candidate->state.actors.at({1}).town_updates == 1,
          "P old1 decrements to0 yet skips gravity/K, no duplicated L update");
    s = fixture();
    s.actors.at({1}).town_updates = std::numeric_limits<int>::max() - 1;
    r = prepare_world_actor_tail(s, input(s));
    check(r.candidate && r.candidate->state.actors.at({1}).town_updates == 0,
          "world L preserves modulo INTMAX behavior");
}
void thresholds() {
    for (unsigned flags : {2U, 2U | 512U, 2U | 1024U, 2U | 16U})
        for (int count = 0; count <= 100; ++count) {
            auto s = fixture();
            auto i = input(s);
            i.spawn_cells = {{1, 1}};
            s.ai.battle.actors.at({1}).control.flags = flags;
            s.actors.at({1}).spawn_updates = count;
            const auto r = prepare_world_actor_tail(s, i);
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
        const auto r = prepare_world_actor_tail(s, input(s));
        check(r.candidate && r.candidate->delete_instance == (count + 1 >= 60),
              "empty G threshold60 preserved on actual owner");
    }
    auto s = fixture();
    s.ai.battle.actors.at({1}).control.flags = 64U;
    s.actors.at({1}).blocked_updates = 199;
    auto r = prepare_world_actor_tail(s, input(s));
    check(r.candidate && r.candidate->cleaned_up &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 19 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action == 11 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action_counter == 9 &&
              r.candidate->state.actors.at({1}).journey &&
              r.candidate->state.facilities.at(3).occupants == std::vector<CharacterId>{{1}, {2}},
          "ab200 r removes first occupation but keeps k/l and G, not n0/O");
    s = fixture(ActorKind::monster);
    r = prepare_world_actor_tail(s, input(s));
    check(r.candidate && r.candidate->delete_instance &&
              r.candidate->deletion_reason == ActorDeletionReason::unbound_monster,
          "monster missing db deletes after earlier timeout guards");
    s.ai.battle.actors.at({1}).control.state = 3;
    r = prepare_world_actor_tail(s, input(s));
    check(r.candidate && !r.candidate->delete_instance,
          "monster death3 exempt from missing-db deletion");
}
void bad_area_and_release() {
    auto s = fixture();
    s.ai.contexts.at({1}).move_area = false;
    s.ai.battle.actors.at({1}).control.action = 7;
    s.actors.at({1}).bad_area_updates = 29;
    auto r = prepare_world_actor_tail(s, input(s));
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
    check(!prepare_world_actor_tail(s, i).candidate && s.actors.at({1}).town_updates == 0,
          "disagreeing map authorities reject before any tail mutation");
    i = input(s);
    s.ai.battle.actors.at({1}).position.x = std::numeric_limits<float>::infinity();
    check(!prepare_world_actor_tail(s, i).candidate && s.actors.at({1}).town_updates == 0,
          "late invalid physics rolls back old-cell counters");
}
} // namespace
int main() {
    try {
        source_order();
        thresholds();
        bad_area_and_release();
        std::cout << checks << " world tail checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
