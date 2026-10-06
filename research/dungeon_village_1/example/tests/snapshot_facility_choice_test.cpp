#include "dungeon_village_reference/snapshot_facility_choice.hpp"

#include "dungeon_village_reference/activity_choice.hpp"
#include "dungeon_village_reference/ranked_facility_choice.hpp"

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

ActivityCandidateCell cell(int x, std::uint64_t instance, int definition, int category,
                           std::int64_t charm, std::optional<std::int64_t> cost, int phase = 1) {
    std::optional<CandidateInstance> binding;
    if (instance != 0) {
        binding = CandidateInstance{{instance}, definition, phase};
    }
    return {{x, 1}, {definition, category, charm}, binding,
            cost,   CandidateOrigin::event,        static_cast<std::size_t>(x)};
}

ActivityCandidateSnapshot snapshot(std::vector<ActivityCandidateCell> cells) {
    ActivityCandidateSnapshot result{std::move(cells), {}};
    for (const auto &entry : result.cells) {
        if (!entry.instance || entry.instance->legacy_phase == 1) {
            ++result.category_counts[static_cast<std::size_t>(entry.definition.legacy_category)];
        }
    }
    return result;
}

SnapshotFacilityTarget choose(const ActivityCandidateSnapshot &input, int category,
                              std::int64_t ticket) {
    const auto result = select_snapshot_facility(input, category, ticket);
    check(result.error == SnapshotFacilityError::none && result.target.has_value(),
          "valid snapshot selection returns complete target");
    return *result.target;
}

void retained_instance_weights() {
    auto mixed = cell(0, 1, 20, 2, 1000, 1);
    mixed.definition.legacy_kind = 6;
    mixed.instance->definition_id = 10;
    mixed.instance_definition = CandidateDefinition{10, 1, 2, 1};
    mixed.legacy_state = 3;
    mixed.route_category = RouteCategory::road;
    auto input = snapshot({mixed, cell(1, 2, 30, 1, 3, 2)});
    check(valid_activity_candidate_snapshot(input) && input.category_counts[1] == 1 &&
              input.category_counts[2] == 1,
          "mixed surface count does not replace the retained instance's category");
    for (const auto ticket : {0, 1, 2, 4}) {
        const auto selected = choose(input, 1, ticket);
        check(selected.goal.instance->instance_id == BuildingId{ticket < 2 ? 1U : 2U},
              "two tickets belong to old instance charm2 and three to ordinary charm3");
    }
    const auto end = select_snapshot_facility(input, 1, 5);
    check(end.error == SnapshotFacilityError::invalid_ticket && !end.target,
          "mixed selection uses old instance total5, never road surface charm1000");
    const auto other = select_snapshot_facility(input, 2, 0);
    check(other.error == SnapshotFacilityError::no_weight && !other.target,
          "surface category count alone does not invent an active instance of that category");
    for (int mutation = 0; mutation < 4; ++mutation) {
        auto bad = input;
        auto &entry = bad.cells.front();
        if (mutation == 0)
            entry.instance_definition.reset();
        else if (mutation == 1)
            entry.instance_definition->definition_id = 99;
        else if (mutation == 2)
            entry.legacy_state = 1;
        else
            entry.route_category = RouteCategory::terminal;
        check(!valid_activity_candidate_snapshot(bad) &&
                  select_snapshot_facility(bad, 1, 0).error ==
                      SnapshotFacilityError::invalid_snapshot,
              "mixed snapshot still rejects missing identity, mismatched definition and illegal "
              "surface");
    }
}

