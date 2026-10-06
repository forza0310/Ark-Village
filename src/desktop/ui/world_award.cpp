// Frozen research e8f66d9 publishes raw87 ranking, award selection and separate questions.
// This desktop table uses source ranks and contributions with the existing original artwork.
#include "world_award.hpp"
#include "skin.hpp"
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
        view.rows.push_back({id, human->name, state.human_calendar.at(id).contribution});
        if (view.pending_human == id)
            view.pending_name = human->name;
    }
    return view;
}
WorldAwardLayout world_award_layout(Extent extent, bool pending) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Annual page requires the supported logical viewport");
    const float width = std::min(310.F, extent.width - 16.F);
    const float height = std::min(250.F, extent.height - 68.F);
    WorldAwardLayout layout;
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &p = layout.panel;
    layout.rows = {p.x + 12, p.y + 65, width - 24, height - 65 - (pending ? 85.F : 42.F)};
    layout.terminate = {p.x + 12, p.y + height - 29, 78, 21};
    layout.grant = {p.x + width - 78, p.y + height - 29, 66, 21};
    layout.prompt = {p.x + 8, p.y + height - 76, width - 16, 38};
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
    const int visible = std::max(1, static_cast<int>(layout.rows.height / 18));
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
            if (hit(input.click, {layout.rows.x, layout.rows.y + n * 18, layout.rows.width, 18}))
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
    skin.window(layout.panel, "年度授勋");
    skin.text.draw("持有勋章", layout.panel.x + 12, layout.panel.y + 27);
    skin.right(std::to_string(view.medals), layout.panel.x + layout.panel.width - 12,
               layout.panel.y + 27, blue);
    skin.text.draw("冒险者", layout.rows.x, layout.panel.y + 47);
    skin.right("贡献", layout.rows.x + layout.rows.width, layout.panel.y + 47);
    skin.content(
        {layout.rows.x - 5, layout.rows.y - 4, layout.rows.width + 10, layout.rows.height + 8});
    const int count = std::max(1, static_cast<int>(layout.rows.height / 18));
    const int first =
        std::clamp(selection.first_row, 0, std::max(0, static_cast<int>(view.rows.size()) - count));
    for (int index = first; index < static_cast<int>(view.rows.size()) && index < first + count;
         ++index) {
        const auto &row = view.rows[index];
        const float y = layout.rows.y + (index - first) * 18;
        if (index == selection.selected)
            DrawRectangleRec({layout.rows.x, y, layout.rows.width, 18}, Color{219, 232, 204, 255});
        // Scale long names only; ranking and numeric contribution retain their source identity.
        const auto name = std::to_string(index + 1) + ". " + row.name;
        const float size =
            std::min(12.F, 12.F * (layout.rows.width - 40) / std::max(1.F, skin.text.width(name)));
        skin.text.draw(name, layout.rows.x, y, ink, size);
        skin.right(std::to_string(row.contribution), layout.rows.x + layout.rows.width, y, blue);
    }
    if (!view.termination_pending && !view.pending_human &&
        static_cast<int>(view.rows.size()) > count)
        skin.text.draw(
            std::to_string(first + 1) + "-" +
                std::to_string(std::min(first + count, static_cast<int>(view.rows.size()))) + "/" +
                std::to_string(view.rows.size()),
            layout.rows.x, layout.panel.y + layout.panel.height - 24, ink, 9);
    if (view.termination_pending || view.pending_human) {
        skin.content(layout.prompt);
        const auto question = view.termination_pending ? std::string("结束本次授勋吗？")
                                                       : "授予" + view.pending_name + "勋章？";
        skin.text.paragraph(question, layout.prompt.x + 4, layout.prompt.y + 4,
                            layout.prompt.width - 8);
        skin.button(layout.yes, "是", enabled && view.initialized);
        skin.button(layout.no, "否", enabled && view.initialized);
        DrawRectangleLinesEx(selection.prompt ? layout.no : layout.yes, 1, blue);
    } else {
        skin.button(layout.terminate, "结束授勋", enabled && view.initialized);
        skin.button(layout.grant, "授予",
                    enabled && view.initialized && view.medals > 0 && !view.rows.empty());
    }
}
} // namespace ark::desktop::ui
