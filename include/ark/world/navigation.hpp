#pragma once

// Logical access state and weighted routing, adapted from research/example/map_access.
// Definition IDs, instance IDs, route categories and sprite frames are separate namespaces.
#include "ark/world/grid.hpp"
#include <cstdint>
#include <limits>
#include <optional>
namespace ark::world {
enum class RouteCategory { terminal, road, ground, blocked, access };
struct TileBinding {
    std::uint64_t instance{};
    int definition_id{}, fragment{};
};
struct RouteCell {
    int legacy_state{4};
    RouteCategory category{RouteCategory::ground};
    int definition_id{17}, direction{-1};
    std::optional<TileBinding> facility;
};
struct RouteMap {
    int width{}, height{};
    std::vector<RouteCell> cells;
    bool contains(Cell cell) const;
    std::size_t index(Cell cell) const;
};
struct ArrivalTarget {
    Cell cell;
    std::uint64_t instance{};
    int definition_id{};
};
enum class RouteError {
    none,
    invalid_map,
    invalid_position,
    invalid_limits,
    invalid_field,
    unreachable,
    cost_limit,
    expansion_limit,
    binding_mismatch
};
struct SearchLimits {
    std::int64_t max_cost{std::numeric_limits<std::int64_t>::max()};
    std::size_t max_expansions{1000000};
    bool allow_first_step_exit{true};
    // Stop before expanding a cost above this threshold; discovered frontier stays traceable.
    // max_cost instead rejects over-budget edges and reports cost_limit.
    std::optional<std::int64_t> max_expanded_cost{};
    bool reverse_equal_cost{}; // Original c.l scans row-major inventory backward on strict ties.
};
struct DistanceField {
    RouteMap map;
    Cell start;
    std::vector<std::optional<std::int64_t>> distances;
    std::vector<std::optional<std::size_t>> previous;
    std::size_t expanded{};
    bool allow_first_step_exit{true};
};
struct SearchResult {
    RouteError error{RouteError::none};
    std::optional<DistanceField> field;
};
struct Route {
    RouteError error{RouteError::none};
    std::vector<Cell> steps; // Excludes start, includes the occupied goal cell.
    std::int64_t cost{};
};
bool valid_map(const RouteMap &map);
bool route_transition(const RouteCell &from, const RouteCell &to, bool first_expansion = false);
// Full distance field; ground departure costs x=50/y=70, others x=5/y=7.
// Default equal-cost predecessors retain the maintained C++ policy. Live callers may explicitly
// request the published original reverse-inventory order and expansion-cost threshold.
SearchResult search(const RouteMap &map, Cell start, SearchLimits limits = {});
bool valid_field(const DistanceField &field);
Route trace(const DistanceField &field, Cell goal);
bool arrival_matches(const RouteMap &map, const ArrivalTarget &target, Cell current);
} // namespace ark::world
