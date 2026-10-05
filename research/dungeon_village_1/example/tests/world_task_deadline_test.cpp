#include "dungeon_village_reference/world_task_deadline.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
void page_rules() {
    WorldTaskDeadlinePageState state;
    auto result = prepare_world_task_deadline_page(
        state, {WorldTaskDeadlinePageAction::initialize, 0, 300, 1000, 4});
    check(result.candidate && result.candidate->state.initialized &&
              result.candidate->state.grade == 4 && result.candidate->state.counter == 0,
          "initialization computes display only without advancing");
    state = result.candidate->state;
    result = prepare_world_task_deadline_page(
        state, {WorldTaskDeadlinePageAction::confirm, 0, 300, 299, 0});
    check(result.candidate && result.candidate->insufficient_funds &&
              result.candidate->state.phase == 0 && result.candidate->state.returned == -1 &&
              !result.candidate->closed && result.candidate->state.grade == 4,
          "renew denied against displayed snapshot without closing or reinitializing");
    for (int choice : {0, 1}) {
        result = prepare_world_task_deadline_page(
            state, {WorldTaskDeadlinePageAction::confirm, choice, 300, choice ? -999 : 300, 0});
        check(result.candidate && result.candidate->state.phase == 1 &&
                  result.candidate->state.returned == choice &&
                  result.candidate->state.counter == 0 && !result.candidate->closed,
              "renew and abandon both record K and start animation without settling");
        auto animation = result.candidate->state;
        for (int frame = 1; frame <= 50; ++frame) {
            result = prepare_world_task_deadline_page(
                animation, {WorldTaskDeadlinePageAction::update, 0, 0, 0, 0});
            check(result.candidate && result.candidate->state.counter == frame &&
                      result.candidate->closed == (frame == 50),
                  "animation closes on50 admitted page updates");
            animation = result.candidate->state;
        }
    }
    state.phase = 1;
    state.returned = 1;
    state.counter = 12;
    result = prepare_world_task_deadline_page(
        state, {WorldTaskDeadlinePageAction::confirm, 0, 900, -1, 0});
    check(result.candidate && result.candidate->state.counter == 50 &&
              result.candidate->state.returned == 1 && !result.candidate->closed,
          "phase1 confirmation only fast forwards; cannot change decision or close yet");
    state = result.candidate->state;
    result = prepare_world_task_deadline_page(state, {});
    check(result.candidate && result.candidate->closed && result.candidate->state.counter == 51,
          "next update checks50 and closes before new input");
    state.counter = std::numeric_limits<int>::max() - 1;
    result = prepare_world_task_deadline_page(state, {});
    check(result.candidate && result.candidate->state.counter == 0,
          "generic page counter wraps moduloINTMAX safely");
    state.phase = 2;
    check(!prepare_world_task_deadline_page(state, {}).candidate,
          "invalid page phase rejects whole candidate");
}
WorldCalendarTasksState fixture(int funds = 1000) {
    WorldCalendarTasksState state;
    state.finish.dungeon.world.ai.accounting = PeriodAccounting(funds);
    state.finish.dungeon.world.ai.next_cash_id = 1;
    state.finish.active_task = 10;
    state.finish.participants = {1, 2, 1};
    state.finish.task_order = {10};
    state.human_details.emplace(1, CalendarTaskHuman{{2}, 100});
    state.human_details.emplace(2, CalendarTaskHuman{{99}, 300});
    state.deadline_page = 7;
    state.task_subperiods = 12;
    WorldScriptPage page;
    page.id = 7;
    page.kind = WorldScriptPageKind::raw_page;
    page.legacy_page = 33;
    page.lifecycle = 4;
    state.scripts.pages.push_back(page);
    state.random = WorldRandomStream::from_raw({9, 8, 7, 6, 5});
    return state;
}
WorldTaskDeadlineConsumer consumer(int fail_id = -1) {
    return
        [fail_id](const WorldCalendarTasksState &s,
                  const WorldTaskDeadlineEffect &effect) -> std::optional<WorldCalendarTasksState> {
            auto next = s;
            if (next.deadline_page || effect.id == fail_id)
                return {};
            const auto random = next.random.draw(10);
            if (random.error != WorldRandomError::none)
                return {};
            if (effect.kind == WorldTaskDeadlineEffectKind::event) {
                ++next.scripts.event_calls[effect.id];
                next.finish.event_calls = next.scripts.event_calls;
            } else if (effect.kind == WorldTaskDeadlineEffectKind::abort_task) {
                next.finish.active_task.reset();
                next.finish.task_order.clear();
            } else {
                next.scripts.notices.push_back({effect.id, -1, 80, "", ""});
            }
            return next;
        };
}
void main_result_rules() {
    const auto state = fixture();
    auto r = prepare_world_task_deadline_result(state, 0, 1, consumer());
    check(r.candidate && r.candidate->renewed && !r.candidate->aborted &&
              r.candidate->current_fee == 500 && !r.candidate->state.deadline_page &&
              r.candidate->state.task_subperiods == 0 &&
              r.candidate->state.finish.dungeon.world.ai.accounting.funds() == 500 &&
              r.candidate->state.finish.active_task == 10 &&
              r.candidate->state.finish.participants == state.finish.participants &&
              r.candidate->effects.size() == 1 && r.candidate->effects[0].id == 25,
          "main entry sums currentan with duplicates, resetsy, charges4 and notices25");
    check(state.random.draws() == 0 && state.deadline_page == 7 &&
              state.finish.dungeon.world.ai.accounting.funds() == 1000,
          "successful transaction never mutates supplied owner");
    auto negative = fixture();
    negative.human_details.at(1).continuation_cost = -100;
    negative.human_details.at(2).continuation_cost = 0;
    r = prepare_world_task_deadline_result(negative, 0, 1, consumer());
    check(r.candidate && r.candidate->current_fee == -200 &&
              r.candidate->state.finish.dungeon.world.ai.accounting.funds() == 1200,
          "source negative fee is preserved rather than clamped or recomputed from levels");
    auto zero = fixture();
    zero.human_details.at(1).continuation_cost = zero.human_details.at(2).continuation_cost = 0;
    r = prepare_world_task_deadline_result(zero, 0, 1, consumer());
    check(r.candidate && r.candidate->renewed &&
              r.candidate->state.finish.dungeon.world.ai.accounting.funds() == 1000,
          "zero fee still renews and notifies without inventing a cash delta");
    for (int choice : {0, 1}) {
        auto poor = fixture(499);
        poor.finish.dungeon.world.ai.pending_completion = 30;
        poor.scripts.pending_completion = 30;
        poor.popularity = 80;
        r = prepare_world_task_deadline_result(poor, choice, 1, consumer());
        check(r.candidate && r.candidate->aborted && !r.candidate->renewed &&
                  !r.candidate->state.finish.active_task &&
                  r.candidate->state.task_subperiods == 12 &&
                  r.candidate->state.finish.dungeon.world.ai.accounting.funds() == 499 &&
                  r.candidate->state.finish.dungeon.world.ai.pending_completion == 20 &&
                  r.candidate->state.scripts.pending_completion == 20 &&
                  r.candidate->state.popularity == 80 &&
                  r.candidate->state.random.draws() == static_cast<std::size_t>(choice ? 4 : 5),
              "abandon or late funds denial runs abort then80 thenpending-10 then162 then26");
        check(r.candidate->effects.front().id == (choice ? 0 : 11) &&
                  r.candidate->effects.back().id == 26,
              "late rejection executes11 before actual abort, voluntary abandon does not");
    }
    r = prepare_world_task_deadline_result(state, 0, 1, consumer(25));
    check(!r.candidate && r.error == WorldTaskDeadlineError::consumer_failed &&
              state.finish.dungeon.world.ai.accounting.funds() == 1000 && state.random.draws() == 0,
          "late notice failure rolls back fee and random owner");
    auto poor = fixture(0);
    r = prepare_world_task_deadline_result(poor, 1, 1, consumer(26));
    check(!r.candidate && poor.finish.active_task == 10 && poor.random.draws() == 0,
          "last abort notice failure discards prior scripts, task removal and draws");
    poor.random = WorldRandomStream::from_raw({0, 1});
    r = prepare_world_task_deadline_result(poor, 1, 1, consumer());
    check(!r.candidate && poor.random.draws() == 0 && poor.deadline_page == 7,
          "late random exhaustion rolls back main entry including aI");
    check(prepare_world_task_deadline_result(state, 0, 1).error ==
              WorldTaskDeadlineError::missing_consumer,
          "no default success consumer");
    auto invalid = state;
    invalid.scripts.pages[0].lifecycle = 2;
    check(prepare_world_task_deadline_result(invalid, 0, 1, consumer()).error ==
              WorldTaskDeadlineError::stale_page,
          "cannot settle before page actually closes");
    invalid = state;
    invalid.human_details.erase(2);
    check(prepare_world_task_deadline_result(invalid, 0, 1, consumer()).error ==
              WorldTaskDeadlineError::missing_human,
          "missing current fee definition rejects rather than treating as zero");
    invalid = state;
    invalid.human_details.at(1).continuation_cost = std::numeric_limits<int>::max();
    check(prepare_world_task_deadline_result(invalid, 0, 1, consumer()).error ==
              WorldTaskDeadlineError::overflow,
          "fee sum overflow is checked before any callbacks");
    auto outside = state;
    const auto original = outside.scripts.pages.front();
    outside.scripts.pages.clear();
    r = prepare_world_task_deadline_result(outside, 0, 1, consumer(), original);
    check(r.candidate && r.candidate->renewed && r.candidate->state.scripts.pages.empty(),
          "main entry accepts the original closed page root after framework removes it, without "
          "reinserting");
    check(prepare_world_task_deadline_result(outside, 0, 1, consumer()).error ==
              WorldTaskDeadlineError::stale_page,
          "removed page requires original outside root, not a guessed current page");
    auto financed = state;
    financed.scripts.finance = WorldScriptFinance{};
    financed.scripts.finance->cash = 1000;
    financed.scripts.finance->month = 3;
    r = prepare_world_task_deadline_result(financed, 0, 1, consumer());
    check(r.candidate && r.candidate->state.scripts.finance->cash == 500 &&
              r.candidate->state.scripts.finance->monthly_totals[3][4][1] == 500,
          "charged ledger and scriptfinance projection agree before callbacks and final writeback");
}
} // namespace
int main() {
    try {
        page_rules();
        main_result_rules();
        std::cout << "world_task_deadline: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_task_deadline: " << e.what() << '\n';
        return 1;
    }
}
