// Schedule-only cases adapted from research d412d6e. Death/pickup domain consumers are
// outside this batch; product growth/equipment composition is covered in people_progression.
#include "ark/app/ai_schedule.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::app;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
std::vector<std::uint64_t> ids(const AiScheduleCandidate &c, AiSchedulePhase phase) {
    std::vector<std::uint64_t> result;
    for (const auto &visit : c.visits)
        if (visit.phase == phase && visit.id)
            result.push_back(*visit.id);
    return result;
}
void all_passes() {
    AiScheduleInput i;
    for (auto &roster : i.rosters)
        roster = {0, 1, 2};
    const auto r =
        prepare_ai_schedule(i, [](const auto &, const auto &) { return AiScheduleResponse{}; });
    check(r.candidate && r.candidate->rosters == i.rosters, "scoped zeroIDs valid and unchanged");
    for (const auto phase : {AiSchedulePhase::human_decision, AiSchedulePhase::human_execution,
                             AiSchedulePhase::monster_decision, AiSchedulePhase::projectile,
                             AiSchedulePhase::object, AiSchedulePhase::encounter})
        check(ids(*r.candidate, phase) == std::vector<std::uint64_t>{2, 1, 0},
              "six reverse passes preserve source reverse order");
    for (const auto phase : {AiSchedulePhase::monster_execution, AiSchedulePhase::facility})
        check(ids(*r.candidate, phase) == std::vector<std::uint64_t>{0, 1, 2},
              "monster d and facility updates use forward order");
    check(r.candidate->visits[2].phase == AiSchedulePhase::human_decision &&
              r.candidate->visits[3].phase == AiSchedulePhase::human_execution &&
              r.candidate->visits.back().phase == AiSchedulePhase::finalize,
          "finish all humans c BEFORE all humans d, finalize last");
    i.admitted = false;
    const auto paused = prepare_ai_schedule(i, {});
    check(paused.candidate && paused.candidate->visits.empty() &&
              paused.candidate->rosters == i.rosters,
          "not admitted does not tick or invoke consumer, not guessed menu gating");
}
void deletion_matrix() {
    for (int n = 0; n <= 8; ++n)
        for (int mask = 0; mask < (1 << n); ++mask)
            for (bool human : {false, true}) {
                AiScheduleInput i;
                const std::size_t list = human ? 0 : 1;
                for (int id = 0; id < n; ++id)
                    i.rosters[list].push_back(id);
                const auto phase =
                    human ? AiSchedulePhase::human_execution : AiSchedulePhase::monster_execution;
                const auto r = prepare_ai_schedule(i, [&](const auto &v, const auto &) {
                    AiScheduleResponse response;
                    if (v.phase == phase)
                        response.remove = (mask & (1 << *v.id)) != 0;
                    return response;
                });
                check(r.candidate.has_value(), "deletion matrix remains valid");
                std::vector<std::uint64_t> expected_visits;
                std::vector<std::uint64_t> expected_remaining;
                if (human) {
                    for (int id = n - 1; id >= 0; --id)
                        expected_visits.push_back(id);
                    for (int id = 0; id < n; ++id)
                        if (!(mask & (1 << id)))
                            expected_remaining.push_back(id);
                } else {
                    for (int id = 0; id < n; ++id) {
                        expected_visits.push_back(id);
                        if (mask & (1 << id)) {
                            if (id + 1 < n)
                                expected_remaining.push_back(++id);
                        } else
                            expected_remaining.push_back(id);
                    }
                }
                check(ids(*r.candidate, phase) == expected_visits,
                      "human reverse no skip; monster forward erase skips next original actor");
                check(r.candidate->rosters[list] == expected_remaining,
                      "only visited true consumers remove, false never inverted");
            }
    AiScheduleInput i;
    i.rosters[0] = {0};
    const auto r = prepare_ai_schedule(i, [](const auto &v, const auto &rosters) {
        AiScheduleResponse response;
        if (v.phase == AiSchedulePhase::human_execution) {
            response.remove = true;
            response.release_current_facility = true;
        }
        if (v.phase == AiSchedulePhase::release_human_facility)
            check(rosters[0] == std::vector<std::uint64_t>{0}, "release runs before roster erase");
        return response;
    });
    check(r.candidate && r.candidate->rosters[0].empty() &&
              r.candidate->visits[2].phase == AiSchedulePhase::release_human_facility,
          "human d deletion releases q first");
}
void same_round_creation() {
    AiScheduleInput i;
    i.rosters[0] = {1};
    i.rosters[4] = {1};
    const auto r = prepare_ai_schedule(i, [](const auto &v, const auto &) {
        AiScheduleResponse response;
        if (v.phase == AiSchedulePhase::human_decision && v.id == 1) {
            response.append = {{AiRosterKind::human, 2}, {AiRosterKind::monster, 2}};
        } else if (v.phase == AiSchedulePhase::human_execution && v.id == 2) {
            response.append = {{AiRosterKind::projectile, 3}};
        } else if (v.phase == AiSchedulePhase::monster_execution && v.id == 2) {
            response.append = {{AiRosterKind::monster, 4}};
        } else if (v.phase == AiSchedulePhase::projectile && v.id == 3) {
            response.append = {{AiRosterKind::object, 5}, {AiRosterKind::projectile, 6}};
        } else if (v.phase == AiSchedulePhase::encounter) {
            response.append = {{AiRosterKind::monster, 7}};
        }
        return response;
    });
    check(r.candidate &&
              ids(*r.candidate, AiSchedulePhase::human_decision) == std::vector<std::uint64_t>{1} &&
              ids(*r.candidate, AiSchedulePhase::human_execution) ==
                  std::vector<std::uint64_t>{2, 1},
          "new reverse-tail human waits c, gets same-round d");
    check(ids(*r.candidate, AiSchedulePhase::monster_decision) == std::vector<std::uint64_t>{2} &&
              ids(*r.candidate, AiSchedulePhase::monster_execution) ==
                  std::vector<std::uint64_t>{2, 4},
          "human-created monster gets both passes, forward append gets d without same-round c");
    check(ids(*r.candidate, AiSchedulePhase::projectile) == std::vector<std::uint64_t>{3} &&
              r.candidate->rosters[2] == std::vector<std::uint64_t>{3, 6},
          "human d projectile updates this round, projectile-created projectile waits next");
    check(ids(*r.candidate, AiSchedulePhase::object) == std::vector<std::uint64_t>{5} &&
              r.candidate->rosters[1].back() == 7,
          "projectile-created object updates this round, encounter-created monster waits next");
}
void errors() {
    AiScheduleInput i;
    i.rosters[0] = {0, 0};
    check(prepare_ai_schedule(i, {}).error == AiScheduleError::duplicate_id,
          "duplicate input IDs rejected");
    i.rosters[0] = {0};
    check(prepare_ai_schedule(i, {}).error == AiScheduleError::invalid_input,
          "admitted round needs handler");
    auto r = prepare_ai_schedule(i, [](const auto &v, const auto &) {
        AiScheduleResponse response;
        if (v.phase == AiSchedulePhase::finalize)
            response.accepted = false;
        if (v.phase == AiSchedulePhase::human_decision)
            response.append = {{AiRosterKind::object, 0}};
        return response;
    });
    check(r.error == AiScheduleError::consumer_failed && !r.candidate && i.rosters[3].empty(),
          "late rejection discards all earlier appended candidate rosters, input immutable");
    r = prepare_ai_schedule(i, [](const auto &, const auto &) {
        AiScheduleResponse response;
        response.append = {{AiRosterKind::human, 0}};
        return response;
    });
    check(r.error == AiScheduleError::duplicate_id && !r.candidate,
          "duplicate append fails explicitly");
    i.rosters[0].clear();
    i.rosters[5] = {0};
    r = prepare_ai_schedule(i, [](const auto &, const auto &) {
        AiScheduleResponse response;
        response.remove = true;
        return response;
    });
    check(r.error == AiScheduleError::invalid_response,
          "facility c is void, no invented removal branch");
    i.rosters[5].clear();
    i.rosters[1] = {0};
    i.dispatch_limit = 10;
    r = prepare_ai_schedule(i, [](const auto &v, const auto &) {
        AiScheduleResponse response;
        if (v.phase == AiSchedulePhase::monster_execution)
            response.append = {{AiRosterKind::monster, *v.id + 1}};
        return response;
    });
    check(r.error == AiScheduleError::dispatch_limit && !r.candidate,
          "unbounded forward appends hit explicit safety limit, no hang or silent truncation");
}
} // namespace
int main() {
    try {
        all_passes();
        deletion_matrix();
        same_round_creation();
        errors();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
