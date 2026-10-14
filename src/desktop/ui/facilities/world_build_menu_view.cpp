#include "world_build_menu.hpp"
#include "../common/skin.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
int integer(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        throw std::invalid_argument("Build menu display exceeds the source integer range");
    return int(value);
}
std::string gold(std::int64_t value) {
    auto text = std::to_string(value);
    const std::size_t sign = value < 0 ? 1 : 0;
    for (std::size_t n = text.size(); n > sign + 3;) {
        n -= 3;
        text.insert(n, ",");
    }
    return text + "Ｇ";
}
} // namespace
bool world_build_menu_page(const sim::rules::WorldScriptPage &page) {
    return page.kind == sim::rules::WorldScriptPageKind::raw_page && page.legacy_page == 21;
}
Vector2 world_build_menu_origin(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Build menu needs the supported logical viewport");
    // Source VIEW0/23/240/217 gives J33/K16; desktop translation applies only once.
    return {float((extent.width - 240) / 2), float((extent.height - 256) / 2)};
}
WorldBuildMenuView world_build_menu_view(const sim::StartupWorldRuntimeState &state,
                                        const sim::rules::WorldScriptPage &page) {
    if (!world_build_menu_page(page) || !state.rules)
        throw std::invalid_argument("Build menu requires source21 and its rule definitions");
    WorldBuildMenuView view;
    view.page = page.id;
    if (page.lifecycle == 0 || page.lifecycle == 4)
        return view;
    const auto cursor = sim::startup_world_build_menu_cursor(state, page.id);
    if (!cursor)
        throw std::invalid_argument("Initialized build menu has an invalid Owner payload");
    auto &in = view.source;
    in.catalogs = state.build_page_catalogs.at(page.id);
    in.cursor = *cursor;
    in.origin = {33, 16};
    in.category_names = {"设备", "一般", "饮食"};
    in.category_new = state.build_category_new;
    in.funds = state.scene.world.world.ai.accounting.funds();
    in.frame2 = state.page_secondary_counters.at(page.id);
    in.finger_frame = 0; // Approved PC static hand; never derive shared SEB phase from a page clock.
    const auto residents = sim::startup_world_residence_candidates(state);
    if (!residents)
        throw std::invalid_argument("Build menu is missing its resident qualification source");
    const auto &catalog = in.catalogs.at(cursor->page);
    for (int n = cursor->scroll; n < int(catalog.size()) && n < cursor->scroll + 5; ++n) {
        sim::SteamBuildMenuRow row;
        row.definition = catalog[n];
        if (row.definition < 0) {
            row.name = row.definition == -1 ? "撤除" : "移动";
            row.quote = row.definition == -1 ? 0 : 300;
        } else {
            const auto d = std::find_if(state.rules->facilities.begin(), state.rules->facilities.end(),
                                         [&](const auto &item) { return item.id == row.definition; });
            const auto quote = sim::startup_world_build_quote(state, row.definition);
            if (d == state.rules->facilities.end() || !quote)
                throw std::invalid_argument("Build menu row is missing its current definition quote");
            row.name = d->name;
            row.mapchip = d->display_id;
            row.kind = d->kind;
            row.quote = integer(quote->construction_cost);
            row.remaining = state.facility_free_builds.at(row.definition);
            row.build_now = state.facility_build_present.at(row.definition);
            row.resident = !residents->empty();
        }
        row.price_text = gold(row.quote);
        in.visible_rows.push_back(std::move(row));
    }
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    view.initialized = true;
    view.interactive = top != state.scripts.pages.rend() && top->id == page.id &&
                       !state.scene.framework_paused && state.page_counters.at(page.id) >= 3;
    return view;
}
void prepare_world_build_menu(WorldBuildMenuView &view, const Skin &skin, bool scrolling) {
    view.skin.reset();
    if (!view.initialized)
        return;
    view.source.scroll_first_touch = scrolling;
    for (auto &row : view.source.visible_rows)
        row.name_width = integer(static_cast<std::int64_t>(skin.text.width(row.name, 10)));
    view.skin = sim::steam_build_menu_skin(view.source);
    if (!view.skin)
        throw std::invalid_argument("Build menu skin rejected its measured source rows");
}
} // namespace ark::desktop::ui
