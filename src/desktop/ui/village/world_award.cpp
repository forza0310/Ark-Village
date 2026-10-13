#include "ark/simulation/actors/startup_world_profile.hpp"
// Research 6f93c0c supplies the three-page plan; PC award/termination questions remain separate.
#include "../actors/world_human_detail.hpp"
#include "../common/skin.hpp"
#include "../common/world_list_controls.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "world_award.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using Page = simulation::rules::WorldScriptPage;
using Kind = simulation::rules::WorldScriptPageKind;
bool hit(std::optional<Vector2> point, Rectangle box) {
    return point && point->x >= box.x && point->y >= box.y && point->x < box.x + box.width &&
           point->y < box.y + box.height;
}
} // namespace
bool world_page_automatic(const Page &page) {
    return page.kind == Kind::raw_page &&
           (page.legacy_page == 16 || page.legacy_page == 56 || page.legacy_page == 57 ||
            page.legacy_page == 97 || page.legacy_page == 98);
}
bool world_page_regular_confirmation(const Page &page) {
    if (world_page_automatic(page))
        return false;
    if (page.kind != Kind::raw_page)
        return true;
    const int raw = page.legacy_page;
    return raw != 87 && raw != 83 && raw != 33 && raw != 4 && raw != 21 && raw != 48 && raw != 74 &&
           raw != 80 && !(raw >= 22 && raw <= 28) && !(raw >= 51 && raw <= 54) &&
           !(raw >= 60 && raw <= 66) && raw != 68 && raw != 70 && raw != 73 && raw != 90;
}
WorldAwardView world_award_view(const simulation::StartupWorldRuntimeState &state,
                                std::uint64_t page) {
    WorldAwardView view;
    const auto ranks = state.award_rankings.find(page);
    // A raw page may be published just before its first update. Do not initialize it in draw.
    if (ranks == state.award_rankings.end())
        return view;
    if (!state.rules || !state.award_termination_pending.count(page))
        throw std::invalid_argument("Annual display requires initialized owner data");
    view.initialized = true;
    view.medals = state.medal_count;
    view.frame = state.page_counters.at(page);
    view.termination_pending = state.award_termination_pending.at(page);
    const auto pending = state.award_pending_humans.find(page);
    if (pending != state.award_pending_humans.end())
        view.pending_human = pending->second;
    for (const int id : ranks->second) {
        const auto human = std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                                        [id](const auto &value) { return value.identity == id; });
        if (human == state.rules->humans.end())
            throw std::invalid_argument("Annual display references an unknown human definition");
        const auto portrait = simulation::startup_world_portrait(state, id);
        if (!portrait)
            throw std::invalid_argument("Annual display references a missing current portrait");
        const auto profile = simulation::startup_world_human_profile(state, id).value();
        view.rows.push_back(
            {id, simulation::startup_world_human_profile(state, human->identity).value().name,
             state.human_calendar.at(id).contribution,
             state.scene.world.world.ai.growth.at(id).definition.legacy_u, portrait->image,
             state.scene.world.world.ai.growth.at(id).definition.current_profession, profile.sex,
             state.scene.world.world.ai.battle.humans.at(id).killed_stat1,
             state.scene.world.world.human_spending.at(id), state.human_homes.at(id)[2],
             state.human_calendar.at(id).celebrations});
        if (view.pending_human == id)
            view.pending_name =
                simulation::startup_world_human_profile(state, human->identity).value().name;
    }
    return view;
}
WorldAwardLayout world_award_layout(Extent extent, bool pending) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Annual page requires the supported logical viewport");
    const float width = pending ? std::min(240.F, extent.width - 16.F) : 220.F;
    const float height = pending ? 120.F : 168.F;
    WorldAwardLayout layout;
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &p = layout.panel;
    layout.rows = {p.x + 3, p.y + 48, width - 6, 95};
    layout.row_height = 19;
    layout.terminate = {p.x, p.y + height + 4, 50, 18};
    layout.grant = {p.x + width - 50, p.y + height + 4, 50, 18};
    if (pending) {
        layout.terminate = {p.x + 12, p.y + height - 29, 78, 21};
        layout.grant = {p.x + width - 78, p.y + height - 29, 66, 21};
    }
    layout.previous = {p.x + 4, p.y, 30, 20};
    layout.next = {p.x + width - 34, p.y, 30, 20};
    layout.prompt = {p.x + 12, p.y + 30, width - 24, height - 66};
    layout.yes = {p.x + width / 2 - 66, p.y + height - 29, 58, 21};
    layout.no = {p.x + width / 2 + 8, p.y + height - 29, 58, 21};
    return layout;
}
std::optional<WorldAwardIntent> world_award_input(const WorldAwardView &view,
                                                  const WorldAwardLayout &layout,
                                                  WorldAwardSelection &selection,
                                                  const WorldAwardInput &input, bool blocked) {
    using Action = simulation::rules::WorldAwardAction;
    if (blocked || !view.initialized)
        return {};
    if (view.termination_pending || view.pending_human) {
        if (input.left || input.right)
            selection.prompt = 1 - selection.prompt;
        if (hit(input.click, layout.yes))
            selection.prompt = 0;
        if (hit(input.click, layout.no) || input.escape)
            selection.prompt = 1;
        if (input.enter || input.escape || hit(input.click, layout.yes) ||
            hit(input.click, layout.no)) {
            const auto action =
                view.termination_pending
                    ? (selection.prompt ? Action::reject_termination : Action::confirm_termination)
                    : (selection.prompt ? Action::reject_award : Action::confirm_award);
            return WorldAwardIntent{action, selection.selected};
        }
        return {};
    }
    const int count = static_cast<int>(view.rows.size());
    constexpr int visible = 5;
    if (count) {
        selection.selected = std::clamp(selection.selected, 0, count - 1);
        selection.first_row =
            std::clamp(selection.first_row + input.wheel_rows, 0, std::max(0, count - visible));
        if (input.wheel_rows)
            selection.selected = std::clamp(selection.selected, selection.first_row,
                                            std::min(count - 1, selection.first_row + visible - 1));
        const auto moved = simulation::steam_human_award_cursor(
            {selection.selected, selection.first_row, selection.tab}, count, input.up, input.down,
            input.left || hit(input.click, layout.previous),
            input.right || hit(input.click, layout.next));
        if (!moved)
            throw std::invalid_argument("Invalid PC annual cursor");
        selection.selected = moved->selection;
        selection.first_row = moved->first;
        selection.tab = moved->tab;
        for (int n = 0; n < visible && n + selection.first_row < count; ++n)
            if (hit(input.click,
                    {layout.rows.x, layout.rows.y + n * layout.row_height, layout.rows.width, 16}))
                selection.selected = selection.first_row + n;
    }
    if (input.escape || hit(input.click, layout.terminate)) {
        selection.prompt = 1; // Source termination question defaults to No.
        return WorldAwardIntent{Action::request_termination, selection.selected};
    }
    if (count && view.medals > 0 && (input.enter || hit(input.click, layout.grant))) {
        selection.prompt = 0; // The award question defaults to Yes.
        return WorldAwardIntent{Action::request_award, selection.selected};
    }
    return {};
}
void draw_world_award(const WorldAwardView &view, const WorldAwardLayout &layout, const Skin &skin,
                      const WorldAwardSelection &selection, bool enabled) {
    if (view.termination_pending || view.pending_human) {
        // S068 is a separate information question, not another row in the candidate list.
        skin.window(layout.panel, "信息");
        const auto question = view.termination_pending ? std::string("要中止授勋仪式吗")
                                                       : "授予" + view.pending_name + "勋章？";
        skin.content(layout.prompt);
        skin.centered(question, layout.prompt, ink,
                      std::min(12.F, 12.F * (layout.prompt.width - 8) /
                                         std::max(1.F, skin.text.width(question))));
        const auto selected = selection.prompt ? layout.no : layout.yes;
        DrawRectangleRec(selected, {255, 155, 48, 255});
        const auto color = enabled && view.initialized ? ink : GRAY;
        skin.centered("是", layout.yes, color);
        skin.centered("否", layout.no, color);
        return;
    }
    const auto plan = prepare_world_award(view, selection, skin);
    if (plan)
        draw_world_human_skin(*plan, world_list_skin_origin(layout.panel), skin);
    else
        skin.window(layout.panel, "勋章授予式");
    skin.choice(layout.terminate, "中止", enabled && view.initialized);
    skin.choice(layout.grant, "授予",
                enabled && view.initialized && view.medals > 0 && !view.rows.empty());
}
std::optional<simulation::SteamHumanOverviewPlan>
prepare_world_award(const WorldAwardView &v, const WorldAwardSelection &s, const Skin &skin) {
    if (!v.initialized || v.termination_pending || v.pending_human)
        return {};
    simulation::SteamHumanAwardInput in;
    in.cursor = {s.selected, s.first_row, s.tab};
    in.frame = v.frame;
    in.medals_left = v.medals;
    for (const auto &h : v.rows) {
        in.rank_order.push_back(h.definition);
        in.humans.push_back({h.definition, h.profession, h.sex, h.contribution, h.effort,
                             h.year_acquired, h.year_spent, h.residence, h.medals, h.name});
    }
    const auto title = "勋章授予式 " + std::to_string(s.tab + 1) + "/3";
    const int width = int(skin.text.width(title));
    in.options.view_y = 23;
    in.options.title_widths = std::array<int, 2>{width, width};
    in.options.footer_widths = std::array<int, 2>{
        int(skin.text.width(std::to_string(v.medals) + "个")), int(skin.text.width("还有"))};
    const auto plan = simulation::steam_human_award_skin(in);
    if (!plan && !v.rows.empty())
        throw std::invalid_argument("Invalid bound annual roster plan");
    return plan;
}
} // namespace ark::desktop::ui
