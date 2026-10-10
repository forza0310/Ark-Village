#include "ark/simulation/village/rules/world_daily.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
WorldDailyState fixture(int state) {
    WorldDailyState s;
    s.world.map = {8, 8, std::vector<LegacyMapCell>(64)};
    for (auto &cell : s.world.map.cells) {
        cell.legacy_state = 4;
        cell.category = RouteCategory::road;
    }
    s.facts = {s.world.map, std::vector<int>(64, 1), std::vector<std::uint32_t>(64), {0, 3, 0, 3}};
    BattleActorRecord a;
    a.id = {1};
    a.control.state = state;
    a.control.flags = 2;
    a.control.queue = {{1, 9, 0}};
    a.capacity = 100;
    a.hp = {0, 100, 100, 100, false, 0};
    a.position = {550, 0, 550};
    a.baseline = 5;
    a.state_counter = 10;
    s.world.ai.battle.actors.emplace(a.id, a);
    s.world.ai.human_order.push_back(a.id);
    s.world.ai.contexts.emplace(a.id, RewardActorContext{{5, 5}, false, {}, {}, true, {11, 11}});
    s.world.actors.emplace(a.id, RescueActorContext{});
    s.world.actors.at(a.id).destination = Position{};
    s.world.ai.growth[0].definition.legacy_u = 50;
    return s;
}
WorldDailyInput input() {
    WorldDailyInput i;
    i.actor = {1};
    i.expressions = {{999, 4, {}}, {999, 3, {}}};
    i.spawn_ticket = 999;
    return i;
}
void idle_priorities() {
    auto s = fixture(5);
    auto i = input();
    auto r = prepare_world_daily_c(s, i);
    check(r.candidate && r.candidate->consumed_expressions == 1 && r.candidate->consumed_spawn &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 5 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                  s.world.ai.battle.actors.at({1}).control.queue,
          "ordinary outwalk expression8 precedes actual L while idle queue remains");
    s.world.ai.battle.actors.at({1}).control.flags |= 16;
    i.spawn_ticket.reset();
    r = prepare_world_daily_c(s, i);
    check(r.candidate && r.candidate->consumed_expressions == 1 && !r.candidate->consumed_spawn,
          "flag16 still consumes expression8 before early return, no L");
    s = fixture(5);
    s.world.ai.task_active = true;
    s.world.actors.at({1}).definition_task_flag = true;
    s.world.ai.battle.actors.at({1}).rescue = CharacterId{9};
    r = prepare_world_daily_c(s, input());
    check(r.candidate && !r.candidate->consumed_spawn &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 0 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 1}},
          "active task/definition2 beats F and carry, queues activity1 with real c0");
    s.world.ai.task_active = false;
    s.world.actors.at({1}).journey = FacilityDeparture{};
    r = prepare_world_daily_c(s, input());
    check(r.candidate && !r.candidate->state.world.actors.at({1}).journey &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 4}} &&
              !r.candidate->consumed_spawn,
          "carry branch clears path, c0 and queued4, no later reselection or L");
    s = fixture(5);
    RewardEncounter e;
    e.runtime.id = 3;
    e.legacy_id = 3;
    e.runtime.center = {7, 7};
    s.world.ai.encounters.emplace(3, e);
    s.world.ai.encounter_order = {3};
    s.world.actors.at({1}).outside_updates = 100;
    s.world.ai.battle.actors.at({1}).hp.target = 20;
    r = prepare_world_daily_c(s, input());
    check(
        r.candidate &&
            r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                std::vector<LegacyActorControl>{{8, 0}} &&
            r.candidate->consumed_spawn,
        "nearby activity6 is replaced by later lowHP c0/activity0; resulting state0 L still draws");
    s.world.ai.battle.actors.at({1}).hp.target = 100;
    r = prepare_world_daily_c(s, input());
    check(r.candidate && r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                             std::vector<LegacyActorControl>{{8, 6}},
          "nearby event at100 keeps queued6 when no lower HP/timer reselection");
    s.world.actors.at({1}).outside_updates = 101;
    r = prepare_world_daily_c(s, input());
    check(r.candidate && r.candidate->state.world.ai.battle.actors.at({1}).control.state == 5,
          "nearby event not checked off M100 boundary");
}
void path_and_spawn_guards() {
    auto s = fixture(0);
    auto i = input();
    WorldPathInput path;
    path.actor = {1};
    path.facts = s.facts;
    i.path = path;
    const auto r = prepare_world_daily_c(s, i);
    check(r.candidate && r.candidate->consumed_spawn && r.candidate->path &&
              !r.candidate->path->arrived && !r.candidate->spawn &&
              r.candidate->state.world.ai.battle.actors.at({1}).state_counter == 10,
          "state0 actual L draws even probability0 then P empty route without counters");
    i.spawn_ticket.reset();
    const auto missing = prepare_world_daily_c(s, i);
    check(!missing.candidate && missing.error == WorldDailyError::missing_ticket,
          "state0 probability0 is not permission to skip actual L random draw");
    s.world.actors.at({1}).destination = Position{1, 1};
    const auto bypass = prepare_world_daily_c(s, i);
    check(bypass.candidate && !bypass.candidate->consumed_spawn && bypass.candidate->path,
          "destination inside actual town suppresses L before RNG, not cached ax");
    s = fixture(5);
    i = input();
    i.spawn_ticket = 0;
    EncounterCreationInput creation;
    creation.year_index = 0;
    creation.month_index = 0;
    i.spawn_creation = creation;
    i.task_centers = {{5, 5}};
    const auto denial = prepare_world_daily_c(s, i);
    check(denial.candidate && denial.candidate->consumed_spawn && denial.candidate->spawn &&
              denial.candidate->spawn->denial == EncounterCreationDenial::task_overlap &&
              denial.candidate->state.world.ai.encounter_order.empty(),
          "L success real k.a task overlap denies before count/definition random");
}
void appearance() {
    for (int state : {8, 9})
        for (int counter : {0, 55, 60, 65, 72, 73, 74})
            for (int mode = 0; mode <= 4; ++mode) {
                auto s = fixture(state);
                auto &a = s.world.ai.battle.actors.at({1});
                a.kind = ActorKind::monster;
                a.state_counter = counter;
                a.control.flags |= 1;
                s.world.ai.human_order.clear();
                s.world.ai.monster_order = {{1}};
                s.world.actors.at({1}).monster_mode = mode;
                auto i = input();
                i.event = [](const RescueWorldState &world,
                             int event) -> std::optional<RescueWorldState> {
                    if (event != 90 ||
                        (world.ai.battle.actors.at({1}).control.state != 8 &&
                         world.ai.battle.actors.at({1}).control.state != 9) ||
                        (world.ai.battle.actors.at({1}).control.flags & 1U))
                        return {};
                    auto next = world;
                    next.ai.battle.events.insert(event);
                    return next; // 显式同步事件夹具；不称真实脚本/窗口验收。
                };
                const auto r = prepare_world_daily_c(s, i);
                check(r.candidate.has_value(),
                      "appearance candidate prepares all states/modes/boundaries");
                const auto &next = r.candidate->state.world.ai.battle.actors.at({1});
                check(next.attack_position.x == 550 && next.attack_position.z == 550 &&
                          next.attack_position.height >= 0 && next.position.height == 0 &&
                          r.candidate->ground_effect21 == (counter == 65) &&
                          r.candidate->state.world.ai.battle.events.count(90) ==
                              (counter >= 73 ? 1U : 0U),
                      "au landing arc and exact B65 ground effect independent from n; event at "
                      "old73 only");
                const std::vector<LegacyActorControl> queue =
                    counter < 73 ? a.control.queue
                    : mode == 0  ? std::vector<LegacyActorControl>{{12}}
                    : mode == 1  ? std::vector<LegacyActorControl>{{8, 7}}
                    : mode == 3  ? std::vector<LegacyActorControl>{{13}}
                    : mode == 4  ? std::vector<LegacyActorControl>{{8, 8}}
                                 : std::vector<LegacyActorControl>{};
                check(next.control.queue == queue &&
                          next.control.state == (counter >= 73 ? 17 : state) &&
                          next.state_counter == (counter >= 73 ? 0 : counter),
                      "synchronous event90 before real c17 and original mode-dependent queue");
            }
    auto s = fixture(8);
    s.world.ai.battle.actors.at({1}).state_counter = 73;
    const auto missing = prepare_world_daily_c(s, input());
    check(!missing.candidate && missing.error == WorldDailyError::missing_consumer &&
              s.world.ai.battle.actors.at({1}).control.state == 8,
          "event90 missing actual consumer rolls back arc/flags/state");
    s.world.ai.battle.events.insert(90);
    const auto seen = prepare_world_daily_c(s, input());
    check(seen.candidate &&
              seen.candidate->state.world.ai.battle.actors.at({1}).control.state == 17,
          "already-seen event90 needs no new consumer or duplicate call");
}
void pickup_and_event_priority() {
    auto s = fixture(11);
    auto object = *prepare_ground_drop({4}, {650, 0, 550}, 0, 2);
    object.state = 3;
    s.world.ai.battle.objects.emplace(4, object);
    s.world.object_order = {4};
    auto i = input();
    i.actor_box = CollisionBox{-4, 4, 8, 8};
    i.object_box = CollisionBox{-4, 4, 8, 8};
    auto r = prepare_world_daily_c(s, i);
    check(r.candidate && r.candidate->pickup &&
              r.candidate->pickup->action == PickupAction::chase &&
              std::abs(r.candidate->state.world.ai.battle.actors.at({1}).position.x - 556.7F) <
                  0.0001F &&
              r.candidate->state.world.ai.contexts.at({1}).cell == Position{5, 5} &&
              r.candidate->state.world.actors.at({1}).horizontal_velocity.x == 6.7F,
          "fresh H actual boxes chase by6.7 with r/n, no grid cache refresh or instant pickup");
    s.world.ai.battle.objects.at(4).position.x = 550;
    r = prepare_world_daily_c(s, i);
    check(r.candidate && r.candidate->pickup->action == PickupAction::pickup &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 12 &&
              r.candidate->state.world.ai.battle.objects.at(4).state == 5 &&
              r.candidate->state.world.ai.battle.actors.at({1}).object_slot == -1,
          "contact commits object5/c12/real pickup queue, not inventory grant or N slot");
    s.world.object_order.clear();
    s.world.ai.battle.objects.clear();
    r = prepare_world_daily_c(s, input());
    check(r.candidate && r.candidate->pickup->action == PickupAction::baseline &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{10, 0}} &&
              r.candidate->state.world.ai.battle.actors.at({1}).state_counter == 10,
          "no H calls b, preserving B and queuing actual baseline wander");
}
} // namespace
int main() {
    try {
        idle_priorities();
        path_and_spawn_guards();
        appearance();
        pickup_and_event_priority();
        std::cout << checks << " world daily checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
