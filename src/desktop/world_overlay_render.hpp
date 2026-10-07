#pragma once

#include "ark/simulation/startup_world_visuals.hpp"
#include "resources.hpp"
#include "world_overlay.hpp"

namespace ark::desktop {
// Convert source-relative commands to the current raster once; never advance display lifetimes.
void draw_world_overlay(const OverlayPlan &plan, Sprites &sprites, Vector2 anchor, float zoom);
// Execute the maintained effect plan without reinterpreting timing or adding SEB offsets twice.
void draw_world_visuals(const std::vector<simulation::StartupVisualDraw> &plan, Sprites &sprites,
                        Vector2 anchor, float zoom);
} // namespace ark::desktop
