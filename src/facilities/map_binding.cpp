// Binding semantics from research/map_access; reject invalid identities before returning a view.
#include "ark/facilities/map_binding.hpp"
#include <algorithm>
#include <stdexcept>
namespace ark::facilities {
world::RouteMap bind_map(world::RouteMap terrain, const std::vector<Definition> &definitions,
                         const std::map<InstanceId, Instance> &instances) {
    if (!world::valid_map(terrain) ||
        std::any_of(terrain.cells.begin(), terrain.cells.end(),
                    [](const auto &c) { return c.facility.has_value(); }))
        throw std::invalid_argument("Expected unbound route terrain");
    std::map<int, const Definition *> catalog;
    for (const auto &d : definitions)
        if (d.id < 0 || !catalog.emplace(d.id, &d).second)
            throw std::invalid_argument("Invalid map definition identity");
    for (const auto &[id, instance] : instances) {
        if (!id || instance.id != id || !catalog.count(instance.definition_id))
            throw std::invalid_argument("Invalid map instance identity");
        const auto &d = *catalog.at(instance.definition_id);
        int state{};
        auto category = world::RouteCategory::terminal;
        switch (d.kind) {
        case 3:
        case 12:
        case 13:
            state = 1;
            break;
        case 1:
            state = 8;
            break;
        case 8:
            state = 9;
            break;
        case 9:
            state = 10;
            break;
        case 4:
            state = 6;
            category = world::RouteCategory::access;
            break;
        case 5:
            state = 7;
            category = world::RouteCategory::access;
            break;
        case 2:
            state = 2;
            category = world::RouteCategory::blocked;
            break;
        default:
            throw std::invalid_argument("Unsupported bound facility kind");
        }
        for (const auto &part : footprint(d.shape, instance.orientation, instance.anchor)) {
            if (!terrain.contains(part.cell) || terrain.cells[terrain.index(part.cell)].facility)
                throw std::invalid_argument("Invalid or overlapping facility footprint");
            terrain.cells[terrain.index(part.cell)] = {state, category, d.id, d.direction,
                                                       world::TileBinding{id, d.id, part.fragment}};
        }
    }
    if (!world::valid_map(terrain))
        throw std::invalid_argument("Invalid resulting map");
    return terrain;
}
} // namespace ark::facilities
