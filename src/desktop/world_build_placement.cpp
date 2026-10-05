#include "world_build_placement.hpp"
#include "ui/layout.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace ark::desktop {
namespace {
namespace rules = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
using Denial = simulation::StartupBuildDenial;
Vector2 anchor(const WorldCameraView &view, float x, float y, float zoom) {
    const auto &v = view.viewport;
    return {zoom * ((v[0] + v[2]) / 2 + x - view.camera[0]),
            zoom * (v[1] + v[3] - (v[1] + v[3]) / 2 - y + view.camera[1])};
}
} // namespace
WorldBuildControls world_build_controls(Extent extent) {
    return {ui::Layout(extent).scene,
            {6, extent.height - 54.F, 58, 22},
            {70, extent.height - 54.F, 58, 22},
            {extent.width - 76.F, extent.height - 54.F, 70, 22}};
}
std::optional<WorldBuildIntent> world_build_input(const WorldBuildControls &controls,
                                                  const WorldBuildInput &input, bool blocked) {
    if (blocked)
        return {};
    const auto hit = [&](Rectangle box) {
        return input.click && input.click->x >= box.x && input.click->y >= box.y &&
               input.click->x < box.x + box.width && input.click->y < box.y + box.height;
    };
    using Action = WorldBuildAction;
    if (input.escape || hit(controls.cancel))
        return WorldBuildIntent{Action::cancel, {}};
    if (hit(controls.rotate))
        return WorldBuildIntent{Action::rotate, {}};
    if (hit(controls.confirm))
        return WorldBuildIntent{Action::confirm, {}};
    // A new map selection consumes this frame even if Enter/R was pressed simultaneously.
    // The controller stores the new anchor, and a later distinct intent may commit it.
    if (hit(controls.scene))
        return WorldBuildIntent{Action::choose, input.click};
    if (input.rotate)
        return WorldBuildIntent{Action::rotate, {}};
    if (input.enter)
        return WorldBuildIntent{Action::confirm, {}};
    return {};
}
std::optional<rules::Position> world_pick_cell(const State &state, const WorldCameraView &view,
                                               Vector2 pointer, float zoom) {
    if (!std::isfinite(zoom) || zoom <= 0 || !std::isfinite(pointer.x) || !std::isfinite(pointer.y))
        return {};
    const auto origin = anchor(view, 0, 0, zoom);
    const double x = (pointer.x - origin.x) / zoom;
    const double y = (origin.y - pointer.y) / zoom;
    const double grid_x = std::floor((x / 30 - y / 15) / 2);
    const double grid_y = std::floor((x / 30 + y / 15) / 2);
    const auto &map = state.scene.world.world.map;
    if (grid_x < 0 || grid_y < 0 || grid_x >= map.width || grid_y >= map.height)
        return {};
    return rules::Position{static_cast<int>(grid_x), static_cast<int>(grid_y)};
}
WorldBuildPreview world_build_preview(const State &state, int id, rules::Position position,
                                      rules::FacilityOrientation orientation) {
    WorldBuildPreview preview;
    preview.definition = id;
    preview.anchor = position;
    preview.orientation = orientation;
    if (!state.rules) {
        preview.missing_source = true;
        return preview;
    }
    const auto &defs = state.rules->facilities;
    const auto item = std::find_if(defs.begin(), defs.end(),
                                   [id](const auto &definition) { return definition.id == id; });
    if (item == defs.end() || state.fence_level < 0 ||
        static_cast<std::size_t>(state.fence_level) >= state.rules->fences.size()) {
        preview.missing_source = true;
        return preview;
    }
    const auto &world = state.scene.world.world;
    const auto footprint =
        rules::facility_footprint(static_cast<rules::FacilityShape>(item->shape), orientation,
                                  position, world.map.width, world.map.height);
    preview.cells = footprint.cells;
    if (footprint.error != rules::GeometryError::none) {
        preview.denial = Denial::outside_map;
        preview.missing_source = footprint.error != rules::GeometryError::outside_map;
        return preview;
    }
    // Keep the published install_facility order: whole footprint occupancy/town, then quote/cash.
    // Source begin_build owns catalogue availability; preview never opens or selects a source page.
    const auto &bounds = state.rules->fences.at(state.fence_level);
    for (const auto &cell : preview.cells) {
        const auto index =
            static_cast<std::size_t>(cell.position.y * world.map.width + cell.position.x);
        if (index >= world.map.cells.size()) {
            preview.missing_source = true;
            return preview;
        }
        const auto &tile = world.map.cells[index];
        if (tile.legacy_state == 1 || tile.legacy_state == 10 || tile.legacy_state == 2 ||
            tile.facility) {
            preview.denial = Denial::occupied;
            return preview;
        }
        if (cell.position.x <= bounds[0].x || cell.position.x >= bounds[1].x ||
            cell.position.y >= bounds[0].y || cell.position.y <= bounds[1].y) {
            preview.denial = Denial::outside_town;
            return preview;
        }
    }
    const auto quote = simulation::startup_world_build_quote(state, id);
    if (!quote || quote->construction_cost > std::numeric_limits<int>::max() ||
        quote->construction_ticks > std::numeric_limits<int>::max() ||
        state.next_facility_identity == std::numeric_limits<std::uint64_t>::max()) {
        preview.missing_source = true;
        return preview;
    }
    preview.cost = quote->construction_cost;
    if (preview.cost > world.ai.accounting.funds())
        preview.denial = Denial::insufficient_funds;
    return preview;
}
void draw_world_build_preview(const WorldBuildPreview &preview, const WorldCameraView &view,
                              float zoom) {
    const Color color = preview.valid() ? Color{70, 194, 94, 190} : Color{222, 74, 58, 190};
    for (const auto &cell : preview.cells) {
        const float x = static_cast<float>(cell.position.x),
                    y = static_cast<float>(cell.position.y);
        const auto center = anchor(view, 30 * (x + y) + 30, 15 * (y - x), zoom);
        const Vector2 top{center.x, center.y - 15 * zoom};
        const Vector2 right{center.x + 30 * zoom, center.y};
        const Vector2 bottom{center.x, center.y + 15 * zoom};
        const Vector2 left{center.x - 30 * zoom, center.y};
        DrawTriangle(top, left, bottom, Fade(color, .3F));
        DrawTriangle(top, bottom, right, Fade(color, .3F));
        DrawLineEx(top, right, zoom, color);
        DrawLineEx(right, bottom, zoom, color);
        DrawLineEx(bottom, left, zoom, color);
        DrawLineEx(left, top, zoom, color);
    }
}
} // namespace ark::desktop
