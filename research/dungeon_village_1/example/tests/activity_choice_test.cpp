#include "dungeon_village_reference/activity_choice.hpp"

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

ActivityCategoryPlan plan(const ActivityChoiceInput &input) {
    const auto result = plan_activity_categories(input);
    check(result.error == ActivityChoiceError::none && result.plan.has_value(),
          "complete valid category plan");
    return *result.plan;
}

bool same(const ActivityCategoryPlan &left, const ActivityCategoryPlan &right) {
    if (left.forced_category != right.forced_category || left.total_weight != right.total_weight ||
        left.options.size() != right.options.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.options.size(); ++index) {
        if (left.options[index].category != right.options[index].category ||
            left.options[index].weight != right.options[index].weight) {
            return false;
        }
    }
    return true;
}

void fixed_plans() {
    ActivityChoiceInput input;
    auto output = plan(input);
    check(output.options.empty() && !output.forced_category && output.total_weight == 0,
          "zero visits and no candidates has no category");
    input.available_category_counts[1] = 1;
    input.available_category_counts[2] = 1;
    input.available_category_counts[4] = 1;
    ActivityCategoryPlan expected;
    expected.options = {{1, 40}, {2, 30}, {4, 40}};
    expected.total_weight = 110;
    check(same(plan(input), expected), "initial ordinary category order and weight");
    input.legacy_visit_counts[0] = 1;
    expected.options.push_back({3, 20});
    expected.total_weight = 130;
    check(same(plan(input), expected), "fallback weight based on three existing options");
    input.legacy_visit_counts[1] = 1;
    expected.options[2].weight = 0;
    expected.total_weight = 90;
    check(same(plan(input), expected), "zero-weight exit remains for fallback list-length input");
    const auto baseline = plan(input);
    input.available_category_counts[1] = 500;
    input.available_category_counts[2] = 20;
    input.available_category_counts[4] = 1000;
    check(same(plan(input), baseline), "candidate quantity does not multiply category weights");
    input.legacy_visit_counts[0] = 5;
    output = plan(input);
    check(output.forced_category == 3 && output.options.empty() && output.total_weight == 0,
          "visit limit requests category three despite zero exit weight");
    input.available_category_counts[4] = 0;
    output = plan(input);
    check(!output.forced_category && output.options.empty(),
          "visit limit without category four does not draw remaining categories");
    input = {};
    input.legacy_flags = 8192;
    input.available_category_counts[1] = 1;
    input.available_category_counts[2] = 1;
    input.available_category_counts[4] = 1;
    expected = {{{1, 40}, {2, 30}}, std::nullopt, 70};
    check(same(plan(input), expected), "special zero visits exclude category four and fallback");
    input.legacy_visit_counts[0] = 1;
    expected = {{{1, 40}, {2, 30}, {4, 40}}, std::nullopt, 110};
    check(same(plan(input), expected), "special one ordinary visit allows exit without fallback");
    input.legacy_visit_counts[2] = 1;
    expected = {{{4, 40}}, std::nullopt, 40};
    check(same(plan(input), expected), "special two visits only category four");
    input.legacy_visit_counts[1] = 1;
    expected = {{{1, 40}, {2, 10}, {4, 0}, {3, 20}}, std::nullopt, 70};
    check(same(plan(input), expected), "special exit visit restores ordinary options and fallback");
    input.legacy_flags |= 1U << 20U;
    check(same(plan(input), expected), "unknown unrelated flags do not affect this rule");
    input = {};
    input.legacy_activity = 6;
    expected = {{{-1, 60}}, std::nullopt, 60};
    check(same(plan(input), expected), "activity six always has negative-one fallback");
    input.available_category_counts[8] = 1;
    input.available_category_counts[6] = 1;
    for (int count = 0; count < 5; ++count) {
        input.legacy_visit_counts[3] = count;
        const auto weight = count == 0 ? 40 : count == 1 ? 20 : 10;
        expected = {{{8, weight}, {6, 40}, {-1, 60}}, std::nullopt, weight + 100};
        check(same(plan(input), expected), "activity-six exact weight table and order");
    }
    for (int mask = 0; mask < 4; ++mask) {
        input.available_category_counts[8] = (mask & 1) != 0 ? 10 : 0;
        input.available_category_counts[6] = (mask & 2) != 0 ? 20 : 0;
        input.legacy_flags = 8192;
        input.legacy_visit_counts.fill(1000);
        expected = {};
        if ((mask & 1) != 0) {
            expected.options.push_back({8, 10});
            expected.total_weight += 10;
        }
        if ((mask & 2) != 0) {
            expected.options.push_back({6, 40});
            expected.total_weight += 40;
        }
        expected.options.push_back({-1, 60});
        expected.total_weight += 60;
        check(same(plan(input), expected), "activity-six availability masks and count clamps");
    }
}

