// Synthetic maps exercise composition, not a claimed post-initialization APK world snapshot.
#include "ark/simulation/facilities/rules/facility_departure.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
ActivityCandidateSnapshot snapshot(std::vector<ActivityCandidateCell> cells) {
    ActivityCandidateSnapshot result{std::move(cells), {}};
    for (const auto &cell : result.cells)
        if (!cell.instance || cell.instance->legacy_phase == 1)
            ++result.category_counts[static_cast<std::size_t>(cell.definition.legacy_category)];
    return result;
}
ActivityCandidateCell candidate(Position position, std::uint64_t instance, int definition,
                                int category, std::int64_t charm,
                                std::optional<std::int64_t> cost) {
    return {position, {definition, category, charm}, CandidateInstance{{instance}, definition, 1},
            cost,     CandidateOrigin::event,        0};
}
LegacyDistanceField field_for(const std::vector<ActivityCandidateCell> &cells,
                              Position start = {2, 2}) {
    std::vector<BoundFacility> facilities;
    for (const auto &cell : cells)
        facilities.push_back({{cell.instance->instance_id, cell.definition.definition_id,
                               FacilityShape::single, FacilityOrientation::first, cell.position},
                              3});
    const auto bound = bind_facility_map({5, 5, std::vector<LegacyMapCell>(25)}, facilities);
    check(bound.error == MapAccessError::none && bound.map.has_value(), "fixture bindings");
    const auto field = search_legacy_map(*bound.map, start);
    check(field.error == MapAccessError::none && field.field.has_value(), "fixture distance field");
    return *field.field;
}
FacilityDeparture departure(const LegacyDistanceField &field,
                            const ActivityCandidateSnapshot &snapshot,
                            FacilityDepartureInput input = {}) {
    const auto result = prepare_facility_departure(field, snapshot, input);
    check(result.error == FacilityDepartureError::none &&
              result.route_error == MapAccessError::none && result.departure.has_value(),
          "successful complete candidate");
    return *result.departure;
}
void refuse(const LegacyDistanceField &field, const ActivityCandidateSnapshot &snapshot,
            FacilityDepartureInput input, FacilityDepartureError error,
            MapAccessError route_error = MapAccessError::none) {
    const auto result = prepare_facility_departure(field, snapshot, input);
    check(result.error == error && result.route_error == route_error && !result.departure,
          "refusal never contains partial path or target");
}

void first_visit_local_weights() {
    const auto scene =
        snapshot({candidate({3, 2}, 1, 30, 1, 5, 50), candidate({1, 2}, 3, 28, 2, 5, 50),
                  candidate({2, 3}, 2, 33, 1, 4, 70)});
    const auto field = field_for(scene.cells);
    check(valid_activity_candidate_snapshot(scene), "fixture follows sorted snapshot contract");
    auto unsorted = scene;
    std::swap(unsorted.cells[1], unsorted.cells[2]);
    refuse(field, unsorted, {}, FacilityDepartureError::invalid_snapshot);
    FacilityDepartureInput input;
    input.legacy_flags = 2U | 8192U;
    int first_category{}, second_category{};
    for (int ticket = 0; ticket < 70; ++ticket) {
        input.category_ticket = ticket;
        const auto result = departure(field, scene, input);
        if (result.category == 1)
            ++first_category;
        else if (result.category == 2)
            ++second_category;
        check(result.route.steps.size() == 1 && result.route.steps.back() == result.binding.goal &&
                  result.selection.goal.position == result.binding.goal,
              "category-instance-path identities agree");
        check(result.legacy_direction == (ticket < 40 ? 1 : 3), "first grid edge facing");
    }
    check(first_category == 40 && second_category == 30,
          "first visitor category tickets are 40:30, not multiplied by shop count");
    input.category_ticket = 0;
    int weapon{}, food{};
    for (int ticket = 0; ticket < 9; ++ticket) {
        input.facility_ticket = ticket;
        const auto result = departure(field, scene, input);
        if (result.binding.definition_id == 30)
            ++weapon;
        else if (result.binding.definition_id == 33)
            ++food;
        check(result.legacy_direction == (ticket < 5 ? 1 : 0), "shop-specific initial direction");
    }
    check(weapon == 5 && food == 4, "weapon and bun shop use definition charm 5:4");
    auto duplicated = scene.cells;
    // Keep the repeated cost-50 occurrence ahead of the cost-70 goal, without deduplication.
    duplicated.insert(duplicated.begin() + 2, scene.cells.front());
    const auto duplicate_scene = snapshot(std::move(duplicated));
    input.facility_ticket = 9;
    const auto duplicate = departure(field, duplicate_scene, input);
    check(duplicate.selection.drawn_snapshot_index == 2 &&
              duplicate.selection.goal_snapshot_index == 0 &&
              duplicate.binding.instance_id.value == 1,
          "repeated occurrence keeps weight but goal remains full-snapshot first match");
    check(scene.cells.size() == 3 && field.start == Position{2, 2} && scene.category_counts[1] == 2,
          "preparation has no scene or actor mutation");
}

