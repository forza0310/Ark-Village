// Ordinary arrival candidate separates visit counters, actor statistics, instance sales and cash
// income. Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/rules/facility_arrival.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
namespace {

bool add_positive(std::int32_t &value, std::int32_t amount) {
    const auto sum = static_cast<std::int64_t>(value) + amount;
    if (sum > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    value = static_cast<std::int32_t>(sum);
    return true;
}

} // namespace

FacilityArrivalResult prepare_facility_arrival(const FacilityArrivalState &state,
                                               const FacilityArrivalInput &input) {
    if (input.character_id.value == 0 || input.instance_id.value == 0 || input.definition_id < 0 ||
        input.legacy_kind < 0 || input.legacy_category < 0 || input.legacy_category >= 11 ||
        input.legacy_detail < 0 || input.legacy_actor_kind < 0 || input.legacy_selection < -2 ||
        input.legacy_month_index < 0 || input.legacy_month_index >= 12 ||
        (state.last_visited_instance && state.last_visited_instance->value == 0) ||
        state.legacy_category_one_count < 0 || state.legacy_category_six_counter < 0 ||
        std::any_of(
            state.legacy_visit_counts.begin(), state.legacy_visit_counts.end(),
            [](auto value) { return value < 0; })) {
        return {FacilityArrivalError::invalid_input, std::nullopt};
    }
    if (input.legacy_selection != -1 || input.legacy_detail == 1 || input.legacy_detail == 4 ||
        input.legacy_detail == 5) {
        return {FacilityArrivalError::unsupported_branch, std::nullopt};
    }
    FacilityArrivalCandidate next{input.character_id,
                                  input.instance_id,
                                  input.definition_id,
                                  input.legacy_month_index,
                                  state,
                                  0};
    next.state.last_visited_instance = input.instance_id;
    auto &visits = next.state.legacy_visit_counts;
    bool valid = true;
    if (input.legacy_category == 2) {
        valid = add_positive(visits[2], 1);
    } else if (input.legacy_category == 1) {
        valid = add_positive(next.state.legacy_category_one_count, 1) && add_positive(visits[0], 1);
    } else if (input.legacy_kind == 7) {
        valid = add_positive(visits[1], 1);
    } else if (input.legacy_kind != 1) {
        if (input.legacy_category == 4) {
            valid = add_positive(visits[1], 1);
        } else if (input.legacy_category == 6 && input.legacy_detail == 2) {
            valid = add_positive(visits[3], 1);
            next.state.legacy_category_six_counter = 0;
        }
    }
    if (!valid) {
        return {FacilityArrivalError::numeric_overflow, std::nullopt};
    }
    // Visit counters above still change when these guards suppress payment; actor_total is not a
    // wallet.
    if ((input.legacy_flags & (512U | 256U)) == 0 && input.legacy_actor_kind == 0 &&
        input.resolved_instance_price > 0) {
        const auto price = input.resolved_instance_price;
        if (!add_positive(next.state.legacy_actor_total, price) ||
            !add_positive(next.state.current_month_facility_sales, price)) {
            return {FacilityArrivalError::numeric_overflow, std::nullopt};
        }
        next.cash_income = price;
    }
    return {FacilityArrivalError::none, next};
}

} // namespace ark::simulation::rules
