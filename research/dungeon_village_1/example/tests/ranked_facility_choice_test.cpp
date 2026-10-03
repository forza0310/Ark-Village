#include "dungeon_village_reference/ranked_facility_choice.hpp"

#include "dungeon_village_reference/map_access.hpp"

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

RankedFacilityCell cell(int x, std::uint64_t instance, int definition, std::int64_t charm,
                        std::int64_t cost, int category = 1, int phase = 1) {
    return {{x, 1}, {instance}, definition, category, phase, charm, cost};
}

RankedFacilityTarget choose(const std::vector<RankedFacilityCell> &cells, int category,
                            std::int64_t ticket) {
    const auto result = select_ranked_facility(cells, category, ticket);
    check(result.error == RankedFacilityError::none && result.target.has_value(),
          "valid complete ranked target");
    return *result.target;
}

void repeated_instance_and_goal() {
    const std::vector<RankedFacilityCell> cells{cell(0, 1, 36, 5, 1), cell(1, 2, 33, 10, 2),
                                                cell(2, 1, 36, 5, 3)};
    for (std::int64_t ticket = 0; ticket < 20; ++ticket) {
        const auto target = choose(cells, 1, ticket);
        const auto expected_draw = ticket < 5 ? 0U : ticket < 15 ? 1U : 2U;
        const auto expected_goal = expected_draw == 2 ? 0U : expected_draw;
        check(target.drawn_index == expected_draw && target.goal_index == expected_goal &&
                  target.goal.instance_id == cells[expected_goal].instance_id &&
                  target.goal.position == cells[expected_goal].position,
              "draw later occurrence but arrive at earliest cell for the chosen instance");
    }
    const std::vector<RankedFacilityCell> multiplicity{cell(0, 1, 36, 5, 1), cell(1, 2, 36, 5, 1),
                                                       cell(2, 2, 36, 5, 1), cell(3, 2, 36, 5, 1),
                                                       cell(4, 2, 36, 5, 1)};
    int first = 0;
    int second = 0;
    for (std::int64_t ticket = 0; ticket < 25; ++ticket) {
        const auto target = choose(multiplicity, 1, ticket);
        if (target.goal.instance_id.value == 1) {
            ++first;
        } else {
            ++second;
            check(target.goal_index == 1, "four-cell instance preserves first supplied tie rank");
        }
    }
    check(first == 5 && second == 20,
          "same-definition one versus four eligible cells has 1:4 weight");
    auto filtered = multiplicity;
    filtered[0].legacy_phase = 2;
    check(choose(filtered, 1, 0).goal.instance_id.value == 2, "nonactive phase skipped");
    filtered[0].legacy_phase = 0;
    check(choose(filtered, 1, 0).goal.instance_id.value == 2, "construction phase skipped");
    filtered[0] = cell(0, 1, 28, 1000, 1, 2);
    check(choose(filtered, 1, 0).goal.instance_id.value == 2,
          "different category excluded from draw");
    filtered[0] = cell(0, 1, 999, 0, 1);
    check(choose(filtered, 1, 0).goal.instance_id.value == 2, "zero charm never drawn");
}

void supported_categories() {
    for (const int category : {1, 2, 6, 8}) {
        const int other_category = category == 1 ? 2 : 1;
        const std::vector<RankedFacilityCell> candidates{cell(0, 1, 100, 1000, 0, other_category),
                                                         cell(1, 2, 101, 3, 1, category),
                                                         cell(2, 2, 101, 3, 2, category)};
        for (std::int64_t ticket = 0; ticket < 6; ++ticket) {
            const auto target = choose(candidates, category, ticket);
            check(target.drawn_index == (ticket < 3 ? 1U : 2U) && target.goal_index == 1 &&
                      target.goal.legacy_category == category && target.goal.instance_id.value == 2,
                  "all supported ordinary categories preserve multiplicity and filter others");
        }
        const auto end = select_ranked_facility(candidates, category, 6);
        check(end.error == RankedFacilityError::invalid_ticket && !end.target,
              "other-category charm does not enlarge the selected category ticket range");
    }
}

void refusals() {
    const std::vector<RankedFacilityCell> original{cell(0, 1, 36, 5, 1), cell(1, 1, 36, 5, 2)};
    const auto reject = [](const auto &cells, int category, std::int64_t ticket,
                           RankedFacilityError expected) {
        const auto result = select_ranked_facility(cells, category, ticket);
        check(result.error == expected && !result.target, "invalid ranked request has no target");
    };
    reject(std::vector<RankedFacilityCell>{}, 1, 0, RankedFacilityError::no_weight);
    reject(std::vector<RankedFacilityCell>{cell(0, 1, 36, 0, 0)}, 1, 0,
           RankedFacilityError::no_weight);
    for (const int category : {-1, 0, 3, 4, 5, 7, 9, 10, 11}) {
        reject(original, category, 0, RankedFacilityError::unsupported_category);
    }
    for (int mutation = 0; mutation < 12; ++mutation) {
        auto bad = original;
        auto &last = bad.back();
        switch (mutation) {
        case 0:
            last.position.x = -1;
            break;
        case 1:
            last.position.y = -1;
            break;
        case 2:
            last.instance_id.value = 0;
            break;
        case 3:
            last.definition_id = -1;
            break;
        case 4:
            last.legacy_category = -1;
            break;
        case 5:
            last.legacy_category = 11;
            break;
        case 6:
            last.legacy_phase = -1;
            break;
        case 7:
            last.definition_charm = -1;
            break;
        case 8:
            last.cost = 0;
            break;
        case 9:
            last.position = bad.front().position;
            break;
        case 10:
            last.definition_id = 33;
            break;
        case 11:
            last.legacy_phase = 2;
            break;
        }
        reject(bad, 1, 0, RankedFacilityError::invalid_input);
    }
    auto bad = original;
    bad.back().instance_id = {2};
    bad.back().definition_charm = 6;
    reject(bad, 1, 0, RankedFacilityError::invalid_input);
    bad = original;
    bad.back().instance_id = {2};
    bad.back().legacy_category = 2;
    reject(bad, 1, 0, RankedFacilityError::invalid_input);
    bad = original;
    bad.front().cost = -1;
    reject(bad, 1, 0, RankedFacilityError::invalid_input);
    reject(original, 1, -1, RankedFacilityError::invalid_ticket);
    reject(original, 1, 10, RankedFacilityError::invalid_ticket);
    const auto maximum = std::numeric_limits<std::int64_t>::max();
    bad = original;
    for (auto &candidate : bad) {
        candidate.definition_charm = maximum;
    }
    reject(bad, 1, 0, RankedFacilityError::numeric_overflow);
    bad.resize(1);
    check(choose(bad, 1, maximum - 1).goal.instance_id.value == 1,
          "maximum exact single weight and final ticket");
    bad.assign(1000001, original.front());
    reject(bad, 1, 0, RankedFacilityError::invalid_input);
}

