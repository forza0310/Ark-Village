#pragma once

// Presentation-only bindings from research/ui/BOUNDARY. Map blocking is already logical state.
#include <vector>

namespace ark::desktop {
struct BoundaryCell {
    int boundary_fragment{-1}, external_direction{-1};
};
struct BoundaryOverlay {
    const char *sprite;
    int frame, offset_x, offset_y, depth_offset;
};
// All offsets are relative to the ground SEB anchor D. The caller establishes valid current
// ground display and viewport eligibility; the SEB consumer adds its own record offsets.
// Invalid marked-cell parameters are rejected rather than silently selecting another frame.
std::vector<BoundaryOverlay> boundary_overlays(const BoundaryCell &cell, int boundary_index,
                                               int display_flags, int display_depth_offset,
                                               bool eligible);
} // namespace ark::desktop
