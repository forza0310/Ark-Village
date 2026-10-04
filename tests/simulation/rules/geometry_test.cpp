#include "ark/simulation/rules/geometry.hpp"

#include <climits>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>

using namespace ark::simulation::rules;

namespace {

int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
std::set<std::pair<int, int>> positions(const GeometryResult &result) {
    std::set<std::pair<int, int>> cells;
    for (const auto &cell : result.cells) {
        cells.emplace(cell.position.x, cell.position.y);
    }
    return cells;
}
FacilityPlacement placement(std::uint64_t id, Position anchor,
                            FacilityShape shape = FacilityShape::single,
                            FacilityOrientation orientation = FacilityOrientation::first) {
    return {BuildingId{id}, 28, shape, orientation, anchor};
}

void exact_offsets_and_fragments() {
    const auto single =
        facility_footprint(FacilityShape::single, FacilityOrientation::first, {3, 3}, 7, 7);
    check(single.error == GeometryError::none && single.cells.size() == 1 &&
              single.cells[0].position == Position{3, 3} && single.cells[0].fragment_index == 0,
          "single first orientation");
    const auto flipped =
        facility_footprint(FacilityShape::single, FacilityOrientation::second, {3, 3}, 7, 7);
    check(flipped.cells[0].fragment_index == 1 && positions(flipped) == positions(single),
          "single orientation changes sprite fragment only");
    const auto vertical =
        facility_footprint(FacilityShape::pair, FacilityOrientation::first, {3, 3}, 7, 7);
    check(vertical.cells.size() == 2 && vertical.cells[0].position == Position{3, 4} &&
              vertical.cells[1].position == Position{3, 3} &&
              vertical.cells[0].fragment_index == 0 && vertical.cells[1].fragment_index == 2,
          "pair first order and fragments");
    const auto horizontal =
        facility_footprint(FacilityShape::pair, FacilityOrientation::second, {3, 3}, 7, 7);
    check(horizontal.cells.size() == 2 && horizontal.cells[0].position == Position{2, 3} &&
              horizontal.cells[1].position == Position{3, 3} &&
              horizontal.cells[0].fragment_index == 1 && horizontal.cells[1].fragment_index == 3,
          "pair second order and fragments");
    const auto square0 =
        facility_footprint(FacilityShape::square, FacilityOrientation::first, {3, 3}, 7, 7);
    const auto square1 =
        facility_footprint(FacilityShape::square, FacilityOrientation::second, {3, 3}, 7, 7);
    check(square0.cells.size() == 4 && positions(square0) == positions(square1),
          "square orientations preserve occupied set");
    check(square0.cells[1].position == Position{2, 3} &&
              square1.cells[1].position == Position{3, 4},
          "square binding order differs");
    for (std::size_t index = 0; index < 4; ++index) {
        check(square0.cells[index].fragment_index == static_cast<int>(2 * index) &&
                  square1.cells[index].fragment_index == static_cast<int>(2 * index + 1),
              "square fragment indices follow observed split table");
    }
}

void invalid_inputs_and_edges() {
    check(facility_footprint(static_cast<FacilityShape>(3), FacilityOrientation::first, {}, 7, 7)
                  .error == GeometryError::invalid_shape,
          "unknown shape rejected");
    check(facility_footprint(FacilityShape::single, static_cast<FacilityOrientation>(2), {}, 7, 7)
                  .error == GeometryError::invalid_orientation,
          "third orientation rejected");
    check(
        facility_footprint(FacilityShape::single, FacilityOrientation::first, {}, INT_MAX, INT_MAX)
                .error == GeometryError::invalid_bounds,
        "oversized map rejected without allocation");
    for (const auto anchor : {Position{-1, 0}, Position{0, -1}, Position{7, 0}, Position{0, 7},
                              Position{INT_MAX, INT_MAX}, Position{INT_MIN, INT_MIN}}) {
        const auto result =
            facility_footprint(FacilityShape::square, FacilityOrientation::first, anchor, 7, 7);
        check(result.error == GeometryError::outside_map && result.cells.empty(),
              "invalid anchor rejects entire footprint without overflow");
    }
    check(facility_footprint(FacilityShape::pair, FacilityOrientation::first, {0, 6}, 7, 7).error ==
              GeometryError::outside_map,
          "pair first extends past bottom edge");
    check(
        facility_footprint(FacilityShape::pair, FacilityOrientation::second, {0, 6}, 7, 7).error ==
            GeometryError::outside_map,
        "pair second extends past left edge");
    check(facility_footprint(FacilityShape::pair, FacilityOrientation::first, {0, 0}, 7, 7).error ==
              GeometryError::none,
          "pair first can use left edge");
    check(
        facility_footprint(FacilityShape::pair, FacilityOrientation::second, {6, 6}, 7, 7).error ==
            GeometryError::none,
        "pair second can use bottom edge");
}

void layout_and_movement() {
    const std::vector<FacilityPlacement> existing = {placement(1, {3, 3}, FacilityShape::pair),
                                                     placement(2, {5, 5}, FacilityShape::square)};
    check(validate_facility_layout(existing, 7, 7) == GeometryError::none,
          "same-definition instances keep independent occupied sets");
    check(evaluate_facility_placement(existing, placement(3, {3, 4}), 7, 7).error ==
              GeometryError::overlap,
          "overlap at non-anchor footprint cell rejected");
    check(evaluate_facility_placement(existing, placement(3, {0, 0}), 7, 7).error ==
              GeometryError::none,
          "non-overlapping placement accepted");
    const auto moved = placement(1, {3, 4}, FacilityShape::pair, FacilityOrientation::second);
    check(evaluate_facility_placement(existing, moved, 7, 7, BuildingId{1}).error ==
              GeometryError::none,
          "move excludes own old footprint but preserves ID");
    check(evaluate_facility_placement(existing, placement(1, {4, 5}, FacilityShape::pair), 7, 7,
                                      BuildingId{1})
                  .error == GeometryError::overlap,
          "move cannot overwrite another instance");
    check(evaluate_facility_placement(existing, moved, 7, 7).error ==
              GeometryError::invalid_identity,
          "new placement cannot reuse existing ID");
    check(evaluate_facility_placement(existing, moved, 7, 7, BuildingId{2}).error ==
              GeometryError::invalid_identity,
          "move argument must match candidate identity");
    auto unknown = moved;
    unknown.instance_id = BuildingId{9};
    check(evaluate_facility_placement(existing, unknown, 7, 7, BuildingId{9}).error ==
              GeometryError::invalid_identity,
          "missing move target rejected");
    auto changed_definition = moved;
    changed_definition.definition_id = 36;
    check(evaluate_facility_placement(existing, changed_definition, 7, 7, BuildingId{1}).error ==
              GeometryError::invalid_identity,
          "move cannot turn instance into different definition");
    auto changed_shape = moved;
    changed_shape.shape = FacilityShape::single;
    check(evaluate_facility_placement(existing, changed_shape, 7, 7, BuildingId{1}).error ==
              GeometryError::invalid_identity,
          "move cannot replace shape silently");
    auto duplicates = existing;
    duplicates.push_back(placement(1, {0, 0}));
    check(validate_facility_layout(duplicates, 7, 7) == GeometryError::invalid_identity,
          "duplicate stable ID rejected");
    auto zero = existing;
    zero.front().instance_id = BuildingId{0};
    check(validate_facility_layout(zero, 7, 7) == GeometryError::invalid_identity,
          "zero instance ID rejected");
    check(existing[0].anchor == Position{3, 3} &&
              existing[0].orientation == FacilityOrientation::first,
          "all quotes preserve input layout");
}

void exhaustive_map_edges() {
    for (const auto shape : {FacilityShape::single, FacilityShape::pair, FacilityShape::square}) {
        for (const auto orientation : {FacilityOrientation::first, FacilityOrientation::second}) {
            for (int y = -2; y <= 8; ++y) {
                for (int x = -2; x <= 8; ++x) {
                    const bool origin_valid = x >= 0 && x < 7 && y >= 0 && y < 7;
                    const bool expected =
                        origin_valid &&
                        (shape == FacilityShape::single ||
                         (shape == FacilityShape::pair &&
                          (orientation == FacilityOrientation::first ? y < 6 : x > 0)) ||
                         (shape == FacilityShape::square && x > 0 && y < 6));
                    const auto result = facility_footprint(shape, orientation, {x, y}, 7, 7);
                    check((result.error == GeometryError::none) == expected,
                          "all shape/orientation anchors agree with geometric bounds");
                    if (expected) {
                        const auto count = shape == FacilityShape::single ? 1U
                                           : shape == FacilityShape::pair ? 2U
                                                                          : 4U;
                        check(positions(result).size() == count && result.cells.size() == count,
                              "successful footprint has no duplicate or missing cells");
                    } else {
                        check(result.cells.empty(), "failed footprint has no partial output");
                    }
                }
            }
        }
    }
}

} // namespace

int main() {
    exact_offsets_and_fragments();
    invalid_inputs_and_edges();
    layout_and_movement();
    exhaustive_map_edges();
    std::cout << checks << " checks passed\n";
}
