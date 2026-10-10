#include "ark/simulation/combat/rules/combat_ai.hpp"

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
CombatStrategyInput base() {
    CombatStrategyInput i;
    i.flags = 128;
    i.in_move_area = i.sensed_enemy = i.same_town_side = i.fresh_enemy = true;
    i.attack_slot = i.group_tick = 10;
    i.weapon_range = 100;
    i.monster_range = 100;
    return i;
}
void choices() {
    const int physical_count[]{90, 50, 50, 70, 70};
    const int spell_count[]{20, 80, 80, 55, 40};
    for (int role = 0; role < 5; ++role)
        for (bool physical : {false, true})
            for (unsigned mask = 0; mask < 4; ++mask) {
                int attack{}, approach{}, offensive{}, healing{}, secondary_draws{};
                for (int ticket = 0; ticket < 100; ++ticket)
                    for (int second = 0; second < 10; ++second) {
                        auto i = base();
                        i.profession_role = role;
                        i.sensed_distance = physical ? 89.0F : 90.0F;
                        i.spells[0] = (mask & 1) != 0;
                        i.spells[3] = i.healing_target = (mask & 2) != 0;
                        i.policy_ticket = ticket;
                        i.healing_ticket = second;
                        const auto r = prepare_combat_strategy(i);
                        check(r.candidate && r.candidate->consumed_policy_ticket,
                              "human attack slot always consumes primary draw");
                        attack += r.candidate->decision == CombatDecision::physical_attack;
                        approach += r.candidate->decision == CombatDecision::approach;
                        offensive += r.candidate->decision == CombatDecision::offensive_spell;
                        healing += r.candidate->decision == CombatDecision::healing_spell;
                        secondary_draws += r.candidate->consumed_healing_ticket;
                    }
                const int expected_attack =
                    physical ? (mask ? physical_count[role] * 10 : 1000) : 0;
                const int expected_spell =
                    mask ? physical ? 1000 - expected_attack : spell_count[role] * 10 : 0;
                check(attack == expected_attack && approach == 1000 - attack - expected_spell,
                      "all role table buckets and physical-only fallback");
                check(offensive == ((mask == 3)   ? expected_spell * 4 / 10
                                    : (mask == 1) ? expected_spell
                                                  : 0) &&
                          healing == ((mask == 3)   ? expected_spell * 6 / 10
                                      : (mask == 2) ? expected_spell
                                                    : 0),
                      "both spells split 4 offensive 6 healing");
                check(secondary_draws == (mask == 3 ? expected_spell : 0),
                      "no speculative secondary randomness");
            }
    auto i = base();
    check(prepare_combat_strategy(i).error == CombatAiError::missing_ticket,
          "random input cannot be invented");
    i.policy_ticket = 100;
    check(prepare_combat_strategy(i).error == CombatAiError::invalid_ticket,
          "out of range ticket rejected without effects");
    i.policy_ticket = 99;
    i.spells[0] = i.spells[3] = i.healing_target = true;
    check(prepare_combat_strategy(i).error == CombatAiError::missing_ticket,
          "spell mixture needs separate ticket");
    i.spells[0] = false;
    check(prepare_combat_strategy(i).candidate->decision == CombatDecision::healing_spell,
          "heal-only branch needs no secondary ticket");
    i = base();
    i.sensed_distance = 300;
    i.spells[0] = true;
    i.policy_ticket = 0;
    check(prepare_combat_strategy(i).candidate->decision == CombatDecision::approach,
          "spell range is strict less than300");
}
void timing_and_guards() {
    for (int tick = 0; tick < 30; ++tick) {
        auto i = base();
        i.group_tick = tick;
        i.attack_slot = 20;
        i.policy_ticket = 0;
        auto r = prepare_combat_strategy(i);
        check(r.candidate && r.candidate->face_enemy == (tick >= 4 && tick < 20) &&
                  r.candidate->clear_animation_flag == (tick >= 4 && tick < 20) &&
                  r.candidate->telegraph == (tick == 14) &&
                  r.candidate->consumed_policy_ticket == (tick == 20),
              "pre-attack facing telegraph and exact slot");
    }
    for (int role = 0; role < 5; ++role)
        for (int kind = 0; kind < 4; ++kind) {
            auto i = base();
            i.group_tick = 11;
            i.sensed_distance = 90;
            i.spells[0] = true;
            i.profession_role = role;
            i.weapon_kind = kind;
            auto r = prepare_combat_strategy(i);
            check(r.candidate->decision ==
                      ((role == 0 || role == 4) ? CombatDecision::approach : CombatDecision::keep),
                  "column1 off-slot movement compares table weights");
            i.group_cycle = 2;
            r = prepare_combat_strategy(i);
            check(r.candidate->decision == ((kind == 1 || role == 1) ? CombatDecision::low_influence
                                            : (role == 0 || role == 4) ? CombatDecision::approach
                                                                       : CombatDecision::keep),
                  "cycle2 range or magic-role low influence override");
        }
    auto i = base();
    i.flags = 4;
    check(prepare_combat_strategy(i).candidate->decision == CombatDecision::keep,
          "attack lock before group creation");
    i.flags = 0;
    check(prepare_combat_strategy(i).candidate->decision == CombatDecision::join_group,
          "ungrouped actor joins before strategy");
    i.in_move_area = false;
    check(prepare_combat_strategy(i).candidate->decision == CombatDecision::battle_prepare,
          "human invalid area prepares battle again");
    i.kind = ActorKind::monster;
    check(prepare_combat_strategy(i).candidate->decision == CombatDecision::baseline,
          "monster invalid area restores baseline");
    i = base();
    i.kind = ActorKind::monster;
    for (int posture = 0; posture < 3; ++posture)
        for (int tick : {9, 10, 11})
            for (float distance : {99.0F, 100.0F, 101.0F}) {
                i.monster_posture = posture;
                i.group_tick = tick;
                i.fresh_distance = distance;
                const auto r = prepare_combat_strategy(i);
                const auto expected =
                    posture == 2 ? CombatDecision::low_influence
                    : tick == 10
                        ? distance < 100 ? CombatDecision::physical_attack : CombatDecision::keep
                    : distance < 100 ? CombatDecision::keep
                    : posture == 0   ? CombatDecision::approach
                                     : CombatDecision::low_influence;
                check(r.candidate && r.candidate->decision == expected &&
                          !r.candidate->consumed_policy_ticket && r.candidate->reset_encounter_idle,
                      "monster posture bypass and strict fresh enemy range");
            }
}
void movement() {
    std::array<CombatMoveSample, 9> samples{};
    auto r = prepare_combat_step(samples, false);
    check(r.candidate && !r.candidate->selected, "zero approach scores do not move");
    r = prepare_combat_step(samples, true);
    check(r.candidate && r.candidate->selected == 0,
          "original low influence includes invalid zero entries");
    for (auto &s : samples)
        s = {true, true, 100, 100};
    check(prepare_combat_step(samples, false).candidate->selected == 0,
          "strict score comparison keeps first tied direction");
    samples[4].influence = 200;
    check(prepare_combat_step(samples, false).candidate->scores[4] == 50,
          "influence capped at100 before average");
    samples[4].influence = 0;
    check(!prepare_combat_step(samples, true).candidate->selected,
          "center wins means keep not movement");
    for (auto &s : samples)
        s = {true, true, 0, 100};
    samples[8].enemy_distance = 75;
    r = prepare_combat_step(samples, false);
    check(r.candidate->selected == 8 && r.candidate->scores[8] == 25,
          "distance improvement25 gives closeness50 average25");
    samples[8].enemy_distance = 0;
    check(prepare_combat_step(samples, false).candidate->scores[8] == 50, "closeness caps at100");
    samples[8].in_encounter_square = false;
    check(!prepare_combat_step(samples, false).candidate->selected,
          "outside encounter square cannot approach");
    samples[0].enemy_distance = std::numeric_limits<float>::infinity();
    check(prepare_combat_step(samples, false).error == CombatAiError::invalid_input,
          "nonfinite distance rejected");
}
void damage() {
    for (auto kind : {ActorKind::human, ActorKind::monster}) {
        auto r = prepare_physical_damage({kind, 99, 100, false, false, 2});
        check(r.candidate && r.candidate->base == 29 && r.candidate->jitter_bound == 5 &&
                  r.candidate->value == 29,
              "99/100 integer quotient0 gives lower endpoint, not continuous ratio");
        r = prepare_physical_damage({kind, 100, 100, false, false, 8});
        check(r.candidate && r.candidate->base == 81 && r.candidate->jitter_bound == 16 &&
                  r.candidate->value == 81,
              "equal effective stats map integer quotient1 to float interpolation81");
        r = prepare_physical_damage({kind, 100, 0, false, false, 12});
        check(r.candidate && r.candidate->base == 120 && r.candidate->value == 120,
              "zero defense explicitly selects ratio1.3 upper endpoint");
        r = prepare_physical_damage({kind, 100, 50, true, true, 12});
        check(r.candidate && r.candidate->value == 72,
              "human and monster boost application order after jitter");
        for (int ticket = 0; ticket < 2; ++ticket) {
            r = prepare_physical_damage({kind, 0, 1, false, false, ticket});
            check(r.candidate && r.candidate->base == 2 && r.candidate->value == ticket + 1,
                  "minimum base2 still permits jitter result1");
        }
    }
    check(prepare_physical_damage({ActorKind::human, 99, 100, false, false, 5}).error ==
              CombatAiError::invalid_ticket,
          "damage random upper bound exclusive");
    check(prepare_physical_damage(
              {ActorKind::human, std::numeric_limits<int>::max(), 1, false, false, 0})
                  .error == CombatAiError::invalid_input,
          "overflowing endpoints refused instead of Java wraparound");
    for (int spell = 0; spell < 3; ++spell)
        for (int ticket = 0; ticket < 100; ++ticket) {
            SpellDamageInput i{100, {true, true, true}, false, spell, ticket, 0};
            auto r = prepare_spell_damage(i);
            check(r.candidate && r.candidate->effect == spell + (ticket < 5 ? 7 : 4),
                  "magic100 enhancement threshold5 and actual display effect identity");
            i.magic = 1000;
            r = prepare_spell_damage(i);
            check(r.candidate && r.candidate->effect == spell + (ticket < 50 ? 7 : 4),
                  "magic1000 enhancement threshold50");
        }
    SpellDamageInput i{100, {false, true, false}, false, 0, 99, 7};
    auto r = prepare_spell_damage(i);
    check(r.candidate && r.candidate->effect == 5 && r.candidate->damage.base == 70 &&
              r.candidate->damage.value == 70,
          "sparse learned spell ticket uses Q source order");
    i.learned = {};
    check(prepare_spell_damage(i).error == CombatAiError::invalid_input,
          "no learned spell cannot invent effect0 or draw bound0");
}
void influence() {
    CombatInfluenceInput i{6, 6, std::vector<int>(36, 1), {}, {{1, true, {4, 4}}}};
    auto r = prepare_combat_influence(i);
    check(r.candidate && r.candidate->width == 12 && r.candidate->height == 12 &&
              r.candidate->human_field[4 * 12 + 4] == 100 &&
              r.candidate->human_field[2 * 12 + 2] == 30 &&
              r.candidate->human_field[3 * 12 + 3] == 50 &&
              r.candidate->monster_field[4 * 12 + 4] == 0,
          "enemy additive25 kernel and distinct side fields");
    i.humans.push_back({1, true, {4, 4}});
    r = prepare_combat_influence(i);
    check(r.candidate && r.candidate->human_field[4 * 12 + 4] == 80 &&
              r.candidate->human_field[3 * 12 + 3] == 47 &&
              r.candidate->monster_field[4 * 12 + 4] == 80,
          "friendly9 kernel multiplies and truncates per cell, not additive negative pressure");
    i.humans.push_back({1, true, {4, 4}});
    check(prepare_combat_influence(i).candidate->human_field[4 * 12 + 4] == 64,
          "two friendly center reductions are successive0.8 truncations");
    i.humans.clear();
    for (int state = 0; state <= 20; ++state) {
        i.monsters[0].state = state;
        const bool excluded = state == 2 || state == 3 || state == 8 || state == 9 || state == 14 ||
                              state == 15 || state == 16;
        check(prepare_combat_influence(i).candidate->human_field[4 * 12 + 4] ==
                  (excluded ? 0 : 100),
              "influence eligibility uses previous area and exact state exclusions");
    }
    i.monsters[0] = {1, true, {3, 3}};
    i.legacy_surface[5 * 6 + 5] = 3;
    r = prepare_combat_influence(i);
    check(r.candidate && r.candidate->human_field[3 * 12 + 3] == 0 &&
              r.candidate->human_field[4 * 12 + 4] == 50,
          "fixed grid5,5 mask clears half-grid2..3, not doubled coordinate10..11");
    i.monsters[0].previous_move_area = false;
    check(prepare_combat_influence(i).candidate->human_field[4 * 12 + 4] == 0,
          "invalid previous K actor does not add influence");
    i.map_width = 0;
    check(prepare_combat_influence(i).error == CombatAiError::invalid_input,
          "invalid map shape cannot partially prepare influence");
}
} // namespace
int main() {
    for (int growth = 0; growth <= 600; ++growth)
        for (int base : {0, 1, 99, 100}) {
            const int normal_tier = growth < 100   ? 1
                                    : growth < 200 ? 2
                                    : growth < 300 ? 3
                                    : growth < 400 ? 4
                                                   : 5;
            const int boss_tier = growth < 9 ? growth + 1 : 10;
            check(prepare_monster_growth(base, growth, 0, false) == base * normal_tier &&
                      prepare_monster_growth(base, growth, 0, true) == base * boss_tier,
                  "growth0 monster normal100 thresholds and boss1..10 multiplier");
            check(prepare_monster_growth(base, growth, 2, false) == normal_tier &&
                      prepare_monster_growth(base, growth, 2, true) == boss_tier,
                  "growth2 tier ignores base");
            const int reward_percent = growth < 100   ? 100
                                       : growth < 200 ? 120
                                       : growth < 300 ? 140
                                       : growth < 400 ? 160
                                       : growth < 500 ? 180
                                                      : 200;
            check(prepare_monster_growth(base, growth, 1, false) == base * reward_percent / 100,
                  "growth1 ordinary reward percentage, truncation and500 cap");
        }
    check(prepare_monster_growth(100, 8, 1, true) == 188 &&
              prepare_monster_growth(100, 9, 1, true) == 200,
          "boss reward integer mapping0..9, not float continuous multiplier");
    check(!prepare_monster_growth(std::numeric_limits<int>::max(), 9, 0, true) &&
              !prepare_monster_growth(100, -1, 0, false),
          "growth overflow and invalid input refused");
    choices();
    timing_and_guards();
    movement();
    damage();
    influence();
    std::cout << checks << " checks passed\n";
}
