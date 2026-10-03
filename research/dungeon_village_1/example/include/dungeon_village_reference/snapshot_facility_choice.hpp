#pragma once

#include "dungeon_village_reference/activity_candidates.hpp"

namespace dungeon_village_reference {

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

SnapshotFacilityResult select_snapshot_facility(const ActivityCandidateSnapshot &snapshot,
                                                std::int32_t category, std::int64_t ticket);

} // namespace dungeon_village_reference
