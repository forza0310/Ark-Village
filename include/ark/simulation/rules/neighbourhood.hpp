#pragma once

// Neighbour modifiers use source instance identity; an occupied footprint is not counted per
// fragment.

#include "ark/simulation/rules/facility_economy.hpp"
#include "ark/simulation/rules/geometry.hpp"

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

// Recompute ordered source lists and road charm from a fully validated layout.
NeighbourhoodResult
derive_facility_neighbourhood(const std::vector<NeighbourDefinition> &definitions,
                              const std::vector<FacilityPlacement> &placements,
                              const std::vector<Position> &roads, int width, int height);

// Add the three instance modifiers to the base input, returning nullopt on narrowing overflow.
std::optional<FacilityEconomyInput>
neighbourhood_economy_input(const FacilityNeighbourhood &neighbourhood,
                            const FacilityEconomyInput &base = {});

} // namespace ark::simulation::rules
