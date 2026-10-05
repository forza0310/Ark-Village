// Frozen research 40972a9 publishes raw87's data/actions, not its complete pixel layout.
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
           (page.legacy_page == 16 || page.legacy_page == 56 || page.legacy_page == 57);
}
bool world_page_regular_confirmation(const Page &page) {
    return !world_page_automatic(page) && !(page.kind == Kind::raw_page && page.legacy_page == 87);
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
    for (const int id : ranks->second) {
        const auto human = std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                                        [id](const auto &value) { return value.identity == id; });
        if (human == state.rules->humans.end())
            throw std::invalid_argument("Annual display references an unknown human definition");
        view.rows.push_back({id, human->name, state.human_calendar.at(id).contribution});
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
    layout.terminate = {p.x + width - 108, p.y + height - 29, 98, 21};
    layout.prompt = {p.x + 8, p.y + height - 76, width - 16, 38};
    layout.yes = {p.x + width / 2 - 66, p.y + height - 29, 58, 21};
    layout.no = {p.x + width / 2 + 8, p.y + height - 29, 58, 21};
    return layout;
}
std::optional<simulation::rules::WorldAwardAction>
world_award_input(const WorldAwardView &view, const WorldAwardLayout &layout,
                  std::optional<Vector2> click, bool enter, bool escape, bool blocked) {
    using Action = simulation::rules::WorldAwardAction;
    if (blocked || !view.initialized)
        return {};
    if (view.termination_pending) {
        if (hit(click, layout.no) || escape)
            return Action::reject_termination;
        if (hit(click, layout.yes) || enter)
            return Action::confirm_termination;
    } else if (hit(click, layout.terminate) || enter) {
        return Action::request_termination;
    }
    return {};
}
void draw_world_award(const WorldAwardView &view, const WorldAwardLayout &layout, const Skin &skin,
                      int first_row, bool enabled) {
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
        std::clamp(first_row, 0, std::max(0, static_cast<int>(view.rows.size()) - count));
    for (int index = first; index < static_cast<int>(view.rows.size()) && index < first + count;
         ++index) {
        const auto &row = view.rows[index];
        const float y = layout.rows.y + (index - first) * 18;
        // Scale long names only; ranking and numeric contribution retain their source identity.
        const auto name = std::to_string(index + 1) + ". " + row.name;
        const float size =
            std::min(12.F, 12.F * (layout.rows.width - 40) / std::max(1.F, skin.text.width(name)));
        skin.text.draw(name, layout.rows.x, y, ink, size);
        skin.right(std::to_string(row.contribution), layout.rows.x + layout.rows.width, y, blue);
    }
    if (!view.termination_pending && static_cast<int>(view.rows.size()) > count)
        skin.text.draw(
            std::to_string(first + 1) + "-" +
                std::to_string(std::min(first + count, static_cast<int>(view.rows.size()))) + "/" +
                std::to_string(view.rows.size()),
            layout.rows.x, layout.panel.y + layout.panel.height - 24, ink, 9);
    if (view.termination_pending) {
        skin.content(layout.prompt);
        skin.centered("结束本次授勋吗？", layout.prompt);
        skin.button(layout.yes, "是", enabled && view.initialized);
        skin.button(layout.no, "否", enabled && view.initialized);
    } else {
        skin.button(layout.terminate, "结束授勋", enabled && view.initialized);
    }
}
} // namespace ark::desktop::ui