ActivityCategoryPlan independent_plan(int first, int exit, int rest, bool special, int mask) {
    ActivityCategoryPlan result;
    const auto available = [&](int bit) { return (mask & (1 << bit)) != 0; };
    if (!special && first + rest >= 5) {
        if (available(2)) {
            result.forced_category = 3;
        }
        return result;
    }
    const bool restricted = special && exit == 0;
    const std::array<bool, 3> allowed{!restricted || first + rest < 2,
                                      !restricted || first + rest < 2,
                                      !special || first + rest != 0};
    const std::array<int, 3> categories{1, 2, 4};
    const std::array<int, 3> weights{40, rest == 0 ? 30 : rest == 1 ? 10 : 5, exit == 0 ? 40 : 0};
    for (int index = 0; index < 3; ++index) {
        if (available(index) && allowed[static_cast<std::size_t>(index)]) {
            result.options.push_back({categories[static_cast<std::size_t>(index)],
                                      weights[static_cast<std::size_t>(index)]});
            result.total_weight += weights[static_cast<std::size_t>(index)];
        }
    }
    if (!restricted && first + rest + exit > 0) {
        const auto count = result.options.size();
        const auto weight = count == 0 ? 3 : count == 1 ? 5 : count == 2 ? 10 : 20;
        result.options.push_back({3, weight});
        result.total_weight += weight;
    }
    return result;
}

void exhaustive_categories() {
    for (int first = 0; first < 7; ++first) {
        for (int exit = 0; exit < 3; ++exit) {
            for (int rest = 0; rest < 7; ++rest) {
                for (int special = 0; special < 2; ++special) {
                    for (int mask = 0; mask < 8; ++mask) {
                        ActivityChoiceInput input;
                        input.legacy_flags = special == 1 ? 8192 : 0;
                        input.legacy_visit_counts = {first, exit, rest, 0, 7, 11};
                        for (int index = 0; index < 3; ++index) {
                            const std::array<std::size_t, 3> categories{1, 2, 4};
                            input.available_category_counts[categories[static_cast<std::size_t>(
                                index)]] = (mask & (1 << index)) != 0 ? 20 : 0;
                        }
                        const auto zero = plan(input);
                        input.legacy_activity = 2;
                        const auto two = plan(input);
                        check(same(zero, two),
                              "activities zero and two have identical category plans");
                        check(same(zero, independent_plan(first, exit, rest, special == 1, mask)),
                              "category plan matches independent eligibility truth-table oracle");
                        if (!zero.forced_category && zero.total_weight > 0) {
                            std::vector<std::int64_t> weights;
                            std::vector<std::size_t> tickets;
                            for (std::size_t index = 0; index < zero.options.size(); ++index) {
                                weights.push_back(zero.options[index].weight);
                                for (std::int64_t count = 0; count < weights.back(); ++count) {
                                    tickets.push_back(index);
                                }
                            }
                            for (std::size_t ticket = 0; ticket < tickets.size(); ++ticket) {
                                const auto selected = select_weighted_ticket(
                                    weights, static_cast<std::int64_t>(ticket));
                                check(selected.error == WeightedTicketError::none &&
                                          selected.index == tickets[ticket],
                                      "every category ticket maps to exact half-open weighted "
                                      "interval");
                            }
                        }
                    }
                }
            }
        }
    }
}

