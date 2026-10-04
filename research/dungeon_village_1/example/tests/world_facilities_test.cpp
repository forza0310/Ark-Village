#include "dungeon_village_reference/world_facilities.hpp"

#include <cmath>
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
RescueWorldState fixture(int category = 2, int detail = 0) {
    RescueWorldState s;
    s.map = {4, 4, std::vector<LegacyMapCell>(16)};
    for (auto &cell : s.map.cells)
        cell.legacy_state = 4;
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.category = category;
    f.detail = detail;
    f.definition_wait = 20;
    f.upgrade_uses = {2, 10};
    s.facilities.emplace(3, f);
    s.map = *bind_facility_map(s.map, {{f.placement, 3}}).map;
    s.facility_uses.emplace(33, FacilityUseProgress{});
    BattleActorRecord a;
    a.id = {1};
    a.control.state = 14;
    a.capacity = 100;
    a.position = {150, 0, 150};
    a.hp = {0, 10, 10, 10, false, 0};
    s.ai.battle.actors.emplace(a.id, a);
    s.ai.human_order = {a.id};
    s.ai.contexts.emplace(a.id, RewardActorContext{{1, 1}, true, {}, {}});
    s.actors.emplace(a.id, RescueActorContext{});
    s.actors.at(a.id).binding = ArrivalBinding{{1, 1}, {3}, 33};
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    s.ai.professions.push_back({{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 5, true});
    s.ai.growth.emplace(0, g);
    return s;
}
void prefix_once() {
    auto s = fixture();
    auto &a = s.ai.battle.actors.at({1});
    a.state_counter = 17;
    a.control.action_counter = 4;
    a.control.alternate_counter = 20;
    a.control.queue = {{21}, {1, 2, 0}, {24}};
    s.ai.contexts.at({1}).effects.display = {{11, 0}};
    s.ai.growth.at(0).pending = {9, -2};
    WorldFacilityExecutionInput i;
    i.control.actor = {1};
    auto r = prepare_world_facility_execution(s, i);
    check(r.candidate && r.candidate->occupied && !r.candidate->exited &&
              r.candidate->state.ai.battle.actors.at({1}).state_counter == 18 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action_counter == 5 &&
              r.candidate->state.ai.battle.actors.at({1}).control.alternate_counter == 21 &&
              r.candidate->state.ai.growth.at(0).pending.counter == -1 &&
              r.candidate->state.ai.contexts.at({1}).effects.display.front()[1] == 1 &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue.front()[1] == 1,
          "common prefix/growth/display/wait advance exactly once before occupation");
    const auto control = prepare_world_inn_control(s, {1});
    check(control.candidate &&
              control.candidate->state.ai.battle.actors.at({1}).state_counter == 17 &&
              control.candidate->state.ai.growth.at(0).pending.counter == -2,
          "control-only entry cannot advance shared d prefix");
    s.ai.growth.clear();
    check(!prepare_world_facility_execution(s, i).candidate && s.facilities.at(3).occupants.empty(),
          "missing definition stops common execution atomically, no fixture fallback");
    s = fixture();
    s.ai.battle.actors.at({1}).control.queue = {{21}, {24}};
    s.facility_uses.at(33).completed_uses = std::numeric_limits<int>::max();
    check(!prepare_world_facility_execution(s, i).candidate &&
              s.ai.battle.actors.at({1}).state_counter == 0 && s.facilities.at(3).occupants.empty(),
          "late exit overflow rolls back common prefix and occupation together");
}
void rest_chain() {
    for (int direction = 0; direction < 4; ++direction) {
        auto s = fixture(8, 2);
        const auto use = prepare_world_facility_use(s, {{1}, 0, Position{150, 150}, direction});
        check(use.state && use.state->ai.battle.actors.at({1}).control.state == 14 &&
                  use.state->ai.battle.actors.at({1}).control.queue.size() == 19,
              "rest plan resolves target and direction without occupation21");
        s = *use.state;
        WorldFacilityControlInput input{{1}, {{0, 3, 0}}, {}};
        int rounds{};
        int expressions{};
        do {
            // In production the common prefix and physics run around this control segment.
            const auto r = prepare_world_facility_control(s, input);
            check(r.candidate.has_value(), "rest full interpreter segment succeeds");
            s = r.candidate->state;
            expressions += static_cast<int>(r.candidate->consumed_expressions);
            check(!r.candidate->occupied && s.facilities.at(3).occupants.empty() &&
                      s.ai.battle.actors.at({1}).hp.target == 10,
                  "category8 never occupies or invokes inn HP restoration");
            if (rounds == 0)
                check(s.ai.battle.actors.at({1}).position.height == 30 &&
                          s.ai.battle.actors.at({1}).physics_pause == 110 &&
                          s.ai.battle.actors.at({1}).control.facing == direction &&
                          !(s.ai.battle.actors.at({1}).control.flags & 2U),
                      "first rest segment assigns height/P and clears2 before wait12");
            ++rounds;
            if (r.candidate->exited)
                break;
        } while (rounds < 200);
        check(rounds == 106 && expressions == 1 && s.facility_uses.at(33).completed_uses == 1 &&
                  s.ai.battle.actors.at({1}).position.height == 0 &&
                  s.ai.battle.actors.at({1}).physics_pause == 0 &&
                  s.ai.battle.actors.at({1}).control.queue ==
                      std::vector<LegacyActorControl>{{8, 0}},
              "12+6+70+10+12 waits overlap continuation calls, one exit and no inn expression9");
        s = fixture(8, 2);
        s.ai.battle.actors.at({1}).state_counter = 170;
        const auto c = prepare_world_inn_c(s, {1});
        check(c.candidate && !c.candidate->recovered,
              "state14 B170 alone is not category2 recovery");
    }
}
void motion_launch_expression() {
    auto s = fixture(6, 3);
    auto use = prepare_world_facility_use(s, {{1}, 0, Position{150, 150}, {}});
    check(use.state && use.ground_effect20, "special entry requests ground20 at arrangement");
    s = *use.state;
    WorldFacilityControlInput i{{1}, {}, {3}};
    for (int n = 0; n < 20; ++n) {
        const auto r = prepare_world_facility_control(s, i);
        check(r.candidate.has_value(), "special entry waiting does not require early launch draw");
        s = r.candidate->state;
        check(r.candidate->consumed_launch_tickets == (n == 19 ? 1U : 0U),
              "launch draws only after wait20 expires");
    }
    const auto &a = s.ai.battle.actors.at({1});
    check(a.control.state == 15 && a.state_counter == 0 && a.control.queue.empty() &&
              std::abs(a.vertical_velocity - 240.0F / 29.0F) < 0.00001F &&
              std::abs(s.actors.at({1}).horizontal_velocity.z + 1000.0F / 60.0F) < 0.00001F,
          "opcode23 uses old s minus10 and center, then actual c15 clears queue/B/C/i");
    s = fixture();
    s.ai.battle.actors.at({1}).control.queue = {{2, 15}, {22, 70, 90}};
    auto r = prepare_world_facility_control(s, {{1}, {}, {}});
    check(r.candidate && r.candidate->state.ai.battle.actors.at({1}).control.queue.empty() &&
              r.candidate->state.ai.battle.actors.at({1}).position.height == 0,
          "opcode2 c15 clears remaining tail; do not restore saved queue");
    s = fixture();
    s.ai.battle.actors.at({1}).control.queue = {{0, 200, 150}, {22, 30, 110}};
    r = prepare_world_facility_control(s, {{1}, {}, {}});
    check(r.candidate && r.candidate->flow == ActorControlFlow::moving &&
              std::abs(r.candidate->state.ai.battle.actors.at({1}).position.x - 156.7F) < 0.001F &&
              r.candidate->state.ai.contexts.at({1}).cell == Position{1, 1} &&
              r.candidate->state.ai.battle.actors.at({1}).position.height == 0,
          "opcode0 moves6.7, stops before suffix and does not eagerly reproject s");
    s.ai.battle.actors.at({1}).control.flags = 64U;
    r = prepare_world_facility_control(s, {{1}, {}, {}});
    check(r.candidate && r.candidate->state.ai.battle.actors.at({1}).position.x == 150,
          "64 locks opcode0 movement without consuming target");
    s = fixture();
    s.ai.battle.actors.at({1}).control.queue = {{21}, {18, 15, 20}};
    check(!prepare_world_facility_control(s, {{1}, {}, {}}).candidate &&
              s.facilities.at(3).occupants.empty(),
          "missing late expression draw rolls back earlier occupation");
    s.ai.contexts.at({1}).effects.display = {{12, 0, 30, 0, 0}};
    r = prepare_world_facility_control(s, {{1}, {{0, 3, {}}}, {}});
    check(r.candidate && r.candidate->consumed_expressions == 1 &&
              r.candidate->state.ai.contexts.at({1}).effects.display.size() == 1,
          "suppressed expression still consumes1000 but no variant draw");
}
void home_and_handoff() {
    auto s = fixture(9);
    s.actors.at({1}).visits.legacy_visit_counts.fill(7);
    s.ai.battle.actors.at({1}).hp = {80, 10, 10, 90, true, 5};
    s.ai.battle.actors.at({1}).control.queue = {{24}};
    auto r = prepare_world_facility_control(s, {{1}, {}, {}});
    check(r.candidate && r.candidate->exited &&
              r.candidate->state.ai.battle.actors.at({1}).hp.target == 100 &&
              r.candidate->state.ai.battle.actors.at({1}).hp.origin == 100 &&
              r.candidate->state.ai.battle.actors.at({1}).hp.animating &&
              r.candidate->state.ai.battle.actors.at({1}).hp.legacy_tick == 5 &&
              r.candidate->state.actors.at({1}).visits.legacy_visit_counts == std::array<int, 6>{},
          "home exit d(h) resets three HP slots and I but preserves animation/tick");
    s = fixture(1);
    s.ai.battle.actors.at({1}).control.queue = {{24}};
    r = prepare_world_facility_control(s, {{1}, {}, {}});
    check(r.candidate && r.candidate->flow == ActorControlFlow::delegated && !r.candidate->exited &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue.front()[0] == 24,
          "shop satisfaction/equipment remains unconsumed explicit handoff");
    s = fixture();
    s.ai.battle.actors.at({1}).object_slot = -2;
    WorldFacilityExecutionInput i;
    i.control.actor = {1};
    check(!prepare_world_facility_execution(s, i).candidate &&
              s.ai.battle.actors.at({1}).state_counter == 0,
          "carry expression17 must be consumed between growth and control");
    i.carry_expression = WorldExpressionTicket{999, 2, {}};
    const auto d = prepare_world_facility_execution(s, i);
    check(d.candidate && d.candidate->state.ai.battle.actors.at({1}).state_counter == 1,
          "failed probability still counts as actual carry-expression draw");
    for (int status : {0, 1, 2, 3}) {
        s = fixture();
        s.facilities.at(3).status = status;
        s.ai.battle.actors.at({1}).control.queue = {{21}, {24}};
        const auto q = prepare_world_facility_control(s, {{1}, {}, {}});
        check(q.candidate && q.candidate->occupied && q.candidate->exited &&
                  !q.candidate->cleaned_up &&
                  q.candidate->state.facility_uses.at(33).completed_uses == 1,
              "q binding is independent of tenant status, including construction0");
    }
    s = fixture();
    s.ai.contexts.at({1}).cell = {0, 0};
    s.actors.at({1}).binding->definition_id = 999;
    s.ai.battle.actors.at({1}).control.queue = {{21}, {1, 2, 0}};
    r = prepare_world_facility_control(s, {{1}, {}, {}});
    check(r.candidate && r.candidate->occupied &&
              r.candidate->state.facilities.at(3).occupants == std::vector<CharacterId>{{1}},
          "occupation21 reads current O cell despite different old s/O definition; q differs");
    s = fixture(5);
    s.ai.battle.actors.at({1}).control.queue = {{21}};
    r = prepare_world_facility_control(s, {{1}, {}, {}});
    check(r.candidate && r.candidate->flow == ActorControlFlow::delegated &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue.front()[0] == 21 &&
              !r.candidate->occupied,
          "category5 occupation cannot silently skip actor growth/page side effects");
}
void special_lifecycle() {
    for (bool inside : {false, true}) {
        auto s = fixture();
        auto &a = s.ai.battle.actors.at({1});
        a.control.state = 15;
        a.control.action = 6;
        a.vertical_velocity = 240.0F / 29.0F;
        s.actors.at({1}).horizontal_velocity = {0, 10};
        s.ai.contexts.at({1}).inside_town = !inside; // Stale ax must not decide B70's h.b(s).
        for (int old_b = 0; old_b <= 70; ++old_b) {
            const auto c = prepare_world_special_entry_c(
                s, {1}, inside ? TownBounds{0, 4, 0, 4} : TownBounds{2, 4, 0, 4},
                old_b == 15 ? std::optional<WorldExpressionTicket>({0, 2, 0}) : std::nullopt);
            check(c.candidate && c.candidate->ground_effect == (old_b == 60) &&
                      c.candidate->completed == (old_b == 70),
                  "state15 reads OLD B at15/60/70 and never advances the counter");
            s = c.candidate->state;
            const auto &actor = s.ai.battle.actors.at({1});
            if (old_b == 15)
                check(s.ai.contexts.at({1}).effects.display.front() ==
                          ActorEffectRecord{12, 0, 30, 16, 0},
                      "c expression16 inserted before this round d display pass");
            if (old_b == 60)
                check(actor.position.z == 760 && actor.position.height == 0 &&
                          actor.vertical_velocity == 0 &&
                          s.actors.at({1}).horizontal_velocity.z == 0,
                      "B60 applies horizontal move first, then stops velocities and height");
            if (old_b == 70) {
                check(actor.state_counter == 0 && actor.control.state == 0 &&
                          actor.control.action == 0 &&
                          actor.control.queue == (inside ? std::vector<LegacyActorControl>{{8, 0}}
                                                         : std::vector<LegacyActorControl>{}),
                      "B70 actual c0 resets control; h.b(old s), not stale ax/new n, queues "
                      "activity0");
                break;
            }
            WorldFacilityExecutionInput input;
            input.control.actor = {1};
            const auto d = prepare_world_facility_execution(s, input);
            check(d.candidate &&
                      d.candidate->state.ai.battle.actors.at({1}).state_counter == old_b + 1,
                  "special-entry c plus common d advances B exactly once");
            s = d.candidate->state;
            const auto &current = s.ai.battle.actors.at({1});
            const auto gravity = prepare_actor_physics({15,
                                                        current.control.flags,
                                                        0,
                                                        current.position.height,
                                                        current.vertical_velocity,
                                                        {current.position.x, current.position.z},
                                                        {},
                                                        true,
                                                        true,
                                                        0});
            check(gravity.has_value(), "parabola shares established d gravity");
            s.ai.battle.actors.at({1}).position.height = gravity->state.height;
            s.ai.battle.actors.at({1}).vertical_velocity = gravity->state.vertical_velocity;
        }
    }
    auto s = fixture();
    s.ai.battle.actors.at({1}).control.state = 15;
    s.ai.battle.actors.at({1}).state_counter = 15;
    s.actors.at({1}).horizontal_velocity = {0, 10};
    check(!prepare_world_special_entry_c(s, {1}, {0, 4, 0, 4}).candidate &&
              s.ai.battle.actors.at({1}).position.z == 150,
          "missing B15 expression rolls back horizontal movement");
    s = fixture(6, 3);
    s.ai.battle.actors.at({1}).kind = ActorKind::monster;
    s.ai.battle.actors.at({1}).encounter = 7;
    s.ai.human_order.clear();
    s.ai.monster_order = {{1}};
    auto use = prepare_world_facility_use(s, {{1}, 0, Position{150, 150}, {}});
    check(use.state.has_value(), "monster actual P special-entry use shares projected queue");
    s = *use.state;
    s.ai.battle.actors.at({1}).control.queue = {{23}, {2, 15}};
    const auto launch = prepare_world_facility_control(s, {{1}, {}, {0}});
    check(launch.candidate && launch.candidate->state.ai.battle.actors.at({1}).encounter == 7 &&
              launch.candidate->state.ai.battle.actors.at({1}).control.state == 15,
          "monster c15 keeps db unlike human transition");
    s = launch.candidate->state;
    s.ai.battle.actors.at({1}).state_counter = 70;
    const auto landing = prepare_world_special_entry_c(s, {1}, {0, 4, 0, 4});
    check(landing.candidate && landing.candidate->state.actors.at({1}).monster_mode == 1 &&
              landing.candidate->state.ai.battle.actors.at({1}).control.state == 17 &&
              landing.candidate->state.ai.battle.actors.at({1}).baseline == 17 &&
              landing.candidate->state.ai.battle.actors.at({1}).encounter == 7 &&
              landing.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 7}},
          "monster B70 sets T1 then actual c17 and activity7 without releasing event");
}
} // namespace
int main() {
    try {
        prefix_once();
        rest_chain();
        motion_launch_expression();
        home_and_handoff();
        special_lifecycle();
        std::cout << checks << " world facility checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
