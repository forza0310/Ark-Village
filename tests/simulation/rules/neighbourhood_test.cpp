#include "ark/simulation/rules/neighbourhood.hpp"

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>

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

FacilityPlacement placement(std::uint64_t id, std::int32_t definition, Position anchor,
                            FacilityShape shape = FacilityShape::single,
                            FacilityOrientation orientation = FacilityOrientation::first) {
    return {{id}, definition, shape, orientation, anchor};
}

std::vector<NeighbourDefinition> original_examples() {
    return {{28, FacilityShape::single, 3, {{2, 10}}},
            {29, FacilityShape::pair, 3, {{2, 10}}},
            {36, FacilityShape::single, 3, {{2, 10}}},
            {55, FacilityShape::square, 3, {{2, 20}}},
            {66, FacilityShape::single, 2, {{0, 20}, {1, 5}}},
            {74, FacilityShape::single, 2, {{0, 70}, {1, 20}}},
            {25, FacilityShape::single, 12, {{2, 10}}}};
}

NeighbourhoodResult derive(const std::vector<NeighbourDefinition> &definitions,
                           const std::vector<FacilityPlacement> &placements,
                           const std::vector<Position> &roads = {}, int width = 9, int height = 8) {
    const auto result =
        derive_facility_neighbourhood(definitions, placements, roads, width, height);
    check(result.error == NeighbourhoodError::none && result.facilities.size() == placements.size(),
          "valid neighbourhood derivation succeeds");
    return result;
}

std::set<std::pair<int, int>> coordinates(const std::vector<Position> &cells) {
    std::set<std::pair<int, int>> result;
    for (const auto cell : cells) {
        result.emplace(cell.x, cell.y);
    }
    return result;
}

std::vector<Position> oracle_footprint(const FacilityPlacement &p) {
    std::vector<Position> result{p.anchor};
    if (p.shape == FacilityShape::pair) {
        result.push_back(p.orientation == FacilityOrientation::first
                             ? Position{p.anchor.x, p.anchor.y + 1}
                             : Position{p.anchor.x - 1, p.anchor.y});
    } else if (p.shape == FacilityShape::square) {
        result.push_back({p.anchor.x - 1, p.anchor.y});
        result.push_back({p.anchor.x, p.anchor.y + 1});
        result.push_back({p.anchor.x - 1, p.anchor.y + 1});
    }
    return result;
}

bool near(Position a, Position b) { return std::abs(a.x - b.x) <= 1 && std::abs(a.y - b.y) <= 1; }

bool near_footprint(Position position, const std::vector<Position> &cells) {
    return std::any_of(cells.begin(), cells.end(),
                       [&](Position cell) { return near(position, cell); });
}

