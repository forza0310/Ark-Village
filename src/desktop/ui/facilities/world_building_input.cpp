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
    // Keep five source rows at ordinary desktop heights; a shorter window scrolls the
    // remaining rows. Other management pages retain their existing responsive geometry.
    const float height =
        raw == 21 ? std::min(300.F, extent.height - 58.F) : std::min(250.F, extent.height - 68.F);
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
    if (raw == 21) {
        // S057: a narrow catalogue begins with the tabs, then five37-pitch picture rows.
        // It has no wood title or buy button. Return is the screen's lower-right soft key.
        constexpr float catalogue_width = 176, catalogue_height = 210;
        layout.panel = {(extent.width - catalogue_width) / 2,
                        (extent.height - catalogue_height) / 2, catalogue_width, catalogue_height};
        if (extent.width < 300)
            layout.panel.y = std::min(layout.panel.y, extent.height - 29.F - 206);
        const auto p = layout.panel;
        layout.row_height = 37;
        layout.body = {p.x + 3, p.y + 21, 162, 185};
        layout.rows = layout.body;
        for (int tab = 0; tab < 3; ++tab)
            layout.tabs[tab] = {p.x + 3 + tab * 57.F, p.y + 3, 57, 16};
        layout.previous = {p.x - 9, p.y + 3, 9, 16};
        layout.next = {p.x + p.width, p.y + 3, 9, 16};
        layout.cancel = Layout(extent).right_button;
        layout.confirm = {}; // Row/Enter activates its bound definition; no hidden command box.
        return layout;
    }
    layout.row_height = raw == 21 ? 37.F : 38.F;
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &p = layout.panel;
    layout.body = {p.x + 12, p.y + 30, width - 24, height - (raw == 21 ? 61.F : 75.F)};
    layout.rows = {p.x + 12, p.y + (raw == 21 ? 56.F : 59.F), width - 24,
                   height - (raw == 21 ? 87.F : 108.F)};
    for (int tab = 0; tab < 3; ++tab)
        layout.tabs[tab] = {layout.body.x + tab * layout.body.width / 3, p.y + 30,
                            layout.body.width / 3, 22};
    layout.cancel = {p.x + 10, p.y + height - 28, 58, 20};
    layout.confirm = {p.x + width - 68, p.y + height - 28, 58, 20};
    layout.previous = {p.x + width / 2 - 32, p.y + height - 28, 28, 20};
    layout.next = {p.x + width / 2 + 4, p.y + height - 28, 28, 20};
    return layout;
}
int world_building_visible_rows(const WorldBuildingLayout &layout) {
    return std::clamp(static_cast<int>(layout.rows.height / layout.row_height), 1, 5);
}
WorldBuildingIcon world_building_icon(const WorldBuildingLayout &layout, int visible_row) {
    if (visible_row < 0 || visible_row >= world_building_visible_rows(layout))
        throw std::invalid_argument("Building icon row is outside the visible catalogue");
    // 2b479f6 PAGES: raw21 uses a 64x32 crop with a +(2,10) source anchor, never a fitted image.
    const Rectangle clip{layout.rows.x + 2, layout.rows.y + visible_row * layout.row_height + 2, 64,
                         32};
    return {clip, {clip.x + 2, clip.y + 10}};
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
    if (view.raw == 21 &&
        (selection.marker_page != view.page || input.enter || input.escape || input.up ||
         input.down || input.left || input.right || input.wheel_rows)) {
        selection.marked_definition.reset();
        selection.marker_page = view.page;
    }
    const auto intent = [&](Action action, int identity = 0) {
        return WorldBuildingIntent{action, view.page, identity};
    };
    if (view.raw != 81 && (input.escape || hit(input.click, layout.cancel)))
        return intent(view.raw == 21   ? Action::cancel_build
                      : view.raw == 80 ? Action::residence_cancel
                                       : Action::facility_cancel);
    bool confirm = input.enter || hit(input.click, layout.confirm);
    if (view.raw == 74) {
        if (view.phase == 1 && !view.definition_preview) {
            const auto details = world_building_detail_layout(layout, view.detail_type);
            const int visible =
                std::min(5, static_cast<int>(details.sources.height / details.source_row_height));
            const int scroll = input.wheel_rows + (input.down ? 1 : 0) - (input.up ? 1 : 0);
            selection.first_row =
                std::clamp(selection.first_row + scroll, 0,
                           std::max(0, static_cast<int>(view.bonus_rows.size()) - visible));
        } else
            selection.first_row = 0;
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
    selection.tab = std::clamp(selection.tab, 0, 2);
    if (view.raw == 21) {
        const int old = selection.tab;
        if (input.left || hit(input.click, layout.previous))
            selection.tab = (selection.tab + 2) % 3;
        if (input.right || hit(input.click, layout.next))
            selection.tab = (selection.tab + 1) % 3;
        for (int tab = 0; tab < 3; ++tab)
            if (hit(input.click, layout.tabs[tab]))
                selection.tab = tab;
        if (old != selection.tab) {
            selection.selected = selection.first_row = 0;
            selection.marked_definition.reset();
        }
    }
    const auto &list = world_building_rows(view, selection.tab);
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
            const bool marked =
                selection.selected == index && selection.marked_definition == list[index].identity;
            selection.selected = row + selection.first_row;
            confirm = view.raw != 21 || marked;
            if (view.raw == 21)
                selection.marked_definition = list[index].identity;
        }
    if (confirm && view.can_confirm)
        return intent(view.raw == 21 ? Action::select_build : Action::residence_select,
                      list[selection.selected].identity);
    return {};
}
} // namespace ark::desktop::ui
