#include "ark/simulation/startup_world_profile.hpp"
// Frozen research e8f66d9 publishes raw87 ranking, award selection and separate questions.
// S067/S068 supply the five-row composition and question text; values remain fixed-APK data.
#include "ark/simulation/startup_world_visuals.hpp"
#include "skin.hpp"
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
        view.rows.push_back(
            {id, simulation::startup_world_human_profile(state, human->identity).value().name,
             state.human_calendar.at(id).contribution,
             state.scene.world.world.ai.growth.at(id).definition.legacy_u, portrait->image});
        if (view.pending_human == id)
            view.pending_name =
                simulation::startup_world_human_profile(state, human->identity).value().name;
    }
    return view;
}
WorldAwardLayout world_award_layout(Extent extent, bool pending) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Annual page requires the supported logical viewport");
    const float width = std::min(240.F, extent.width - 16.F);
    const float height = pending ? 120.F : 188.F;
    WorldAwardLayout layout;
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &p = layout.panel;
    layout.rows = {p.x + 12, p.y + 47, width - 24, height - 101};
    layout.row_height = layout.rows.height / 5;
    layout.terminate = {p.x + 12, p.y + height - 29, 78, 21};
    layout.grant = {p.x + width - 78, p.y + height - 29, 66, 21};
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
        if (input.up)
            selection.selected = (selection.selected + count - 1) % count;
        if (input.down)
            selection.selected = (selection.selected + 1) % count;
        if (input.up || input.down)
            selection.first_row =
                std::clamp(selection.first_row, std::max(0, selection.selected - visible + 1),
                           std::min(selection.selected, std::max(0, count - visible)));
        for (int n = 0; n < visible && n + selection.first_row < count; ++n)
            if (hit(input.click, {layout.rows.x, layout.rows.y + n * layout.row_height,
                                  layout.rows.width, layout.row_height}))
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
    skin.window(layout.panel, "授勋仪式");
    skin.centered(
        "勋章 " + std::to_string(view.medals) + "个 剩余",
        {layout.panel.x, layout.panel.y + layout.panel.height - 48, layout.panel.width, 16});
    skin.text.draw("名称", layout.rows.x + 4, layout.panel.y + 29);
    skin.right("贡献", layout.rows.x + layout.rows.width - 47, layout.panel.y + 29, ink, 10);
    skin.right("勤奋度", layout.rows.x + layout.rows.width - 2, layout.panel.y + 29, ink, 10);
    skin.content(
        {layout.rows.x - 5, layout.rows.y - 4, layout.rows.width + 10, layout.rows.height + 8});
    constexpr int count = 5;
    const int first =
        std::clamp(selection.first_row, 0, std::max(0, static_cast<int>(view.rows.size()) - count));
    for (int index = first; index < static_cast<int>(view.rows.size()) && index < first + count;
         ++index) {
        const auto &row = view.rows[index];
        const float y = layout.rows.y + (index - first) * layout.row_height;
        if (index == selection.selected) {
            DrawRectangleRec({layout.rows.x, y, layout.rows.width, layout.row_height},
                             Color{255, 155, 48, 255});
            skin.sprites.draw("finger_r.seb", 0, {layout.rows.x - 4, y + layout.row_height / 2},
                              WHITE, Sprites::Binding::common);
        }
        const float portrait_size = std::min(15.F, layout.row_height - 2);
        skin.sprites.human_image(row.portrait_image, {1, 27, 15, 14},
                                 {layout.rows.x + 2, y + 1, portrait_size, portrait_size});
        // Scale long names only; ranking and numeric contribution retain their source identity.
        const auto &name = row.name;
        const float size =
            std::min({11.F, layout.row_height - 2,
                      12.F * (layout.rows.width - 106) / std::max(1.F, skin.text.width(name))});
        skin.text.draw(name, layout.rows.x + 20, y + 1, ink, size);
        skin.right(std::to_string(row.contribution), layout.rows.x + layout.rows.width - 47, y + 1,
                   blue, std::min(11.F, layout.row_height - 2));
        skin.right(std::to_string(row.effort), layout.rows.x + layout.rows.width - 2, y + 1, blue,
                   std::min(11.F, layout.row_height - 2));
    }
    if (!view.termination_pending && !view.pending_human &&
        static_cast<int>(view.rows.size()) > count) {
        const Rectangle track{layout.rows.x + layout.rows.width + 4, layout.rows.y, 4,
                              layout.rows.height};
        DrawRectangleRec(track, {33, 20, 91, 255});
        const float thumb = track.height * count / view.rows.size();
        DrawRectangleRec({track.x,
                          track.y + (track.height - thumb) * first / (view.rows.size() - count),
                          track.width, thumb},
                         {47, 165, 237, 255});
    }
    skin.button(layout.terminate, "中止", enabled && view.initialized);
    skin.button(layout.grant, "授予",
                enabled && view.initialized && view.medals > 0 && !view.rows.empty());
}
} // namespace ark::desktop::ui
