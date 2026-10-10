#include "world_editing.hpp"
#include "ark/simulation/facilities/startup_world_editing.hpp"
#include "../ui/common/skin.hpp"
#include <algorithm>

namespace ark::desktop {
namespace {
using Position = simulation::rules::Position;
Vector2 road_anchor(const WorldCameraView &view, Position cell, float zoom) {
    const auto &v = view.viewport;
    // Match the maintained startup_view projection and the product surface anchor.
    // This is the published renderer adaptation, not certification of the original D helper.
    return {zoom * ((v[0] + v[2]) / 2 + 30.F * (cell.x + cell.y) - view.camera[0]),
            zoom *
                (v[1] + v[3] - (v[1] + v[3]) / 2 - 15.F * (cell.y - cell.x) - 15 + view.camera[1])};
}
} // namespace
WorldEditView world_edit_view(const simulation::StartupWorldRuntimeState &state,
                              std::optional<Position> pointer_cell) {
    WorldEditView view;
    view.mode = state.build_mode;
    view.active =
        state.scene.scene_state == 1 && (view.mode == 1 || view.mode == 2 || view.mode == 3 ||
                                         view.mode == 5 || view.mode == 6 || view.mode == 7);
    if (!view.active)
        return view;
    constexpr const char *prompts[]{"要建在哪里呢",   "从哪里开始铺呢", "铺到哪里呢", "撤除哪里呢",
                                    "从哪里开始撤除", "撤到哪里",       "移动哪个",   "移动去哪里"};
    view.caption = prompts[view.mode];
    view.cancel_label = view.mode == 2 || view.mode == 5 || view.mode == 7 ? "中止" : "返回";
    view.confirm_label = view.mode == 1   ? "选起点"
                         : view.mode == 2 ? "铺设"
                         : view.mode == 6 ? "选设施"
                         : view.mode == 7 ? "移动"
                                          : "撤除";
    const auto &map = state.scene.world.world.map;
    if (pointer_cell && pointer_cell->x >= 0 && pointer_cell->y >= 0 &&
        pointer_cell->x < map.width && pointer_cell->y < map.height)
        view.current_cell = pointer_cell;
    if (view.current_cell && (view.mode == 2 || view.mode == 5)) {
        const auto segment = simulation::startup_world_edit_segment(state, *view.current_cell);
        if (segment)
            view.segment = *segment;
    }
    if (view.mode == 2)
        for (const auto &cell : view.segment)
            if (map.cells.at(static_cast<std::size_t>(cell.y * map.width + cell.x)).legacy_state !=
                1)
                view.road_preview.push_back(cell);
    if (view.mode == 7 && state.rules && state.build_definition) {
        const auto &definitions = state.rules->facilities;
        const auto definition =
            std::find_if(definitions.begin(), definitions.end(),
                         [&](const auto &d) { return d.id == *state.build_definition; });
        view.rotate_allowed = definition != definitions.end() && (definition->flags & 32) != 0;
    }
    return view;
}
std::optional<WorldBuildIntent> world_edit_input(const WorldEditView &view, Extent extent,
                                                 const WorldBuildInput &input, bool blocked) {
    if (!view.active)
        return {};
    return world_build_input(world_build_controls(extent), input, blocked, view.rotate_allowed);
}
void draw_world_edit_preview(const WorldEditView &view, const WorldCameraView &camera, float zoom,
                             Sprites &sprites) {
    if (!view.active || (view.mode != 1 && view.mode != 2))
        return;
    // BOUNDARY: overlay after the scene queue, source frame7, then the original
    // pointer cell frame0. The current cursor never snaps to the projected endpoint.
    for (const auto &cell : view.road_preview)
        sprites.draw("frame.seb", 7, road_anchor(camera, cell, zoom), WHITE,
                     Sprites::Binding::common2, zoom);
    if (view.current_cell)
        sprites.draw("cursor_rect00.seb", 0, road_anchor(camera, *view.current_cell, zoom), WHITE,
                     Sprites::Binding::common, zoom);
}
void draw_world_edit_controls(const WorldEditView &view, Extent extent, const ui::Skin &skin,
                              bool enabled, const std::string &feedback) {
    if (!view.active)
        return;
    const auto controls = world_build_controls(extent);
    skin.button(controls.cancel, view.cancel_label, enabled);
    if (view.rotate_allowed)
        skin.button(controls.rotate, "旋转 R", enabled);
    skin.button(controls.confirm, view.confirm_label, enabled && view.current_cell.has_value());
    const std::string message = feedback.empty() ? view.caption : feedback;
    const Rectangle label{6, extent.height - 73.F, extent.width - 12.F, 16};
    const float size = std::min(11.F, 11.F * label.width / std::max(1.F, skin.text.width(message)));
    skin.text.draw(message, label.x, label.y, feedback.empty() ? ui::ink : MAROON, size);
}
} // namespace ark::desktop
