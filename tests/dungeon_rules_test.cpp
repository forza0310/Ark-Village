// Adapted from published research 4ba4805; independent product build.
#include "ark/facilities/dungeon.hpp"
#include <algorithm>

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::facilities;
namespace people = ark::people;
namespace world = ark::world;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
DungeonCrewInput fixture(int count = 1) {
    DungeonCrewInput i;
    i.state.extent = 10000;
    i.active_task_extent = 10000;
    for (int n = 1; n <= count; ++n) {
        const people::ActorId id{static_cast<std::uint64_t>(n)};
        i.state.occupants.push_back(id);
        i.state.actors.emplace(id, DungeonActorProgress{});
    }
    return i;
}
void advance_and_cap() {
    for (int count = 1; count <= 8; ++count)
        for (int constrained = 0; constrained < count; ++constrained)
            for (int progress : {-700, 0, 350, 1000})
                for (int old_max : {0, 500, 2000}) {
                    auto i = fixture(count);
                    i.state.progress = old_max;
                    for (int n = 1; n <= count; ++n) {
                        auto &a = i.state.actors.at({static_cast<std::uint64_t>(n)});
                        a.progress = progress;
                        a.constrained = n > 1 && n <= constrained + 1;
                    }
                    const auto r = prepare_dungeon_crew(i);
                    check(r.candidate.has_value(), "legal signed expedition progress");
                    const auto &s = r.candidate->state;
                    const int step = 10 + constrained * 3;
                    const int leader = progress + (progress >= old_max ? step : 2 * step);
                    check(r.candidate->leader_step == step && s.actors.at({1}).progress == leader,
                          "contiguous OLD bw prefix determines leader bonus before clearing");
                    int cap = leader;
                    for (int n = 1; n <= count; ++n) {
                        const auto &a = s.actors.at({static_cast<std::uint64_t>(n)});
                        if (n > 1)
                            cap = std::min(progress + 20, cap - 300);
                        check(a.progress == cap && a.previous == progress && a.retreat_updates == 1,
                              "reverse ordinary advance then signed adjacent MIN cap");
                    }
                    const int raw_max = count == 1 ? leader : std::max(leader, progress + 20);
                    check(s.progress == std::max(old_max, std::max(0, raw_max)),
                          "crew g uses maximum BEFORE follower/challenge setbacks");
                }
    auto i = fixture(4);
    i.state.actors.at({2}).constrained = true;
    i.state.actors.at({4}).constrained = true;
    auto r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->leader_step == 13,
          "noncontiguous old constrained follower cannot add bonus");
    i = fixture();
    i.state.actors.at({1}).progress = 1000;
    i.state.extent = 2000;
    i.active_task_extent = 4000;
    r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->state.percent == 50 &&
              r.candidate->state.actors.at({1}).percent == 25,
          "display uses own extent, challenges use global active task extent");
    i.active_task_extent.reset();
    r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->state.actors.at({1}).percent == 0,
          "no global task makes actor/challenge percent0, not own extent fallback");
    i.active_task_extent = 0;
    r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->state.actors.at({1}).percent == 0,
          "zero extent returns interpolation origin even for positive progress");
}
void challenge_timing() {
    for (int count = 1; count <= 4; ++count)
        for (int strength = 1; strength <= 5; ++strength)
            for (int endurance = -1; endurance <= 3; ++endurance)
                for (int update : {24, 25, 26, 50}) {
                    auto i = fixture(count);
                    i.state.facility_updates = update;
                    i.state.progress = 2000;
                    for (auto &[id, a] : i.state.actors) {
                        (void)id;
                        a.progress = 2000;
                        a.endurance = endurance;
                    }
                    i.state.challenges = {{0, 1, 0, 17, 47, strength}};
                    const auto r = prepare_dungeon_crew(i);
                    check(r.candidate.has_value(), "monster ID47 is not reward category");
                    const auto &s = r.candidate->state;
                    const bool deplete = update % 25 == 0;
                    check(
                        s.challenges[0][5] == strength - (deplete ? count : 0),
                        "all eligible references consume challenge, even after strength reaches0");
                    check(s.challenges[0][2] == (deplete && strength <= count ? 1 : 0),
                          "challenge beaten during reverse member scan");
                    check(r.candidate->requests.size() ==
                              (deplete ? static_cast<unsigned>(count) : 0U),
                          "231 label emitted only on the actual modulo25 step");
                    for (int n = count; n >= 1; --n) {
                        const auto &a = s.actors.at({static_cast<std::uint64_t>(n)});
                        const int remaining = strength - (count - n + 1);
                        const bool beaten = deplete && remaining <= 0;
                        const bool retreat = deplete && !beaten && endurance <= 1;
                        check(
                            a.endurance == endurance - (deplete ? 1 : 0) &&
                                a.retreat_state == (retreat ? 1 : 0) &&
                                a.retreat_updates == (retreat ? 0 : 1),
                            "depletion/retreat ordered after strength check, no implicit HP loss");
                        if (!beaten && !retreat)
                            check(a.progress == 2000, "blocked challenge restores W, not capped V");
                    }
                    check(s.progress >= (count == 1 ? 2010 : 2020),
                          "global g remains ahead of blocked actors");
                }
    auto i = fixture();
    i.state.challenges = {{0, 0, 0, 5, 0, 9}, {0, 0, 0, 8, 1, 46}};
    auto r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->requests.size() == 2 &&
              r.candidate->requests[0].second == 46 && r.candidate->requests[1].second == 9 &&
              r.candidate->state.actors.at({1}).progress == 10,
          "reverse direct rewards turn2 but cannot block in same old-status0 branch");
    i.state = r.candidate->state;
    r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->requests.empty() &&
              r.candidate->state.actors.at({1}).progress == 10,
          "old-status2 reward blocks next step without granting again");
    for (int age = 0; age <= 42; ++age)
        for (int type : {0, 1})
            for (int status : {1, 2, 3}) {
                i = fixture();
                i.state.challenges = {{0, type, status, age, 0, 1}};
                r = prepare_dungeon_crew(i);
                const bool erase = status == 1 && age + 1 >= (type ? 40 : 10);
                check(r.candidate && r.candidate->state.challenges.empty() == erase,
                      "age increments BEFORE 10/40 removal thresholds");
                if (status == 2 && type == 0)
                    check(r.candidate->state.challenges[0][2] == (age + 1 >= 40 ? 3 : 2) &&
                              r.candidate->state.actors.at({1}).progress == 0,
                          "status2 blocks on transition step40 too");
                check(r.candidate->requests.size() == (erase && type == 0 ? 1U : 0U),
                      "only type0/status1 spawns thrown reward, not direct catalog grant");
            }
}
void retreat_and_completion() {
    auto i = fixture(3);
    i.state.occupants = {{1}, {2}, {1}, {3}};
    i.state.actors.at({1}).retreat_state = 1;
    i.state.actors.at({1}).retreat_updates = 19;
    i.retreats = {{{1}, true}};
    auto r = prepare_dungeon_crew(i);
    check(
        r.error == DungeonCrewError::unresolved_retreat && !r.candidate &&
            i.state.actors.at({1}).retreat_updates == 19,
        "duplicate reference is revisited after FIRST removal; missing second retreat rolls back");
    i.retreats.push_back({{1}, true});
    r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->consumed_retreats == 2 &&
              r.candidate->state.actors.at({1}).retreat_updates == 21 &&
              r.candidate->state.actors.at({2}).retreat_updates == 1 &&
              r.candidate->state.occupants == std::vector<people::ActorId>{{3}, {2}},
          "live index after first-reference removal repeats actor1, actor2 then becomes leader");
    i = fixture(3);
    i.state.actors.at({1}).progress = 1000;
    i.state.actors.at({2}).progress = 100;
    i.state.actors.at({3}).progress = 2000;
    i.state.actors.at({2}).retreat_state = 1;
    i.state.actors.at({2}).retreat_updates = 19;
    i.retreats = {{{2}, false}};
    r = prepare_dungeon_crew(i);
    check(r.candidate &&
              r.candidate->state.occupants == std::vector<people::ActorId>{{3}, {1}, {2}},
          "failed retreat still triggers strict pairwise progress reorder");
    i = fixture(3);
    i.state.extent = 10;
    i.state.percent = 72;
    r = prepare_dungeon_crew(i);
    check(r.candidate && r.candidate->completed && r.candidate->state.phase == 2 &&
              r.candidate->state.facility_updates == 0 &&
              r.candidate->state.previous_percent == 72 && r.candidate->state.percent == 100 &&
              r.candidate->requests.size() == 3 && r.candidate->requests[2].first == 10 &&
              r.candidate->state.occupants.size() == 3,
          "completion phases2 and queues original ordered exits; does not clear occupation early");
}
void strict_errors_and_entry() {
    for (int level = 1; level <= 10; ++level) {
        const auto a = prepare_dungeon_actor_entry({level, level, level});
        check(a && a->endurance == 3 + 3 * ((level - 1) / 2) && a->progress == 0 &&
                  a->retreat_updates == 0 && !a->constrained,
              "entry resets V/W/X/Z/aa and derives Y by per-profession integer truncation");
    }
    check(!dungeon_endurance({0}) && !dungeon_endurance({11}) && !dungeon_endurance({}),
          "entry contract rejects invalid/incomplete profession snapshots");
    auto i = fixture();
    i.state.actors.at({1}).progress = std::numeric_limits<int>::max();
    check(prepare_dungeon_crew(i).error == DungeonCrewError::numeric_overflow &&
              i.state.actors.at({1}).retreat_updates == 0,
          "advance overflow rejects without incrementing input counters");
    i = fixture(2);
    i.state.actors.at({1}).retreat_state = 2;
    i.state.actors.at({1}).progress = std::numeric_limits<int>::min();
    check(prepare_dungeon_crew(i).error == DungeonCrewError::numeric_overflow,
          "signed follower cap underflow is explicit maintenance error");
    i = fixture();
    i.state.facility_updates = 25;
    i.state.challenges = {{0, 1, 0, 0, 9, std::numeric_limits<int>::min()}, {0, 0, 0, 0, 0, 1}};
    check(!prepare_dungeon_crew(i).candidate && i.state.challenges[1][2] == 0,
          "late challenge overflow returns no partial reward state");
    i = fixture();
    i.state.challenges = {{0, 0, 0, 0, 4, 1}};
    check(prepare_dungeon_crew(i).error == DungeonCrewError::invalid_input,
          "type0 reward category4 remains invalid; no weakened reward validation");
}
void extent_and_completion() {
    for (int q : {1, 21, 80, 100})
        for (int difficulty = 1; difficulty <= 9; ++difficulty)
            check(dungeon_extent(difficulty, q) == q * 120 + (difficulty - 1) * q * 120 / 8,
                  "extent uses current calendarQ and per-difficulty integer interpolation");
    check(!dungeon_extent(0, 80) && !dungeon_extent(10, 80) &&
              !dungeon_extent(1, std::numeric_limits<int>::max()),
          "invalid extent inputs/Java arithmetic overflow rejected explicitly");
    DungeonCompletionInput i;
    i.source_site = {4, 2};
    i.site_tasks = {{0, {}}, {0, world::Cell{4, 2}}, {0, world::Cell{4, 3}}, {1, {}}};
    for (int update = 0; update <= 12; ++update) {
        i.updates = update;
        const auto r = prepare_dungeon_completion(i);
        check(r && r->requests.size() == (update < 10 ? 0U : 3U),
              "phase2 settles only after post-prefix count10");
        if (update >= 10)
            check(r->requests[0].kind == DungeonCompletionRequestKind::restore_site &&
                      r->requests[0].first == 0 && r->requests[1].first == 1 &&
                      r->requests[2].first == 0,
                  "no active task restores unsuccessful site then reverse-removes null/same kind0");
    }
    i.active_task = DungeonCompletionTask{1, 0, {}, {}};
    auto r = prepare_dungeon_completion(i);
    check(r && r->requests.size() == 1 &&
              r->requests[0].kind == DungeonCompletionRequestKind::clear_active_task &&
              r->consumed_summary_tickets == 0,
          "kind1 only clears active task, no fabricated XP/summary/site mutation");
    i.active_task = DungeonCompletionTask{
        0, 3, {7, 9, 7}, {{5, 0, 3, 50, 0, 2}, {9, 1, 0, 0, 47, 5}, {97, 0, 0, 0, 1, 46}}};
    i.human_order = {{{2}, 7}, {{1}, 7}};
    i.active_task->definition = 8;
    i.active_task->pending_completion_value = 17;
    i.summary_tickets = {0, 1};
    r = prepare_dungeon_completion(i);
    check(r && r->requests.size() == 15 &&
              r->summary_rewards == std::vector<std::array<int, 2>>{{0, 2}, {1, 46}} &&
              r->consumed_summary_tickets == 2,
          "global challenges feed remaining type0 summary regardless status, not local "
          "rewards/grants");
    check(r->requests[0].actor == people::ActorId{2} && r->requests[0].first == -8 &&
              r->requests[1].first == 20 && r->requests[1].second == 15 &&
              r->requests[3].actor == people::ActorId{2} &&
              r->requests[6].kind == DungeonCompletionRequestKind::summary30 &&
              r->requests[7].first == 7 && r->requests[7].second == 9,
          "duplicate participants reward first live actor twice; absent9 skipped but selectable "
          "summary");
    check(r->requests[8].kind == DungeonCompletionRequestKind::event126 &&
              r->requests[9].kind == DungeonCompletionRequestKind::record_complete &&
              r->requests[10].kind == DungeonCompletionRequestKind::restore_site &&
              r->requests[10].first == 1 && r->requests[9].first == 17 &&
              r->requests[11].kind == DungeonCompletionRequestKind::record_task_success &&
              r->requests[11].first == 8 &&
              r->requests[12].kind == DungeonCompletionRequestKind::clear_active_task,
          "completion preserves event/pending-value/site/success-statistics/global-reset order");
    i.event201_seen = i.event92_seen = true;
    r = prepare_dungeon_completion(i);
    check(r && r->requests.size() == 13, "seen completion/tutorial events not repeated");
    i.active_task->definition = 0;
    i.active_task->pending_completion_value = 0;
    r = prepare_dungeon_completion(i);
    check(r && r->requests[9].first == 0 && r->requests[11].first == 0,
          "zero original task ID and zero pending value remain valid");
    i.active_task->definition = -1;
    check(!prepare_dungeon_completion(i), "negative original task identity rejects whole result");
    i.active_task->definition = 0;
    i.active_task->pending_completion_value = -1;
    check(!prepare_dungeon_completion(i), "negative pending completion value rejects whole result");
    i.active_task->pending_completion_value = 0;
    i.summary_tickets = {0};
    check(!prepare_dungeon_completion(i),
          "missing second summary draw cannot commit partial rewards");
    check(prepare_dungeon_actor_entry({1}, true)->constrained,
          "occupation must preserve previous bw, not reset it with V/W/X/Z/aa");
}
} // namespace
int main() {
    try {
        advance_and_cap();
        challenge_timing();
        retreat_and_completion();
        strict_errors_and_entry();
        extent_and_completion();
        std::cout << checks << " dungeon AI checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
