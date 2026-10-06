// 村办领域候选：目录、开展与单个人物效果；页面和世界事务由唯一 Owner 组合。
#include "dungeon_village_reference/world_village_activity.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace dungeon_village_reference {
namespace {
bool fits(std::int64_t value) {
    return value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max();
}
bool valid(const WorldVillageActivityDefinition &a) {
    return a.identity >= 0 && a.status >= 0 && a.held >= 0 && a.kind >= 0 && a.kind <= 6 &&
           a.points >= 0 && (a.kind != 1 || (a.attribute >= 0 && a.attribute < 6));
}
} // namespace

std::optional<std::vector<int>>
catalogue_world_village_activities(const std::vector<WorldVillageActivityDefinition> &definitions) {
    std::vector<int> result;
    std::set<int> seen;
    for (const auto &a : definitions) {
        if (!valid(a) || !seen.insert(a.identity).second)
            return {};
        if (a.identity != 27 && a.status == 1 && !((a.flags & 2U) && a.held > 0) && !(a.flags & 4U))
            result.push_back(a.identity);
    }
    return result;
}
WorldVillageActivityGate check_world_village_activity(const WorldVillageActivityDefinition &a,
                                                      int quarter_slots, int village_points) {
    if (!valid(a) || village_points < 0 || village_points > 999)
        return {WorldVillageActivityError::invalid_input, {}};
    if (quarter_slots <= 0)
        return {WorldVillageActivityError::none, WorldVillageActivityDenial::no_quarter_slots50};
    if (village_points < a.points)
        return {WorldVillageActivityError::none, WorldVillageActivityDenial::insufficient_points12};
    return {};
}
WorldVillageActivityStartResult
prepare_world_village_activity_start(const WorldVillageActivityDefinition &a, int village_points,
                                     int events_held) {
    if (!valid(a) || village_points < 0 || village_points > 999 || events_held < 0)
        return {WorldVillageActivityError::invalid_input, {}};
    if (a.held == std::numeric_limits<int>::max() || events_held == std::numeric_limits<int>::max())
        return {WorldVillageActivityError::overflow, {}};
    auto next = a;
    ++next.held;
    next.flags |= 4U;
    const auto remaining = static_cast<std::int64_t>(village_points) - a.points;
    return {WorldVillageActivityError::none,
            WorldVillageActivityStart{next,
                                      static_cast<int>(std::clamp<std::int64_t>(remaining, 0, 999)),
                                      events_held + 1}};
}
std::optional<WorldVillageActivityAnimation> world_village_activity_animation(int counter,
                                                                              bool confirm) {
    if (counter < 0)
        return {};
    return WorldVillageActivityAnimation{counter == 70, confirm && counter >= 120};
}
WorldVillageHumanEffectResult
prepare_world_village_human_effect(const WorldVillageActivityDefinition &a,
                                   const HumanDefinitionStatsInput &definition,
                                   const std::vector<HumanProfessionRule> &professions,
                                   const HumanDerivedStats &cached, int satisfaction) {
    if (!valid(a) || satisfaction < 0 || satisfaction > 100)
        return {WorldVillageActivityError::invalid_input, {}};
    WorldVillageHumanEffect next{definition, satisfaction, 0, {}};
    if (a.kind == 0) {
        next.previous_value = satisfaction;
        // c.d.a(float)的乘除顺序和单精度截断；不改成赠礼整数插值。
        const float value = static_cast<float>(std::clamp(satisfaction, 15, 55));
        const float ratio = 1.0f + ((value - 15.0f) * (0.5f - 1.0f)) / (55.0f - 15.0f);
        const float request = ratio * static_cast<float>(a.magnitude);
        if (!std::isfinite(request) ||
            static_cast<double>(request) < std::numeric_limits<int>::min() ||
            static_cast<double>(request) > std::numeric_limits<int>::max())
            return {WorldVillageActivityError::overflow, {}};
        const auto result = static_cast<std::int64_t>(satisfaction) + static_cast<int>(request);
        if (!fits(result))
            return {WorldVillageActivityError::overflow, {}};
        next.satisfaction = static_cast<int>(std::clamp<std::int64_t>(result, 0, 100));
    } else if (a.kind == 1) {
        if (definition.current_profession < 0 ||
            static_cast<std::size_t>(definition.current_profession) >= professions.size())
            return {WorldVillageActivityError::invalid_input, {}};
        next.previous_value = cached.attributes[a.attribute];
        const auto scaled =
            static_cast<std::int64_t>(a.magnitude) *
            professions[definition.current_profession].attribute_percent[a.attribute];
        if (!fits(scaled))
            return {WorldVillageActivityError::overflow, {}};
        const auto extra = static_cast<std::int64_t>(definition.extra[a.attribute]) + scaled / 100;
        if (!fits(extra))
            return {WorldVillageActivityError::overflow, {}};
        next.definition.extra[a.attribute] = static_cast<int>(extra);
        const auto stats = derive_human_stats(next.definition, professions);
        if (!stats.candidate)
            return {stats.error == HumanGrowthError::numeric_overflow
                        ? WorldVillageActivityError::overflow
                        : WorldVillageActivityError::invalid_input,
                    {}};
        next.stats = *stats.candidate;
    } else {
        return {WorldVillageActivityError::unsupported_kind, {}};
    }
    return {WorldVillageActivityError::none, std::move(next)};
}
} // namespace dungeon_village_reference
