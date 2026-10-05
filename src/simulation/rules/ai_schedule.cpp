#include "ark/simulation/rules/ai_schedule.hpp"

#include <algorithm>
#include <set>

namespace ark::simulation::rules {
namespace {
constexpr std::size_t index(AiRosterKind kind) { return static_cast<std::size_t>(kind); }
} // namespace
AiScheduleResult prepare_ai_schedule(const AiScheduleInput &i, const AiScheduleHandler &handler) {
    if (i.dispatch_limit == 0 || i.dispatch_limit > 1000000)
        return {AiScheduleError::invalid_input, std::nullopt};
    std::array<std::set<std::uint64_t>, 6> identities;
    std::size_t total{};
    for (std::size_t list = 0; list < i.rosters.size(); ++list) {
        total += i.rosters[list].size();
        if (total > 1000000)
            return {AiScheduleError::invalid_input, std::nullopt};
        for (const auto id : i.rosters[list])
            if (!identities[list].insert(id).second)
                return {AiScheduleError::duplicate_id, std::nullopt};
    }
    AiScheduleCandidate c{i.rosters, {}};
    if (!i.admitted)
        return {AiScheduleError::none, c};
    if (!handler)
        return {AiScheduleError::invalid_input, std::nullopt};
    AiScheduleError error{AiScheduleError::none};
    const auto call = [&](AiScheduleVisit visit) -> std::optional<AiScheduleResponse> {
        if (c.visits.size() >= i.dispatch_limit) {
            error = AiScheduleError::dispatch_limit;
            return std::nullopt;
        }
        c.visits.push_back(visit);
        auto response = handler(visit, c.rosters);
        if (!response.accepted) {
            error = AiScheduleError::consumer_failed;
            return std::nullopt;
        }
        if ((response.release_current_facility &&
             (visit.phase != AiSchedulePhase::human_execution || !response.remove)) ||
            (response.remove && (visit.phase == AiSchedulePhase::release_human_facility ||
                                 visit.phase == AiSchedulePhase::finalize))) {
            error = AiScheduleError::invalid_response;
            return std::nullopt;
        }
        for (const auto &addition : response.append) {
            const auto list = index(addition.kind);
            if (list >= c.rosters.size() || total >= 1000000) {
                error = AiScheduleError::invalid_response;
                return std::nullopt;
            }
            if (!identities[list].insert(addition.id).second) {
                error = AiScheduleError::duplicate_id;
                return std::nullopt;
            }
            c.rosters[list].push_back(addition.id);
            ++total;
        }
        return response;
    };
    const auto pass = [&](AiRosterKind kind, AiSchedulePhase phase, bool reverse) -> bool {
        auto &roster = c.rosters[index(kind)];
        std::int64_t cursor = reverse ? static_cast<std::int64_t>(roster.size()) - 1 : 0;
        while (cursor >= 0 && static_cast<std::size_t>(cursor) < roster.size()) {
            const auto offset = static_cast<std::size_t>(cursor);
            const auto id = roster[offset];
            const auto response = call({phase, id});
            if (!response)
                return false;
            if (response->release_current_facility &&
                !call({AiSchedulePhase::release_human_facility, id}))
                return false;
            if (response->remove) {
                roster.erase(roster.begin() + static_cast<std::ptrdiff_t>(offset));
                identities[index(kind)].erase(id);
                --total;
            }
            // Forward deletion still advances the index: the next shifted entry is skipped.
            cursor += reverse ? -1 : 1;
        }
        return true;
    };
    if (!pass(AiRosterKind::human, AiSchedulePhase::human_decision, true) ||
        !pass(AiRosterKind::human, AiSchedulePhase::human_execution, true) ||
        !pass(AiRosterKind::monster, AiSchedulePhase::monster_decision, true) ||
        !pass(AiRosterKind::monster, AiSchedulePhase::monster_execution, false) ||
        !pass(AiRosterKind::projectile, AiSchedulePhase::projectile, true) ||
        !pass(AiRosterKind::object, AiSchedulePhase::object, true) ||
        !pass(AiRosterKind::encounter, AiSchedulePhase::encounter, true) ||
        !pass(AiRosterKind::facility, AiSchedulePhase::facility, false) ||
        !call({AiSchedulePhase::finalize, std::nullopt}))
        return {error, std::nullopt};
    return {AiScheduleError::none, c};
}
} // namespace ark::simulation::rules
