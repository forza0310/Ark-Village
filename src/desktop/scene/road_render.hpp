#pragma once
// Research ui/README#road-patches: presentation-only whole PNG overlays, not SEB frames.
#include "ark/simulation/world/rules/domain.hpp"
#include <optional>
#include <vector>
namespace ark::desktop {
struct RoadPatch {
    simulation::rules::Position cell;
    const char *image{};
    int width{}, height{}, offset_x{}, offset_y{}, depth_offset{-10};
};
// Exact published patch parameters, offsets relative to the base SEB anchor D (not centre).
std::optional<RoadPatch> road_patch(simulation::rules::Position cell, bool visible, bool quad,
                                    bool edge);
} // namespace ark::desktop
