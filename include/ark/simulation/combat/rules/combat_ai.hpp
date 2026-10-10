#pragma once

#include "ark/simulation/ai/rules/actor_ai.hpp"

#include <array>
#include <functional>

namespace ark::simulation::rules {
// 只允许捕获外层私有Owner的随机状态；失败时Owner整体丢弃，不推进真实随机流。
using CombatRandomDraw = std::function<std::optional<int>(int)>;
enum class CombatAiError { none, invalid_input, missing_ticket, invalid_ticket };
enum class CombatDecision {
    keep,
    baseline,
    battle_prepare,
    join_group,
    approach,
    low_influence,
    physical_attack,
    offensive_spell,
    healing_spell
};
struct CombatStrategyInput {
    ActorKind kind{ActorKind::human};
    std::uint32_t flags{};
    int action{}; // k, not A. Caller invokes only for A==1.
    bool in_move_area{};
    bool sensed_enemy{};
    bool same_town_side{};
    bool fresh_enemy{};    // Second e() lookup in this branch.
    int monster_posture{}; // ay, 0..2.
    int group_tick{};
    int attack_slot{};                 // an.
    int group_cycle{};                 // dc.g, 0..2.
    int profession_role{};             // h.g, 0..4; not the profession definition ID.
    float sensed_distance{};           // aA used by human policy.
    float fresh_distance{};            // e().bv used by monster policy.
    int weapon_range{};                // p.i.
    int weapon_kind{};                 // p.h, 0..3.
    int monster_range{};               // k.E[g].
    std::array<bool, 4> spells{};      // e.Q; attack0..2, healing3.
    bool healing_target{};             // J() already resolved in original roster order.
    std::optional<int> policy_ticket;  // d.a(100), only human at attack_slot.
    std::optional<int> healing_ticket; // d.a(10), only if both spell kinds available and chosen.
    CombatRandomDraw draw{};           // 缺显式票号时，实际分支才请求100/10。
};
struct CombatStrategyCandidate {
    CombatDecision decision{CombatDecision::keep};
    bool increment_attack_idle{true}; // av++ occurs before lock/area checks.
    bool reset_encounter_idle{};      // Monster with db resets event.m before the lock.
    bool face_enemy{};
    bool clear_animation_flag{}; // z bit2 in [an-16,an).
    bool telegraph{};            // cd23 exactly at an-6.
    bool consumed_policy_ticket{};
    bool consumed_healing_ticket{};
    int policy_column{-1};
};
struct CombatStrategyResult {
    CombatAiError error{CombatAiError::none};
    std::optional<CombatStrategyCandidate> candidate;
};
// Pure state1 policy. Attack/spell setup and command execution remain separate commits.
// No random draw outside the proven branches; failed preparation has no partial request.
CombatStrategyResult prepare_combat_strategy(const CombatStrategyInput &input);

struct CombatMoveSample {
    bool in_half_grid{};
    bool in_encounter_square{};
    int influence{}; // Source influence clamped at upper100 only.
    float enemy_distance{1000.0F};
};
struct CombatMoveCandidate {
    std::array<int, 9> scores{};
    std::optional<std::size_t> selected; // Empty for no positive approach score or center4.
};
struct CombatMoveResult {
    CombatAiError error{CombatAiError::none};
    std::optional<CombatMoveCandidate> candidate;
};
// Source order: (-1,+1),(0,+1),(+1,+1),(-1,0),(0,0),(+1,0),(-1,-1),(0,-1),(+1,-1).
// Low-influence mode deliberately leaves invalid samples at score0: it is not pathfinding.
CombatMoveResult prepare_combat_step(const std::array<CombatMoveSample, 9> &samples,
                                     bool low_influence);

struct PhysicalDamageInput {
    ActorKind kind{ActorKind::human};
    int effective_attack{};  // Human w1 or already growth-adjusted monster attack.
    int effective_defense{}; // Human w2 or already growth-adjusted monster defense.
    bool human_boost{};   // Instance2048: attacker for human damage, defender for monster damage.
    bool monster_boost{}; // Instance4096: defender for human damage, attacker for monster damage.
    std::optional<int> jitter_ticket;
    CombatRandomDraw draw{};
};
struct DamageCandidate {
    int base{};
    int jitter_bound{};
    int value{}; // Damage amount; no HP/reward/knockback commit here.
};
struct DamageResult {
    CombatAiError error{CombatAiError::none};
    std::optional<DamageCandidate> candidate;
};
// Preserve Java integer attack/defense division BEFORE float interpolation; not a continuous ratio.
// Caller provides effective monster growth stats; overflowing maintenance inputs are refused.
DamageResult prepare_physical_damage(const PhysicalDamageInput &input);
// Definition v growth: category0 HP/attack/defense,1 reward fields,2 tier count.
// Boss is DEFINITION flags4, distinct from instance4096 damage/rage. Overflow is refused.
std::optional<int> prepare_monster_growth(int base, int growth, int category, bool boss);
struct SpellDamageInput {
    int magic{};
    std::array<bool, 3> learned{};
    bool human_boost{};
    std::optional<int> spell_ticket;       // Original Q0..2 learned order.
    std::optional<int> enhancement_ticket; // 0..99, below mapped5..50 => enhanced.
    std::optional<int> jitter_ticket;
    CombatRandomDraw draw{};
};
struct SpellDamageCandidate {
    DamageCandidate damage;
    int effect{}; // 4..6 normal,7..9 enhanced; not an actor state/opcode.
};
struct SpellDamageResult {
    CombatAiError error{CombatAiError::none};
    std::optional<SpellDamageCandidate> candidate;
};
SpellDamageResult prepare_spell_damage(const SpellDamageInput &input);

struct InfluenceActor {
    int state{};
    bool previous_move_area{}; // h.e() runs BEFORE this round's actor c() recomputes K().
    Position half_cell;
};
struct CombatInfluenceInput {
    int map_width{};
    int map_height{};
    std::vector<int> legacy_surface;    // Row-major full grid i.g, independent from logical state.
    std::vector<InfluenceActor> humans; // Original global roster order, not group membership.
    std::vector<InfluenceActor> monsters;
};
struct CombatInfluenceCandidate {
    int width{}; // Original half grid: map_width*2.
    int height{};
    std::vector<int> human_field;
    std::vector<int> monster_field;
};
struct CombatInfluenceResult {
    CombatAiError error{CombatAiError::none};
    std::optional<CombatInfluenceCandidate> candidate;
};
bool valid_combat_influence_field(const CombatInfluenceCandidate &field);
// Add opposing25-cell kernels first, then multiply friendly9-cell kernels with per-actor
// truncation. Preserve the fixed bytecode's terrain mask at (gridX/2,gridY/2), NOT gridX*2/gridY*2.
CombatInfluenceResult prepare_combat_influence(const CombatInfluenceInput &input);
} // namespace ark::simulation::rules
