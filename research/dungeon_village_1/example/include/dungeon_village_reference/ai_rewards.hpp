#pragma once

// Private battle/reward owner for death, encounter completion and definition-shared XP.
// Rendering, map refresh, battle-group update and quest spawning remain explicit requests.
#include "dungeon_village_reference/accounting.hpp"
#include "dungeon_village_reference/actor_lifecycle.hpp"
#include "dungeon_village_reference/battle_commit.hpp"
#include "dungeon_village_reference/combat_ai.hpp"
#include "dungeon_village_reference/human_growth.hpp"

namespace dungeon_village_reference {
struct RewardHumanDefinition {
    HumanDefinitionStatsInput definition;
    HumanDerivedStats derived;
    int experience{};
    DelayedRewardState pending;
    bool notice_pending{};
    std::array<std::array<int, 2>, 4> notice_attributes{{{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}}};
};
struct RewardActorContext {
    Position cell; // c() cached logical cell; not recomputed from world position after d().
    bool inside_town{};
    ActorEffectState effects;
    std::optional<int> facility_category;
    bool move_area{};     // aB[0], source qualification cached at c(), not a route result.
    Position half_cell{}; // t, separate from s whole cell.
    bool low_hp{};        // ak, refreshed from am3 at own c, not recomputed during J(other).
};
struct RewardMonsterDefinition {
    int defeats{};                  // k.u.
    int growth{};                   // k.v, increment AFTER event reward is resolved.
    int base_death_reward{};        // k.R.
    int base_cash_reward{};         // k.S.
    int base_hp{};                  // k.i, all existing instances read shared growth through h().
    int body{};                     // k.d.
    int sprite_variant{};           // k.f.
    int required_progress{};        // k.h, distinct from drop progress UserData.k.
    int status{};                   // k.p, maintained catalogue supports0/1.
    bool newly_unlocked{};          // k.r.
    bool introduced{};              // k.y.
    bool has_introduction_script{}; // k.t.length>0, execution stays an ordered request.
    int base_attack{};              // k.j.
    int base_defense{};             // k.k.
};
struct RewardEncounter {
    EncounterRuntimeState runtime;
    std::vector<CharacterId> members; // Original j, distinct from all matching db instances.
    bool group_exists{true};
    BattleGroupState group;
    int legacy_id{}; // f164b, distinct from maintenance runtime.id.
    std::optional<CombatInfluenceCandidate> influence{}; // p/r copied at event update, not own c.
    std::vector<int> human_scratch{};   // q, copied from p before each human movement evaluation.
    std::vector<int> monster_scratch{}; // s, copied from r before each monster evaluation.
};
struct AiRewardState {
    BattleCommitState battle; // Sole actor HP/control/J/K/statistics owner.
    std::map<CharacterId, BattleActorRecord>
        retired_actors; // Removed from bm, still referenced by g.
    std::vector<CharacterId> human_order;
    std::vector<CharacterId> monster_order;
    std::map<CharacterId, RewardActorContext> contexts;
    std::map<int, RewardHumanDefinition> growth;
    std::vector<HumanProfessionRule> professions; // Shared unlock state, not per human.
    std::map<int, RewardMonsterDefinition> monster_growth;
    std::vector<int> monster_definition_order;
    int monster_progress{}; // UserData.x, NOT drop_progress k.
    int monster_limit{4};   // Source n.Z initial; not a per-batch cap.
    std::map<std::uint64_t, RewardEncounter> encounters;
    std::vector<std::uint64_t> encounter_order; // bn source order, never sort restored originalIDs.
    std::map<std::uint64_t, RewardEncounter> retired_encounters; // Removed from bn, held by db/dc.
    // Owners outside this battle projection (facility crews, task/page references) must publish
    // their current roots BEFORE calling a consumer, since consumers can collect internally.
    std::set<CharacterId> external_actor_roots;
    // 共同设施所有者每次从实时occupants重建；与任务/UI显式根分开。
    std::vector<CharacterId> facility_actor_roots;
    std::set<std::uint64_t> external_encounter_roots; // Encounter ID0 remains a valid identity.
    std::map<std::uint64_t, ProjectileState> projectiles;
    std::vector<std::uint64_t> projectile_order;
    std::uint64_t next_projectile_id{1};
    PeriodAccounting accounting;
    std::uint64_t next_cash_id{1};
    std::uint64_t next_actor_id{3}; // Maintenance allocator, not original first-free UID.
    std::uint64_t next_encounter_id{1};
    int legacy_encounter_counter{}; // Source f.A: increment/modulo before collision scan.
    std::uint64_t period{1};  // Maintenance ledger identity, not original calendar/month number.
    int pending_completion{}; // UserData.f215e; not immediately converted to money/points.
    bool task_active{};
    bool task_completed{};
    bool feature16{};
};
enum class AiRewardError { none, invalid_input, stale_actor, stale_encounter, preparation_failed };
struct AiRewardCandidate {
    AiRewardState state;
    bool removed{};
    std::vector<LifecycleRequest> death_requests;
    std::vector<EncounterRequest> encounter_requests; // Includes still-external map/group/UI work.
    std::vector<HumanGrowthRequest> growth_requests;
};
struct AiRewardResult {
    AiRewardError error{AiRewardError::none};
    std::optional<AiRewardCandidate> candidate;
};
AiRewardResult prepare_monster_death_commit(const AiRewardState &state, CharacterId monster);
struct EncounterCommitInput {
    std::uint64_t encounter{};
    bool town_overlap{};
    EncounterQuest quest;
    std::vector<QuestSpawnCell> cells;
    std::vector<EncounterRandomTicket> tickets;
    std::vector<int> posture_tickets; // Separate group sub-consumer draws, before event draws.
    std::optional<std::array<int, 2>> spawn_offset_tickets;   // Two draws100 on actual spawn only.
    std::optional<CombatInfluenceCandidate> snapshot_field{}; // h.e cached BEFORE actor c/d.
};
// Rebuilds actor/shared reward inputs from the owner; quest spawn needs two offset tickets.
AiRewardResult prepare_encounter_reward_commit(const AiRewardState &state,
                                               const EncounterCommitInput &input);
// Runs once per calling HUMAN INSTANCE d(), not once per shared definition/world round.
AiRewardResult prepare_actor_growth_commit(const AiRewardState &state, CharacterId actor);
// c.f.a: caller alone gets128/an0/dc, both references append with duplicates preserved.
AiRewardResult prepare_battle_group_join(const AiRewardState &state, std::uint64_t encounter,
                                         CharacterId caller, CharacterId opponent);
// Reads each reference's current flags, including retired-but-still-referenced Java objects.
AiRewardResult prepare_battle_group_commit(const AiRewardState &state, std::uint64_t encounter,
                                           const std::vector<int> &posture_tickets = {});
// One c.f private a -> n.a -> c(8) -> db/bm/j append. No invented T2/3/4 constructors.
AiRewardResult prepare_encounter_monster_spawn(const AiRewardState &state, std::uint64_t encounter,
                                               const EncounterSpawnCandidate &spawn,
                                               const std::array<int, 2> &offset_tickets);
struct WorldProjectileInput {
    std::uint64_t projectile{};
    std::optional<CollisionBox> box; // n.a(0,8/9), required only for live arrow/spell consumers.
    std::array<std::optional<CollisionBox>, 4> monster_boxes; // n.a(1,g+3).
    std::optional<int> physical_jitter;
    int current_weapon_kind{}; // Resolved caster N() kind at collision, not at launch.
    bool caster_visible{};
    std::optional<int> drop_ticket;
    std::optional<DropSelectionInput> drop_selection;
};
struct WorldProjectileCandidate {
    AiRewardState state;
    ProjectileStepCandidate step;
    std::optional<HitCandidate> hit;
    std::optional<DamageCandidate> physical_damage;
    std::optional<std::uint64_t> spawned_projectile;
    std::vector<std::uint64_t> spawned_objects;
};
struct WorldProjectileResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldProjectileCandidate> candidate;
};
// Roster and reference liveness differ: delayed target/caster may be retained Java objects.
// Collision reads current monster roster/order and caster miss; all hit/drop/spawn changes atomic.
WorldProjectileResult prepare_world_projectile(const AiRewardState &state,
                                               const WorldProjectileInput &input);
// Trace Java-style object reachability from live actors/events/projectiles and explicit external
// roots, including R/S/db/dc. Removing an external root permits collection on the next call.
// No orphan-cycle retention and no re-entry into running rosters. Missing references stay errors
// for their consumer; collection does not manufacture an object to repair invalid input.
AiRewardState collect_ai_references(AiRewardState state);
// Rebuild effective attack/defense and boosts from current shared definitions/instances.
DamageResult prepare_actor_physical_damage(const AiRewardState &state, CharacterId attacker,
                                           CharacterId target, std::optional<int> jitter);
} // namespace dungeon_village_reference
