#pragma once

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

FacilitySatisfactionResult prepare_facility_satisfaction(const FacilitySatisfactionInput &input);

} // namespace dungeon_village_reference
