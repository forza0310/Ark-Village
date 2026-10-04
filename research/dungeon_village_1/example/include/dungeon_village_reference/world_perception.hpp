#pragma once

// Current map facts plus cached actor facts. c perception NEVER eagerly reprojects s/t from n.
#include "dungeon_village_reference/activity_candidates.hpp"
#include "dungeon_village_reference/actor_housekeeping.hpp"
#include "dungeon_village_reference/ai_perception.hpp"
#include "dungeon_village_reference/ai_rewards.hpp"

namespace dungeon_village_reference {
struct WorldMapFacts {
    LegacyMap map;
    std::vector<int> surface;         // Source i.g, not logical state or route category.
    std::vector<std::uint32_t> flags; // i.o; bit2 is the current event map, not guessed centers.
    TownBounds town;                  // Current h.l[n.o] interior, exclusive boundaries.
};
struct WorldPerceptionCandidate {
    AiRewardState state;
    MoveAreaCandidate area;
    std::optional<EnemySelectionCandidate> enemy;
    float sensed_distance{}; // aA; no enemy means actual Float.MAX_VALUE.
    bool restored_baseline{};
};
struct WorldPerceptionResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldPerceptionCandidate> candidate;
};
bool valid_world_map_facts(const WorldMapFacts &facts);
// K from current n+four unit probes; db may reference an object already removed from bn.
std::optional<MoveAreaCandidate>
query_world_move_area(const AiRewardState &state, CharacterId actor, const WorldMapFacts &facts);
// e reads each opponent's currently cached area; group duplicates/retired references preserved.
EnemySelectionResult query_current_combat_enemy(const AiRewardState &state, CharacterId actor);
// c common prefix only: flags2/bu/ak/ax/K -> optional baseline150 -> aj decrement -> e/az/128.
// R repair, preemption, state branch and physics are subsequent consumers, not silently skipped.
WorldPerceptionResult prepare_world_perception_prefix(const AiRewardState &state, CharacterId actor,
                                                      const WorldMapFacts &facts,
                                                      int monster_mode = 0);
// The next c segment: R repair then rescue-before-object idle preemption. No state16 follow here.
// object_order is current bp order, separate from the stable-ID storage map.
WorldPerceptionResult prepare_world_reference_preemption(
    const AiRewardState &state, CharacterId actor, const WorldMapFacts &facts, bool rescue_enabled,
    bool definition_task_flag, const std::vector<std::uint64_t> &object_order);
// Global h.e BEFORE any current c: preserve original order, old aB0/t and surface masking.
CombatInfluenceResult prepare_world_influence(const AiRewardState &state,
                                              const WorldMapFacts &facts);
// J is a cached-ak/state2 query, not a fresh recomputation of all other actors' HP.
struct WorldHealingTargetResult {
    AiRewardError error{AiRewardError::none};
    std::optional<CharacterId> target;
};
WorldHealingTargetResult query_world_healing_target(const AiRewardState &state, CharacterId actor);
// Original d projection point only. Retention counters must read old cached s BEFORE this call.
WorldPerceptionResult prepare_world_actor_projection(const AiRewardState &state, CharacterId actor);
struct WorldEventTask {
    bool definition_task_flag{};
    int kind{};
    Position center;
    std::optional<std::uint64_t> encounter;
};
struct WorldEventGateCandidate {
    AiRewardState state;
    EventGateCandidate gate; // New task creation is a request; F true does not imply success.
};
struct WorldEventGateResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldEventGateCandidate> candidate;
};
// F rebuilds current bn order and map bit2, preserves old db on no match, unlike G1024 guard.
WorldEventGateResult prepare_world_event_gate(const AiRewardState &state, CharacterId actor,
                                              const WorldMapFacts &facts,
                                              const WorldEventTask &task = {});
struct WorldPhysicsCandidate {
    AiRewardState state;
    bool queried_area{};
    std::optional<int> diagnostic;
};
struct WorldPhysicsResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldPhysicsCandidate> candidate;
};
// d tail AFTER old-s L/M counters: P/gravity -> freshK -> battle/knockback reversal -> s/t.
// Height retained when only horizontal n is restored; physics never refreshes cached ax.
WorldPhysicsResult prepare_world_physics_projection(const AiRewardState &state, CharacterId actor,
                                                    const WorldMapFacts &facts);
struct WorldCombatMoveCandidate {
    AiRewardState state;
    CombatMoveCandidate scores;
    std::optional<WorldPosition> target;
    int diagnostic{5}; // bp4 missing db,5 no move/center,6 b(world).
};
struct WorldCombatMoveResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldCombatMoveCandidate> candidate;
};
// Execute event's snapshot_influence request only at its original update point, not before c.
WorldPerceptionResult
prepare_world_encounter_influence(const AiRewardState &state, std::uint64_t encounter,
                                  const CombatInfluenceCandidate &global_field);
// c(world)/d(world): copy source to scratch, original nine samples, then6.7 move toward
// half-center. Invalid low-influence score0 may win; no invented nav filtering/snapping/cell
// refresh here.
WorldCombatMoveResult prepare_world_combat_move(const AiRewardState &state, CharacterId actor,
                                                CharacterId enemy, const WorldMapFacts &facts,
                                                bool low_influence);
struct WorldExecutionPrefixCandidate {
    AiRewardState state;
    std::vector<ActorEffectSound> sounds;
    std::vector<HumanGrowthRequest> growth_requests;
    bool request_carry_expression{}; // c17 MUST consume display probability after growth, before v.
};
struct WorldExecutionPrefixResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldExecutionPrefixCandidate> candidate;
};
// Actual common d prefix: i/l/B/aw -> ce/cd -> aq -> HP display -> human definition growth.
// No control, old-s retention or physics here; one owner calls each segment once in source order.
WorldExecutionPrefixResult prepare_world_execution_prefix(const AiRewardState &state,
                                                          CharacterId actor);
struct WorldBattlePreparationResult {
    AiRewardError error{AiRewardError::none};
    std::optional<AiRewardState> state;
    std::optional<BattlePreparationCandidate> preparation;
};
// State18 after common c: current G first, then old-s map/event/legacyID+center, then512.
WorldBattlePreparationResult prepare_world_battle_preparation(const AiRewardState &state,
                                                              CharacterId actor,
                                                              const WorldMapFacts &facts,
                                                              int monster_mode = 0);
} // namespace dungeon_village_reference
