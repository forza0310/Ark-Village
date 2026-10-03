#include "dungeon_village_reference/actor_lifecycle.hpp"
#include "dungeon_village_reference/ai_schedule.hpp"
#include "dungeon_village_reference/encounter_lifecycle.hpp"
#include "dungeon_village_reference/object_ai.hpp"

#include <iostream>
#include <stdexcept>

using namespace dungeon_village_reference;
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
void death_then_victory() {
    AiScheduleInput i;
    i.rosters[0] = {1};
    i.rosters[1] = {2};
    i.rosters[4] = {3};
    int corpse_counter = 11;
    EncounterRuntimeState event{3, {5, 5}, 0, 0, 0, 1, 0, 100};
    DelayedRewardState reward;
    bool victory{};
    for (int round = 0; round < 3; ++round) {
        const auto r = prepare_ai_schedule(i, [&](const auto &v, const auto &rosters) {
            AiScheduleResponse response;
            if (v.phase == AiSchedulePhase::human_execution) {
                const auto step = advance_delayed_reward(reward);
                reward = *complete_delayed_reward_step(*step, false);
            } else if (v.phase == AiSchedulePhase::monster_decision) {
                TimedLifecycleInput corpse;
                corpse.kind = ActorKind::monster;
                corpse.state = 3;
                corpse.old_counter = corpse_counter;
                const auto c = prepare_timed_lifecycle(corpse);
                check(c.candidate.has_value(), "corpse state3 has valid lifetime plan");
                response.remove = c.candidate->delete_instance;
            } else if (v.phase == AiSchedulePhase::monster_execution) {
                ++corpse_counter;
            } else if (v.phase == AiSchedulePhase::encounter) {
                EncounterStepInput e;
                e.state = event;
                e.humans = {{{1}, 10, {5, 5}, false, 1, 2, 0, 25, 1}};
                for (const auto id : rosters[1])
                    e.monsters.push_back({{id}, 3});
                e.tickets = {{1000, 999}, {100, 99}};
                const auto c = prepare_encounter_step(e);
                check(c.candidate.has_value(),
                      "event reads post-monster roster, not stale snapshot");
                event = c.candidate->state;
                if (c.candidate->victory) {
                    victory = true;
                    for (const auto &request : c.candidate->requests)
                        if (request.kind == EncounterRequestKind::reward_accumulation)
                            reward = *prepare_delayed_reward(reward, request.value, request.delay);
                }
            }
            return response;
        });
        check(r.candidate.has_value(), "life/event/reward combination tick accepted");
        i.rosters = r.candidate->rosters;
        if (round == 0)
            check(!victory && corpse_counter == 12 && !i.rosters[1].empty(),
                  "old11 c retains corpse even though d advances12, event cannot win early");
        if (round == 1)
            check(victory && i.rosters[1].empty() && reward.amount == 75 && reward.counter == -50,
                  "old12 next c erases, same event pass wins AFTER human d, reward not advanced");
        if (round == 2)
            check(reward.counter == -49 && event.state == 1,
                  "next human d starts delayed growth, retired event cannot enqueue victory twice");
    }
}
void pickup_then_reward() {
    AiScheduleInput i;
    i.rosters[0] = {1, 2};
    i.rosters[3] = {3};
    auto object = *prepare_ground_drop({3}, {}, 0, 1);
    object.state = 3;
    int pickups{};
    int grants{};
    for (int round = 0; round < 60; ++round) {
        const auto r = prepare_ai_schedule(i, [&](const auto &v, const auto &) {
            AiScheduleResponse response;
            if (v.phase == AiSchedulePhase::human_decision) {
                const auto selected =
                    select_ground_object({}, {{{3}, object.state, object.position}});
                PickupInput pickup;
                pickup.touching = true;
                if (selected.selected)
                    pickup.nearest = object;
                const auto c = prepare_ground_pickup(pickup);
                check(c.candidate.has_value(), "fresh H observes earlier reversed actor pickup");
                if (c.candidate->object) {
                    object = *c.candidate->object;
                    ++pickups;
                    check(v.id == 2, "reverse-last human picks before earlier source actor");
                }
            } else if (v.phase == AiSchedulePhase::object) {
                const auto c = advance_ground_object(object, false, 0, false);
                check(c.candidate.has_value(), "pickup state5 progresses after both actor passes");
                object = c.candidate->state;
                response.remove = c.candidate->remove;
                for (const auto &request : c.candidate->requests)
                    if (request.kind == ObjectRewardRequestKind::grant_definition)
                        ++grants;
            }
            return response;
        });
        check(r.candidate.has_value(), "pickup/object combination round accepted");
        i.rosters = r.candidate->rosters;
        if (round == 0)
            check(pickups == 1 && object.counter == 1,
                  "same-round object advances1, no double pickup");
        if (round == 18)
            check(grants == 0, "old18 new19 does not grant early");
        if (round == 19)
            check(grants == 1, "twentieth admitted round grants exactly once");
    }
    check(pickups == 1 && grants == 1 && i.rosters[3].empty(),
          "new60 deletes object after single grant");
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
        death_then_victory();
        pickup_then_reward();
        errors();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
