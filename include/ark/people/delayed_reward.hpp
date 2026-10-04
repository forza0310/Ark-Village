#pragma once

// Definition-shared N/O reward from research encounter_lifecycle, independent of encounters.
#include <optional>

namespace ark::people {
struct DelayedRewardState {
    int amount{};
    int counter{};
};
// Merge only the unconsumed portion, replace the delay, and leave live definition XP untouched.
std::optional<DelayedRewardState> prepare_delayed_reward(const DelayedRewardState &state,
                                                         int amount, int delay);
struct DelayedRewardStep {
    DelayedRewardState state;
    int increment{};
};
// Nine truncated increments; N==0 returns before incrementing O. Consumers apply XP themselves.
std::optional<DelayedRewardStep> advance_delayed_reward(const DelayedRewardState &state);
// Apply after evaluating growth: a real upgrade discards the remaining reward, as does step9.
std::optional<DelayedRewardState> complete_delayed_reward_step(const DelayedRewardStep &step,
                                                               bool level_increased);
} // namespace ark::people
