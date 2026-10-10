#pragma once

// Weighted four-neighbour routing for the early fixture grid; map_access handles facility tile
// bindings.

#include "ark/simulation/world/rules/domain.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace ark::simulation::rules {

// Categories preserve observed route semantics, not building definition IDs.
enum class RouteCategory { terminal, road, ground, blocked, access };

struct RouteGrid {
    int width{};
    int height{};
    std::vector<RouteCategory> cells;
};

enum class RouteError {
    none,
    invalid_grid,
    invalid_position,
    invalid_limits,
    blocked_endpoint,
    unreachable,
    cost_limit,
    expansion_limit
};

struct RouteLimits {
    std::int64_t max_cost{std::numeric_limits<std::int64_t>::max()};
    std::size_t max_expansions{1000000};
    bool allow_terminal_start_exit{true};
};

struct RouteResult {
    RouteError error{RouteError::none};
    std::vector<Position> steps;
    std::int64_t cost{};
    std::size_t expanded{};
};

bool valid_route_grid(const RouteGrid &grid);
bool within_route_grid(const RouteGrid &grid, Position position);
// A successful path excludes start and includes goal; budget exhaustion is not proven
// unreachability.
RouteResult find_route(const RouteGrid &grid, Position start, Position goal,
                       RouteLimits limits = {});

} // namespace ark::simulation::rules
