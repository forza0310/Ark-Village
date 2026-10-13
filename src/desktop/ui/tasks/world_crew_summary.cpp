// Frozen prototype page31 presents up to five rows. Scrolling and native-window dimensions are
// desktop adaptations; this module never initializes the page, pays a reward or changes a task.
#include "world_crew_summary.hpp"
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
} // namespace ark::desktop::ui