void directions_and_equal_endpoints() {
    const Position goals[] = {{2, 3}, {3, 2}, {2, 1}, {1, 2}};
    for (int direction = 0; direction < 4; ++direction) {
        const auto scene =
            snapshot({candidate(goals[direction], 1, 30, 1, 5, direction % 2 == 0 ? 70 : 50)});
        const auto field = field_for(scene.cells);
        check(departure(field, scene).legacy_direction == direction,
              "legacy directions correspond to grid axes, not display axes");
    }
    const auto scene = snapshot({candidate({2, 2}, 1, 30, 1, 5, 0)});
    const auto field = field_for(scene.cells);
    const auto same = departure(field, scene);
    check(same.route.steps.empty() && same.route.cost == 0 && !same.legacy_direction,
          "equal endpoints preserve facing; preparation is not an arrival event");
}

void validation_and_explicit_boundaries() {
    const auto scene = snapshot({candidate({3, 2}, 1, 30, 1, 5, 50)});
    const auto field = field_for(scene.cells);
    FacilityDepartureInput input;
    for (const int ticket : {-1, 40}) {
        input.category_ticket = ticket;
        refuse(field, scene, input, FacilityDepartureError::invalid_ticket);
    }
    input = {};
    for (const int ticket : {-1, 5}) {
        input.facility_ticket = ticket;
        refuse(field, scene, input, FacilityDepartureError::invalid_ticket);
    }
    input = {};
    input.legacy_visit_counts[0] = -1;
    refuse(field, scene, input, FacilityDepartureError::invalid_input);
    input = {};
    input.legacy_activity = 5;
    refuse(field, scene, input, FacilityDepartureError::unsupported_activity);
    refuse(field, {}, {}, FacilityDepartureError::no_category);
    auto zero = scene;
    zero.cells.front().definition.definition_charm = 0;
    refuse(field, zero, {}, FacilityDepartureError::no_facility_weight);
    auto unbound = scene;
    unbound.cells.front().instance.reset();
    refuse(field, unbound, {}, FacilityDepartureError::no_facility_weight);
    auto invalid = scene;
    invalid.category_counts[1] = 2;
    refuse(field, invalid, {}, FacilityDepartureError::invalid_snapshot);
    invalid = scene;
    invalid.cells.front().cost = 51;
    refuse(field, invalid, {}, FacilityDepartureError::invalid_snapshot);
    invalid = scene;
    invalid.cells.front().instance->instance_id.value = 9;
    refuse(field, invalid, {}, FacilityDepartureError::binding_mismatch);
    auto wrong_field = field;
    wrong_field.distances.pop_back();
    refuse(wrong_field, scene, {}, FacilityDepartureError::invalid_field,
           MapAccessError::invalid_field);
    invalid = scene;
    invalid.cells.front().position = {5, 2};
    invalid.cells.front().cost.reset();
    refuse(field, invalid, {}, FacilityDepartureError::route_failure,
           MapAccessError::invalid_position);

    const auto overflow =
        snapshot({candidate({3, 2}, 1, 30, 1, std::numeric_limits<std::int64_t>::max(), 50),
                  candidate({2, 3}, 2, 33, 1, 1, 70)});
    refuse(field_for(overflow.cells), overflow, {}, FacilityDepartureError::numeric_overflow);
    auto other = snapshot({candidate({3, 2}, 1, 30, 4, 5, 50)});
    refuse(field, other, {}, FacilityDepartureError::unsupported_category);
    input = {};
    input.legacy_visit_counts[0] = 5;
    input.category_ticket = -1;
    refuse(field, other, input, FacilityDepartureError::unsupported_category);
    other = snapshot({candidate({3, 2}, 1, 30, 8, 5, 50)});
    input = {};
    input.legacy_activity = 6;
    check(departure(field, other, input).category == 8, "known ordinary activity-six branch");
    input.category_ticket = 40;
    refuse(field, other, input, FacilityDepartureError::unsupported_category);
}

void unreachable_is_not_reselection() {
    const auto scene = snapshot({candidate({3, 2}, 1, 30, 1, 5, std::nullopt)});
    auto map = field_for(scene.cells).map;
    for (int y = 0; y < 5; ++y)
        map.cells[static_cast<std::size_t>(y * 5 + 2)] = {5, RouteCategory::blocked, std::nullopt};
    const auto search = search_legacy_map(map, {0, 2});
    check(search.error == MapAccessError::none && search.field, "separated fixture search");
    refuse(*search.field, scene, {}, FacilityDepartureError::route_failure,
           MapAccessError::unreachable);
    check(scene.cells.size() == 1 && !scene.cells.front().cost,
          "unreachable selected event is retained, not silently redrawn or given a fake path");
}
} // namespace

int main() {
    try {
        first_visit_local_weights();
        directions_and_equal_endpoints();
        validation_and_explicit_boundaries();
        unreachable_is_not_reselection();
        std::cout << "departure checks=" << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