void invalid_and_large() {
    ActivityChoiceInput input;
    for (std::size_t index = 0; index < input.available_category_counts.size(); ++index) {
        input.available_category_counts[index] = -1;
        const auto result = plan_activity_categories(input);
        check(result.error == ActivityChoiceError::invalid_input && !result.plan,
              "all negative candidate counts rejected without plan");
        input.available_category_counts[index] = 0;
    }
    for (std::size_t index = 0; index < input.legacy_visit_counts.size(); ++index) {
        input.legacy_visit_counts[index] = -1;
        const auto result = plan_activity_categories(input);
        check(result.error == ActivityChoiceError::invalid_input && !result.plan,
              "all negative visit counters rejected without plan");
        input.legacy_visit_counts[index] = 0;
    }
    for (const int activity : {-1, 1, 3, 4, 5, 7, 8, 100}) {
        input.legacy_activity = activity;
        const auto result = plan_activity_categories(input);
        check(result.error == ActivityChoiceError::unsupported_activity && !result.plan,
              "other activity branches explicitly unsupported");
    }
    input = {};
    input.legacy_visit_counts.fill(std::numeric_limits<std::int64_t>::max());
    input.available_category_counts.fill(std::numeric_limits<std::int64_t>::max());
    check(plan(input).forced_category == 3, "wide combined visits do not wrap before forced gate");
    input.legacy_flags = 8192;
    const ActivityCategoryPlan expected{{{1, 40}, {2, 5}, {4, 0}, {3, 20}}, std::nullopt, 65};
    check(same(plan(input), expected), "maximum visit counts safely clamp table lookup");
    auto result = select_weighted_ticket({}, 0);
    check(result.error == WeightedTicketError::no_weight && !result.index, "empty weights no draw");
    result = select_weighted_ticket({0, 0}, 0);
    check(result.error == WeightedTicketError::no_weight && !result.index, "zero weights no draw");
    result = select_weighted_ticket({1, -1}, 0);
    check(result.error == WeightedTicketError::invalid_weight && !result.index,
          "later invalid weight rejects before returning an early index");
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    result = select_weighted_ticket({maximum, 1}, 0);
    check(result.error == WeightedTicketError::numeric_overflow && !result.index,
          "later sum overflow rejects the entire draw");
    result = select_weighted_ticket({1}, -1);
    check(result.error == WeightedTicketError::invalid_ticket && !result.index,
          "negative ticket rejected");
    result = select_weighted_ticket({1}, 1);
    check(result.error == WeightedTicketError::invalid_ticket && !result.index,
          "exclusive upper ticket bound");
    check(select_weighted_ticket({0, maximum}, maximum - 1).index == 1,
          "last ticket of exact maximum total works without addition overflow");
}

void random_ticket_oracle() {
    std::mt19937 random(0xACA17U);
    for (int trial = 0; trial < 200; ++trial) {
        std::vector<std::int64_t> weights;
        std::vector<std::size_t> tickets;
        const auto size = static_cast<std::size_t>(random() % 16U) + 1;
        for (std::size_t index = 0; index < size; ++index) {
            weights.push_back(random() % 6U);
            for (std::int64_t count = 0; count < weights.back(); ++count) {
                tickets.push_back(index);
            }
        }
        for (std::size_t ticket = 0; ticket < tickets.size(); ++ticket) {
            const auto output = select_weighted_ticket(weights, static_cast<std::int64_t>(ticket));
            check(output.error == WeightedTicketError::none && output.index == tickets[ticket],
                  "random weights match independent explicit ticket expansion");
        }
        const auto past_end =
            select_weighted_ticket(weights, static_cast<std::int64_t>(tickets.size()));
        check(past_end.error == (tickets.empty() ? WeightedTicketError::no_weight
                                                 : WeightedTicketError::invalid_ticket) &&
                  !past_end.index,
              "random exclusive upper bound and all-zero distinction");
    }
}

} // namespace

int main() {
    fixed_plans();
    exhaustive_categories();
    invalid_and_large();
    random_ticket_oracle();
    std::cout << checks << " checks passed\n";
}
