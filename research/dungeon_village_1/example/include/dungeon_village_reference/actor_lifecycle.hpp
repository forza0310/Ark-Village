#pragma once

#include "dungeon_village_reference/actor_ai.hpp"

namespace dungeon_village_reference {
enum class LifecycleError { none, invalid_input, unsupported_state, stale_target };
enum class LifecycleRequestKind {
    state,
    restore_baseline,
    activity,
    immediate_activity,
    expression,
    ground_effect,
    clear_path,
    trigger_event,
    wander_event,
    follow_actor,
    remove_event_member,
    normal_death_rewards,
    cancelled_death_effect
};
struct LifecycleRequest {
    LifecycleRequestKind kind;
    int parameter{};
};
struct TimedLifecycleInput {
    ActorKind kind{ActorKind::human};
    int state{};
    int old_counter{};
    int monster_mode{};
    int death_parameter{};
    std::uint32_t flags{};
    bool has_object{};
    bool inside_town{};
    bool event90_seen{};
    WorldPosition position;
    WorldPosition horizontal_velocity;
    float height{};
    int hp_slot1{};
    int hp_capacity{1};
};
struct TimedLifecycleCandidate {
    WorldPosition position;
    WorldPosition horizontal_velocity;
    float height{};
    std::uint32_t flags{};
    std::optional<int> write_hp_slot1_and3;
    bool reset_all_hp_to_capacity{};
    bool zero_vertical_velocity{};
    bool delete_instance{};
    std::optional<int> monster_mode;
    std::vector<LifecycleRequest>
        requests; // Source commit order; immediate_activity is not queued.
};
struct TimedLifecycleResult {
    LifecycleError error{LifecycleError::none};
    std::optional<TimedLifecycleCandidate> candidate;
};
// c() timed states2/3/4/8/9/10/12/15/20 only. Main horizontal movement is separate from au
// rendering. No rewards or HP-display progression committed here; caller applies all requests
// atomically.
TimedLifecycleResult prepare_timed_lifecycle(const TimedLifecycleInput &input);

struct RescueBindingInput {
    CharacterId rescuer;
    int rescuer_state{13};
    int object_slot{-1}; // -2 is already occupied, not an empty slot.
    bool rescue_enabled{};
    std::optional<CharacterId> target;
    int target_state{2};
    WorldPosition target_position;
    bool touching{}; // Original actor/rescue collision rectangles, not center equality.
};
enum class RescueBindingAction { baseline, chase, bind };
struct RescueBindingCandidate {
    RescueBindingAction action{RescueBindingAction::baseline};
    std::optional<CharacterId> rescuer_reference;
    std::optional<CharacterId> target_reference;
    std::optional<int> object_slot;
    std::optional<int> target_state;
    std::optional<int> sound;
    std::vector<LifecycleRequest> requests;
};
struct RescueBindingResult {
    LifecycleError error{LifecycleError::none};
    std::optional<RescueBindingCandidate> candidate;
};
// State13: bind both R references with N=-2; owner must revalidate target state/identity on commit.
RescueBindingResult prepare_rescue_binding(const RescueBindingInput &input);

struct CleanupCandidate {
    int state{};
    std::uint32_t flags{};
    int waiting_updates{};
    int activity{5};
    bool clear_carry_reference{true};
    bool clear_encounter_and_group{true};
    bool release_current_facility{true};
    bool clear_commands{true};
    bool zero_height{true};
};
// r() does not delete immediately and DOES NOT reset N/object slot or automatically clear path.
std::optional<CleanupCandidate> prepare_actor_cleanup(ActorKind kind, std::uint32_t flags);
} // namespace dungeon_village_reference
