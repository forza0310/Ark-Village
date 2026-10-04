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
};
struct RewardMonsterDefinition {
    int defeats{};           // k.u.
    int growth{};            // k.v, increment AFTER event reward is resolved.
    int base_death_reward{}; // k.R.
    int base_cash_reward{};  // k.S.
    int base_hp{};           // k.i, all existing instances read shared growth through h().
    int body{};              // k.d.
    int sprite_variant{};    // k.f.
};
struct RewardEncounter {
    EncounterRuntimeState runtime;
    std::vector<CharacterId> members; // Original j, distinct from all matching db instances.
    bool group_exists{true};
    BattleGroupState group;
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
    std::map<std::uint64_t, RewardEncounter> encounters;
    PeriodAccounting accounting;
    std::uint64_t next_cash_id{1};
    std::uint64_t next_actor_id{3}; // Maintenance allocator, not original first-free UID.
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
    std::optional<std::array<int, 2>> spawn_offset_tickets; // Two draws100 on actual spawn only.
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
} // namespace dungeon_village_reference
