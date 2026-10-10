#include "ark/simulation/world/startup_world_runtime.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
StartupWorldRuntimeState initial(ref::WorldRandomStream random) {
    StartupSession original;
    StartupWorldRuntimeSession world(original.state(), std::move(random));
    return world.state();
}
void first() {
    auto s = initial(ref::WorldRandomStream::from_raw({149, 0, 1}));
    const auto adapter = startup_world_runtime_adapter();
    check(adapter.arrival != nullptr && s.arrival_counter == 420 &&
              s.scene.world.world.ai.human_order.empty() && s.actor_metadata.empty(),
          "source new game420 and empty actual roster");
    for (int i = 0; i < 419; ++i) {
        const auto next = adapter.arrival(s);
        check(next && next->arrival_counter == 419 - i && next->scene.random.draws() == 0 &&
                  next->scene.world.world.ai.human_order.empty(),
              "first419 admitted arrival calls only decrement true B, no random/actor");
        s = *next;
    }
    const auto before = s;
    const auto arrived = adapter.arrival(s);
    check(arrived && arrived->scene.world.world.ai.human_order.size() == 1,
          "420th admitted source arrival creates actual Character");
    const auto &n = *arrived;
    const auto id = n.scene.world.world.ai.human_order.front();
    const auto &actor = n.scene.world.world.ai.battle.actors.at(id);
    const auto &expected = startup_evidence().first_character;
    const auto spawn = startup_evidence().spawn_points.at(1);
    check(actor.definition == expected.definition_id && actor.legacy_id == 0 &&
              actor.hp.target == expected.hp[2] && actor.hp.origin == expected.hp[1] &&
              actor.hp.displayed == expected.hp[0] && actor.capacity == expected.combat[0] &&
              actor.control.flags == (2U | 8192U) && actor.control.state == 0 &&
              actor.control.queue == std::vector<ref::LegacyActorControl>{{8, 0}},
          "true first definition/UID0/HP22/flags/queued activity from source");
    const auto &cache = n.scene.world.world.ai.contexts.at(id);
    const auto &metadata = n.actor_metadata.at(id);
    check(
        cache.cell == spawn && cache.half_cell == ref::Position{spawn.x * 2 + 1, spawn.y * 2 + 1} &&
            actor.position.x == spawn.x * 100.0F + 50.0F &&
            actor.position.z == spawn.y * 100.0F + 50.0F &&
              actor.attack_position.x == 0 && actor.attack_position.height == 0 &&
              actor.attack_position.z == 0 &&
            n.scene.world.world.actors.at(id).destination == std::optional<ref::Position>{{0, 0}} &&
            !cache.inside_town && !cache.move_area && !actor.attack_armed &&
            actor.combo_count == 0 && actor.perceived_distance == 0,
          "real n/s/t/u birth center, constructorau stays zero before real action8/9 d");
    check(metadata.sex == expected.sex && metadata.profession == expected.job_id &&
              metadata.weapon == expected.equipment[0] &&
              metadata.cached_view.x ==
                  static_cast<int>(actor.position.x * 0.3F + actor.position.z * 0.3F) &&
              metadata.cached_view.y ==
                  static_cast<int>(actor.position.x * -0.15F + actor.position.z * 0.15F) &&
              n.shop_actors.at(id).weapon == expected.equipment[0] &&
              n.shop_humans.at(expected.definition_id).reselect[0] == 6 &&
              n.scene.world.world.ai.growth.at(expected.definition_id).derived.combat ==
                  expected.combat,
          "actual profession/equipment/shared A0 and unshifted view projection");
    check(n.arrival_counter == 249 && n.scene.random.draws() == 3 && n.event89_count == 1 &&
              ref::world_script_seen(n.scripts, 89) && n.scene.scene_state == 6 &&
              n.scene.scene_counter == 0 && n.scripts.selected_actor == id.value &&
              n.scripts.pages.size() == 2 && n.scripts.pages.back().source_record == 69 &&
              n.scene.world.world.ai.accounting.funds() == 5000,
          "reset/top3/spawn2 unified draws, actual89 talk69/scene6, first is free");
    check(before.arrival_counter == 1 && before.scene.random.draws() == 0 &&
              before.scene.world.world.ai.human_order.empty() && before.scripts.pages.size() == 1,
          "input unique Owner unchanged by candidate creation/script");
}
void initialization_order() {
    auto s = initial(ref::WorldRandomStream::from_raw({0, 0, 0}));
    s.arrival_counter = 1;
    auto &human = s.scene.world.world.ai.growth.at(1);
    const auto old_hp = human.derived.combat[0];
    human.definition.extra[0] = 10; // 显式时序夹具，不替换真实新局数据。
    human.definition.learned_spells[0] = true;
    check(!human.derived.available_spells[0],
          "fixture has stale derivedQ before source equipment refresh");
    const auto adapter = startup_world_runtime_adapter();
    const auto arrived = adapter.arrival(s);
    check(arrived.has_value(), "real creation consumes fixture's changed shared base");
    const auto &n = *arrived;
    const auto id = n.scene.world.world.ai.human_order.front();
    const auto &actor = n.scene.world.world.ai.battle.actors.at(id);
    const auto &updated = n.scene.world.world.ai.growth.at(1);
    check(actor.hp.target == old_hp && actor.hp.origin == old_hp && actor.hp.displayed == old_hp &&
              actor.capacity == updated.derived.combat[0] && actor.capacity > old_hp &&
              updated.derived.available_spells[0] && ref::world_script_seen(n.scripts, 218) &&
              n.scripts.continuations.size() == 1 && n.scripts.continuations.front().event == 218 &&
              n.scripts.continuations.front().remaining_updates == 300,
          "HP before shared derived/Q refresh; actual218 waits300 after actual89");
    auto unarmed = s;
    unarmed.shop_humans.at(1).equipment[0].reset();
    const auto no_weapon = adapter.arrival(unarmed);
    check(no_weapon &&
              no_weapon->actor_metadata.at(no_weapon->scene.world.world.ai.human_order.front())
                      .weapon == -1 &&
              !no_weapon->scene.world.world.ai.growth.at(1).definition.equipment[0] &&
              no_weapon->shop_humans.at(1).reselect[0] == 6,
          "source v0=-1 supported, shared equipment cleared, not invented weapon0");
}
void same_round() {
    auto s = initial(ref::WorldRandomStream::from_java_seed(0));
    s.arrival_counter = 1;
    const auto adapter = startup_world_runtime_adapter();
    auto actors = adapter.actors;
    bool first_control_prefix{};
    const auto actual_command = actors.owned_command;
    actors.owned_command = [&](const auto &owner, const auto &routes, auto id,
                               const auto &command) {
        const auto &actor = routes.world.ai.battle.actors.at(id);
        if (command == ref::LegacyActorControl{8, 0})
            first_control_prefix = actor.control.action_counter == 1 && actor.state_counter == 1;
        return actual_command(owner, routes, id, command);
    };
    actors.other = [&](const auto &owner, const auto &call, const auto &field) {
        return ref::prepare_owned_world_runtime_domain(owner, call, field, adapter);
    };
    const auto run = ref::prepare_world_actor_schedule(s, {true}, actors);
    check(run.state.has_value(), "true arrival/c/d/facility/final shared world round completes");
    const auto &n = *run.state;
    const auto id = n.scene.world.world.ai.human_order.front();
    check(run.decisions.size() == 1 && run.controls.size() == 1 && run.audit &&
              run.audit->effects.size() == 1 && first_control_prefix &&
              n.scene.world.world.ai.contexts.at(id).move_area && n.scene.scene_state == 6 &&
              n.scripts.pages.back().source_record == 69,
          "real same-round appended actor gets c/d once despite89 setting next frame scene6");
    check(s.scene.world.world.ai.human_order.empty() && s.arrival_counter == 1 &&
              s.scene.random.draws() == 0,
          "full same-round candidate still preserves original Owner/random");
}
void rollback() {
    auto s = initial(ref::WorldRandomStream::from_raw({0, 0}));
    s.arrival_counter = 1;
    const auto adapter = startup_world_runtime_adapter();
    check(!adapter.arrival(s) && s.scene.world.world.ai.human_order.empty() &&
              s.scene.random.draws() == 0 && s.arrival_counter == 1 && s.actor_metadata.empty(),
          "missing spawn random rolls back allocation/shared stats/metadata/page/B");
    s.scene.random = ref::WorldRandomStream::from_raw({0, 0, 0});
    auto bad_catalog = adapter;
    bad_catalog.catalog.events.erase(89);
    configure_startup_world_runtime_arrival_adapter(bad_catalog);
    check(!bad_catalog.arrival(s) && s.scene.world.world.ai.human_order.empty() &&
              s.actor_metadata.empty() && s.scripts.pages.size() == 1 &&
              s.shop_humans.at(1).reselect[0] == 0,
          "missing actual89 after true creation rejects private Owner without metadata/equip "
          "residue");
}
} // namespace
int main() {
    try {
        first();
        initialization_order();
        same_round();
        rollback();
        std::cout << "startup world arrival checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
