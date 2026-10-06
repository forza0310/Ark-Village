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
// Interpolate only continuous walking for the raster. Teleports, action changes, hiding and
// pauses snap to current source facts; collision, HP, RNG and the canonical positions never change.
simulation::rules::CombatPoint
world_actor_render_position(const simulation::StartupWorldRuntimeState &state,
                            simulation::rules::CharacterId actor,
                            const simulation::StartupWorldRuntimeState *previous, float alpha);
// UI camera operations use this tiny presentation value, never copy a mutable business world.
struct WorldCameraView {
    std::array<float, 2> camera{};
    std::array<int, 4> viewport{};
};
// Maintained prototype hit box/order, transformed by the desktop camera/zoom. The
// worker rechecks this live identity before opening its shared human definition.
std::optional<simulation::rules::CharacterId>
world_pick_human(const simulation::StartupWorldRuntimeState &state, const WorldCameraView &view,
                 Vector2 pointer, float zoom);
void world_zoom_camera(WorldCameraView &view, Extent extent, Vector2 pointer, float wheel,
                       float &zoom);
// Stable source depth order for per-cell surfaces, fences, doors, all actors, HP and cash effects.
void draw_world_scene(const simulation::StartupWorldRuntimeState &state, Sprites &sprites,
                      float zoom, const simulation::StartupWorldRuntimeState *previous = nullptr,
                      float alpha = 1, const WorldCameraView *view = nullptr);
} // namespace ark::desktop
