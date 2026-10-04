#pragma once

// Read-only canonical-world rendering; platform view changes never choose actor goals or move them.
#include "ark/simulation/startup_world_runtime.hpp"
#include "projection.hpp"
#include "resources.hpp"

namespace ark::desktop {
struct WorldActorPose {
    bool monster{};
    int sprite{}, image{}, frame{};
};
// Inverse raster zoom supplies source visibility bounds before the next simulation round.
std::array<int, 4> world_viewport(Extent extent, float zoom);
// Project source x/height/z positions, including the canonical camera and viewport midpoint.
Vector2 world_anchor(const simulation::StartupWorldRuntimeState &state,
                     simulation::rules::CombatPoint position, float zoom);
// Change only presentation camera/viewport while retaining the world point under the pointer.
void world_zoom_at(simulation::StartupWorldRuntimeState &state, Extent extent, Vector2 pointer,
                   float wheel, float &zoom);
// Read original action-counter phase and profession/body bindings, independent of render FPS.
WorldActorPose world_actor_pose(const simulation::StartupWorldRuntimeState &state,
                                simulation::rules::CharacterId actor);
// Stable source depth order for per-cell surfaces, fences, doors, all actors, HP and cash effects.
void draw_world_scene(const simulation::StartupWorldRuntimeState &state, Sprites &sprites,
                      float zoom);
} // namespace ark::desktop
