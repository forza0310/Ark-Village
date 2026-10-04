#pragma once

#include "ark/simulation/rules/actor_lifecycle.hpp"

namespace ark::simulation::rules {
struct BattlePreparationEncounter {
    int legacy_id{};
    Position center;
};
enum class BattlePreparationAction { keep, battle, restore_baseline };
struct BattlePreparationInput {
    bool battle_gate{};
    bool cell_in_map{};
    bool cell_event_flag{};
    std::optional<BattlePreparationEncounter> encounter;
    std::vector<BattlePreparationEncounter> live_encounters;
    std::uint32_t flags{};
};
struct BattlePreparationCandidate {
    BattlePreparationAction action{BattlePreparationAction::keep};
    bool clear_encounter{};
};
// State18: G precedes map/event checks. Existing event is matched by legacyID AND center, not
// pointer equality; an off-map cell alone does not restore baseline or clear db.
BattlePreparationCandidate prepare_battle_preparation(const BattlePreparationInput &input);

struct ActorPhysicsInput {
    int state{};
    std::uint32_t flags{};
    int pause{}; // P, old positive value skips gravity/K even when decremented to0.
    float height{};
    float vertical_velocity{};
    WorldPosition position;
    WorldPosition decision_start; // bu from the beginning of c, restored only in battle/knockback.
    bool area_after{};            // Fresh K result when physics is admitted; otherwise ignored.
    bool previous_area_after{};
    int blocked_battle_steps{}; // at, reset by c at150, not by every state transition.
};
struct ActorPhysicsCandidate {
    ActorPhysicsInput state;
    bool query_area_after{};
    std::optional<int> diagnostic; // bp7/8 when reverting horizontal movement.
};
std::optional<ActorPhysicsCandidate> prepare_actor_physics(const ActorPhysicsInput &input);

struct ActorRetentionState {
    ActorKind kind{ActorKind::human};
    int state{};
    std::uint32_t flags{};
    int town_updates{};       // L, modulo INT_MAX.
    int outside_updates{};    // M, non0 state without16 only.
    int blocked_updates{};    // ab.
    int spawn_updates{};      // aF.
    int no_path_updates{};    // aG.
    int short_exit_updates{}; // aH.
    int bad_area_updates{};   // aI.
};
struct ActorRetentionInput {
    ActorRetentionState state;
    bool old_cell_inside_town{}; // Before d reprojects world n into logical s.
    bool at_spawn_after_projection{};
    bool area_before{}; // aB0 from c, NOT aB1 from physics.
    std::size_t route_cells{};
    bool has_encounter{};
    int reported_hp{}; // g() is read AFTER r on bad-area recovery; action reset is separate.
    bool location_counters_already_advanced{}; // World owner ran old-s L/M before physics.
};
enum class ActorRetentionRequest {
    cleanup,
    clear_path,
    mark_escape32768,
    assign_hp1,
    reset_action
};
enum class ActorDeletionReason { none, spawn_timeout, empty_route, short_exit, unbound_monster };
struct ActorRetentionCandidate {
    ActorRetentionState state;
    bool delete_instance{};
    ActorDeletionReason reason{ActorDeletionReason::none};
    std::vector<ActorRetentionRequest> requests;
};
// d tail. Early deletion preserves later counters; ab cleanup modifies flags/state BEFORE all
// following guards. Returns ordered owner requests, never removes an actor or facility itself.
std::optional<ActorRetentionCandidate> prepare_actor_retention(const ActorRetentionInput &input);
struct ActorDecisionPrefix {
    std::uint32_t flags{};
    int blocked_battle_steps{};
    int attack_cooldown{}; // aj.
    bool low_hp{};
    bool restore_baseline{};
};
std::optional<ActorDecisionPrefix> prepare_actor_decision_prefix(std::uint32_t flags,
                                                                 int blocked_battle_steps,
                                                                 int attack_cooldown, int hp_target,
                                                                 int capacity);
} // namespace ark::simulation::rules
