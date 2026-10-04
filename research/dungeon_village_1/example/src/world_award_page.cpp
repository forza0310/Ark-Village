#include "dungeon_village_reference/world_award_page.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
bool fits(std::int64_t value) {
    return value >= std::numeric_limits<std::int32_t>::min() &&
           value <= std::numeric_limits<std::int32_t>::max();
}
bool valid(const WorldAwardPageState &state) {
    if (state.medal_count < 0 || state.page_counter < 0)
        return false;
    std::set<int> definitions;
    std::set<int> active;
    for (const auto &human : state.humans) {
        if (human.definition < 0 || !definitions.insert(human.definition).second)
            return false;
        if (human.presence != 0)
            active.insert(human.definition);
    }
    if (active.empty())
        return false;
    if (!state.initialized)
        return state.ranked_definitions.empty() && !state.announced && !state.termination_pending &&
               !state.closed;
    std::set<int> ranked;
    for (const int definition : state.ranked_definitions)
        if (!active.count(definition) || !ranked.insert(definition).second)
            return false;
    return ranked == active;
}
// c/d.java49–62：z=false不截断，区间退化返回下界25。
int mapped(std::int32_t value, int mean, float sigma) {
    const float low = static_cast<float>(mean) - sigma;
    const float high = static_cast<float>(mean) + sigma;
    return static_cast<int>(high - low != 0.0f
                                ? 25.0f + ((static_cast<float>(value) - low) * 50.0f) / (high - low)
                                : 25.0f);
}
} // namespace
WorldAwardResult prepare_world_award_page_initialization(const WorldAwardPageState &state) {
    if (!valid(state))
        return {WorldAwardError::invalid_owner, {}};
    WorldAwardCandidate candidate{state, {}};
    if (state.initialized)
        return {WorldAwardError::none, std::move(candidate)};
    if (state.medal_count == std::numeric_limits<int>::max())
        return {WorldAwardError::overflow, {}};
    auto &next = candidate.state;
    ++next.medal_count;
    std::array<std::int64_t, 2> totals{};
    int count{};
    for (const auto &human : next.humans) {
        if (human.presence == 0)
            continue;
        ++count;
        next.ranked_definitions.push_back(human.definition);
        for (std::size_t i = 0; i < totals.size(); ++i) {
            totals[i] += human.yearly_totals[i + 1];
            if (!fits(totals[i]))
                return {WorldAwardError::overflow, {}};
        }
    }
    std::array<int, 2> means{static_cast<int>(totals[0] / count),
                             static_cast<int>(totals[1] / count)};
    std::array<std::int64_t, 2> squares{};
    for (const auto &human : next.humans) {
        if (human.presence == 0)
            continue;
        for (std::size_t i = 0; i < squares.size(); ++i) {
            const auto delta = static_cast<std::int64_t>(human.yearly_totals[i + 1]) - means[i];
            if (!fits(delta) || std::abs(delta) > 46340)
                return {WorldAwardError::overflow, {}};
            squares[i] += delta * delta;
            if (!fits(squares[i]))
                return {WorldAwardError::overflow, {}};
        }
    }
    // 原Java先做整数方差除法，double sqrt后转float，不能改成浮点平均方差。
    const std::array<float, 2> sigma{
        static_cast<float>(std::sqrt(static_cast<double>(squares[0] / count))),
        static_cast<float>(std::sqrt(static_cast<double>(squares[1] / count)))};
    for (auto &human : next.humans) {
        if (human.presence == 0)
            continue;
        const int earned = mapped(human.yearly_totals[1], means[0], sigma[0]);
        const int spent = mapped(human.yearly_totals[2], means[1], sigma[1]);
        human.contribution = std::clamp(
            static_cast<int>(static_cast<float>(spent) * 0.5f + static_cast<float>(earned) * 0.5f),
            0, 100);
    }
    auto contribution = [&](int definition) {
        return std::find_if(next.humans.begin(), next.humans.end(),
                            [&](const auto &human) { return human.definition == definition; })
            ->contribution;
    };
    for (std::size_t i = 0; i + 1 < next.ranked_definitions.size(); ++i)
        for (std::size_t j = next.ranked_definitions.size() - 1; j > i; --j)
            if (contribution(next.ranked_definitions[j]) > contribution(next.ranked_definitions[i]))
                std::swap(next.ranked_definitions[i], next.ranked_definitions[j]);
    next.announced = false;
    next.initialized = true;
    return {WorldAwardError::none, std::move(candidate)};
}
WorldAwardResult prepare_world_award_page(const WorldAwardPageState &state,
                                          WorldAwardAction action) {
    if (!valid(state) || !state.initialized || state.closed)
        return {WorldAwardError::invalid_owner, {}};
    switch (action) {
    case WorldAwardAction::update:
    case WorldAwardAction::request_termination:
    case WorldAwardAction::confirm_termination:
    case WorldAwardAction::reject_termination:
        break;
    case WorldAwardAction::request_award:
    case WorldAwardAction::confirm_award:
    default:
        return {WorldAwardError::unsupported_action, {}};
    }
    if ((action == WorldAwardAction::confirm_termination ||
         action == WorldAwardAction::reject_termination) &&
        !state.termination_pending)
        return {WorldAwardError::invalid_owner, {}};
    WorldAwardCandidate candidate{state, {}};
    auto effect = [&](WorldAwardEffectKind kind, int value = 0, std::optional<int> argument = {}) {
        candidate.effects.push_back({kind, value, argument});
    };
    // 原更新回调共同前缀优先于终止确认；event23提前返回，不消费待确认结果。
    if (state.page_counter == 1) {
        effect(WorldAwardEffectKind::sound, 3);
        if (state.medal_count > 0 && state.announced) {
            effect(WorldAwardEffectKind::event, 23, state.medal_count);
            return {WorldAwardError::none, std::move(candidate)};
        }
        candidate.state.announced = true;
    }
    if (action == WorldAwardAction::confirm_termination) {
        effect(WorldAwardEffectKind::event, 22);
        effect(WorldAwardEffectKind::refresh);
        effect(WorldAwardEffectKind::close);
        candidate.state.closed = true;
        candidate.state.termination_pending = false;
        return {WorldAwardError::none, std::move(candidate)};
    }
    if (action == WorldAwardAction::reject_termination)
        candidate.state.termination_pending = false;
    if (state.medal_count <= 0) {
        effect(WorldAwardEffectKind::refresh);
        effect(WorldAwardEffectKind::event, 22);
        effect(WorldAwardEffectKind::close);
        candidate.state.closed = true;
        candidate.state.termination_pending = false;
    } else if (action == WorldAwardAction::request_termination) {
        if (state.termination_pending)
            return {WorldAwardError::invalid_owner, {}};
        effect(WorldAwardEffectKind::termination_prompt);
        candidate.state.termination_pending = true;
    }
    return {WorldAwardError::none, std::move(candidate)};
}
} // namespace dungeon_village_reference
