#pragma once

// Product protocol adapted from research WORLD_SCHEDULE (2026-10-04). Actual domain owners
// provide every consumer; this protocol is not a second world or a default-success AI engine.
#include "ark/app/ai_schedule.hpp"
#include <utility>

namespace ark::app {
struct WorldScheduleState {
    AiRosters rosters;
    int updates{}; // UserData.p, distinct from calendar/global steps/actor counters.
    std::vector<std::vector<int>> hints;
    std::vector<std::vector<int>> floating_notes;
    std::vector<std::array<int, 3>> popularity_queue;
    bool rescue_available{};
};
enum class WorldScheduleStage {
    influence,
    arrival_front,
    popularity,
    rescue_query,
    decision_prefix,
    decision,
    execution_prefix,
    carry_expression,
    control,
    actor_tail,
    remove_actor,
    projectile,
    object,
    encounter,
    facility,
    finalize
};
struct WorldScheduleCall {
    WorldScheduleStage stage{};
    std::optional<std::uint64_t> id;
    std::optional<AiRosterKind> roster;
    std::optional<std::array<int, 2>> popularity; // delta, show flag.
    bool from_execution{}; // remove_actor: human d removal releases q first; c removal does not.
};
struct WorldScheduleStep {
    WorldScheduleState state;
    bool remove_requested{}; // Only decision/control/tail and non-actor domain stages.
};
using WorldScheduleConsumer = std::function<std::optional<WorldScheduleStep>(
    const WorldScheduleState &, const WorldScheduleCall &)>;
struct WorldScheduleInput {
    bool admitted{true};                 // Main-scene gates already resolved by the caller.
    std::size_t dispatch_limit{1000000}; // Maintenance guard, not source gameplay limit.
};
enum class WorldScheduleError {
    none,
    invalid_state,
    missing_consumer,
    consumer_failed,
    invalid_mutation,
    dispatch_limit
};
struct WorldScheduleCandidate {
    WorldScheduleState state;
    std::vector<WorldScheduleCall> calls;
};
struct WorldScheduleResult {
    WorldScheduleError error{WorldScheduleError::none};
    std::optional<WorldScheduleCandidate> candidate;
};
// The old-roster influence consumer runs before arrival. Every stage is explicit and required.
// Consumers may append live identities only at domain stages. Actual domain deletion happens
// at remove_actor; this protocol erases its roster entry after that callback succeeds.
WorldScheduleResult prepare_world_schedule(const WorldScheduleState &state,
                                           const WorldScheduleInput &input,
                                           const WorldScheduleConsumer &consumer);

template <class Owner> struct OwnedWorldScheduleStep {
    Owner state;
    bool remove_requested{};
};
template <class Owner> struct OwnedWorldScheduleAdapter {
    std::function<const WorldScheduleState &(const Owner &)> read;
    std::function<WorldScheduleState &(Owner &)> write;
    std::function<std::optional<OwnedWorldScheduleStep<Owner>>(const Owner &,
                                                               const WorldScheduleCall &)>
        consume;
};
template <class Owner> struct OwnedWorldScheduleResult {
    WorldScheduleError error{WorldScheduleError::none};
    std::optional<Owner> state;
    std::vector<WorldScheduleCall> calls;
};
// Copy the complete caller owner, including its random stream and ledger. A late consumer
// failure exposes no partial owner; callbacks must not capture/write the actual live world.
template <class Owner>
OwnedWorldScheduleResult<Owner>
prepare_owned_world_schedule(const Owner &state, const WorldScheduleInput &input,
                             const OwnedWorldScheduleAdapter<Owner> &adapter) {
    if (!adapter.read || !adapter.write || (input.admitted && !adapter.consume))
        return {WorldScheduleError::missing_consumer, {}, {}};
    Owner scratch = state;
    const auto result = prepare_world_schedule(
        adapter.read(state), input,
        [&](const WorldScheduleState &common,
            const WorldScheduleCall &call) -> std::optional<WorldScheduleStep> {
            adapter.write(scratch) = common;
            auto next = adapter.consume(scratch, call);
            if (!next)
                return {};
            scratch = std::move(next->state);
            return WorldScheduleStep{adapter.read(scratch), next->remove_requested};
        });
    if (!result.candidate)
        return {result.error, {}, {}};
    adapter.write(scratch) = result.candidate->state;
    return {WorldScheduleError::none, std::move(scratch), result.candidate->calls};
}
} // namespace ark::app
