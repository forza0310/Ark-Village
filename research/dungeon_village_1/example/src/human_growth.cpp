#include "dungeon_village_reference/human_growth.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
bool fits(std::int64_t v) {
    return v >= std::numeric_limits<int>::min() && v <= std::numeric_limits<int>::max();
}
bool valid(const HumanDefinitionStatsInput &i, const std::vector<HumanProfessionRule> &p) {
    if (p.empty() || p.size() > 1000 || p.size() != i.profession_levels.size() ||
        i.current_profession < 0 || static_cast<std::size_t>(i.current_profession) >= p.size() ||
        i.legacy_u < 0 || i.spell_professions.size() > 4)
        return false;
    for (std::size_t n = 0; n < p.size(); ++n) {
        if (i.profession_levels[n] < 1 || i.profession_levels[n] > 10 || p[n].difficulty < 1 ||
            p[n].difficulty > 5)
            return false;
        for (const int v : p[n].maximum_growth)
            if (v < 0)
                return false;
        for (const int v : p[n].attribute_percent)
            if (v < 0)
                return false;
    }
    for (const int v : i.base)
        if (v < 0)
            return false;
    for (const int v : i.spell_professions)
        if (v < 0 || static_cast<std::size_t>(v) >= p.size())
            return false;
    return true;
}
} // namespace
HumanStatsResult derive_human_stats(const HumanDefinitionStatsInput &i,
                                    const std::vector<HumanProfessionRule> &p) {
    if (!valid(i, p))
        return {HumanGrowthError::invalid_input, std::nullopt};
    HumanDerivedStats c;
    for (std::size_t n = 0; n < p.size(); ++n)
        for (std::size_t a = 0; a < 6; ++a) {
            const auto growth =
                static_cast<std::int64_t>(i.profession_levels[n] - 1) * p[n].maximum_growth[a];
            const auto sum = static_cast<std::int64_t>(c.growth[a]) + growth / 9;
            if (!fits(growth) || !fits(sum))
                return {HumanGrowthError::numeric_overflow, std::nullopt};
            c.growth[a] = static_cast<int>(sum);
        }
    for (std::size_t a = 0; a < 6; ++a) {
        const auto subtotal = static_cast<std::int64_t>(i.base[a]) + c.growth[a];
        const auto sum = subtotal + i.extra[a];
        if (!fits(subtotal) || !fits(sum))
            return {HumanGrowthError::numeric_overflow, std::nullopt};
        const auto scaled = sum * p[i.current_profession].attribute_percent[a];
        if (!fits(scaled))
            return {HumanGrowthError::numeric_overflow, std::nullopt};
        const auto first = std::min<std::int64_t>(scaled / 100, 9999);
        const auto multiplier = 100 + static_cast<std::int64_t>(i.legacy_u / 10) * 10;
        if (!fits(multiplier))
            return {HumanGrowthError::numeric_overflow, std::nullopt};
        const auto second = first * multiplier;
        if (!fits(second))
            return {HumanGrowthError::numeric_overflow, std::nullopt};
        c.attributes[a] = static_cast<int>(std::min<std::int64_t>(second / 100, 9999));
    }
    constexpr std::size_t mapping[]{0, 1, 3, 4};
    for (std::size_t a = 0; a < 4; ++a) {
        std::int64_t value = c.attributes[mapping[a]];
        for (const auto &e : i.equipment)
            if (e) {
                value += (*e)[a];
                if (!fits(value))
                    return {HumanGrowthError::numeric_overflow, std::nullopt};
            }
        c.combat[a] = static_cast<int>(std::min<std::int64_t>(value, 9999));
    }
    c.available_spells = i.learned_spells;
    for (std::size_t n = 0; n < i.spell_professions.size(); ++n) {
        const int profession = i.spell_professions[n];
        if (profession == i.current_profession || i.profession_levels[profession] == 10)
            c.available_spells[n] = true;
    }
    return {HumanGrowthError::none, c};
}
std::optional<int> human_growth_threshold(int level, int difficulty) {
    if (level < 1 || level > 10 || difficulty < 1 || difficulty > 5)
        return std::nullopt;
    const int base = level * 3 * level + level * 4 + 5;
    return base + (difficulty - 1) * (base * 5) / 4;
}
HumanRewardResult prepare_human_reward(const HumanDefinitionStatsInput &definition,
                                       const std::vector<HumanProfessionRule> &professions,
                                       int satisfaction, int celebrations, int pending_completion,
                                       int satisfaction_request, int effort_request,
                                       bool celebrate) {
    if (definition.legacy_u < 0 || definition.legacy_u > 100 || satisfaction < 0 ||
        satisfaction > 100 || celebrations < 0 || satisfaction_request < 0 || effort_request < 0)
        return {HumanGrowthError::invalid_input, {}};
    const auto C = static_cast<std::int64_t>(satisfaction) + satisfaction_request;
    const auto u = static_cast<std::int64_t>(definition.legacy_u) + effort_request;
    const auto E = static_cast<std::int64_t>(celebrations) + (celebrate ? 1 : 0);
    const auto completion = static_cast<std::int64_t>(pending_completion) + satisfaction_request;
    if (!fits(C) || !fits(u) || !fits(E) || !fits(completion))
        return {HumanGrowthError::numeric_overflow, {}};
    HumanRewardCandidate c;
    c.definition = definition;
    c.satisfaction = static_cast<int>(std::min<std::int64_t>(C, 100));
    c.definition.legacy_u = static_cast<int>(std::min<std::int64_t>(u, 100));
    c.celebrations = static_cast<int>(E);
    c.pending_completion = static_cast<int>(completion);
    c.reward_display = {{{satisfaction, definition.legacy_u},
                         {c.satisfaction, c.definition.legacy_u},
                         {satisfaction_request, effort_request}}};
    if (c.definition.legacy_u / 10 > definition.legacy_u / 10) {
        const auto before = derive_human_stats(definition, professions);
        const auto after = derive_human_stats(c.definition, professions);
        if (!before.candidate || !after.candidate)
            return {before.candidate ? after.error : before.error, {}};
        std::array<std::array<int, 4>, 3> display{
            before.candidate->combat, after.candidate->combat, {}};
        for (std::size_t n = 0; n < 4; ++n) {
            const auto delta = static_cast<std::int64_t>(display[1][n]) - display[0][n];
            if (!fits(delta))
                return {HumanGrowthError::numeric_overflow, {}};
            display[2][n] = static_cast<int>(delta);
        }
        c.effort_display = display;
        c.derived = after.candidate;
    }
    return {HumanGrowthError::none, std::move(c)};
}
HumanGrowthResult prepare_human_growth(const HumanGrowthInput &i) {
    if (!valid(i.definition, i.professions) || i.experience < 0 || i.pending.amount < 0 ||
        i.pending.counter == std::numeric_limits<int>::max())
        return {HumanGrowthError::invalid_input, std::nullopt};
    HumanGrowthCandidate c;
    c.definition = i.definition;
    c.experience = i.experience;
    c.pending = i.pending;
    c.effects = i.effects;
    c.notice_pending = i.notice_pending;
    c.notice_attributes = i.notice_attributes;
    if (c.pending.amount == 0)
        return {HumanGrowthError::none, c};
    const auto step = advance_delayed_reward(c.pending);
    if (!step)
        return {HumanGrowthError::invalid_input, std::nullopt};
    c.pending = step->state;
    if (c.pending.counter < 0)
        return {HumanGrowthError::none, c};
    const auto experience = static_cast<std::int64_t>(c.experience) + step->increment;
    if (!fits(experience))
        return {HumanGrowthError::numeric_overflow, std::nullopt};
    c.experience = static_cast<int>(experience);
    const int job = c.definition.current_profession;
    auto &level = c.definition.profession_levels[job];
    bool seen109 = i.event109_seen;
    do {
        const int threshold = *human_growth_threshold(level, i.professions[job].difficulty);
        if (c.experience < threshold)
            break;
        c.experience -= threshold; // Even level10 subtracts ONCE, then exits without a real gain.
        if (level < 10) {
            ++level;
            ++c.levels_gained;
            const auto stats = derive_human_stats(c.definition, i.professions);
            if (!stats.candidate)
                return {stats.error, std::nullopt};
            c.stats = stats.candidate;
            if (!seen109) {
                c.requests.push_back({HumanGrowthRequestKind::event109, job});
                seen109 = true;
            }
        }
    } while (level != 10);
    if (c.levels_gained != 0) {
        c.pending = {};
        for (auto &r : c.notice_attributes)
            r = {-1, -1};
        auto previous = c.definition;
        --previous.profession_levels[job];
        const auto old_stats = derive_human_stats(previous, i.professions);
        if (!old_stats.candidate)
            return {old_stats.error, std::nullopt};
        std::vector<std::array<int, 2>> changes;
        for (int a = 0; a < 4; ++a) {
            const auto difference =
                static_cast<std::int64_t>(c.stats->combat[a]) - old_stats.candidate->combat[a];
            if (!fits(difference))
                return {HumanGrowthError::numeric_overflow, std::nullopt};
            if (difference > 0)
                changes.push_back({a, static_cast<int>(difference)});
        }
        for (std::size_t a = 0; a + 1 < changes.size(); ++a)
            for (std::size_t b = changes.size(); b-- > a + 1;)
                if (changes[b][1] > changes[a][1])
                    std::swap(changes[a], changes[b]);
        for (std::size_t a = 0; a < changes.size(); ++a)
            c.notice_attributes[a] = changes[a];
        c.requests.push_back({HumanGrowthRequestKind::report_growth, job});
        c.notice_pending = true;
        bool has14{};
        for (const auto &effect : c.effects.display)
            if (effect.size() < 2 || effect[0] < 0 || effect[0] > 26)
                return {HumanGrowthError::invalid_input, std::nullopt};
        c.effects.display.erase(std::remove_if(c.effects.display.begin(), c.effects.display.end(),
                                               [](const auto &e) { return e[0] == 24; }),
                                c.effects.display.end());
        for (const auto &e : c.effects.display)
            if (e[0] == 14)
                has14 = true;
        if (!has14)
            c.effects.display.push_back({14, 0});
        if (level == 10) {
            c.requests.push_back({HumanGrowthRequestKind::page70, job});
            if (!i.professions[job].unlocked) {
                c.requests.push_back({HumanGrowthRequestKind::unlock_profession, job});
                c.requests.push_back({HumanGrowthRequestKind::page94, job});
                if (!i.event113_seen)
                    c.requests.push_back({HumanGrowthRequestKind::event113, job});
                c.requests.push_back({HumanGrowthRequestKind::notice33, job});
            }
        }
    } else if (c.pending.counter >= 9) {
        c.pending = {};
    }
    return {HumanGrowthError::none, c};
}
} // namespace dungeon_village_reference
