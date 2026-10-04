#pragma once

// Actual14..17 command segment on the same HP/statistics/reward owner used by projectiles.
// d() counters/effects/HP/growth precede this segment; no duplicate ticking happens here.
#include "dungeon_village_reference/ai_perception.hpp"
#include "dungeon_village_reference/ai_rewards.hpp"

namespace dungeon_village_reference {
struct CombatWeaponRule {
    int kind{};
    int range{};
    int combo{1};
    int miss_low{};
    int miss_high{};
};
struct WorldAttackSetupInput {
    CharacterId actor;
    CharacterId target;
    CombatWeaponRule weapon; // Resolved current N(), not a per-attack cached damage value.
    std::array<int, 5> human_tickets{};
    std::optional<int> monster_miss_ticket; // d.a(100)<aR12.
    std::optional<int> facing; // Source projected facing helper; never guessed from grid axes.
};
enum class WorldAttackVisual { expression, contact, spell_source, healing_source, healing_target };
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
} // namespace dungeon_village_reference
