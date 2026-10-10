#include "ark/simulation/ai/rules/dungeon_ai.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
namespace {
bool fits(std::int64_t n) {
    return n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max();
}
std::optional<int> scaled(int value, int extent) {
    if (extent < 0)
        return {};
    if (value < 0 || extent == 0)
        return 0;
    if (value > extent)
        return 100;
    const auto numerator = static_cast<std::int64_t>(value) * 100;
    if (!fits(numerator))
        return {};
    return static_cast<int>(numerator / extent);
}
bool valid(const DungeonCrewInput &i) {
    const auto &s = i.state;
    if (s.phase != 1 || s.facility_updates < 0 || s.progress < 0 || s.extent < 0 ||
        s.occupants.empty() || s.occupants.size() > 100000 || s.challenges.size() > 100000 ||
        (i.active_task_extent && *i.active_task_extent < 0))
        return false;
    for (const auto id : s.occupants) {
        const auto a = s.actors.find(id);
        if (!id.value || a == s.actors.end() || a->second.retreat_state < 0 ||
            a->second.retreat_updates < 0 ||
            a->second.retreat_updates == std::numeric_limits<int>::max())
            return false;
    }
    for (const auto &challenge : s.challenges)
        if (challenge[0] < 0 || challenge[1] < 0 || challenge[1] > 1 || challenge[2] < 0 ||
            challenge[2] > 3 || challenge[3] < 0 ||
            challenge[3] == std::numeric_limits<int>::max() || challenge[4] < 0 ||
            (challenge[1] == 0 && (challenge[4] > 3 || challenge[5] < 0)))
            return false;
    return true;
}
} // namespace
std::optional<int> dungeon_endurance(const std::vector<int> &levels) {
    if (levels.empty() || levels.size() > 1000)
        return {};
    int value = 3;
    for (const int level : levels) {
        if (level < 1 || level > 10)
            return {};
        value += (level - 1) / 2;
    }
    return value;
}
std::optional<DungeonActorProgress> prepare_dungeon_actor_entry(const std::vector<int> &levels,
                                                                bool old_constrained) {
    const auto endurance = dungeon_endurance(levels);
    if (!endurance)
        return {};
    DungeonActorProgress a;
    a.endurance = *endurance;
    a.constrained = old_constrained; // Tenant.a resets V/W/X/Z/aa, but does NOT clear bw.
    return a;
}
DungeonCrewResult prepare_dungeon_crew(const DungeonCrewInput &i) {
    const auto failed = [](DungeonCrewError error) -> DungeonCrewResult { return {error, {}}; };
    if (!valid(i))
        return failed(DungeonCrewError::invalid_input);
    DungeonCrewCandidate c;
    c.state = i.state;
    auto &s = c.state;
    for (std::size_t n = 1; n < s.occupants.size() && s.actors.at(s.occupants[n]).constrained; ++n)
        c.leader_step += 3;
    for (const auto id : s.occupants)
        s.actors.at(id).constrained = false;
    int maximum{};
    bool reorder{};
    for (std::size_t n = s.occupants.size(); n > 0; --n) {
        const auto id = s.occupants[n - 1];
        auto &a = s.actors.at(id);
        if (a.retreat_updates == std::numeric_limits<int>::max())
            return failed(DungeonCrewError::numeric_overflow);
        ++a.retreat_updates;
        if (a.retreat_state == 0) {
            a.previous = a.progress;
            const int step = n == 1 ? c.leader_step : 10;
            const auto next = static_cast<std::int64_t>(a.progress) +
                              (n == 1 && a.progress >= s.progress ? step : step * 2);
            if (!fits(next))
                return failed(DungeonCrewError::numeric_overflow);
            a.progress = static_cast<int>(next);
            const auto percent = scaled(a.progress, i.active_task_extent.value_or(0));
            if (!percent)
                return failed(DungeonCrewError::numeric_overflow);
            a.percent = *percent;
            maximum = std::max(maximum, a.progress);
        } else if (a.retreat_state == 1 && a.retreat_updates >= 20) {
            if (c.consumed_retreats >= i.retreats.size() ||
                !(i.retreats[c.consumed_retreats].actor == id))
                return failed(DungeonCrewError::unresolved_retreat);
            c.requests.push_back({DungeonCrewRequestKind::retreat, id, 0, 0});
            if (i.retreats[c.consumed_retreats].remove_first_occupation) {
                const auto first = std::find(s.occupants.begin(), s.occupants.end(), id);
                s.occupants.erase(first); // m.b removes FIRST matching reference, not this index.
            }
            ++c.consumed_retreats;
            reorder = true; // Also true when source has no legal retreat target.
        }
    }
    if (reorder) {
        // Match the source pairwise strict comparison, preserving its non-stable tie outcome.
        for (std::size_t left = 0; left < s.occupants.size(); ++left)
            for (std::size_t right = s.occupants.size(); right > left + 1; --right)
                if (s.actors.at(s.occupants[right - 1]).progress >
                    s.actors.at(s.occupants[left]).progress)
                    std::swap(s.occupants[left], s.occupants[right - 1]);
    }
    for (std::size_t n = 1; n < s.occupants.size(); ++n) {
        auto &a = s.actors.at(s.occupants[n]);
        const auto cap = static_cast<std::int64_t>(s.actors.at(s.occupants[n - 1]).progress) - 300;
        if (!fits(cap))
            return failed(DungeonCrewError::numeric_overflow);
        const int previous = a.progress;
        a.progress = std::min(a.progress, static_cast<int>(cap));
        const auto percent = scaled(a.progress, i.active_task_extent.value_or(0));
        if (!percent)
            return failed(DungeonCrewError::numeric_overflow);
        a.percent = *percent;
        a.constrained = previous != a.progress;
    }
    s.progress = std::max(s.progress, maximum);
    const auto crew_percent = scaled(s.progress, i.active_task_extent.value_or(0));
    if (!crew_percent)
        return failed(DungeonCrewError::numeric_overflow);
    for (std::size_t n = s.challenges.size(); n > 0; --n) {
        auto &challenge = s.challenges[n - 1];
        const int old_status = challenge[2];
        ++challenge[3];
        if (old_status == 0 && *crew_percent >= challenge[0]) {
            if (challenge[1] == 0) {
                challenge[2] = 2;
                challenge[3] = 0;
                c.requests.push_back(
                    {DungeonCrewRequestKind::grant_catalog_reward, {}, challenge[4], challenge[5]});
            } else {
                for (auto member = s.occupants.rbegin(); member != s.occupants.rend(); ++member) {
                    auto &a = s.actors.at(*member);
                    if (a.percent < challenge[0] || a.retreat_state != 0)
                        continue;
                    const bool depleted = s.facility_updates % 25 == 0;
                    if (depleted) {
                        if (a.endurance == std::numeric_limits<int>::min() ||
                            challenge[5] == std::numeric_limits<int>::min())
                            return failed(DungeonCrewError::numeric_overflow);
                        --a.endurance;
                        --challenge[5];
                        c.requests.push_back({DungeonCrewRequestKind::progress_label231, *member,
                                              -8 + a.percent * 161 / 100 + 16, 0});
                    }
                    if (challenge[5] <= 0) {
                        challenge[2] = 1;
                        challenge[3] = 0;
                    } else if (a.endurance > 0 || !depleted)
                        a.progress = a.previous;
                    else {
                        a.retreat_state = 1;
                        a.retreat_updates = 0;
                    }
                    // Do not break when the challenge is beaten: later references still consume it.
                }
            }
        } else if (old_status == 1) {
            if (challenge[1] == 0 && challenge[3] >= 10) {
                c.requests.push_back(
                    {DungeonCrewRequestKind::spawn_catalog_reward, {}, challenge[4], challenge[5]});
                s.challenges.erase(s.challenges.begin() + static_cast<std::ptrdiff_t>(n - 1));
            } else if (challenge[1] == 1 && challenge[3] >= 40)
                s.challenges.erase(s.challenges.begin() + static_cast<std::ptrdiff_t>(n - 1));
        } else if (old_status == 2 && challenge[1] == 0) {
            for (const auto id : s.occupants) {
                auto &a = s.actors.at(id);
                if (a.percent >= challenge[0] && a.retreat_state == 0)
                    a.progress = a.previous;
            }
            if (challenge[3] >= 40) {
                challenge[2] = 3;
                challenge[3] = 0;
            }
        }
    }
    if (s.progress >= s.extent) {
        c.completed = true;
        s.phase = 2;
        s.facility_updates = 0;
        for (std::size_t n = 0; n < s.occupants.size(); ++n)
            c.requests.push_back(
                {DungeonCrewRequestKind::exit_crew, s.occupants[n], static_cast<int>(n) * 5, 0});
    }
    const auto final_percent = scaled(s.progress, s.extent);
    if (!final_percent)
        return failed(DungeonCrewError::numeric_overflow);
    s.previous_percent = s.percent;
    s.percent = *final_percent;
    return {DungeonCrewError::none, c};
}
std::optional<int> dungeon_extent(int difficulty, int q) {
    if (difficulty < 1 || difficulty > 9 || q <= 0)
        return {};
    const auto low = static_cast<std::int64_t>(q) * 120;
    const auto high = static_cast<std::int64_t>(q) * 240;
    const auto numerator = (difficulty - 1) * (high - low);
    const auto value = low + numerator / 8;
    if (!fits(low) || !fits(high) || !fits(numerator) || !fits(value))
        return {};
    return static_cast<int>(value);
}
std::optional<DungeonCompletionCandidate>
prepare_dungeon_completion(const DungeonCompletionInput &i) {
    if (i.updates < 0 || i.source_site.x < 0 || i.source_site.y < 0)
        return {};
    DungeonCompletionCandidate c;
    if (i.updates < 10)
        return c;
    const auto request = [&](DungeonCompletionRequestKind kind, int first = 0, int second = 0,
                             std::optional<CharacterId> actor = {}) {
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
} // namespace ark::simulation::rules
