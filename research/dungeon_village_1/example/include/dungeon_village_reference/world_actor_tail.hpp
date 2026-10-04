#pragma once

// Actual d tail on the shared owner. Interpreter true must bypass this entire segment.
#include "dungeon_village_reference/world_facilities.hpp"

namespace dungeon_village_reference {
struct WorldActorTailInput {
    CharacterId actor;
    WorldMapFacts facts;
    std::vector<Position> spawn_cells; // Map.f, raw order; no invented entrances.
    // 原a.a(n,u)之后、留存r之前的朝向；旧v由外层表现缓存捕获，不把像素引入核心。
    std::function<std::optional<int>(const BattleActorRecord &)> facing_after_projection{};
};
struct WorldActorTailCandidate {
    RescueWorldState state;
    bool delete_instance{}; // Return to schedule; removal is a DIFFERENT source point.
    ActorDeletionReason deletion_reason{ActorDeletionReason::none};
    bool cleaned_up{};
    bool queried_area{};
    std::optional<BattleActorRecord> projected_actor{}; // 物理/朝向后、r留存前的n/u输入。
};
struct WorldActorTailResult {
    RescueWorldError error{RescueWorldError::none};
    std::optional<WorldActorTailCandidate> candidate;
};
// Old-s L/M -> physics/K/s/t -> ab/r -> spawn/empty path/short exit -> monster db -> bad area.
// No common d prefix/control, no eager ax update, no removal or garbage collection here.
WorldActorTailResult prepare_world_detached_actor_tail(const RescueWorldState &state,
                                                       const WorldActorTailInput &input);
WorldActorTailResult prepare_world_actor_tail(const RescueWorldState &state,
                                              const WorldActorTailInput &input);
// Schedule's actual erase: human d true first releases current q FIRST matching occupant;
// c true does not. Removed objects remain available to db/dc/R/S/az consumers until GC.
WorldActorTailResult prepare_world_actor_remove(const RescueWorldState &state, CharacterId actor,
                                                bool from_execution);
} // namespace dungeon_village_reference