void ordered_rings_and_exhaustive_clipping() {
    const std::vector<Position> single = {{0, 1},  {1, 1},   {1, 0},  {1, -1},
                                          {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}};
    const std::vector<Position> pair_first = {{0, 2},  {1, 2},   {1, 1},  {1, 0},  {1, -1},
                                              {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {-1, 2}};
    const std::vector<Position> pair_second = {{1, 0},  {1, -1}, {0, -1}, {-1, -1}, {-2, -1},
                                               {-2, 0}, {-2, 1}, {-1, 1}, {0, 1},   {1, 1}};
    const std::vector<Position> square = {{1, 0},  {1, -1}, {0, -1}, {-1, -1}, {-2, -1}, {-2, 0},
                                          {-2, 1}, {-2, 2}, {-1, 2}, {0, 2},   {1, 2},   {1, 1}};
    for (const auto shape : {FacilityShape::single, FacilityShape::pair, FacilityShape::square}) {
        for (const auto orientation : {FacilityOrientation::first, FacilityOrientation::second}) {
            auto expected = shape == FacilityShape::single              ? single
                            : shape == FacilityShape::square            ? square
                            : orientation == FacilityOrientation::first ? pair_first
                                                                        : pair_second;
            for (auto &position : expected) {
                position.x += 3;
                position.y += 3;
            }
            const auto ring = facility_surroundings(shape, orientation, {3, 3}, 9, 8);
            check(ring.error == GeometryError::none && ring.cells == expected,
                  "full outer ring order matches fixed source table");
            for (int x = -2; x <= 9; ++x) {
                for (int y = -2; y <= 9; ++y) {
                    const auto p = placement(1, 28, {x, y}, shape, orientation);
                    const auto occupied = oracle_footprint(p);
                    const bool inside =
                        std::all_of(occupied.begin(), occupied.end(), [](Position cell) {
                            return cell.x >= 0 && cell.y >= 0 && cell.x < 8 && cell.y < 7;
                        });
                    const auto actual = facility_surroundings(shape, orientation, {x, y}, 8, 7);
                    if (!inside) {
                        check(actual.error == GeometryError::outside_map && actual.cells.empty(),
                              "outside occupied footprint refused rather than clipped");
                        continue;
                    }
                    std::vector<Position> oracle;
                    for (int cy = 0; cy < 7; ++cy) {
                        for (int cx = 0; cx < 8; ++cx) {
                            const Position cell{cx, cy};
                            if (std::find(occupied.begin(), occupied.end(), cell) ==
                                    occupied.end() &&
                                near_footprint(cell, occupied)) {
                                oracle.push_back(cell);
                            }
                        }
                    }
                    check(actual.error == GeometryError::none &&
                              coordinates(actual.cells) == coordinates(oracle),
                          "clipped ring equals independent eight-neighbour set");
                    check(coordinates(actual.cells).size() == actual.cells.size(),
                          "outer ring never duplicates corner tiles");
                }
            }
        }
    }
    check(facility_surroundings(static_cast<FacilityShape>(3), FacilityOrientation::first, {}, 3, 3)
                  .error == GeometryError::invalid_shape,
          "invalid perimeter shape refused");
    check(
        facility_surroundings(FacilityShape::single, static_cast<FacilityOrientation>(2), {}, 3, 3)
                .error == GeometryError::invalid_orientation,
        "invalid perimeter orientation refused");
    check(
        facility_surroundings(FacilityShape::single, FacilityOrientation::first, {}, 0, 3).error ==
            GeometryError::invalid_bounds,
        "invalid perimeter bounds refused");
    check(facility_surroundings(FacilityShape::single, FacilityOrientation::first,
                                {INT_MAX, INT_MIN}, 3, 3)
                  .error == GeometryError::outside_map,
          "extreme anchor refused before ring arithmetic");
}

void original_records_and_road_effects() {
    const std::vector<FacilityPlacement> placements = {
        placement(1, 28, {3, 3}), placement(2, 36, {4, 3}), placement(3, 66, {2, 3}),
        placement(4, 25, {3, 2})};
    const auto result = derive(original_examples(), placements, {{4, 4}});
    check(result.facilities[0].modifiers == std::array<std::int64_t, 3>{20, 5, 12} &&
              result.facilities[0].road_cells == 1,
          "inn receives sunflower price quality, cafe appeal and diagonal road appeal");
    check(result.facilities[1].modifiers == std::array<std::int64_t, 3>{0, 0, 12},
          "cafe receives inn appeal but distant sunflower does not reach it");
    check(result.facilities[2].modifiers == std::array<std::int64_t, 3>{} &&
              result.facilities[3].modifiers == std::array<std::int64_t, 3>{},
          "decoration and house are not merchant targets");
    check(result.facilities[0].sources.size() == 2 &&
              result.facilities[0].sources[0].instance_id.value == 2 &&
              result.facilities[0].sources[1].instance_id.value == 3,
          "house with populated modifier column does not propagate, source order preserved");
    const auto repeated = derive(original_examples(), placements, {{4, 4}});
    check(repeated.facilities[0].modifiers == result.facilities[0].modifiers,
          "complete recomputation is not cumulative application");
    const auto without_road = derive(original_examples(), placements);
    check(without_road.facilities[0].modifiers[2] == 10,
          "removed road removes its modifier on recomputation");
    for (const auto shape : {FacilityShape::single, FacilityShape::pair, FacilityShape::square}) {
        const auto id = static_cast<std::int32_t>(1000 + static_cast<int>(shape));
        const NeighbourDefinition definition{id, shape, 3, {}};
        for (const auto orientation : {FacilityOrientation::first, FacilityOrientation::second}) {
            const auto building = placement(1, id, {3, 3}, shape, orientation);
            const auto roads = facility_surroundings(shape, orientation, {3, 3}, 9, 8).cells;
            const auto maximum = derive({definition}, {building}, roads);
            check(maximum.facilities[0].road_cells == roads.size() &&
                      maximum.facilities[0].modifiers[2] ==
                          static_cast<std::int64_t>(roads.size() * 2),
                  "all 8/10/12 surrounding road tiles count once including diagonals");
        }
    }
}

void deduplication_same_definition_and_rotation() {
    auto catalog = original_examples();
    catalog.push_back({900, FacilityShape::single, 3, {}});
    const std::vector<FacilityPlacement> buildings = {
        placement(1, 29, {3, 2}, FacilityShape::pair), placement(2, 66, {2, 2}),
        placement(3, 66, {4, 2}), placement(4, 900, {2, 3})};
    const auto result = derive(catalog, buildings);
    check(result.facilities[0].modifiers == std::array<std::int64_t, 3>{40, 10, 0},
          "two same-definition sources stack, touching multiple target cells does not multiply");
    check(result.facilities[0].sources.size() == 3 &&
              result.facilities[0].sources[0].instance_id.value == 2 &&
              result.facilities[0].sources[1].instance_id.value == 3 &&
              result.facilities[0].sources[2].instance_id.value == 4,
          "empty modifier source still registered with distinct instance identity");
    const auto removed = derive(catalog, {buildings[0], buildings[2]});
    check(removed.facilities[0].modifiers == std::array<std::int64_t, 3>{20, 5, 0} &&
              removed.facilities[0].sources.size() == 1,
          "removing source drops its effect and identity without touching shared definition");
    auto pair = placement(2, 29, {5, 2}, FacilityShape::pair);
    const auto target = placement(1, 28, {3, 3});
    check(derive(catalog, {target, pair}).facilities[0].modifiers[2] == 0,
          "vertical pair not adjacent in fixture");
    pair.orientation = FacilityOrientation::second;
    check(derive(catalog, {target, pair}).facilities[0].modifiers[2] == 10,
          "rotation changes ring and introduces diagonal adjacency");
}

void refusal_and_numeric_boundaries() {
    auto catalog = original_examples();
    const auto target = placement(1, 28, {2, 2});
    const auto expect = [&](const std::vector<NeighbourDefinition> &definitions,
                            const std::vector<FacilityPlacement> &placements,
                            const std::vector<Position> &roads, NeighbourhoodError error) {
        const auto result = derive_facility_neighbourhood(definitions, placements, roads, 6, 6);
        check(result.error == error && result.facilities.empty(), "refusal has no partial output");
    };
    expect({}, {target}, {}, NeighbourhoodError::invalid_definition);
    auto bad_catalog = catalog;
    bad_catalog.push_back(catalog.front());
    expect(bad_catalog, {target}, {}, NeighbourhoodError::invalid_definition);
    bad_catalog = catalog;
    bad_catalog[0].shape = FacilityShape::pair;
    expect(bad_catalog, {target}, {}, NeighbourhoodError::invalid_definition);
    bad_catalog = catalog;
    bad_catalog[0].modifiers = {{3, 1}};
    expect(bad_catalog, {target}, {}, NeighbourhoodError::invalid_definition);
    bad_catalog[0].modifiers = {{-1, 1}};
    expect(bad_catalog, {target}, {}, NeighbourhoodError::invalid_definition);
    bad_catalog = catalog;
    bad_catalog[0].legacy_kind = -1;
    expect(bad_catalog, {target}, {}, NeighbourhoodError::invalid_definition);
    expect(catalog, {target, target}, {}, NeighbourhoodError::invalid_layout);
    expect(catalog, {target}, {{2, 2}}, NeighbourhoodError::invalid_roads);
    expect(catalog, {target}, {{1, 1}, {1, 1}}, NeighbourhoodError::invalid_roads);
    expect(catalog, {target}, {{-1, 1}}, NeighbourhoodError::invalid_roads);
    expect(catalog, {target}, {{6, 1}}, NeighbourhoodError::invalid_roads);
    const auto small = derive_facility_neighbourhood(catalog, {}, {}, 0, 1);
    check(small.error == NeighbourhoodError::invalid_layout && small.facilities.empty(),
          "invalid world bounds have no partial output");
    std::vector<NeighbourDefinition> extremes = {{28, FacilityShape::single, 3, {}},
                                                 {66, FacilityShape::single, 2, {{0, INT64_MAX}}},
                                                 {67, FacilityShape::single, 2, {{0, 1}}}};
    const std::vector<FacilityPlacement> buildings = {target, placement(2, 66, {1, 2}),
                                                      placement(3, 67, {3, 2})};
    expect(extremes, buildings, {}, NeighbourhoodError::numeric_overflow);
    extremes[1].modifiers = {{0, INT64_MIN}};
    extremes[2].modifiers = {{0, -1}};
    expect(extremes, buildings, {}, NeighbourhoodError::numeric_overflow);
    extremes[1].modifiers = {{0, INT64_MAX}};
    extremes[2].modifiers = {{0, -INT64_MAX}};
    check(derive(extremes, buildings).facilities[0].modifiers[0] == 0,
          "signed source effects can cancel without overflow");
    extremes[1].modifiers = {{2, INT64_MAX}};
    extremes[2].modifiers.clear();
    expect(extremes, buildings, {{2, 3}}, NeighbourhoodError::numeric_overflow);
    extremes[1].modifiers = {{0, 4}, {0, 5}};
    check(derive(extremes, buildings).facilities[0].modifiers[0] == 9,
          "repeated selector entries add separately within one source");
}

void checked_economy_bridge() {
    FacilityNeighbourhood neighbourhood;
    neighbourhood.modifiers = {20, 5, 10};
    FacilityEconomyInput base;
    base.level = 1;
    base.instance_modifiers = {999, 999, 999, 123};
    base.legacy_job_counts[0] = 2;
    const auto input = neighbourhood_economy_input(neighbourhood, base);
    check(input.has_value() &&
              input->instance_modifiers == std::array<std::int32_t, 4>{20, 5, 10, 123} &&
              input->legacy_job_counts == base.legacy_job_counts && input->level == 1,
          "bridge replaces stale modifiers and preserves other economy input");
    FacilityEconomyDefinition inn;
    inn.attributes = {LevelEndpoints{300, 450}, LevelEndpoints{5, 50}, LevelEndpoints{5, 50},
                      LevelEndpoints{240, 480}};
    inn.upgrade_uses = {50, 500};
    const auto values = derive_facility_economy(inn, *input);
    check(values.error == FacilityEconomyError::none &&
              values.values->instance_attributes == std::array<std::int64_t, 4>{320, 10, 15, 240},
          "neighbour modifiers feed price quality appeal but not maintenance");
    neighbourhood.modifiers = {1000, 1000, 1000};
    const auto capped = derive_facility_economy(inn, *neighbourhood_economy_input(neighbourhood));
    check(capped.values->instance_attributes == std::array<std::int64_t, 4>{900, 100, 100, 240},
          "economy consumer performs cap after full neighbour derivation");
    neighbourhood.modifiers = {INT32_MIN, INT32_MAX, 0};
    check(neighbourhood_economy_input(neighbourhood).has_value(),
          "exact narrow range edges accepted");
    neighbourhood.modifiers[0] = static_cast<std::int64_t>(INT32_MIN) - 1;
    check(!neighbourhood_economy_input(neighbourhood).has_value(), "negative narrowing refused");
    neighbourhood.modifiers[0] = static_cast<std::int64_t>(INT32_MAX) + 1;
    check(!neighbourhood_economy_input(neighbourhood).has_value(), "positive narrowing refused");
}

void explicit_road_bindings() {
    const auto definitions = original_examples();
    const std::vector<FacilityPlacement> placements{placement(1, 28, {3, 3}),
                                                    placement(2, 36, {4, 3})};
    const std::vector<Position> roads{{4, 3}};
    const NeighbourRoadBinding binding{{4, 3}, {2}, 36, 0};
    check(derive_facility_neighbourhood(definitions, placements, roads, 9, 8).error ==
              NeighbourhoodError::invalid_roads,
          "ordinary layout API still rejects implicit road and owner overlap");
    // 当前地图投影合同夹具；不声称普通kind3可直接被玩家道路覆盖。
    const auto mixed =
        derive_facility_neighbourhood(definitions, placements, roads, 9, 8, {binding});
    check(mixed.error == NeighbourhoodError::none && mixed.facilities.size() == 2 &&
              mixed.facilities[0].modifiers == std::array<std::int64_t, 3>{0, 0, 12} &&
              mixed.facilities[0].road_cells == 1 && mixed.facilities[0].sources.size() == 1 &&
              mixed.facilities[0].sources[0].instance_id.value == 2 &&
              mixed.facilities[1].modifiers == std::array<std::int64_t, 3>{0, 0, 10} &&
              mixed.facilities[1].road_cells == 0,
          "explicit same-cell owner gives source appeal10 and separate road2 without self road");
    for (int fault = 0; fault < 8; ++fault) {
        auto supplied = std::vector<NeighbourRoadBinding>{binding};
        auto current_roads = roads;
        if (fault == 0)
            supplied[0].instance_id.value = 999;
        else if (fault == 1)
            supplied[0].definition_id = 28;
        else if (fault == 2)
            supplied[0].fragment_index = 1;
        else if (fault == 3)
            supplied.push_back(binding);
        else if (fault == 4)
            supplied[0].position = {8, 7}; // 无owner，不能用授权列表造绑定。
        else if (fault == 5)
            supplied[0].position = {-1, 3};
        else if (fault == 6)
            current_roads = {{2, 3}}; // 绑定本身真实，但不是本次state3集合。
        else
            current_roads.push_back({4, 3});
        check(
            derive_facility_neighbourhood(definitions, placements, current_roads, 9, 8, supplied)
                    .error == NeighbourhoodError::invalid_roads,
            "explicit roads reject wrong ID/definition/fragment, duplicate, absent owner or road");
    }
}

void random_layout_differential() {
    std::mt19937 random(0xAE2026U);
    const auto catalog = original_examples();
    for (int trial = 0; trial < 200; ++trial) {
        std::vector<FacilityPlacement> buildings;
        for (int attempt = 0; attempt < 40 && buildings.size() < 10; ++attempt) {
            const auto &definition = catalog[random() % catalog.size()];
            const auto candidate =
                placement(buildings.size() + 1, definition.definition_id,
                          {static_cast<int>(random() % 9), static_cast<int>(random() % 8)},
                          definition.shape, static_cast<FacilityOrientation>(random() % 2));
            if (evaluate_facility_placement(buildings, candidate, 9, 8).error ==
                GeometryError::none) {
                buildings.push_back(candidate);
            }
        }
        std::vector<std::vector<Position>> footprints;
        std::vector<const NeighbourDefinition *> definitions;
        for (const auto &building : buildings) {
            footprints.push_back(oracle_footprint(building));
            definitions.push_back(
                &*std::find_if(catalog.begin(), catalog.end(), [&](const auto &d) {
                    return d.definition_id == building.definition_id;
                }));
        }
        std::vector<Position> roads;
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 9; ++x) {
                const Position cell{x, y};
                const bool occupied =
                    std::any_of(footprints.begin(), footprints.end(), [&](const auto &f) {
                        return std::find(f.begin(), f.end(), cell) != f.end();
                    });
                if (!occupied && random() % 4 == 0) {
                    roads.push_back(cell);
                }
            }
        }
        const auto actual = derive(catalog, buildings, roads);
        for (std::size_t target = 0; target < buildings.size(); ++target) {
            std::array<std::int64_t, 3> expected{};
            std::vector<std::uint64_t> sources;
            std::size_t road_count = 0;
            if (definitions[target]->legacy_kind == 3) {
                for (std::size_t source = 0; source < buildings.size(); ++source) {
                    const auto kind = definitions[source]->legacy_kind;
                    if (source == target || (kind != 2 && kind != 3)) {
                        continue;
                    }
                    const bool touches = std::any_of(
                        footprints[target].begin(), footprints[target].end(),
                        [&](Position cell) { return near_footprint(cell, footprints[source]); });
                    if (touches) {
                        for (const auto modifier : definitions[source]->modifiers) {
                            expected[static_cast<std::size_t>(modifier.attribute_slot)] +=
                                modifier.delta;
                        }
                        sources.push_back(buildings[source].instance_id.value);
                    }
                }
                for (const auto road : roads) {
                    if (near_footprint(road, footprints[target])) {
                        expected[2] += 2;
                        ++road_count;
                    }
                }
            }
            const auto &value = actual.facilities[target];
            check(value.modifiers == expected && value.road_cells == road_count,
                  "random effect aggregate matches independent Chebyshev neighbourhood oracle");
            check(value.sources.size() == sources.size(),
                  "random source count matches deduplicated oracle");
            for (std::size_t index = 0; index < sources.size(); ++index) {
                check(value.sources[index].instance_id.value == sources[index],
                      "random source identity and input order match oracle");
            }
        }
    }
}

} // namespace

int main() {
    ordered_rings_and_exhaustive_clipping();
    original_records_and_road_effects();
    deduplication_same_definition_and_rotation();
    refusal_and_numeric_boundaries();
    checked_economy_bridge();
    explicit_road_bindings();
    random_layout_differential();
    std::cout << checks << " checks passed\n";
}
