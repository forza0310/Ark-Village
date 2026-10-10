#pragma once

// Exit helpers model shared uses and satisfaction requests, not the entire exit sequence.

#include "ark/simulation/actors/rules/character_motion.hpp"
#include "ark/simulation/world/rules/domain.hpp"
#include "ark/simulation/facilities/rules/facility_economy.hpp"

namespace ark::simulation::rules {

enum class FacilityExitError {
    none,
    invalid_input,
    numeric_overflow,
    unsupported_branch,
    missing_ticket,
    invalid_ticket
};

struct FacilityUseProgress {
    int level{1};
    std::int32_t completed_uses{};
    bool upgrade_pending{};
};

struct FacilityUseCandidate {
    std::int32_t definition_id{};
    FacilityUseProgress progress;
    std::int64_t upgrade_threshold{};
};

struct FacilityUseResult {
    FacilityExitError error{FacilityExitError::none};
    std::optional<FacilityUseCandidate> candidate;
};

// Increment definition-shared uses and latch upgrade_pending without changing the level.
FacilityUseResult prepare_facility_use_completion(std::int32_t definition_id,
                                                  LevelEndpoints upgrade_uses,
                                                  const FacilityUseProgress &progress);

struct FacilitySatisfactionInput {
    CharacterId character_id;
    BuildingId instance_id;
    std::int32_t definition_id{};
    std::int32_t satisfaction{};
    std::array<std::int32_t, 2> legacy_job_thresholds{};
    std::int32_t resolved_instance_quality{};
    int ticket{};
};

struct PopularityRequest {
    std::int32_t delta{};
    bool show_notice{};
    int legacy_reason{};
    int legacy_countdown{};
};

struct FacilitySatisfactionCandidate {
    CharacterId character_id;
    BuildingId instance_id;
    std::int32_t definition_id{};
    std::int64_t adjusted_threshold{};
    std::int32_t satisfaction{};
    std::int32_t satisfaction_delta{};
    PopularityRequest popularity;
};

struct FacilitySatisfactionResult {
    FacilityExitError error{FacilityExitError::none};
    std::optional<FacilitySatisfactionCandidate> candidate;
};

// Use an injected ticket in [0,10); return a popularity request, not a global popularity mutation.
FacilitySatisfactionResult prepare_facility_satisfaction(const FacilitySatisfactionInput &input);

enum class FacilityExitPositionStatus { retained, relocated };
struct FacilityExitPositionCandidate {
    BuildingId instance_id;
    Position logical_cell;
    WorldPosition position;
    FacilityExitPositionStatus status{FacilityExitPositionStatus::retained};
};
struct FacilityExitPositionResult {
    FacilityExitError error{FacilityExitError::none};
    std::optional<FacilityExitPositionCandidate> candidate;
};

// Validate the entire footprint and current logical binding. Multi-cell exits scan legacy state
// 3/4, not route category or path cost; no available exit still returns a retained candidate.
// This prepares position only, without releasing occupancy or choosing the next activity.
FacilityExitPositionResult prepare_facility_exit_position(const LegacyMap &map,
                                                          const FacilityPlacement &facility,
                                                          WorldPosition current);

struct FacilityAttributeEffect {
    int attribute_index{};
    std::int32_t delta{};
};
enum class ExitDeferredAction { choose_activity, shop_marker, attribute, expression };
struct ExitDeferredRequest {
    ExitDeferredAction action;
    int parameter{};
    std::int32_t value{};
};
struct OrdinaryExitTail {
    bool evaluate_satisfaction_now{};
    std::vector<ExitDeferredRequest> requests;
};
struct OrdinaryExitTailResult {
    FacilityExitError error{FacilityExitError::none};
    std::optional<OrdinaryExitTail> candidate;
};
// Category1/detail0 and category2/detail0 only. Satisfaction is requested during exit;
// attributes/expressions stay AFTER activity0 in the control queue. A successful activity choice
// ends that interpreter call, so the tail must not be eagerly applied by the caller.
OrdinaryExitTailResult
prepare_ordinary_exit_tail(int category, int detail,
                           const std::vector<FacilityAttributeEffect> &effects,
                           std::optional<int> effect_ticket = std::nullopt);

} // namespace ark::simulation::rules
