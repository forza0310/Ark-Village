#pragma once

// Facility tile bindings, legacy path admission and distance-field queries; not a game.gmap reader.

#include "dungeon_village_reference/geometry.hpp"
#include "dungeon_village_reference/navigation.hpp"

namespace dungeon_village_reference {

struct FacilityTileBinding {
    BuildingId instance_id;
    std::int32_t definition_id{};
    int fragment_index{};
};

struct LegacyMapCell {
    int legacy_state{4};
    RouteCategory category{RouteCategory::ground};
    std::optional<FacilityTileBinding> facility;
};

struct LegacyMap {
    int width{};
    int height{};
    std::vector<LegacyMapCell> cells;
};

struct BoundFacility {
    FacilityPlacement placement;
    std::int32_t legacy_kind{3};
};

enum class MapAccessError {
    none,
    invalid_map,
    invalid_position,
    invalid_limits,
    invalid_layout,
    unsupported_kind,
    invalid_field,
    binding_mismatch,
    unreachable,
    cost_limit,
    expansion_limit
};

struct MapBindingResult {
    MapAccessError error{MapAccessError::none};
    std::optional<LegacyMap> map;
};

struct LegacySearchLimits {
    std::int64_t max_cost{std::numeric_limits<std::int64_t>::max()};
    std::size_t max_expansions{1000000};
    bool allow_first_step_exit{true};
    // c.l.a(limit) stops before expanding a node above this limit, retaining discovered frontier.
    // Distinct from max_cost, which rejects over-budget edges and returns cost_limit.
    std::optional<std::int64_t> max_expanded_cost{};
    bool reverse_equal_cost{}; // c.l scans its row-major inventory backward; strict-less ties.
};

struct LegacyDistanceField {
    LegacyMap map;
    Position start;
    std::vector<std::optional<std::int64_t>> distances;
    std::vector<std::optional<std::size_t>> previous;
    std::size_t expanded{};
    bool allow_first_step_exit{true};
};

struct LegacySearchResult {
    MapAccessError error{MapAccessError::none};
    std::optional<LegacyDistanceField> field;
};

struct LegacyPathResult {
    MapAccessError error{MapAccessError::none};
    std::vector<Position> steps;
    std::int64_t cost{};
};

struct ReachableFacilityCell {
    FootprintCell cell;
    std::int64_t cost{};
};

struct FacilityAccessStatus {
    BuildingId instance_id;
    std::int32_t definition_id{};
    std::vector<ReachableFacilityCell> cells;
};

struct FacilityAccessResult {
    MapAccessError error{MapAccessError::none};
    std::vector<FacilityAccessStatus> facilities;
};

struct ArrivalBinding {
    Position goal;
    BuildingId instance_id;
    std::int32_t definition_id{};
};

bool valid_legacy_map(const LegacyMap &map);
bool valid_legacy_distance_field(const LegacyDistanceField &field);
bool legacy_route_transition(const LegacyMapCell &from, const LegacyMapCell &to,
                             bool first_expansion = false);
// Bind every footprint fragment to stable instance/definition IDs on a copy of unbound terrain.
MapBindingResult bind_facility_map(const LegacyMap &terrain,
                                   const std::vector<BoundFacility> &facilities);
// Build a full weighted distance field; cost/expansion limits are distinct from unreachable.
LegacySearchResult search_legacy_map(const LegacyMap &map, Position start,
                                     LegacySearchLimits limits = {});
// Validate the field and trace goal back to start; equal endpoints succeed with no steps.
LegacyPathResult trace_legacy_path(const LegacyDistanceField &field, Position goal);
// Validate layout bindings and collect reachable occupied cells, not outer-ring entrances.
FacilityAccessResult inspect_facility_access(const LegacyDistanceField &field,
                                             const std::vector<FacilityPlacement> &placements);
// Recheck position, instance and definition at arrival so a stale target cannot be consumed.
bool arrival_binding_matches(const LegacyMap &map, const ArrivalBinding &target, Position current);

} // namespace dungeon_village_reference
