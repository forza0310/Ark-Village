// Grid footprints and ordered neighbourhood rings; no screen projection or path entrance policy.
// Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/map/rules/geometry.hpp"

#include <algorithm>
#include <array>
#include <set>

namespace ark::simulation::rules {
namespace {

bool valid_bounds(int width, int height) {
    return width > 0 && height > 0 &&
           static_cast<std::int64_t>(width) * static_cast<std::int64_t>(height) <= 1000000;
}

GeometryError validate_identity(const FacilityPlacement &placement) {
    return placement.instance_id.value != 0 && placement.definition_id >= 0
               ? GeometryError::none
               : GeometryError::invalid_identity;
}

} // namespace

GeometryResult facility_footprint(FacilityShape shape, FacilityOrientation orientation,
                                  Position anchor, int width, int height) {
    if (!valid_bounds(width, height)) {
        return {GeometryError::invalid_bounds, {}};
    }
    if (orientation != FacilityOrientation::first && orientation != FacilityOrientation::second) {
        return {GeometryError::invalid_orientation, {}};
    }
    std::vector<Position> offsets;
    switch (shape) {
    case FacilityShape::single:
        offsets = {{0, 0}};
        break;
    case FacilityShape::pair:
        offsets = orientation == FacilityOrientation::first
                      ? std::vector<Position>{{0, 1}, {0, 0}}
                      : std::vector<Position>{{-1, 0}, {0, 0}};
        break;
    case FacilityShape::square:
        offsets = orientation == FacilityOrientation::first
                      ? std::vector<Position>{{-1, 1}, {-1, 0}, {0, 1}, {0, 0}}
                      : std::vector<Position>{{-1, 1}, {0, 1}, {-1, 0}, {0, 0}};
        break;
    default:
        return {GeometryError::invalid_shape, {}};
    }
    std::vector<FootprintCell> cells;
    for (std::size_t index = 0; index < offsets.size(); ++index) {
        const auto x = static_cast<std::int64_t>(anchor.x) + offsets[index].x;
        const auto y = static_cast<std::int64_t>(anchor.y) + offsets[index].y;
        if (x < 0 || y < 0 || x >= width || y >= height) {
            return {GeometryError::outside_map, {}};
        }
        const auto fragment =
            static_cast<int>(index * 2) + (orientation == FacilityOrientation::first ? 0 : 1);
        cells.push_back({{static_cast<int>(x), static_cast<int>(y)}, fragment});
    }
    return {GeometryError::none, std::move(cells)};
}

SurroundingCellsResult facility_surroundings(FacilityShape shape, FacilityOrientation orientation,
                                             Position anchor, int width, int height) {
    const auto footprint = facility_footprint(shape, orientation, anchor, width, height);
    if (footprint.error != GeometryError::none) {
        return {footprint.error, {}};
    }
    auto minimum = footprint.cells.front().position;
    auto maximum = minimum;
    for (const auto &cell : footprint.cells) {
        minimum.x = std::min(minimum.x, cell.position.x);
        minimum.y = std::min(minimum.y, cell.position.y);
        maximum.x = std::max(maximum.x, cell.position.x);
        maximum.y = std::max(maximum.y, cell.position.y);
    }
    const int left = minimum.x - 1;
    const int right = maximum.x + 1;
    const int top = minimum.y - 1;
    const int bottom = maximum.y + 1;
    std::vector<Position> ring;
    for (int y = minimum.y; y >= top; --y) {
        ring.push_back({right, y});
    }
    for (int x = right - 1; x >= left; --x) {
        ring.push_back({x, top});
    }
    for (int y = top + 1; y <= bottom; ++y) {
        ring.push_back({left, y});
    }
    for (int x = left + 1; x <= right; ++x) {
        ring.push_back({x, bottom});
    }
    for (int y = bottom - 1; y > minimum.y; --y) {
        ring.push_back({right, y});
    }
    const bool starts_below =
        shape == FacilityShape::single ||
        (shape == FacilityShape::pair && orientation == FacilityOrientation::first);
    const Position first = starts_below ? Position{minimum.x, bottom} : Position{right, minimum.y};
    std::rotate(ring.begin(), std::find(ring.begin(), ring.end(), first), ring.end());
    ring.erase(std::remove_if(ring.begin(), ring.end(),
                              [&](Position position) {
                                  return position.x < 0 || position.y < 0 || position.x >= width ||
                                         position.y >= height;
                              }),
               ring.end());
    return {GeometryError::none, std::move(ring)};
}

GeometryError validate_facility_layout(const std::vector<FacilityPlacement> &placements, int width,
                                       int height) {
    if (!valid_bounds(width, height)) {
        return GeometryError::invalid_bounds;
    }
    const auto capacity = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (placements.size() > capacity) {
        return GeometryError::invalid_layout;
    }
    std::set<BuildingId> ids;
    std::set<std::pair<int, int>> occupied;
    for (const auto &placement : placements) {
        if (validate_identity(placement) != GeometryError::none ||
            !ids.insert(placement.instance_id).second) {
            return GeometryError::invalid_identity;
        }
        const auto footprint = facility_footprint(placement.shape, placement.orientation,
                                                  placement.anchor, width, height);
        if (footprint.error != GeometryError::none) {
            return footprint.error;
        }
        for (const auto &cell : footprint.cells) {
            if (!occupied.emplace(cell.position.x, cell.position.y).second) {
                return GeometryError::overlap;
            }
        }
    }
    return GeometryError::none;
}

GeometryResult evaluate_facility_placement(const std::vector<FacilityPlacement> &existing,
                                           const FacilityPlacement &candidate, int width,
                                           int height, std::optional<BuildingId> moving) {
    const auto layout_error = validate_facility_layout(existing, width, height);
    if (layout_error != GeometryError::none) {
        return {layout_error, {}};
    }
    if (validate_identity(candidate) != GeometryError::none ||
        (moving.has_value() && !(*moving == candidate.instance_id))) {
        return {GeometryError::invalid_identity, {}};
    }
    bool found_moving = !moving.has_value();
    std::set<std::pair<int, int>> occupied;
    for (const auto &placement : existing) {
        if (moving.has_value() && placement.instance_id == *moving) {
            if (placement.definition_id != candidate.definition_id ||
                placement.shape != candidate.shape) {
                return {GeometryError::invalid_identity, {}};
            }
            found_moving = true;
            continue;
        }
        if (placement.instance_id == candidate.instance_id) {
            return {GeometryError::invalid_identity, {}};
        }
        const auto footprint = facility_footprint(placement.shape, placement.orientation,
                                                  placement.anchor, width, height);
        for (const auto &cell : footprint.cells) {
            occupied.emplace(cell.position.x, cell.position.y);
        }
    }
    if (!found_moving) {
        return {GeometryError::invalid_identity, {}};
    }
    auto result =
        facility_footprint(candidate.shape, candidate.orientation, candidate.anchor, width, height);
    if (result.error != GeometryError::none) {
        return result;
    }
    for (const auto &cell : result.cells) {
        if (occupied.find({cell.position.x, cell.position.y}) != occupied.end()) {
            return {GeometryError::overlap, {}};
        }
    }
    return result;
}

} // namespace ark::simulation::rules
