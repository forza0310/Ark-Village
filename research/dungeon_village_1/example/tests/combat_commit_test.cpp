#include "dungeon_village_reference/combat_commit.hpp"

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
} // namespace
int main() {
    human_windows();
    magic_and_healing();
    monsters_and_setup();
    std::cout << "combat commit checks: " << checks << '\n';
}