void three_index_spaces_and_duplicates() {
    auto input =
        snapshot({cell(0, 0, 0, 1, 1000, 0), cell(1, 9, 90, 1, 1000, 1, 2),
                  cell(2, 3, 30, 2, 50, 2), cell(3, 1, 10, 1, 5, 3), cell(3, 1, 10, 1, 5, 3),
                  cell(4, 2, 20, 1, 5, 4), cell(5, 1, 10, 1, 5, 5)});
    input.cells[3].origin = CandidateOrigin::map_scan;
    input.cells[4].source_index = 99;
    check(valid_activity_candidate_snapshot(input) && input.category_counts[1] == 5,
          "consistent duplicate coordinates and non-weighted category-count occurrence");
    for (std::int64_t ticket = 0; ticket < 20; ++ticket) {
        const auto output = choose(input, 1, ticket);
        const auto drawn = static_cast<std::size_t>(ticket / 5) + 3;
        const auto goal = drawn == 5 ? 5U : 3U;
        check(output.drawn_active_index == drawn - 2 && output.drawn_snapshot_index == drawn &&
                  output.goal_snapshot_index == goal &&
                  output.goal.position == input.cells[goal].position &&
                  output.goal.source_index == input.cells[goal].source_index &&
                  output.goal.origin == input.cells[goal].origin,
              "active index includes other categories, full index includes inactive and unbound");
    }
    check(input.cells.size() == 7 && input.cells[4].source_index == 99 &&
              input.category_counts[1] == 5,
          "selection does not mutate or deduplicate snapshot");
    for (const int category : {1, 2, 6, 8}) {
        auto two = snapshot({cell(0, 1, 10, category, 2, 1), cell(0, 1, 10, category, 2, 1),
                             cell(1, 2, 20, category, 2, 2)});
        int first = 0;
        for (std::int64_t ticket = 0; ticket < 6; ++ticket) {
            const auto output = choose(two, category, ticket);
            if (output.goal.instance->instance_id.value == 1)
                ++first;
        }
        check(first == 4, "every supported category preserves 2:1 repeated-coordinate weights");
    }
    const auto unbound = snapshot({cell(0, 0, 0, 1, 999, 0)});
    const auto no_weight = select_snapshot_facility(unbound, 1, 0);
    check(unbound.category_counts[1] == 1 && no_weight.error == SnapshotFacilityError::no_weight &&
              !no_weight.target,
          "available category count does not guarantee ordinary weight");
}

void refusals_and_limits() {
    const auto refuse = [](const auto &input, int category, std::int64_t ticket, auto error) {
        const auto result = select_snapshot_facility(input, category, ticket);
        check(result.error == error && !result.target, "selection refusal has no partial target");
    };
    const auto original =
        snapshot({cell(0, 1, 10, 1, 5, 1), cell(0, 1, 10, 1, 5, 1), cell(1, 1, 10, 1, 5, 2)});
    for (const int category : {-1, 0, 3, 4, 5, 7, 9, 10, 11}) {
        refuse(original, category, 0, SnapshotFacilityError::unsupported_category);
    }
    refuse(ActivityCandidateSnapshot{}, 1, 0, SnapshotFacilityError::no_weight);
    for (int mutation = 0; mutation < 15; ++mutation) {
        auto bad = original;
        auto &last = bad.cells.back();
        switch (mutation) {
        case 0:
            last.position.x = -1;
            break;
        case 1:
            last.position.y = -1;
            break;
        case 2:
            last.definition.definition_id = -1;
            break;
        case 3:
            last.definition.legacy_category = 11;
            break;
        case 4:
            last.definition.definition_charm = -1;
            break;
        case 5:
            last.cost = -1;
            break;
        case 6:
            last.origin = static_cast<CandidateOrigin>(99);
            break;
        case 7:
            last.instance->instance_id.value = 0;
            break;
        case 8:
            last.instance->legacy_phase = -1;
            break;
        case 9:
            last.instance->definition_id = 99;
            break;
        case 10:
            last.instance->legacy_phase = 2;
            bad.category_counts[1] -= 1;
            break;
        case 11:
            last.definition.definition_charm = 6;
            break;
        case 12:
            bad.category_counts[1] -= 1;
            break;
        case 13:
            bad.cells[0].cost.reset();
            break;
        case 14:
            bad.cells[1].instance.reset();
            break;
        }
        check(!valid_activity_candidate_snapshot(bad), "shared validator rejects forged snapshot");
        refuse(bad, 1, 0, SnapshotFacilityError::invalid_snapshot);
    }
    refuse(original, 1, -1, SnapshotFacilityError::invalid_ticket);
    refuse(original, 1, 15, SnapshotFacilityError::invalid_ticket);
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    const auto exact = snapshot({cell(0, 1, 10, 1, maximum - 1, 0), cell(1, 2, 20, 1, 1, 1)});
    check(choose(exact, 1, maximum - 1).goal_snapshot_index == 1,
          "exact maximum summed weight accepts last ticket without wrapping");
    auto overflow = exact;
    overflow.cells[1].definition.definition_charm = 2;
    refuse(overflow, 1, 0, SnapshotFacilityError::numeric_overflow);
    auto bounded = snapshot(std::vector<ActivityCandidateCell>(4096, cell(0, 1, 10, 1, 0, 0)));
    check(valid_activity_candidate_snapshot(bounded), "exact snapshot limit accepted");
    refuse(bounded, 1, 0, SnapshotFacilityError::no_weight);
    bounded.cells.push_back(bounded.cells[0]);
    bounded.category_counts[1] += 1;
    refuse(bounded, 1, 0, SnapshotFacilityError::invalid_snapshot);
    const auto unreachable = snapshot({cell(0, 1, 10, 1, 3, std::nullopt)});
    check(!choose(unreachable, 1, 2).goal.cost,
          "unreachable event occurrence can be chosen without fabricating finite cost");
    const auto zero = snapshot({cell(0, 1, 10, 1, 0, 0), cell(1, 2, 20, 1, 3, 1)});
    check(choose(zero, 1, 0).drawn_active_index == 1, "zero charm never selected");
}

