#include "dungeon_village_reference/map_access.hpp"

#include <array>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <random>

using namespace dungeon_village_reference;

namespace {

int checks = 0;
constexpr std::array<Position, 4> directions = {Position{0, 1}, Position{1, 0}, Position{0, -1},
                                                Position{-1, 0}};

void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

LegacyMap ground_map(int width, int height) {
    return {width, height, std::vector<LegacyMapCell>(static_cast<std::size_t>(width * height))};
}

std::size_t index_of(const LegacyMap &map, Position position) {
    return static_cast<std::size_t>(position.y * map.width + position.x);
}

bool within(const LegacyMap &map, Position position) {
    return position.x >= 0 && position.y >= 0 && position.x < map.width && position.y < map.height;
}

FacilityPlacement placement(std::uint64_t id, FacilityShape shape, FacilityOrientation orientation,
                            Position anchor, std::int32_t definition = 28) {
    return {{id}, definition, shape, orientation, anchor};
}

LegacyDistanceField search(const LegacyMap &map, Position start, bool first_step = true) {
    const auto result = search_legacy_map(map, start, {INT64_MAX, 1000000, first_step});
    check(result.error == MapAccessError::none && result.field.has_value(), "full search succeeds");
    check(valid_legacy_distance_field(*result.field), "generated distance field validates");
    return *result.field;
}

LegacyMap bind(const LegacyMap &terrain, const std::vector<BoundFacility> &facilities) {
    const auto result = bind_facility_map(terrain, facilities);
    check(result.error == MapAccessError::none && result.map.has_value(), "map binding succeeds");
    return *result.map;
}

void transition_matrix_and_first_step() {
    for (int from = 0; from < 5; ++from) {
        for (int to = 0; to < 5; ++to) {
            const LegacyMapCell a{1, static_cast<RouteCategory>(from), std::nullopt};
            const LegacyMapCell b{1, static_cast<RouteCategory>(to), std::nullopt};
            const bool expected = (from == 1 || from == 2 || from == 4) && to != 3;
            check(legacy_route_transition(a, b) == expected, "all 25 directed category pairs");
            for (const int state : {3, 4, 5, 6}) {
                auto changed = b;
                changed.legacy_state = state;
                check(legacy_route_transition(a, changed, true) ==
                          (expected || state == 3 || state == 4),
                      "first step exception uses display state not destination category");
            }
        }
    }
    check(!legacy_route_transition({4, static_cast<RouteCategory>(99), std::nullopt}, {}),
          "invalid source category refused");
    check(!legacy_route_transition({}, {13, RouteCategory::ground, std::nullopt}, true),
          "unknown display state refused even at first step");
}

void binding_shapes_kinds_and_identity() {
    const auto terrain = ground_map(7, 7);
    for (const auto shape : {FacilityShape::single, FacilityShape::pair, FacilityShape::square}) {
        for (const auto orientation : {FacilityOrientation::first, FacilityOrientation::second}) {
            const auto building = placement(1, shape, orientation, {3, 3});
            const auto map = bind(terrain, {{building, 3}});
            const auto footprint = facility_footprint(shape, orientation, {3, 3}, 7, 7);
            for (const auto &tile : footprint.cells) {
                const auto &cell = map.cells[index_of(map, tile.position)];
                check(cell.legacy_state == 1 && cell.category == RouteCategory::terminal &&
                          cell.facility->instance_id.value == 1 &&
                          cell.facility->definition_id == 28 &&
                          cell.facility->fragment_index == tile.fragment_index,
                      "every ordered footprint tile binds to same instance with its own fragment");
            }
            check(!terrain.cells[index_of(terrain, {3, 3})].facility.has_value(),
                  "binding never mutates underlying terrain");
            const auto field = search(map, {0, 0});
            const auto status = inspect_facility_access(field, {building});
            check(status.error == MapAccessError::none && status.facilities.size() == 1 &&
                      status.facilities[0].cells.size() == footprint.cells.size(),
                  "all exterior footprint cells reachable in open terrain");
            for (std::size_t index = 0; index < footprint.cells.size(); ++index) {
                check(status.facilities[0].cells[index].cell.position ==
                              footprint.cells[index].position &&
                          status.facilities[0].cells[index].cell.fragment_index ==
                              footprint.cells[index].fragment_index,
                      "access report preserves footprint order instead of inventing entrance");
            }
        }
    }
    struct Kind {
        int value;
        int state;
        RouteCategory category;
    };
    const std::array<Kind, 9> kinds = {{{3, 1, RouteCategory::terminal},
                                        {12, 1, RouteCategory::terminal},
                                        {13, 1, RouteCategory::terminal},
                                        {1, 8, RouteCategory::terminal},
                                        {8, 9, RouteCategory::terminal},
                                        {9, 10, RouteCategory::terminal},
                                        {4, 6, RouteCategory::access},
                                        {5, 7, RouteCategory::access},
                                        {2, 2, RouteCategory::blocked}}};
    const auto building = placement(1, FacilityShape::single, FacilityOrientation::first, {3, 3});
    for (const auto kind : kinds) {
        const auto map = bind(terrain, {{building, kind.value}});
        const auto &cell = map.cells[index_of(map, {3, 3})];
        check(cell.legacy_state == kind.state && cell.category == kind.category,
              "known binding consumers map kind to separate state and path category");
    }
    for (const int kind : {-1, 0, 6, 7, 10, 11, 14}) {
        const auto result = bind_facility_map(terrain, {{building, kind}});
        check(result.error == MapAccessError::unsupported_kind && !result.map.has_value(),
              "terrain-only and unknown kinds are not silently bound as facilities");
    }
    const auto another = placement(2, FacilityShape::single, FacilityOrientation::first, {5, 5});
    const auto map = bind(terrain, {{building, 3}, {another, 3}});
    const auto status = inspect_facility_access(search(map, {0, 0}), {building, another});
    check(status.facilities.size() == 2 && status.facilities[0].instance_id.value == 1 &&
              status.facilities[1].instance_id.value == 2,
          "same definition instances stay distinct");
}

void terminal_transit_and_multi_cell_access() {
    const auto building = placement(1, FacilityShape::single, FacilityOrientation::first, {1, 0});
    const auto map = bind(ground_map(3, 1), {{building, 3}});
    const auto field = search(map, {0, 0});
    check(field.distances[1] == 50 && !field.distances[2].has_value(),
          "can arrive at shop but cannot walk through it");
    const auto path = trace_legacy_path(field, {1, 0});
    check(path.error == MapAccessError::none && path.cost == 50 &&
              path.steps == std::vector<Position>{{1, 0}},
          "path ends inside occupied tile");
    check(trace_legacy_path(field, {2, 0}).error == MapAccessError::unreachable,
          "beyond shop is unreachable");
    const auto inside = search(map, {1, 0});
    check(inside.distances[0] == 5 && inside.distances[2] == 5,
          "new search can leave occupied terminal via first step");
    const auto no_exit = search(map, {1, 0}, false);
    check(!no_exit.distances[0].has_value() && !no_exit.distances[2].has_value(),
          "first-step exit is explicit");
    auto no_facility = ground_map(3, 1);
    no_facility.cells[1] = {1, RouteCategory::terminal, std::nullopt};
    const auto isolated = search(no_facility, {1, 0});
    check(isolated.expanded == 0 && !isolated.distances[0].has_value(),
          "unbound category zero is excluded from expansion set");
    const auto destination = search(no_facility, {0, 0});
    check(destination.distances[1] == 50 && !destination.distances[2].has_value(),
          "excluded terminal can get finite distance but cannot expand");
    const auto decoration = bind(ground_map(3, 1), {{building, 2}});
    check(!search(decoration, {0, 0}).distances[1].has_value(),
          "ordinary step cannot enter decoration category three");
    check(search(decoration, {1, 0}).distances[2] == 5,
          "bound category three start can exit to ground");
    auto entrance_terrain = ground_map(2, 1);
    const auto source = placement(1, FacilityShape::single, FacilityOrientation::first, {0, 0});
    const auto entrance =
        placement(2, FacilityShape::single, FacilityOrientation::first, {1, 0}, 75);
    const auto entrance_map = bind(entrance_terrain, {{source, 3}, {entrance, 4}});
    check(!search(entrance_map, {0, 0}).distances[1].has_value(),
          "terminal does not gain arbitrary first-step access to entrance state six");
    check(search(entrance_map, {1, 0}).distances[0] == 5,
          "entrance can enter terminal in reverse direction");

    auto enclosed = ground_map(4, 4);
    for (auto &cell : enclosed.cells) {
        cell = {5, RouteCategory::blocked, std::nullopt};
    }
    enclosed.cells[index_of(enclosed, {2, 3})] = {};
    const auto pair = placement(7, FacilityShape::pair, FacilityOrientation::first, {2, 1}, 29);
    const auto pair_map = bind(enclosed, {{pair, 3}});
    const auto pair_field = search(pair_map, {2, 3});
    const auto status = inspect_facility_access(pair_field, {pair});
    check(status.error == MapAccessError::none && status.facilities[0].cells.size() == 1 &&
              status.facilities[0].cells[0].cell.position == Position{2, 2} &&
              status.facilities[0].cells[0].cost == 70,
          "pair connected when only non-anchor first occupied tile is reachable");
    check(!pair_field.distances[index_of(pair_map, {2, 1})].has_value(),
          "anchor need not be reachable for facility connectivity");
    const auto absent = search(pair_map, {0, 0});
    check(inspect_facility_access(absent, {pair}).facilities[0].cells.empty(),
          "disconnected facility is reported not rejected as an illegal placement");
}

void arrival_and_stale_bindings() {
    const auto building = placement(1, FacilityShape::pair, FacilityOrientation::first, {2, 1}, 29);
    const auto map = bind(ground_map(5, 5), {{building, 3}});
    const ArrivalBinding target{{2, 2}, {1}, 29};
    check(arrival_binding_matches(map, target, {2, 2}), "exact occupied target matches");
    check(!arrival_binding_matches(map, target, {2, 1}),
          "same building different tile is not original chosen arrival");
    check(!arrival_binding_matches(map, target, {-1, 0}), "out of map current position refused");
    check(!arrival_binding_matches(map, {{2, 2}, {2}, 29}, {2, 2}),
          "same definition does not replace expected instance identity");
    check(!arrival_binding_matches(map, {{2, 2}, {1}, 28}, {2, 2}),
          "definition changed at target invalidates arrival");
    check(!arrival_binding_matches(map, {{2, 2}, {0}, 29}, {2, 2}), "zero target ID refused");
    auto removed = map;
    removed.cells[index_of(removed, {2, 2})] = {};
    check(!arrival_binding_matches(removed, target, {2, 2}), "removed tile invalidates arrival");
    const auto field = search(map, {0, 0});
    auto rotated = building;
    rotated.orientation = FacilityOrientation::second;
    const auto mismatch = inspect_facility_access(field, {rotated});
    check(mismatch.error == MapAccessError::binding_mismatch && mismatch.facilities.empty(),
          "stale geometry cannot inspect old binding even when cells overlap");
    auto wrong_definition = building;
    wrong_definition.definition_id = 28;
    check(inspect_facility_access(field, {wrong_definition}).error ==
              MapAccessError::binding_mismatch,
          "access report checks definition identity");
}

void invalid_maps_limits_and_corrupt_fields() {
    check(!valid_legacy_map({}) && !valid_legacy_map({INT_MAX, INT_MAX, {}}),
          "invalid dimensions refused without allocating");
    check(!valid_legacy_map({2, 2, {LegacyMapCell{}}}), "cell count checked");
    auto map = ground_map(3, 1);
    for (const int state : {-1, 13}) {
        map.cells[1].legacy_state = state;
        check(!valid_legacy_map(map), "unknown state refused");
    }
    map = ground_map(3, 1);
    map.cells[1].facility = FacilityTileBinding{{0}, 28, 0};
    check(!valid_legacy_map(map), "invalid tile identity refused");
    map.cells[1].facility = FacilityTileBinding{{1}, -1, 0};
    check(!valid_legacy_map(map), "negative tile definition refused");
    map.cells[1].facility = FacilityTileBinding{{1}, 28, 8};
    check(!valid_legacy_map(map), "fragment range checked");
    map = ground_map(3, 1);
    check(search_legacy_map(map, {3, 0}).error == MapAccessError::invalid_position,
          "start position checked");
    check(search_legacy_map(map, {0, 0}, {-1, 100, true}).error == MapAccessError::invalid_limits,
          "negative search cost budget checked");
    const auto cost_pruned = search_legacy_map(map, {0, 0}, {99, 100, true});
    check(cost_pruned.error == MapAccessError::cost_limit && !cost_pruned.field.has_value(),
          "cost pruning cannot leak partial reachability report");
    check(search_legacy_map(map, {0, 0}, {100, 100, true}).error == MapAccessError::none,
          "inclusive complete cost boundary");
    const auto expansion_pruned = search_legacy_map(map, {0, 0}, {1000, 1, true});
    check(expansion_pruned.error == MapAccessError::expansion_limit &&
              !expansion_pruned.field.has_value(),
          "expansion pruning cannot leak partial report");
    check(search_legacy_map(map, {0, 0}, {1000, 0, true}).error == MapAccessError::expansion_limit,
          "zero expansion budget cannot search open start");
    const auto field = search(map, {0, 0});
    const auto stationary = trace_legacy_path(field, {0, 0});
    check(stationary.error == MapAccessError::none && stationary.cost == 0 &&
              stationary.steps.empty(),
          "start equals goal is valid empty path");
    check(trace_legacy_path(field, {-1, 0}).error == MapAccessError::invalid_position,
          "invalid goal refused");
    for (int corruption = 0; corruption < 8; ++corruption) {
        auto corrupt = field;
        switch (corruption) {
        case 0:
            corrupt.distances.pop_back();
            break;
        case 1:
            corrupt.previous[2] = 99;
            break;
        case 2:
            corrupt.previous[2] = 2;
            break;
        case 3:
            corrupt.previous[0] = 2;
            break;
        case 4:
            corrupt.distances[1] = INT64_MAX;
            break;
        case 5:
            corrupt.distances[1] = -1;
            break;
        case 6:
            corrupt.distances[1].reset();
            break;
        case 7:
            corrupt.map.cells[0].category = RouteCategory::terminal;
            break;
        }
        check(!valid_legacy_distance_field(corrupt), "corrupt field validation fails");
        check(trace_legacy_path(corrupt, {2, 0}).error == MapAccessError::invalid_field,
              "corrupt predecessor path refused without loop or out of bounds access");
        const auto status = inspect_facility_access(corrupt, {});
        check(status.error == MapAccessError::invalid_field && status.facilities.empty(),
              "corrupt field cannot produce even empty success report");
    }
    const auto building = placement(1, FacilityShape::single, FacilityOrientation::first, {1, 0});
    const auto collision = bind_facility_map(map, {{building, 3}, {building, 3}});
    check(collision.error == MapAccessError::invalid_layout && !collision.map.has_value(),
          "duplicate identity gives no partial map");
    auto other = building;
    other.instance_id.value = 2;
    check(bind_facility_map(map, {{building, 3}, {other, 3}}).error ==
              MapAccessError::invalid_layout,
          "overlap gives no partial map");
    other.anchor = {-1, 0};
    check(bind_facility_map(map, {{other, 3}}).error == MapAccessError::invalid_layout,
          "out of bounds footprint refused");
    const auto bound = bind(map, {{building, 3}});
    check(bind_facility_map(bound, {}).error == MapAccessError::invalid_map,
          "base terrain cannot contain hidden existing bindings");
    auto pair = building;
    pair.shape = FacilityShape::pair;
    check(inspect_facility_access(field, {pair}).error == MapAccessError::invalid_layout,
          "bad facility layout is not interpreted as disconnected");
}

// Full repeated edge relaxation is deliberately independent of the heap search.
std::vector<std::optional<std::int64_t>> oracle_distances(const LegacyMap &map, Position start,
                                                          bool first_step) {
    const auto start_index = index_of(map, start);
    std::vector<std::optional<std::int64_t>> result(map.cells.size());
    result[start_index] = 0;
    for (std::size_t pass = 0; pass < map.cells.size(); ++pass) {
        bool changed = false;
        for (std::size_t index = 0; index < map.cells.size(); ++index) {
            const auto &from = map.cells[index];
            const int category = static_cast<int>(from.category);
            if (!result[index].has_value() ||
                (!from.facility.has_value() && (category == 0 || category == 3))) {
                continue;
            }
            const Position position{static_cast<int>(index % static_cast<std::size_t>(map.width)),
                                    static_cast<int>(index / static_cast<std::size_t>(map.width))};
            for (const auto direction : directions) {
                const Position to{position.x + direction.x, position.y + direction.y};
                if (!within(map, to)) {
                    continue;
                }
                const auto next = index_of(map, to);
                const int next_category = static_cast<int>(map.cells[next].category);
                const auto next_state = map.cells[next].legacy_state;
                const bool special =
                    first_step && index == start_index && (next_state == 3 || next_state == 4);
                const bool normal =
                    (category == 1 || category == 2 || category == 4) && next_category != 3;
                if (!special && !normal) {
                    continue;
                }
                const auto total = *result[index] + (category == 2 ? (direction.x == 0 ? 70 : 50)
                                                                   : (direction.x == 0 ? 7 : 5));
                if (!result[next].has_value() || total < *result[next]) {
                    result[next] = total;
                    changed = true;
                }
            }
        }
        if (!changed) {
            break;
        }
    }
    return result;
}

void random_map_differential() {
    std::mt19937 random(0xD02026U);
    for (int trial = 0; trial < 200; ++trial) {
        auto map = ground_map(5, 4);
        for (std::size_t index = 0; index < map.cells.size(); ++index) {
            auto &cell = map.cells[index];
            cell.category = static_cast<RouteCategory>(random() % 5);
            cell.legacy_state = static_cast<int>(random() % 13);
            if (random() % 2 == 0) {
                cell.facility = FacilityTileBinding{{index + 1}, 28, 0};
            }
        }
        const Position start{static_cast<int>(random() % 5), static_cast<int>(random() % 4)};
        const bool first = random() % 2 == 0;
        const auto field = search(map, start, first);
        const auto expected = oracle_distances(map, start, first);
        check(field.distances == expected, "all random distances match independent relaxation");
        for (std::size_t index = 0; index < expected.size(); ++index) {
            const Position goal{static_cast<int>(index % 5), static_cast<int>(index / 5)};
            const auto path = trace_legacy_path(field, goal);
            if (expected[index].has_value()) {
                check(path.error == MapAccessError::none && path.cost == *expected[index],
                      "random reachable path has independently proven minimum cost");
                check(goal == start ? path.steps.empty() : path.steps.back() == goal,
                      "random path excludes start and includes goal");
            } else {
                check(path.error == MapAccessError::unreachable && path.steps.empty(),
                      "random unreachable goal produces no path");
            }
        }
    }
}

} // namespace

