#include "ark/simulation/rules/navigation.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <random>
#include <string>

using namespace ark::simulation::rules;

namespace {

int checks = 0;

void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

RouteGrid grid(int width, int height, RouteCategory category = RouteCategory::ground) {
    return {width, height,
            std::vector<RouteCategory>(static_cast<std::size_t>(width * height), category)};
}

void validation_and_limits() {
    check(find_route({}, {}, {}).error == RouteError::invalid_grid, "empty grid rejected");
    check(find_route({INT_MAX, INT_MAX, {}}, {}, {}).error == RouteError::invalid_grid,
          "oversized dimensions rejected without allocating");
    check(find_route({2, 2, {RouteCategory::ground}}, {}, {}).error == RouteError::invalid_grid,
          "cell count mismatch rejected");
    auto map = grid(2, 2);
    map.cells[1] = static_cast<RouteCategory>(99);
    check(!valid_route_grid(map), "unknown category rejected");
    map = grid(2, 2);
    check(find_route(map, {-1, 0}, {1, 1}).error == RouteError::invalid_position,
          "negative start rejected");
    check(find_route(map, {0, 0}, {2, 1}).error == RouteError::invalid_position,
          "goal outside map rejected");
    check(find_route(map, {0, 0}, {1, 1}, {-1, 10, true}).error == RouteError::invalid_limits,
          "negative cost limit rejected");
    check(find_route(map, {0, 0}, {1, 1}, {1000, 0, true}).error == RouteError::expansion_limit,
          "zero expansion budget rejected before expanding");
    const auto stationary = find_route(map, {0, 0}, {0, 0}, {0, 0, true});
    check(stationary.error == RouteError::none && stationary.steps.empty() &&
              stationary.cost == 0 && stationary.expanded == 0,
          "same cell is valid zero-length route");
    check(find_route(map, {0, 0}, {1, 0}, {49, 100, true}).error == RouteError::cost_limit,
          "cost limit rejects more expensive route");
    check(find_route(map, {0, 0}, {1, 0}, {50, 100, true}).cost == 50, "cost limit is inclusive");
    check(find_route(map, {0, 0}, {1, 1}, {1000, 1, true}).error == RouteError::expansion_limit,
          "bounded search reports incomplete exploration");
}

void departure_costs_and_terminals() {
    auto map = grid(2, 2);
    check(find_route(map, {0, 0}, {1, 0}).cost == 50, "ground horizontal cost");
    check(find_route(map, {0, 0}, {0, 1}).cost == 70, "ground vertical cost");
    map.cells[0] = RouteCategory::road;
    check(find_route(map, {0, 0}, {1, 0}).cost == 5, "road departure to ground costs five");
    check(find_route(map, {1, 0}, {0, 0}).cost == 50, "reverse ground departure costs fifty");
    check(find_route(map, {0, 0}, {0, 1}).cost == 7, "road vertical departure costs seven");
    map = grid(3, 1);
    map.cells[1] = RouteCategory::terminal;
    check(find_route(map, {0, 0}, {1, 0}).error == RouteError::none, "terminal can be reached");
    check(find_route(map, {0, 0}, {2, 0}).error == RouteError::unreachable,
          "terminal cannot be used as transit cell");
    check(find_route(map, {1, 0}, {2, 0}).cost == 5, "terminal start can leave to ground");
    check(find_route(map, {1, 0}, {2, 0}, {1000, 100, false}).error == RouteError::unreachable,
          "terminal start exit can be disabled");
    map.cells[2] = RouteCategory::access;
    check(find_route(map, {1, 0}, {2, 0}).error == RouteError::unreachable,
          "terminal first step does not admit access category");
    check(find_route(map, {2, 0}, {1, 0}).cost == 5, "access category admits terminal");
    map.cells[1] = RouteCategory::blocked;
    check(find_route(map, {0, 0}, {1, 0}).error == RouteError::blocked_endpoint,
          "blocked endpoint rejected");
    check(find_route(map, {0, 0}, {2, 0}).error == RouteError::unreachable,
          "blocked transit prevents reachability");
}

void road_detour_is_cheaper() {
    auto map = grid(5, 3, RouteCategory::road);
    for (int x = 0; x < 5; ++x) {
        map.cells[static_cast<std::size_t>(5 + x)] = RouteCategory::ground;
    }
    const auto route = find_route(map, {0, 1}, {4, 1});
    check(route.error == RouteError::none && route.cost == 97, "road detour beats ground");
    check(route.steps.size() == 6, "cheapest path has more steps than direct path");
    check(route.steps.back() == Position{4, 1}, "goal included and start excluded");
    const auto repeated = find_route(map, {0, 1}, {4, 1});
    check(route.steps == repeated.steps, "equal-cost tie policy is deterministic");
}

// An independent exhaustive relaxation oracle checks cost, not implementation tie order.
std::int64_t oracle_cost(const RouteGrid &map, Position start, Position goal) {
    constexpr auto infinity = std::numeric_limits<std::int64_t>::max();
    std::vector<std::int64_t> costs(map.cells.size(), infinity);
    const auto index = [&map](Position p) {
        return static_cast<std::size_t>(p.y * map.width + p.x);
    };
    costs[index(start)] = 0;
    for (std::size_t iteration = 0; iteration < map.cells.size(); ++iteration) {
        bool changed = false;
        for (int y = 0; y < map.height; ++y) {
            for (int x = 0; x < map.width; ++x) {
                const auto current = index({x, y});
                if (costs[current] == infinity || map.cells[current] == RouteCategory::blocked ||
                    map.cells[current] == RouteCategory::terminal) {
                    continue;
                }
                const std::array<Position, 4> neighbors = {Position{x, y + 1}, Position{x + 1, y},
                                                           Position{x, y - 1}, Position{x - 1, y}};
                for (const auto next : neighbors) {
                    if (!within_route_grid(map, next) ||
                        map.cells[index(next)] == RouteCategory::blocked) {
                        continue;
                    }
                    const auto base = map.cells[current] == RouteCategory::ground ? 10 : 1;
                    const auto candidate = costs[current] + base * (next.x == x ? 7 : 5);
                    if (candidate < costs[index(next)]) {
                        costs[index(next)] = candidate;
                        changed = true;
                    }
                }
            }
        }
        if (!changed) {
            break;
        }
    }
    return costs[index(goal)];
}

void differential_paths() {
    std::mt19937 random(1408);
    for (int scenario = 0; scenario < 200; ++scenario) {
        auto map = grid(5, 5);
        for (auto &cell : map.cells) {
            cell = static_cast<RouteCategory>(random() % 5);
        }
        map.cells.front() = RouteCategory::ground;
        map.cells.back() = RouteCategory::terminal;
        const auto expected = oracle_cost(map, {0, 0}, {4, 4});
        const auto actual = find_route(map, {0, 0}, {4, 4});
        if (expected == std::numeric_limits<std::int64_t>::max()) {
            check(actual.error == RouteError::unreachable && actual.steps.empty(),
                  "oracle confirms unreachable");
            continue;
        }
        check(actual.error == RouteError::none && actual.cost == expected,
              "minimum cost equals independent oracle");
        auto previous = Position{0, 0};
        std::int64_t total = 0;
        for (const auto next : actual.steps) {
            check(std::abs(next.x - previous.x) + std::abs(next.y - previous.y) == 1,
                  "path is four-connected");
            const auto from = map.cells[static_cast<std::size_t>(previous.y * 5 + previous.x)];
            check(from != RouteCategory::terminal && from != RouteCategory::blocked,
                  "no blocked or terminal transit");
            total += (from == RouteCategory::ground ? 10 : 1) * (next.x == previous.x ? 7 : 5);
            previous = next;
        }
        check(previous == Position{4, 4} && total == actual.cost, "route cost matches all steps");
    }
}

} // namespace

int main() {
    validation_and_limits();
    departure_costs_and_terminals();
    road_detour_is_cheaper();
    differential_paths();
    std::cout << checks << " checks passed\n";
}
