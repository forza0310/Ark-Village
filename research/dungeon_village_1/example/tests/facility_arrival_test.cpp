#include "dungeon_village_reference/facility_arrival.hpp"

#include "dungeon_village_reference/accounting.hpp"
#include "dungeon_village_reference/facility_economy.hpp"

#include <algorithm>
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

bool same(const FacilityArrivalState &left, const FacilityArrivalState &right) {
    return left.legacy_visit_counts == right.legacy_visit_counts &&
           left.legacy_category_one_count == right.legacy_category_one_count &&
           left.legacy_category_six_counter == right.legacy_category_six_counter &&
           left.last_visited_instance == right.last_visited_instance &&
           left.legacy_actor_total == right.legacy_actor_total &&
           left.current_month_facility_sales == right.current_month_facility_sales;
}

FacilityArrivalInput request() {
    FacilityArrivalInput input;
    input.character_id = {1};
    input.instance_id = {2};
    input.definition_id = 28;
    input.legacy_kind = 3;
    input.legacy_category = 2;
    input.resolved_instance_price = 300;
    return input;
}

FacilityArrivalCandidate prepare(const FacilityArrivalState &state,
                                 const FacilityArrivalInput &input) {
    const auto result = prepare_facility_arrival(state, input);
    check(result.error == FacilityArrivalError::none && result.candidate.has_value(),
          "valid arrival returns complete candidate");
    return *result.candidate;
}

FacilityArrivalState oracle(FacilityArrivalState state, const FacilityArrivalInput &input) {
    const int category = input.legacy_category;
    const int kind = input.legacy_kind;
    const bool first = category == 1;
    const bool second = category == 2;
    const bool later = !first && !second;
    const bool one = later && (kind == 7 || (kind != 1 && category == 4));
    const bool three = later && kind != 7 && kind != 1 && category == 6 && input.legacy_detail == 2;
    state.legacy_visit_counts[0] += first ? 1 : 0;
    state.legacy_visit_counts[1] += one ? 1 : 0;
    state.legacy_visit_counts[2] += second ? 1 : 0;
    state.legacy_visit_counts[3] += three ? 1 : 0;
    state.legacy_category_one_count += first ? 1 : 0;
    if (three) {
        state.legacy_category_six_counter = 0;
    }
    state.last_visited_instance = input.instance_id;
    const auto income = input.legacy_actor_kind == 0 && (input.legacy_flags & 768U) == 0
                            ? std::max(0, input.resolved_instance_price)
                            : 0;
    state.legacy_actor_total += income;
    state.current_month_facility_sales += income;
    return state;
}

void branch_truth_table() {
    const FacilityArrivalState source{{3, 4, 5, 6, 7, 8}, 20, 9, BuildingId{99}, 17, 23};
    for (const int kind : {0, 1, 3, 7}) {
        for (int category = 0; category < 11; ++category) {
            for (const int detail : {0, 2, 3}) {
                for (const std::uint32_t flags : {0U, 256U, 512U, 768U, 8192U}) {
                    for (const int actor : {0, 1, 2}) {
                        for (const int price : {-10, 0, 1, 300}) {
                            auto input = request();
                            input.legacy_kind = kind;
                            input.legacy_category = category;
                            input.legacy_detail = detail;
                            input.legacy_flags = flags;
                            input.legacy_actor_kind = actor;
                            input.resolved_instance_price = price;
                            const auto actual = prepare(source, input);
                            const auto expected = oracle(source, input);
                            check(same(actual.state, expected),
                                  "arrival branch priority truth table");
                            check(
                                actual.cash_income == expected.legacy_actor_total - 17 &&
                                    actual.character_id == input.character_id &&
                                    actual.instance_id == input.instance_id &&
                                    actual.definition_id == input.definition_id,
                                "cash guard is independent of visit counters and target identity");
                        }
                    }
                }
            }
        }
    }
    check(source.legacy_visit_counts == std::array<std::int32_t, 6>{3, 4, 5, 6, 7, 8} &&
              source.last_visited_instance == std::optional<BuildingId>{BuildingId{99}},
          "truth-table source never mutated");
}

