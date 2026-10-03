#pragma once

// Derived terrain appearance only; this does not change route categories or construction rules.
#include "ark/world/grid.hpp"
namespace ark::world {
// STARTUP's four-neighbour mask and original 16-frame table. Supply current unoccupied terrain,
// so constructing over a road also removes that cell from its neighbours' visual connections.
SourceMap connect_roads(SourceMap terrain, int road_display_id);
} // namespace ark::world
