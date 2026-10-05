#include "ark/app/world_schedule.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace ark::app;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
// These are explicit protocol fixtures. Domain correctness belongs to Game/consumer tests.
std::optional<WorldScheduleStep> keep(const WorldScheduleState &state, const WorldScheduleCall &) {
    return WorldScheduleStep{state, false};
}
void first_arrival_and_queues() {
    WorldScheduleState state;
    state.hints = {{99, 19, 7}, {88, 3}};
    state.floating_notes = {{7, 15}, {2, 30}, {7, 10}};
    state.popularity_queue = {{1, 7, 1}, {2, 8, 1}, {0, 9, 2}};
    std::vector<int> popularity;
    bool old_empty{}, arrived_in_decision{};
    const auto result = prepare_world_schedule(state, {}, [&](const auto &input, const auto &call) {
        WorldScheduleStep step{input};
        if (call.stage == WorldScheduleStage::influence)
            old_empty = input.rosters[0].empty();
        if (call.stage == WorldScheduleStage::arrival_front)
            step.state.rosters[0].push_back(1);
        if (call.stage == WorldScheduleStage::popularity) {
            popularity.push_back((*call.popularity)[0]);
            check((*call.popularity)[1] == ((*call.popularity)[0] == 7 ? 1 : 0),
                  "show requires exact1");
        }
        if (call.stage == WorldScheduleStage::rescue_query)
            step.state.rescue_available = true;
        if (call.stage == WorldScheduleStage::decision) {
            arrived_in_decision = call.id == 1 && input.rescue_available;
            step.state.popularity_queue.insert(step.state.popularity_queue.begin(), {1, 20, 1});
        }
        return std::optional<WorldScheduleStep>(step);
    });
    check(result.candidate && old_empty && arrived_in_decision,
          "old influence then same-round new arrival decision");
    const auto &next = result.candidate->state;
    check(next.updates == 1 && next.hints == std::vector<std::vector<int>>{{88, 3}},
          "only first hint advances with multi-hint threshold20");
    check(next.floating_notes == std::vector<std::vector<int>>{{3, 30}},
          "notes advance backward and expire at8");
    check(popularity == std::vector<int>{9, 7} &&
              next.popularity_queue == std::vector<std::array<int, 3>>{{1, 20, 1}, {1, 8, 1}},
          "old popularity reversed; AI-created requests wait next round");
    check(result.candidate->calls.back().stage == WorldScheduleStage::finalize &&
              std::count_if(
                  result.candidate->calls.begin(), result.candidate->calls.end(),
                  [](const auto &c) { return c.stage == WorldScheduleStage::finalize; }) == 1,
          "final L exactly once");
}
void actor_order_and_delete() {
    WorldScheduleState state;
    state.rosters[0] = {1, 2};
    state.rosters[1] = {3, 4, 5};
    state.rosters[5] = {10};
    const auto result = prepare_world_schedule(state, {}, [](const auto &input, const auto &call) {
        return std::optional<WorldScheduleStep>(
            {input, call.stage == WorldScheduleStage::control && (call.id == 2 || call.id == 3)});
    });
    check(result.candidate.has_value(), "two-pass removal schedule accepted");
    std::vector<std::uint64_t> decisions, controls, tails, removals;
    for (const auto &call : result.candidate->calls) {
        if (call.stage == WorldScheduleStage::decision)
            decisions.push_back(*call.id);
        if (call.stage == WorldScheduleStage::control)
            controls.push_back(*call.id);
        if (call.stage == WorldScheduleStage::actor_tail)
            tails.push_back(*call.id);
        if (call.stage == WorldScheduleStage::remove_actor) {
            removals.push_back(*call.id);
            check(call.from_execution, "d removal exposes source release point");
        }
    }
    check(decisions == std::vector<std::uint64_t>{2, 1, 5, 4, 3}, "human/monster c reverse");
    check(controls == std::vector<std::uint64_t>{2, 1, 3, 5},
          "monster d forward removal skips shifted entry");
    check(tails == std::vector<std::uint64_t>{1, 5} && removals == std::vector<std::uint64_t>{2, 3},
          "true control bypasses tail before removal");
    check(result.candidate->state.rosters[0] == std::vector<std::uint64_t>{1} &&
              result.candidate->state.rosters[1] == std::vector<std::uint64_t>{4, 5},
          "protocol and live lists commit same removal");
    const auto &calls = result.candidate->calls;
    for (std::size_t n = 0; n < calls.size(); ++n)
        if (calls[n].stage == WorldScheduleStage::control)
            check(n >= 2 && calls[n - 1].stage == WorldScheduleStage::carry_expression &&
                      calls[n - 2].stage == WorldScheduleStage::execution_prefix,
                  "d common prefix and carry precede each control exactly once");
}
void ownership_and_errors() {
    struct Owner {
        WorldScheduleState schedule;
        int ledger{10};
        int random_cursor{};
    } owner;
    owner.schedule.rosters[0] = {1};
    OwnedWorldScheduleAdapter<Owner> adapter;
    adapter.read = [](const Owner &s) -> const WorldScheduleState & { return s.schedule; };
    adapter.write = [](Owner &s) -> WorldScheduleState & { return s.schedule; };
    adapter.consume =
        [](const Owner &s,
           const WorldScheduleCall &call) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        if (call.stage == WorldScheduleStage::finalize)
            return {};
        auto next = s;
        if (call.stage == WorldScheduleStage::control) {
            next.ledger += 30;
            ++next.random_cursor;
        }
        return OwnedWorldScheduleStep<Owner>{next};
    };
    const auto failed = prepare_owned_world_schedule(owner, {}, adapter);
    check(!failed.state && failed.error == WorldScheduleError::consumer_failed &&
              owner.ledger == 10 && owner.random_cursor == 0 && owner.schedule.updates == 0,
          "late L failure exposes no owner or random partial commit");
    check(prepare_owned_world_schedule(owner, {false}, adapter).state.has_value(),
          "not admitted performs no consumers");
    check(prepare_world_schedule({}, {}, {}).error == WorldScheduleError::missing_consumer,
          "no empty-success consumer fallback");
    for (const auto stage : {WorldScheduleStage::influence, WorldScheduleStage::decision_prefix,
                             WorldScheduleStage::finalize}) {
        const auto bad =
            prepare_world_schedule(owner.schedule, {}, [stage](const auto &s, const auto &call) {
                auto next = s;
                if (call.stage == stage)
                    next.rosters[0].clear();
                return std::optional<WorldScheduleStep>({next});
            });
        check(!bad.candidate && bad.error == WorldScheduleError::invalid_mutation,
              "common stage cannot rewrite live rosters");
    }
    WorldScheduleState state;
    state.popularity_queue = {{1, 10, 1}};
    auto bad = prepare_world_schedule(state, {}, [](const auto &s, const auto &call) {
        auto next = s;
        if (call.stage == WorldScheduleStage::popularity)
            next.popularity_queue.clear();
        return std::optional<WorldScheduleStep>({next});
    });
    check(!bad.candidate && bad.error == WorldScheduleError::invalid_mutation,
          "popularity consumer cannot destroy in-flight queue");
    state = {};
    state.updates = std::numeric_limits<int>::max() - 1;
    state.hints = {{7, 59, 6}};
    auto result = prepare_world_schedule(state, {}, keep);
    check(result.candidate && result.candidate->state.updates == 0 &&
              result.candidate->state.hints.empty(),
          "p wraps and single hint expires at60");
    check(prepare_world_schedule(state, {true, 1}, keep).error ==
              WorldScheduleError::dispatch_limit,
          "finite safety budget rejects entire schedule");
    state.hints = {{7}};
    check(prepare_world_schedule(state, {}, keep).error == WorldScheduleError::invalid_state,
          "truncated hint rejected");
}
void live_append_and_decision_removal() {
    WorldScheduleState state;
    state.rosters[0] = {1, 2};
    state.rosters[2] = {8};
    state.rosters[4] = {9};
    const auto result = prepare_world_schedule(state, {}, [](const auto &input, const auto &call) {
        auto next = input;
        if (call.stage == WorldScheduleStage::arrival_front) {
            next.rosters[2].clear();
            next.rosters[4].clear();
        }
        if (call.stage == WorldScheduleStage::decision && call.id == 2)
            next.rosters[0].push_back(3);
        return std::optional<WorldScheduleStep>(
            {next, call.stage == WorldScheduleStage::decision && call.id == 1});
    });
    check(result.candidate.has_value(), "arrival clearing and same-round domain append accepted");
    std::vector<std::uint64_t> decisions, controls;
    for (const auto &call : result.candidate->calls) {
        if (call.stage == WorldScheduleStage::decision)
            decisions.push_back(*call.id);
        if (call.stage == WorldScheduleStage::control)
            controls.push_back(*call.id);
        if (call.stage == WorldScheduleStage::remove_actor)
            check(!call.from_execution && call.id == 1, "c removal does not request d release");
    }
    check(decisions == std::vector<std::uint64_t>{2, 1} &&
              controls == std::vector<std::uint64_t>{3, 2},
          "reverse c append appears in later d; removed actor gets no d");
    check(result.candidate->state.rosters[0] == std::vector<std::uint64_t>{2, 3} &&
              result.candidate->state.rosters[2].empty() &&
              result.candidate->state.rosters[4].empty(),
          "live identity edits committed once");
}
} // namespace
int main() {
    try {
        first_arrival_and_queues();
        actor_order_and_delete();
        ownership_and_errors();
        live_append_and_decision_removal();
        std::cout << checks << " common world-schedule protocol checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
