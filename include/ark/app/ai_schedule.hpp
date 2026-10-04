// Adapted from research d412d6e example/include/dungeon_village_reference/ai_schedule.hpp;
// independent product build.
#pragma once

// Live roster ordering for one admitted UserData AI round, not a renderer/frame clock.
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace ark::app {
enum class AiRosterKind { human, monster, projectile, object, encounter, facility };
using AiRosters =
    std::array<std::vector<std::uint64_t>, 6>; // IDs are scoped per roster, zero valid.
enum class AiSchedulePhase {
    human_decision,
    human_execution,
    release_human_facility,
    monster_decision,
    monster_execution,
    projectile,
    object,
    encounter,
    facility,
    finalize
};
struct AiScheduleVisit {
    AiSchedulePhase phase{};
    std::optional<std::uint64_t> id; // Finalize has no instance.
};
struct AiRosterAppend {
    AiRosterKind kind{};
    std::uint64_t id{};
};
struct AiScheduleResponse {
    bool accepted{true}; // Consumer could not prepare its domain candidate => fail whole plan.
    bool remove{};       // True from c()/d()/object/event consumer, never inverted.
    bool release_current_facility{}; // Human d() true and q()!=null, before instance erase.
    std::vector<AiRosterAppend> append;
};
// Handler MUST prepare into a private owner copy, not mutate the live world. It sees the current
// candidate rosters, including earlier same-round appends/removals; external effects stay requests.
using AiScheduleHandler =
    std::function<AiScheduleResponse(const AiScheduleVisit &, const AiRosters &)>;
struct AiScheduleInput {
    AiRosters rosters;
    bool admitted{true}; // Caller already resolved actual UserData/main-scene update gates.
    std::size_t dispatch_limit{1000000}; // Safety contract, not a source gameplay cap.
};
enum class AiScheduleError {
    none,
    invalid_input,
    duplicate_id,
    consumer_failed,
    invalid_response,
    dispatch_limit
};
struct AiScheduleCandidate {
    AiRosters rosters;
    std::vector<AiScheduleVisit> visits; // Includes release/finalize in execution order.
};
struct AiScheduleResult {
    AiScheduleError error{AiScheduleError::none};
    std::optional<AiScheduleCandidate> candidate;
};
// Human c/d reverse, monster c reverse/d forward-with-skip, projectiles/objects/events reverse,
// facilities forward, then finalize. Does not flatten both actor passes into per-actor c+d.
AiScheduleResult prepare_ai_schedule(const AiScheduleInput &input,
                                     const AiScheduleHandler &handler);
} // namespace ark::app
