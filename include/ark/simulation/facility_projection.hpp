#pragma once

// Original 60x30 presentation projection; never put these pixel offsets in domain rules.
#include "ark/simulation/rules/character_motion.hpp"

namespace ark::simulation {
struct FacilityUseProjection {
    ark::simulation::rules::Position projected_target;
    ark::simulation::rules::Position world_target;
};
// h.j uses OLD cached s, not the instance anchor or current n. Category6/detail3 and8/detail2
// alone have projected in-facility movement. Integer h.e x/z truncate toward zero.
std::optional<FacilityUseProjection>
project_facility_use_target(ark::simulation::rules::Position old_cell, int category, int detail,
                            std::optional<int> direction = {});
} // namespace ark::simulation
