#pragma once

// Narrow battle/object owner projections. Definition statistics are shared, actor HP is not.
#include "dungeon_village_reference/object_ai.hpp"

#include <map>
#include <set>

namespace dungeon_village_reference {
struct HumanBattleRecord {
    int kills{};              // B0.
    int killed_stat1{};       // B1.
    int battle_reward_stat{}; // F.
    int task_kills{};         // H.
    int participant_downs{};  // I.
    int recent_reward{};      // J.
    int recent_kills{};       // K.
    int luck{};               // Derived x5, not raw base attribute.
};
struct MonsterBattleRecord {
    int human_kills{};  // x.
    int stat1{};        // l.
    int statF{};        // e(), resolved definition value.
    int death_reward{}; // d(), used by e.a(monster) recent reward50%.
    int rank{};
    std::uint32_t flags{};
};
struct BattleActorRecord {
    CharacterId id; // Maintenance global identity, independent of roster-scoped legacyUID.
    ActorKind kind{ActorKind::human};
    int definition{};
    ActorControlState control; // Sole local flags/action/state owner.
    CharacterHpState hp;
    int capacity{};
    int baseline{};
    int state_counter{};
    int state_parameter{};
    int attack_count{};
    int down_timer{};
    int object_slot{-1};
    std::optional<CharacterId> rescue;
    std::optional<std::uint64_t> encounter;
    std::optional<std::uint64_t> group; // Owning encounter of dc, not a global roster identity.
    int attack_slot{};                  // an.
    int monster_posture{};              // ay.
    int legacy_id{}; // f143e is roster-scoped/reusable, unlike maintenance stable id.
    int body{};
    int sprite{}; // k.d*30+k.f, not a global SEB resource index.
    CombatPoint position;
    CombatPoint attack_position;
    bool miss{};
    int damage_total{};
    int hit_count{};
    int hit_flash{};
    int label_timer{};
    bool miss_label{};
};
struct BattleCommitState {
    std::map<CharacterId, BattleActorRecord> actors;
    std::map<int, HumanBattleRecord> humans;
    std::map<int, MonsterBattleRecord> monsters;
    std::vector<int> participants;            // Source UserData.m, preserve duplicates and order.
    std::set<std::uint64_t> quest_encounters; // Only kind3 membership, no invented quest reward.
    std::set<int> events;
    int global_downs{};
    std::vector<int> defeated_definitions; // UserData.N appends original definitionID, duplicates.
    int drop_progress{};
    std::map<std::uint64_t, GroundObjectState> objects;
    std::uint64_t next_object_id{1};
};
struct BattleCommitInput {
    CharacterId attacker;
    CharacterId target;
    int damage{};
    int weapon_kind{};
    bool attacker_visible{};
    std::optional<int> drop_ticket; // Lethal monster: always consumes100 before first-visit guard.
    std::optional<DropSelectionInput> drop_selection; // Required only when hit requests spawn_drop.
};
enum class BattleCommitError {
    none,
    invalid_input,
    stale_actor,
    preparation_failed,
    numeric_overflow
};
struct BattleCommitCandidate {
    BattleCommitState state;
    HitCandidate hit;
    std::optional<DropSelectionCandidate> drop;
    std::vector<HitRequest> presentation; // Sounds/expressions/facing remain typed requests.
};
struct BattleCommitResult {
    BattleCommitError error{BattleCommitError::none};
    std::optional<BattleCommitCandidate> candidate;
};
// Prepare both actor/definition/global/rescue/drop mutations on a private copy. Late failure
// returns no partial state. Retains corpse re-hits: no fabricated death deduplication barrier.
BattleCommitResult prepare_battle_commit(const BattleCommitState &state,
                                         const BattleCommitInput &input);
} // namespace dungeon_village_reference
