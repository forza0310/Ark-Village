#pragma once
// Research ui/README#road-patches: presentation-only whole PNG overlays, not SEB frames.
#include "ark/world/grid.hpp"
#include <optional>
#include <vector>
namespace ark::desktop {
struct RoadPatch {
    world::Cell cell;
    const char *image{};
    int width{}, height{}, offset_x{}, offset_y{}, depth_offset{-10};
};
// Re-evaluate current unoccupied terrain. Same road display corresponds to definition18 here.
// Preserve second-pass y-descending/x-ascending order and quad-before-edge precedence.
std::vector<RoadPatch> road_patches(const world::SourceMap &terrain, int road_display);
// Exact published patch parameters, offsets relative to the base SEB anchor D (not centre).
std::optional<RoadPatch> road_patch(world::Cell cell, bool visible, bool quad, bool edge);
} // namespace ark::desktop
