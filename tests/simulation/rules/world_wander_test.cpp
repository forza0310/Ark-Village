#include "ark/simulation/ai/rules/world_wander.hpp"

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
RescueWorldState fixture(int opcode = 10) {
    RescueWorldState s;
    s.map = {5, 5, std::vector<LegacyMapCell>(25)};
    BattleActorRecord a;
    a.id = {1};
    a.kind = ActorKind::human;
    a.control.state = 5;
    a.control.action = 7;
    a.control.action_counter = 9;
    a.control.alternate_counter = 11;
    a.control.queue = {opcode == 10 ? LegacyActorControl{10, 0} : LegacyActorControl{opcode},
                       {6, 512}};
    a.state_counter = 27;
    a.position = {420, 10, 430}; // Deliberately different from old cached s.
    a.encounter = 0;
    a.follow = CharacterId{2};
    s.ai.battle.actors.emplace(a.id, a);
    s.ai.human_order = {{1}};
    s.ai.contexts.emplace(a.id, RewardActorContext{{1, 1}, true, {}, {}, true});
    s.actors.emplace(a.id, RescueActorContext{});
    s.actors.at(a.id).journey = FacilityDeparture{};
    s.actors.at(a.id).journey->route.steps = {{1, 1}, {1, 2}};
    s.actors.at(a.id).waypoint = 1;
    BattleActorRecord followed;
    followed.id = {2};
    followed.kind = ActorKind::monster;
    followed.control.state = 3; // Follow13 does not requalify S by state/HP/membership.
    followed.position = {50, 0, 50};
    s.ai.battle.actors.emplace(followed.id, followed);
    s.ai.monster_order = {{2}};
    s.ai.contexts.emplace(followed.id, RewardActorContext{{3, 3}, false, {}, {}, false});
    RewardEncounter event;
    event.runtime.id = 0;
    event.runtime.center = {2, 2};
    s.ai.encounters.emplace(0, event);
    s.ai.encounter_order = {0};
    return s;
}
WorldWanderInput input(const RescueWorldState &s) {
    return {{1},
            {s.map, std::vector<int>(25, 3), std::vector<std::uint32_t>(25, 0), {-4, -3, -4, -3}},
            {0, 2, 3, 4, 99, 3, 5}};
}
void live_sources_and_fifo() {
    for (int opcode : {10, 12, 13}) {
        auto s = fixture(opcode);
        auto i = input(s);
        const auto r = prepare_world_wander(s, i);
        const Position center = opcode == 10   ? Position{1, 1}
                                : opcode == 12 ? Position{2, 2}
                                               : Position{3, 3};
        const Position first = opcode == 10   ? Position{1, 2}
                               : opcode == 12 ? Position{1, 3}
                                              : Position{2, 4};
        check(r.candidate && r.candidate->center == center && r.candidate->cells.front() == first,
              "10 reads own OLD s,12 current db center,13 followed OLD s, never current n");
        const auto &c = *r.candidate;
        const auto &a = c.state.ai.battle.actors.at({1});
        check(c.consumed_tickets == (opcode == 13 ? 4u : 7u) &&
                  c.append.front() == LegacyActorControl{0, first.x * 100 + 12, first.y * 100 + 13},
              "source candidate draw before x80/z80 offsets and opcode-specific waits");
        check(a.control.queue.front() == LegacyActorControl{6, 512} &&
                  a.control.queue[1] == c.append.front() && a.control.queue.back()[0] == opcode,
              "front wander erased BEFORE expansion appends BEHIND original FIFO tail");
        check(a.position.x == 420 && a.position.z == 430 && a.position.height == 10 &&
                  a.state_counter == 27 && a.control.action == 7 && a.control.action_counter == 9 &&
                  a.control.alternate_counter == 11 && a.control.flags == 0 &&
                  c.state.ai.contexts.at({1}).cell == Position{1, 1} &&
                  c.state.actors.at({1}).journey->route.steps ==
                      std::vector<Position>{{1, 1}, {1, 2}} &&
                  c.state.actors.at({1}).waypoint == 1 &&
                  s.ai.battle.actors.at({1}).control.queue.front()[0] == opcode,
              "expansion does not run movement0, pending tail, path, reproject or duplicate d "
              "counters");
    }
    auto s = fixture(12);
    s.ai.retired_encounters.emplace(0, s.ai.encounters.at(0));
    s.ai.encounters.clear();
    s.ai.encounter_order.clear();
    const auto retired_db = prepare_world_wander(s, input(s));
    check(retired_db.candidate && retired_db.candidate->center == Position{2, 2},
          "nonnull db still reads retired referenced encounter after removal from bn");
    s = fixture(13);
    s.ai.retired_actors.emplace(CharacterId{2}, s.ai.battle.actors.at({2}));
    s.ai.battle.actors.erase({2});
    s.ai.monster_order.clear();
    const auto retired_s = prepare_world_wander(s, input(s));
    check(retired_s.candidate && retired_s.candidate->center == Position{3, 3},
          "nonnull S still reads retired cached context without live bm eligibility");
}
void missing_references() {
    for (int opcode : {12, 13}) {
        auto s = fixture(opcode);
        if (opcode == 12)
            s.ai.battle.actors.at({1}).encounter.reset();
        else
            s.ai.battle.actors.at({1}).follow.reset();
        auto i = input(s);
        i.facts = {}; // Source does not query the map on this null-reference branch.
        i.tickets = {-999};
        const auto r = prepare_world_wander(s, i);
        check(r.candidate && !r.candidate->center && !r.candidate->consumed_tickets &&
                  r.candidate->append.empty() && r.candidate->cells.empty() &&
                  r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                      std::vector<LegacyActorControl>{{6, 512}},
              "missing db/S erases command only, no map reads, draws or self-repeat");
        s.ai.battle.actors.at({1}).control.queue.pop_back();
        const auto empty = prepare_world_wander(s, i);
        check(empty.candidate &&
                  empty.candidate->state.ai.battle.actors.at({1}).control.queue.empty(),
              "sole missing-reference opcode empties FIFO without synthetic wait");
    }
    auto s = fixture(12);
    s.ai.encounters.clear();
    check(!prepare_world_wander(s, input(s)).candidate,
          "nonnull dangling db rejected, not silently reinterpreted as no db");
    s = fixture(13);
    s.ai.battle.actors.erase({2});
    check(!prepare_world_wander(s, input(s)).candidate,
          "nonnull dangling S rejected instead of zero-draw success");
    s = fixture(13);
    s.ai.contexts.erase({2});
    check(!prepare_world_wander(s, input(s)).candidate,
          "nonnull S lacking its actual cached s rejected, not rebuilt from n");
}
void map_filters() {
    auto s = fixture();
    auto i = input(s);
    i.facts.town = {0, 2, 1, 3}; // Strict interior contains only candidate(1,2).
    auto r = prepare_world_wander(s, i);
    check(r.candidate && r.candidate->cells == std::vector<Position>{{2, 1}, {1, 0}, {0, 1}},
          "10 excludes strict town interior; its boundary itself is outside");
    s.ai.battle.actors.at({1}).control.queue.front()[1] = 1;
    i.facts.flags[7] = 2; // (2,1), source event-map bit2, no db needed for opcode10.
    r = prepare_world_wander(s, i);
    check(
        r.candidate && r.candidate->cells == std::vector<Position>{{2, 1}},
        "10 parameter1 keeps current map flags2, not actor.on_event_cell or encounter membership");
    s.map.cells[7].legacy_state = 5;
    i.facts.map = s.map;
    i.tickets = {99};
    r = prepare_world_wander(s, i);
    check(r.candidate && r.candidate->cells.empty() && r.candidate->consumed_tickets == 1 &&
              r.candidate->append == std::vector<LegacyActorControl>{{1, 119, 0}, {10, 1}},
          "no valid neighbor consumes ONLY wait100 and repeats original10 parameter");
    s = fixture(12);
    i = input(s);
    i.facts.town = {0, 2, 2, 4}; // First ring candidate(1,3) is inside.
    r = prepare_world_wander(s, i);
    check(r.candidate && r.candidate->cells.size() == 7 &&
              r.candidate->cells.front() == Position{2, 3},
          "12 applies current town filter in original eight-neighbor order");
    s = fixture(13);
    i = input(s);
    i.facts.town = {0, 4, 0, 5};
    r = prepare_world_wander(s, i);
    check(r.candidate && r.candidate->cells.size() == 8 &&
              r.candidate->append[1] == LegacyActorControl{1, 9, 0},
          "13 has no town/event/surface3 filter and uses wait10+5");
    for (auto &cell : s.map.cells)
        cell.legacy_state = 0;
    i.facts.map = s.map;
    i.tickets = {0};
    r = prepare_world_wander(s, i);
    check(r.candidate && r.candidate->append == std::vector<LegacyActorControl>{{1, 20, 0}, {13}} &&
              r.candidate->consumed_tickets == 1,
          "13 empty ring uses wait100+20, not its populated wait10+5");
}
void outside_centers_and_failure() {
    auto s = fixture();
    s.ai.contexts.at({1}).cell = {-1, 1};
    auto r = prepare_world_wander(s, input(s));
    check(r.candidate && r.candidate->cells == std::vector<Position>{{0, 1}} &&
              r.candidate->append.front() == LegacyActorControl{0, 12, 113},
          "outside cached center still admits an inside neighbor; no invented center guard");
    for (int opcode : {10, 12, 13})
        for (int x : {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
            s = fixture(opcode);
            if (opcode == 10)
                s.ai.contexts.at({1}).cell = {x, 1};
            else if (opcode == 12)
                s.ai.encounters.at(0).runtime.center = {x, 1};
            else
                s.ai.contexts.at({2}).cell = {x, 1};
            auto i = input(s);
            i.tickets = {0};
            r = prepare_world_wander(s, i);
            check(
                r.candidate && r.candidate->cells.empty() && r.candidate->consumed_tickets == 1,
                "signed extreme cached center skips wide-arithmetic outside neighbors without UB");
        }
    s = fixture();
    auto i = input(s);
    i.tickets = {0, 2, 3, 4, 99, 3}; // Last draw20 missing after partial pure preparation.
    check(!prepare_world_wander(s, i).candidate &&
              s.ai.battle.actors.at({1}).control.queue.front() == LegacyActorControl{10, 0},
          "late missing RNG rolls back initial command erase and all generated tail");
    i.tickets.push_back(20);
    check(!prepare_world_wander(s, i).candidate,
          "last ticket must be below CURRENT20 bound, no clamping or replacement RNG");
    i = input(s);
    i.facts.map.cells[0].legacy_state = 0;
    check(!prepare_world_wander(s, i).candidate,
          "contradictory logical map authorities reject before committing removal");
    s.ai.battle.actors.at({1}).control.queue.push_back({999});
    check(!prepare_world_wander(s, input(s)).candidate,
          "malformed future tail rejected as maintenance preflight, never deleted to pass");
    s = fixture();
    s.ai.human_order.push_back({1});
    check(!prepare_world_wander(s, input(s)).candidate,
          "current actor requires exactly one original-roster membership");
}
} // namespace
int main() {
    try {
        live_sources_and_fifo();
        missing_references();
        map_filters();
        outside_centers_and_failure();
        std::cout << checks << " world wander checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
