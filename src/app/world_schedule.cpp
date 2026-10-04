#include "ark/app/world_schedule.hpp"
#include <algorithm>
#include <limits>
#include <set>

namespace ark::app {
namespace {
bool valid(const WorldScheduleState &state) {
    if (state.updates < 0 || state.updates == std::numeric_limits<int>::max())
        return false;
    if (!prepare_ai_schedule({state.rosters, false, 1000000}, {}).candidate)
        return false;
    for (const auto &hint : state.hints)
        if (hint.size() < 2 || hint[1] < 0 || hint[1] == std::numeric_limits<int>::max())
            return false;
    for (const auto &note : state.floating_notes)
        if (note.empty() || note[0] < 0 || note[0] == std::numeric_limits<int>::max())
            return false;
    for (const auto &request : state.popularity_queue)
        if (request[0] == std::numeric_limits<int>::min())
            return false;
    return true;
}
bool prefix(const std::vector<std::uint64_t> &before, const std::vector<std::uint64_t> &after) {
    return after.size() >= before.size() && std::equal(before.begin(), before.end(), after.begin());
}
bool domain(WorldScheduleStage stage) {
    return stage == WorldScheduleStage::decision || stage == WorldScheduleStage::control ||
           stage == WorldScheduleStage::actor_tail || stage == WorldScheduleStage::projectile ||
           stage == WorldScheduleStage::object || stage == WorldScheduleStage::encounter ||
           stage == WorldScheduleStage::facility;
}
} // namespace
WorldScheduleResult prepare_world_schedule(const WorldScheduleState &state,
                                           const WorldScheduleInput &input,
                                           const WorldScheduleConsumer &consumer) {
    if (!valid(state) || !input.dispatch_limit || input.dispatch_limit > 1000000)
        return {WorldScheduleError::invalid_state, {}};
    WorldScheduleCandidate candidate{state, {}};
    if (!input.admitted)
        return {WorldScheduleError::none, candidate};
    if (!consumer)
        return {WorldScheduleError::missing_consumer, {}};
    WorldScheduleError error{WorldScheduleError::none};
    const auto invoke = [&](WorldScheduleCall call) -> std::optional<bool> {
        if (candidate.calls.size() >= input.dispatch_limit) {
            error = WorldScheduleError::dispatch_limit;
            return {};
        }
        const auto before = candidate.state;
        candidate.calls.push_back(call);
        auto next = consumer(before, call);
        if (!next) {
            error = WorldScheduleError::consumer_failed;
            return {};
        }
        if (!valid(next->state)) {
            error = WorldScheduleError::invalid_state;
            return {};
        }
        bool permitted = !next->remove_requested ||
                         (domain(call.stage) && call.stage != WorldScheduleStage::facility);
        for (std::size_t list = 0; list < before.rosters.size(); ++list) {
            const auto &old = before.rosters[list];
            const auto &now = next->state.rosters[list];
            if (call.stage == WorldScheduleStage::arrival_front)
                permitted &= list == 0                ? prefix(old, now)
                             : list == 2 || list == 4 ? (old == now || now.empty())
                                                      : old == now;
            else
                permitted &= domain(call.stage) ? prefix(old, now) : old == now;
        }
        if (call.stage == WorldScheduleStage::popularity)
            permitted &= next->state.popularity_queue == before.popularity_queue;
        if (!permitted) {
            error = WorldScheduleError::invalid_mutation;
            return {};
        }
        candidate.state = std::move(next->state);
        return next->remove_requested;
    };
    const auto plain = [&](WorldScheduleStage stage) {
        return invoke({stage, {}, {}, {}, false}).has_value();
    };
    if (!plain(WorldScheduleStage::influence) || !plain(WorldScheduleStage::arrival_front))
        return {error, {}};
    auto &common = candidate.state;
    if (!common.hints.empty() && ++common.hints.front()[1] >= (common.hints.size() >= 2 ? 20 : 60))
        common.hints.erase(common.hints.begin());
    common.updates = static_cast<int>((static_cast<std::int64_t>(common.updates) + 1) %
                                      std::numeric_limits<int>::max());
    for (std::size_t n = common.floating_notes.size(); n-- > 0;)
        if (++common.floating_notes[n][0] >= 8)
            common.floating_notes.erase(common.floating_notes.begin() +
                                        static_cast<std::ptrdiff_t>(n));
    for (std::size_t n = common.popularity_queue.size(); n-- > 0;) {
        auto &request = common.popularity_queue[n];
        if (--request[0] > 0)
            continue;
        if (!invoke({WorldScheduleStage::popularity,
                     {},
                     {},
                     {{request[1], request[2] == 1 ? 1 : 0}},
                     false}))
            return {error, {}};
        common.popularity_queue.erase(common.popularity_queue.begin() +
                                      static_cast<std::ptrdiff_t>(n));
    }
    common.rescue_available = false;
    if (!plain(WorldScheduleStage::rescue_query))
        return {error, {}};
    const auto scheduled = prepare_ai_schedule(
        {common.rosters, true, input.dispatch_limit},
        [&](const AiScheduleVisit &visit, const AiRosters &current) {
            AiScheduleResponse response;
            const auto reject = [&] {
                response.accepted = false;
                return response;
            };
            if (current != common.rosters) {
                error = WorldScheduleError::invalid_mutation;
                return reject();
            }
            const bool decision = visit.phase == AiSchedulePhase::human_decision ||
                                  visit.phase == AiSchedulePhase::monster_decision;
            const bool execution = visit.phase == AiSchedulePhase::human_execution ||
                                   visit.phase == AiSchedulePhase::monster_execution;
            const std::size_t list = decision || execution
                                         ? (visit.phase == AiSchedulePhase::human_decision ||
                                                    visit.phase == AiSchedulePhase::human_execution
                                                ? 0
                                                : 1)
                                     : visit.phase == AiSchedulePhase::projectile ? 2
                                     : visit.phase == AiSchedulePhase::object     ? 3
                                     : visit.phase == AiSchedulePhase::encounter  ? 4
                                                                                  : 5;
            const auto call = [&](WorldScheduleStage stage) {
                return invoke({stage,
                               visit.id,
                               visit.id
                                   ? std::optional<AiRosterKind>(static_cast<AiRosterKind>(list))
                                   : std::nullopt,
                               {},
                               execution});
            };
            std::optional<bool> remove;
            if (decision) {
                if (!call(WorldScheduleStage::decision_prefix))
                    return reject();
                remove = call(WorldScheduleStage::decision);
            } else if (execution) {
                if (!call(WorldScheduleStage::execution_prefix) ||
                    !call(WorldScheduleStage::carry_expression))
                    return reject();
                remove = call(WorldScheduleStage::control);
                if (remove && !*remove)
                    remove = call(WorldScheduleStage::actor_tail);
            } else {
                const auto stage =
                    visit.phase == AiSchedulePhase::projectile  ? WorldScheduleStage::projectile
                    : visit.phase == AiSchedulePhase::object    ? WorldScheduleStage::object
                    : visit.phase == AiSchedulePhase::encounter ? WorldScheduleStage::encounter
                    : visit.phase == AiSchedulePhase::facility  ? WorldScheduleStage::facility
                                                                : WorldScheduleStage::finalize;
                remove = call(stage);
            }
            if (!remove)
                return reject();
            if (*remove && (decision || execution) && !call(WorldScheduleStage::remove_actor))
                return reject();
            // Return every appended identity to the underlying live-roster scheduler in order.
            for (std::size_t n = 0; n < current.size(); ++n)
                for (std::size_t at = current[n].size(); at < common.rosters[n].size(); ++at)
                    response.append.push_back(
                        {static_cast<AiRosterKind>(n), common.rosters[n][at]});
            if (*remove) {
                auto &roster = common.rosters[list];
                roster.erase(std::find(roster.begin(), roster.end(), *visit.id));
                response.remove = true;
            }
            return response;
        });
    if (!scheduled.candidate)
        return {error != WorldScheduleError::none ? error
                : scheduled.error == AiScheduleError::dispatch_limit
                    ? WorldScheduleError::dispatch_limit
                    : WorldScheduleError::invalid_mutation,
                {}};
    if (scheduled.candidate->rosters != common.rosters)
        return {WorldScheduleError::invalid_mutation, {}};
    return {WorldScheduleError::none, std::move(candidate)};
}
} // namespace ark::app
