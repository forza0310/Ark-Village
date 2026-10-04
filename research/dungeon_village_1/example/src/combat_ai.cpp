#include "dungeon_village_reference/combat_ai.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dungeon_village_reference {
bool valid_combat_influence_field(const CombatInfluenceCandidate &f) {
    return f.width > 0 && f.height > 0 &&
           static_cast<std::uint64_t>(f.width) * f.height <= 4000000 &&
           static_cast<std::uint64_t>(f.width) * f.height == f.human_field.size() &&
           f.human_field.size() == f.monster_field.size() &&
           std::none_of(f.human_field.begin(), f.human_field.end(), [](int v) { return v < 0; }) &&
           std::none_of(f.monster_field.begin(), f.monster_field.end(),
                        [](int v) { return v < 0; });
}
std::optional<int> prepare_monster_growth(int base, int growth, int category, bool boss) {
    if (base < 0 || growth < 0 || category < 0 || category > 2)
        return std::nullopt;
    const int tier = std::min(growth / 100 + 1, 5);
    const int boss_tier =
        static_cast<int>(std::min<std::int64_t>(static_cast<std::int64_t>(growth) + 1, 10));
    std::int64_t value{};
    if (category == 2)
        value = boss ? boss_tier : tier;
    else if (category == 0)
        value = static_cast<std::int64_t>(base) * (boss ? boss_tier : tier);
    else {
        const int percentage =
            boss ? 100 + std::min(growth, 9) * 100 / 9 : 100 + std::min(growth / 100, 5) * 20;
        value = static_cast<std::int64_t>(base) * percentage / 100;
    }
    if (value > std::numeric_limits<int>::max())
        return std::nullopt;
    return static_cast<int>(value);
}
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

namespace {
std::optional<int> float_integer(float value) {
    if (!std::isfinite(value) || static_cast<double>(value) >= 2147483648.0 || value < 0)
        return std::nullopt;
    return static_cast<int>(value);
}
DamageResult jitter_damage(int base, std::optional<int> ticket, bool human_boost,
                           bool monster_boost, ActorKind kind) {
    const auto span = std::max<std::int64_t>(static_cast<std::int64_t>(base) * 2 / 10, 2);
    if (!ticket)
        return {CombatAiError::missing_ticket, std::nullopt};
    if (*ticket < 0 || *ticket >= span)
        return {CombatAiError::invalid_ticket, std::nullopt};
    const auto raw =
        std::max<std::int64_t>(static_cast<std::int64_t>(base) + *ticket - span / 2, 0);
    if (raw > std::numeric_limits<int>::max())
        return {CombatAiError::invalid_input, std::nullopt};
    int result = static_cast<int>(raw);
    if (human_boost) {
        const auto adjusted =
            float_integer(static_cast<float>(result) * (kind == ActorKind::human ? 1.2F : 0.5F));
        if (!adjusted)
            return {CombatAiError::invalid_input, std::nullopt};
        result = *adjusted;
    }
    if (monster_boost) {
        const auto adjusted =
            float_integer(static_cast<float>(result) * (kind == ActorKind::human ? 0.5F : 1.2F));
        if (!adjusted)
            return {CombatAiError::invalid_input, std::nullopt};
        result = *adjusted;
    }
    return {CombatAiError::none, DamageCandidate{base, static_cast<int>(span), result}};
}
} // namespace
DamageResult prepare_physical_damage(const PhysicalDamageInput &i) {
    if ((i.kind != ActorKind::human && i.kind != ActorKind::monster) || i.effective_attack < 0 ||
        i.effective_defense < 0)
        return {CombatAiError::invalid_input, std::nullopt};
    const auto low = static_cast<std::int64_t>(i.effective_attack) * 30 / 100;
    const auto high = static_cast<std::int64_t>(i.effective_attack) * 120 / 100;
    if (high > std::numeric_limits<int>::max())
        return {CombatAiError::invalid_input, std::nullopt};
    const float ratio = i.effective_defense == 0
                            ? 1.3F
                            : static_cast<float>(i.effective_attack / i.effective_defense);
    // c.d.a uses float endpoints, with out-of-range branches returning the exact endpoint.
    const float mapped =
        ratio < 0.6F   ? static_cast<float>(low)
        : ratio > 1.3F ? static_cast<float>(high)
                       : static_cast<float>(low) +
                             ((ratio - 0.6F) * static_cast<float>(high - low)) / (1.3F - 0.6F);
    const auto base = float_integer(mapped);
    if (!base)
        return {CombatAiError::invalid_input, std::nullopt};
    return jitter_damage(std::max(*base, 2), i.jitter_ticket, i.human_boost, i.monster_boost,
                         i.kind);
}
SpellDamageResult prepare_spell_damage(const SpellDamageInput &i) {
    if (i.magic < 0)
        return {CombatAiError::invalid_input, std::nullopt};
    int count = 0;
    for (bool learned : i.learned)
        count += learned;
    if (count == 0)
        return {CombatAiError::invalid_input, std::nullopt};
    if (!i.spell_ticket || !i.enhancement_ticket)
        return {CombatAiError::missing_ticket, std::nullopt};
    if (*i.spell_ticket < 0 || *i.spell_ticket >= count || *i.enhancement_ticket < 0 ||
        *i.enhancement_ticket >= 100)
        return {CombatAiError::invalid_ticket, std::nullopt};
    int selected = 0;
    int remaining = *i.spell_ticket;
    for (int n = 0; n < 3; ++n)
        if (i.learned[n] && remaining-- == 0) {
            selected = n;
            break;
        }
    const int chance = 5 + (std::clamp(i.magic, 100, 1000) - 100) * 45 / 900;
    const bool enhanced = *i.enhancement_ticket < chance;
    static constexpr float coefficients[2][3] = {{0.5F, 0.7F, 0.9F}, {1.1F, 1.3F, 1.5F}};
    const auto base = float_integer(coefficients[enhanced][selected] * static_cast<float>(i.magic));
    if (!base)
        return {CombatAiError::invalid_input, std::nullopt};
    const auto damage = jitter_damage(*base == 0 ? 2 : *base, i.jitter_ticket, i.human_boost, false,
                                      ActorKind::human);
    if (!damage.candidate)
        return {damage.error, std::nullopt};
    return {CombatAiError::none,
            SpellDamageCandidate{*damage.candidate, selected + (enhanced ? 7 : 4)}};
}

