// Adapted from the maintained world_dungeon task-success consumer, 2026-10-04.
// Source semantics/ordering: research/dungeon_village_1/rules/ai/DUNGEONS.md#completion-consumers.
#include "ark/facilities/dungeon_task.hpp"
#include <algorithm>
#include <limits>

namespace ark::facilities {
std::optional<DungeonTaskSuccessCandidate>
prepare_dungeon_task_success(const DungeonTaskSuccessState &state, int definition, int raw_year,
                             int raw_month) {
    const auto found = state.definitions.find(definition);
    if (definition < 0 || found == state.definitions.end() || found->second.kind < 0 ||
        found->second.kind > 1 || found->second.completed < 0 || state.successes < 0 ||
        state.ordinary_explorations < 0 || state.exploration_stage < 0 ||
        state.exploration_stage > 5 || state.task_pool_progress < 0 || raw_year < 0 ||
        raw_month < 0 || raw_month >= 12)
        return {};

    const auto increment = [](int &value) {
        if (value == std::numeric_limits<int>::max())
            return false;
        ++value;
        return true;
    };
    DungeonTaskSuccessCandidate candidate{state, {}};
    auto &task = candidate.state.definitions.at(definition);
    if (!increment(candidate.state.successes) || !increment(task.completed) ||
        (task.kind == 0 && !(task.flags & 8U) && !increment(candidate.state.ordinary_explorations)))
        return {};

    // Stage5 still records the current raw date; only advancement is capped.
    if (task.flags & 2U) {
        candidate.state.exploration_dates[candidate.state.exploration_stage] = {raw_year,
                                                                                raw_month};
        candidate.state.exploration_stage = std::min(candidate.state.exploration_stage + 1, 5);
    }
    if (task.flags & 4U) {
        const auto monster = candidate.state.monsters.find(task.monster_definition);
        if (task.monster_definition < 0 || monster == candidate.state.monsters.end() ||
            monster->second.status < 0)
            return {};
        if (monster->second.status == 0)
            monster->second.pending_notice = true;
        monster->second.status = 1;
    }

    // The just-completed task is still in bq here and can itself suppress G. Preserve the
    // source short circuit: once flag2 is found, later references/G are not consumed.
    for (const int current : state.remaining_task_definitions) {
        const auto remaining = state.definitions.find(current);
        if (remaining == state.definitions.end())
            return {};
        if (remaining->second.flags & 2U)
            return candidate;
    }
    const int amount = task.kind == 0 ? 50 : 100;
    if (state.task_pool_progress > std::numeric_limits<int>::max() - amount)
        return {};
    candidate.state.task_pool_progress += amount;
    constexpr int thresholds[]{300, 400, 500};
    for (int index = 0; index < 3; ++index)
        if (state.task_pool_progress < thresholds[index] &&
            candidate.state.task_pool_progress >= thresholds[index])
            candidate.threshold_notice_ids.push_back(29 + index);
    return candidate;
}
} // namespace ark::facilities
