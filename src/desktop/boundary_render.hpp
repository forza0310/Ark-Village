#pragma once

// Presentation-only bindings from research/ui/BOUNDARY. Map blocking is already logical state.
#include "ark/world/loaded_map.hpp"
#include <vector>

namespace ark::desktop {
struct BoundaryOverlay {
    const char *sprite;
    int frame, offset_x, offset_y, depth_offset;
};
// All offsets are relative to the ground SEB anchor D. The caller establishes valid current
// ground display and viewport eligibility; the SEB consumer adds its own record offsets.
// Invalid marked-cell parameters are rejected rather than silently selecting another frame.
std::vector<BoundaryOverlay> boundary_overlays(const world::LoadedCell &cell, int boundary_index,
                                               int display_flags, int display_depth_offset,
                                               bool eligible);
} // namespace ark::desktop
