#include "ark/simulation/rules/actor_ai.hpp"

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
void gates() {
    for (auto kind : {ActorKind::human, ActorKind::monster})
        for (unsigned flags : {0U, 512U, 1024U, 1536U, 128U})
            for (int counter : {0, 4, 5, 6, 900})
                for (unsigned mask = 0; mask < 32; ++mask) {
                    const BattleGateInput input{kind,
                                                flags,
                                                counter,
                                                (mask & 1) != 0,
                                                (mask & 2) != 0,
                                                (mask & 4) != 0,
                                                (mask & 8) != 0,
                                                (mask & 16) != 0};
                    const auto result = prepare_battle_gate(input);
                    const bool blocked = (flags & 1536) != 0;
                    const bool human = kind == ActorKind::human;
                    const bool same = input.inside_town == input.enemy_inside_town;
                    const bool allowed = !blocked && counter >= 5 && input.in_move_area &&
                                         (!human || !input.has_object) && input.has_enemy && same;
                    std::optional<int> diagnostic;
                    if (!blocked) {
                        if (counter < 5)
                            diagnostic = 1;
                        else if (!input.in_move_area)
                            diagnostic = 2;
                        else if (human)
                            diagnostic = input.has_object ? 3 : !input.has_enemy ? 4 : same ? 6 : 5;
                    }
                    check(result.candidate && result.candidate->allowed == allowed &&
                              result.candidate->legacy_diagnostic_write == diagnostic,
                          "G guard order and absent diagnostic write");
                }
    check(prepare_battle_gate({ActorKind::human, 0, -1}).error == ActorAiError::invalid_input,
          "negative old B rejected");
}
void enemies() {
    EnemySelectionInput input{
        {0, 0},
        false,
        9,
        {{{1}, 1, true, {4, 0}, 9}, {{2}, 1, true, {1, 0}, 9}, {{3}, 1, true, {0, 1}, 9}}};
    auto result = select_combat_enemy(input);
    check(result.candidate && result.candidate->id.value == 3,
          "rightmost tied minimum displaces larger head");
    input.opposite_roster[0].position = {1, 0};
    check(select_combat_enemy(input).candidate->id.value == 1, "already-minimal head retains tie");
    for (int state = 0; state <= 20; ++state) {
        input.opposite_roster = {{{1}, state, true, {3, 4}, 9}};
        const bool excluded = state == 2 || state == 3 || state == 8 || state == 9 || state == 14 ||
                              state == 15 || state == 16;
        result = select_combat_enemy(input);
        check(result.error == ActorAiError::none && bool(result.candidate) == !excluded,
              "exact seven excluded enemy states");
        if (result.candidate)
            check(result.candidate->world_distance == 5, "world Euclidean distance");
    }
    input.opposite_roster = {{{1}, 1, true, {3, 4}, 10}};
    check(!select_combat_enemy(input).candidate, "ordinary encounter identity required");
    input.active_battle_group = true;
    check(bool(select_combat_enemy(input).candidate), "group roster bypasses encounter filter");
    input.opposite_roster[0].in_move_area = false;
    check(!select_combat_enemy(input).candidate, "invalid movement area excluded");
    input.opposite_roster = {{{1}, 1, true, {3, 4}, 9}, {{1}, 1, true, {1, 1}, 9}};
    check(select_combat_enemy(input).error == ActorAiError::invalid_input,
          "same ID conflicting position rejected");
    input.opposite_roster = {{{1}, 1, true, {4, 0}, 9},
                             {{2}, 1, true, {1, 0}, 9},
                             {{3}, 1, true, {0, 1}, 9},
                             {{2}, 1, true, {1, 0}, 9}};
    result = select_combat_enemy(input);
    check(result.candidate && result.candidate->id == CharacterId{2},
          "group repeated rightmost minimum preserved, not sorted/deduplicated");
    input.active_battle_group = false;
    check(select_combat_enemy(input).error == ActorAiError::invalid_input,
          "ordinary global roster still rejects duplicate identity");
    input.opposite_roster.clear();
    input.position.x = std::numeric_limits<float>::quiet_NaN();
    check(select_combat_enemy(input).error == ActorAiError::invalid_input,
          "nonfinite world rejected");
}
void human_idle() {
    for (unsigned mask = 0; mask < 64; ++mask)
        for (int hp : {0, 24, 25, 50, 100, 150})
            for (int effort : {-10, 0, 1, 50, 100, 150})
                for (int counter : {0, 99, 100, 499, 500, 1199, 1200}) {
                    HumanIdleInput input{(mask & 1) ? 16U : 0U,
                                         (mask & 2) != 0,
                                         (mask & 4) != 0,
                                         (mask & 8) != 0,
                                         (mask & 16) != 0,
                                         (mask & 32) != 0,
                                         counter,
                                         hp,
                                         100,
                                         effort};
                    const auto result = prepare_human_idle(input);
                    check(bool(result.candidate), "idle accepts clamped HP and effort");
                    const auto &c = *result.candidate;
                    const int bounded_hp = hp > 100 ? 100 : hp;
                    const int bounded_effort = effort < 0 ? 0 : effort > 100 ? 100 : effort;
                    const int threshold = 500 + ((bounded_hp + bounded_effort) / 2) * 7;
                    check(c.hp_percent == bounded_hp && c.reselect_threshold == threshold,
                          "integer interpolation boundaries");
                    if (mask & 1)
                        check(c.action == IdleAiAction::keep && !c.run_spawn_probe,
                              "flag16 blocks");
                    else if ((mask & 6) == 6)
                        check(c.activity == 1 && !c.run_spawn_probe, "task takes priority");
                    else if (mask & 8)
                        check(c.action == IdleAiAction::battle_prepare && !c.run_spawn_probe,
                              "F before carrying");
                    else if (mask & 16)
                        check(c.activity == 4 && c.clear_path && !c.run_spawn_probe,
                              "rescue activity clears path");
                    else {
                        std::optional<int> activity;
                        if ((mask & 32) && counter % 100 == 0)
                            activity = 6;
                        if (bounded_hp < 25 || counter >= threshold)
                            activity = 0;
                        check(c.activity == activity && c.run_spawn_probe,
                              "activity0 overwrites nearby6 without skipping L");
                    }
                }
    check(prepare_human_idle({0, false, false, false, false, false, 0, 0, 0, 0}).error ==
              ActorAiError::invalid_input,
          "zero capacity rejected");
    HumanIdleInput input;
    input.capacity_hp = 100;
    input.reported_hp = -1;
    check(prepare_human_idle(input).candidate->hp_percent == 0, "negative HP clamps to zero");
}
void monsters() {
    for (int mode = 0; mode < 5; ++mode)
        for (bool gate : {false, true})
            for (bool path : {false, true})
                for (int counter : {0, 499, 500, 1500}) {
                    auto result = prepare_monster_idle({mode, gate, path, counter});
                    const bool advances = mode == 1 || mode == 4;
                    const auto action = !advances && gate ? MonsterIdleAction::enter_battle
                                        : mode == 1 && !path && counter >= 500
                                            ? MonsterIdleAction::cleanup
                                            : MonsterIdleAction::keep;
                    check(result.candidate && result.candidate->advance_path == advances &&
                              result.candidate->action == action,
                          "T0..4 and P true never propagated as deletion");
                }
    check(prepare_monster_idle({5}).error == ActorAiError::invalid_input, "unknown mode rejected");
}
} // namespace
int main() {
    gates();
    enemies();
    human_idle();
    monsters();
    std::cout << checks << " checks passed\n";
}
