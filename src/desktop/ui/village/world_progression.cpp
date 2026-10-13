#include "world_progression.hpp"
#include "../common/skin.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include "ark/simulation/village/rules/world_calendar_tasks.hpp"
#include "world_village_skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
bool hit(std::optional<Vector2> p, Rectangle r) {
    return p && p->x >= r.x && p->y >= r.y && p->x < r.x + r.width && p->y < r.y + r.height;
}
std::string human_name(const State &state, int id) {
    const auto found = std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                                    [id](const auto &h) { return h.identity == id; });
    if (found == state.rules->humans.end())
        throw std::invalid_argument("Progression page references an unknown human definition");
    return simulation::startup_world_human_profile(state, id).value().name;
}
} // namespace
bool world_progression_page(const Page &page) {
    if (page.kind != simulation::rules::WorldScriptPageKind::raw_page)
        return false;
    const int raw = page.legacy_page;
    return raw == 48 || raw == 49 || raw == 50 || raw == 67 || raw == 88 || raw == 95 || raw == 96;
}
WorldProgressionView world_progression_view(const State &state, const Page &page) {
    if (!state.rules || !world_progression_page(page))
        throw std::invalid_argument("Progression view requires source data and supported page");
    WorldProgressionView view;
    view.page = page.id;
    view.raw = page.legacy_page;
    view.rank = state.rank;
    view.title = view.raw == 48   ? "晋级申请"
                 : view.raw == 49 ? "城镇等级"
                 : view.raw == 50 ? "晋级庆典"
                 : view.raw == 67 ? "能力上升"
                 : view.raw == 88 ? "勋章授予"
                                  : "自宅完成";
    const auto counter = state.page_counters.find(page.id);
    if (counter == state.page_counters.end())
        return view;
    view.counter = counter->second;
    const auto phase = state.page_phases.find(page.id);
    if (phase != state.page_phases.end())
        view.phase = phase->second;
    if (view.raw == 48 || view.raw == 49 || view.raw == 50 || view.raw == 95) {
        if (page.lifecycle == 0 || page.lifecycle == 4 ||
            !simulation::valid_startup_world_town_progress_page(state, page))
            return view;
        if (view.raw == 48 || view.raw == 49)
            view.selection = view.phase;
        if (view.raw == 95) {
            view.title = "活动";
            view.confirm_enabled = true;
            view.initialized = true;
            return view;
        }
    }
    if (view.raw == 48 || view.raw == 49) {
        if (state.rank >= 5)
            return view; // Owner replaces the page with the source maximum-rank message.
        if (view.raw == 48)
            view.rows.push_back("晋级申请");
        static constexpr const char *labels[]{"月收入",     "设施数",   "居住数",    "指定建设",
                                              "任务完成数", "街道人气", "举办活动数"};
        const auto terms = simulation::rules::fixed_calendar_task_rank_terms().at(state.rank);
        for (std::size_t n = 0; n < terms.size(); ++n)
            view.rows.push_back(std::string(labels[terms[n].type]) + " " +
                                std::to_string(state.rank_values[n]) +
                                (state.rank_met[n] ? " 达成" : " 未达成"));
        view.confirm_enabled = true;
    } else if (view.raw == 50) {
        const auto cast = state.rank_celebration_participants.find(page.id);
        if (cast == state.rank_celebration_participants.end() || phase == state.page_phases.end())
            return view;
        for (const auto &actor : cast->second)
            view.rows.push_back(human_name(state, actor[0]));
        view.confirm_enabled = view.phase == 2 && view.counter >= 140;
    } else {
        const auto binding = state.page_human_bindings.find(page.id);
        if (binding == state.page_human_bindings.end())
            return view;
        view.human = human_name(state, binding->second);
        if (view.raw == 67) {
            static constexpr const char *stats[]{"HP", "攻击", "防御", "魔法"};
            for (std::size_t n = 0; n < 4; ++n)
                view.rows.push_back(std::string(stats[n]) + " " +
                                    std::to_string(state.effort_display[0][n]) + " > " +
                                    std::to_string(state.effort_display[1][n]));
            view.confirm_enabled = view.counter < 67 || view.counter >= 73;
        } else {
            if (phase == state.page_phases.end())
                return view;
            for (std::size_t n = 0; n < 2; ++n)
                view.rows.push_back(std::string(n == 0 ? "满足 " : "努力 ") +
                                    std::to_string(state.reward_display[0][n]) + " > " +
                                    std::to_string(state.reward_display[1][n]));
            view.confirm_enabled = view.raw == 96 || view.phase == 1;
        }
    }
    view.initialized = true;
    return view;
}
WorldProgressionLayout world_progression_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Unsupported progression viewport");
    WorldProgressionLayout out;
    out.extent = extent;
    const bool source = raw == 48 || raw == 49 || raw == 50 || raw == 95;
    const float width =
        source ? (raw == 48 || raw == 49 ? 220.F : 222.F) : std::min(310.F, extent.width - 16.F);
    const float height =
        source ? (raw == 48 || raw == 49 ? 168.F : 170.F) : std::min(250.F, extent.height - 68.F);
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    out.body = {out.panel.x + 12, out.panel.y + 52, width - 24, height - 87};
    out.cancel = {out.panel.x + width - 50, out.panel.y + height + 4, 50, 18};
    out.confirm = {out.panel.x, out.panel.y + height + 4, 50, 18};
    return out;
}
void prepare_world_progression(WorldProgressionView &v, const State &s, const Skin &skin) {
    v.skin.reset();
    if (!v.initialized)
        return;
    if (v.raw != 48 && v.raw != 49 && v.raw != 50 && v.raw != 95)
        return;
    simulation::SteamTownProgressOptions o;
    o.view_y = 23;
    const std::string title = v.raw == 48 || v.raw == 49 ? "街道升级"
                              : v.raw == 50              ? "街道等级上升"
                                                         : "活动";
    o.title_widths = std::array<int, 2>{int(skin.text.width(title)), int(skin.text.width(title))};
    o.condition_suffix_width = int(skin.text.width("的升级条件"));
    const bool met =
        std::all_of(s.rank_met.begin(), s.rank_met.end(), [](bool value) { return value; });
    o.bottom_width = int(skin.text.width(v.raw == 49 ? "了解" : met ? "升级!" : "申请"));
    o.promoted_suffix_width = int(skin.text.width("升级！"));
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [&](const auto &p) { return p.id == v.page; });
    if (page == s.scripts.pages.end())
        throw std::invalid_argument("Town plan lost its page");
    o.reward_number_width = int(skin.text.width(std::to_string(page->legacy_s)));
    o.bubble_widths = {int(skin.text.width("恭喜!")), int(skin.text.width("太好了!")),
                       int(skin.text.width("万岁")), int(skin.text.width("请"))};
    v.skin = v.raw == 48 || v.raw == 49 ? simulation::steam_town_conditions_skin(s, v.page, o)
             : v.raw == 50              ? simulation::steam_town_celebration_skin(s, v.page, o)
                                        : simulation::steam_town_gift_skin(s, v.page, o);
    if (!v.skin)
        throw std::invalid_argument("Invalid initialized town progress plan");
}
std::optional<WorldProgressionIntent>
world_progression_input(const WorldProgressionView &v, const WorldProgressionLayout &l,
                        const WorldProgressionInput &i, bool blocked,
                        const std::optional<WorldFacilityPaintedControls> &painted) {
    if (blocked || !v.initialized)
        return {};
    if (v.raw == 48 || v.raw == 49 || v.raw == 50 || v.raw == 95) {
        const bool fresh = painted && painted->page == v.page && painted->raw == v.raw &&
                           painted->phase == v.phase && painted->selection == v.selection &&
                           painted->extent.width == l.extent.width &&
                           painted->extent.height == l.extent.height;
        simulation::StartupTownRankInput f;
        f.up = v.raw == 48 && i.up;
        f.down = v.raw == 48 && i.down;
        f.confirm = v.confirm_enabled && i.enter;
        f.cancel = v.raw == 48 && i.escape;
        if (fresh) {
            f.confirm |= v.confirm_enabled && hit(i.click, painted->confirm);
            f.cancel |= v.raw == 48 && hit(i.click, painted->cancel);
            if (v.raw == 48)
                for (const auto &row : painted->rows)
                    if (row.index >= 1 && row.index <= 4 && hit(i.click, row.rectangle)) {
                        f.selection = row.index;
                        f.confirm = true;
                    }
        }
        if (!f.up && !f.down && !f.confirm && !f.cancel && !f.selection)
            return {};
        return WorldProgressionIntent{
            v.raw == 48, !f.confirm && f.cancel, f.selection.value_or(v.selection),
            v.raw == 48 ? std::optional<simulation::StartupTownRankInput>{f} : std::nullopt};
    }
    if (v.confirm_enabled && (i.enter || hit(i.click, l.confirm)))
        return WorldProgressionIntent{false, false, 0, {}};
    return {};
}
std::optional<WorldFacilityPaintedControls> draw_world_progression(const WorldProgressionView &v,
                                                                   const WorldProgressionLayout &l,
                                                                   const Skin &skin, bool enabled) {
    if (v.raw == 48 || v.raw == 49 || v.raw == 50 || v.raw == 95) {
        if (!v.initialized) {
            skin.window(l.panel, v.title);
            return {};
        }
        if (!v.skin)
            throw std::invalid_argument("Town rendering requires a measured source plan");
        WorldFacilityPaintedControls c;
        c.page = v.page;
        c.raw = v.raw;
        c.phase = v.phase;
        c.selection = v.selection;
        c.extent = l.extent;
        c = draw_world_town_skin(*v.skin, l.panel, skin, std::move(c));
        if (v.raw == 48) {
            c.cancel = l.cancel;
            skin.choice(l.cancel, "返回", enabled);
        }
        // Original coordinate-free confirmations use the established PC soft key.
        if (v.raw != 48 && c.confirm.width <= 0) {
            c.confirm = l.confirm;
            skin.choice(l.confirm, v.raw == 95 ? (v.counter >= 40 ? "领取" : "继续") : "确定",
                        enabled && v.confirm_enabled);
        }
        return c;
    }
    skin.window(l.panel, v.title);
    skin.content(l.body);
    skin.text.draw(v.human, l.panel.x + 12, l.panel.y + 28);
    for (std::size_t n = 0; n < v.rows.size(); ++n) {
        const float size =
            std::min(12.F, 12.F * (l.body.width - 6) / std::max(1.F, skin.text.width(v.rows[n])));
        skin.text.draw(v.rows[n], l.body.x + 3, l.body.y + n * 22 + 3, ink, size);
    }
    skin.choice(l.confirm, "确定", enabled && v.initialized && v.confirm_enabled);
    return {};
}
} // namespace ark::desktop::ui
