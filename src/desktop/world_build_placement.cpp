#include "world_build_placement.hpp"
#include "ui/layout.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

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
WorldBuildGraphic world_build_graphic(const simulation::StartupDefinition &definition,
                                      rules::FacilityOrientation orientation) {
    const auto &displays = simulation::startup_evidence().displays;
    const auto display = std::find_if(displays.begin(), displays.end(), [&](const auto &item) {
        return item.id == definition.display_id;
    });
    if (display == displays.end())
        throw std::invalid_argument("Building graphic references an unknown map display");
    // A small local grid only obtains source relative offsets, not a second business map.
    const auto footprint = rules::facility_footprint(
        static_cast<rules::FacilityShape>(definition.shape), orientation, {1, 0}, 3, 3);
    if (footprint.error != rules::GeometryError::none)
        throw std::invalid_argument("Building graphic has an unsupported source footprint");
    WorldBuildGraphic graphic;
    graphic.sprite = display->sprite;
    for (const auto &cell : footprint.cells) {
        const int x = cell.position.x - 1, y = cell.position.y;
        const int frame = definition.kind == 6
                              ? (orientation == rules::FacilityOrientation::second ? 1 : 11)
                              : cell.fragment_index;
        graphic.frames.push_back({frame, {30.F * (x + y), 15.F * (x - y)}});
    }
    return graphic;
}
WorldBuildControls world_build_controls(Extent extent) {
    return {ui::Layout(extent).scene,
            {6, extent.height - 54.F, 58, 22},
            {70, extent.height - 54.F, 58, 22},
            {extent.width - 76.F, extent.height - 54.F, 70, 22}};
}
std::optional<WorldBuildIntent> world_build_input(const WorldBuildControls &controls,
                                                  const WorldBuildInput &input, bool blocked,
                                                  bool rotation_allowed) {
    if (blocked)
        return {};
    const auto hit = [&](Rectangle box) {
        return input.click && input.click->x >= box.x && input.click->y >= box.y &&
               input.click->x < box.x + box.width && input.click->y < box.y + box.height;
    };
    using Action = WorldBuildAction;
    if (input.escape || hit(controls.cancel))
        return WorldBuildIntent{Action::cancel, {}};
    if (rotation_allowed && hit(controls.rotate))
        return WorldBuildIntent{Action::rotate, {}};
    if (hit(controls.confirm))
        return WorldBuildIntent{Action::confirm, {}};
    // A new map selection consumes this frame even if Enter/R was pressed simultaneously.
    // The controller stores the new anchor, and a later distinct intent may commit it.
    if (hit(controls.scene))
        return WorldBuildIntent{Action::choose, input.click};
    if (input.rotate)
        return rotation_allowed ? std::optional{WorldBuildIntent{Action::rotate, {}}}
                                : std::nullopt;
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
    preview.graphic = world_build_graphic(*item, orientation);
    const auto &world = state.scene.world.world;
    preview.cursor_in_map = position.x >= 0 && position.y >= 0 && position.x < world.map.width &&
                            position.y < world.map.height;
    preview.rotation_hint = (item->flags & 32) != 0;
    // 2b479f6 PAGES: f103b is the admitted scene-update counter, not render frames/time.
    // An in-map cursor may still display a building rejected by footprint/funds checks.
    preview.graphic_visible = preview.cursor_in_map && state.scene.scene_state == 1 &&
                              (state.build_mode == 0 || state.build_mode == 7) &&
                              state.scene.scene_counter >= 0 && state.scene.scene_counter % 20 < 10;
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
    if (state.build_mode == 7) {
        // Moving preserves an existing facility; a fresh construction quote is irrelevant.
        preview.cost = 300;
        preview.missing_source =
            state.next_facility_identity == std::numeric_limits<std::uint64_t>::max();
        if (preview.cost > world.ai.accounting.funds())
            preview.denial = Denial::insufficient_funds;
        return preview;
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
                              float zoom, Sprites &sprites) {
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
    if (preview.graphic_visible) {
        // Match the installed surface's raster anchor. The cursor's independent pulse does
        // not become a building fade; the source building is either drawn normally or absent.
        const auto p = anchor(view, 30.F * (preview.anchor.x + preview.anchor.y),
                              15.F * (preview.anchor.y - preview.anchor.x) + 15, zoom);
        for (const auto &[frame, offset] : preview.graphic.frames)
            sprites.draw(preview.graphic.sprite, frame,
                         {p.x + offset.x * zoom, p.y + offset.y * zoom}, WHITE,
                         Sprites::Binding::map, zoom);
    }
}
} // namespace ark::desktop