void map_plan_path_composition() {
    LegacyMap terrain{7, 3, std::vector<LegacyMapCell>(21)};
    for (int y = 0; y < 3; ++y)
        terrain.cells[static_cast<std::size_t>(y * 7 + 3)] = {5, RouteCategory::blocked,
                                                              std::nullopt};
    const auto bound = bind_facility_map(
        terrain, {{{{1}, 10, FacilityShape::single, FacilityOrientation::first, {1, 1}}, 3},
                  {{{2}, 20, FacilityShape::single, FacilityOrientation::first, {5, 1}}, 3}});
    check(bound.error == MapAccessError::none && bound.map, "composition facility binding");
    const auto field = search_legacy_map(*bound.map, {0, 1});
    check(field.error == MapAccessError::none && field.field, "composition full distance field");
    ActivityCandidateInput input;
    input.town = {0, 6, 0, 2};
    input.cell_definition_ids.assign(21, 0);
    input.cell_definition_ids[8] = 10;
    input.cell_definition_ids[12] = 20;
    input.definitions = {{0, 0, 0}, {10, 1, 5}, {20, 1, 5}};
    input.instances = {{{1}, 10, 1}, {{2}, 20, 1}};
    input.events = {{{1, 1}, 1}, {{5, 1}, 1}, {{5, 1}, 1}};
    const auto result = collect_activity_candidates(*field.field, input);
    check(result.error == ActivityCandidateError::none && result.snapshot &&
              result.snapshot->cells.size() == 4 && result.snapshot->category_counts[1] == 4,
          "map scan plus duplicate reachable and unreachable event occurrences");
    ActivityChoiceInput choice;
    choice.available_category_counts = result.snapshot->category_counts;
    const auto plan = plan_activity_categories(choice);
    check(plan.error == ActivityChoiceError::none && plan.plan && plan.plan->options.size() == 1 &&
              plan.plan->options[0].category == 1 && plan.plan->options[0].weight == 40,
          "candidate count enables category but does not multiply first-level weight");
    int unreachable_draws = 0;
    for (std::int64_t ticket = 0; ticket < 20; ++ticket) {
        const auto target = choose(*result.snapshot, 1, ticket);
        const auto route = trace_legacy_path(*field.field, target.goal.position);
        const auto &instance = *target.goal.instance;
        if (!target.goal.cost) {
            ++unreachable_draws;
            check(route.error == MapAccessError::unreachable && route.steps.empty(),
                  "selected unreachable event still fails path reconstruction");
            check(!arrival_binding_matches(
                      *bound.map,
                      {target.goal.position, instance.instance_id, instance.definition_id},
                      field.field->start),
                  "candidate selection alone cannot make starting actor arrive");
        } else {
            check(route.error == MapAccessError::none && route.cost == *target.goal.cost &&
                      arrival_binding_matches(
                          *bound.map,
                          {target.goal.position, instance.instance_id, instance.definition_id},
                          target.goal.position),
                  "finite target traces exact field and revalidates physical identity");
        }
    }
    check(unreachable_draws == 10, "unreachable repeated events retain half the ordinary weight");
}

