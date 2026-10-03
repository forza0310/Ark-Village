#pragma once
// Ordinary motion/entry from research/character_motion, independent of renderer and AI priority.
#include "ark/world/navigation.hpp"
namespace ark::people {
struct MotionStep {
    world::WorldPosition position;
    world::Cell cell;
    bool waypoint_overlap{};
};
std::optional<world::Cell> world_cell(world::WorldPosition position);
world::WorldPosition waypoint(world::Cell cell, int legacy_state, int definition_direction);
// One eligible update, not a render frame or seconds; flag64 blocks movement, not overlap check.
MotionStep advance_motion(world::WorldPosition current, world::WorldPosition target,
                          std::uint32_t flags);
enum class EntryStatus { invalid_input, inactive_route, not_entered, stale_binding, ready };
// Check before motion using the preceding update's logical cell. Centre arrival is not required.
EntryStatus inspect_entry(const world::RouteMap &map, world::ArrivalTarget target,
                          world::WorldPosition current, bool active);
enum class TravelPhase { travelling, entered, stale_target, blocked };
struct Travel {
    world::Cell start;
    world::ArrivalTarget target;
    std::vector<world::Cell> path;
    world::WorldPosition position;
    std::size_t next{};
    TravelPhase phase{TravelPhase::travelling};
};
struct TravelStep {
    Travel travel;
    bool entered{};
};
// Explicit researched target only. No selector, reservation, payment or automatic next activity.
std::optional<Travel> plan_travel(const world::RouteMap &map, world::Cell start,
                                  world::ArrivalTarget target);
// Pure candidate. Stale/blocked routes stop safely; a caller owns committing/replanning policy.
TravelStep advance_travel(const world::RouteMap &map, const Travel &travel, std::uint32_t flags);
} // namespace ark::people
