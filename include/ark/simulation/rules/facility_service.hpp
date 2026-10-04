#pragma once

// Service orchestration prepares values; a world owner revalidates IDs and submits atomically.
// These plans do not execute category5 dungeon exploration or render equipment animations.
#include "ark/simulation/rules/actor_lifecycle.hpp"
#include "ark/simulation/rules/facility_exit.hpp"

namespace ark::simulation::rules {
enum class FacilityServiceError {
    none,
    invalid_input,
    unresolved_selection,
    missing_ticket,
    invalid_ticket,
    numeric_overflow
};
struct ResolvedArrivalInput {
    FacilityArrivalInput arrival;
    FacilityArrivalState statistics;
    // af/ag/ah and definition price resolved by weapon_choice BEFORE payment guards.
    std::optional<int> equipment_id;
    std::optional<int> equipment_price;
    // N>=0 category1/7 means deliver a.g; caller proves the carried object still exists.
    bool carried_object_exists{};
    // N==-2 belongs to the two-actor rescue transaction, never silently stripped here.
};
struct ResolvedArrivalCandidate {
    FacilityArrivalCandidate arrival;
    int object_slot{-1};
    std::optional<int> delivered_object;
    std::optional<int> selected_equipment; // Stored locally, not equipped yet.
    int selected_detail{};
};
struct ResolvedArrivalResult {
    FacilityServiceError error{FacilityServiceError::none};
    std::optional<ResolvedArrivalCandidate> candidate;
};
// Delivery/selection precedes counters/payment. Bonus5000 only joins an otherwise payable price.
ResolvedArrivalResult prepare_resolved_arrival(const ResolvedArrivalInput &input);

struct FacilityUsePlanInput {
    ActorControlState control;
    int category{};
    int detail{};
    int activity{}; // Helper mode:0/1 waits at inn; rescue carrier mode2 does not.
    int definition_wait{};
    int category_six_counter{};
    std::optional<Position> world_target; // Resolved h.e result truncated toward zero for6/8.
    std::optional<int> direction_ticket;  // Category8/detail2, [0,4).
};
struct FacilityUsePlan {
    ActorControlState control;
    int category_six_counter{};
    bool reset_state_counter_and_parameter{};
    bool clear_encounter{};
    bool cleanup{};
    bool ground_effect20{}; // At the source projected position; a presentation request only.
};
struct FacilityUsePlanResult {
    FacilityServiceError error{FacilityServiceError::none};
    std::optional<FacilityUsePlan> candidate;
};
// Full a(definition, mode) branch plan. Resets the old queue even for no-op categories.
FacilityUsePlanResult prepare_facility_use_plan(const FacilityUsePlanInput &input);

enum class FacilityExitCommit {
    position,
    shared_use,
    clear_flags33,
    release_occupation,
    reset_control,
    enqueue_activity,
    satisfaction,
    home_hp_and_visits
};
struct FacilityServiceExitInput {
    CharacterId actor;
    ActorControlState control;
    bool binding_valid{}; // q() exact O position/definition/instance AND current instance live.
    LegacyMap map;
    FacilityPlacement facility;
    WorldPosition position;
    int category{};
    int detail{};
    FacilityUseProgress progress;
    LevelEndpoints upgrade_uses;
    std::vector<CharacterId> occupants; // Original ordered vector; duplicates are retained.
    FacilitySatisfactionInput satisfaction;
    std::vector<FacilityAttributeEffect> effects;
    std::optional<int> effect_ticket;
    EquipmentExitTailInput equipment;
};
struct FacilityServiceExitCandidate {
    ActorControlState control;
    std::optional<CleanupCandidate> cleanup;
    std::optional<FacilityExitPositionCandidate> position;
    std::optional<FacilityUseCandidate> shared_use;
    std::optional<FacilitySatisfactionCandidate> satisfaction;
    std::vector<CharacterId> occupants;
    bool home_hp_and_visits{}; // Category9 requests d(h()) and clears all six visit counts now.
    std::vector<FacilityExitCommit> order;
};
struct FacilityServiceExitResult {
    FacilityServiceError error{FacilityServiceError::none};
    std::optional<FacilityServiceExitCandidate> candidate;
};
// Removes opcode24, prepares the complete front and ordinary/equipment/home tails.
// Missing/stale q binding produces r cleanup, no uses/payment. Occupancy removes FIRST match only.
FacilityServiceExitResult prepare_facility_service_exit(const FacilityServiceExitInput &input);
} // namespace ark::simulation::rules
