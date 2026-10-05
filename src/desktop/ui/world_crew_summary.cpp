// Frozen prototype page31 presents up to five rows. Scrolling and native-window dimensions are
// desktop adaptations; this module never initializes the page, pays a reward or changes a task.
#include "world_crew_summary.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
WorldCrewSummaryView world_crew_summary_view(const simulation::StartupWorldRuntimeState &state,
                                             std::uint64_t page) {
    WorldCrewSummaryView view;
    const auto crew = state.crew_summaries.find(page);
    if (crew == state.crew_summaries.end())
        return view;
    if (!state.rules)
        throw std::invalid_argument("Crew summary requires the human catalogue");
    view.initialized = true;
    for (const int id : crew->second) {
        const auto definition =
            std::find_if(state.rules->humans.begin(), state.rules->humans.end(),
                         [id](const auto &human) { return human.identity == id; });
        if (definition == state.rules->humans.end())
            throw std::invalid_argument("Crew summary references an unknown human definition");
        const auto &human = state.scene.world.world.ai.battle.humans.at(id);
        // The initializer already normalized special-task H. Preserve X order and duplicates;
        // do not substitute current participants, recompute kills or award experience here.
        view.rows.push_back({id, definition->name, human.task_kills, human.participant_downs});
    }
    return view;
}
WorldCrewSummaryLayout world_crew_summary_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Crew summary requires the supported logical viewport");
    const float width = std::min(310.F, extent.width - 16.F);
    const float height = std::min(202.F, extent.height - 68.F);
    WorldCrewSummaryLayout layout;
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &box = layout.panel;
    layout.rows = {box.x + 12, box.y + 49, width - 24, height - 89};
    layout.confirm = {box.x + width - 68, box.y + height - 28, 58, 20};
    return layout;
}
int world_crew_summary_visible_rows(const WorldCrewSummaryLayout &layout) {
    return std::clamp(static_cast<int>(layout.rows.height / 17), 1, 5);
}
void draw_world_crew_summary(const WorldCrewSummaryView &view, const WorldCrewSummaryLayout &layout,
                             const Skin &skin, int first_row, bool enabled) {
    skin.window(layout.panel, "任务成果");
    const float kill_right = layout.rows.x + layout.rows.width - 45;
    const float down_right = layout.rows.x + layout.rows.width;
    skin.text.draw("姓名", layout.rows.x, layout.panel.y + 28);
    skin.right("打倒数", kill_right, layout.panel.y + 28, ink, 10);
    skin.right("倒地", down_right, layout.panel.y + 28, ink, 10);
    skin.content(
        {layout.rows.x - 5, layout.rows.y - 5, layout.rows.width + 10, layout.rows.height + 10});
    const int count = world_crew_summary_visible_rows(layout);
    const int first =
        std::clamp(first_row, 0, std::max(0, static_cast<int>(view.rows.size()) - count));
    for (int i = first; i < static_cast<int>(view.rows.size()) && i < first + count; ++i) {
        const auto &row = view.rows[i];
        const float y = layout.rows.y + (i - first) * 17;
        const float size = std::min(12.F, 12.F * (layout.rows.width - 90) /
                                              std::max(1.F, skin.text.width(row.name)));
        skin.text.draw(row.name, layout.rows.x, y, ink, size);
        skin.right(std::to_string(row.kills), kill_right, y, blue);
        skin.right(std::to_string(row.downs), down_right, y, blue);
    }
    if (static_cast<int>(view.rows.size()) > count)
        skin.text.draw(
            std::to_string(first + 1) + "-" +
                std::to_string(std::min(first + count, static_cast<int>(view.rows.size()))) + "/" +
                std::to_string(view.rows.size()),
            layout.rows.x, layout.confirm.y + 4, ink, 9);
    skin.button(layout.confirm, "确定", enabled && view.initialized);
}
} // namespace ark::desktop::ui
