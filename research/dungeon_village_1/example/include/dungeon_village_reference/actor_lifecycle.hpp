#pragma once

#include "dungeon_village_reference/actor_ai.hpp"
#include "dungeon_village_reference/actor_control.hpp"
#include "dungeon_village_reference/facility_arrival.hpp"
#include "dungeon_village_reference/facility_use.hpp"

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

struct CarryReferenceRepairInput {
    std::uint32_t flags{};
    int object_slot{-1};
    bool has_reference{};
    bool other_has_reference{}; // c() tests R.R!=null, not equality to self.
};
struct CarryReferenceRepairCandidate {
    int object_slot{-1};
    bool clear_reference{};
    bool reset_all_hp{};
    bool reset_action{};
};
std::optional<CarryReferenceRepairCandidate>
prepare_carry_reference_repair(const CarryReferenceRepairInput &input);
struct RescuedFollowCandidate {
    bool cleanup{};
    WorldPosition position;
    float height{};
};
// State16 follows R.n with +16 height; absent/out-of-roster carrier invokes r(), not pathfinding.
std::optional<RescuedFollowCandidate> prepare_rescued_follow(bool reference, bool in_human_roster,
                                                             WorldPosition carrier, float height);
enum class RescueReleaseRequestKind {
    clear_rescued_reference,
    rescued_state0,
    rescued_arrival,
    rescued_use1,
    copy_target_binding,
    copy_world_position,
    copy_logical_cell,
    clear_rescuer_reference_and_slot,
    set_rescuer_flag256
};
struct RescueReleaseCandidate {
    bool released{};
    std::vector<RescueReleaseRequestKind> requests;
};
// Arrival helper releases N=-2 only to category2/8, before ordinary rescuer visit statistics.
// Clear R of the rescued actor BEFORE recursive arrival, preventing another rescue recursion.
std::optional<RescueReleaseCandidate> prepare_rescue_release(int object_slot, int category,
                                                             bool rescued_reference);
struct RescueInnInput {
    FacilityArrivalState rescuer_arrival;
    FacilityArrivalState rescued_arrival;
    FacilityArrivalInput rescuer;
    FacilityArrivalInput rescued; // Uses old rescued s/flags for arrival pricing, before copy.
    bool bound_both_ways{};
    bool rescued_roster_member{};
};
struct RescueInnCandidate {
    FacilityArrivalCandidate rescuer_arrival;
    FacilityArrivalCandidate rescued_arrival;
    FacilityUseState rescued_use; // Mode1, wait200; occupation still first rescued d().
    std::uint32_t rescuer_flags{};
    std::int64_t cash_income{};
    std::vector<LegacyActorControl> rescuer_queue; // Mode2: occupy then immediate exit, no wait.
    RescueReleaseCandidate release;
};
struct RescueInnResult {
    LifecycleError error{LifecycleError::none};
    std::optional<RescueInnCandidate> candidate;
};
// Narrow, atomic preparation for ordinary category2/detail0 rescue delivery. No partial revenue
// if either actor's counters/ownership fail; IDs here use the existing nonzero arrival adapter.
RescueInnResult prepare_rescue_inn_arrival(const RescueInnInput &input);

struct RestoreActorIdentity {
    int legacy_id{};
    CharacterId id;
};
struct RestoreEncounterIdentity {
    int legacy_id{};
    std::uint64_t id{};
    std::uint64_t group_id{};
};
struct ActorReferenceIds {
    int rescue{-1}; // R from HUMAN roster even when restoring a monster.
    int follow{-1}; // S from MONSTER roster.
    int encounter{-1};
};
struct RestoredActorReferences {
    std::optional<CharacterId> rescue;
    std::optional<CharacterId> follow;
    std::optional<std::uint64_t> encounter;
    std::optional<std::uint64_t> group;
};
// This is reference rebinding, NOT a save parser/migration. Missing IDs remain null; duplicate
// source IDs take the first match. Group comes only from restored db, never a serialized dc ID.
RestoredActorReferences
restore_actor_references(const ActorReferenceIds &ids,
                         const std::vector<RestoreActorIdentity> &humans,
                         const std::vector<RestoreActorIdentity> &monsters,
                         const std::vector<RestoreEncounterIdentity> &encounters);
std::vector<CharacterId> restore_roster_references(const std::vector<int> &saved_ids,
                                                   const std::vector<RestoreActorIdentity> &roster);

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
