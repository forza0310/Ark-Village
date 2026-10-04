#pragma once

// Ordinary path motion and facility entry, not physics, collision avoidance or the AI interpreter.
// World x/z units are independent of rendering. See rules/CHARACTERS.md#continuous-motion.
#include "dungeon_village_reference/map_access.hpp"

namespace dungeon_village_reference {
struct WorldPosition {
    float x{};
    float z{};
};
enum class CharacterMotionError { none, invalid_input };
struct WorldWaypointResult {
    CharacterMotionError error{CharacterMotionError::none};
    std::optional<WorldPosition> target;
};
struct CharacterMotionStep {
    WorldPosition position;
    Position logical_cell;
    bool waypoint_overlap{};
    std::optional<WorldPosition> velocity{}; // 原r实际写值；64或零距离时为空，调用方保留旧r。
};
struct CharacterMotionResult {
    CharacterMotionError error{CharacterMotionError::none};
    std::optional<CharacterMotionStep> step;
};

// Java-style truncation toward zero, not floor. The example rejects nonfinite or >1e6 world units
// before conversion; this is an implementation safety bound, not the game's map extent.
std::optional<Position> character_world_cell(WorldPosition position);
// Entry states 6/7 offset a path waypoint by +/-40 using DEFINITION direction, not cell.m.
WorldWaypointResult character_waypoint(Position cell, int legacy_state, int definition_direction);
// One eligible update advances 6.7 world units even if it overshoots. Bit64 prevents movement;
// overlap is still evaluated after that guard. No elapsed seconds, path snapping or cash effects.
CharacterMotionResult advance_character_motion(WorldPosition current, WorldPosition target,
                                               std::uint32_t legacy_flags);

enum class FacilityEntryStatus { invalid_input, inactive_route, not_entered, stale_binding, ready };
// Evaluate BEFORE the next motion step, using the logical cell from the preceding update. Ready
// does not mean at the waypoint centre or paid; the caller must deduplicate its arrival
// transaction.
FacilityEntryStatus inspect_facility_entry(const LegacyMap &map, const ArrivalBinding &target,
                                           WorldPosition current, bool path_active);
} // namespace dungeon_village_reference