void refusals_and_boundaries() {
    const auto refuse = [](const auto &state, const auto &input, auto error) {
        const auto result = prepare_facility_arrival(state, input);
        check(result.error == error && !result.candidate,
              "complete arrival refusal without output");
    };
    for (int mutation = 0; mutation < 12; ++mutation) {
        auto input = request();
        FacilityArrivalState state;
        switch (mutation) {
        case 0:
            input.character_id.value = 0;
            break;
        case 1:
            input.instance_id.value = 0;
            break;
        case 2:
            input.definition_id = -1;
            break;
        case 3:
            input.legacy_kind = -1;
            break;
        case 4:
            input.legacy_category = -1;
            break;
        case 5:
            input.legacy_category = 11;
            break;
        case 6:
            input.legacy_detail = -1;
            break;
        case 7:
            input.legacy_actor_kind = -1;
            break;
        case 8:
            input.legacy_selection = -3;
            break;
        case 9:
            input.legacy_month_index = -1;
            break;
        case 10:
            input.legacy_month_index = 12;
            break;
        case 11:
            state.last_visited_instance = BuildingId{0};
            break;
        }
        refuse(state, input, FacilityArrivalError::invalid_input);
    }
    for (int slot = 0; slot < 8; ++slot) {
        FacilityArrivalState state;
        if (slot < 6)
            state.legacy_visit_counts[static_cast<std::size_t>(slot)] = -1;
        else if (slot == 6)
            state.legacy_category_one_count = -1;
        else
            state.legacy_category_six_counter = -1;
        refuse(state, request(), FacilityArrivalError::invalid_input);
    }
    for (const int detail : {1, 4, 5}) {
        auto input = request();
        input.legacy_detail = detail;
        input.legacy_flags = 512;
        refuse(FacilityArrivalState{}, input, FacilityArrivalError::unsupported_branch);
    }
    for (const int selection : {-2, 0, 99}) {
        auto input = request();
        input.legacy_selection = selection;
        refuse(FacilityArrivalState{}, input, FacilityArrivalError::unsupported_branch);
    }
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    for (int location = 0; location < 7; ++location) {
        auto input = request();
        FacilityArrivalState state;
        switch (location) {
        case 0:
            input.legacy_category = 1;
            state.legacy_visit_counts[0] = maximum;
            break;
        case 1:
            input.legacy_kind = 7;
            input.legacy_category = 4;
            state.legacy_visit_counts[1] = maximum;
            break;
        case 2:
            state.legacy_visit_counts[2] = maximum;
            break;
        case 3:
            input.legacy_category = 6;
            input.legacy_detail = 2;
            state.legacy_visit_counts[3] = maximum;
            break;
        case 4:
            input.legacy_category = 1;
            state.legacy_category_one_count = maximum;
            break;
        case 5:
            state.legacy_actor_total = maximum;
            break;
        case 6:
            state.current_month_facility_sales = maximum;
            break;
        }
        const auto before = state;
        refuse(state, input, FacilityArrivalError::numeric_overflow);
        check(same(state, before), "overflow preserves caller state after all earlier increments");
    }
    FacilityArrivalState state;
    state.legacy_visit_counts.fill(maximum);
    state.legacy_category_one_count = maximum;
    state.legacy_category_six_counter = maximum;
    state.legacy_actor_total = maximum;
    state.current_month_facility_sales = maximum;
    auto input = request();
    input.legacy_kind = 1;
    input.legacy_category = 4;
    input.legacy_flags = 256;
    check(prepare(state, input).cash_income == 0, "untouched maximum values do not overflow");
    state = {};
    state.legacy_actor_total = std::numeric_limits<std::int32_t>::min();
    state.current_month_facility_sales = -10;
    input = request();
    input.definition_id = 0;
    input.legacy_month_index = 11;
    input.resolved_instance_price = maximum;
    const auto exact = prepare(state, input);
    check(exact.state.legacy_actor_total == -1 &&
              exact.state.current_month_facility_sales == maximum - 10 &&
              exact.legacy_month_index == 11 && exact.definition_id == 0,
          "signed legacy totals preserved and exact maximum positive amount accepted");
}

