#pragma once

// Animation-window, projectile and hit candidates. No world ownership or renderer.
#include "dungeon_village_reference/actor_control.hpp"
#include "dungeon_village_reference/character_hp.hpp"
#include "dungeon_village_reference/combat_ai.hpp"

namespace dungeon_village_reference {
struct CombatPoint {
    float x{};
    float height{};
    float z{};
};
struct AttackSetupInput {
    int dexterity{}; // e.x[2], distinct from combat w[].
    int weapon_kind{};
    int weapon_combo{}; // p.j.
    int miss_low{};     // p.k/l, mapped using dexterity.
    int miss_high{};
    bool boosted{};
    std::array<int, 5> tickets{}; // Four perturbations then miss; all draws consumed.
    CombatRandomDraw draw{};      // 存在provider时替代固定数组，按源顺序实际抽5次100。
};
struct AttackSetupCandidate {
    int action{};
    int combo_count{};
    bool miss{};
    std::vector<LegacyActorControl> queue; // Excludes caller-specific facing/setup effects.
};
struct AttackSetupResult {
    CombatAiError error{CombatAiError::none};
    std::optional<AttackSetupCandidate> candidate;
};
AttackSetupResult prepare_human_attack(const AttackSetupInput &input);

struct HumanAttackFrame {
    int weapon_kind{};
    int counter{}; // l AFTER d() increment, BEFORE command14's extra increment.
    int combo_index{};
    int combo_count{1};
    bool armed{true}; // ai, consumed even if hit reports miss.
    bool enemy_found{};
    float enemy_distance{};
    int weapon_range{};
    std::optional<int> action{}; // Current k; absent uses setup's normal p.C[kind] projection.
};
struct HumanAttackCandidate {
    int counter{};
    int combo_index{};
    bool armed{};
    bool request_damage{};
    bool request_arrow{};
    bool completed{};
    bool query_enemy{}; // True only at an actual direct window or bow's current-action first point.
};
struct HumanAttackResult {
    CombatAiError error{CombatAiError::none};
    std::optional<HumanAttackCandidate> candidate;
};
// Command14 only; caller supplies a fresh e() result at each window, not the original target.
HumanAttackResult prepare_human_attack_frame(const HumanAttackFrame &input);
struct SpellFrameCandidate {
    bool prepare_visual{}; // l==1; offensive uses (u.x,u.y-1), healing uses u.
    bool query_target{};   // l==26; e() or J() must be fresh.
    bool request_effect{}; // Damage projectile or immediate healing request.
    bool completed{};      // l>=42, resets av; missing target at26 does not finish.
};
std::optional<SpellFrameCandidate> prepare_spell_frame(int counter, bool target_found,
                                                       int action = 4);
DamageResult prepare_healing_amount(int magic, std::optional<int> jitter_ticket,
                                    const CombatRandomDraw &draw = {});

struct AttackTargetSnapshot {
    CharacterId id;
    CombatPoint position;
    Position half_cell;
    bool eligible{}; // Shared seven-state/area gate only for fallback scan.
};
struct MonsterAttackFrame {
    int action{6};
    int counter{}; // Already advanced by d().
    bool armed{true};
    CombatPoint origin;            // n logical world position, never changed here.
    CombatPoint destination;       // p attack endpoint.
    CombatPoint previous_position; // au: hit lookup deliberately precedes this frame's movement.
    Position half_cell;
    int range{};
    std::optional<CharacterId> preferred; // az gets range/roster checks, NOT fallback eligibility.
    std::vector<AttackTargetSnapshot> humans;
};
struct MonsterAttackCandidate {
    CombatPoint position;
    bool armed{};
    std::optional<CharacterId> damage_target;
    bool completed{};
};
struct MonsterAttackResult {
    CombatAiError error{CombatAiError::none};
    std::optional<MonsterAttackCandidate> candidate;
};
MonsterAttackResult prepare_monster_attack_frame(const MonsterAttackFrame &input);

enum class ProjectileKind { arrow, spell, delayed_damage };
struct ProjectileState {
    ProjectileKind kind{ProjectileKind::arrow};
    CharacterId caster;
    CharacterId original_target;
    CombatPoint position;
    CombatPoint velocity;
    CombatPoint acceleration;
    int facing{};
    int counter{}; // n, old value checked BEFORE increment for type2.
    int delay{};   // j.
    int effect{};  // k on type1, 4..9.
    int damage{};  // l on type1, k on type2.
};
struct ProjectileResult {
    CombatAiError error{CombatAiError::none};
    std::optional<ProjectileState> candidate;
};
// Fixed source uses dx/vx even for vertical shots (0/0 => Java int0), then clamps to4.
// Maintenance rejects coincident positions instead of producing NaN velocities.
ProjectileResult prepare_projectile(ProjectileKind kind, CharacterId caster, CharacterId target,
                                    CombatPoint origin, CombatPoint destination, int facing,
                                    int effect = 0, int damage = 0, int delay = 0);
struct CollisionBox {
    float x_offset{};
    float z_offset{}; // Source z is the BOTTOM; extent runs towards negative z.
    float width{};
    float depth{};
};
struct ProjectileTarget {
    CharacterId id;
    CombatPoint position;
    CollisionBox box;
};
struct ProjectileContext {
    bool caster_reference{true};
    bool original_target_reference{true};   // Required even if another monster is hit.
    CollisionBox box;                       // Resolved n.a(0,8/9), not a guessed radius.
    std::vector<ProjectileTarget> monsters; // bm source order, NO seven-state/area filter.
};
struct ProjectileStepCandidate {
    ProjectileState state;
    bool remove{};
    std::optional<CharacterId> damage_target;
    bool physical_damage{}; // Recompute damage at arrow collision, not at launch.
    bool contact_effect{};  // Arrow contact even when the hit consumer reports miss.
    bool ground_effect22{};
    int visual_effect{};
    std::optional<ProjectileState> spawned; // Magic contact creates a delayed damage instance.
};
struct ProjectileStepResult {
    CombatAiError error{CombatAiError::none};
    std::optional<ProjectileStepCandidate> candidate;
};
ProjectileStepResult advance_projectile(const ProjectileState &state,
                                        const ProjectileContext &context);

enum class HitRequestKind {
    face_attacker,
    attack_sound,
    state,
    expression,
    reset_down_timer,
    drop_rescued_actor,
    clear_rescue_links,
    participant_down_count,
    global_down_count,
    event131,
    clear_recent_reward_and_kills,
    monster_human_kills,
    copy_attack_position,
    kill_count,
    kill_stat1,
    kill_statF,
    record_monster,
    participant_task_kills,
    spawn_drop,
    event217,
    global_monster_record
};
struct HitRequest {
    HitRequestKind kind{};
    int parameter{};
};
struct HitTargetState {
    ActorKind kind{ActorKind::human};
    std::uint32_t flags{};
    CharacterHpState hp;
    int capacity{};
    int damage_total{}; // ao.
    int hit_count{};    // ap.
    int hit_flash{};    // aw.
    int label_timer{};  // aq.
    bool miss_label{};  // ar.
};
struct HitContext {
    bool attacker_miss{}; // Read CURRENT caster.w; not captured by projectile creation.
    ActorKind attacker_kind{ActorKind::human};
    int weapon_kind{};
    bool attacker_visible{};
    bool attacker_first_visit{}; // 8192: suppresses drop AFTER consuming ticket100.
    bool target_carries_rescued_actor{};
    bool rescue_reference{};
    bool target_attack_locked{};      // E(): monster k3/6/9 or state8 preserves au.
    int victim_participant_matches{}; // Every matching definitionID in UserData.m, duplicates
                                      // count.
    int killer_participant_matches{};
    bool task_encounter{}; // db.k==3.
    int global_down_count{};
    bool event131_present{};
    bool boss_flags4{};
    int monster_rank{}; // k.f59f.
    bool event217_present{};
    int monster_stat1{}; // k.l, unrelated to delay/animation l.
    int monster_statF{}; // k.e(), already resolved from definition.
    int target_action{}; // g() reports0 for k7, otherwise the new am[3] target.
    std::optional<int> drop_ticket;
    CombatRandomDraw draw{}; // 真正怪物致命命中才抽100，首访8192仍先消费。
};
struct HitCandidate {
    HitTargetState target;
    bool landed{};
    bool lethal{};
    bool consumed_drop_ticket{};
    std::vector<HitRequest> requests; // Owner applies in order; state setter remains separate.
};
struct HitResult {
    CombatAiError error{CombatAiError::none};
    std::optional<HitCandidate> candidate;
};
// Pure local HP/labels/flags plus ordered cross-owner requests. No automatic corpse dedup:
// source collision can target already down/dead instances before their eventual removal.
HitResult prepare_hit(const HitTargetState &target, int damage, const HitContext &context);
} // namespace dungeon_village_reference
