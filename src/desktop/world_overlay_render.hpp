#pragma once

#include "resources.hpp"
#include "world_overlay.hpp"

namespace ark::desktop {
// Convert source-relative commands to the current raster once; never advance display lifetimes.
void draw_world_overlay(const OverlayPlan &plan, Sprites &sprites, Vector2 anchor, float zoom);
} // namespace ark::desktop
