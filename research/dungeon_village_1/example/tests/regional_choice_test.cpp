#include "dungeon_village_reference/activity_choice.hpp"
#include "dungeon_village_reference/regional_choice.hpp"

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

ActivityCandidateSnapshot fixture() {
    ActivityCandidateSnapshot result;
    for (int i = 0; i < 3; ++i) {
        result.cells.push_back({{i, i + 1},
                                {0, 0, 0},
                                std::nullopt,
                                std::nullopt,
                                CandidateOrigin::event,
                                static_cast<std::size_t>(i)});
        ++result.category_counts[0];
    }
    return result;
}

RegionalChoicePlan select(const ActivityCandidateSnapshot &snapshot, int bottom,
                          const std::vector<std::int64_t> &draws) {
    const auto result = select_regional_candidate(snapshot, bottom, draws);
    check(result.error == RegionalChoiceError::none && result.plan.has_value(), "valid plan");
    return *result.plan;
}

void boundaries() {
    const auto input = fixture();
    for (std::size_t hit = 0; hit < 5; ++hit) {
        std::vector<std::int64_t> draws(6, 0);
        draws[hit] = 1;
        const auto plan = select(input, 0, draws);
        check(plan.status == RegionalChoiceStatus::selected && plan.consumed_draws == hit + 1 &&
                  !plan.used_fallback && plan.target->snapshot_index == 1,
              "five early positions and exact bottom-plus-two boundary");
    }
    for (std::size_t count = 0; count <= 5; ++count) {
        const auto plan = select(input, 0, std::vector<std::int64_t>(count, 0));
        check(plan.status == RegionalChoiceStatus::needs_draw && plan.consumed_draws == count &&
                  plan.next_draw_bound == 3 && !plan.target,
              "every insufficient prefix requests another bounded draw");
    }
    const auto fallback = select(input, 10, {0, 0, 0, 0, 0, 2});
    check(fallback.target->snapshot_index == 2 && fallback.consumed_draws == 6 &&
              fallback.used_fallback && !fallback.target->cell.cost,
          "sixth fresh draw is unconditional, including unreachable unbound cells");
    const auto ignored = select(input, 0, {1, -9, 100});
    check(ignored.consumed_draws == 1, "unconsumed tail is not interpreted");
    const auto empty = select({}, 0, {-1});
    check(empty.status == RegionalChoiceStatus::no_candidate && empty.consumed_draws == 0,
          "empty snapshot consumes no randomness");
    check(select(input, std::numeric_limits<int>::max(), {0, 0, 0, 0, 0, 0}).used_fallback,
          "threshold does not overflow");
    check(select(input, std::numeric_limits<int>::min(), {0}).consumed_draws == 1,
          "minimum bottom accepts first draw");
    const auto refuse = [](const auto &snapshot, const auto &draws, auto error) {
        const auto result = select_regional_candidate(snapshot, 0, draws);
        check(result.error == error && !result.plan, "refusal returns no partial plan");
    };
    refuse(input, std::vector<std::int64_t>{-1}, RegionalChoiceError::invalid_ticket);
    refuse(input, std::vector<std::int64_t>{3}, RegionalChoiceError::invalid_ticket);
    refuse(input, std::vector<std::int64_t>{0, 0, 0, 0, 0, -1},
           RegionalChoiceError::invalid_ticket);
    refuse(input, std::vector<std::int64_t>(7, 1), RegionalChoiceError::too_many_draws);
    auto forged = input;
    ++forged.category_counts[0];
    refuse(forged, std::vector<std::int64_t>{1}, RegionalChoiceError::invalid_snapshot);
}

void differential_and_composition() {
    const auto input = fixture();
    std::mt19937 random(20261003);
    for (int trial = 0; trial < 1000; ++trial) {
        std::vector<std::int64_t> draws;
        for (int i = 0; i < 6; ++i)
            draws.push_back(random() % 3);
        const int bottom = static_cast<int>(random() % 5) - 1;
        std::size_t consumed = 6;
        for (std::size_t i = 0; i < 5; ++i) {
            if (input.cells[static_cast<std::size_t>(draws[i])].position.y >= bottom + 2) {
                consumed = i + 1;
                break;
            }
        }
        for (std::size_t length = 0; length <= 6; ++length) {
            const auto plan = select(input, bottom, {draws.begin(), draws.begin() + length});
            if (length < consumed) {
                check(plan.status == RegionalChoiceStatus::needs_draw &&
                          plan.consumed_draws == length,
                      "prefix differential pending");
            } else {
                check(plan.status == RegionalChoiceStatus::selected &&
                          plan.consumed_draws == consumed &&
                          plan.used_fallback == (consumed == 6) &&
                          plan.target->snapshot_index ==
                              static_cast<std::size_t>(draws[consumed - 1]),
                      "prefix differential selected");
            }
        }
    }
    LegacyMap map{3, 4, std::vector<LegacyMapCell>(12)};
    const auto field = search_legacy_map(map, {0, 0});
    check(field.field.has_value(), "distance field");
    ActivityCandidateInput scan;
    scan.legacy_activity = 6;
    scan.town = {0, 2, 0, 1};
    scan.cell_definition_ids.assign(12, 0);
    scan.definitions = {{0, 0, 0}};
    const auto candidates = collect_activity_candidates(*field.field, scan);
    check(candidates.error == ActivityCandidateError::none, "regional scan accepts valid bounds");
    check(candidates.snapshot && !candidates.snapshot->cells.empty(), "regional scan");
    ActivityChoiceInput category;
    category.legacy_activity = 6;
    category.available_category_counts = candidates.snapshot->category_counts;
    const auto plan = plan_activity_categories(category);
    check(plan.plan && plan.plan->options.size() == 1 && plan.plan->options[0].category == -1 &&
              plan.plan->total_weight == 60,
          "regional category plan");
    const auto target = select(*candidates.snapshot, scan.town.bottom, {0, 0, 0, 0, 0, 0});
    check(target.target && trace_legacy_path(*field.field, target.target->cell.position).error ==
                               MapAccessError::none,
          "scan, category, target and path compose");
}
} // namespace

int main() {
    boundaries();
    differential_and_composition();
    std::cout << checks << " regional choice checks passed\n";
}
