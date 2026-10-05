#pragma once

// Read-only page74 data over the canonical world. This does not open a source page,
// pause the world, refresh neighbourhoods or implement any facility action.
#include "ark/simulation/startup_world_runtime.hpp"

namespace ark::app {
enum class WorldFacilityTemplate { ordinary, equipment, recruitment, booster, home };
enum class WorldFacilityQueryError {
    none,
    missing_instance,
    missing_source,
    invalid_state,
    inconsistent_neighbourhood,
    numeric_overflow
};
struct WorldFacilityNeighbour {
    std::uint64_t instance{};
    int definition{};
    std::string name;
    int level{}; // Shared definition level, never an instance ordinal.
    std::vector<simulation::rules::NeighbourModifier> modifiers;
};
struct WorldFacilityConstruction {
    int updates{};
    int limit{}; // Fixed at construction start; never re-quote with current jobs.
};
struct WorldFacilityDetail {
    std::uint64_t instance{};
    int definition{};
    WorldFacilityTemplate type{WorldFacilityTemplate::ordinary};
    std::string name;
    int display_id{};
    int kind{};
    int activity_category{};
    int activity_detail{};
    simulation::rules::FacilityPlacement placement;
    int level{};
    int completed_uses{};
    bool upgrade_pending{};
    std::optional<std::int64_t> remaining_uses; // None at MAX; preserve source d()-K.
    // Effective instance price/quality/charm/maintenance, in source slot order.
    std::array<std::int64_t, 4> attributes{};
    int status{};
    std::optional<WorldFacilityConstruction> construction;
    std::vector<simulation::rules::CharacterId> occupants; // Preserve source order/duplicates.
    int month{};                                           // Source zero-based month.
    std::int64_t monthly_income{};
    std::int64_t monthly_expense{};
    // Original scene footer only applies to kind3/9: rows0..month, income minus expense.
    std::optional<std::int64_t> cumulative_profit;
    std::array<int, 3> neighbour_modifiers{};
    std::vector<WorldFacilityNeighbour> neighbours; // Facility sources only; roads are not rows.
};
struct WorldFacilityDetailResult {
    WorldFacilityQueryError error{WorldFacilityQueryError::none};
    std::optional<WorldFacilityDetail> detail;
};

// Missing initial source caches may be reconstructed by the published pure geometry rule,
// but only if they agree with the Owner's effective modifiers. Stale/different data is an
// explicit error, never a zero bonus or a mutation that silently repairs the simulation.
WorldFacilityDetailResult
query_world_facility_detail(const simulation::StartupWorldRuntimeState &state,
                            std::uint64_t instance);
} // namespace ark::app
