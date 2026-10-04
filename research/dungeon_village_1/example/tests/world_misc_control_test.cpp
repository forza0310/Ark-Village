#include "dungeon_village_reference/world_actor_tail.hpp"
#include "dungeon_village_reference/world_facilities.hpp"
#include "dungeon_village_reference/world_misc_control.hpp"

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
WorldMiscControlState fixture() {
    WorldMiscControlState s;
    s.world.map = {4, 4, std::vector<LegacyMapCell>(16)};
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.occupants = {{1}, {1}};
    s.world.facilities.emplace(3, f);
    s.world.map = *bind_facility_map(s.world.map, {{f.placement, 3}}).map;
    for (CharacterId id : {CharacterId{1}, CharacterId{2}}) {
        BattleActorRecord a;
        a.id = id;
        a.definition = 9;
        a.control.action = 6;
        a.control.action_counter = 8;
        a.control.alternate_counter = 7;
        a.state_counter = 23;
        a.capacity = 100;
        a.hp = {0, 25, 25, 25, false, 0};
        a.position = {150, 10, 150};
        a.vertical_velocity = -3;
        a.object_slot = 7;
        s.world.ai.battle.actors.emplace(id, a);
        s.world.ai.human_order.push_back(id);
        s.world.ai.contexts.emplace(id, RewardActorContext{{1, 1}, true, {}, {}});
        s.world.actors.emplace(id, RescueActorContext{});
        s.world.actors.at(id).binding = ArrivalBinding{{1, 1}, {3}, 33};
    }
    s.human_definition_state.emplace(9, 2);
    return s;
}
WorldMiscControlInput input() {
    return {{1}, Position{120, 45}, [](Position old_view) -> std::optional<Position> {
                return Position{old_view.x + 7, 900 - old_view.y};
            }}; // 镜头/屏幕算式为明确夹具；核心不把这组数值当原版初始镜头。
}
void hp_and_visuals() {
    for (int target = -5; target <= 120; ++target)
        for (int amount : {-20, 0, 1, 20, 100})
            for (int state : {0, 2}) {
                auto s = fixture();
                auto &a = s.world.ai.battle.actors.at({1});
                a.control.state = state;
                a.control.queue = {{25, amount}, {33}};
                a.hp = {0, 9, 7, target, true, 4};
                const auto r = prepare_world_misc_control(s, input());
                const int hp_target = amount > 0 ? std::min(target + amount, 100) : target + amount;
                const int counter =
                    state == 2 ? std::max(23, std::clamp(hp_target, 0, 100) * 900 / 100) : 23;
                check(
                    r.candidate && r.candidate->action == WorldControlAction::continue_same_call &&
                        r.candidate->state.world.ai.battle.actors.at({1}).hp.target == hp_target &&
                        r.candidate->state.world.ai.battle.actors.at({1}).hp.displayed == target &&
                        r.candidate->state.world.ai.battle.actors.at({1}).hp.origin == target &&
                        r.candidate->state.world.ai.battle.actors.at({1}).hp.requested_delta ==
                            amount &&
                        r.candidate->state.world.ai.battle.actors.at({1}).hp.legacy_tick == 0 &&
                        r.candidate->state.world.ai.battle.actors.at({1}).state_counter == counter,
                    "25 real signed HP protocol and down progress use old target, not display");
                check(r.candidate->state.world.ai.contexts.at({1}).effects.display ==
                              std::vector<ActorEffectRecord>{{2, 0, amount, 160, -26, 2},
                                                             {6, 0, 120, 45}} &&
                          r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                              std::vector<LegacyActorControl>{{33}} &&
                          r.candidate->sounds.empty(),
                      "25 appends numeric healing then source at OLD u, no sound/random/current n");
            }
    auto s = fixture();
    auto &a = s.world.ai.battle.actors.at({1});
    a.control.state = 2;
    a.state_counter = 899;
    a.control.queue = {{25, 1}};
    const auto r = prepare_world_misc_control(s, input());
    check(r.candidate && r.candidate->state.world.ai.battle.actors.at({1}).state_counter == 899 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 2 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.action == 6,
          "heal never lowers old down B or changes A/k, even if animation is not a down action");
}
void jump_sound_and_delete() {
    auto s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{32}, {33}};
    const auto jump = prepare_world_misc_control(s, {{1}, {}, {}});
    check(jump.candidate && jump.candidate->action == WorldControlAction::continue_same_call &&
              std::abs(jump.candidate->state.world.ai.battle.actors.at({1}).vertical_velocity -
                       28.0F / 6.0F) < 0.00001F &&
              jump.candidate->state.world.ai.battle.actors.at({1}).position.height == 10 &&
              jump.candidate->state.world.ai.battle.actors.at({1}).control.action_counter == 8 &&
              jump.candidate->state.world.ai.battle.actors.at({1}).control.queue.front()[0] == 33,
          "32 sets real aN only, no immediate physics/state/action or common counter advance");
    const auto sound = prepare_world_misc_control(jump.candidate->state, input());
    check(sound.candidate && sound.candidate->sounds.size() == 1 &&
              sound.candidate->sounds.front().sound == 8 &&
              sound.candidate->sounds.front().position == Position{127, 855} &&
              sound.candidate->state.world.ai.battle.actors.at({1}).object_slot == 7 &&
              sound.candidate->state.world.ai.battle.objects.empty() &&
              sound.candidate->state.world.ai.contexts.at({1}).effects.display.empty(),
          "33 uses supplied current camera projection of old u, no item grant/clear/effect");
    s.world.ai.battle.actors.at({1}).control.queue = {{26}, {25, 99}};
    const auto remove = prepare_world_misc_control(s, {{1}, {}, {}});
    check(remove.candidate && remove.candidate->action == WorldControlAction::delete_true &&
              remove.candidate->state.human_definition_state.at(9) == 1 &&
              remove.candidate->state.world.ai.human_order.size() == 2 &&
              remove.candidate->state.world.facilities.at(3).occupants.size() == 2 &&
              remove.candidate->state.world.ai.battle.actors.at({1}).control.state == 0 &&
              remove.candidate->state.world.ai.battle.actors.at({1}).control.queue.front()[0] == 25,
          "26 writes shared definition m then requests true, no eager release/remove/c0/tail");
    const auto erased = prepare_world_actor_remove(remove.candidate->state.world, {1}, true);
    check(erased.candidate &&
              erased.candidate->state.ai.human_order == std::vector<CharacterId>{{2}} &&
              erased.candidate->state.facilities.at(3).occupants == std::vector<CharacterId>{{1}} &&
              erased.candidate->state.ai.retired_actors.at({1}).state_counter == 23 &&
              erased.candidate->state.ai.battle.actors.at({2}).control.state == 0 &&
              remove.candidate->state.human_definition_state.at(9) == 1,
          "source schedule deletion releases first q reference and retires separately from m1");
}
struct Harness {
    WorldMiscControlState misc;
    std::vector<WorldMiscSoundRequest> sounds;
};
WorldControlAdapter<Harness> adapter(bool project = true) {
    WorldControlAdapter<Harness> a;
    a.read = [](const Harness &s, CharacterId id) -> const ActorControlState * {
        const auto actor = s.misc.world.ai.battle.actors.find(id);
        return actor == s.misc.world.ai.battle.actors.end() ? nullptr : &actor->second.control;
    };
    a.write = [](Harness &s, CharacterId id, const ActorControlState &control) {
        const auto actor = s.misc.world.ai.battle.actors.find(id);
        if (actor == s.misc.world.ai.battle.actors.end())
            return false;
        actor->second.control = control;
        return true;
    };
    a.domain = [project](const Harness &s,
                         CharacterId id) -> std::optional<WorldControlStep<Harness>> {
        auto i = input();
        i.actor = id;
        if (!project)
            i.sound_projection = {};
        const auto r = prepare_world_misc_control(s.misc, i);
        if (!r.candidate)
            return {};
        auto next = s;
        next.misc = r.candidate->state;
        next.sounds.insert(next.sounds.end(), r.candidate->sounds.begin(),
                           r.candidate->sounds.end());
        return WorldControlStep<Harness>{std::move(next), r.candidate->action};
    };
    return a;
}
void actual_fifo() {
    Harness s{fixture(), {}};
    s.misc.world.ai.battle.actors.at({1}).control.queue = {{25, 25}, {32}, {33}, {26}, {25, 20}};
    const auto r = prepare_world_control(s, {1}, adapter());
    check(r.candidate && r.candidate->flow == WorldControlFlow::delete_requested &&
              r.candidate->domain_segments == 4 && r.candidate->state.sounds.size() == 1 &&
              r.candidate->state.misc.world.ai.battle.actors.at({1}).hp.target == 50 &&
              r.candidate->state.misc.world.ai.battle.actors.at({1}).state_counter == 23 &&
              r.candidate->state.misc.world.ai.battle.actors.at({1}).position.height == 10 &&
              r.candidate->state.misc.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{25, 20}},
          "actual25/32/33 continue same v,26 stops true before heal tail and all d tail");
    s.misc.world.ai.battle.actors.at({1}).control.queue = {{25, 25}, {1, 2, 0}, {32}};
    const auto hold = prepare_world_control(s, {1}, adapter());
    check(hold.candidate && hold.candidate->flow == WorldControlFlow::held &&
              hold.candidate->state.misc.world.ai.battle.actors.at({1}).control.queue.front()[1] ==
                  1 &&
              hold.candidate->state.misc.world.ai.battle.actors.at({1}).vertical_velocity == -3,
          "actual heal then local positive wait holds before32, no double decrement");
    s.misc.world.ai.battle.actors.at({1}).control.queue = {{25, 25}, {32}, {33}, {26}};
    const auto failed = prepare_world_control(s, {1}, adapter(false));
    check(failed.error == WorldControlError::consumer_failed && !failed.candidate &&
              s.misc.world.ai.battle.actors.at({1}).hp.target == 25 &&
              s.misc.world.ai.battle.actors.at({1}).vertical_velocity == -3 &&
              s.misc.human_definition_state.at(9) == 2 && s.sounds.empty(),
          "late missing33 projection rolls back preceding real25/32 and no external sound");

    s = {fixture(), {}};
    auto &monster = s.misc.world.ai.battle.actors.at({1});
    monster.kind = ActorKind::monster;
    s.misc.world.ai.human_order.erase(s.misc.world.ai.human_order.begin());
    s.misc.world.ai.monster_order = {{1}};
    monster.control.queue = {{0, 150, 150}, {26}, {25, 20}};
    auto exit_adapter = adapter();
    const auto misc_consumer = exit_adapter.domain;
    exit_adapter.domain =
        [misc_consumer](const Harness &w,
                        CharacterId id) -> std::optional<WorldControlStep<Harness>> {
        if (w.misc.world.ai.battle.actors.at(id).control.queue.front()[0] != 0)
            return misc_consumer(w, id);
        const auto motion = prepare_world_facility_control(w.misc.world, {id, {}, {}});
        if (!motion.candidate)
            return {};
        auto next = w;
        next.misc.world = motion.candidate->state;
        return WorldControlStep<Harness>{next, motion.candidate->flow == ActorControlFlow::moving
                                                   ? WorldControlAction::hold_false
                                                   : WorldControlAction::continue_same_call};
    };
    const auto exit = prepare_world_control(s, {1}, exit_adapter);
    check(exit.candidate && exit.candidate->flow == WorldControlFlow::delete_requested &&
              exit.candidate->domain_segments == 2 &&
              exit.candidate->state.misc.human_definition_state.at(9) == 1 &&
              exit.candidate->state.misc.world.ai.monster_order == std::vector<CharacterId>{{1}} &&
              exit.candidate->state.misc.world.facilities.at(3).occupants ==
                  std::vector<CharacterId>{{1}, {1}} &&
              exit.candidate->state.misc.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{25, 20}},
          "real monster ground exit0 then26 writes shared bv m1, skips tail, scheduler owns "
          "deletion");
    s.misc.human_definition_state.clear();
    check(!prepare_world_control(s, {1}, exit_adapter).candidate &&
              s.misc.world.ai.battle.actors.at({1}).control.queue.front()[0] == 0,
          "missing shared bv on monster exit26 rolls back preceding actual movement/control");
}
void rejects() {
    auto s = fixture();
    auto &a = s.world.ai.battle.actors.at({1});
    a.control.queue = {{25, 5}};
    check(prepare_world_misc_control(s, {{1}, {}, {}}).error ==
              WorldMiscControlError::missing_projection,
          "25 cannot fabricate old cached u from current n");
    a.hp.target = std::numeric_limits<int>::max();
    check(prepare_world_misc_control(s, input()).error ==
                  WorldMiscControlError::preparation_failed &&
              a.control.queue.size() == 1 && s.world.ai.contexts.at({1}).effects.display.empty(),
          "signed HP overflow rejects numeric/display/control as one transaction");
    a.control.queue = {{26}};
    s.human_definition_state.clear();
    check(prepare_world_misc_control(s, input()).error ==
                  WorldMiscControlError::missing_definition &&
              a.control.queue.size() == 1 && s.world.ai.human_order.size() == 2,
          "26 cannot invent missing shared definition or partially remove actor");
    a.kind = ActorKind::monster;
    s.world.ai.human_order.erase(s.world.ai.human_order.begin());
    s.world.ai.monster_order = {{1}};
    check(
        prepare_world_misc_control(s, input()).error == WorldMiscControlError::missing_definition,
        "monster26 still requires its actual shared bv definition, not monster catalogue fallback");
    s.human_definition_state.emplace(9, 2);
    const auto monster_exit = prepare_world_misc_control(s, input());
    check(monster_exit.candidate &&
              monster_exit.candidate->action == WorldControlAction::delete_true &&
              monster_exit.candidate->state.human_definition_state.at(9) == 1 &&
              a.control.queue.front()[0] == 26,
          "monster26 follows original unguarded n and shared m mutation without changing input");
    a.control.queue = {{33}};
    auto i = input();
    i.sound_projection = [](Position) -> std::optional<Position> { return {}; };
    check(prepare_world_misc_control(s, i).error == WorldMiscControlError::preparation_failed &&
              a.control.queue.size() == 1,
          "late33 projection failure keeps original queue and emits no sound");
    a.control.queue = {{32}, {99}};
    check(prepare_world_misc_control(s, input()).error == WorldMiscControlError::invalid_input &&
              a.vertical_velocity == -3,
          "full queue malformed preflight precedes otherwise safe32");
    a.control.queue = {{32}};
    s.world.ai.monster_order.push_back({1});
    check(prepare_world_misc_control(s, input()).error == WorldMiscControlError::stale_actor,
          "duplicate roster identity rejects inconsistent owner");
}
void generic_state_commands() {
    for (const ActorKind kind : {ActorKind::human, ActorKind::monster})
        for (int state = 0; state <= 20; ++state)
            for (int action : {3, 6, 7}) {
                auto s = fixture();
                s.world.facilities.at(3).category = 1;
                auto &a = s.world.ai.battle.actors.at({1});
                a.kind = kind;
                if (kind == ActorKind::monster) {
                    s.world.ai.human_order.erase(s.world.ai.human_order.begin());
                    s.world.ai.monster_order = {{1}};
                }
                a.baseline = 5;
                a.control.action = action;
                a.control.flags = 16U | 4U;
                a.control.queue = {{2, state}, {25, 99}};
                a.encounter = 7;
                a.group = 8;
                a.attack_count = 11;
                a.state_parameter = 5;
                a.attack_position = {210, 25, 310};
                RewardHumanDefinition g;
                g.definition.legacy_u = 100;
                s.world.ai.growth.emplace(9, g);
                ActorStateTransitionInput expected;
                expected.control = a.control;
                expected.human = kind == ActorKind::human;
                expected.next_state = state;
                expected.baseline = 5;
                expected.legacy_u = 100;
                expected.boost_ticket = 0;
                expected.current_facility_category = 1;
                const auto pure = prepare_actor_state_transition(expected);
                const auto r = prepare_world_state_command(s.world, {{1}, 0});
                check(
                    r.candidate && pure && !r.candidate->cleaned_up &&
                        world_control_detail::same_control(
                            r.candidate->state.ai.battle.actors.at({1}).control, pure->control) &&
                        r.candidate->state.ai.battle.actors.at({1}).baseline == pure->baseline &&
                        r.candidate->state.ai.battle.actors.at({1}).state_counter == 0 &&
                        r.candidate->state.ai.battle.actors.at({1}).state_parameter == 0,
                    "all human/monster0..20 actual setters apply canonical B/C/i/control/baseline");
                const auto &after = r.candidate->state.ai.battle.actors.at({1});
                const auto position = pure->copy_attack_position ? a.attack_position : a.position;
                check(after.position.x == position.x && after.position.z == position.z &&
                          after.position.height == position.height &&
                          after.encounter == (pure->clear_encounter ? std::optional<std::uint64_t>{}
                                                                    : a.encounter) &&
                          after.group == a.group && after.attack_count == (state == 18 ? 0 : 11) &&
                          after.object_slot == 7 &&
                          r.candidate->consumed_boost_ticket == (state == 18) &&
                          r.candidate->event_requests ==
                              (state == 18 ? std::vector<int>{116} : std::vector<int>{}),
                      "c3 copies full au only for3/6, human db differs from dc,18 consumes one "
                      "boost");
                check(a.state_counter == 23 && a.control.queue.size() == 2 &&
                          a.attack_count == 11 && !s.world.ai.battle.events.count(116),
                      "state command candidate never changes original actor/event owner");
            }
}
void boost_and_q() {
    auto s = fixture();
    auto &a = s.world.ai.battle.actors.at({1});
    a.control.queue = {{2, 18}};
    check(prepare_world_state_command(s.world, {{1}, 0}).error ==
              WorldMiscControlError::missing_definition,
          "unboosted18 cannot invent n shared legacy_u");
    RewardHumanDefinition g;
    s.world.ai.growth.emplace(9, g);
    check(prepare_world_state_command(s.world, {{1}, {}}).error ==
              WorldMiscControlError::preparation_failed,
          "legacy_u zero18 still needs actual draw100, not deterministic no-draw fallback");
    auto zero = prepare_world_state_command(s.world, {{1}, 0});
    check(zero.candidate && zero.candidate->consumed_boost_ticket &&
              !(zero.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U) &&
              zero.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{10, 1}},
          "18 zero threshold consumes exactly one ticket and appends actual wander10");
    s.world.ai.growth.at(9).definition.legacy_u = 100;
    s.world.ai.battle.events.insert(116);
    const auto seen = prepare_world_state_command(s.world, {{1}, 0});
    check(seen.candidate && seen.candidate->consumed_boost_ticket &&
              (seen.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U) &&
              seen.candidate->event_requests.empty(),
          "seen116 suppresses only request, not actual boost or ticket");
    a.control.flags |= 2048U;
    s.world.ai.growth.clear();
    const auto already = prepare_world_state_command(s.world, {{1}, {}});
    check(already.candidate && !already.candidate->consumed_boost_ticket &&
              already.candidate->event_requests.empty(),
          "already2048 returns before shared n lookup and random draw");
    s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{2, 10}, {25, 99}};
    s.world.ai.battle.actors.at({1}).control.flags = 1024U | 2048U;
    s.world.ai.battle.actors.at({1}).rescue = CharacterId{2};
    s.world.ai.battle.actors.at({1}).encounter = 7;
    s.world.ai.battle.actors.at({1}).group = 8;
    s.world.facilities.at(3).status = 0;
    const auto inn = prepare_world_state_command(s.world, {{1}, {}});
    check(inn.candidate && inn.candidate->cleaned_up &&
              inn.candidate->state.ai.battle.actors.at({1}).control.state == 19 &&
              inn.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{1, 120, 0}, {8, 5}} &&
              inn.candidate->state.ai.battle.actors.at({1}).control.action == 6 &&
              inn.candidate->state.ai.battle.actors.at({1}).control.action_counter == 8 &&
              (inn.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U) &&
              !inn.candidate->state.ai.battle.actors.at({1}).rescue &&
              !inn.candidate->state.ai.battle.actors.at({1}).encounter &&
              !inn.candidate->state.ai.battle.actors.at({1}).group &&
              inn.candidate->state.facilities.at(3).occupants == std::vector<CharacterId>{{1}},
          "c10 at exact q category2 status0 performs real r AFTER public reset, no ordinary "
          "clear2048");
    s.world.ai.contexts.at({1}).cell = {0, 0};
    const auto mismatch = prepare_world_state_command(s.world, {{1}, {}});
    check(mismatch.candidate && !mismatch.candidate->cleaned_up &&
              mismatch.candidate->state.ai.battle.actors.at({1}).control.state == 10 &&
              !(mismatch.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U) &&
              mismatch.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{1, 5, 0}, {32}, {3, 6}, {1, 14, 0}, {3, 0}} &&
              mismatch.candidate->state.facilities.at(3).occupants.size() == 2,
          "q wrong old-s is original null and regular c10 victory, not invented cleanup/error");
    s.world.map.cells.clear();
    check(prepare_world_state_command(s.world, {{1}, {}}).error ==
              WorldMiscControlError::invalid_input,
          "malformed map cannot be interpreted as legitimate q-null victory");
    s = fixture();
    auto &monster = s.world.ai.battle.actors.at({1});
    monster.kind = ActorKind::monster;
    monster.control.queue = {{2, 10}, {25, 99}};
    monster.rescue = CharacterId{2};
    monster.encounter = 7;
    monster.group = 8;
    s.world.ai.human_order.erase(s.world.ai.human_order.begin());
    s.world.ai.monster_order = {{1}};
    const auto monster_inn = prepare_world_state_command(s.world, {{1}, {}});
    check(monster_inn.candidate && monster_inn.candidate->cleaned_up &&
              monster_inn.candidate->state.ai.battle.actors.at({1}).control.state == 0 &&
              monster_inn.candidate->state.ai.battle.actors.at({1}).control.action == 0 &&
              monster_inn.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 5}} &&
              !monster_inn.candidate->state.ai.battle.actors.at({1}).encounter &&
              !monster_inn.candidate->state.ai.battle.actors.at({1}).group &&
              monster_inn.candidate->state.ai.battle.actors.at({1}).object_slot == 7 &&
              monster_inn.candidate->state.facilities.at(3).occupants ==
                  std::vector<CharacterId>{{1}},
          "monster c10 inn real r releases q then c0/n0/activity5, preserving N");
    monster.control.queue = {{2, 3}};
    monster.control.action = 3;
    monster.attack_position.x = std::numeric_limits<float>::quiet_NaN();
    check(prepare_world_state_command(s.world, {{1}, {}}).error ==
                  WorldMiscControlError::invalid_input &&
              monster.control.state == 0 && monster.control.queue.size() == 1,
          "invalid au copy rejects state/control mutation atomically");
    monster.control.queue = {{2, 18}};
    s.world.ai.growth.emplace(9, RewardHumanDefinition{});
    check(prepare_world_state_command(s.world, {{1}, -1}).error ==
                  WorldMiscControlError::preparation_failed &&
              prepare_world_state_command(s.world, {{1}, 100}).error ==
                  WorldMiscControlError::preparation_failed,
          "boost ticket must match actual bound100, even with zero threshold");
    s.world.ai.growth.at(9).definition.legacy_u = 101;
    check(prepare_world_state_command(s.world, {{1}, 0}).error ==
              WorldMiscControlError::preparation_failed,
          "invalid shared legacy_u rejects rather than clamps to a guessed source rule");
}
void setter_fifo_and_atomic_failure() {
    Harness s{fixture(), {}};
    s.misc.world.facilities.at(3).category = 1;
    s.misc.world.ai.battle.actors.at({1}).control.queue = {{2, 10}, {25, 99}};
    auto a = adapter();
    const auto misc = a.domain;
    a.domain = [misc](const Harness &old,
                      CharacterId id) -> std::optional<WorldControlStep<Harness>> {
        if (old.misc.world.ai.battle.actors.at(id).control.queue.front()[0] != 2)
            return misc(old, id);
        const auto r = prepare_world_state_command(old.misc.world, {id, {}});
        if (!r.candidate)
            return {};
        auto next = old;
        next.misc.world = r.candidate->state;
        return WorldControlStep<Harness>{std::move(next)};
    };
    const auto r = prepare_world_control(s, {1}, a);
    check(
        r.candidate && r.candidate->flow == WorldControlFlow::held &&
            r.candidate->state.misc.world.ai.battle.actors.at({1}).control.queue ==
                std::vector<LegacyActorControl>{{1, 4, 0}, {32}, {3, 6}, {1, 14, 0}, {3, 0}} &&
            r.candidate->state.misc.world.ai.battle.actors.at({1}).hp.target == 25,
        "actual setter clears old heal tail, newly arranged victory wait ticks once during same v");
    s.misc.world.facilities.at(3).category = 2;
    const auto late = prepare_world_control(s, {1}, a);
    check(late.error == WorldControlError::consumer_failed && !late.candidate &&
              s.misc.world.facilities.at(3).occupants.size() == 2 &&
              s.misc.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{2, 10}, {25, 99}} &&
              s.misc.world.ai.battle.actors.at({1}).position.height == 10,
          "late missing actual8 consumer rolls back real c10/r release, height and replaced FIFO");
}
} // namespace
int main() {
    try {
        hp_and_visuals();
        jump_sound_and_delete();
        actual_fifo();
        rejects();
        generic_state_commands();
        boost_and_q();
        setter_fifo_and_atomic_failure();
        std::cout << "world_misc_control checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
