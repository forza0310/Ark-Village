// Adapted from published research 4ba4805; independent product build.
// Phase2 emits task/map/UI requests; summary rewards never re-grant inventory.
#include "ark/facilities/dungeon.hpp"
#include <algorithm>
namespace ark::facilities {
std::optional<DungeonCompletionCandidate>
prepare_dungeon_completion(const DungeonCompletionInput &i) {
    if (i.updates < 0 || i.source_site.x < 0 || i.source_site.y < 0)
        return {};
    DungeonCompletionCandidate c;
    if (i.updates < 10)
        return c;
    const auto request = [&](DungeonCompletionRequestKind kind, int first = 0, int second = 0,
                             std::optional<people::ActorId> actor = {}) {
        c.requests.push_back({kind, actor, first, second});
    };
    if (!i.active_task) {
        request(DungeonCompletionRequestKind::restore_site, 0);
        for (std::size_t n = i.site_tasks.size(); n > 0; --n) {
            const auto &t = i.site_tasks[n - 1];
            if (t.kind == 0 && (!t.site || *t.site == i.source_site))
                request(DungeonCompletionRequestKind::remove_site_task, static_cast<int>(n - 1));
        }
        return c;
    }
    const auto &task = *i.active_task;
    if (task.kind < 0 || task.kind > 1)
        return {};
    if (task.kind == 1) {
        request(DungeonCompletionRequestKind::clear_active_task);
        return c;
    }
    if (task.definition < 0 || task.pending_completion_value < 0 || task.difficulty < 1 ||
        task.difficulty > 9 || task.participants.empty() || task.participants.size() > 100000 ||
        i.summary_tickets.size() < 2)
        return {};
    for (const int ticket : {i.summary_tickets[0], i.summary_tickets[1]})
        if (ticket < 0 || static_cast<std::size_t>(ticket) >= task.participants.size())
            return {};
    for (const auto &a : i.human_order)
        if (!a.id.value || a.definition < 0)
            return {};
    for (const int definition : task.participants) {
        if (definition < 0)
            return {};
        const auto actor = std::find_if(i.human_order.begin(), i.human_order.end(),
                                        [&](const auto &a) { return a.definition == definition; });
        if (actor == i.human_order.end())
            continue; // Source no live instance => no display, growth or boost clearing.
        request(DungeonCompletionRequestKind::actor_reward_display, 20 - 28, task.difficulty * 5,
                actor->id);
        request(DungeonCompletionRequestKind::definition_reward, 20, task.difficulty * 5,
                actor->id);
        request(DungeonCompletionRequestKind::clear_boost, 2048, 0, actor->id);
    }
    request(DungeonCompletionRequestKind::summary30, task.difficulty * 5);
    request(DungeonCompletionRequestKind::summary32, task.participants[i.summary_tickets[0]],
            task.participants[i.summary_tickets[1]]);
    c.consumed_summary_tickets = 2;
    for (const auto &challenge : task.global_challenges)
        if (challenge[1] == 0) {
            if (challenge[4] < 0 || challenge[4] > 3 || challenge[5] < 0)
                return {};
            c.summary_rewards.push_back({challenge[4], challenge[5]});
        }
    request(DungeonCompletionRequestKind::event126);
    request(DungeonCompletionRequestKind::record_complete, task.pending_completion_value);
    request(DungeonCompletionRequestKind::restore_site, 1);
    request(DungeonCompletionRequestKind::record_task_success, task.definition);
    request(DungeonCompletionRequestKind::clear_active_task);
    if (!i.event201_seen)
        request(DungeonCompletionRequestKind::event201);
    if (!i.event92_seen)
        request(DungeonCompletionRequestKind::event92);
    return c;
}
} // namespace ark::facilities
