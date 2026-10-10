// ui/DUNGEON_RENDER.md:21-41 defines the task bar and separately sorted target diamond.
#include "ark/presentation/world_dungeon_visuals.hpp"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace ark::desktop {
namespace {
int mapped(int progress, int extent, int width) {
    if (extent == 0)
        return 0;
    return static_cast<int>(
        std::clamp<std::int64_t>(static_cast<std::int64_t>(progress) * width / extent, 0, width));
}
} // namespace
WorldDungeonView world_dungeon_view(const simulation::StartupWorldRuntimeState &state) {
    WorldDungeonView view;
    if (!state.active_task)
        return view;
    if (!state.rules)
        throw std::invalid_argument("Task display requires the source catalogue");
    const auto &task = state.tasks.at(*state.active_task);
    const auto definition =
        std::find_if(state.rules->tasks.begin(), state.rules->tasks.end(),
                     [&](const auto &d) { return d.factory.identity == task.definition; });
    if (definition == state.rules->tasks.end() || !task.site)
        throw std::invalid_argument("Task display requires an actual definition and map site");
    view.active = true;
    view.site = *task.site;
    view.kind = definition->factory.kind;
    if (!task.facility)
        return view;
    const auto &world = state.scene.world.world;
    const auto &facility = world.facilities.at(*task.facility);
    const auto &progress = state.dungeon_facilities.at(*task.facility);
    const auto tenant =
        std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                     [&](const auto &d) { return d.id == facility.placement.definition_id; });
    if (tenant == state.rules->facilities.end())
        throw std::invalid_argument("Task display references an unknown facility definition");
    view.has_facility = true;
    view.phase = facility.status;
    view.progress = progress.progress;
    view.extent = progress.extent;
    view.special_marker = (tenant->flags & 524288) != 0;
    // Use ordered footprint geometry and the current surface display binding. Multi-cell
    // facilities can select different images; a catalogue default image is not sufficient.
    const auto &placement = facility.placement;
    const auto footprint =
        simulation::rules::facility_footprint(placement.shape, placement.orientation,
                                              placement.anchor, world.map.width, world.map.height);
    if (footprint.cells.empty())
        throw std::invalid_argument("Task display requires a valid occupied footprint");
    const auto cell = footprint.cells.front().position;
    const auto &surface =
        state.surface.at(static_cast<std::size_t>(cell.y) * world.map.width + cell.x);
    const auto &displays = simulation::startup_evidence().displays;
    const auto display = std::find_if(displays.begin(), displays.end(), [&](const auto &d) {
        return d.id == surface.display_definition;
    });
    if (display == displays.end())
        throw std::invalid_argument("Task display has no maintained tenant sprite binding");
    view.tenant_sprite = display->sprite;
    view.tenant_frame = surface.variant;
    return view;
}
std::vector<WorldDungeonLayer> world_dungeon_visuals(const WorldDungeonView &view,
                                                     int tenant_png_height, bool alternate_text) {
    if (!view.active)
        return {};
    if (view.extent < 0)
        throw std::invalid_argument("Task display extent must be nonnegative");
    std::vector<WorldDungeonLayer> layers;
    OverlayPlan bar;
    // A = D + (4,-21). No bound instance has a label but no fabricated progress value.
    if (!view.has_facility) {
        bar.emplace_back(OverlaySprite{"cursor_rect02.seb", 1, 29, -17});
    } else if (view.phase == 1 && view.progress >= 0) {
        if (tenant_png_height <= 0)
            throw std::invalid_argument("Task progress bar requires the bound tenant PNG height");
        const float y = -21.F + 38 - tenant_png_height;
        bar.emplace_back(OverlayImage{"dungeon_bar.png", {0, 0, 52, 10}, {4, y, 52, 10}});
        const float fill = static_cast<float>(mapped(view.progress, view.extent, 50));
        if (fill > 0)
            bar.emplace_back(
                OverlayImage{"dungeon_bar.png", {1, 10, fill, 8}, {5, y + 1, fill, 8}});
        bar.emplace_back(
            OverlaySprite{"cursor_rect02.seb", 1, alternate_text ? 12.F : 16.F, y - 1});
        const int percent = mapped(view.progress, view.extent, 100);
        const auto digits = std::to_string(percent);
        const float edge = 64.F + (alternate_text ? 4.F : 0.F);
        float x = edge - 9 - digits.size() * 8.F;
        for (const char digit : digits) {
            bar.emplace_back(OverlaySprite{"number05.seb", digit - '0', x, y - 12});
            x += 8;
        }
        bar.emplace_back(OverlaySprite{"menuRT01.seb", 0, edge - 9, y - 12});
    }
    if (!bar.empty())
        layers.push_back({0, std::move(bar)});
    if (view.kind == 0 && view.has_facility) {
        OverlayPlan marker;
        marker.emplace_back(
            OverlaySprite{view.special_marker ? "cursor_rect03.seb" : "cursor_rect02.seb", 0,
                          view.special_marker ? 0.F : -2.F, 0});
        layers.push_back({20, std::move(marker)});
    }
    return layers;
}
} // namespace ark::desktop
