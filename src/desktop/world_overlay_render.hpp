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
// cd13 and equipment lifts share one source list. Preserve its order and each text sandwich.
void draw_world_actor_effects(const simulation::StartupWorldRuntimeState &state,
                              simulation::rules::CharacterId actor, Sprites &sprites,
                              const Text &text, Vector2 anchor, float zoom);
} // namespace ark::desktop
