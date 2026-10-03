#include "dungeon_village_reference/facility_events.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

using namespace dungeon_village_reference;

namespace {

int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

FacilityEventTransition transition(const FacilityEventResult &result) {
    check(result.error == FacilityEventError::none && result.transition.has_value(),
          "valid event transition produces a complete candidate");
    return *result.transition;
}

bool same(const FacilityEventCounters &left, const FacilityEventCounters &right) {
    return left.item_confirmations == right.item_confirmations &&
           left.months_since_trigger == right.months_since_trigger;
}

void gates_and_order() {
    for (int icon = 0; icon <= 5; ++icon) {
        for (std::int64_t items = 0; items <= 6; ++items) {
            for (std::int64_t months = 34; months <= 38; ++months) {
                const FacilityEventCounters input{items, months};
                const auto output = transition(confirm_facility_item(input, icon));
                const bool expected = (icon == 2 || icon == 3) && items >= 4 && months >= 36;
                check(output.script_triggered == expected, "inclusive gate checked after item");
                check(same(output.counters, expected ? FacilityEventCounters{}
                                                     : FacilityEventCounters{items + 1, months}),
                      "trigger resets both instance counters, other icons preserve age");
                check(same(input, {items, months}), "source counters unchanged");
                const auto monthly = transition(advance_facility_event_months(input, 2));
                check(!monthly.script_triggered && same(monthly.counters, {items, months + 2}),
                      "month crossing never evaluates or resets event gate");
            }
        }
    }
    auto counters = FacilityEventCounters{};
    for (int index = 0; index < 5; ++index) {
        const auto result = transition(confirm_facility_item(counters, 2));
        check(!result.script_triggered, "items before maturity do not trigger");
        counters = result.counters;
    }
    const auto mature = transition(advance_facility_event_months(counters, 36));
    check(!mature.script_triggered && same(mature.counters, {5, 36}),
          "maturity alone does not trigger even with five prior items");
    const auto triggered = transition(confirm_facility_item(mature.counters, 2));
    check(triggered.script_triggered && same(triggered.counters, {}),
          "sixth confirmation triggers after months arrived last");
    const auto again = transition(confirm_facility_item(triggered.counters, 2));
    check(!again.script_triggered && same(again.counters, {1, 0}),
          "fresh cycle requires rebuilding both counters");
    const auto aged = transition(advance_facility_event_months({}, 36));
    counters = aged.counters;
    for (int index = 1; index <= 5; ++index) {
        const auto result = transition(confirm_facility_item(counters, 3));
        check(result.script_triggered == (index == 5), "months-first fifth item triggers");
        counters = result.counters;
    }
    const FacilityEventCounters other_instance{4, 35};
    check(same(other_instance, {4, 35}), "one instance reset does not reset another");
}

void plan_structure() {
    for (std::int32_t id = 0; id < 85; ++id) {
        for (const int icon : {2, 3}) {
            const auto result = inspect_facility_legend_plan(id, icon, {{6, 100}, {40, id}});
            check(result.status == FacilityPlanStatus::supported && result.plan.has_value(),
                  "complete supported structure produces a plan");
            check(result.plan->definition_id == id && result.plan->delay_ticks == 100 &&
                      result.plan->mode == (icon == 2 ? LegendPresentationMode::legacy_icon_2
                                                      : LegendPresentationMode::legacy_icon_3),
                  "plan keeps definition identity, logical delay and mode distinct");
        }
    }
    for (const int delay : {0, 1, std::numeric_limits<std::int32_t>::max()}) {
        const auto result = inspect_facility_legend_plan(36, 2, {{6, delay}, {40, 36}});
        check(result.status == FacilityPlanStatus::supported && result.plan->delay_ticks == delay,
              "nonnegative delay has no invented frame or seconds conversion");
    }
    for (const auto &program : {FacilityEventProgram{{}},
                                {{6}},
                                {{40, 36}},
                                {{6, -1}, {40, 36}},
                                {{6, 100}, {40, 35}},
                                {{6, 100, 0}, {40, 36}},
                                {{6, 100}, {40, 36, 0}},
                                {{7, 100}, {40, 36}},
                                {{6, 100}, {41, 36}},
                                {{40, 36}, {6, 100}},
                                {{6, 100}, {40, 36}, {777, 1}}}) {
        const auto original = program;
        const auto result = inspect_facility_legend_plan(36, 2, program);
        check(result.status == FacilityPlanStatus::unsupported && !result.plan,
              "unknown or incomplete plan is not partially executed");
        check(program == original, "unsupported matrix retained unchanged");
    }
    for (const int icon : {0, 1, 4, 100}) {
        const auto result = inspect_facility_legend_plan(36, icon, {{6, 100}, {40, 36}});
        check(result.status == FacilityPlanStatus::unsupported && !result.plan,
              "other icons cannot use the limited presentation plan");
    }
    check(inspect_facility_legend_plan(36, 2, {}).status == FacilityPlanStatus::no_script,
          "empty script explicit, not supported presentation");
    check(inspect_facility_legend_plan(-1, 2, {}).status == FacilityPlanStatus::invalid_input &&
              inspect_facility_legend_plan(36, -1, {}).status == FacilityPlanStatus::invalid_input,
          "invalid identity and icon rejected before empty-script handling");
    const auto result = transition(confirm_facility_item({4, 36}, 2));
    check(result.script_triggered &&
              inspect_facility_legend_plan(36, 2, {}).status == FacilityPlanStatus::no_script,
          "counter trigger does not imply a populated script or immediate popularity award");
}

void invalid_and_overflow() {
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    for (const auto counters : {FacilityEventCounters{-1, 0}, FacilityEventCounters{0, -1}}) {
        const auto item = confirm_facility_item(counters, 2);
        const auto month = advance_facility_event_months(counters, 0);
        check(item.error == FacilityEventError::invalid_input && !item.transition &&
                  month.error == FacilityEventError::invalid_input && !month.transition,
              "negative state has no partial candidate");
    }
    check(confirm_facility_item({}, -1).error == FacilityEventError::invalid_input &&
              advance_facility_event_months({}, -1).error == FacilityEventError::invalid_input,
          "negative input rejected");
    const auto item_overflow = confirm_facility_item({maximum, 36}, 2);
    check(item_overflow.error == FacilityEventError::numeric_overflow && !item_overflow.transition,
          "item increment overflow rejected before gate reset");
    const auto month_overflow = advance_facility_event_months({4, maximum}, 1);
    check(month_overflow.error == FacilityEventError::numeric_overflow &&
              !month_overflow.transition,
          "month overflow rejected");
    const auto last = transition(advance_facility_event_months({maximum, maximum - 1}, 1));
    check(!last.script_triggered && same(last.counters, {maximum, maximum}),
          "exact maximum months allowed with no item increment");
    check(same(transition(advance_facility_event_months(last.counters, 0)).counters, last.counters),
          "zero months at maximum remains valid");
    check(transition(confirm_facility_item({maximum - 1, maximum}, 2)).script_triggered,
          "last representable confirmation triggers without wrap");
}

void random_sequences() {
    std::mt19937 random(0xE7E170U);
    for (int trial = 0; trial < 100; ++trial) {
        const auto icon = static_cast<int>(random() % 6U);
        FacilityEventCounters actual;
        std::int64_t item_count = 0;
        std::int64_t elapsed_months = 0;
        for (int step = 0; step < 200; ++step) {
            bool expected_trigger = false;
            FacilityEventTransition next;
            if (random() % 3U == 0) {
                const auto months = static_cast<std::int64_t>(random() % 13U);
                elapsed_months += months;
                next = transition(advance_facility_event_months(actual, months));
            } else {
                ++item_count;
                if (icon >= 2 && icon <= 3 && item_count > 4 && elapsed_months > 35) {
                    expected_trigger = true;
                    item_count = 0;
                    elapsed_months = 0;
                }
                next = transition(confirm_facility_item(actual, icon));
            }
            check(next.script_triggered == expected_trigger &&
                      same(next.counters, {item_count, elapsed_months}),
                  "random event stream matches independent scalar lifecycle");
            actual = next.counters;
        }
    }
    for (int trial = 0; trial < 200; ++trial) {
        const auto first = static_cast<std::int64_t>(random() % 1000U);
        const auto second = static_cast<std::int64_t>(random() % 1000U);
        const FacilityEventCounters input{100, 20};
        const auto split = transition(advance_facility_event_months(
            transition(advance_facility_event_months(input, first)).counters, second));
        const auto bulk = transition(advance_facility_event_months(input, first + second));
        check(same(split.counters, bulk.counters) && !split.script_triggered &&
                  !bulk.script_triggered,
              "month chunking is equivalent and does not invent event delivery");
    }
}

} // namespace

int main() {
    gates_and_order();
    plan_structure();
    invalid_and_overflow();
    random_sequences();
    std::cout << checks << " checks passed\n";
}
