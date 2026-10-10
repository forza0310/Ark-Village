// Weighted four-neighbour routing for the early fixture grid; map_access handles facility tile
// bindings. Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/map/rules/navigation.hpp"

#include <algorithm>
#include <array>
#include <functional>
#include <queue>
#include <utility>

namespace ark::simulation::rules {
namespace {

constexpr std::size_t kMaxCells = 1000000;
constexpr std::array<Position, 4> kDirections = {Position{0, 1}, Position{1, 0}, Position{0, -1},
                                                 Position{-1, 0}};

std::size_t index_of(const RouteGrid &grid, Position position) {
    return static_cast<std::size_t>(position.y) * static_cast<std::size_t>(grid.width) +
           static_cast<std::size_t>(position.x);
}

Position position_of(const RouteGrid &grid, std::size_t index) {
    const auto width = static_cast<std::size_t>(grid.width);
    return {static_cast<int>(index % width), static_cast<int>(index / width)};
}

bool may_enter(RouteCategory from, RouteCategory to, bool first_step_exit) {
    if (first_step_exit && (to == RouteCategory::road || to == RouteCategory::ground)) {
        return true;
    }
    const bool has_outgoing = from == RouteCategory::road || from == RouteCategory::ground ||
                              from == RouteCategory::access;
    return has_outgoing && to != RouteCategory::blocked;
}

} // namespace

bool valid_route_grid(const RouteGrid &grid) {
    if (grid.width <= 0 || grid.height <= 0) {
        return false;
    }
    const auto width = static_cast<std::size_t>(grid.width);
    const auto height = static_cast<std::size_t>(grid.height);
    if (width > kMaxCells || height > kMaxCells / width || grid.cells.size() != width * height) {
        return false;
    }
    return std::all_of(grid.cells.begin(), grid.cells.end(), [](RouteCategory category) {
        switch (category) {
        case RouteCategory::terminal:
        case RouteCategory::road:
        case RouteCategory::ground:
        case RouteCategory::blocked:
        case RouteCategory::access:
            return true;
        }
        return false;
    });
}

bool within_route_grid(const RouteGrid &grid, Position position) {
    return position.x >= 0 && position.y >= 0 && position.x < grid.width &&
           position.y < grid.height;
}

RouteResult find_route(const RouteGrid &grid, Position start, Position goal, RouteLimits limits) {
    if (!valid_route_grid(grid)) {
        return {RouteError::invalid_grid, {}, 0, 0};
    }
    if (!within_route_grid(grid, start) || !within_route_grid(grid, goal)) {
        return {RouteError::invalid_position, {}, 0, 0};
    }
    if (limits.max_cost < 0) {
        return {RouteError::invalid_limits, {}, 0, 0};
    }
    const auto start_index = index_of(grid, start);
    const auto goal_index = index_of(grid, goal);
    if (grid.cells[start_index] == RouteCategory::blocked ||
        grid.cells[goal_index] == RouteCategory::blocked) {
        return {RouteError::blocked_endpoint, {}, 0, 0};
    }
    const auto infinity = std::numeric_limits<std::int64_t>::max();
    std::vector<std::int64_t> distance(grid.cells.size(), infinity);
    std::vector<std::size_t> previous(grid.cells.size(), grid.cells.size());
    using QueueEntry = std::pair<std::int64_t, std::size_t>;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> frontier;
    distance[start_index] = 0;
    frontier.emplace(0, start_index);
    std::size_t expanded = 0;
    bool pruned_by_cost = false;

    while (!frontier.empty()) {
        const auto [cost, index] = frontier.top();
        frontier.pop();
        if (cost != distance[index]) {
            continue;
        }
        if (index == goal_index) {
            std::vector<Position> steps;
            for (auto cursor = goal_index; cursor != start_index; cursor = previous[cursor]) {
                steps.push_back(position_of(grid, cursor));
            }
            std::reverse(steps.begin(), steps.end());
            return {RouteError::none, std::move(steps), cost, expanded};
        }
        if (expanded == limits.max_expansions) {
            return {RouteError::expansion_limit, {}, 0, expanded};
        }
        ++expanded;
        const auto from = grid.cells[index];
        const auto position = position_of(grid, index);
        const bool first_step_exit = index == start_index && from == RouteCategory::terminal &&
                                     limits.allow_terminal_start_exit;
        for (const auto direction : kDirections) {
            const Position next{position.x + direction.x, position.y + direction.y};
            if (!within_route_grid(grid, next)) {
                continue;
            }
            const auto next_index = index_of(grid, next);
            if (!may_enter(from, grid.cells[next_index], first_step_exit)) {
                continue;
            }
            const std::int64_t step_cost = from == RouteCategory::ground
                                               ? (direction.x == 0 ? 70 : 50)
                                               : (direction.x == 0 ? 7 : 5);
            if (step_cost > limits.max_cost || cost > limits.max_cost - step_cost) {
                pruned_by_cost = true;
                continue;
            }
            const auto next_cost = cost + step_cost;
            if (next_cost < distance[next_index]) {
                distance[next_index] = next_cost;
                previous[next_index] = index;
                frontier.emplace(next_cost, next_index);
            }
        }
    }
    return {pruned_by_cost ? RouteError::cost_limit : RouteError::unreachable, {}, 0, expanded};
}

} // namespace ark::simulation::rules
