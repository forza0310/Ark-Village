// Desktop geometry and page-bound intents; the worker revalidates every purchase or upgrade.
#include "world_building.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using Action = WorldBuildingAction;
bool hit(const std::optional<Vector2> &point, Rectangle box) {
    return point && point->x >= box.x && point->y >= box.y && point->x < box.x + box.width &&
           point->y < box.y + box.height;
}
}
WorldBuildingLayout world_building_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Building page requires the supported logical viewport");
    const float width = std::min(raw == 74 ? 224.F : 310.F, extent.width - 16.F);
    const float height = std::min(250.F, extent.height - 68.F);
    WorldBuildingLayout layout;
    if (raw == 81) {
        layout.panel = {(extent.width - 222.F) / 2, (extent.height - 170.F) / 2, 222, 170};
        layout.body = {layout.panel.x + 6, layout.panel.y + 104, 209, 63};
        // Existing PC confirmation area is an explicit adaptation; no invented source rectangle.
        layout.confirm = {layout.panel.x + 174, layout.panel.y + 151, 42, 18};
        return layout;
    }
    if (raw == 74) {
        // S019 and the supplied page2 share a compact ~224x172 window. PAGES gives
        // the 97x74 picture and two right-hand fields; desktop centering is an adaptation.
        layout.panel = {(extent.width - 224.F) / 2, (extent.height - 172.F) / 2, 224, 172};
        const auto p = layout.panel;
        layout.body = {p.x + 8, p.y + 40, 208, 110};
        layout.rows = {p.x + 12, p.y + 44, 196, 102};
        layout.cancel = Layout(extent).right_button;
        const float available = layout.cancel.x - 64;
        layout.footer_name = {62, extent.height - 20.F, available * .48F, 19};
        layout.footer_profit = {layout.footer_name.x + layout.footer_name.width + 2,
                                layout.footer_name.y, available * .52F - 2, 19};
        layout.confirm = {p.x + 62, p.y + 152, 100, 18};
        layout.previous = {p.x + 8, p.y + 2, 14, 17};
        layout.next = {p.x + 202, p.y + 2, 14, 17};
        return layout;
    }
    layout.row_height = 38.F;
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &p = layout.panel;
    layout.body = {p.x + 12, p.y + 30, width - 24, height - 75.F};
    layout.rows = {p.x + 12, p.y + 59.F, width - 24,
                   height - 108.F};
    layout.cancel = {p.x + 10, p.y + height - 28, 58, 20};
    layout.confirm = {p.x + width - 68, p.y + height - 28, 58, 20};
    layout.previous = {p.x + width / 2 - 32, p.y + height - 28, 28, 20};
    layout.next = {p.x + width / 2 + 4, p.y + height - 28, 28, 20};
    return layout;
}
int world_building_visible_rows(const WorldBuildingLayout &layout) {
    return std::clamp(static_cast<int>(layout.rows.height / layout.row_height), 1, 5);
}
WorldBuildingDetailLayout world_building_detail_layout(const WorldBuildingLayout &layout,
                                                       app::WorldFacilityTemplate type) {
    const auto b = layout.body;
    const auto p = layout.panel;
    WorldBuildingDetailLayout out;
    out.name = {p.x + 30, p.y + 25, 90, 14};
    out.price = {p.x + 124, p.y + 25, 87, 14};
    out.picture = {type == app::WorldFacilityTemplate::ordinary ? p.x + 13
                                                                : b.x + (b.width - 97) / 2,
                   p.y + 44, 97, 74};
    out.level = {out.picture.x + 3, out.picture.y + 61, 44, 10};
    out.values = {p.x + 113, p.y + 44, 90, 34};
    out.effects = {p.x + 113, p.y + 80, 90, 38};
    out.remaining = {p.x + 30, p.y + 128, 173, 14};
    out.source_heading = {p.x + 17, p.y + 25, 88, 14};
    out.maintenance = {p.x + 124, p.y + 25, 87, 14};
    out.sources = {b.x + 6, b.y + 6, b.width - 18, b.height - 12};
    out.source_scroll = {b.x + b.width + 1, b.y, 5, b.height};
    out.source_footer = {p.x + 15, p.y + 153, p.width - 30, 17};
    return out;
}
std::optional<WorldBuildingIntent> world_building_input(const WorldBuildingView &view,
                                                        const WorldBuildingLayout &layout,
                                                        WorldBuildingSelection &selection,
                                                        const WorldBuildingInput &input,
                                                        bool blocked) {
    if (blocked || !view.initialized)
        return {};
    const auto intent = [&](Action action, int identity = 0) {
        return WorldBuildingIntent{action, view.page, identity};
    };
    if (view.raw != 81 && (input.escape || hit(input.click, layout.cancel)))
        return intent(view.raw == 80 ? Action::residence_cancel : Action::facility_cancel);
    bool confirm = input.enter || hit(input.click, layout.confirm);
    if (view.raw == 74) {
        if (view.phase == 1 && !view.definition_preview) {
            const int count = static_cast<int>(view.bonus_rows.size());
            const auto chosen = simulation::steam_facility_bonus_cursor(
                count, {selection.selected, selection.first_row}, input.up, input.down);
            if (!chosen)
                throw std::invalid_argument("Facility bonus selection is outside its source list");
            auto cursor = *chosen;
            if (input.wheel_rows) {
                const auto scrolled = simulation::steam_facility_bonus_scroll(
                    count, cursor, simulation::SteamFacilityScrollEvent::relative, input.wheel_rows);
                if (!scrolled)
                    throw std::invalid_argument("Invalid facility bonus wheel event");
                cursor = *scrolled;
            }
            // PC row clicks select only. Physical scrollbar dragging remains separate
            // from the source's logical relative/absolute events.
            for (int row = 0; row < 5 && cursor.first + row < count; ++row)
                if (hit(input.click, {layout.body.x, layout.panel.y + 50 + row * 19.F,
                                      layout.body.width, 18}))
                    cursor.selection = cursor.first + row;
            selection.selected = cursor.selection;
            selection.first_row = cursor.first;
        } else if (view.definition_preview)
            selection.selected = selection.first_row = 0;
        if (view.page_count > 1 && (input.left || hit(input.click, layout.previous)))
            return intent(Action::facility_previous);
        if (view.page_count > 1 && (input.right || hit(input.click, layout.next)))
            return intent(Action::facility_next);
        if (confirm && view.can_confirm)
            return intent(Action::facility_confirm);
        return {};
    }
    if (view.raw == 81)
        return confirm && view.can_confirm ? std::optional{intent(Action::confirm_upgrade)}
                                           : std::nullopt;
    const auto &list = view.residents;
    if (list.empty()) {
        selection.selected = selection.first_row = 0;
        return {};
    }
    const int count = static_cast<int>(list.size());
    selection.selected = std::clamp(selection.selected, 0, count - 1);
    if (input.up)
        selection.selected = (selection.selected + count - 1) % count;
    if (input.down)
        selection.selected = (selection.selected + 1) % count;
    const int visible = world_building_visible_rows(layout);
    if (input.wheel_rows)
        selection.selected = std::clamp(selection.selected + input.wheel_rows, 0, count - 1);
    selection.first_row = std::clamp(selection.first_row, 0, std::max(0, count - visible));
    selection.first_row = std::min(selection.first_row, selection.selected);
    selection.first_row = std::max(selection.first_row, selection.selected - visible + 1);
    for (int row = 0; row < visible && row + selection.first_row < count; ++row)
        if (hit(input.click, {layout.rows.x, layout.rows.y + row * layout.row_height,
                              layout.rows.width, layout.row_height})) {
            const int index = row + selection.first_row;
            selection.selected = index;
            confirm = true;
        }
    if (confirm && view.can_confirm)
        return intent(Action::residence_select, list[selection.selected].identity);
    return {};
}
} // namespace ark::desktop::ui
