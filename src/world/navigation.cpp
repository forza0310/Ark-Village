// Maintained map_access rules. This module neither approves construction nor chooses AI goals.
#include "ark/world/navigation.hpp"
#include <algorithm>
#include <array>
#include <functional>
#include <queue>
#include <stdexcept>
namespace ark::world {
namespace {
constexpr std::array<Cell, 4> directions{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
Cell position(const RouteMap &map, std::size_t i) {
    return {static_cast<int>(i % map.width), static_cast<int>(i / map.width)};
}
bool valid_cell(const RouteCell &cell) {
    const int category = static_cast<int>(cell.category);
    return category >= 0 && category < 5 && cell.legacy_state >= 0 && cell.legacy_state <= 12 &&
           cell.definition_id >= 0 && cell.direction >= -1 && cell.direction <= 3 &&
           (!cell.facility ||
            (cell.facility->instance && cell.facility->definition_id == cell.definition_id &&
             cell.facility->fragment >= 0 && cell.facility->fragment <= 7));
}
bool may_expand(const RouteCell &cell) {
    return cell.facility ||
           (cell.category != RouteCategory::terminal && cell.category != RouteCategory::blocked);
}
std::int64_t departure_cost(const RouteCell &cell, Cell direction) {
    return cell.category == RouteCategory::ground ? (direction.x == 0 ? 70 : 50)
                                                  : (direction.x == 0 ? 7 : 5);
}
} // namespace
bool RouteMap::contains(Cell p) const {
    return p.x >= 0 && p.y >= 0 && p.x < width && p.y < height;
}
std::size_t RouteMap::index(Cell p) const {
    if (!contains(p))
        throw std::out_of_range("Route cell outside map");
    return static_cast<std::size_t>(p.y) * width + p.x;
}
bool valid_map(const RouteMap &map) {
    return map.width > 0 && map.height > 0 &&
           static_cast<std::int64_t>(map.width) * map.height <= 1000000 &&
           map.cells.size() == static_cast<std::size_t>(map.width) * map.height &&
           std::all_of(map.cells.begin(), map.cells.end(), valid_cell);
}
bool route_transition(const RouteCell &from, const RouteCell &to, bool first) {
    if (!valid_cell(from) || !valid_cell(to))
        return false;
    if (first && (to.legacy_state == 3 || to.legacy_state == 4))
        return true;
    return (from.category == RouteCategory::road || from.category == RouteCategory::ground ||
            from.category == RouteCategory::access) &&
           to.category != RouteCategory::blocked;
}
SearchResult search(const RouteMap &map, Cell start, SearchLimits limits) {
    if (!valid_map(map))
        return {RouteError::invalid_map, {}};
    if (!map.contains(start))
        return {RouteError::invalid_position, {}};
    if (limits.max_cost < 0 || (limits.max_expanded_cost && *limits.max_expanded_cost < 0))
        return {RouteError::invalid_limits, {}};
    DistanceField field{map, start, {}, {}, 0, limits.allow_first_step_exit};
    field.distances.resize(map.cells.size());
    field.previous.resize(map.cells.size());
    const auto origin = map.index(start);
    field.distances[origin] = 0;
    using Node = std::pair<std::int64_t, std::size_t>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> frontier;
    const auto queue_index = [&](std::size_t index) {
        return limits.reverse_equal_cost ? map.cells.size() - 1 - index : index;
    };
    if (may_expand(map.cells[origin]))
        frontier.emplace(0, queue_index(origin));
    std::vector<bool> settled(map.cells.size());
    bool cost_pruned{};
    while (!frontier.empty()) {
        const auto [cost, queued_index] = frontier.top();
        const auto index = queue_index(queued_index);
        frontier.pop();
        if (settled[index] || field.distances[index] != cost)
            continue;
        if (limits.max_expanded_cost && cost > *limits.max_expanded_cost)
            break;
        if (field.expanded == limits.max_expansions)
            return {RouteError::expansion_limit, {}};
        settled[index] = true;
        ++field.expanded;
        const auto from = position(map, index);
        for (const auto d : directions) {
            const Cell to{from.x + d.x, from.y + d.y};
            if (!map.contains(to))
                continue;
            const auto next = map.index(to);
            if (settled[next] || !route_transition(map.cells[index], map.cells[next],
                                                   index == origin && limits.allow_first_step_exit))
                continue;
            const auto step = departure_cost(map.cells[index], d);
            if (step > limits.max_cost || cost > limits.max_cost - step) {
                cost_pruned = cost_pruned || !field.distances[next];
                continue;
            }
            const auto total = cost + step;
            if (!field.distances[next] || total < *field.distances[next]) {
                field.distances[next] = total;
                field.previous[next] = index;
                if (may_expand(map.cells[next]))
                    frontier.emplace(total, queue_index(next));
            }
        }
    }
    if (cost_pruned)
        return {RouteError::cost_limit, {}};
    return {RouteError::none, std::move(field)};
}
bool valid_field(const DistanceField &field) {
    const auto &map = field.map;
    if (!valid_map(map) || !map.contains(field.start) ||
        field.distances.size() != map.cells.size() || field.previous.size() != map.cells.size() ||
        field.expanded > map.cells.size())
        return false;
    const auto origin = map.index(field.start);
    if (field.distances[origin] != 0 || field.previous[origin])
        return false;
    for (std::size_t i = 0; i < map.cells.size(); ++i) {
        if (!field.distances[i]) {
            if (field.previous[i])
                return false;
            continue;
        }
        if (*field.distances[i] < 0)
            return false;
        if (i == origin)
            continue;
        if (!field.previous[i] || *field.previous[i] >= map.cells.size())
            return false;
        const auto previous = *field.previous[i];
        if (!field.distances[previous] || !may_expand(map.cells[previous]))
            return false;
        const auto here = position(map, i), before = position(map, previous);
        const Cell d{here.x - before.x, here.y - before.y};
        if (std::find(directions.begin(), directions.end(), d) == directions.end() ||
            !route_transition(map.cells[previous], map.cells[i],
                              previous == origin && field.allow_first_step_exit))
            return false;
        const auto step = departure_cost(map.cells[previous], d);
        if (*field.distances[previous] > std::numeric_limits<std::int64_t>::max() - step ||
            *field.distances[i] != *field.distances[previous] + step)
            return false;
    }
    return true;
}
Route trace(const DistanceField &field, Cell goal) {
    if (!valid_field(field))
        return {RouteError::invalid_field, {}, 0};
    if (!field.map.contains(goal))
        return {RouteError::invalid_position, {}, 0};
    const auto target = field.map.index(goal), origin = field.map.index(field.start);
    if (!field.distances[target])
        return {RouteError::unreachable, {}, 0};
    std::vector<Cell> steps;
    for (auto cursor = target; cursor != origin; cursor = *field.previous[cursor])
        steps.push_back(position(field.map, cursor));
    std::reverse(steps.begin(), steps.end());
    return {RouteError::none, std::move(steps), *field.distances[target]};
}
bool arrival_matches(const RouteMap &map, const ArrivalTarget &target, Cell current) {
    if (!valid_map(map) || !map.contains(current) || current != target.cell || !target.instance ||
        target.definition_id < 0)
        return false;
    const auto &binding = map.cells[map.index(current)].facility;
    return binding && binding->instance == target.instance &&
           binding->definition_id == target.definition_id;
}
} // namespace ark::world
