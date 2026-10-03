// Small verified category planner and injected-ticket sampler, not the complete Character priority
// machine. Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_reference/activity_choice.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {

template <std::size_t Size>
std::int64_t table_weight(const std::array<std::int64_t, Size> &table, std::int64_t count) {
    return table[static_cast<std::size_t>(std::min<std::int64_t>(count, Size - 1))];
}

} // namespace

ActivityCategoryResult plan_activity_categories(const ActivityChoiceInput &input) {
    const auto negative = [](std::int64_t value) { return value < 0; };
    if (std::any_of(input.available_category_counts.begin(), input.available_category_counts.end(),
                    negative) ||
        std::any_of(input.legacy_visit_counts.begin(), input.legacy_visit_counts.end(), negative)) {
        return {ActivityChoiceError::invalid_input, std::nullopt};
    }
    if (input.legacy_activity != 0 && input.legacy_activity != 2 && input.legacy_activity != 6) {
        return {ActivityChoiceError::unsupported_activity, std::nullopt};
    }
    ActivityCategoryPlan plan;
    const auto append = [&](int category, std::int64_t weight) {
        plan.options.push_back({category, weight});
        plan.total_weight += weight;
    };
    const auto available = [&](std::size_t category) {
        return input.available_category_counts[category] > 0;
    };
    const auto &visits = input.legacy_visit_counts;
    if (input.legacy_activity == 6) {
        if (available(8)) {
            append(8, table_weight(std::array<std::int64_t, 3>{40, 20, 10}, visits[3]));
        }
        if (available(6)) {
            append(6, 40);
        }
        append(-1, 60);
        return {ActivityChoiceError::none, plan};
    }
    const bool special = (input.legacy_flags & 8192U) != 0;
    const auto combined_visits =
        static_cast<std::uint64_t>(visits[0]) + static_cast<std::uint64_t>(visits[2]);
    const bool allow_ordinary = !special || visits[1] != 0 || combined_visits < 2;
    const bool allow_exit = !special || !allow_ordinary || combined_visits > 0;
    if (allow_ordinary && available(1)) {
        append(1, 40);
    }
    if (allow_ordinary && available(2)) {
        append(2, table_weight(std::array<std::int64_t, 3>{30, 10, 5}, visits[2]));
    }
    const bool exit_in_plan = allow_exit && available(4);
    if (exit_in_plan) {
        append(4, visits[1] == 0 ? 40 : 0);
    }
    if ((!special || visits[1] != 0) && (visits[0] > 0 || visits[1] > 0 || visits[2] > 0)) {
        const std::array<std::int64_t, 6> fallback{3, 5, 10, 20, 30, 50};
        append(3, fallback[plan.options.size()]);
    }
    if (!special && combined_visits >= 5) {
        // This path bypasses the random draw and requests category 3, not category 4.
        plan.options.clear();
        plan.total_weight = 0;
        if (exit_in_plan) {
            plan.forced_category = 3;
        }
    }
    return {ActivityChoiceError::none, plan};
}

WeightedTicketResult select_weighted_ticket(const std::vector<std::int64_t> &weights,
                                            std::int64_t ticket) {
    std::int64_t total = 0;
    for (const auto weight : weights) {
        if (weight < 0) {
            return {WeightedTicketError::invalid_weight, std::nullopt};
        }
        if (weight > std::numeric_limits<std::int64_t>::max() - total) {
            return {WeightedTicketError::numeric_overflow, std::nullopt};
        }
        total += weight;
    }
    if (total == 0) {
        return {WeightedTicketError::no_weight, std::nullopt};
    }
    if (ticket < 0 || ticket >= total) {
        return {WeightedTicketError::invalid_ticket, std::nullopt};
    }
    for (std::size_t index = 0; index < weights.size(); ++index) {
        if (ticket < weights[index]) {
            return {WeightedTicketError::none, index};
        }
        ticket -= weights[index];
    }
    return {WeightedTicketError::invalid_ticket, std::nullopt};
}

} // namespace dungeon_village_reference
