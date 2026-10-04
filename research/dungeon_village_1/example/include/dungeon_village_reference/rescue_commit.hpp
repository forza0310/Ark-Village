#pragma once

// Shared rescue/facility owner: actor HP/control live only in ai.battle; B2 is definition-shared.
// This is a private transaction projection, not the default window or a save format.
#include "dungeon_village_reference/ai_perception.hpp"
#include "dungeon_village_reference/ai_rewards.hpp"
#include "dungeon_village_reference/facility_departure.hpp"
#include "dungeon_village_reference/facility_service.hpp"

namespace dungeon_village_reference {
struct RescueFacility {
    FacilityPlacement placement;
    int kind{2};
    int category{2};
    int detail{};
    int price{};
    int status{1}; // Source m.e, rescue availability requires EXACTLY1.
    LevelEndpoints upgrade_uses;
    int sales{};
    std::vector<CharacterId> occupants;
    int definition_wait{}; // o.w; distinct from instance condition or actor counters.
};
struct RescueActorContext {
    FacilityArrivalState visits; // B2/sales fields are temporary projections, never authorities.
    std::optional<ArrivalBinding> binding; // O; independent of current cached s.
    bool path_pending{};
    bool on_event_cell{}; // Current map flags2, not inferred from event centers before refresh.
    bool definition_task_flag{};
    std::optional<FacilityDeparture> journey;
    std::size_t waypoint{};
    WorldPosition horizontal_velocity; // r.x/z for the special-entry parabola.
    int town_updates{};
    int outside_updates{};
    int blocked_updates{};
    int spawn_updates{};
    int no_path_updates{};
    int short_exit_updates{};
    int bad_area_updates{};
    int monster_mode{}; // T, normal special-entry landing sets1.
    // O0/1 survive r() and are independent of an instance identity; a ground goal has no binding.
    // The original freshly allocated O array is all-zero, not generally a null destination.
    std::optional<Position> destination;
    std::optional<LegacyPathResult>
        unbound_route; // Real G for ground goals, never a fake building.
};
struct RescueWorldState {
    AiRewardState ai;
    LegacyMap map;
    std::map<CharacterId, RescueActorContext> actors;
    std::map<std::uint64_t, RescueFacility> facilities;
    std::map<int, int> human_spending; // e.B[2], shared by all instances of that definition.
    std::map<int, FacilityUseProgress> facility_uses;
    int month_index{}; // Original0..11, distinct from accounting period identity.
    std::vector<std::uint64_t> object_order; // bp order, independent of numeric object IDs.
};
enum class RescueWorldError { none, invalid_input, stale_actor, stale_binding, preparation_failed };
struct RescueWorldCandidate {
    RescueWorldState state;
    RescueBindingAction binding_action{RescueBindingAction::baseline};
    std::vector<LifecycleRequest> requests;
    bool arrived{};
    bool occupied{};
    bool exited{};
    bool recovered{};
    bool cleaned_up{};
    std::optional<DepartureOverrideCandidate> departure_override;
    ActorControlFlow flow{ActorControlFlow::empty};
};
struct RescueWorldResult {
    RescueWorldError error{RescueWorldError::none};
    std::optional<RescueWorldCandidate> candidate;
};
// The caller supplies the freshly selected I() target and collision result, not a cached enemy.
// Availability is rebuilt from live facility instances, not a UI toggle.
RescueWorldResult prepare_world_rescue_bind(const RescueWorldState &state, CharacterId rescuer,
                                            std::optional<CharacterId> target, bool touching);
// Fresh I() from the owner's original roster, then actual rectangles and straight6.7-unit chase.
RescueWorldResult prepare_world_rescue_seek(const RescueWorldState &state, CharacterId rescuer,
                                            CollisionBox actor_box, CollisionBox rescue_box);
// Activity4 cost-minimum inn (strict-less ties, including unfinished nonzero instances).
// Source-map definitions/IDs/town are read-only inputs; instance/visit fields are rebuilt.
// Higher-priority task/rescue/object selection is returned as a handoff, never ignored.
RescueWorldResult prepare_world_rescue_return(const RescueWorldState &state, CharacterId rescuer,
                                              const ActivityCandidateInput &map_view);
// P() service interval: validate early logical-cell entry before advancing a source waypoint.
RescueWorldResult prepare_world_rescue_path_c(const RescueWorldState &state, CharacterId rescuer);
// c() reference repair then state16 follow. Copy n/height only; cached logical s stays unchanged.
RescueWorldResult prepare_world_rescue_follow(const RescueWorldState &state, CharacterId actor);
// Recursive rescued arrival uses OLD flags/s; copies O/n/s only AFTER its use1 is arranged.
// Successful release is not replayable: N/R/binding revalidation rejects a repeated delivery.
struct RescueDeliveryProjection {
    std::optional<Position> rescued_target; // Category8: derived from rescued OLD s, before copy.
    std::optional<int> rescued_direction;   // First draw4, recursive use1 precedes carrier use2.
    std::optional<Position> carrier_target;
    std::optional<int> carrier_direction;
};
RescueWorldResult prepare_world_rescue_delivery(const RescueWorldState &state, CharacterId rescuer,
                                                const RescueDeliveryProjection &projection = {});
// Split c()/d(): c reads old B170; d advances counters/HP, local prefix, occupation and exit.
// Exit keeps queued activity8 and expression18 for the next interpreter consumer.
RescueWorldResult prepare_world_inn_c(const RescueWorldState &state, CharacterId actor);
RescueWorldResult prepare_world_inn_d(const RescueWorldState &state, CharacterId actor);
// Control ONLY, after the common d prefix. Never advances counters/effects/HP/growth.
// Common exits include5 as well as2/7/8/9; category1 exit and category5 occupation delegate.
RescueWorldResult prepare_world_inn_control(const RescueWorldState &state, CharacterId actor,
                                            std::size_t domain_limit = 1000000);
// r() is staged departure, not deletion; N and path are deliberately retained.
RescueWorldResult prepare_world_rescue_cleanup(const RescueWorldState &state, CharacterId actor);
// 同一r()也被怪物17/T1调用；按真实kind选择c19/c0，不制造怪物专属离村流程。
RescueWorldResult prepare_world_actor_cleanup(const RescueWorldState &state, CharacterId actor);
RescueWorldResult prepare_world_detached_actor_cleanup(const RescueWorldState &state,
                                                       CharacterId actor);
} // namespace dungeon_village_reference
