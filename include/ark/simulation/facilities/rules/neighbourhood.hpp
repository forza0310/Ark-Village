#pragma once

// Neighbour modifiers use source instance identity; an occupied footprint is not counted per
// fragment.

#include "ark/simulation/facilities/rules/facility_economy.hpp"
#include "ark/simulation/map/rules/geometry.hpp"

#include <array>

namespace ark::simulation::rules {

struct NeighbourModifier {
    int attribute_slot{};
    std::int64_t delta{};
};

struct NeighbourDefinition {
    std::int32_t definition_id{};
    FacilityShape shape{FacilityShape::single};
    std::int32_t legacy_kind{3};
    std::vector<NeighbourModifier> modifiers;
};

struct NeighbourSource {
    BuildingId instance_id;
    std::int32_t definition_id{};
};

struct FacilityNeighbourhood {
    BuildingId instance_id;
    std::int32_t definition_id{};
    std::array<std::int64_t, 3> modifiers{};
    std::vector<NeighbourSource> sources;
    std::size_t road_cells{};
};

enum class NeighbourhoodError {
    none,
    invalid_definition,
    invalid_layout,
    invalid_roads,
    numeric_overflow
};

struct NeighbourhoodResult {
    NeighbourhoodError error{NeighbourhoodError::none};
    std::vector<FacilityNeighbourhood> facilities;
};

// 原state3道路仍可保留x。只接受地图Owner已验证的当前格绑定，不记录铺路历史。
struct NeighbourRoadBinding {
    Position position;
    BuildingId instance_id;
    std::int32_t definition_id{};
    int fragment_index{};
};

// Recompute ordered source lists and road charm from a fully validated layout.
NeighbourhoodResult
derive_facility_neighbourhood(const std::vector<NeighbourDefinition> &definitions,
                              const std::vector<FacilityPlacement> &placements,
                              const std::vector<Position> &roads, int width, int height);

// 显式双身份入口：每项必须同时属于roads与真实占地，实例/定义/片号精确相符且不重复。
// 未列出的road/owner重叠仍拒绝；实体邻接与道路每格魅力+2按原两轮分别计入。
NeighbourhoodResult
derive_facility_neighbourhood(const std::vector<NeighbourDefinition> &definitions,
                              const std::vector<FacilityPlacement> &placements,
                              const std::vector<Position> &roads, int width, int height,
                              const std::vector<NeighbourRoadBinding> &road_bindings);

// Add the three instance modifiers to the base input, returning nullopt on narrowing overflow.
std::optional<FacilityEconomyInput>
neighbourhood_economy_input(const FacilityNeighbourhood &neighbourhood,
                            const FacilityEconomyInput &base = {});

} // namespace ark::simulation::rules
