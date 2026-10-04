// Neighbour modifiers use source instance identity; an occupied footprint is not counted per
// fragment. Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/rules/neighbourhood.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace ark::simulation::rules {
namespace {

bool add_checked(std::int64_t &value, std::int64_t delta) {
    if ((delta > 0 && value > std::numeric_limits<std::int64_t>::max() - delta) ||
        (delta < 0 && value < std::numeric_limits<std::int64_t>::min() - delta)) {
        return false;
    }
    value += delta;
    return true;
}

} // namespace

NeighbourhoodResult
derive_facility_neighbourhood(const std::vector<NeighbourDefinition> &definitions,
                              const std::vector<FacilityPlacement> &placements,
                              const std::vector<Position> &roads, int width, int height) {
    if (validate_facility_layout(placements, width, height) != GeometryError::none) {
        return {NeighbourhoodError::invalid_layout, {}};
    }
    std::map<std::int32_t, const NeighbourDefinition *> catalog;
    for (const auto &definition : definitions) {
        if (definition.definition_id < 0 || definition.legacy_kind < 0 ||
            definition.legacy_kind > 13 ||
            !catalog.emplace(definition.definition_id, &definition).second ||
            facility_footprint(definition.shape, FacilityOrientation::first, {1, 0}, 3, 3).error !=
                GeometryError::none ||
            std::any_of(definition.modifiers.begin(), definition.modifiers.end(),
                        [](const NeighbourModifier &modifier) {
                            return modifier.attribute_slot < 0 || modifier.attribute_slot >= 3;
                        })) {
            return {NeighbourhoodError::invalid_definition, {}};
        }
    }
    std::vector<const NeighbourDefinition *> bound_definitions;
    bound_definitions.reserve(placements.size());
    for (const auto &placement : placements) {
        const auto found = catalog.find(placement.definition_id);
        if (found == catalog.end() || found->second->shape != placement.shape) {
            return {NeighbourhoodError::invalid_definition, {}};
        }
        bound_definitions.push_back(found->second);
    }
    const auto capacity = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    const auto tile_index = [width](Position position) {
        return static_cast<std::size_t>(position.y) * static_cast<std::size_t>(width) +
               static_cast<std::size_t>(position.x);
    };
    std::vector<std::optional<std::size_t>> owners(capacity);
    std::vector<FacilityNeighbourhood> result;
    std::vector<std::vector<Position>> surroundings;
    for (std::size_t index = 0; index < placements.size(); ++index) {
        const auto &placement = placements[index];
        const auto footprint = facility_footprint(placement.shape, placement.orientation,
                                                  placement.anchor, width, height);
        for (const auto &cell : footprint.cells) {
            owners[tile_index(cell.position)] = index;
        }
        result.push_back({placement.instance_id, placement.definition_id, {}, {}, 0});
        surroundings.push_back(facility_surroundings(placement.shape, placement.orientation,
                                                     placement.anchor, width, height)
                                   .cells);
    }
    if (roads.size() > capacity) {
        return {NeighbourhoodError::invalid_roads, {}};
    }
    std::vector<bool> road_cells(capacity);
    for (const auto road : roads) {
        if (road.x < 0 || road.y < 0 || road.x >= width || road.y >= height) {
            return {NeighbourhoodError::invalid_roads, {}};
        }
        const auto index = tile_index(road);
        if (road_cells[index] || owners[index].has_value()) {
            return {NeighbourhoodError::invalid_roads, {}};
        }
        road_cells[index] = true;
    }
    for (std::size_t source = 0; source < placements.size(); ++source) {
        const auto &definition = *bound_definitions[source];
        if (definition.legacy_kind != 2 && definition.legacy_kind != 3) {
            continue;
        }
        std::set<std::size_t> affected;
        for (const auto position : surroundings[source]) {
            const auto target = owners[tile_index(position)];
            if (!target.has_value() || bound_definitions[*target]->legacy_kind != 3 ||
                !affected.insert(*target).second) {
                continue;
            }
            auto &neighbourhood = result[*target];
            for (const auto modifier : definition.modifiers) {
                auto &value =
                    neighbourhood.modifiers[static_cast<std::size_t>(modifier.attribute_slot)];
                if (!add_checked(value, modifier.delta)) {
                    return {NeighbourhoodError::numeric_overflow, {}};
                }
            }
            neighbourhood.sources.push_back(
                {placements[source].instance_id, definition.definition_id});
        }
    }
    for (std::size_t target = 0; target < placements.size(); ++target) {
        if (bound_definitions[target]->legacy_kind != 3) {
            continue;
        }
        for (const auto position : surroundings[target]) {
            if (road_cells[tile_index(position)]) {
                if (!add_checked(result[target].modifiers[2], 2)) {
                    return {NeighbourhoodError::numeric_overflow, {}};
                }
                ++result[target].road_cells;
            }
        }
    }
    return {NeighbourhoodError::none, std::move(result)};
}

std::optional<FacilityEconomyInput>
neighbourhood_economy_input(const FacilityNeighbourhood &neighbourhood,
                            const FacilityEconomyInput &base) {
    auto input = base;
    for (std::size_t slot = 0; slot < neighbourhood.modifiers.size(); ++slot) {
        const auto value = neighbourhood.modifiers[slot];
        if (value < std::numeric_limits<std::int32_t>::min() ||
            value > std::numeric_limits<std::int32_t>::max()) {
            return std::nullopt;
        }
        input.instance_modifiers[slot] = static_cast<std::int32_t>(value);
    }
    return input;
}

} // namespace ark::simulation::rules
