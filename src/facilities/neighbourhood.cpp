// FACILITIES: source kinds2/3 -> target kind3. Roads add charm per ring cell, not per source.
#include "ark/facilities/neighbourhood.hpp"
#include <limits>
#include <set>
#include <stdexcept>
namespace ark::facilities {
namespace {
void add(std::int64_t &value, std::int64_t delta) {
    if ((delta > 0 && value > std::numeric_limits<std::int64_t>::max() - delta) ||
        (delta < 0 && value < std::numeric_limits<std::int64_t>::min() - delta))
        throw std::overflow_error("Neighbourhood modifier overflow");
    value += delta;
}
} // namespace
std::map<InstanceId, Neighbourhood>
derive_neighbourhood(const std::vector<Definition> &definitions,
                     const std::map<InstanceId, Instance> &instances,
                     const std::vector<world::Cell> &roads, const world::SourceMap &map) {
    if (map.width <= 0 || map.height <= 0 ||
        static_cast<std::int64_t>(map.width) * map.height > 1000000)
        throw std::invalid_argument("Invalid neighbourhood map size");
    std::map<int, const Definition *> catalog;
    for (const auto &d : definitions) {
        if (d.id < 0 || d.kind < 0 || d.kind > 13 || d.shape < 0 || d.shape > 2 ||
            !catalog.emplace(d.id, &d).second)
            throw std::invalid_argument("Invalid neighbourhood definition");
        for (const auto &m : d.neighbours)
            if (m.slot < 0 || m.slot >= 3)
                throw std::invalid_argument("Invalid neighbour slot");
    }
    std::map<world::Cell, InstanceId> owners;
    std::map<InstanceId, std::vector<world::Cell>> rings;
    std::map<InstanceId, Neighbourhood> result;
    for (const auto &[id, instance] : instances) {
        if (!id || id != instance.id || !catalog.count(instance.definition_id))
            throw std::invalid_argument("Invalid neighbourhood instance identity");
        const auto &d = *catalog.at(instance.definition_id);
        rings.emplace(id, surroundings(d.shape, instance.orientation, instance.anchor, map));
        for (const auto &part : footprint(d.shape, instance.orientation, instance.anchor))
            if (!owners.emplace(part.cell, id).second)
                throw std::invalid_argument("Overlapping facilities");
        result.emplace(id, Neighbourhood{id, {}, {}, 0});
    }
    std::set<world::Cell> road_set;
    for (const auto cell : roads)
        if (!map.contains(cell) || owners.count(cell) || !road_set.insert(cell).second)
            throw std::invalid_argument("Invalid neighbourhood road snapshot");
    for (const auto &[id, instance] : instances) {
        const auto &d = *catalog.at(instance.definition_id);
        if (d.kind != 2 && d.kind != 3)
            continue;
        std::set<InstanceId> affected;
        for (const auto cell : rings.at(id)) {
            const auto owner = owners.find(cell);
            if (owner == owners.end() || !affected.insert(owner->second).second)
                continue;
            const auto &target = instances.at(owner->second);
            if (catalog.at(target.definition_id)->kind != 3)
                continue;
            auto &value = result.at(target.id);
            for (const auto &m : d.neighbours)
                add(value.modifiers[m.slot], m.delta);
            value.sources.push_back({id, d.id});
        }
    }
    for (const auto &[id, instance] : instances) {
        if (catalog.at(instance.definition_id)->kind != 3)
            continue;
        for (const auto cell : rings.at(id))
            if (road_set.count(cell)) {
                add(result.at(id).modifiers[2], 2);
                ++result.at(id).road_cells;
            }
    }
    return result;
}
EconomyInput with_neighbours(const EconomyInput &base, const Neighbourhood &neighbours) {
    auto result = base;
    for (std::size_t i = 0; i < 3; ++i) {
        const auto value = neighbours.modifiers[i];
        if (value < std::numeric_limits<std::int32_t>::min() ||
            value > std::numeric_limits<std::int32_t>::max())
            throw std::overflow_error("Neighbourhood modifier narrowing overflow");
        result.modifiers[i] =
            static_cast<std::int32_t>(value); // Replace, never add to an old snapshot.
    }
    return result;
}
} // namespace ark::facilities
