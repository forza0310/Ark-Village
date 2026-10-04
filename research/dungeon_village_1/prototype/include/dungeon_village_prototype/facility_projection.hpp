#pragma once

// Original 60x30 presentation projection; never put these pixel offsets in domain rules.
#include "dungeon_village_reference/character_motion.hpp"

namespace dungeon_village_prototype {
struct FacilityUseProjection {
    dungeon_village_reference::Position projected_target;
    dungeon_village_reference::Position world_target;
};
// h.j uses OLD cached s, not the instance anchor or current n. Category6/detail3 and8/detail2
// alone have projected in-facility movement. Integer h.e x/z truncate toward zero.
std::optional<FacilityUseProjection>
project_facility_use_target(dungeon_village_reference::Position old_cell, int category, int detail,
                            std::optional<int> direction = {});
} // namespace dungeon_village_prototype