void source_frontier_limit() {
    const auto map = ground_map(13, 1);
    LegacySearchLimits limits;
    limits.max_expanded_cost = 500;
    limits.reverse_equal_cost = true;
    const auto bounded = search_legacy_map(map, {0, 0}, limits);
    check(bounded.field && bounded.field->expanded == 11 && bounded.field->distances[10] == 500 &&
              bounded.field->distances[11] == 550 && !bounded.field->distances[12],
          "source expansion limit keeps550 frontier from expanded500, not later600");
    check(valid_legacy_distance_field(*bounded.field) &&
              trace_legacy_path(*bounded.field, {11, 0}).error == MapAccessError::none,
          "retained frontier remains a valid route candidate above expansion limit");
    limits.max_expanded_cost = 0;
    const auto start_only = search_legacy_map(map, {0, 0}, limits);
    check(start_only.field && start_only.field->expanded == 1 &&
              start_only.field->distances[1] == 50 && !start_only.field->distances[2],
          "zero expansion cost still discovers first frontier rather than failing cost check");
    limits.max_expanded_cost = -1;
    check(search_legacy_map(map, {0, 0}, limits).error == MapAccessError::invalid_limits,
          "negative independent expansion limit rejected");
    const auto square = ground_map(2, 2);
    limits.max_expanded_cost.reset();
    const auto reversed = search_legacy_map(square, {0, 0}, limits);
    const auto normal = search_legacy_map(square, {0, 0});
    check(reversed.field && normal.field && reversed.field->distances == normal.field->distances,
          "reverse source ties preserve all shortest costs and default search contract");
    auto detour = ground_map(3, 3);
    detour.cells[4] = {0, RouteCategory::blocked, {}};
    const auto reverse_detour = search_legacy_map(detour, {1, 0}, limits);
    const auto normal_detour = search_legacy_map(detour, {1, 0});
    check(reverse_detour.field && normal_detour.field && reverse_detour.field->previous[7] == 8 &&
              normal_detour.field->previous[7] == 6 &&
              reverse_detour.field->distances[7] == normal_detour.field->distances[7],
          "source reverse equal-cost inventory chooses right detour; default keeps left tie");
}
int main() {
    source_frontier_limit();
    transition_matrix_and_first_step();
    binding_shapes_kinds_and_identity();
    terminal_transit_and_multi_cell_access();
    arrival_and_stale_bindings();
    invalid_maps_limits_and_corrupt_fields();
    random_map_differential();
    std::cout << checks << " checks passed\n";
}
