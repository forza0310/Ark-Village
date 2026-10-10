#include "ark/simulation/tasks/rules/world_task_display.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
const WorldTaskDisplayCandidate &success(const WorldTaskDisplayResult &result) {
    check(result.error == WorldTaskDisplayError::none && result.candidate.has_value(),
          "page prepare succeeded");
    return *result.candidate;
}
WorldTaskDisplayState waiting() {
    WorldTaskDisplayState state;
    state.legacy_page = 100;
    state.initialized = true;
    for (auto &row : state.bd) {
        row[0] = 10000;
        row[4] = 1000;
        row[5] = 2000;
        row[6] = 16000;
        row[7] = 1;
    }
    return state;
}
WorldTaskDisplayResult prepare(const WorldTaskDisplayState &state, WorldTaskDisplayAction action,
                               const WorldRandomStream &random, int counter = 0) {
    return prepare_world_task_display_page(state, {action, counter}, random);
}
void notice_page() {
    WorldTaskDisplayState state;
    const auto random = WorldRandomStream::from_raw({});
    for (const auto action : {WorldTaskDisplayAction::initialize, WorldTaskDisplayAction::update,
                              WorldTaskDisplayAction::confirm}) {
        const auto result = prepare(state, action, random, 39);
        const auto &c = success(result);
        check(c.state.initialized && c.state.bd == state.bd && c.random.draws() == 0 && !c.closed,
              "99 never runs E/F, consumes no random and cannot confirm before40");
    }
    const auto close = prepare(state, WorldTaskDisplayAction::confirm, random, 40);
    check(success(close).closed && close.candidate->random.draws() == 0,
          "99 closes at40 without animation or fast forward");
    const auto update = prepare(state, WorldTaskDisplayAction::update, random, 100);
    check(!success(update).closed, "99 time alone does not dismiss the notice");
}
void exact_initialization() {
    WorldTaskDisplayState state;
    state.legacy_page = 100;
    for (auto &row : state.bd) {
        row[1] = 12345;
        row[2] = 1;
        row[3] = 777;
        row[4] = 888;
        row[8] = 456;
    }
    // 前三行每行位置/下界/停留，后五行每行边界侧/下界。
    const auto random = WorldRandomStream::from_raw(
        {0, 0, 0, 99, 99, 99, 50, 50, 50, 49, 1, 50, 2, 0, 3, 99, 4, 10, 5});
    const auto result = prepare(state, WorldTaskDisplayAction::initialize, random);
    const auto &c = success(result);
    check(c.random.draws() == 19 && c.random.raw_cursor() == 19 && c.state.initialized,
          "E consumes exactly19 original nextInt tickets");
    check(c.state.bd[0][0] == 2000 && c.state.bd[0][5] == 2000 && c.state.bd[0][4] == 5,
          "first row keeps endpoint interpolation");
    check(c.state.bd[1][0] == 15000 && c.state.bd[1][5] == 16000 && c.state.bd[1][4] == 60,
          "second row offset and upper endpoint are exact");
    check(c.state.bd[2][0] == 19030 && c.state.bd[2][5] == 9070 && c.state.bd[2][4] == 32,
          "integer division truncates before adding start");
    const int positions[]{2000, 22000, 2000, 22000, 2000};
    for (std::size_t i = 0; i < c.state.bd.size(); ++i) {
        const auto &row = c.state.bd[i];
        check(row[1] == 12345 && row[2] == 1 && row[8] == 456,
              "E preserves destination direction and speed from the shared table");
        check(row[3] == 0 && row[6] == row[5] + 6000 && row[7] == (i < 3 ? 1 : 0),
              "E resets only source-written age bounds and state");
        if (i >= 3)
            check(row[0] == positions[i - 3] && row[4] == static_cast<int>(i) * 10,
                  "later rows use strict50 threshold and index delay");
    }
    const auto again = prepare(c.state, WorldTaskDisplayAction::initialize, c.random);
    check(success(again).state.bd == c.state.bd && again.candidate->random.draws() == 19,
          "reinitializing the same page does not rerun E");
    check(state.bd[0][3] == 777 && random.draws() == 0,
          "successful candidate does not mutate caller state or random owner");
}
void transitions_and_order() {
    auto state = waiting();
    state.bd[0][7] = 0;
    state.bd[0][3] = 8;
    state.bd[0][4] = 10;
    state.bd[1][3] = 9;
    state.bd[1][4] = 10;
    state.bd[1][0] = 9000;
    state.bd[1][5] = 2000;
    state.bd[1][6] = 16000;
    state.bd[2][3] = 0;
    state.bd[2][4] = 0;
    const auto random = WorldRandomStream::from_raw({99, 0, 0, 99});
    const auto result = prepare(state, WorldTaskDisplayAction::update, random);
    const auto &c = success(result);
    check(c.state.bd[0][7] == 0 && c.state.bd[0][3] == 9 && c.random.draws() == 4,
          "age increments first but row0 waits strictly below duration");
    check(c.state.bd[1][0] == 9000 && c.state.bd[1][1] == 16000 && c.state.bd[1][2] == 0 &&
              c.state.bd[1][3] == 10 && c.state.bd[1][4] == 10 && c.state.bd[1][7] == 2 &&
              c.state.bd[1][8] == 30,
          "f2 uses destination then speed without clearing age duration or moving this frame");
    check(c.state.bd[2][0] == 10000 && c.state.bd[2][1] == 2000 && c.state.bd[2][2] == 1 &&
              c.state.bd[2][3] == 1 && c.state.bd[2][8] == 200,
          "later row consumes the next pair of tickets in vector order");
    auto equal = waiting();
    equal.bd[0][0] = 2000;
    equal.bd[0][4] = 1;
    const auto equality =
        prepare(equal, WorldTaskDisplayAction::update, WorldRandomStream::from_raw({0, 0}));
    check(success(equality).state.bd[0][2] == 1,
          "equal position and destination select direction1");
}
void movement_priority() {
    auto state = waiting();
    state.bd[0] = {21990, 22000, 0, 3, 20, 2000, 16000, 2, 30};
    state.bd[1] = {21990, 23000, 0, 3, 20, 2000, 16000, 2, 30};
    state.bd[2] = {2010, 2000, 1, 3, 20, 2000, 16000, 2, 30};
    state.bd[3] = {2010, 1000, 1, 3, 20, 2000, 16000, 2, 30};
    const auto result =
        prepare(state, WorldTaskDisplayAction::update, WorldRandomStream::from_raw({0, 99}));
    const auto &c = success(result);
    check(c.state.bd[0][0] == 22020 && c.state.bd[0][7] == 1 && c.state.bd[0][2] == 0 &&
              c.state.bd[0][3] == 0 && c.state.bd[0][4] == 5,
          "right destination takes priority over boundary without clamping overshoot");
    check(c.state.bd[1][0] == 22020 && c.state.bd[1][7] == 2 && c.state.bd[1][2] == 1 &&
              c.state.bd[1][3] == 4,
          "right boundary only flips direction and does not consume random");
    check(c.state.bd[2][0] == 1980 && c.state.bd[2][7] == 1 && c.state.bd[2][2] == 1 &&
              c.state.bd[2][3] == 0 && c.state.bd[2][4] == 60,
          "left destination has the same priority and consumes the next stop ticket");
    check(c.state.bd[3][0] == 1980 && c.state.bd[3][7] == 2 && c.state.bd[3][2] == 0 &&
              c.random.draws() == 2,
          "left boundary only flips direction");
}
void confirm_and_automatic_init() {
    auto state = waiting();
    state.bd[0][4] = 0;
    const auto random = WorldRandomStream::from_raw({});
    for (const int counter : {0, 39, 40, 100}) {
        const auto result = prepare(state, WorldTaskDisplayAction::confirm, random, counter);
        const auto &c = success(result);
        check(c.state.bd == state.bd && c.random.draws() == 0 && c.closed == (counter >= 40),
              "confirm checks40 only and never duplicates this frame's F or fast-forwards");
    }
    state.initialized = false;
    std::vector<std::int32_t> tape(19, 0);
    const auto initialize_random = WorldRandomStream::from_raw(tape);
    const auto update = prepare(state, WorldTaskDisplayAction::update, initialize_random);
    check(success(update).state.bd[0][3] == 1 && update.candidate->random.draws() == 19,
          "first update performs E followed by one F");
    const auto confirm = prepare(state, WorldTaskDisplayAction::confirm, initialize_random, 40);
    check(success(confirm).state.bd[0][3] == 0 && confirm.candidate->random.draws() == 19 &&
              confirm.candidate->closed,
          "first confirm may initialize but still does not update");
}
void rollback_and_validation() {
    auto state = waiting();
    state.initialized = false;
    const auto short_tape = WorldRandomStream::from_raw(std::vector<std::int32_t>(18, 0));
    const auto init = prepare(state, WorldTaskDisplayAction::initialize, short_tape);
    check(init.error == WorldTaskDisplayError::random_failed && !init.candidate &&
              !state.initialized && state.bd[7][0] == 10000 && short_tape.draws() == 0,
          "late E exhaustion discards every earlier row and ticket");
    state.initialized = true;
    state.bd[0][4] = 1;
    state.bd[7][4] = 1;
    const auto late_tape = WorldRandomStream::from_raw({99, 99, 99});
    const auto update = prepare(state, WorldTaskDisplayAction::update, late_tape);
    check(update.error == WorldTaskDisplayError::random_failed && !update.candidate &&
              state.bd[0][7] == 1 && state.bd[7][3] == 0 && late_tape.draws() == 0,
          "late F exhaustion also rolls back whole table and cursor");
    const auto assert_invalid = [&](WorldTaskDisplayState invalid, WorldTaskDisplayInput input) {
        const auto result = prepare_world_task_display_page(invalid, input, late_tape);
        check(result.error == WorldTaskDisplayError::invalid_input && !result.candidate,
              "invalid input is rejected without a candidate");
    };
    auto invalid = waiting();
    invalid.legacy_page = 98;
    assert_invalid(invalid, {});
    invalid = waiting();
    assert_invalid(invalid, {WorldTaskDisplayAction::update, -1});
    assert_invalid(invalid, {static_cast<WorldTaskDisplayAction>(99), 0});
    for (const auto field : {2, 3, 4, 7, 8}) {
        invalid = waiting();
        invalid.bd[7][field] = -1;
        assert_invalid(invalid, {});
    }
    invalid = waiting();
    invalid.bd[7][7] = 3;
    assert_invalid(invalid, {});
    invalid = waiting();
    invalid.bd[7][2] = 2;
    assert_invalid(invalid, {});
    constexpr auto minimum = std::numeric_limits<std::int32_t>::min();
    constexpr auto maximum = std::numeric_limits<std::int32_t>::max();
    for (const auto field : {0, 3}) {
        auto overflow = waiting();
        overflow.bd[7][field] = maximum;
        if (field == 0) {
            overflow.bd[7][7] = 2;
            overflow.bd[7][8] = 1;
        }
        const auto result =
            prepare(overflow, WorldTaskDisplayAction::update, WorldRandomStream::from_raw({}));
        check(result.error == WorldTaskDisplayError::overflow && !result.candidate &&
                  overflow.bd[0][3] == 0,
              "late age or right motion overflow preserves all earlier rows");
    }
    auto left = waiting();
    left.bd[0] = {minimum, minimum, 1, 0, 100, 2000, 16000, 2, 1};
    const auto underflow = prepare(left, WorldTaskDisplayAction::update, late_tape);
    check(underflow.error == WorldTaskDisplayError::overflow && !underflow.candidate,
          "left motion checks signed underflow");
    auto wide = waiting();
    wide.bd[0][4] = 1;
    wide.bd[0][5] = minimum;
    wide.bd[0][6] = maximum;
    const auto extreme =
        prepare(wide, WorldTaskDisplayAction::update, WorldRandomStream::from_raw({99, 0}));
    check(success(extreme).state.bd[0][1] == maximum,
          "wide interval subtraction is promoted before interpolation, without signed UB");
    wide.bd[0][5] = maximum;
    wide.bd[0][6] = minimum;
    const auto reverse =
        prepare(wide, WorldTaskDisplayAction::update, WorldRandomStream::from_raw({99, 0}));
    check(success(reverse).state.bd[0][1] == minimum,
          "descending interval preserves source interpolation rather than inventing a guard");
}
void bounded_seeded_replay() {
    WorldTaskDisplayState a;
    a.legacy_page = 100;
    auto b = a;
    auto random_a = WorldRandomStream::from_java_seed(42);
    auto random_b = random_a;
    for (int counter = 0; counter < 600; ++counter) {
        const auto result_a = prepare(a, WorldTaskDisplayAction::update, random_a, counter);
        const auto result_b = prepare(b, WorldTaskDisplayAction::update, random_b, counter);
        const auto &ca = success(result_a);
        const auto &cb = success(result_b);
        check(ca.state.bd == cb.state.bd && ca.random.draws() == cb.random.draws() && !ca.closed,
              "bounded seeded E/F replay stays deterministic and does not auto-close");
        for (const auto &row : ca.state.bd)
            check(row[3] >= 0 && row[4] >= 0 && row[7] >= 0 && row[7] <= 2,
                  "seeded animation retains valid counters and phases");
        a = ca.state;
        b = cb.state;
        random_a = ca.random;
        random_b = cb.random;
    }
    check(random_a.draws() > 19 && random_a.draw(100).raw == random_b.draw(100).raw,
          "replayed global cursor includes animation transitions and agrees on next raw");
}
} // namespace
int main() {
    try {
        notice_page();
        exact_initialization();
        transitions_and_order();
        movement_priority();
        confirm_and_automatic_init();
        rollback_and_validation();
        bounded_seeded_replay();
        std::cout << "world_task_display: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_task_display: " << e.what() << '\n';
        return 1;
    }
}