void map_composition() {
    const LegacyMap terrain{8, 6, std::vector<LegacyMapCell>(48)};
    const std::vector<FacilityPlacement> placements{
        {{1}, 55, FacilityShape::square, FacilityOrientation::first, {3, 2}},
        {{2}, 36, FacilityShape::single, FacilityOrientation::first, {6, 2}}};
    const auto bound = bind_facility_map(terrain, {{placements[0], 3}, {placements[1], 3}});
    check(bound.error == MapAccessError::none && bound.map.has_value(), "composition map binding");
    const auto search = search_legacy_map(*bound.map, {0, 0});
    check(search.error == MapAccessError::none && search.field.has_value(),
          "composition distance field");
    const auto access = inspect_facility_access(*search.field, placements);
    check(access.error == MapAccessError::none && access.facilities.size() == 2,
          "composition multi-cell access");
    std::vector<RankedFacilityCell> candidates;
    for (const auto &facility : access.facilities) {
        for (const auto &reachable : facility.cells) {
            candidates.push_back({reachable.cell.position, facility.instance_id,
                                  facility.definition_id, 1, 1, 5, reachable.cost});
        }
    }
    std::sort(candidates.begin(), candidates.end(), [](const auto &left, const auto &right) {
        if (left.cost != right.cost) {
            return left.cost < right.cost;
        }
        return left.position.y * 8 + left.position.x < right.position.y * 8 + right.position.x;
    });
    check(candidates.size() == 5,
          "square and single have all five reachable eligible cells in fixture");
    int square_count = 0;
    for (std::int64_t ticket = 0; ticket < 25; ++ticket) {
        const auto target = choose(candidates, 1, ticket);
        const auto route = trace_legacy_path(*search.field, target.goal.position);
        check(route.error == MapAccessError::none && route.cost == target.goal.cost,
              "selected ranked goal traces exact field route");
        check(arrival_binding_matches(
                  *bound.map,
                  {target.goal.position, target.goal.instance_id, target.goal.definition_id},
                  target.goal.position),
              "selected goal binding preserves instance and definition");
        if (target.goal.instance_id.value == 1) {
            ++square_count;
        }
    }
    check(square_count == 20, "map-derived square contributes four charm occurrences");
}

void random_occurrence_oracle() {
    std::mt19937 random(0xFAC1117U);
    for (int trial = 0; trial < 100; ++trial) {
        std::vector<RankedFacilityCell> candidates;
        const auto instances = random() % 6U + 1;
        for (std::uint32_t id = 1; id <= instances; ++id) {
            const auto count = random() % 4U + 1;
            const auto charm = static_cast<std::int64_t>(random() % 6U);
            const auto phase = static_cast<int>(random() % 3U);
            const auto category = static_cast<int>(random() % 2U) + 1;
            for (std::uint32_t occurrence = 0; occurrence < count; ++occurrence) {
                candidates.push_back(cell(static_cast<int>(candidates.size()), id,
                                          static_cast<int>(100 + id), charm, 1, category, phase));
            }
        }
        std::shuffle(candidates.begin(), candidates.end(), random);
        std::vector<std::size_t> expanded;
        for (std::size_t index = 0; index < candidates.size(); ++index) {
            const auto &candidate = candidates[index];
            if (candidate.legacy_phase == 1 && candidate.legacy_category == 1) {
                for (std::int64_t count = 0; count < candidate.definition_charm; ++count) {
                    expanded.push_back(index);
                }
            }
        }
        for (std::size_t ticket = 0; ticket < expanded.size(); ++ticket) {
            const auto drawn = expanded[ticket];
            std::size_t goal = 0;
            while (!(candidates[goal].instance_id == candidates[drawn].instance_id)) {
                ++goal;
            }
            const auto output = choose(candidates, 1, static_cast<std::int64_t>(ticket));
            check(
                output.drawn_index == drawn && output.goal_index == goal &&
                    output.goal.position == candidates[goal].position,
                "random repeated-instance selection matches explicit ticket and first-rank oracle");
        }
        const auto past_end =
            select_ranked_facility(candidates, 1, static_cast<std::int64_t>(expanded.size()));
        check(past_end.error == (expanded.empty() ? RankedFacilityError::no_weight
                                                  : RankedFacilityError::invalid_ticket) &&
                  !past_end.target,
              "random complete request upper boundary");
    }
}

} // namespace

int main() {
    repeated_instance_and_goal();
    supported_categories();
    refusals();
    map_composition();
    random_occurrence_oracle();
    std::cout << checks << " checks passed\n";
}
