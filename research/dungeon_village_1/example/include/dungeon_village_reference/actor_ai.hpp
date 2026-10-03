#pragma once

// Shared human/monster perception and small state decisions, not a full AI interpreter.
#include "dungeon_village_reference/character_motion.hpp"

namespace dungeon_village_reference {
enum class ActorKind { human, monster };
enum class ActorAiError { none, invalid_input };
struct BattleGateInput {
    ActorKind kind{ActorKind::human};
    std::uint32_t flags{};
    std::int32_t state_counter{}; // Old B from c(), before d() increments.
    bool in_move_area{};
    bool has_object{}; // N != -1, including the rescue sentinel -2.
    bool has_enemy{};
    bool inside_town{};
    bool enemy_inside_town{};
};
struct BattleGateCandidate {
    bool allowed{};
    std::optional<int> legacy_diagnostic_write; // Preserve old diagnostic when absent.
};
struct BattleGateResult {
    ActorAiError error{ActorAiError::none};
    std::optional<BattleGateCandidate> candidate;
};
// G() only: does not create encounters, check attack range or run movement-area K().
BattleGateResult prepare_battle_gate(const BattleGateInput &input);

struct EnemySnapshot {
    CharacterId id;
    int legacy_state{};
    bool in_move_area{};
    WorldPosition position;
    std::optional<std::uint64_t> encounter_id;
};
struct EnemySelectionInput {
    WorldPosition position;
    bool active_battle_group{}; // Bit128: caller supplies that group's opposite roster.
    std::optional<std::uint64_t> encounter_id;
    std::vector<EnemySnapshot> opposite_roster; // Original vector order, no sorting by ID.
};
struct EnemySelectionCandidate {
    CharacterId id;
    float world_distance{};
};
struct EnemySelectionResult {
    ActorAiError error{ActorAiError::none};
    std::optional<EnemySelectionCandidate> candidate; // Empty + none is a valid no-enemy result.
};
// Reject seven states and invalid movement area. Event-scoped input filters matching db identity;
// group-scoped input is already supplied by the owner. Preserve reverse-scan strict-less ties.
EnemySelectionResult select_combat_enemy(const EnemySelectionInput &input);

enum class IdleAiAction { keep, activity, battle_prepare };
struct HumanIdleInput {
    std::uint32_t flags{};
    bool active_task{};
    bool definition_task_flag{};
    bool battle_event_ready{}; // Resolved F(), deliberately not the G() combat gate.
    bool carrying_character{};
    bool nearby_event{};            // Caller resolves inclusive +/-3 grid test, not a path result.
    std::int32_t outside_counter{}; // M.
    std::int32_t reported_hp{};     // Result of g(), not automatically one particular HP slot.
    std::int32_t capacity_hp{};
    int effort{};
};
struct HumanIdleCandidate {
    IdleAiAction action{IdleAiAction::keep};
    std::optional<int> activity;
    bool clear_path{};
    bool run_spawn_probe{}; // Probe L() must observe resulting state, not cached state5.
    int hp_percent{};
    int reselect_threshold{};
};
struct HumanIdleResult {
    ActorAiError error{ActorAiError::none};
    std::optional<HumanIdleCandidate> candidate;
};
// State5 only, after common perception/preemption. Return the final queue request: a low-HP/timer
// activity0 can replace the earlier nearby-event activity6. Does not enqueue or create monsters.
HumanIdleResult prepare_human_idle(const HumanIdleInput &input);

enum class MonsterIdleAction { keep, enter_battle, cleanup };
struct MonsterIdleInput {
    int mode{}; // Original T, 0..4; this function is only state17.
    bool battle_gate{};
    bool path_returned_true{};     // P() return value; state17 does NOT propagate it as deletion.
    std::int32_t inside_counter{}; // L, not M.
};
struct MonsterIdleCandidate {
    MonsterIdleAction action{MonsterIdleAction::keep};
    bool advance_path{};
};
struct MonsterIdleResult {
    ActorAiError error{ActorAiError::none};
    std::optional<MonsterIdleCandidate> candidate;
};
// T0/2/3 use G(); T1/4 advance P(); T1 cleans up only when P is false and L>=500.
// There is no inferred T0 M>=1500 transition in the fixed bytecode.
MonsterIdleResult prepare_monster_idle(const MonsterIdleInput &input);
} // namespace dungeon_village_reference
