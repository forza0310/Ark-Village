// The loaded TSV is reset evidence, not a live occupancy cache. Rebind all current instances.
#include "ark/app/game.hpp"
#include "ark/facilities/map_binding.hpp"
#include <set>
#include <stdexcept>
namespace ark::app {
world::RouteMap Game::route_map() const {
    const auto &data = startup_data();
    world::RouteMap base{data.map.width, data.map.height, {}};
    for (const auto &cell : data.loaded_cells) {
        // Clearing an ordinary occupied tile yields state4/category2. Entrances are always
        // rebound in this finite construction-only model; removal/expansion is not implemented.
        base.cells.push_back(cell.instance_id
                                 ? world::RouteCell{}
                                 : world::RouteCell{cell.legacy_state,
                                                    cell.category,
                                                    cell.definition_id,
                                                    definition(cell.definition_id).direction,
                                                    {}});
    }
    std::set<facilities::InstanceId> ids;
    for (auto id : state_.instance_order)
        if (!state_.facilities.count(id) || !ids.insert(id).second)
            throw std::logic_error("Invalid facility vector order");
    if (ids.size() != state_.facilities.size())
        throw std::logic_error("Incomplete facility order");
    return facilities::bind_map(std::move(base), data.definitions, state_.facilities);
}
world::Route Game::route_to(world::Cell start, world::ArrivalTarget target) const {
    const auto map = route_map();
    if (!world::arrival_matches(map, target, target.cell))
        return {world::RouteError::binding_mismatch, {}, 0};
    const auto field = world::search(map, start);
    return field.field ? world::trace(*field.field, target.cell) : world::Route{field.error, {}, 0};
}
} // namespace ark::app
