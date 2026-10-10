#pragma once

// Pointer picking and ghost footprints share the canonical-world renderer's raster transform.
// Preview is advisory and read-only: the simulation rechecks the whole transaction at commit.
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "world_scene.hpp"

namespace ark::desktop {
// Complete ordered tenant fragments at relative raster anchors. Both catalogue icons and
// placement ghosts use the definition -> display -> SEB chain, never the legacy Game.
struct WorldBuildGraphic {
    std::string sprite;
    std::vector<std::pair<int, Vector2>> frames;
};
WorldBuildGraphic world_build_graphic(const simulation::StartupWorldRuntimeState &state,
                                      int definition,
                                      simulation::rules::FacilityOrientation orientation);
// One physical input frame produces at most one intent. Map clicks precede keyboard
// confirmation so selecting a new cell can never buy the previously locked anchor.
struct WorldBuildControls {
    Rectangle scene, cancel, rotate, confirm;
};
struct WorldBuildInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, rotate{};
};
enum class WorldBuildAction { choose, rotate, confirm, cancel };
struct WorldBuildIntent {
    WorldBuildAction action;
    std::optional<Vector2> point; // Present only for choose; conversion to map cell stays separate.
};
WorldBuildControls world_build_controls(Extent extent);
// Hidden rotation controls leave their scene region available for choosing a map cell.
std::optional<WorldBuildIntent> world_build_input(const WorldBuildControls &controls,
                                                  const WorldBuildInput &input, bool blocked,
                                                  bool rotation_allowed);
struct WorldBuildPreview {
    int definition{};
    simulation::rules::Position anchor;
    simulation::rules::FacilityOrientation orientation{};
    simulation::StartupBuildDenial denial{simulation::StartupBuildDenial::none};
    bool missing_source{};
    bool cursor_in_map{}, graphic_visible{}, rotation_hint{};
    WorldBuildGraphic graphic;
    std::vector<simulation::rules::FootprintCell> cells;
    std::int64_t cost{};
    bool valid() const { return !missing_source && denial == simulation::StartupBuildDenial::none; }
};
std::optional<simulation::rules::Position>
world_pick_cell(const simulation::StartupWorldRuntimeState &state, const WorldCameraView &view,
                Vector2 pointer, float zoom);
WorldBuildPreview world_build_preview(const simulation::StartupWorldRuntimeState &state,
                                      int definition, simulation::rules::Position anchor,
                                      simulation::rules::FacilityOrientation orientation);
void draw_world_build_preview(const WorldBuildPreview &preview, const WorldCameraView &view,
                              float zoom, Sprites &sprites);
} // namespace ark::desktop
