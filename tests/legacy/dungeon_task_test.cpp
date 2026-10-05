#include "ark/facilities/dungeon.hpp"
#include "ark/facilities/dungeon_task.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::facilities;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
// Explicit test owner, not a claim about new-game task catalogue or statistics.
DungeonTaskSuccessState fixture() {
    DungeonTaskSuccessState state;
    state.definitions.emplace(0, DungeonTaskDefinitionProgress{0, 0, 2, 7});
    state.monsters.emplace(7, DungeonMonsterAvailability{0, false});
    state.task_pool_progress = 290;
    return state;
}
void pool_thresholds() {
    for (int kind : {0, 1})
        for (int old = 240; old <= 520; ++old) {
            auto state = fixture();
            state.definitions.at(0).kind = kind;
            state.task_pool_progress = old;
            const auto result = prepare_dungeon_task_success(state, 0, 1, 4);
            const int amount = kind == 0 ? 50 : 100;
            std::vector<int> notices;
            for (int index = 0; index < 3; ++index)
                if (old < 300 + 100 * index && old + amount >= 300 + 100 * index)
                    notices.push_back(29 + index);
            check(result && result->state.successes == 1 &&
                      result->state.ordinary_explorations == (kind == 0 ? 1 : 0) &&
                      result->state.definitions.at(0).completed == 3 &&
                      result->state.task_pool_progress == old + amount &&
                      result->threshold_notice_ids == notices,
                  "kind-specific increments and exact threshold crossings");
            check(state.successes == 0 && state.definitions.at(0).completed == 2 &&
                      state.task_pool_progress == old,
                  "successful candidate does not mutate its owner input");
        }
}
void flags_and_dates() {
    for (int stage = 0; stage <= 5; ++stage)
        for (int month : {0, 11}) {
            auto state = fixture();
            state.definitions.at(0).flags = 2U | 4U | 8U;
            state.remaining_task_definitions = {0};
            state.exploration_stage = stage;
            state.exploration_dates[6] = {99, 9};
            const auto result = prepare_dungeon_task_success(state, 0, 2, month);
            check(
                result && result->state.exploration_dates[stage] == std::array<int, 2>{2, month} &&
                    result->state.exploration_dates[6] == std::array<int, 2>{99, 9} &&
                    result->state.exploration_stage == std::min(stage + 1, 5) &&
                    result->state.ordinary_explorations == 0 &&
                    result->state.monsters.at(7).status == 1 &&
                    result->state.monsters.at(7).pending_notice &&
                    result->state.task_pool_progress == 290 && result->threshold_notice_ids.empty(),
                "special task writes original date before suppressing pool; flag8 excludes w");
        }
    for (int status : {0, 1, 2})
        for (bool pending : {false, true}) {
            auto state = fixture();
            state.definitions.at(0).flags = 4U;
            state.monsters.at(7) = {status, pending};
            const auto result = prepare_dungeon_task_success(state, 0, 0, 0);
            check(result && result->state.monsters.at(7).status == 1 &&
                      result->state.monsters.at(7).pending_notice == (status == 0 || pending),
                  "monster availability overwrite preserves existing pending notice");
        }
    auto state = fixture();
    auto result = prepare_dungeon_task_success(state, 0, 0, 0);
    check(result.has_value(), "zero task identity and original zero year/month are legal");
    result = prepare_dungeon_task_success(result->state, 0, 0, 0);
    check(result && result->state.successes == 2 && result->state.definitions.at(0).completed == 4,
          "repeated calls increment again; no invented idempotency barrier");
}
void ordered_completion_handoff() {
    DungeonCompletionInput input;
    input.updates = 10;
    input.source_site = {3, 4};
    input.active_task = DungeonCompletionTask{0, 1, {5}, {}};
    input.active_task->definition = 0;
    input.summary_tickets = {0, 0};
    const auto completion = prepare_dungeon_completion(input);
    check(completion.has_value(), "phase2 provides actual task-success request");
    const auto &requests = completion->requests;
    const auto success = std::find_if(requests.begin(), requests.end(), [](const auto &request) {
        return request.kind == DungeonCompletionRequestKind::record_task_success;
    });
    check(success != requests.end() && success + 1 != requests.end() &&
              (success + 1)->kind == DungeonCompletionRequestKind::clear_active_task,
          "success consumer runs before clearing the active task");
    auto state = fixture();
    state.definitions.at(0).flags = 2U;
    state.remaining_task_definitions = {0};
    const auto result = prepare_dungeon_task_success(state, success->first, 3, 6);
    check(result && result->state.task_pool_progress == 290 &&
              result->state.exploration_dates[0] == std::array<int, 2>{3, 6} &&
              result->state.remaining_task_definitions == std::vector<int>{0},
          "request consumes current task owner without prematurely clearing bq");
}
void rollback_and_contracts() {
    auto state = fixture();
    state.definitions.at(0).flags = 2U | 4U;
    state.remaining_task_definitions = {99};
    check(!prepare_dungeon_task_success(state, 0, 1, 0) && state.successes == 0 &&
              state.exploration_stage == 0 && state.exploration_dates[0][0] == 0 &&
              state.monsters.at(7).status == 0 && !state.monsters.at(7).pending_notice,
          "late missing reference rolls back counters, dates and monster opening");
    state.remaining_task_definitions.clear();
    state.monsters.clear();
    check(!prepare_dungeon_task_success(state, 0, 1, 0),
          "flag4 requires the actual referenced monster definition");
    state = fixture();
    state.monsters.clear();
    check(prepare_dungeon_task_success(state, 0, 1, 0).has_value(),
          "unconsumed monster definition is not fabricated or required");
    const int maximum = std::numeric_limits<int>::max();
    for (int field = 0; field < 4; ++field) {
        state = fixture();
        int *target = field == 0   ? &state.successes
                      : field == 1 ? &state.ordinary_explorations
                      : field == 2 ? &state.definitions.at(0).completed
                                   : &state.task_pool_progress;
        *target = maximum;
        check(!prepare_dungeon_task_success(state, 0, 1, 0) && *target == maximum,
              "overflow rejects the entire candidate");
    }
    state = fixture();
    state.task_pool_progress = maximum;
    state.definitions.at(0).flags = 2U;
    state.remaining_task_definitions = {0, 99};
    check(prepare_dungeon_task_success(state, 0, 1, 0).has_value(),
          "source short circuit skips later references and suppressed G arithmetic");
    state = fixture();
    for (int kind : {-1, 2}) {
        state.definitions.at(0).kind = kind;
        check(!prepare_dungeon_task_success(state, 0, 1, 0), "only researched task kinds0/1");
    }
    state = fixture();
    check(!prepare_dungeon_task_success(state, -1, 1, 0) &&
              !prepare_dungeon_task_success(state, 99, 1, 0) &&
              !prepare_dungeon_task_success(state, 0, -1, 0) &&
              !prepare_dungeon_task_success(state, 0, 1, -1) &&
              !prepare_dungeon_task_success(state, 0, 1, 12),
          "missing identities and non-source dates are rejected");
    for (int stage : {-1, 6}) {
        state.exploration_stage = stage;
        check(!prepare_dungeon_task_success(state, 0, 1, 0), "stage constrained to known0..5");
    }
}
} // namespace
int main() {
    try {
        pool_thresholds();
        flags_and_dates();
        ordered_completion_handoff();
        rollback_and_contracts();
        std::cout << checks << " dungeon task-success checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
