#pragma once

// Read-only projection of the canonical editing mode. The controller keeps desktop
// pointer/selection state and sends all begin/confirm/cancel operations through FIFO.
#include "world_build_placement.hpp"
#include <string>

namespace ark::desktop {
namespace ui {
class Skin;
}
struct WorldEditView {
    int mode{};
    bool active{}, rotate_allowed{};
    std::optional<simulation::rules::Position> current_cell;
    std::vector<simulation::rules::Position> segment, road_preview;
    std::string caption, cancel_label, confirm_label;
};
WorldEditView world_edit_view(const simulation::StartupWorldRuntimeState &state,
                              std::optional<simulation::rules::Position> pointer_cell);
std::optional<WorldBuildIntent> world_edit_input(const WorldEditView &view, Extent extent,
                                                 const WorldBuildInput &input, bool blocked);
// Only road modes have a published overlay sprite contract. Other modes retain
// controls while their unproved cursor images remain absent.
void draw_world_edit_preview(const WorldEditView &view, const WorldCameraView &camera, float zoom,
                             Sprites &sprites);
void draw_world_edit_controls(const WorldEditView &view, Extent extent, const ui::Skin &skin,
                              bool enabled, const std::string &feedback = {});
} // namespace ark::desktop
