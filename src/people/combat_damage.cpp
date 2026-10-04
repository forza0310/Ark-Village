// Adapted from published research e8bd81c; independent standard-C++ product rules.
// Explicit random tickets preserve source integer/float truncation and boost order.
#include "ark/people/combat_ai.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace ark::people {
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

} // namespace ark::people
