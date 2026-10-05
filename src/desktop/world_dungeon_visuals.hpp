#pragma once

// Source-scale task overlays. No raylib, texture ownership, clocks, random draws or world writes.
#include "ark/simulation/startup_world_runtime.hpp"
#include "world_overlay.hpp"
#include <string>
#include <vector>

namespace ark::desktop {
struct WorldDungeonView {
    bool active{};
    simulation::rules::Position site;
    bool has_facility{};
    int kind{}, phase{}, progress{}, extent{};
    bool special_marker{};
    std::string tenant_sprite;
    int tenant_frame{};
};
struct WorldDungeonLayer {
    float depth_offset{}; // Added to the projected task ground anchor D.y, before zoom.
    OverlayPlan plan;     // All commands are relative to the same ground anchor D.
};
WorldDungeonView world_dungeon_view(const simulation::StartupWorldRuntimeState &state);
// Height is the entire PNG bound by the first footprint cell's actual tenant sprite frame,
// not SEB bounds or a guessed diamond height. alternate_text explicitly selects the documented
// language-offset branch; this function does not infer the APK platform predicate from fonts.
std::vector<WorldDungeonLayer> world_dungeon_visuals(const WorldDungeonView &view,
                                                     int tenant_png_height,
                                                     bool alternate_text = false);
} // namespace ark::desktop
