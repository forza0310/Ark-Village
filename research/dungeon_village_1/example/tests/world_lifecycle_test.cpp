#include "dungeon_village_reference/world_control.hpp"
#include "dungeon_village_reference/world_lifecycle.hpp"
#include "dungeon_village_reference/world_schedule.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
RescueWorldState fixture(int state, ActorKind kind = ActorKind::human) {
    RescueWorldState s;
    s.map = {6, 6, std::vector<LegacyMapCell>(36)};
    for (auto &cell : s.map.cells) {
        cell.legacy_state = 4;
        cell.category = RouteCategory::road;
    }
    BattleActorRecord a;
    a.id = {1};
    a.kind = kind;
    a.control.state = state;
    a.control.action = 7;
    a.control.action_counter = 9;
    a.control.alternate_counter = 13;
    a.control.queue = {{1, 8, 0}, {8, 3}};
    a.control.flags = 16U | 64U | 4U;
    a.baseline = kind == ActorKind::human ? 5 : 17;
    a.state_counter = 23;
    a.state_parameter = 7;
    a.capacity = 100;
    a.hp = {11, 25, 19, 27, true, 6};
    a.position = {150, 4, 150};
    a.encounter = 99;
    a.group = 99;
    a.vertical_velocity = 2;
    s.ai.battle.actors.emplace(a.id, a);
    (kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order).push_back(a.id);
    s.ai.contexts.emplace(a.id, RewardActorContext{{1, 1}, true, {}, {}, true, {3, 3}});
    s.actors.emplace(a.id, RescueActorContext{});
    auto &path = s.actors.at(a.id);
    path.horizontal_velocity = {2, -3};
    path.journey = FacilityDeparture{};
    path.journey->route.steps = {{1, 1}, {2, 1}};
    path.unbound_route = LegacyPathResult{};
    path.unbound_route->steps = {{2, 1}};
    path.destination = Position{2, 1};
    path.path_pending = true;
    path.waypoint = 1;
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    s.ai.professions.push_back({{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 5, true});
    g.derived = *derive_human_stats(g.definition, s.ai.professions).candidate;
    s.ai.growth.emplace(0, g);
    s.ai.battle.humans.emplace(0, HumanBattleRecord{});
    return s;
}
WorldLifecycleInput input(std::vector<WorldExpressionTicket> expressions = {}) {
    return {{1}, std::move(expressions)};
}
void down_recovery() {
    for (const auto kind : {ActorKind::human, ActorKind::monster})
        for (int capacity : {1, 100, 899, 900, 1800, std::numeric_limits<int>::max()})
            for (int displayed : {-10, 0, 25, 150, std::numeric_limits<int>::max()})
                for (int counter : {0, 899, 900, 901}) {
                    auto s = fixture(2, kind);
                    auto &old = s.ai.battle.actors.at({1});
                    old.capacity = capacity;
                    old.hp.displayed = displayed;
                    old.state_counter = counter;
                    const auto r = prepare_world_lifecycle_c(s, input({{999, 1, {}}, {0, 1, 0}}));
                    check(r.candidate.has_value(), "down actual owner candidate prepares");
                    const auto &a = r.candidate->state.ai.battle.actors.at({1});
                    const int healed = static_cast<int>(std::min<std::int64_t>(
                        static_cast<std::int64_t>(displayed) + std::max(capacity / 900, 1),
                        capacity));
                    check(a.hp.displayed == (counter >= 900 ? capacity : healed) &&
                              a.hp.target == a.hp.displayed &&
                              a.hp.origin == (counter >= 900 ? capacity : 19) &&
                              a.hp.requested_delta == 11 && a.hp.animating && a.hp.legacy_tick == 6,
                          "am1/3 recover from signed display; d(h) writes1/2/3 without animation "
                          "reset");
                    check(a.state_counter == counter && a.state_parameter == 7 &&
                              a.control.alternate_counter == 13 &&
                              a.control.state == (counter >= 900 ? old.baseline : 2) &&
                              r.candidate->restored_baseline == (counter >= 900) &&
                              r.candidate->consumed_expressions == (counter >= 900 ? 2U : 1U) &&
                              r.candidate->consumed_variants == (counter >= 900 ? 1U : 0U),
                          "old B900 chooses b not c; exact expression calls/tickets and preserved "
                          "B/C/i");
                    check(r.candidate->state.actors.at({1}).journey.has_value() &&
                              r.candidate->state.ai.contexts.at({1}).cell == Position{1, 1} &&
                              s.ai.battle.actors.at({1}).hp.displayed == displayed,
                          "c recovery does not clear path/project cache or mutate source owner");
                }
    auto s = fixture(2);
    s.ai.battle.actors.at({1}).state_counter = 900;
    const auto suppressed = prepare_world_lifecycle_c(s, input({{0, 2, 1}, {0, 1, {}}}));
    check(suppressed.candidate && suppressed.candidate->consumed_expressions == 2 &&
              suppressed.candidate->consumed_variants == 1 &&
              suppressed.candidate->state.ai.contexts.at({1}).effects.display ==
                  std::vector<ActorEffectRecord>{{12, 0, 30, 3, 1}} &&
              suppressed.candidate->state.ai.battle.actors.at({1}).hp.target == 100,
          "expression3 precedes4; second is suppressed but probability ticket still consumed");
    const auto missing = prepare_world_lifecycle_c(s, input({{999, 1, {}}}));
    check(!missing.candidate && missing.error == WorldLifecycleError::missing_ticket &&
              s.ai.battle.actors.at({1}).hp.displayed == 25 &&
              s.ai.contexts.at({1}).effects.display.empty(),
          "late missing expression4 ticket rolls back earlier recovery and display");
}
void knockback_and_baseline() {
    for (const auto kind : {ActorKind::human, ActorKind::monster})
        for (int counter : {5, 6, 7})
            for (int slot : {-2, -1, 0, 10})
                for (int mode = 0; mode <= 4; ++mode) {
                    auto s = fixture(4, kind);
                    auto &a = s.ai.battle.actors.at({1});
                    a.state_counter = counter;
                    a.object_slot = slot;
                    s.actors.at({1}).monster_mode = mode;
                    const auto r = prepare_world_lifecycle_c(s, input());
                    check(r.candidate.has_value(),
                          "knockback prepares for both kinds and rescue sentinel");
                    const auto &next = r.candidate->state.ai.battle.actors.at({1});
                    check(std::abs(next.position.x - 151.2F) < 0.0001F &&
                              std::abs(next.position.z - 148.2F) < 0.0001F &&
                              next.position.height == 4 && next.vertical_velocity == 2 &&
                              r.candidate->state.ai.contexts.at({1}).cell == Position{1, 1},
                          "damped r precedes n movement; c does not gravity/project/refresh s");
                    if (counter < 6) {
                        check(next.control.state == 4 && next.control.queue == a.control.queue &&
                                  next.state_counter == 5,
                              "old B5 retains state and original waiting queue");
                    } else if (slot == -1) {
                        check(
                            next.control.state == 1 && next.state_counter == 0 &&
                                next.state_parameter == 0 && next.control.alternate_counter == 0 &&
                                next.control.queue.empty() && next.control.action_counter == 9 &&
                                !(next.control.flags & (16U | 4U)) && next.encounter == a.encounter,
                            "no object uses real c1 resetting B/C/i/queue but preserving action "
                            "count");
                    } else {
                        const std::vector<LegacyActorControl> queue =
                            kind == ActorKind::human ? std::vector<LegacyActorControl>{{10, 0}}
                            : mode == 0              ? std::vector<LegacyActorControl>{{12}}
                            : mode == 3              ? std::vector<LegacyActorControl>{{13}}
                                                     : std::vector<LegacyActorControl>{};
                        check(next.control.state == a.baseline && next.control.queue == queue &&
                                  next.control.action == 0 && next.control.action_counter == 0 &&
                                  next.control.alternate_counter == 13 &&
                                  next.state_counter == counter && next.state_parameter == 7 &&
                                  next.object_slot == slot &&
                                  (kind == ActorKind::monster || !next.encounter),
                              "carrying b restores human/monster queue without resetting B/C/i/N");
                    }
                }
    auto s = fixture(4);
    s.actors.at({1}).horizontal_velocity = {0.016F, -0.016F};
    const auto r = prepare_world_lifecycle_c(s, input());
    check(r.candidate && r.candidate->state.actors.at({1}).horizontal_velocity.x == 0 &&
              r.candidate->state.actors.at({1}).horizontal_velocity.z == 0 &&
              r.candidate->state.ai.battle.actors.at({1}).position.x == 150,
          "strict abs threshold applies after0.6, before displacement");
}
void win_pickup_and_empty_branches() {
    for (const auto kind : {ActorKind::human, ActorKind::monster})
        for (int counter : {43, 44, 45}) {
            auto s = fixture(10, kind);
            s.ai.battle.actors.at({1}).state_counter = counter;
            const auto r = prepare_world_lifecycle_c(s, input());
            check(r.candidate && r.candidate->restored_baseline == (counter >= 44) &&
                      r.candidate->state.ai.battle.actors.at({1}).state_counter == counter,
                  "win old44 restores b with retained counter for both kinds");
        }
    for (const auto kind : {ActorKind::human, ActorKind::monster})
        for (int counter : {64, 65, 66}) {
            auto s = fixture(12, kind);
            s.ai.battle.actors.at({1}).state_counter = counter;
            const auto r = prepare_world_lifecycle_c(s, input());
            check(r.candidate.has_value(), "pickup completion prepares");
            const auto &a = r.candidate->state.ai.battle.actors.at({1});
            const auto &path = r.candidate->state.actors.at({1});
            if (counter < 65)
                check(a.control.state == 12 &&
                          a.control.queue == s.ai.battle.actors.at({1}).control.queue &&
                          path.journey.has_value(),
                      "pickup old64 does not finish or erase waiting commands");
            else
                check(a.control.state == 0 && a.baseline == 0 && a.state_counter == 0 &&
                          a.control.queue == std::vector<LegacyActorControl>{{8, 0}} &&
                          (a.control.flags & 2U) && !(a.control.flags & 64U) && !path.journey &&
                          !path.unbound_route && !path.path_pending && path.waypoint == 0 &&
                          path.destination == Position{2, 1},
                      "pickup old65 O→c0→b0 clears G/H but retains O, replaces original control "
                      "queue");
        }
    for (int state : {6, 7, 19}) {
        auto s = fixture(state);
        const auto r = prepare_world_lifecycle_c(s, input({{-1, 0, {}}}));
        check(r.candidate && r.candidate->consumed_expressions == 0 &&
                  r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                      s.ai.battle.actors.at({1}).control.queue &&
                  r.candidate->state.ai.battle.actors.at({1}).state_counter == 23 &&
                  r.candidate->state.actors.at({1}).journey.has_value(),
              "no independent c branch still retains real control/path, consumes no unused ticket");
    }
    for (int state : {0, 1, 3, 5, 8, 9, 11, 13, 15, 17, 18, 20}) {
        const auto r = prepare_world_lifecycle_c(fixture(state), input());
        check(!r.candidate && r.error == WorldLifecycleError::unsupported_state,
              "not-yet-routed state is explicit handoff, never empty success");
    }
}
void follow_and_inn() {
    auto s = fixture(16);
    auto carrier = s.ai.battle.actors.at({1});
    carrier.id = {2};
    carrier.position = {275, 8, 325};
    carrier.rescue.reset(); // 前段已经运行；分支不再用R.R重做引用修复。
    s.ai.battle.actors.emplace(carrier.id, carrier);
    s.ai.human_order.push_back(carrier.id);
    s.ai.contexts.emplace(carrier.id, RewardActorContext{{2, 3}, true, {}, {}});
    s.actors.emplace(carrier.id, RescueActorContext{});
    s.ai.battle.actors.at({1}).rescue = carrier.id;
    const auto followed = prepare_world_lifecycle_c(s, input());
    check(
        followed.candidate && !followed.candidate->cleaned_up &&
            followed.candidate->state.ai.battle.actors.at({1}).position.x == 275 &&
            followed.candidate->state.ai.battle.actors.at({1}).position.z == 325 &&
            followed.candidate->state.ai.battle.actors.at({1}).position.height == 24 &&
            followed.candidate->state.ai.battle.actors.at({1}).hp.target == 27 &&
            followed.candidate->state.ai.contexts.at({1}).cell == Position{1, 1} &&
            followed.candidate->state.ai.battle.actors.at({1}).rescue == carrier.id,
        "state16 copies current carrier n+16 without rerunning repair/sensing/HP or projecting s");
    s.ai.human_order.pop_back();
    const auto retired = prepare_world_lifecycle_c(s, input());
    check(retired.candidate && retired.candidate->cleaned_up &&
              retired.candidate->state.ai.battle.actors.at({1}).control.state == 19 &&
              !retired.candidate->state.ai.battle.actors.at({1}).rescue &&
              retired.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 5}} &&
              retired.candidate->state.actors.at({1}).journey.has_value(),
          "carrier absent from bl calls actual r, no straight chase or automatic path clearing");
    for (const auto kind : {ActorKind::human, ActorKind::monster})
        for (int category : {1, 2, 9})
            for (int counter : {169, 170, 171}) {
                s = fixture(14, kind);
                RescueFacility f;
                f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
                f.category = category;
                f.status = 0; // q()无设施运行状态守卫。
                s.facilities.emplace(3, f);
                s.map = *bind_facility_map(s.map, {{f.placement, 3}}).map;
                s.actors.at({1}).binding = ArrivalBinding{{1, 1}, {3}, 33};
                s.ai.battle.actors.at({1}).state_counter = counter;
                const auto r = prepare_world_lifecycle_c(s, input());
                check(r.candidate.has_value(),
                      "state14 valid live q prepares for both actor kinds");
                const auto &hp = r.candidate->state.ai.battle.actors.at({1}).hp;
                const bool recovery = category == 2 && counter == 170;
                check(
                    r.candidate && !r.candidate->cleaned_up && hp.target == (recovery ? 100 : 27) &&
                        hp.displayed == (recovery ? 27 : 25) &&
                        r.candidate->state.ai.battle.actors.at({1}).state_counter == counter,
                    "state14 source has no kind/status guard; only inn exact old170 requests g(h)");
            }
    s = fixture(14, ActorKind::monster);
    const auto cleanup = prepare_world_lifecycle_c(s, input());
    check(cleanup.candidate && cleanup.candidate->cleaned_up &&
              cleanup.candidate->state.ai.battle.actors.at({1}).control.state == 0 &&
              cleanup.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 5}} &&
              cleanup.candidate->state.ai.monster_order == std::vector<CharacterId>{{1}},
          "missing q calls same r for monster too; c0 and queued5, not immediate deletion");
}
void transition_reuse_and_failures() {
    auto s = fixture(4);
    const auto direct = prepare_world_state_transition(s, {{1}, 1, {}});
    auto command_source = s;
    command_source.ai.battle.actors.at({1}).control.queue.insert(
        command_source.ai.battle.actors.at({1}).control.queue.begin(), {2, 1});
    const auto command = prepare_world_state_command(command_source, {{1}, {}});
    check(direct.candidate && command.candidate &&
              direct.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  command.candidate->state.ai.battle.actors.at({1}).control.queue &&
              direct.candidate->state.ai.battle.actors.at({1}).state_counter == 0 &&
              command.candidate->state.ai.battle.actors.at({1}).state_counter == 0 &&
              s.ai.battle.actors.at({1}).control.queue.front()[0] == 1,
          "direct state branch and opcode2 share actual setter without synthetic front command");
    check(!prepare_world_state_transition(s, {{1}, 21, {}}).candidate,
          "out-of-range direct state never silently writes A");
    s.ai.battle.actors.at({1}).id = {2};
    check(prepare_world_lifecycle_c(s, input()).error == WorldLifecycleError::stale_actor,
          "mismatched runtime identity rejected");
    s = fixture(4);
    s.ai.human_order.push_back({1});
    check(prepare_world_lifecycle_c(s, input()).error == WorldLifecycleError::stale_actor,
          "duplicate live roster is not processed twice");
    s = fixture(4);
    s.actors.at({1}).horizontal_velocity.x = std::numeric_limits<float>::infinity();
    check(!prepare_world_lifecycle_c(s, input()).candidate,
          "nonfinite physical velocity cannot create partial position");
    s = fixture(12);
    s.ai.battle.actors.at({1}).control.queue = {{25}};
    check(!prepare_world_lifecycle_c(s, input()).candidate,
          "malformed original queue rejected even when c0 would erase it");
}
void monster_modes() {
    for (int mode : {0, 2, 3})
        for (int counter : {4, 5, 1500})
            for (std::uint32_t flags : {0U, 512U, 1024U})
                for (bool area : {false, true})
                    for (bool enemy_inside : {false, true}) {
                        auto s = fixture(17, ActorKind::monster);
                        auto &a = s.ai.battle.actors.at({1});
                        a.control.flags = flags;
                        a.state_counter = counter;
                        a.perceived_enemy = CharacterId{2};
                        s.actors.at({1}).monster_mode = mode;
                        s.actors.at({1}).outside_updates = 1500;
                        s.ai.contexts.at({1}).move_area = area;
                        auto enemy = a;
                        enemy.id = {2};
                        enemy.kind = ActorKind::human;
                        enemy.position = {999, 0, 999}; // 位置与缓存ax刻意不一致，G仍读ax。
                        s.ai.battle.actors.emplace(enemy.id, enemy);
                        s.ai.human_order.push_back(enemy.id);
                        s.ai.contexts.emplace(enemy.id,
                                              RewardActorContext{{5, 5}, enemy_inside, {}, {}});
                        s.actors.emplace(enemy.id, RescueActorContext{});
                        const auto r = prepare_world_monster_act_c(s, {{1}, WorldPathInput{}});
                        const bool battle = counter >= 5 && !flags && area && enemy_inside;
                        check(r.candidate && r.candidate->called_battle_gate &&
                                  !r.candidate->path && !r.candidate->cleaned_up &&
                                  r.candidate->entered_battle == battle &&
                                  r.candidate->state.ai.battle.actors.at({1}).control.state ==
                                      (battle ? 1 : 17) &&
                                  r.candidate->state.ai.battle.actors.at({1}).state_counter ==
                                      (battle ? 0 : counter) &&
                                  r.candidate->state.ai.battle.actors.at({1}).encounter ==
                                      a.encounter,
                              "T0/2/3 use cached G and actual c1, ignore path input and M1500 "
                              "empty branch");
                    }
    for (int mode : {1, 4})
        for (int counter : {499, 500, 501})
            for (bool category3 : {false, true}) {
                auto s = fixture(17, ActorKind::monster);
                auto &a = s.ai.battle.actors.at({1});
                a.control.flags = 0;
                a.encounter.reset();
                a.group.reset();
                auto &runtime = s.actors.at({1});
                runtime.monster_mode = mode;
                runtime.town_updates = counter;
                runtime.journey.reset();
                runtime.unbound_route.reset();
                runtime.path_pending = false;
                if (category3) {
                    RescueFacility f;
                    f.placement = {
                        {3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
                    f.category = 3;
                    s.facilities.emplace(3, f);
                    s.map = *bind_facility_map(s.map, {{f.placement, 3}}).map;
                    runtime.binding = ArrivalBinding{{1, 1}, {3}, 33};
                    runtime.destination = Position{1, 1};
                    runtime.journey = FacilityDeparture{};
                    runtime.journey->binding = *runtime.binding;
                    runtime.journey->route.steps = {{1, 1}};
                    runtime.waypoint = 0;
                    runtime.path_pending = true;
                }
                WorldPathInput path;
                path.actor = {1};
                path.facts = {
                    s.map, std::vector<int>(36, 1), std::vector<std::uint32_t>(36), {0, 5, 0, 5}};
                const auto r = prepare_world_monster_act_c(s, {{1}, path});
                const bool cleanup = mode == 1 && counter >= 500 && !category3;
                check(
                    r.candidate && !r.candidate->called_battle_gate && r.candidate->path &&
                        r.candidate->path->path_returned_true == category3 &&
                        !r.candidate->path->delete_instance && r.candidate->cleaned_up == cleanup &&
                        r.candidate->state.ai.battle.actors.at({1}).control.state ==
                            (cleanup ? 0 : 17) &&
                        r.candidate->state.ai.monster_order == std::vector<CharacterId>{{1}},
                    "T1/4 run real P; only T1 false with oldL500 r; Ptrue never c-deletes state17");
                check(r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                          (cleanup     ? std::vector<LegacyActorControl>{{8, 5}}
                           : category3 ? std::vector<LegacyActorControl>{}
                                       : a.control.queue),
                      "post-P cleanup replaces queue, true category3 retains P queue clearing, "
                      "falseT4 retains original");
                const auto missing = prepare_world_monster_act_c(s, {{1}, {}});
                check(!missing.candidate && missing.error == WorldLifecycleError::invalid_input,
                      "path modes cannot accept fabricated false/default path consumer");
                path.facts.map.width = 7;
                const auto failed = prepare_world_monster_act_c(s, {{1}, path});
                check(!failed.candidate && s.ai.battle.actors.at({1}).control.state == 17 &&
                          s.actors.at({1}).town_updates == counter,
                      "real P failure does not cleanup or expose a partial monster world");
            }
}
std::optional<WorldScheduleStep> scheduled_consumer(const WorldScheduleState &s,
                                                    const WorldScheduleCall &call,
                                                    const CombatInfluenceCandidate &) {
    WorldScheduleStep next{s};
    if (call.stage == WorldScheduleStage::decision) {
        const auto r = prepare_world_lifecycle_c(s.world, {{*call.id}, {{999, 1, {}}, {0, 1, 0}}});
        if (!r.candidate)
            return {};
        next.state.world = r.candidate->state;
    } else if (call.stage == WorldScheduleStage::control) {
        WorldControlAdapter<WorldScheduleState> adapter;
        adapter.read = [](const WorldScheduleState &owner, CharacterId id) {
            return &owner.world.ai.battle.actors.at(id).control;
        };
        adapter.write = [](WorldScheduleState &owner, CharacterId id,
                           const ActorControlState &control) {
            owner.world.ai.battle.actors.at(id).control = control;
            return true;
        };
        const auto r = prepare_world_control(s, {*call.id}, adapter);
        if (!r.candidate)
            return {};
        next.state = r.candidate->state;
    } else if (call.stage == WorldScheduleStage::finalize) {
        const auto r = prepare_world_schedule_overlap(s, {});
        if (!r)
            return {};
        next.state = *r;
    } else if (call.stage != WorldScheduleStage::arrival_front) {
        return {}; // 本夹具无其他域；出现真实请求则拒绝，不用空成功掩盖缺消费者。
    }
    return next;
}
void common_rounds() {
    WorldScheduleState s;
    s.world = fixture(2);
    auto &a = s.world.ai.battle.actors.at({1});
    a.baseline = 0;
    a.control.flags = 0;
    a.control.queue.clear();
    a.encounter.reset();
    a.group.reset();
    a.state_counter = 899;
    a.physics_pause = 10;
    s.world.actors.at({1}).journey.reset();
    s.world.actors.at({1}).unbound_route.reset();
    s.world.actors.at({1}).path_pending = false;
    s.world.actors.at({1}).waypoint = 0;
    s.surface.assign(36, 1);
    s.map_flags.assign(36, 0);
    s.town = {0, 5, 0, 5};
    const auto first = prepare_world_schedule(s, {}, scheduled_consumer);
    check(first.candidate &&
              first.candidate->state.world.ai.battle.actors.at({1}).control.state == 2 &&
              first.candidate->state.world.ai.battle.actors.at({1}).state_counter == 900 &&
              first.candidate->state.world.ai.battle.actors.at({1}).hp.target == 26,
          "common world c reads899 then one d advances900, not early restoration");
    const auto second = prepare_world_schedule(first.candidate->state, {}, scheduled_consumer);
    check(second.candidate &&
              second.candidate->state.world.ai.battle.actors.at({1}).control.state == 0 &&
              second.candidate->state.world.ai.battle.actors.at({1}).state_counter == 901 &&
              second.candidate->state.world.ai.battle.actors.at({1}).hp.target == 100 &&
              second.candidate->state.world.ai.contexts.at({1}).effects.display ==
                  std::vector<ActorEffectRecord>{{12, 1, 30, 4, 0}},
          "next c old900 restores b and expression4; same round d advances new cd once and B901");
    const auto third = prepare_world_schedule(second.candidate->state, {}, scheduled_consumer);
    check(!third.candidate && third.error == WorldScheduleError::consumer_failed &&
              second.candidate->state.world.ai.battle.actors.at({1}).state_counter == 901,
          "unconnected next state0 refuses whole next round instead of pretending closed world");
    s.world.ai.battle.actors.at({1}).control.state = 10;
    s.world.ai.battle.actors.at({1}).baseline = 6;
    s.world.ai.battle.actors.at({1}).state_counter = 43;
    s.world.ai.battle.actors.at({1}).hp.animating = false;
    s.world.ai.battle.actors.at({1}).hp.legacy_tick = 30;
    s.world.ai.battle.actors.at({1}).position.height = 0;
    s.world.ai.battle.actors.at({1}).vertical_velocity = 0;
    s.world.actors.at({1}).horizontal_velocity = {};
    for (int round = 0; round < 20; ++round) {
        const auto next = prepare_world_schedule(s, {}, scheduled_consumer);
        check(next.candidate &&
                  next.candidate->state.world.ai.battle.actors.at({1}).control.state ==
                      (round == 0 ? 10 : 6) &&
                  next.candidate->state.world.ai.battle.actors.at({1}).state_counter ==
                      44 + round &&
                  next.candidate->state.updates == round + 1 &&
                  next.candidate->state.world.ai.human_order == std::vector<CharacterId>{{1}},
              "twenty actual common rounds preserve win→baseline6 and one B/p advance per round");
        s = next.candidate->state;
    }
}
} // namespace
int main() {
    try {
        down_recovery();
        knockback_and_baseline();
        win_pickup_and_empty_branches();
        follow_and_inn();
        transition_reuse_and_failures();
        monster_modes();
        common_rounds();
        std::cout << checks << " world lifecycle checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