void random_expanded_ticket_oracle() {
    std::mt19937 random(0x5AA9507U);
    const int categories[] = {1, 2, 6, 8};
    for (int trial = 0; trial < 200; ++trial) {
        std::vector<ActivityCandidateCell> cells;
        for (std::uint64_t id = 1; id <= 6; ++id) {
            const int category = categories[random() % 4U];
            const int phase = static_cast<int>(random() % 3U);
            const auto charm = static_cast<std::int64_t>(random() % 6U);
            const auto occurrences = random() % 4U + 1;
            for (std::uint32_t occurrence = 0; occurrence < occurrences; ++occurrence) {
                const auto cost = random() % 5U == 0 ? std::optional<std::int64_t>{}
                                                     : std::optional<std::int64_t>{random() % 6U};
                cells.push_back(cell(static_cast<int>(cells.size()), id, static_cast<int>(id),
                                     category, charm, cost, phase));
            }
        }
        cells.push_back(cell(static_cast<int>(cells.size()), 0, 0, 1, 999, 0));
        for (int duplicate = 0; duplicate < 4; ++duplicate) {
            auto repeated = cells[random() % cells.size()];
            repeated.source_index = static_cast<std::size_t>(100 + duplicate);
            cells.push_back(repeated);
        }
        std::stable_sort(cells.begin(), cells.end(), [](const auto &left, const auto &right) {
            return left.cost && (!right.cost || *left.cost < *right.cost);
        });
        const auto input = snapshot(std::move(cells));
        check(valid_activity_candidate_snapshot(input), "random snapshot internal consistency");
        for (const int category : categories) {
            std::vector<std::pair<std::size_t, std::size_t>> expanded;
            std::size_t active_index = 0;
            for (std::size_t index = 0; index < input.cells.size(); ++index) {
                const auto &entry = input.cells[index];
                if (!entry.instance || entry.instance->legacy_phase != 1)
                    continue;
                if (entry.definition.legacy_category == category) {
                    for (std::int64_t weight = 0; weight < entry.definition.definition_charm;
                         ++weight)
                        expanded.emplace_back(active_index, index);
                }
                ++active_index;
            }
            for (std::size_t ticket = 0; ticket < expanded.size(); ++ticket) {
                const auto drawn = expanded[ticket];
                const auto chosen_id = input.cells[drawn.second].instance->instance_id;
                std::size_t goal = 0;
                while (!input.cells[goal].instance ||
                       !(input.cells[goal].instance->instance_id == chosen_id))
                    ++goal;
                const auto output = choose(input, category, static_cast<std::int64_t>(ticket));
                check(
                    output.drawn_active_index == drawn.first &&
                        output.drawn_snapshot_index == drawn.second &&
                        output.goal_snapshot_index == goal &&
                        output.goal.position == input.cells[goal].position &&
                        output.goal.cost == input.cells[goal].cost,
                    "random full-active mapping matches explicit repetition and first-rank oracle");
            }
            const auto end = select_snapshot_facility(input, category,
                                                      static_cast<std::int64_t>(expanded.size()));
            check(end.error == (expanded.empty() ? SnapshotFacilityError::no_weight
                                                 : SnapshotFacilityError::invalid_ticket) &&
                      !end.target,
                  "random upper ticket boundary");
        }
    }
}

void compatible_ranked_subset() {
    const auto input = snapshot({cell(0, 1, 10, 1, 2, 0), cell(1, 2, 20, 2, 3, 1),
                                 cell(2, 1, 10, 1, 2, 2), cell(3, 3, 30, 1, 9, 3, 2)});
    std::vector<RankedFacilityCell> ranked;
    for (const auto &entry : input.cells) {
        ranked.push_back({entry.position, entry.instance->instance_id,
                          entry.definition.definition_id, entry.definition.legacy_category,
                          entry.instance->legacy_phase, entry.definition.definition_charm,
                          *entry.cost});
    }
    for (std::int64_t ticket = 0; ticket < 4; ++ticket) {
        const auto reference = select_ranked_facility(ranked, 1, ticket);
        const auto output = choose(input, 1, ticket);
        check(reference.error == RankedFacilityError::none && reference.target &&
                  output.drawn_snapshot_index == reference.target->drawn_index &&
                  output.goal_snapshot_index == reference.target->goal_index &&
                  output.goal.position == reference.target->goal.position,
              "R2-I unique finite bound subset agrees without weakening its contract");
    }
}

} // namespace

int main() {
    retained_instance_weights();
    three_index_spaces_and_duplicates();
    refusals_and_limits();
    map_plan_path_composition();
    random_expanded_ticket_oracle();
    compatible_ranked_subset();
    std::cout << checks << " checks passed\n";
}
