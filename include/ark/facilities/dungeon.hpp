// Adapted from published research 4ba4805 and its prerequisites; independent product build.
#pragma once

// Category5 expedition crew, distinct from homes9 and recruitment10. No map/UI/catalog owner.
#include "ark/people/actor_id.hpp"
#include "ark/world/grid.hpp"
#include <optional>

#include <array>
#include <map>
#include <vector>

namespace ark::facilities {
struct DungeonActorProgress {
    int progress{};      // V; follower cap can make it NEGATIVE, not a malformed HP value.
    int previous{};      // W, captured before this facility update's ordinary advance.
    int percent{};       // X, scaled using GLOBAL active task's facility h, not necessarily this h.
    int endurance{3};    // Y =3+sum((professionLevel-1)/2), allowed to go negative after depletion.
    int retreat_state{}; // Z,0 exploring/1 retiring; other nonnegative values retained.
    int retreat_updates{}; // aa, increments for every occupant reference, not per unique ID.
    bool constrained{};    // bw, previous round's adjacent cap feeds leader bonus.
};
// Type0: threshold,type,status,age,rewardKind,rewardID; type1: last two are monsterID,strength.
using DungeonChallenge = std::array<int, 6>;
struct DungeonCrewState {
    int facility_updates{}; // f210f after outer Tenant.c increment, BEFORE crew branch.
    int phase{1};
    int progress{};         // g, monotonic max before challenge setbacks and follower cap.
    int extent{};           // h, completion guard. Global task extent is a SEPARATE input.
    int percent{};          // i.
    int previous_percent{}; // j.
    std::vector<people::ActorId> occupants; // o order; same Java object may occur more than once.
    std::map<people::ActorId, DungeonActorProgress> actors;
    std::vector<DungeonChallenge> challenges; // k, original ordered vector.
};
enum class DungeonCrewError { none, invalid_input, numeric_overflow, unresolved_retreat };
enum class DungeonCrewRequestKind {
    retreat,
    progress_label231,
    grant_catalog_reward,
    spawn_catalog_reward,
    exit_crew
};
struct DungeonCrewRequest {
    DungeonCrewRequestKind kind;
    std::optional<people::ActorId> actor;
    int first{};  // label x, reward kind, or exit delay.
    int second{}; // reward definition id.
};
struct DungeonRetreatResolution {
    people::ActorId actor;
    bool remove_first_occupation{}; // Actual a(cell)/q result; no assumed success or implicit RNG.
};
struct DungeonCrewInput {
    DungeonCrewState state;
    std::optional<int> active_task_extent;          // No active global task => a(progress)==0.
    std::vector<DungeonRetreatResolution> retreats; // Actual old-Z1/aa20+ reverse-order consumers.
};
struct DungeonCrewCandidate {
    DungeonCrewState state;
    int leader_step{10};
    std::size_t consumed_retreats{};
    std::vector<DungeonCrewRequest> requests; // Original dispatch order; NOT global mutations.
    bool completed{};
};
struct DungeonCrewResult {
    DungeonCrewError error{DungeonCrewError::none};
    std::optional<DungeonCrewCandidate> candidate;
};
// Category5 phase1 ONLY, after Tenant's counter/display prefix. Map retreat, item catalog/drop,
// participant progress labels and actor c0/exit queues are explicit ordered world requests.
DungeonCrewResult prepare_dungeon_crew(const DungeonCrewInput &input);
std::optional<int> dungeon_endurance(const std::vector<int> &profession_levels);
// Occupation21 resets V/W/X/Z/aa and derives Y; does not imply ownership of a home.
std::optional<DungeonActorProgress> prepare_dungeon_actor_entry(const std::vector<int> &levels,
                                                                bool old_constrained = false);
// Tenant.b(1): difficulty1..9 interpolates Q*120..Q*240, integer arithmetic.
std::optional<int> dungeon_extent(int difficulty, int calendar_q);
struct DungeonCompletionActor {
    people::ActorId id;
    int definition{};
};
struct DungeonCompletionTask {
    int kind{}; // m.d:1 cancels; other supported kinds follow completion.
    int difficulty{};
    std::vector<int> participants;                   // Global UserData.m, including duplicates.
    std::vector<DungeonChallenge> global_challenges; // Global task's facility, NOT caller facility.
    int definition{};               // Original task definition identity; 0 is valid.
    int pending_completion_value{}; // Task m.m -> UserData.f215e; not immediate XP or cash.
};
struct DungeonSiteTask {
    int kind{};
    std::optional<world::Cell> site;
};
enum class DungeonCompletionRequestKind {
    restore_site,
    remove_site_task,
    clear_active_task,
    actor_reward_display,
    definition_reward,
    clear_boost,
    summary30,
    summary32,
    event126,
    record_complete,
    record_task_success,
    event201,
    event92
};
struct DungeonCompletionRequest {
    DungeonCompletionRequestKind kind;
    std::optional<people::ActorId> actor;
    int first{};
    int second{};
};
struct DungeonCompletionInput {
    int updates{};           // AFTER Tenant.c prefix; no completion before10.
    world::Cell source_site; // z[0].n, not assumed equal to instance r.
    std::optional<DungeonCompletionTask> active_task;
    std::vector<DungeonCompletionActor> human_order; // Actual bl, first matching definition wins.
    std::vector<DungeonSiteTask> site_tasks; // bq original order, no-task removal scans reverse.
    std::vector<int> summary_tickets; // Two independent participant-count draws at summary32.
    bool event201_seen{};
    bool event92_seen{};
};
struct DungeonCompletionCandidate {
    std::vector<DungeonCompletionRequest> requests;
    std::vector<std::array<int, 2>> summary_rewards; // ALL remaining type0 records, any status.
    std::size_t consumed_summary_tickets{};
};
// Phase2 ONLY: ordered requests for the task/map/UI owners, not an automatic catalog re-grant.
std::optional<DungeonCompletionCandidate>
prepare_dungeon_completion(const DungeonCompletionInput &input);
} // namespace ark::facilities
