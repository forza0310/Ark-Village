#include "world_build_menu.hpp"
#include "../common/world_list_input.hpp"
#include "../../resources/resources.hpp"
#include "ark/simulation/presentation/steam_manual_skin.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
Rectangle area(std::array<int, 4> r, const std::array<int, 4> &margin, Vector2 origin) {
    return {origin.x + r[0] - margin[0], origin.y + r[1] - margin[2],
            float(r[2] + margin[0] + margin[1]), float(r[3] + margin[2] + margin[3])};
}
bool same_cursor(const sim::StartupBuildMenuCursor &a, const sim::StartupBuildMenuCursor &b) {
    return a.page == b.page && a.selection == b.selection && a.scroll == b.scroll;
}
} // namespace
WorldBuildMenuControls world_build_menu_controls(const WorldBuildMenuView &view, Extent extent,
                                                Sprites &sprites, std::uint64_t generation,
                                                std::uint64_t epoch) {
    if (!view.initialized || !view.skin)
        throw std::invalid_argument("Build menu controls require the measured source plan");
    const auto origin = world_build_menu_origin(extent);
    WorldBuildMenuControls out;
    out.page = view.page;
    out.generation = generation;
    out.epoch = epoch;
    out.cursor = view.source.cursor;
    out.catalogs = view.source.catalogs;
    out.extent = extent;
    out.cancel = {float(extent.width - 58), float(extent.height - 25), 54, 21};
    if (view.skin->soft_labels[0] == 7)
        out.details = {4, float(extent.height - 25), 54, 21};
    for (const auto &operation : view.skin->operations) {
        if (const auto *part = std::get_if<sim::SteamBuildMenuTouch>(&operation)) {
            if (!part->touch.rectangle)
                continue;
            const auto r = area(*part->touch.rectangle, part->margin, origin);
            const int component = part->touch.component;
            if (component == 11) {
                const int visible = (part->touch.value & 0xffff) - out.cursor.scroll;
                if (visible < 0 || visible >= 5 || part->touch.option != 16)
                    throw std::invalid_argument("Build menu row lost its source absolute marker");
                out.rows.at(visible) = r; // All five published rows exist, including empty rows.
            } else if (component == 3) {
                out.categories.at(part->touch.value & 0xffff) = r;
            } else if (component == 12) {
                out.thumb = r;
                if (part->drag_grid_y)
                    out.drag_grid = *part->drag_grid_y;
            } else if (component == 25)
                out.track = r;
        } else if (const auto *part = std::get_if<sim::SteamBuildMenuArrowTouch>(&operation)) {
            const auto resource = sim::steam_facility_resource(sim::SteamFacilityAsset::title_arrows);
            if (!resource || !resource->published_sprite)
                throw std::invalid_argument("Build menu arrow has no published SEB bounds");
            const auto bounds = sprites.frame_bounds(
                std::filesystem::path(resource->published_sprite).filename().string(), part->frame);
            const std::array<int, 4> r{part->position[0] + int(bounds.x),
                                        part->position[1] + int(bounds.y), int(bounds.width),
                                        int(bounds.height)};
            if (part->value == 16)
                out.previous = area(r, part->margin, origin);
            else if (part->value == 18)
                out.next = area(r, part->margin, origin);
            else
                throw std::invalid_argument("Build menu arrow has an unsupported source value");
        }
    }
    // The drag grid is installed only in the source's active-drag plan. Keep its float32
    // arithmetic when starting the PC gesture rather than using an unrelated list's thumb.
    const int count = int(out.catalogs.at(out.cursor.page).size());
    if (count > 5 && out.drag_grid <= 0) {
        const int length = 945 / std::max(count, 5);
        const float grid = float(189 - length) / float(count - 5);
        out.drag_grid = grid > 0 ? grid : 1.F;
    }
    return out;
}
std::optional<sim::StartupBuildMenuInput>
world_build_menu_input(const WorldBuildMenuView &view, Extent extent, std::uint64_t generation,
                       std::uint64_t epoch, const std::optional<WorldBuildMenuControls> &painted,
                       WorldBuildMenuGesture &gesture, const WorldBuildMenuInput &in,
                       bool blocked) {
    const auto cursor = view.source.cursor;
    if (gesture.page != view.page || gesture.generation != generation ||
        gesture.category != cursor.page || in.focus_lost) {
        gesture = {};
        gesture.page = view.page;
        gesture.generation = generation;
        gesture.category = cursor.page;
    }
    if (in.keyboard_event || in.up || in.down || in.left || in.right || in.enter || in.escape ||
        in.inspect || in.wheel_rows)
        gesture.marked_row.reset();
    const bool fresh = painted && painted->page == view.page && painted->generation == generation &&
                       painted->epoch <= epoch && same_cursor(painted->cursor, cursor) &&
                       painted->catalogs == view.source.catalogs &&
                       painted->extent.width == extent.width && painted->extent.height == extent.height;
    if (in.focus_lost || !view.initialized || !view.interactive) {
        gesture.scrolling = false;
        return {};
    }
    if (blocked)
        return {}; // A pending selection or Draw cannot consume a second pointer release.
    sim::StartupBuildMenuInput out;
    out.directions = {in.up, in.down, in.left, in.right};
    out.confirm = in.enter;
    out.cancel = in.escape;
    const int selected_visible = cursor.selection - cursor.scroll;
    const bool details = selected_visible >= 0 &&
                         selected_visible < int(view.source.visible_rows.size()) &&
                         view.source.visible_rows[selected_visible].definition >= 0 &&
                         (view.source.visible_rows[selected_visible].kind == 2 ||
                          view.source.visible_rows[selected_visible].kind == 3 ||
                          view.source.visible_rows[selected_visible].kind == 13);
    out.details = in.inspect && details;
    const int count = int(view.source.catalogs.at(cursor.page).size());
    const int upper = std::max(count - 1, 4);
    std::optional<int> first;
    bool consumed = gesture.consumed;
    if (fresh) {
        if (count > 5 && in.pointer_pressed && world_facility_hit(in.pointer, painted->track)) {
            consumed = gesture.consumed = true;
            gesture.marked_row.reset();
            if (painted->thumb && world_facility_hit(in.pointer, *painted->thumb)) {
                gesture.scrolling = true;
                gesture.start_y = in.pointer->y;
                gesture.start_first = cursor.scroll;
                gesture.grid = painted->drag_grid;
            } else {
                const bool above = painted->thumb && in.pointer->y < painted->thumb->y;
                first = std::clamp(cursor.scroll + (above ? -5 : 5), 0, count - 5);
            }
        }
        if (gesture.scrolling && in.pointer && (in.pointer_down || in.pointer_released)) {
            const double wanted = gesture.start_first +
                                  std::trunc(double(in.pointer->y - gesture.start_y) / gesture.grid);
            first = int(std::clamp(wanted, 0., double(count - 5)));
        }
        if (!consumed) {
            out.cancel |= world_facility_hit(in.click, painted->cancel);
            out.details |= world_facility_hit(in.click, painted->details);
            out.directions.left |= world_facility_hit(in.click, painted->previous);
            out.directions.right |= world_facility_hit(in.click, painted->next);
            consumed = world_facility_hit(in.click, painted->cancel) ||
                       world_facility_hit(in.click, painted->details) ||
                       world_facility_hit(in.click, painted->previous) ||
                       world_facility_hit(in.click, painted->next);
            for (int c = 0; c < 3; ++c)
                if (world_facility_hit(in.click, painted->categories[c])) {
                    out.page = c;
                    gesture.marked_row.reset();
                    consumed = true;
                    break;
                }
            for (int r = 0; !consumed && r < 5; ++r) {
                const int row = cursor.scroll + r;
                if (row <= upper && world_facility_hit(in.click, painted->rows[r])) {
                    out.selection = row;
                    out.confirm |= gesture.marked_row == row && cursor.selection == row;
                    gesture.marked_row = row;
                    break;
                }
            }
        }
    }
    if (in.wheel_rows && fresh && count > 5)
        first = int(std::clamp(std::int64_t(cursor.scroll) + in.wheel_rows, std::int64_t{0},
                              std::int64_t(count - 5)));
    if (first) {
        out.scroll = *first;
        out.selection = std::clamp(cursor.selection, *first, std::min(upper, *first + 4));
        gesture.marked_row.reset();
    }
    if (out.directions.left || out.directions.right || out.cancel || out.details)
        gesture.marked_row.reset();
    if (in.pointer_released || !in.pointer)
        gesture.scrolling = gesture.consumed = false;
    if (out.page || out.selection || out.scroll || out.directions.up || out.directions.down ||
        out.directions.left || out.directions.right || out.confirm || out.cancel || out.details)
        return out;
    return {};
}
} // namespace ark::desktop::ui