void randomized_state_oracle() {
    std::mt19937 random(0xA221A1U);
    for (int trial = 0; trial < 1000; ++trial) {
        FacilityArrivalState state;
        for (auto &count : state.legacy_visit_counts)
            count = static_cast<int>(random() % 1000U);
        state.legacy_category_one_count = static_cast<int>(random() % 1000U);
        state.legacy_category_six_counter = static_cast<int>(random() % 1000U);
        state.legacy_actor_total = static_cast<int>(random() % 2000U) - 1000;
        state.current_month_facility_sales = static_cast<int>(random() % 2000U) - 1000;
        auto input = request();
        input.instance_id = {random() % 100U + 1};
        input.legacy_kind = static_cast<int>(random() % 14U);
        input.legacy_category = static_cast<int>(random() % 11U);
        input.legacy_detail = random() % 2U ? 0 : 2;
        input.legacy_actor_kind = static_cast<int>(random() % 2U);
        input.legacy_flags = (random() % 2U ? 256U : 0U) | (random() % 2U ? 512U : 0U);
        input.resolved_instance_price = static_cast<int>(random() % 10000U) - 5000;
        check(same(prepare(state, input).state, oracle(state, input)),
              "randomized independent branch and signed-total oracle");
    }
}

void economy_and_ledger_composition() {
    FacilityEconomyDefinition definition;
    definition.attributes = {LevelEndpoints{300, 450}, LevelEndpoints{5, 50}, LevelEndpoints{5, 50},
                             LevelEndpoints{240, 480}};
    const auto values = derive_facility_economy(definition, {});
    check(values.error == FacilityEconomyError::none && values.values, "fixture price derivation");
    auto input = request();
    input.resolved_instance_price =
        static_cast<std::int32_t>(values.values->instance_attributes[0]);
    FacilityArrivalState initial;
    const auto candidate = prepare(initial, input);
    PeriodAccounting ledger(1000, 10);
    const CashEntry income{1, 1, CashCategory::facilities, CashDirection::income,
                           candidate.cash_income};
    check(ledger.post_cash(income) == AccountingError::none && ledger.funds() == 1300 &&
              ledger.village_points() == 10 && candidate.state.legacy_actor_total == 300 &&
              candidate.state.current_month_facility_sales == 300,
          "arrival produces one facility income, not a debit or village points");
    check(ledger.post_cash(income) == AccountingError::none && ledger.funds() == 1300,
          "cash event retry is independently idempotent");
    check(ledger.prepare_report({1, {}, {}}) == AccountingError::none &&
              ledger.reports().at(1).displayed.income == 300 && ledger.funds() == 1300,
          "arrival belongs to current posted period, report never repays it");
    check(ledger.claim_report(1) == AccountingError::none && ledger.funds() == 1300 &&
              ledger.village_points() == 10,
          "ordinary arrival does not award defeat points");
    check(same(prepare(initial, input).state, candidate.state),
          "same source yields same candidate");
    const auto again = prepare(candidate.state, input);
    check(again.state.legacy_visit_counts[2] == 2 && again.state.legacy_actor_total == 600,
          "arrival transition itself is not an idempotent event application");
    const CashEntry late{2, 1, CashCategory::facilities, CashDirection::income, again.cash_income};
    check(ledger.post_cash(late) == AccountingError::sealed_period && ledger.funds() == 1300 &&
              initial.legacy_visit_counts[2] == 0,
          "rejected external ledger does not mutate the source arrival projection");
}

} // namespace

int main() {
    branch_truth_table();
    refusals_and_boundaries();
    randomized_state_oracle();
    economy_and_ledger_composition();
    std::cout << checks << " checks passed\n";
}
