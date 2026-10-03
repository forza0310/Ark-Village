// Keep original 100-unit cells, 6.7-unit advance, +/-40 entrance waypoints and entry ordering.
#include "ark/people/motion.hpp"
#include <cmath>
#include <stdexcept>
namespace ark::people {
namespace {
bool valid(world::WorldPosition p) {
    return std::isfinite(p.x) && std::isfinite(p.z) && std::abs(p.x) <= 1000000 &&
           std::abs(p.z) <= 1000000;
}
} // namespace
std::optional<world::Cell> world_cell(world::WorldPosition p) {
    if (!valid(p))
        return {};
    return world::Cell{static_cast<int>(p.x / 100), static_cast<int>(p.z / 100)};
}
world::WorldPosition waypoint(world::Cell p, int state, int direction) {
    if (p.x < -9999 || p.x > 9999 || p.y < -9999 || p.y > 9999 || state < 0 || state > 12 ||
        ((state == 6 || state == 7) && (direction < 0 || direction > 3)))
        throw std::invalid_argument("Invalid motion waypoint");
    world::WorldPosition target{p.x * 100.0F + 50, p.y * 100.0F + 50};
    if (state == 6 || state == 7) {
        const float offset = direction % 2 == 0 ? 40.0F : -40.0F;
        if (direction < 2)
            target.z += offset;
        else
            target.x += offset;
    }
    return target;
}
MotionStep advance_motion(world::WorldPosition current, world::WorldPosition target,
                          std::uint32_t flags) {
    if (!valid(current) || !valid(target))
        throw std::invalid_argument("Invalid motion position");
    auto next = current;
    if (!(flags & 64U)) {
        const auto dx = target.x - current.x, dz = target.z - current.z;
        const auto distance = std::sqrt(dx * dx + dz * dz);
        if (distance != 0) {
            next.x += dx * 6.7F / distance;
            next.z += dz * 6.7F / distance;
        }
    }
    const auto cell = world_cell(next);
    if (!cell)
        throw std::invalid_argument("Motion outside numeric bounds");
    return {next, *cell, std::abs(next.x - target.x) <= 4 && std::abs(next.z - target.z) <= 4};
}
EntryStatus inspect_entry(const world::RouteMap &map, world::ArrivalTarget target,
                          world::WorldPosition current, bool active) {
    const auto cell = world_cell(current);
    if (!cell || !world::valid_map(map) || !target.instance || target.definition_id < 0 ||
        !map.contains(target.cell))
        return EntryStatus::invalid_input;
    if (!active)
        return EntryStatus::inactive_route;
    if (*cell != target.cell)
        return EntryStatus::not_entered;
    return world::arrival_matches(map, target, *cell) ? EntryStatus::ready
                                                      : EntryStatus::stale_binding;
}
std::optional<Travel> plan_travel(const world::RouteMap &map, world::Cell start,
                                  world::ArrivalTarget target) {
    if (!world::arrival_matches(map, target, target.cell))
        return {};
    const auto field = world::search(map, start);
    if (!field.field)
        return {};
    const auto path = world::trace(*field.field, target.cell);
    if (path.error != world::RouteError::none)
        return {};
    return Travel{start, target, path.steps, waypoint(start, 4, -1), 0, TravelPhase::travelling};
}
TravelStep advance_travel(const world::RouteMap &map, const Travel &travel, std::uint32_t flags) {
    if (!world::valid_map(map) || !world_cell(travel.position) ||
        !map.contains(travel.target.cell) || !travel.target.instance ||
        travel.target.definition_id < 0 || !map.contains(travel.start) ||
        static_cast<int>(travel.phase) < 0 || static_cast<int>(travel.phase) > 3 ||
        travel.next > travel.path.size())
        throw std::invalid_argument("Invalid travel state");
    auto previous = travel.start;
    for (const auto cell : travel.path) {
        if (!map.contains(cell) ||
            std::abs(cell.x - previous.x) + std::abs(cell.y - previous.y) != 1)
            throw std::invalid_argument("Invalid travel path geometry");
        previous = cell;
    }
    if (previous != travel.target.cell)
        throw std::invalid_argument("Travel path target mismatch");
    TravelStep result{travel, false};
    if (travel.phase != TravelPhase::travelling)
        return result;
    auto &next = result.travel;
    // Product safety guard for an obsolete explicit route; not original AI replanning policy.
    if (!world::arrival_matches(map, travel.target, travel.target.cell)) {
        next.phase = TravelPhase::stale_target;
        return result;
    }
    if (inspect_entry(map, travel.target, travel.position, true) == EntryStatus::ready) {
        next.phase = TravelPhase::entered;
        result.entered = true;
        return result;
    }
    if (travel.next >= travel.path.size())
        throw std::invalid_argument("Exhausted travel outside target");
    const auto cell = travel.path[travel.next], current = *world_cell(travel.position);
    if (!map.contains(cell) || !map.contains(current))
        throw std::invalid_argument("Travel outside map");
    const auto &tile = map.cells[map.index(cell)];
    if (cell != current && !world::route_transition(map.cells[map.index(current)], tile,
                                                    travel.next == 0 && current == travel.start)) {
        next.phase = TravelPhase::blocked;
        return result;
    }
    const auto step =
        advance_motion(travel.position, waypoint(cell, tile.legacy_state, tile.direction), flags);
    next.position = step.position;
    if (step.waypoint_overlap && next.next + 1 < next.path.size())
        ++next.next;
    return result;
}
} // namespace ark::people
