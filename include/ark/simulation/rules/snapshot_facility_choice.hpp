#pragma once

// Full snapshot selection separates the drawn active index, snapshot index and final goal index.

#include "ark/simulation/rules/activity_candidates.hpp"

namespace ark::simulation::rules {

struct SnapshotFacilityTarget {
    std::size_t drawn_active_index{};
    std::size_t drawn_snapshot_index{};
    std::size_t goal_snapshot_index{};
    ActivityCandidateCell goal;
};

enum class SnapshotFacilityError {
    none,
    unsupported_category,
    invalid_snapshot,
    no_weight,
    numeric_overflow,
    invalid_ticket
};

struct SnapshotFacilityResult {
    SnapshotFacilityError error{SnapshotFacilityError::none};
    std::optional<SnapshotFacilityTarget> target;
};

// Weight active cells without deduplication; the first matching full-snapshot cell may lack a
// route.
SnapshotFacilityResult select_snapshot_facility(const ActivityCandidateSnapshot &snapshot,
                                                std::int32_t category, std::int64_t ticket);

} // namespace ark::simulation::rules
