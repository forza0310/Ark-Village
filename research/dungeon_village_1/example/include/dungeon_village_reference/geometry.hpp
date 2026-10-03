#pragma once

#include "dungeon_village_reference/domain.hpp"

namespace dungeon_village_reference {

enum class FacilityShape { single, pair, square };
enum class FacilityOrientation { first, second };
enum class GeometryError {
    none,
    invalid_shape,
    invalid_orientation,
    invalid_bounds,
    invalid_identity,
    invalid_layout,
    outside_map,
    overlap
};

struct FootprintCell {
    Position position;
    int fragment_index{};
};

struct FacilityPlacement {
    BuildingId instance_id;
    std::int32_t definition_id{};
    FacilityShape shape{FacilityShape::single};
    FacilityOrientation orientation{FacilityOrientation::first};
    Position anchor;
};

struct GeometryResult {
    GeometryError error{GeometryError::none};
    std::vector<FootprintCell> cells;
};

struct SurroundingCellsResult {
    GeometryError error{GeometryError::none};
    std::vector<Position> cells;
};

GeometryResult facility_footprint(FacilityShape shape, FacilityOrientation orientation,
                                  Position anchor, int width, int height);
SurroundingCellsResult facility_surroundings(FacilityShape shape, FacilityOrientation orientation,
                                             Position anchor, int width, int height);
GeometryError validate_facility_layout(const std::vector<FacilityPlacement> &placements, int width,
                                       int height);
GeometryResult evaluate_facility_placement(const std::vector<FacilityPlacement> &existing,
                                           const FacilityPlacement &candidate, int width,
                                           int height,
                                           std::optional<BuildingId> moving = std::nullopt);

} // namespace dungeon_village_reference
