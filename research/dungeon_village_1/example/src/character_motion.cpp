// Independent numerical rules for ordinary travel; no copied state-machine implementation.
// Arrival uses logical tile admission, while path advancement uses the small world rectangles.
#include "dungeon_village_reference/character_motion.hpp"

#include <cmath>

namespace dungeon_village_reference {
namespace {
bool valid(WorldPosition position) {
    return std::isfinite(position.x) && std::isfinite(position.z) &&
           std::abs(position.x) <= 1000000.0F && std::abs(position.z) <= 1000000.0F;
}
} // namespace

std::optional<Position> character_world_cell(WorldPosition position) {
    if (!valid(position))
        return std::nullopt;
    return Position{static_cast<int>(position.x / 100.0F), static_cast<int>(position.z / 100.0F)};
}

WorldWaypointResult character_waypoint(Position cell, int legacy_state, int direction) {
    if (cell.x < -9999 || cell.x > 9999 || cell.y < -9999 || cell.y > 9999 || legacy_state < 0 ||
        legacy_state > 12 ||
        ((legacy_state == 6 || legacy_state == 7) && (direction < 0 || direction > 3)))
        return {CharacterMotionError::invalid_input, std::nullopt};
    WorldPosition target{cell.x * 100.0F + 50.0F, cell.y * 100.0F + 50.0F};
    if (legacy_state == 6 || legacy_state == 7) {
        const float offset = (direction % 2 == 0) ? 40.0F : -40.0F;
        if (direction < 2)
            target.z += offset;
        else
            target.x += offset;
    }
    return {CharacterMotionError::none, target};
}

CharacterMotionResult advance_character_motion(WorldPosition current, WorldPosition target,
                                               std::uint32_t flags) {
    if (!valid(current) || !valid(target))
        return {CharacterMotionError::invalid_input, std::nullopt};
    auto next = current;
    if ((flags & 64U) == 0) {
        const float dx = target.x - current.x, dz = target.z - current.z;
        const float distance = std::sqrt(dx * dx + dz * dz);
        if (distance != 0.0F) {
            next.x += dx * 6.7F / distance;
            next.z += dz * 6.7F / distance;
        }
    }
    const auto cell = character_world_cell(next);
    if (!cell)
        return {CharacterMotionError::invalid_input, std::nullopt};
    // Shapes 0 and 2 both use offsets(-4,+4), width/height4 and inclusive rectangle edges.
    const bool overlap = std::abs(next.x - target.x) <= 4.0F && std::abs(next.z - target.z) <= 4.0F;
    return {CharacterMotionError::none, CharacterMotionStep{next, *cell, overlap}};
}

FacilityEntryStatus inspect_facility_entry(const LegacyMap &map, const ArrivalBinding &target,
                                           WorldPosition current, bool active) {
    const auto cell = character_world_cell(current);
    if (!cell || !valid_legacy_map(map) || target.instance_id.value == 0 ||
        target.definition_id < 0 || target.goal.x < 0 || target.goal.x >= map.width ||
        target.goal.y < 0 || target.goal.y >= map.height)
        return FacilityEntryStatus::invalid_input;
    if (!active)
        return FacilityEntryStatus::inactive_route;
    if (!(*cell == target.goal))
        return FacilityEntryStatus::not_entered;
    return arrival_binding_matches(map, target, *cell) ? FacilityEntryStatus::ready
                                                       : FacilityEntryStatus::stale_binding;
}
} // namespace dungeon_village_reference
