// Maintained N/O reward rules from research 730e7ee encounter_lifecycle.cpp.
#include "ark/people/delayed_reward.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
namespace ark::people {
namespace {
bool fits(std::int64_t value) { return value >= 0 && value <= std::numeric_limits<int>::max(); }
} // namespace
std::optional<DelayedRewardState> prepare_delayed_reward(const DelayedRewardState &s, int amount,
                                                         int delay) {
    if (s.amount < 0 || s.counter == std::numeric_limits<int>::min() || amount < 0 || delay < 0)
        return std::nullopt;
    std::int64_t remaining = s.amount;
    if (s.counter >= 0 && s.counter < 9)
        remaining -= static_cast<std::int64_t>(s.counter) * s.amount / 9;
    remaining += amount;
    if (!fits(remaining))
        return std::nullopt;
    return DelayedRewardState{static_cast<int>(remaining), -delay};
}
std::optional<DelayedRewardStep> advance_delayed_reward(const DelayedRewardState &s) {
    if (s.amount < 0 || s.counter == std::numeric_limits<int>::max())
        return std::nullopt;
    DelayedRewardStep c{s, 0};
    if (s.amount == 0)
        return c; // Original a(character) returns BEFORE O increment when N==0.
    ++c.state.counter;
    if (c.state.counter >= 0) {
        const auto amount = static_cast<std::int64_t>(s.amount);
        const int now = std::clamp(c.state.counter, 0, 9);
        const int previous = std::clamp(c.state.counter - 1, 0, 9);
        c.increment = static_cast<int>(now * amount / 9 - previous * amount / 9);
    }
    // Level/job/HP/upgrade side effects between increments and N clearing are owner requests,
    // so preserve N here rather than guessing when all current definition changes have finished.
    return c;
}
std::optional<DelayedRewardState> complete_delayed_reward_step(const DelayedRewardStep &s,
                                                               bool increased) {
    if (s.state.amount < 0 || s.increment < 0)
        return std::nullopt;
    if (increased || s.state.counter >= 9)
        return DelayedRewardState{};
    return s.state;
}
} // namespace ark::people
