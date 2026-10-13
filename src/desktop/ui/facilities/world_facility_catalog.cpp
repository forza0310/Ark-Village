#include "world_facility_catalog.hpp"
#include "../actors/world_human_detail.hpp"
#include "../common/skin.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using Action = simulation::StartupFacilityCatalogAction;
WorldFacilityCatalogRow equipment(const simulation::StartupWorldRuntimeState &state, int kind,
                                  int id) {
    const auto found = std::find_if(
        state.rules->equipment.begin(), state.rules->equipment.end(),
        [=](const auto &entry) { return entry.shop.kind == kind && entry.shop.id == id; });
    if (found == state.rules->equipment.end())
        throw std::invalid_argument("Facility catalogue lost equipment definition");
    return {id,
            found->shop.price,
            found->name,
            found->shop.combat,
            state.catalog.at({kind, id}).newly_unlocked,
            kind == 1   ? 12
            : kind == 2 ? 20
                        : 21,
            kind == 1 ? found->shop.type : found->render_image};
}
} // namespace
bool world_facility_catalog_page(const simulation::rules::WorldScriptPage &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 79 || page.legacy_page == 72 || page.legacy_page == 82);
}
WorldFacilityCatalogView
world_facility_catalog_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page) {
    if (!state.rules || !world_facility_catalog_page(page))
        throw std::invalid_argument("Unsupported facility catalogue view");
    WorldFacilityCatalogView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = out.raw == 79 ? "商品" : out.raw == 72 ? "装备信息" : "传说中的商店";
    // Facility Init publishes its payload independently of the script lifecycle transition.
    if (!state.facility_catalog_pages_initialized.count(page.id))
        return out;
    const auto source = simulation::inspect_startup_world_facility_catalog_page(state, page.id);
    if (!source || source->raw != out.raw)
        throw std::invalid_argument("Invalid initialized facility catalogue payload");
    out.phase = source->phase;
    out.counter = source->counter;
    out.selection = source->selection;
    out.first_visible = source->first_visible;
    if (out.raw == 82) {
        const auto facility =
            std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                         [&](const auto &entry) { return entry.id == source->binding; });
        if (facility == state.rules->facilities.end())
            throw std::invalid_argument("Facility publicity lost definition binding");
        out.title = facility->name;
        for (int id : source->entries) {
            const auto human =
                std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                             [id](const auto &entry) { return entry.identity == id; });
            if (human == state.rules->humans.end())
                throw std::invalid_argument("Facility publicity lost shared human definition");
            out.participants.push_back(
                simulation::startup_world_human_profile(state, human->identity).value().name);
        }
    } else {
        const int kind = out.raw == 79 ? (source->mode == 1   ? 1
                                          : source->mode == 4 ? 2
                                                              : 3)
                                       : (source->mode == 0   ? 1
                                          : source->mode == 3 ? 3
                                                              : 2);
        if (out.raw == 79) {
            for (int id : source->entries)
                out.rows.push_back(equipment(state, kind, id));
            out.choice = out.rows.at(out.selection);
        } else {
            out.choice = equipment(state, kind, source->binding);
        }
    }
    out.initialized = true;
    return out;
}
WorldFacilityCatalogLayout world_facility_catalog_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Unsupported facility viewport");
    WorldFacilityCatalogLayout out;
    out.extent = extent;
    const float width = raw == 72   ? 170.F
                        : raw == 82 ? 222.F
                                    : 220.F,
                height = raw == 72   ? 125.F
                         : raw == 82 ? 170.F
                                     : 168.F;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto origin = world_facility_skin_origin(out.panel);
    out.rows = {origin.x + 23, origin.y + 96, 191, 96};
    out.row_height = 24;
    out.status = {out.panel.x, out.panel.y + height + 24, width, 12};
    out.cancel = {out.panel.x + width - 50, out.panel.y + height + 4, 50, 18};
    out.confirm = raw == 82 ? out.cancel : Rectangle{};
    if (raw == 82)
        out.cancel = {};
    out.inspect = {out.panel.x, out.panel.y + height + 4, 50, 18};
    return out;
}
void prepare_world_facility_catalog(WorldFacilityCatalogView &v,
                                    const simulation::StartupWorldRuntimeState &s, const Skin &skin,
                                    bool scrolling) {
    v.skin.reset();
    v.reputation_skin.reset();
    if (!v.initialized)
        return;
    if (v.raw == 82) {
        simulation::SteamFacilityReputationOptions o;
        o.view_y = 23;
        o.title_widths = std::array<int, 2>{int(skin.text.width("传说中的商店")),
                                            int(skin.text.width("传说中的商店"))};
        o.increase_suffix_width = int(skin.text.width("上升!"));
        v.reputation_skin = simulation::steam_facility_reputation_skin(s, v.page, o);
        if (!v.reputation_skin)
            throw std::invalid_argument("Invalid bound reputation plan");
    } else {
        const auto source = simulation::inspect_startup_world_facility_catalog_page(s, v.page);
        if (!source)
            throw std::invalid_argument("Missing commodity binding");
        simulation::SteamHumanOverviewOptions o;
        o.view_y = 23;
        o.catalogue_scroll_first_touch = scrolling;
        const auto title = v.raw == 79 ? "商品 " + std::to_string(v.phase + 1) + "/2"
                                       : "装备信息 " + std::to_string(v.phase + 1) + "/" +
                                             std::to_string(source->entries.size());
        o.title_widths =
            std::array<int, 2>{int(skin.text.width(title)), int(skin.text.width(title))};
        const auto bottom = std::to_string(v.rows.size()) + "种" +
                            (source->mode == 1   ? "武器"
                             : source->mode == 4 ? "防具"
                                                 : "饰品") +
                            "出售中";
        o.bottom_widths[0] = int(skin.text.width(bottom));
        v.skin = v.raw == 79 ? simulation::steam_facility_commodity_skin(s, v.page, o)
                             : simulation::steam_facility_commodity_detail_skin(s, v.page, o);
        if (!v.skin)
            throw std::invalid_argument("Invalid bound commodity plan");
    }
}
std::optional<WorldFacilityCatalogIntent>
world_facility_catalog_input(const WorldFacilityCatalogView &v, const WorldFacilityCatalogLayout &,
                             const WorldFacilityCatalogInput &i, bool blocked,
                             const std::optional<WorldFacilityPaintedControls> &painted,
                             Extent extent, WorldFacilityGesture *g) {
    const bool fresh = painted && painted->page == v.page && painted->raw == v.raw &&
                       painted->phase == v.phase && painted->selection == v.selection &&
                       painted->first == v.first_visible && painted->count == int(v.rows.size()) &&
                       painted->choice == (v.choice ? v.choice->identity : -1) &&
                       painted->extent.width == extent.width &&
                       painted->extent.height == extent.height;
    if (g && g->page != v.page) {
        g->page = v.page;
        g->marked_row.reset();
        g->scrolling = false;
    }
    const auto drag =
        g ? world_facility_scroll_input(
                *g, fresh ? &*painted : nullptr, fresh && !blocked && v.initialized && v.raw == 79,
                i.pointer, i.pointer_pressed, i.pointer_down, i.pointer_released)
          : WorldFacilityScrollInput{};
    if (blocked || !v.initialized)
        return {};
    simulation::StartupFacilityCatalogInput f;
    f.up = v.raw == 79 && i.up;
    f.down = v.raw == 79 && i.down;
    f.left = v.raw != 82 && i.left;
    f.right = v.raw != 82 && i.right;
    f.confirm = i.enter;
    f.cancel = v.raw != 82 && i.escape;
    f.inspect = v.raw == 79 && i.inspect;
    if (v.raw == 79 && i.wheel_rows)
        f.scroll = simulation::StartupFacilityCatalogInput::Scroll{false, i.wheel_rows};
    if (drag.absolute)
        f.scroll = simulation::StartupFacilityCatalogInput::Scroll{true, *drag.absolute};
    if (fresh && !drag.consume_click) {
        f.cancel |= world_facility_hit(i.click, painted->cancel);
        f.confirm |= world_facility_hit(i.click, painted->confirm);
        f.inspect |= v.raw == 79 && world_facility_hit(i.click, painted->inspect);
        f.left |= world_facility_hit(i.click, painted->previous);
        f.right |= world_facility_hit(i.click, painted->next);
        for (const auto &row : painted->rows)
            if (row.index >= 0 && row.index < int(v.rows.size()) &&
                row.identity == v.rows[row.index].identity &&
                world_facility_hit(i.click, row.rectangle)) {
                f.selection = row.index;
                f.confirm = true; // Source79 confirms/clears NEW; it never purchases.
            }
    }
    if (!f.up && !f.down && !f.left && !f.right && !f.confirm && !f.cancel && !f.inspect &&
        !f.selection && !f.scroll)
        return {};
    return WorldFacilityCatalogIntent{f.confirm        ? Action::confirm
                                      : f.cancel       ? Action::cancel
                                      : f.inspect      ? Action::inspect
                                      : f.selection    ? Action::select
                                      : f.up || f.left ? Action::previous
                                                       : Action::next,
                                      f.selection.value_or(0), f};
}
std::optional<WorldFacilityPaintedControls>
draw_world_facility_catalog(const WorldFacilityCatalogView &v, const WorldFacilityCatalogLayout &l,
                            const Skin &skin, bool enabled, const std::string &feedback) {
    if (!v.initialized) {
        skin.window(l.panel, v.title);
        return {};
    }
    WorldFacilityPaintedControls c;
    c.page = v.page;
    c.raw = v.raw;
    c.phase = v.phase;
    c.selection = v.selection;
    c.first = v.first_visible;
    c.count = int(v.rows.size());
    c.choice = v.choice ? v.choice->identity : -1;
    c.extent = l.extent;
    c.cancel = l.cancel;
    c.confirm = l.confirm;
    c.inspect = v.raw == 79 ? l.inspect : Rectangle{};
    const auto origin = world_facility_skin_origin(l.panel);
    if (v.raw == 82) {
        if (!v.reputation_skin)
            throw std::invalid_argument("Reputation requires measured source plan");
        draw_world_facility_reputation(*v.reputation_skin, origin, skin);
        skin.choice(l.confirm, "确定", enabled);
    } else {
        if (!v.skin)
            throw std::invalid_argument("Commodity requires measured source plan");
        draw_world_human_skin(*v.skin, origin, skin);
        std::vector<int> ids;
        for (const auto &row : v.rows)
            ids.push_back(row.identity);
        c = world_facility_controls(*v.skin, origin, l.panel, skin, std::move(c), ids);
        skin.choice(l.cancel, "返回", enabled);
        if (v.raw == 79)
            skin.choice(l.inspect, "情报", enabled);
    }
    if (!feedback.empty())
        skin.text.draw(feedback, l.status.x, l.status.y, MAROON, 9);
    return c;
}
} // namespace ark::desktop::ui
