#pragma once

#include "dungeon_village_reference/domain.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace dungeon_village_reference {

struct RankedFacilityCell {
    Position position;
    BuildingId instance_id;
    std::int32_t definition_id{};
    std::int32_t legacy_category{};
    std::int32_t legacy_phase{1};
    std::int64_t definition_charm{};
    std::int64_t cost{};
};

struct RankedFacilityTarget {
    std::size_t drawn_index{};
    std::size_t goal_index{};
    RankedFacilityCell goal;
};

enum class RankedFacilityError {
    none,
    invalid_input,
    unsupported_category,
    no_weight,
    numeric_overflow,
    invalid_ticket
};

struct RankedFacilityResult {
    RankedFacilityError error{RankedFacilityError::none};
    std::optional<RankedFacilityTarget> target;
};

RankedFacilityResult select_ranked_facility(const std::vector<RankedFacilityCell> &candidates,
                                            std::int32_t category, std::int64_t ticket);

} // namespace dungeon_village_reference
