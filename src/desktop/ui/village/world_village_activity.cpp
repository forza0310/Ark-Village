#include "world_village_activity.hpp"
#include "../common/skin.hpp"
#include "ark/presentation/script_text.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "world_village_skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupVillageActivityAction;
constexpr const char *attributes[]{"体力", "力量", "灵活", "结实", "魔力", "运气"};
const simulation::StartupWorldActivity &definition(const State &state, int id) {
    const auto found = std::find_if(state.rules->activities.begin(), state.rules->activities.end(),
                                    [&](const auto &a) { return a.identity == id; });
    if (found == state.rules->activities.end())
        throw std::invalid_argument("Village activity view lost its source definition");
    return *found;
}
} // namespace
bool world_village_activity_page(const Page &page) {
    return page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
           page.legacy_page >= 51 && page.legacy_page <= 54;
}
WorldVillageActivityView world_village_activity_view(const State &state, const Page &page) {
    if (!state.rules || !world_village_activity_page(page))
        throw std::invalid_argument("Village activity view requires a source management page");
    WorldVillageActivityView out;
    out.page = page.id;
    out.raw = page.legacy_page;
    out.title = out.raw == 51   ? "村办活动"
                : out.raw == 52 ? "开展活动"
                : out.raw == 53 ? "活动进行中"
                                : "活动结果";
    const auto source = simulation::inspect_startup_world_village_activity_page(state, page.id);
    if (!source)
        return out;
    // A returned child answer must be consumed by the next real parent update. Reading its
    // old list remains valid, but another click must not bypass that source resume boundary.
    if (out.raw == 51 && state.activity_page_answers.count(page.id))
        return out;
    out.selection = source->selection;
    out.first_visible = source->first_visible;
    out.counter = source->counter;
    out.points = state.village_points;
    out.quarter_slots = state.quarter_counter;
    if (source->activity) {
        const auto &activity = definition(state, *source->activity);
        out.activity = activity.identity;
        out.kind = activity.parameters[2];
        out.attribute = activity.parameters[3];
        out.name = activity.name;
        out.detail = activity.detail;
        out.description = activity.description;
        if (out.raw == 54) {
            const int attribute = activity.parameters[3];
            if (activity.parameters[2] == 0)
                out.result_attribute = "满足";
            else if (activity.parameters[2] == 1 && attribute >= 0 && attribute < 6)
                out.result_attribute = attributes[attribute];
            else
                throw std::invalid_argument("Village result references an unsupported effect");
        }
    }
    for (const int entry : source->entries) {
        WorldVillageActivityRow row;
        row.definition = entry;
        if (out.raw == 51) {
            const auto &activity = definition(state, entry);
            row.name = activity.name;
            row.points = activity.parameters[4];
            row.kind = activity.parameters[2];
            row.supported = (row.kind >= 0 && row.kind <= 3) || row.kind == 5 || row.kind == 6;
            row.fresh = state.scripts.activities.at(entry).pending_notice;
        } else {
            const auto human = simulation::startup_world_human_details(state, entry);
            const auto name = std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                                           [&](const auto &h) { return h.identity == entry; });
            if (!human || name == state.rules->humans.end() || !source->activity)
                throw std::invalid_argument("Village result lost its frozen human reference");
            const auto &activity = definition(state, *source->activity);
            row.name = name->name;
            row.before = state.human_activity_previous.at(entry);
            row.current = activity.parameters[2] == 0
                              ? human->satisfaction
                              : human->attributes.at(activity.parameters[3]);
            row.supported = true;
        }
        out.rows.push_back(std::move(row));
    }
    if (out.raw == 51 || out.raw == 54)
        for (std::size_t n = 0; n < source->display_humans.size(); ++n) {
            const int human = source->display_humans[n];
            if (state.human_presence.at(human) == 0)
                continue;
            const auto portrait = simulation::startup_world_portrait(state, human);
            if (!portrait)
                throw std::invalid_argument("Village display lost its current profession portrait");
            out.portrait_images[n] = portrait->image;
        }
    out.can_cancel = out.raw != 53;
    out.can_confirm = out.raw == 53   ? out.counter >= 120
                      : out.raw == 51 ? !out.rows.empty() && out.rows.at(out.selection).supported
                                      : true;
    if (out.raw == 51 && !out.rows.empty() && !out.rows.at(out.selection).supported)
        out.status = "此活动尚未接入";
    out.initialized = true;
    out.interactive = page.lifecycle != 0 && page.lifecycle != 4;
    return out;
}
WorldVillageActivityLayout world_village_activity_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Unsupported village viewport");
    WorldVillageActivityLayout out;
    out.extent = extent;
    const float width = raw == 51 || raw == 54 ? 220.F : 222.F,
                height = raw == 51 || raw == 54 ? 168.F : 170.F;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto origin = world_list_skin_origin(out.panel);
    out.rows = {origin.x + 23, origin.y + 95, 191, 95};
    out.row_height = 19;
    out.cancel = {out.panel.x + width - 50, out.panel.y + height + 4, 50, 18};
    out.confirm = {out.panel.x, out.panel.y + height + 4, 50, 18};
    out.feedback = {out.panel.x, out.panel.y + height + 24, width, 12};
    return out;
}
void prepare_world_village_activity(WorldVillageActivityView &v, const State &s, const Skin &skin,
                                    bool scrolling) {
    v.skin.reset();
    if (!v.initialized)
        return;
    simulation::SteamVillageActivityOptions o;
    o.view_y = 23;
    o.scroll_first_touch = scrolling;
    const std::string title = v.raw == 51 ? "开展活动" : v.raw == 54 ? "活动结束" : v.name;
    o.title_widths = std::array<int, 2>{int(skin.text.width(title)), int(skin.text.width(title))};
    o.description_width = int(skin.text.width(v.description));
    const bool execute = v.activity >= 0 && (s.activity_flags.at(v.activity) & 8U);
    o.button_widths = {int(skin.text.width(execute ? "实施" : "开展")),
                       int(skin.text.width(execute ? "不实施" : "不开展"))};
    o.increase_suffix_width = int(skin.text.width("上升了!"));
    if (v.raw == 53 && v.kind == 1) {
        constexpr const char *names[]{"体力", "力", "器用", "丈夫", "魔力", "运"};
        if (v.attribute < 0 || v.attribute >= 6)
            throw std::invalid_argument("Invalid activity attribute");
        o.attribute_widths = {int(skin.text.width("冒险者们的")),
                              int(skin.text.width(names[v.attribute])), 0};
    }
    v.skin = simulation::steam_village_activity_skin(s, v.page, o);
    if (!v.skin)
        throw std::invalid_argument("Invalid bound village activity plan");
}
std::optional<WorldVillageActivityIntent>
world_village_activity_input(const WorldVillageActivityView &v, const WorldVillageActivityLayout &l,
                             const WorldVillageActivityInput &i, bool blocked,
                             const std::optional<WorldFacilityPaintedControls> &painted,
                             WorldFacilityGesture *gesture) {
    const bool fresh = painted && painted->page == v.page && painted->raw == v.raw &&
                       painted->selection == v.selection && painted->first == v.first_visible &&
                       painted->count == (v.raw == 52 ? 2 : int(v.rows.size())) &&
                       painted->choice == v.activity && painted->extent.width == l.extent.width &&
                       painted->extent.height == l.extent.height;
    if (gesture && gesture->page != v.page) {
        gesture->page = v.page;
        gesture->marked_row.reset();
        gesture->scrolling = false;
    }
    const auto drag = gesture
                          ? world_facility_scroll_input(
                                *gesture, fresh ? &*painted : nullptr,
                                fresh && !blocked && v.initialized && v.interactive &&
                                    (v.raw == 51 || v.raw == 54),
                                i.pointer, i.pointer_pressed, i.pointer_down, i.pointer_released)
                          : WorldFacilityScrollInput{};
    if (blocked || !v.initialized || !v.interactive)
        return {};
    simulation::StartupVillageActivityInput f;
    f.up = v.raw != 53 && i.up;
    f.down = v.raw != 53 && i.down;
    f.confirm = v.can_confirm && i.enter;
    f.cancel = v.can_cancel && i.escape;
    if ((v.raw == 51 || v.raw == 54) && i.wheel_rows)
        f.scroll = simulation::StartupVillageActivityInput::Scroll{false, i.wheel_rows};
    if (drag.absolute)
        f.scroll = simulation::StartupVillageActivityInput::Scroll{true, *drag.absolute};
    if (fresh && !drag.consume_click) {
        f.confirm |= v.can_confirm && world_facility_hit(i.click, painted->confirm);
        f.cancel |= v.can_cancel && world_facility_hit(i.click, painted->cancel);
        for (const auto &row : painted->rows)
            if (world_facility_hit(i.click, row.rectangle) && row.index >= 0 &&
                row.index < (v.raw == 52 ? 2 : int(v.rows.size())) &&
                row.identity == (v.raw == 52 ? row.index : v.rows[row.index].definition)) {
                f.selection = row.index;
                // Original list/button components have no marker16; one release confirms.
                f.confirm = v.raw != 51 || v.rows[row.index].supported;
            }
    }
    if (!f.up && !f.down && !f.confirm && !f.cancel && !f.selection && !f.scroll)
        return {};
    return WorldVillageActivityIntent{f.confirm     ? Action::confirm
                                      : f.cancel    ? Action::cancel
                                      : f.selection ? Action::select
                                      : f.up        ? Action::previous
                                                    : Action::next,
                                      v.page, f.selection.value_or(0), f};
}
std::optional<WorldFacilityPaintedControls>
draw_world_village_activity(const WorldVillageActivityView &v, const WorldVillageActivityLayout &l,
                            const Skin &skin, bool enabled, const std::string &feedback) {
    if (!v.initialized) {
        skin.window(l.panel, v.title);
        return {};
    }
    if (!v.skin)
        throw std::invalid_argument("Village rendering requires measured source plan");
    WorldFacilityPaintedControls out;
    out.page = v.page;
    out.raw = v.raw;
    out.selection = v.selection;
    out.first = v.first_visible;
    out.count = v.raw == 52 ? 2 : int(v.rows.size());
    out.choice = v.activity;
    out.extent = l.extent;
    std::vector<int> ids;
    if (v.raw == 52)
        ids = {0, 1};
    else
        for (const auto &row : v.rows)
            ids.push_back(row.definition);
    out = draw_world_village_skin(*v.skin, l.panel, skin, std::move(out), ids);
    if (v.can_cancel) {
        out.cancel = l.cancel;
        skin.choice(l.cancel, "返回", enabled);
    }
    if (v.raw != 52) {
        out.confirm = l.confirm;
        skin.choice(l.confirm, v.raw == 54 ? "关闭" : "确定", enabled && v.can_confirm);
    }
    if (!feedback.empty())
        skin.text.draw(feedback, l.feedback.x, l.feedback.y, MAROON, 9);
    return out;
}
} // namespace ark::desktop::ui
