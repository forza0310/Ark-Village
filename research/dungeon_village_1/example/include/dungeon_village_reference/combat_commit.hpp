#pragma once

// Actual14..17 command segment on the same HP/statistics/reward owner used by projectiles.
// d() counters/effects/HP/growth precede this segment; no duplicate ticking happens here.
#include "dungeon_village_reference/ai_perception.hpp"
#include "dungeon_village_reference/ai_rewards.hpp"
#include "dungeon_village_reference/world_perception.hpp"

namespace dungeon_village_reference {
struct CombatWeaponRule {
    int kind{};
    int range{};
    int combo{1};
    int miss_low{};
    int miss_high{};
};
using WorldCombatFacingConsumer =
    std::function<std::optional<int>(CharacterId, CharacterId)>; // 原b(self,target)，旧u决定。
struct WorldAttackSetupInput {
    CharacterId actor;
    CharacterId target;
    CombatWeaponRule weapon; // Resolved current N(), not a per-attack cached damage value.
    std::array<int, 5> human_tickets{};
    std::optional<int> monster_miss_ticket; // d.a(100)<aR12.
    std::optional<int> facing; // Source projected facing helper; never guessed from grid axes.
    CombatRandomDraw draw{};
    WorldCombatExpressionConsumer expression{}; // human c1先于五次attack100。
};
enum class WorldAttackVisual {
    expression,
    contact,
    spell_source,
    healing_source,
    healing_target,
    telegraph,
    cast_sound
};
struct WorldAttackRequest {
    WorldAttackVisual kind{};
    CharacterId actor;
    std::optional<CharacterId> target;
    int parameter{};
};
struct WorldAttackInput {
    CharacterId actor;
    CombatWeaponRule weapon;
    std::optional<int> physical_jitter;
    std::optional<int> spell_ticket;
    std::optional<int> enhancement_ticket;
    std::optional<int> magic_jitter;
    std::optional<int> facing; // Required only when an arrow/spell is actually launched.
    bool actor_visible{};
    std::optional<int> drop_ticket;
    std::optional<DropSelectionInput> drop_selection;
    CombatRandomDraw draw{};
    WorldCombatExpressionConsumer expression{}; // 真实down c2/16、monster成功命中后c0。
    WorldCombatEventConsumer event{};           // 原hit内131/217同步脚本，typed全球/I写回。
    WorldCombatFacingConsumer facing_for{}; // 仅真实发射目标确定之后调用，不能按旧j复用。
};
struct WorldAttackCandidate {
    AiRewardState state;
    bool completed{};
    bool early_stop{true}; // Ongoing command holds interpreter; completed consumer can continue.
    std::optional<CharacterId> target;
    std::optional<HitCandidate> hit;
    std::optional<DamageCandidate> physical_damage;
    std::optional<std::uint64_t> projectile;
    std::vector<std::uint64_t> objects;
    std::vector<WorldAttackRequest> requests; // Display/sound/face-position adapters stay explicit.
    std::optional<std::vector<std::array<int, 3>>> popularity_queue{};
};
struct WorldAttackResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldAttackCandidate> candidate;
};
// b(target): queue reset, attack count/armed/miss/endpoint, actual queue; random failure atomic.
WorldAttackResult prepare_world_attack_setup(const AiRewardState &state,
                                             const WorldAttackSetupInput &input);
// Current front command only. Fresh e()/J(), current growth/boosts, hit/drop/projectile/healing
// and command completion commit together. No eager execution of the next action/wait command.
WorldAttackResult prepare_world_attack_control(const AiRewardState &state,
                                               const WorldAttackInput &input);
// Local v prefix -> at most one14..17 segment -> same-d local continuation after completion.
// Unhandled domain front commands remain delegated; never consumes the common d counters twice.
WorldAttackResult prepare_world_attack_execution(const AiRewardState &state,
                                                 const WorldAttackInput &input);
struct WorldCombatPolicyInput {
    CharacterId actor;
    CombatWeaponRule weapon; // Current N() catalogue projection, no mutable attribute snapshot.
    int profession_role{};   // Current cached ad -> immutable h.g, NOT profession definition ID.
    int monster_range{};     // Immutable a.k.E[g] catalogue projection.
    int monster_mode{};
    std::optional<int> policy_ticket;
    std::optional<int> healing_ticket;
    std::optional<int> boost_ticket;
    std::array<int, 5> attack_tickets{};
    std::optional<int> monster_miss_ticket;
    std::optional<int> facing; // Original projected-facing adapter; consumed only when needed.
    CombatRandomDraw draw{};
    WorldCombatExpressionConsumer expression{};
    WorldCombatFacingConsumer facing_for{};
};
struct WorldCombatPolicyCandidate {
    AiRewardState state;
    CombatStrategyCandidate strategy;
    std::optional<CharacterId> fresh_enemy;
    std::optional<WorldPosition> move_target;
    std::vector<WorldAttackRequest> requests;
    bool consumed_boost_ticket{};
};
struct WorldCombatPolicyResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldCombatPolicyCandidate> candidate;
};
// Execute state1 only AFTER the common c prefix/reference-preemption. Counters/group/state,
// strategy movement and attack/spell queues commit on the same private owner, never eager hits.
WorldCombatPolicyResult prepare_world_combat_policy(const AiRewardState &state,
                                                    const WorldCombatPolicyInput &input,
                                                    const WorldMapFacts &facts);
} // namespace dungeon_village_reference
