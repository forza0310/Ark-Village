// Facility tile bindings, legacy path admission and distance-field queries; not a game.gmap reader.
// Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/rules/map_access.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <queue>
#include <utility>

namespace ark::simulation::rules {
namespace {

constexpr std::array<Position, 4> kDirections = {Position{0, 1}, Position{1, 0}, Position{0, -1},
                                                 Position{-1, 0}};
constexpr std::size_t kMaxCells = 1000000;

bool within(const LegacyMap &map, Position position) {
    return position.x >= 0 && position.y >= 0 && position.x < map.width && position.y < map.height;
}

std::size_t index_of(const LegacyMap &map, Position position) {
    return static_cast<std::size_t>(position.y) * static_cast<std::size_t>(map.width) +
           static_cast<std::size_t>(position.x);
}

Position position_of(const LegacyMap &map, std::size_t index) {
    const auto width = static_cast<std::size_t>(map.width);
    return {static_cast<int>(index % width), static_cast<int>(index / width)};
}

bool valid_cell(const LegacyMapCell &cell) {
    const auto category = static_cast<int>(cell.category);
    return category >= 0 && category < 5 && cell.legacy_state >= 0 && cell.legacy_state <= 12 &&
           (!cell.facility.has_value() ||
            (cell.facility->instance_id.value != 0 && cell.facility->definition_id >= 0 &&
             cell.facility->fragment_index >= 0 && cell.facility->fragment_index <= 7));
}

bool may_expand(const LegacyMapCell &cell) {
    return cell.facility.has_value() ||
           (cell.category != RouteCategory::terminal && cell.category != RouteCategory::blocked);
}

std::int64_t departure_cost(const LegacyMapCell &cell, Position direction) {
    return cell.category == RouteCategory::ground ? (direction.x == 0 ? 70 : 50)
                                                  : (direction.x == 0 ? 7 : 5);
}

std::optional<LegacyMapCell> facility_cell(std::int32_t kind, FacilityTileBinding binding) {
    switch (kind) {
    case 3:
    case 12:
    case 13:
        return LegacyMapCell{1, RouteCategory::terminal, binding};
    case 1:
        return LegacyMapCell{8, RouteCategory::terminal, binding};
    case 8:
        return LegacyMapCell{9, RouteCategory::terminal, binding};
    case 9:
        return LegacyMapCell{10, RouteCategory::terminal, binding};
    case 4:
        return LegacyMapCell{6, RouteCategory::access, binding};
    case 5:
        return LegacyMapCell{7, RouteCategory::access, binding};
    case 2:
        return LegacyMapCell{2, RouteCategory::blocked, binding};
    default:
        return std::nullopt;
    }
}

} // namespace

bool valid_legacy_map(const LegacyMap &map) {
    if (map.width <= 0 || map.height <= 0) {
        return false;
    }
    const auto width = static_cast<std::size_t>(map.width);
    const auto height = static_cast<std::size_t>(map.height);
    return width <= kMaxCells && height <= kMaxCells / width &&
           map.cells.size() == width * height &&
           std::all_of(map.cells.begin(), map.cells.end(), valid_cell);
}

bool legacy_route_transition(const LegacyMapCell &from, const LegacyMapCell &to,
                             bool first_expansion) {
    if (!valid_cell(from) || !valid_cell(to)) {
        return false;
    }
    if (first_expansion && (to.legacy_state == 3 || to.legacy_state == 4)) {
        return true;
    }
    const bool has_outgoing = from.category == RouteCategory::road ||
                              from.category == RouteCategory::ground ||
                              from.category == RouteCategory::access;
    return has_outgoing && to.category != RouteCategory::blocked;
}

MapBindingResult bind_facility_map(const LegacyMap &terrain,
                                   const std::vector<BoundFacility> &facilities) {
    if (!valid_legacy_map(terrain) ||
        std::any_of(terrain.cells.begin(), terrain.cells.end(),
                    [](const LegacyMapCell &cell) { return cell.facility.has_value(); })) {
        return {MapAccessError::invalid_map, std::nullopt};
    }
    std::vector<FacilityPlacement> placements;
    placements.reserve(facilities.size());
    for (const auto &facility : facilities) {
        placements.push_back(facility.placement);
    }
    if (validate_facility_layout(placements, terrain.width, terrain.height) !=
        GeometryError::none) {
        return {MapAccessError::invalid_layout, std::nullopt};
    }
    auto map = terrain;
    for (const auto &facility : facilities) {
        const auto &placement = facility.placement;
        const auto footprint = facility_footprint(placement.shape, placement.orientation,
                                                  placement.anchor, map.width, map.height);
        for (const auto &cell : footprint.cells) {
            const auto bound =
                facility_cell(facility.legacy_kind, {placement.instance_id, placement.definition_id,
                                                     cell.fragment_index});
            if (!bound.has_value()) {
                return {MapAccessError::unsupported_kind, std::nullopt};
            }
            map.cells[index_of(map, cell.position)] = *bound;
        }
    }
    return {MapAccessError::none, std::move(map)};
}

LegacySearchResult search_legacy_map(const LegacyMap &map, Position start,
                                     LegacySearchLimits limits) {
    if (!valid_legacy_map(map)) {
        return {MapAccessError::invalid_map, std::nullopt};
    }
    if (!within(map, start)) {
        return {MapAccessError::invalid_position, std::nullopt};
    }
    if (limits.max_cost < 0 || (limits.max_expanded_cost && *limits.max_expanded_cost < 0)) {
        return {MapAccessError::invalid_limits, std::nullopt};
    }
    LegacyDistanceField field{map, start, {}, {}, 0, limits.allow_first_step_exit};
    field.distances.resize(map.cells.size());
    field.previous.resize(map.cells.size());
    const auto start_index = index_of(map, start);
    field.distances[start_index] = 0;
    using Node = std::pair<std::int64_t, std::size_t>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> frontier;
    const auto queue_index = [&](std::size_t index) {
        return limits.reverse_equal_cost ? map.cells.size() - 1 - index : index;
    };
    if (may_expand(map.cells[start_index])) {
        frontier.emplace(0, queue_index(start_index));
    }
    std::vector<bool> settled(map.cells.size());
    bool pruned_by_cost = false;
    while (!frontier.empty()) {
        const auto [cost, queued_index] = frontier.top();
        const auto index = queue_index(queued_index);
        frontier.pop();
        if (settled[index] || field.distances[index] != cost) {
            continue;
        }
        if (limits.max_expanded_cost && cost > *limits.max_expanded_cost)
            break;
        if (field.expanded == limits.max_expansions) {
            return {MapAccessError::expansion_limit, std::nullopt};
        }
        settled[index] = true;
        ++field.expanded;
        const auto from = position_of(map, index);
        for (const auto direction : kDirections) {
            const Position to{from.x + direction.x, from.y + direction.y};
            if (!within(map, to)) {
                continue;
            }
            const auto next = index_of(map, to);
            const auto first = index == start_index && limits.allow_first_step_exit;
            if (settled[next] ||
                !legacy_route_transition(map.cells[index], map.cells[next], first)) {
                continue;
            }
            const auto step = departure_cost(map.cells[index], direction);
            if (step > limits.max_cost || cost > limits.max_cost - step) {
                pruned_by_cost = pruned_by_cost || !field.distances[next].has_value();
                continue;
            }
            const auto total = cost + step;
            if (!field.distances[next].has_value() || total < *field.distances[next]) {
                field.distances[next] = total;
                field.previous[next] = index;
                if (may_expand(map.cells[next])) {
                    frontier.emplace(total, queue_index(next));
                }
            }
        }
    }
    if (pruned_by_cost) {
        return {MapAccessError::cost_limit, std::nullopt};
    }
    return {MapAccessError::none, std::move(field)};
}

bool valid_legacy_distance_field(const LegacyDistanceField &field) {
    const auto &map = field.map;
    if (!valid_legacy_map(map) || !within(map, field.start) ||
        field.distances.size() != map.cells.size() || field.previous.size() != map.cells.size() ||
        field.expanded > map.cells.size()) {
        return false;
    }
    const auto start = index_of(map, field.start);
    if (field.distances[start] != 0 || field.previous[start].has_value()) {
        return false;
    }
    for (std::size_t index = 0; index < field.distances.size(); ++index) {
        if (!field.distances[index].has_value()) {
            if (field.previous[index].has_value()) {
                return false;
            }
            continue;
        }
        if (*field.distances[index] < 0) {
            return false;
        }
        if (index == start) {
            continue;
        }
        if (!field.previous[index].has_value() || *field.previous[index] >= map.cells.size()) {
            return false;
        }
        const auto previous = *field.previous[index];
        if (!field.distances[previous].has_value() || !may_expand(map.cells[previous])) {
            return false;
        }
        const auto here = position_of(map, index);
        const auto before = position_of(map, previous);
        const Position direction{here.x - before.x, here.y - before.y};
        if (std::none_of(kDirections.begin(), kDirections.end(),
                         [&](Position value) { return value == direction; }) ||
            !legacy_route_transition(map.cells[previous], map.cells[index],
                                     previous == start && field.allow_first_step_exit)) {
            return false;
        }
        const auto step = departure_cost(map.cells[previous], direction);
        if (*field.distances[previous] > std::numeric_limits<std::int64_t>::max() - step ||
            *field.distances[index] != *field.distances[previous] + step) {
            return false;
        }
    }
    return true;
}

LegacyPathResult trace_legacy_path(const LegacyDistanceField &field, Position goal) {
    if (!valid_legacy_distance_field(field)) {
        return {MapAccessError::invalid_field, {}, 0};
    }
    if (!within(field.map, goal)) {
        return {MapAccessError::invalid_position, {}, 0};
    }
    const auto start = index_of(field.map, field.start);
    const auto target = index_of(field.map, goal);
    if (!field.distances[target].has_value()) {
        return {MapAccessError::unreachable, {}, 0};
    }
    std::vector<Position> steps;
    for (auto cursor = target; cursor != start; cursor = *field.previous[cursor]) {
        steps.push_back(position_of(field.map, cursor));
    }
    std::reverse(steps.begin(), steps.end());
    return {MapAccessError::none, std::move(steps), *field.distances[target]};
}

FacilityAccessResult inspect_facility_access(const LegacyDistanceField &field,
                                             const std::vector<FacilityPlacement> &placements) {
    if (!valid_legacy_distance_field(field)) {
        return {MapAccessError::invalid_field, {}};
    }
    const auto &map = field.map;
    if (validate_facility_layout(placements, map.width, map.height) != GeometryError::none) {
        return {MapAccessError::invalid_layout, {}};
    }
    std::vector<FacilityAccessStatus> statuses;
    for (const auto &placement : placements) {
        FacilityAccessStatus status{placement.instance_id, placement.definition_id, {}};
        const auto footprint = facility_footprint(placement.shape, placement.orientation,
                                                  placement.anchor, map.width, map.height);
        for (const auto &cell : footprint.cells) {
            const auto index = index_of(map, cell.position);
            const auto &binding = map.cells[index].facility;
            if (!binding.has_value() || !(binding->instance_id == placement.instance_id) ||
                binding->definition_id != placement.definition_id ||
                binding->fragment_index != cell.fragment_index) {
                return {MapAccessError::binding_mismatch, {}};
            }
            if (field.distances[index].has_value()) {
                status.cells.push_back({cell, *field.distances[index]});
            }
        }
        statuses.push_back(std::move(status));
    }
    return {MapAccessError::none, std::move(statuses)};
}

bool arrival_binding_matches(const LegacyMap &map, const ArrivalBinding &target, Position current) {
    if (!valid_legacy_map(map) || !within(map, current) || !(current == target.goal) ||
        target.instance_id.value == 0 || target.definition_id < 0) {
        return false;
    }
    const auto &binding = map.cells[index_of(map, current)].facility;
    return binding.has_value() && binding->instance_id == target.instance_id &&
           binding->definition_id == target.definition_id;
}

} // namespace ark::simulation::rules
