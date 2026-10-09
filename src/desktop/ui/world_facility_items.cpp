#include "world_facility_items.hpp"
#include "../world_overlay_render.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupFacilityItemAction;
bool hit(const WorldFacilityItemsInput &input, Rectangle box) {
    return input.click && input.click->x >= box.x && input.click->y >= box.y &&
           input.click->x < box.x + box.width && input.click->y < box.y + box.height;
}
void fitted(const Skin &skin, const std::string &value, Rectangle box, Color color = ink) {
    skin.text.draw(value, box.x, box.y, color,
                   std::min(11.F, 12.F * box.width / std::max(1.F, skin.text.width(value))));
}
WorldFacilityItemRow item(const State &state, int id) {
    const auto source = std::find_if(state.rules->items.begin(), state.rules->items.end(),
                                     [id](const auto &d) { return d.identity == id; });
    if (source == state.rules->items.end())
        throw std::invalid_argument("Facility item view lost its item definition");
    WorldFacilityItemRow row;
    row.identity = id;
    row.name = source->name;
    row.owned = state.items.at(id).inventory;
    const auto icon = simulation::startup_world_item_icon_draws(state, id);
    if (!icon)
        throw std::invalid_argument("Facility item has invalid icon data");
    row.icon = *icon;
    for (int n = 0; n < 3; ++n) {
        const int value = source->facility_improvements[n];
        row.hint[n] = value == 0                   ? 0
                      : value < (n == 0 ? 50 : 3)  ? 1
                      : value < (n == 0 ? 100 : 6) ? 2
                                                   : 3;
    }
    return row;
}
} // namespace
bool world_facility_items_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           page.legacy_page >= 75 && page.legacy_page <= 77;
}
WorldFacilityItemsView world_facility_items_view(const State &state, const Page &page) {
    if (!state.rules || !world_facility_items_page(page))
        throw std::invalid_argument("Facility item view requires a supported source page");
    WorldFacilityItemsView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = "设施强化"; // S044 and S045 share this caption; no change to page identity.
    if (!state.facility_item_pages_initialized.count(page.id))
        return out;
    if (!simulation::valid_startup_world_facility_item_page(state, page))
        throw std::invalid_argument("Facility item view has invalid initialized source payload");
    out.facility = state.facility_page_bindings.at(page.id);
    const auto &instance = state.scene.world.world.facilities.at(out.facility);
    const auto definition =
        std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                     [&](const auto &d) { return d.id == page.legacy_f; });
    if (definition == state.rules->facilities.end())
        throw std::invalid_argument("Facility item view lost its facility definition");
    out.name = definition->name;
    // PAGES: the facility footer is cumulative v[0..month][income - cost], only kinds3/9.
    // The validated Owner supplies the 12-month array; this read cannot charge or reward.
    if (definition->kind == 3 || definition->kind == 9) {
        out.profit = 0;
        const auto &months = state.facility_monthly_cash.at(out.facility);
        for (int month = 0; month <= state.scene.calendar.month; ++month)
            *out.profit += static_cast<std::int64_t>(months.at(month)[0]) - months.at(month)[1];
    }
    out.graphic = world_build_graphic(state, definition->id, instance.placement.orientation);
    out.counter = state.page_counters.at(page.id);
    out.response = state.facility_item_response;
    if (out.raw == 75) {
        out.selection = state.facility_item_page_selections.at(page.id);
        for (const int id : state.facility_item_page_lists.at(page.id))
            out.rows.push_back(item(state, id));
        out.first_visible =
            std::clamp(out.selection - 4, 0, std::max(0, static_cast<int>(out.rows.size()) - 5));
        out.choice = out.rows.at(out.selection);
    } else {
        out.choice = item(state, state.facility_item_page_items.at(page.id));
        out.attributes = state.facility_upgrade_display;
    }
    out.can_cancel = out.raw == 75;
    out.can_confirm = out.raw == 75 || (out.raw == 77 && (out.counter < 49 || out.counter >= 55));
    out.initialized = true;
    return out;
}
WorldFacilityItemsLayout world_facility_items_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Facility item page requires supported logical viewport");
    WorldFacilityItemsLayout out;
    const float width = std::min(224.F, extent.width - 16.F);
    const float height = std::min(240.F, extent.height - 58.F);
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    out.heading = {p.x + 10, p.y + 27, width - 20, 14};
    // PAGES raw75: five rows at 19 pitch; selected background is 191x16.
    // Keep result76/77 geometry separate so this list-only adaptation changes no consumer.
    out.result = {p.x + 10, p.y + 47, width - 20, height - 118};
    const float row_width = std::min(191.F, width - 20);
    out.rows = {p.x + (width - row_width) / 2, p.y + 43, row_width, 5 * 19.F};
    out.row_height = 19;
    out.hints = {p.x + 10, p.y + 140, width - 20, 14};
    out.feedback = {p.x + 10, p.y + height - 43, width - 20, 12};
    out.cancel = {p.x + 10, p.y + height - 26, 48, 20};
    out.confirm = {p.x + width - 58, p.y + height - 26, 48, 20};
    // S045 places the real facility above three new-value/delta rows. Compress vertically
    // at the minimum window without changing counters or manufacturing animation frames.
    const float picture_height = std::max(34.F, std::min(74.F, (out.result.height - 4) * .5F));
    out.result_picture = {p.x + (width - 97) / 2, out.result.y, 97, picture_height};
    out.result_values = {out.result.x, out.result_picture.y + picture_height + 4, out.result.width,
                         out.result.height - picture_height - 4};
    return out;
}
Rectangle world_facility_item_highlight(const WorldFacilityItemsLayout &layout, int visible_row) {
    if (visible_row < 0 || visible_row >= 5)
        throw std::invalid_argument("Facility item highlight requires a visible source row");
    return {layout.rows.x, layout.rows.y + visible_row * 19 - 2, std::min(191.F, layout.rows.width),
            16};
}
std::optional<WorldFacilityItemsIntent> world_facility_items_input(
    const WorldFacilityItemsView &view, const WorldFacilityItemsLayout &layout,
    WorldFacilityItemsSelection &selection, const WorldFacilityItemsInput &input, bool blocked) {
    if (selection.page != view.page)
        selection = {view.page, {}};
    if (input.keyboard_event || input.enter || input.escape || input.up || input.down)
        selection.marked_row.reset();
    if (blocked || !view.initialized || view.raw == 76)
        return {};
    const auto intent = [&](Action action, int selection = -1) {
        return WorldFacilityItemsIntent{action, view.page, selection};
    };
    if (view.can_cancel && (input.escape || hit(input, layout.cancel)))
        return intent(Action::cancel);
    if (view.raw == 75 && !view.rows.empty()) {
        if (input.up || input.down)
            return intent(input.up ? Action::previous : Action::next);
        if (input.wheel_rows)
            return intent(Action::select, std::clamp(view.selection + input.wheel_rows, 0,
                                                     static_cast<int>(view.rows.size()) - 1));
        for (int n = 0; n < 5 && n + view.first_visible < static_cast<int>(view.rows.size()); ++n)
            if (hit(input, world_facility_item_highlight(layout, n))) {
                const int index = n + view.first_visible;
                // cc4fb06 registers a component + absolute row key, with no timer.
                // ADR-0058 requires the selected row to be committed before a second UP uses it.
                const bool confirm = selection.marked_row == index && view.selection == index;
                selection.marked_row = index;
                return confirm ? intent(Action::confirm) : intent(Action::select, index);
            }
    }
    if (view.can_confirm && (input.enter || hit(input, layout.confirm)))
        return intent(Action::confirm);
    return {};
}
void draw_world_facility_items(const WorldFacilityItemsView &view,
                               const WorldFacilityItemsLayout &layout, const Skin &skin,
                               bool enabled, const std::string &feedback) {
    skin.window(layout.panel, view.title);
    const auto content = view.raw == 75 ? layout.rows : layout.result;
    skin.content({content.x - 2, content.y - 3, content.width + 4, content.height + 5});
    if (view.initialized) {
        if (view.raw == 75) {
            fitted(skin, "名称", {layout.rows.x + 29, layout.heading.y, 100, 14});
            skin.right("剩余", layout.rows.x + layout.rows.width, layout.heading.y);
            for (int n = 0; n < 5 && n + view.first_visible < static_cast<int>(view.rows.size());
                 ++n) {
                const int index = n + view.first_visible;
                const auto &row = view.rows[index];
                Rectangle box{layout.rows.x, layout.rows.y + n * layout.row_height,
                              layout.rows.width, layout.row_height};
                if (index == view.selection) {
                    DrawRectangleRec(world_facility_item_highlight(layout, n), {255, 153, 55, 255});
                    skin.sprites.draw("finger_r.seb", 0, {box.x + 2, box.y + 7}, WHITE,
                                      Sprites::Binding::common);
                }
                draw_world_visuals(row.icon, skin.sprites, {box.x + 11, box.y - 1}, 1);
                fitted(skin, row.name, {box.x + 29, box.y, box.width - 61, 16});
                skin.right(std::to_string(row.owned), box.x + box.width - 3, box.y, blue, 10);
            }
            if (view.choice) {
                constexpr const char *labels[]{"价格", "品质", "魅力"};
                for (int n = 0; n < 3; ++n)
                    fitted(skin, std::string(labels[n]) + std::string(view.choice->hint[n], '+'),
                           {layout.hints.x + n * layout.hints.width / 3, layout.hints.y,
                            layout.hints.width / 3, layout.hints.height},
                           blue);
            }
        } else if (view.raw == 76) {
            skin.sprites.thumbnail(view.graphic.sprite, view.graphic.frames,
                                   {layout.result.x + 10, layout.result.y + 5,
                                    layout.result.width - 20, layout.result.height - 25});
            if (view.choice)
                skin.centered(view.choice->name,
                              {layout.result.x, layout.result.y + layout.result.height - 20,
                               layout.result.width, 18});
        } else {
            skin.content(layout.result_picture, {248, 245, 206, 255});
            skin.sprites.thumbnail(view.graphic.sprite, view.graphic.frames, layout.result_picture);
            constexpr const char *labels[]{"价格", "品质", "魅力"};
            for (int n = 0; n < 3; ++n) {
                const auto b = layout.result_values;
                const float y = b.y + n * b.height / 3;
                fitted(skin, labels[n], {b.x + 3, y, 39, 14}, blue);
                skin.right(std::to_string(view.attributes[1][n]) + (n == 0 ? "G" : ""),
                           b.x + b.width * .64F, y, blue, 11);
                if (view.attributes[2][n] != 0)
                    skin.right((view.attributes[2][n] > 0 ? "+" : "") +
                                   std::to_string(view.attributes[2][n]) + (n == 0 ? "G" : ""),
                               b.x + b.width - 3, y, MAROON, 11);
            }
        }
    }
    if (view.initialized)
        fitted(skin,
               view.name + (view.profit ? "  收益 " + std::to_string(*view.profit) + "G" : ""),
               layout.feedback, blue);
    const bool active = enabled && view.initialized;
    if (view.can_cancel)
        skin.button(layout.cancel, "返回", active);
    if (view.raw != 76)
        skin.choice(layout.confirm, view.raw == 75 ? "使用" : "确定", active && view.can_confirm);
    if (!feedback.empty())
        fitted(skin, feedback, layout.feedback, MAROON);
}
} // namespace ark::desktop::ui
