#include "world_facility_items.hpp"
#include "world_facility_skin.hpp"
#include "../../scene/world_overlay_render.hpp"
#include "../common/skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupFacilityItemAction;
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
        out.first_visible = state.facility_item_page_scroll.at(page.id);
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
WorldFacilityItemsLayout world_facility_items_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Unsupported facility viewport");
    WorldFacilityItemsLayout out;
    out.extent = extent;
    const float width = raw == 75 ? 220.F : 222.F, height = raw == 75 ? 168.F : 170.F;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto origin = world_facility_skin_origin(out.panel);
    out.heading = {origin.x + 30, origin.y + 69, 195, 14};
    out.rows = {origin.x + 23, origin.y + 97, 191, 95};
    out.row_height = 19;
    out.hints = {origin.x + 30, origin.y + 201, 192, 12};
    out.cancel = {out.panel.x + width - 50, out.panel.y + height + 4, 50, 18};
    out.confirm = {out.panel.x, out.panel.y + height + 4, 50, 18};
    out.feedback = {out.panel.x, out.panel.y + height + 24, width, 12};
    return out;
}
Rectangle world_facility_item_highlight(const WorldFacilityItemsLayout &l, int row) {
    if (row < 0 || row >= 5)
        throw std::invalid_argument("Facility item highlight requires a visible row");
    return {l.rows.x, l.rows.y + row * 19 - 2, 191, 16};
}
void prepare_world_facility_items(WorldFacilityItemsView &v, const State &s, const Skin &skin,
                                  bool scrolling) {
    v.skin.reset();
    if (!v.initialized)
        return;
    simulation::SteamFacilityEffectOptions o;
    o.view_y = 23;
    o.scroll_first_touch = scrolling;
    o.title_widths =
        std::array<int, 2>{int(skin.text.width(v.title)), int(skin.text.width(v.title))};
    v.skin = simulation::steam_facility_effect_skin(s, v.page, o);
    if (!v.skin)
        throw std::invalid_argument("Invalid bound facility improvement plan");
}
std::optional<WorldFacilityItemsIntent>
world_facility_items_input(const WorldFacilityItemsView &v, const WorldFacilityItemsLayout &,
                           WorldFacilityItemsSelection &g, const WorldFacilityItemsInput &i,
                           bool blocked, const std::optional<WorldFacilityPaintedControls> &painted,
                           Extent extent) {
    if (g.page != v.page) {
        g.page = v.page;
        g.marked_row.reset();
        g.scrolling = false;
    }
    if (i.keyboard_event || i.enter || i.escape || i.up || i.down || i.wheel_rows)
        g.marked_row.reset();
    const bool fresh = painted && painted->page == v.page && painted->raw == v.raw &&
                       painted->selection == v.selection && painted->first == v.first_visible &&
                       painted->count == int(v.rows.size()) &&
                       painted->choice == (v.choice ? v.choice->identity : -1) &&
                       painted->extent.width == extent.width &&
                       painted->extent.height == extent.height;
    const auto drag = world_facility_scroll_input(
        g, fresh ? &*painted : nullptr, fresh && !blocked && v.initialized && v.raw == 75,
        i.pointer, i.pointer_pressed, i.pointer_down, i.pointer_released);
    if (blocked || !v.initialized || v.raw == 76)
        return {};
    simulation::StartupFacilityItemInput f;
    f.up = v.raw == 75 && i.up;
    f.down = v.raw == 75 && i.down;
    f.cancel = v.can_cancel && i.escape;
    f.confirm = v.can_confirm && i.enter;
    if (v.raw == 75 && i.wheel_rows)
        f.scroll = simulation::StartupFacilityItemInput::Scroll{false, i.wheel_rows};
    if (drag.absolute)
        f.scroll = simulation::StartupFacilityItemInput::Scroll{true, *drag.absolute};
    if (fresh && !drag.consume_click) {
        f.cancel |= v.can_cancel && world_facility_hit(i.click, painted->cancel);
        f.confirm |= v.can_confirm && world_facility_hit(i.click, painted->confirm);
        for (const auto &row : painted->rows)
            if (row.index >= 0 && row.index < int(v.rows.size()) &&
                row.identity == v.rows[row.index].identity &&
                world_facility_hit(i.click, row.rectangle)) {
                f.selection = row.index;
                f.confirm = g.marked_row == row.index &&
                            v.selection == row.index; // ADR-0058: second committed-row click only.
                g.marked_row = row.index;
            }
    }
    if (!f.up && !f.down && !f.confirm && !f.cancel && !f.selection && !f.scroll)
        return {};
    return WorldFacilityItemsIntent{f.confirm     ? Action::confirm
                                    : f.cancel    ? Action::cancel
                                    : f.selection ? Action::select
                                    : f.up        ? Action::previous
                                                  : Action::next,
                                    v.page, f.selection.value_or(-1), f};
}
std::optional<WorldFacilityPaintedControls>
draw_world_facility_items(const WorldFacilityItemsView &v, const WorldFacilityItemsLayout &l,
                          const Skin &skin, bool enabled, const std::string &feedback) {
    if (!v.initialized) {
        skin.window(l.panel, v.title);
        return {};
    }
    if (!v.skin)
        throw std::invalid_argument("Improvement page requires measured source plan");
    const auto origin = world_facility_skin_origin(l.panel);
    draw_world_facility_effect(*v.skin, origin, skin);
    WorldFacilityPaintedControls c;
    c.page = v.page;
    c.raw = v.raw;
    c.selection = v.selection;
    c.first = v.first_visible;
    c.count = int(v.rows.size());
    c.choice = v.choice ? v.choice->identity : -1;
    c.extent = l.extent;
    c.cancel = v.can_cancel ? l.cancel : Rectangle{};
    c.confirm = v.raw != 76 ? l.confirm : Rectangle{};
    std::vector<int> ids;
    for (const auto &row : v.rows)
        ids.push_back(row.identity);
    c = world_facility_controls(*v.skin, origin, l.panel, std::move(c), ids);
    if (v.can_cancel)
        skin.choice(l.cancel, "返回", enabled);
    if (v.raw != 76)
        skin.choice(l.confirm, v.raw == 75 ? "使用" : "确定", enabled && v.can_confirm);
    if (!feedback.empty())
        skin.text.draw(feedback, l.feedback.x, l.feedback.y, MAROON, 9);
    return c;
}
} // namespace ark::desktop::ui
