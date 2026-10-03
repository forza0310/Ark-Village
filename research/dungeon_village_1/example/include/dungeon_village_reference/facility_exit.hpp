#pragma once

// Exit helpers model shared uses and satisfaction requests, not the entire exit sequence.

#include "dungeon_village_reference/domain.hpp"
#include "dungeon_village_reference/facility_economy.hpp"

namespace dungeon_village_reference {

enum class FacilityExitError { none, invalid_input, numeric_overflow };

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

} // namespace dungeon_village_reference
