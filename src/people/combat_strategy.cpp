// Adapted from published research e8bd81c; independent standard-C++ product rules.
// State1 policy and nine-direction scoring only; no motion or attack commit.
#include "ark/people/combat_ai.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace ark::people {
CombatStrategyResult prepare_combat_strategy(const CombatStrategyInput &i) {
    if ((i.kind != ActorKind::human && i.kind != ActorKind::monster) || i.action < 0 ||
        i.action > 11 || i.monster_posture < 0 || i.monster_posture > 2 || i.group_tick < 0 ||
        i.attack_slot < 0 || i.group_cycle < 0 || i.group_cycle > 2 || i.profession_role < 0 ||
        i.profession_role > 4 || !std::isfinite(i.sensed_distance) || i.sensed_distance < 0 ||
        !std::isfinite(i.fresh_distance) || i.fresh_distance < 0 || i.weapon_range < 0 ||
        i.monster_range < 0 || i.weapon_kind < 0 || i.weapon_kind > 3)
        return {CombatAiError::invalid_input, std::nullopt};
    CombatStrategyCandidate c;
    c.reset_encounter_idle = i.kind == ActorKind::monster; // Owner applies only when db exists.
    if ((i.flags & 4U) || i.action == 4)
        return {CombatAiError::none, c};
    if (!i.in_move_area || !i.sensed_enemy || !i.same_town_side) {
        c.decision =
            i.kind == ActorKind::human ? CombatDecision::battle_prepare : CombatDecision::baseline;
        return {CombatAiError::none, c};
    }
    if (!i.fresh_enemy)
        return {CombatAiError::none, c};
    if (!(i.flags & 128U)) {
        c.decision = CombatDecision::join_group;
        return {CombatAiError::none, c};
    }
    if (i.kind == ActorKind::monster && i.monster_posture == 2) {
        c.decision = CombatDecision::low_influence;
        return {CombatAiError::none, c};
    }
    c.face_enemy =
        static_cast<std::int64_t>(i.group_tick) >= static_cast<std::int64_t>(i.attack_slot) - 16 &&
        i.group_tick < i.attack_slot;
    c.clear_animation_flag = c.face_enemy;
    c.telegraph =
        static_cast<std::int64_t>(i.group_tick) == static_cast<std::int64_t>(i.attack_slot) - 6;
    const bool physical = i.sensed_distance < static_cast<float>(i.weapon_range) - 10.0F;
    const bool offensive =
        i.sensed_distance < 300.0F && (i.spells[0] || i.spells[1] || i.spells[2]);
    const bool healing = i.healing_target && i.spells[3];
    const int column = offensive || healing ? physical ? 2 : 1 : physical ? 2 : 0;
    if (i.group_tick == i.attack_slot) {
        if (i.kind == ActorKind::monster) {
            if (i.fresh_distance < static_cast<float>(i.monster_range))
                c.decision = CombatDecision::physical_attack;
        } else {
            static constexpr int weights[5][3] = {
                {100, 20, 90}, {100, 80, 50}, {100, 80, 50}, {100, 55, 70}, {100, 40, 70}};
            static constexpr int choices[3][2] = {{0, 0}, {1, 0}, {2, 1}};
            if (!i.policy_ticket)
                return {CombatAiError::missing_ticket, std::nullopt};
            if (*i.policy_ticket < 0 || *i.policy_ticket >= 100)
                return {CombatAiError::invalid_ticket, std::nullopt};
            c.consumed_policy_ticket = true;
            c.policy_column = column;
            int choice = choices[column][*i.policy_ticket >= weights[i.profession_role][column]];
            if (choice == 1 && !offensive && !healing)
                choice = physical ? 2 : 0;
            if (choice == 0)
                c.decision = CombatDecision::approach;
            else if (choice == 2)
                c.decision = CombatDecision::physical_attack;
            else {
                bool use_healing = healing && !offensive;
                if (offensive && healing) {
                    if (!i.healing_ticket)
                        return {CombatAiError::missing_ticket, std::nullopt};
                    if (*i.healing_ticket < 0 || *i.healing_ticket >= 10)
                        return {CombatAiError::invalid_ticket, std::nullopt};
                    c.consumed_healing_ticket = true;
                    use_healing = *i.healing_ticket >= 4;
                }
                c.decision =
                    use_healing ? CombatDecision::healing_spell : CombatDecision::offensive_spell;
            }
        }
    } else if (i.kind == ActorKind::human) {
        if (i.group_cycle == 2 &&
            (i.weapon_kind == 1 || ((offensive || healing) && i.profession_role == 1)))
            c.decision = CombatDecision::low_influence;
        else if (column == 0 || (column == 1 && (i.profession_role == 0 || i.profession_role == 4)))
            c.decision = CombatDecision::approach;
    } else if (i.fresh_distance >= static_cast<float>(i.monster_range)) {
        c.decision =
            i.monster_posture == 0 ? CombatDecision::approach : CombatDecision::low_influence;
    }
    return {CombatAiError::none, c};
}

CombatMoveResult prepare_combat_step(const std::array<CombatMoveSample, 9> &samples,
                                     bool low_influence) {
    CombatMoveCandidate c;
    for (const auto &s : samples)
        if (!std::isfinite(s.enemy_distance) || s.enemy_distance < 0 || s.influence < 0)
            return {CombatAiError::invalid_input, std::nullopt};
    for (std::size_t n = 0; n < samples.size(); ++n) {
        const auto &s = samples[n];
        if (!s.in_half_grid || !s.in_encounter_square)
            continue;
        const int influence = std::min(s.influence, 100);
        const float distance = s.in_half_grid ? s.enemy_distance : 1000.0F;
        const float center = samples[4].in_half_grid ? samples[4].enemy_distance : 1000.0F;
        const int closeness =
            static_cast<int>(std::clamp((center - distance) * 100.0F / 50.0F, 0.0F, 100.0F));
        c.scores[n] = low_influence ? influence : (closeness + influence) / 2;
    }
    int best = low_influence ? std::numeric_limits<int>::max() : 0;
    for (std::size_t n = 0; n < c.scores.size(); ++n) {
        if (low_influence ? c.scores[n] < best : c.scores[n] > best) {
            best = c.scores[n];
            c.selected = n;
        }
    }
    if (c.selected == 4)
        c.selected.reset();
    return {CombatAiError::none, c};
}

} // namespace ark::people