CombatInfluenceResult prepare_combat_influence(const CombatInfluenceInput &i) {
    const auto cells = static_cast<std::int64_t>(i.map_width) * i.map_height;
    if (i.map_width <= 0 || i.map_height <= 0 || cells > 1000000 ||
        i.legacy_surface.size() != static_cast<std::size_t>(cells))
        return {CombatAiError::invalid_input, std::nullopt};
    for (const auto *roster : {&i.humans, &i.monsters})
        for (const auto &a : *roster)
            if (a.state < 0 || a.state > 20)
                return {CombatAiError::invalid_input, std::nullopt};
    CombatInfluenceCandidate c;
    c.width = i.map_width * 2;
    c.height = i.map_height * 2;
    c.human_field.resize(static_cast<std::size_t>(cells) * 4);
    c.monster_field.resize(c.human_field.size());
    const auto eligible = [](const InfluenceActor &a) {
        const int s = a.state;
        return a.previous_move_area && s != 2 && s != 3 && s != 8 && s != 9 && s != 14 && s != 15 &&
               s != 16;
    };
    static constexpr int weights[25] = {30, 30, 30, 30, 30, 30, 50, 50, 50, 30, 30, 50, 100,
                                        50, 30, 30, 50, 50, 50, 30, 30, 30, 30, 30, 30};
    static constexpr float friendly[9] = {0.95F, 0.9F, 0.95F, 0.9F, 0.8F, 0.9F, 0.95F, 0.9F, 0.95F};
    const auto index = [&](std::int64_t x, std::int64_t y) -> std::optional<std::size_t> {
        if (x < 0 || x >= c.width || y < 0 || y >= c.height)
            return std::nullopt;
        return static_cast<std::size_t>(y * c.width + x);
    };
    const auto add = [&](std::vector<int> &field, const std::vector<InfluenceActor> &roster) {
        for (const auto &a : roster)
            if (eligible(a))
                for (int n = 0; n < 25; ++n) {
                    const auto p = index(static_cast<std::int64_t>(a.half_cell.x) + n % 5 - 2,
                                         static_cast<std::int64_t>(a.half_cell.y) + 2 - n / 5);
                    if (p) {
                        if (field[*p] > std::numeric_limits<int>::max() - weights[n])
                            return false;
                        field[*p] += weights[n];
                    }
                }
        return true;
    };
    const auto multiply = [&](std::vector<int> &field, const std::vector<InfluenceActor> &roster) {
        for (const auto &a : roster)
            if (eligible(a))
                for (int n = 0; n < 9; ++n) {
                    const auto p = index(static_cast<std::int64_t>(a.half_cell.x) + n % 3 - 1,
                                         static_cast<std::int64_t>(a.half_cell.y) + 1 - n / 3);
                    if (p)
                        field[*p] = static_cast<int>(static_cast<float>(field[*p]) * friendly[n]);
                }
    };
    if (!add(c.human_field, i.monsters) || !add(c.monster_field, i.humans))
        return {CombatAiError::invalid_input, std::nullopt};
    multiply(c.human_field, i.humans);
    multiply(c.monster_field, i.monsters);
    for (int y = 0; y < i.map_height; ++y)
        for (int x = 0; x < i.map_width; ++x) {
            const int surface = i.legacy_surface[static_cast<std::size_t>(y) * i.map_width + x];
            if (surface == 0 || surface == 3)
                for (int dy = 0; dy <= 1; ++dy)
                    for (int dx = 0; dx <= 1; ++dx) {
                        const auto p = *index(x / 2 + dx, y / 2 + dy);
                        c.human_field[p] = c.monster_field[p] = 0;
                    }
        }
    return {CombatAiError::none, c};
}
} // namespace dungeon_village_reference
