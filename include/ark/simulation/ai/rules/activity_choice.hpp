#pragma once

// Small verified category planner and injected-ticket sampler, not the complete Character priority
// machine.

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace ark::simulation::rules {

struct ActivityChoiceInput {
    std::int32_t legacy_activity{};
    std::array<std::int64_t, 11> available_category_counts{};
    std::array<std::int64_t, 6> legacy_visit_counts{};
    std::uint32_t legacy_flags{};
};

struct WeightedActivityCategory {
    std::int32_t category{};
    std::int64_t weight{};
};

struct ActivityCategoryPlan {
    std::vector<WeightedActivityCategory> options;
    std::optional<std::int32_t> forced_category;
    std::int64_t total_weight{};
};

enum class ActivityChoiceError { none, invalid_input, unsupported_activity };
struct ActivityCategoryResult {
    ActivityChoiceError error{ActivityChoiceError::none};
    std::optional<ActivityCategoryPlan> plan;
};

// Keep legacy counters/flags explicit; category weights are not multiplied by candidate counts.
ActivityCategoryResult plan_activity_categories(const ActivityChoiceInput &input);

enum class WeightedTicketError {
    none,
    invalid_weight,
    no_weight,
    numeric_overflow,
    invalid_ticket
};
struct WeightedTicketResult {
    WeightedTicketError error{WeightedTicketError::none};
    std::optional<std::size_t> index;
};

// Consume a caller-supplied ticket in [0,sum(weights)); this helper does not generate randomness.
WeightedTicketResult select_weighted_ticket(const std::vector<std::int64_t> &weights,
                                            std::int64_t ticket);

} // namespace ark::simulation::rules
