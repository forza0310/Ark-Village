#pragma once

// Regional fallback consumes an explicit random prefix so partial plans can be replayed.

#include "dungeon_village_reference/activity_candidates.hpp"

namespace dungeon_village_reference {

enum class RegionalChoiceStatus { no_candidate, needs_draw, selected };

struct RegionalCandidateTarget {
    std::size_t snapshot_index{};
    ActivityCandidateCell cell;
};

struct RegionalChoicePlan {
    RegionalChoiceStatus status{RegionalChoiceStatus::no_candidate};
    std::size_t consumed_draws{};
    std::size_t next_draw_bound{};
    bool used_fallback{};
    std::optional<RegionalCandidateTarget> target;
};

enum class RegionalChoiceError { none, invalid_snapshot, too_many_draws, invalid_ticket };

struct RegionalChoiceResult {
    RegionalChoiceError error{RegionalChoiceError::none};
    std::optional<RegionalChoicePlan> plan;
};

// Return needs_draw for an incomplete prefix; the sixth draw bypasses the regional threshold.
RegionalChoiceResult select_regional_candidate(const ActivityCandidateSnapshot &snapshot,
                                               std::int32_t town_bottom,
                                               const std::vector<std::int64_t> &draw_prefix);

} // namespace dungeon_village_reference
