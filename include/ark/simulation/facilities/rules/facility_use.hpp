#pragma once

// Ordinary food/inn timing only, in eligible c()/d() pairs, not seconds or render frames.
// Arrival, map binding, occupation ownership and the complete exit remain caller responsibilities.
#include "ark/simulation/world/rules/domain.hpp"

namespace ark::simulation::rules {

struct FacilityUseInput {
    CharacterId character_id;
    BuildingId instance_id;
    std::int32_t definition_id{};
    std::int32_t legacy_category{};
    std::int32_t legacy_detail{};
    std::int32_t legacy_activity{};
    std::int32_t definition_wait{};
    std::uint32_t legacy_flags{};
};

enum class FacilityUsePhase { queued, in_use, exit_ready };

struct FacilityUseState {
    FacilityUseInput input;
    FacilityUsePhase phase{FacilityUsePhase::queued};
    std::int32_t duration{};
    std::int32_t elapsed{}; // B after the previous d(); c() reads this BEFORE incrementing.
    std::int32_t remaining{};
    std::uint32_t legacy_flags{};
};

struct FacilityUseStep {
    FacilityUseState state;
    bool register_occupation{}; // First d(): append usage, not exclusive reservation/admission.
    bool request_capacity_hp{}; // c(): invoke the existing HP protocol BEFORE d() animation.
    bool request_exit{}; // Wait depleted: opcode 24 follows in this same d(), not a later tick.
};

enum class FacilityUseError {
    none,
    invalid_input,
    unsupported_branch,
    invalid_state,
    exit_pending
};

struct FacilityUseTimingResult {
    FacilityUseError error{FacilityUseError::none};
    std::optional<FacilityUseStep> candidate;
};

// Prepare after a separately validated arrival. Only category 1/2, detail 0, activity 0/1.
// No occupation or charge happens here; category-2 flag clearing is immediate.
FacilityUseTimingResult prepare_facility_use(const FacilityUseInput &input);
// One uninterrupted eligible pair. The first call covers the entry round's d() (B initially 0).
// At exit_ready the owner must resolve exit position/effects/next activity; this helper stops.
// Pure candidates need owner-side atomic commit and deduplication, including zero-price visits.
FacilityUseTimingResult advance_facility_use(const FacilityUseState &state);

} // namespace ark::simulation::rules
