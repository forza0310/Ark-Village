#include "ark/simulation/combat/rules/combat_commit.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
AiRewardState fixture() {
    AiRewardState s;
    for (CharacterId id : {CharacterId{1}, CharacterId{2}, CharacterId{3}}) {
        BattleActorRecord a;
        a.id = id;
        a.kind = id.value == 1 ? ActorKind::human : ActorKind::monster;
        a.definition = id.value == 1 ? 1 : 7;
        a.capacity = 10000;
        a.hp = {0, 10000, 10000, 10000, false, 0};
        a.control.state = 1;
        a.control.action = id.value == 1 ? 1 : 6;
        a.control.flags = 2U;
        a.encounter = 0;
        a.position = {static_cast<float>((id.value - 1) * 40), 0, 0};
        a.attack_position = a.position;
        a.baseline = id.value == 1 ? 5 : 17;
        s.battle.actors.emplace(id, a);
        s.contexts.emplace(id, RewardActorContext{{0, 0}, false, {}, {}, true, {0, 0}});
        (id.value == 1 ? s.human_order : s.monster_order).push_back(id);
    }
    RewardHumanDefinition g;
    g.derived.attributes[2] = 0;
    g.derived.combat = {10000, 100, 100, 100};
    g.derived.available_spells[0] = true;
    s.growth.emplace(1, g);
    s.battle.humans.emplace(1, HumanBattleRecord{});
    s.battle.monsters.emplace(7, MonsterBattleRecord{});
    RewardMonsterDefinition m;
    m.base_hp = 10000;
    m.base_attack = m.base_defense = 100;
    s.monster_growth.emplace(7, m);
    s.encounters.emplace(0, RewardEncounter{{0, {0, 0}, 0, 0, 0, 2, 2, 0}, {{2}, {3}}, true, {}});
    return s;
}
WorldAttackInput input(CharacterId actor = {1}) {
    WorldAttackInput i;
    i.actor = actor;
    i.weapon = {0, 100, 1, 0, 0};
    i.physical_jitter = 0;
    i.magic_jitter = 0;
    i.spell_ticket = 0;
    i.enhancement_ticket = 99;
    i.facing = 2;
    i.drop_ticket = 99;
    return i;
}
void human_windows() {
    for (int kind = 0; kind <= 3; ++kind)
        for (int counter = 0; counter <= 16; ++counter) {
            auto s = fixture();
            auto &a = s.battle.actors.at({1});
            a.control.queue = {{14}, {3, 0}, {1, 10, 0}, {7, 4}};
            a.control.action = kind == 1 ? 2 : kind == 2 ? 3 : 1;
            a.control.action_counter = counter;
            auto i = input();
            i.weapon.kind = kind;
            const auto r = prepare_world_attack_control(s, i);
            const bool direct = kind != 1 && counter >= 5 && counter <= 8;
            check(r.candidate && r.candidate->hit.has_value() == direct &&
                      r.candidate->projectile.has_value() == (kind == 1 && counter == 8),
                  "direct5..8 vs bow8 current-world consumption");
            if (direct)
                check(r.candidate->state.battle.actors.at({2}).hp.target == 9927 &&
                          !r.candidate->state.battle.actors.at({1}).attack_armed,
                      "current attack/defense integer damage and armed consumed atomically");
            if (kind == 1 && counter == 8)
                check(r.candidate->state.battle.actors.at({2}).hp.target == 10000 &&
                          r.candidate->state.projectile_order.size() == 1,
                      "bow launch has no eager HP damage");
            check(s.battle.actors.at({2}).hp.target == 10000 && s.projectiles.empty(),
                  "input unchanged after command preparation");
        }
    auto s = fixture();
    auto &a = s.battle.actors.at({1});
    a.control.queue = {{14}};
    a.control.action_counter = 5;
    a.miss = true;
    auto r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->hit && !r.candidate->hit->landed &&
              !r.candidate->state.battle.actors.at({1}).attack_armed &&
              r.candidate->state.battle.actors.at({2}).hp.target == 10000,
          "miss still consumes ai, HP unchanged");
    s = r.candidate->state;
    s.battle.actors.at({1}).control.action_counter = 6;
    auto i = input();
    i.physical_jitter.reset();
    r = prepare_world_attack_control(s, i);
    check(r.candidate && !r.candidate->hit, "already consumed ai needs no second damage ticket");
    s = fixture();
    s.battle.actors.at({1}).control.queue = {{14}};
    s.battle.actors.at({1}).control.action_counter = 5;
    s.battle.actors.at({2}).control.state = 2;
    r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->target == CharacterId{3},
          "fresh e avoids newly down original target and finds current other opponent");
    s.battle.actors.at({1}).control.flags |= 128U;
    s.battle.actors.at({1}).group = 0;
    s.encounters.at(0).group.monsters = {{{3}, 128U}, {{3}, 128U}};
    r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->target == CharacterId{3},
          "real repeated group references accepted without deduplication");
    s = fixture();
    s.battle.actors.at({1}).control.queue = {{14}};
    s.battle.actors.at({1}).control.action_counter = 5;
    s.battle.actors.at({2}).hp.target = 1;
    i = input();
    i.drop_ticket.reset();
    check(!prepare_world_attack_control(s, i).candidate && s.battle.actors.at({1}).attack_armed &&
              s.battle.actors.at({2}).hp.target == 1,
          "late lethal ticket failure rolls back attack-armed consumption and HP");
    s = fixture();
    s.battle.actors.at({1}).control.queue = {{14}, {3, 0}};
    s.battle.actors.at({1}).control.action_counter = 7;
    s.battle.actors.at({1}).combo_count = 2;
    r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->hit && !r.candidate->completed &&
              r.candidate->state.battle.actors.at({1}).combo_index == 1 &&
              r.candidate->state.battle.actors.at({1}).control.action_counter == 0 &&
              r.candidate->state.battle.actors.at({1}).attack_armed,
          "nonfinal7 hits then restarts same command with x1/ai true/l0");
    s = r.candidate->state;
    s.battle.actors.at({1}).control.action_counter = 4;
    r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->hit &&
              r.candidate->state.battle.actors.at({1}).control.action_counter == 5,
          "later combo adds extra l before window and can consume new hit");
    s = fixture();
    s.battle.actors.at({1}).control.queue = {{14}};
    s.battle.actors.at({1}).control.action_counter = 12;
    s.battle.actors.at({1}).control.flags |= 128U;
    s.battle.actors.at({1}).group.reset();
    r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->completed,
          "outside actual attack window does not query missing group or require damage draws");
}
void magic_and_healing() {
    auto s = fixture();
    auto &a = s.battle.actors.at({1});
    a.control.queue = {{15}, {3, 0}};
    a.control.action = 4;
    a.control.action_counter = 26;
    auto r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->projectile &&
              r.candidate->state.projectiles.at(*r.candidate->projectile).kind ==
                  ProjectileKind::spell &&
              r.candidate->state.projectiles.at(*r.candidate->projectile).damage == 45 &&
              r.candidate->state.battle.actors.at({2}).hp.target == 10000,
          "offensive26 queries fresh target and creates stored-damage spell, not direct HP");
    auto i = input();
    i.enhancement_ticket.reset();
    check(!prepare_world_attack_control(s, i).candidate && s.projectiles.empty(),
          "missing actual spell draw rejects entire command");
    s.monster_order.clear();
    i = {};
    i.actor = {1};
    r = prepare_world_attack_control(s, i);
    check(r.candidate && !r.candidate->projectile && !r.candidate->completed &&
              r.candidate->state.battle.actors.at({1}).control.queue.size() == 2,
          "no target26 stops without random consumption or premature completion");
    s.battle.actors.at({1}).control.action_counter = 42;
    s.battle.actors.at({1}).attack_idle = 99;
    r = prepare_world_attack_control(s, i);
    check(r.candidate && r.candidate->completed && !r.candidate->early_stop &&
              r.candidate->state.battle.actors.at({1}).attack_idle == 0 &&
              r.candidate->state.battle.actors.at({1}).control.queue.front() ==
                  LegacyActorControl{3, 0},
          "spell42 removes only command, clears av and permits interpreter continuation");
    s = fixture();
    auto down = s.battle.actors.at({1});
    down.id = {4};
    down.control.state = 2;
    down.hp = {0, 0, 0, 0, false, 0};
    down.capacity = 100;
    down.state_counter = 5;
    s.battle.actors.emplace(down.id, down);
    s.contexts.emplace(down.id, s.contexts.at({1}));
    s.human_order.insert(s.human_order.begin(), down.id);
    s.battle.actors.at({1}).control.queue = {{16}};
    s.battle.actors.at({1}).control.action_counter = 26;
    r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->target == CharacterId{4} &&
              r.candidate->state.battle.actors.at({4}).hp.target == 90 &&
              r.candidate->state.battle.actors.at({4}).state_counter == 810 &&
              r.candidate->state.contexts.at({4}).effects.display.back() ==
                  ActorEffectRecord{2, 0, 90, 160, -26, 2},
          "J first down target gets HP90, recovery B810 and six-field healing display append");
    s.growth.at(1).derived.combat[3] = 0;
    r = prepare_world_attack_control(s, input());
    check(r.candidate && r.candidate->state.battle.actors.at({4}).hp.target == -1 &&
              r.candidate->state.battle.actors.at({4}).state_counter == 5,
          "magic0 negative heal preserved without eager death/baseline or reduced old B");
}
void monsters_and_setup() {
    for (int ticket : {0, 11, 12, 99}) {
        auto s = fixture();
        WorldAttackSetupInput i;
        i.actor = {2};
        i.target = {1};
        i.facing = 2;
        i.monster_miss_ticket = ticket;
        const auto r = prepare_world_attack_setup(s, i);
        check(r.candidate && r.candidate->state.battle.actors.at({2}).miss == (ticket < 12) &&
                  r.candidate->state.battle.actors.at({2}).attack_destination.x == 30 &&
                  r.candidate->state.battle.actors.at({2}).control.queue ==
                      std::vector<LegacyActorControl>{{3, 6}, {17}, {3, 0}, {1, 10, 0}, {7, 4}},
              "monster setup12 threshold, minimum quarter-distance endpoint, ordered queue");
    }
    auto s = fixture();
    WorldAttackSetupInput setup;
    setup.actor = {1};
    setup.target = {2};
    setup.weapon = {0, 100, 1, 0, 0};
    setup.human_tickets = {99, 99, 99, 99, 99};
    setup.facing = 2;
    auto r = prepare_world_attack_setup(s, setup);
    check(r.candidate && r.candidate->state.battle.actors.at({1}).attack_count == 1 &&
              r.candidate->state.battle.actors.at({1}).combo_count == 1 &&
              r.candidate->state.battle.actors.at({1}).control.flags & 4U,
          "human setup reads current shared dexterity and commits count/queue/flag");
    s.battle.actors.at({2}).control.queue = {{17}};
    s.battle.actors.at({2}).control.action = 6;
    s.battle.actors.at({2}).control.action_counter = 12;
    s.battle.actors.at({2}).attack_position = {0, 0, 0};
    s.battle.actors.at({2}).attack_destination = {300, 0, 0};
    s.battle.actors.at({2}).perceived_enemy = CharacterId{1};
    s.battle.actors.at({1}).control.state = 2;
    r = prepare_world_attack_control(s, input({2}));
    check(
        r.candidate && r.candidate->target == CharacterId{1} && r.candidate->hit &&
            r.candidate->state.battle.actors.at({2}).attack_position.x == 300 &&
            r.candidate->state.battle.actors.at({2}).position.x == 40,
        "preferred down target accepted in bl; hit uses previous au0 before new au300, n40 stays");
    s.battle.actors.at({2}).attack_position = {1000, 0, 0};
    s.contexts.at({1}).half_cell = {9, 9};
    s.battle.actors.at({1}).control.state = 1;
    r = prepare_world_attack_control(s, input({2}));
    check(r.candidate && !r.candidate->hit && r.candidate->state.battle.actors.at({2}).attack_armed,
          "far previous au and invalid fallback half-distance do not consume ai");
}
AiRewardState policy_fixture() {
    auto s = fixture();
    for (auto &[id, a] : s.battle.actors) {
        a.position = {275 + static_cast<float>((id.value - 1) * 40), 0, 275};
        a.control.action = 0;
        a.control.flags = 128U | 2U;
        a.group = 0;
        a.attack_slot = 20;
        a.perceived_enemy = id.value == 1 ? CharacterId{2} : CharacterId{1};
        a.perceived_distance = id.value == 3 ? 80 : 40;
        s.contexts.at(id).cell = {2, 2};
        s.contexts.at(id).half_cell = {5, 5};
    }
    auto &e = s.encounters.at(0);
    e.runtime.center = {2, 2};
    e.runtime.idle = 19;
    e.group.tick = 20;
    e.group.humans = {{{1}, 128U}};
    e.group.monsters = {{{2}, 128U}, {{3}, 128U}};
    return s;
}
WorldCombatPolicyInput policy_input(CharacterId actor = {1}) {
    WorldCombatPolicyInput i;
    i.actor = actor;
    i.weapon = {0, 100, 1, 0, 0};
    i.attack_tickets = {99, 99, 99, 99, 99};
    i.monster_miss_ticket = 99;
    i.monster_range = 70;
    i.policy_ticket = 0;
    i.healing_ticket = 0;
    i.boost_ticket = 99;
    i.facing = 2;
    return i;
}
void actual_policy() {
    const WorldMapFacts f{{8, 8, std::vector<LegacyMapCell>(64)},
                          std::vector<int>(64, 1),
                          std::vector<std::uint32_t>(64),
                          {0, 5, 0, 5}};
    for (int role = 0; role < 5; ++role)
        for (int weapon = 0; weapon < 4; ++weapon)
            for (int distance : {40, 100, 300})
                for (int ticket = 0; ticket < 100; ++ticket) {
                    auto s = policy_fixture();
                    s.battle.actors.at({1}).perceived_distance = static_cast<float>(distance);
                    auto i = policy_input();
                    i.profession_role = role;
                    i.weapon.kind = weapon;
                    i.policy_ticket = ticket;
                    const auto r = prepare_world_combat_policy(s, i, f);
                    check(r.candidate && r.candidate->strategy.consumed_policy_ticket &&
                              r.candidate->fresh_enemy == CharacterId{2} &&
                              r.candidate->state.battle.actors.at({1}).attack_idle == 1,
                          "current world policy exact attack slot always consumes100 and av++");
                    const auto &a = r.candidate->state.battle.actors.at({1});
                    if (r.candidate->strategy.decision == CombatDecision::physical_attack)
                        check(a.control.queue[1][0] == 14 && a.attack_count == 1 &&
                                  (a.control.flags & 4U) &&
                                  s.battle.actors.at({2}).hp.target == 10000,
                              "physical strategy prepares queue but doesn't execute damage in c");
                    if (r.candidate->strategy.decision == CombatDecision::offensive_spell)
                        check(a.control.queue[1][0] == 15 && a.attack_count == 1 && !a.miss &&
                                  r.candidate->requests.back().parameter == 10,
                              "offensive spell increments as/clears miss/sound10, queue not run");
                    if (r.candidate->strategy.decision == CombatDecision::approach)
                        check(r.candidate->move_target && a.position.x > 275 &&
                                  r.candidate->state.contexts.at({1}).half_cell == Position{5, 5},
                              "approach actual nine-sample movement preserves cached t until d");
                }
    auto s = policy_fixture();
    auto i = policy_input();
    auto r = prepare_world_combat_policy(s, i, f);
    check(r.candidate && r.candidate->strategy.decision == CombatDecision::physical_attack,
          "normal melee slot selects actual setup");
    auto current = r.candidate->state;
    const auto local = prepare_local_control_prefix(current.battle.actors.at({1}).control);
    check(local.candidate && local.candidate->flow == ActorControlFlow::delegated &&
              local.candidate->state.action == 1 && local.candidate->state.queue.front()[0] == 14,
          "following d interprets action before handing current front14 to combat owner");
    current.battle.actors.at({1}).control = local.candidate->state;
    current.battle.actors.at({1}).control.action_counter = 5;
    const auto hit = prepare_world_attack_control(current, input());
    check(hit.candidate && hit.candidate->hit &&
              hit.candidate->state.battle.actors.at({2}).hp.target == 9927,
          "policy -> local interpreter -> actual attack shares HP owner and current definitions");
    i.facing.reset();
    check(!prepare_world_combat_policy(s, i, f).candidate &&
              s.battle.actors.at({1}).attack_idle == 0 && s.encounters.at(0).runtime.idle == 19,
          "late setup failure exposes no partial av/group/control changes");
    i = policy_input();
    s.growth.at(1).derived.available_spells[3] = true;
    s.contexts.at({1}).low_hp = true;
    i.profession_role = 1;
    i.policy_ticket = 99;
    i.healing_ticket = 6;
    r = prepare_world_combat_policy(s, i, f);
    check(r.candidate && r.candidate->strategy.decision == CombatDecision::healing_spell &&
              r.candidate->state.battle.actors.at({1}).attack_count == 0 &&
              r.candidate->state.battle.actors.at({1}).control.queue[1][0] == 16 &&
              r.candidate->requests.empty(),
          "healing choice consumes10 but no Q count/no offensive sound/no target reservation");
    s = policy_fixture();
    s.battle.actors.at({1}).control.flags &= ~128U;
    s.battle.actors.at({1}).group.reset();
    s.encounters.at(0).group.humans.clear();
    s.encounters.at(0).group.monsters.clear();
    r = prepare_world_combat_policy(s, policy_input(), f);
    check(r.candidate && r.candidate->strategy.decision == CombatDecision::join_group &&
              r.candidate->state.battle.actors.at({1}).group == 0 &&
              r.candidate->state.battle.actors.at({1}).attack_slot == 0 &&
              r.candidate->state.encounters.at(0).group.humans.size() == 1 &&
              !r.candidate->strategy.consumed_policy_ticket,
          "unjoined actor fresh e -> actual group append, only caller gets128/an0/dc");
    s = policy_fixture();
    s.battle.actors.at({1}).perceived_enemy.reset();
    s.battle.actors.at({1}).state_counter = 70;
    s.battle.actors.at({1}).attack_count = 6;
    i = policy_input();
    i.boost_ticket.reset();
    check(!prepare_world_combat_policy(s, i, f).candidate,
          "human baseline fallback c18 requires boost draw even when threshold zero");
    i.boost_ticket = 99;
    r = prepare_world_combat_policy(s, i, f);
    check(r.candidate && r.candidate->state.battle.actors.at({1}).control.state == 18 &&
              r.candidate->state.battle.actors.at({1}).state_counter == 0 &&
              r.candidate->state.battle.actors.at({1}).attack_count == 0 &&
              r.candidate->consumed_boost_ticket,
          "human c18 keeps db/reset B/C/as and queues battle wander plus real boost consumer");
    s = policy_fixture();
    s.battle.actors.at({2}).control.flags |= 4U;
    r = prepare_world_combat_policy(s, policy_input({2}), f);
    check(r.candidate && r.candidate->strategy.decision == CombatDecision::keep &&
              r.candidate->state.battle.actors.at({2}).attack_idle == 1 &&
              r.candidate->state.encounters.at(0).runtime.idle == 0,
          "locked monster increments av and resets db.m before lock early return");
    s = policy_fixture();
    s.encounters.at(0).group.tick = 14;
    r = prepare_world_combat_policy(s, policy_input(), f);
    check(r.candidate && r.candidate->strategy.telegraph && r.candidate->strategy.face_enemy &&
              !(r.candidate->state.battle.actors.at({1}).control.flags & 2U) &&
              r.candidate->requests.front().kind == WorldAttackVisual::telegraph,
          "an-6 telegraph and an-16..an facing/clear2 precede policy decision");
    s = policy_fixture();
    s.battle.actors.at({2}).monster_posture = 2;
    r = prepare_world_combat_policy(s, policy_input({2}), f);
    check(r.candidate && r.candidate->strategy.decision == CombatDecision::low_influence &&
              !r.candidate->strategy.telegraph && r.candidate->move_target,
          "posture2 monster bypasses slot/facing/telegraph and actually low-field moves");
}
void interpreter_continuation() {
    auto s = fixture();
    auto &a = s.battle.actors.at({1});
    a.control.queue = {{14}, {3, 0}, {1, 10, 0}, {7, 4}};
    a.control.flags |= 4U;
    a.control.action_counter = 14;
    auto r = prepare_world_attack_execution(s, input());
    check(r.candidate && r.candidate->completed && r.candidate->early_stop &&
              r.candidate->state.battle.actors.at({1}).control.action == 0 &&
              r.candidate->state.battle.actors.at({1}).control.queue.front() ==
                  LegacyActorControl{1, 9, 0} &&
              (r.candidate->state.battle.actors.at({1}).control.flags & 4U),
          "completed14 same-d executes action0 and first wait decrement before tail clears4");
    s = r.candidate->state;
    s.battle.actors.at({1}).control.queue.front()[1] = 1;
    r = prepare_world_attack_execution(s, input());
    check(r.candidate && !r.candidate->completed && !r.candidate->early_stop &&
              r.candidate->state.battle.actors.at({1}).control.queue.empty() &&
              !(r.candidate->state.battle.actors.at({1}).control.flags & 4U),
          "wait old1 reaches0 then clear4 continues same execution, without another d prefix");
    s = fixture();
    s.battle.actors.at({1}).control.queue = {{3, 4}, {15}, {24}};
    r = prepare_world_attack_execution(s, input());
    check(r.candidate && !r.candidate->completed && r.candidate->early_stop &&
              r.candidate->state.battle.actors.at({1}).control.action == 4 &&
              r.candidate->state.battle.actors.at({1}).control.action_counter == 0 &&
              r.candidate->state.battle.actors.at({1}).control.queue.front()[0] == 15,
          "new action4 resets l before15, never consumes stale prior count");
    s.battle.actors.at({1}).control.queue = {{3, 0}, {24}};
    r = prepare_world_attack_execution(s, input());
    check(r.candidate && r.candidate->early_stop &&
              r.candidate->state.battle.actors.at({1}).control.queue.front()[0] == 24,
          "other domain command remains explicit delegated front, no silent pop/no-op");
    s = fixture();
    s.battle.actors.at({1}).control.queue = {{14}, {3, 0}, {24}};
    s.battle.actors.at({1}).control.action_counter = 14;
    r = prepare_world_attack_execution(s, input());
    check(r.candidate && r.candidate->completed && r.candidate->early_stop &&
              r.candidate->state.battle.actors.at({1}).control.queue.front()[0] == 24,
          "attack completion continues local tail and stops at next external consumer");
}
} // namespace
int main() {
    human_windows();
    magic_and_healing();
    monsters_and_setup();
    actual_policy();
    interpreter_continuation();
    std::cout << "combat commit checks: " << checks << '\n';
}
