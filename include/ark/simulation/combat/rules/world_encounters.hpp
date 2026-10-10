#pragma once

// Actual event map/task entry/snapshot side effects; not a main-scene or random clock owner.
#include "ark/simulation/combat/rules/encounter_creation.hpp"
#include "ark/simulation/ai/rules/world_perception.hpp"

namespace ark::simulation::rules {
struct WorldEventMapResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldMapFacts> facts;
};
// GameForm.i(): clear ONLY bit2, then current bn k0/k3 nine-cell squares outside inclusive town.
WorldEventMapResult prepare_world_event_map(const AiRewardState &state, const WorldMapFacts &facts);
// a.m.c(): flags4 adds min(completions*2,10); the base z is not a difficulty/rank or spawn count.
std::optional<int> prepare_task_encounter_quota(int base, int completions, std::uint32_t flags);
struct WorldEventEntryInput {
    CharacterId actor;
    WorldEventTask task;
    int base_quota{};
    int task_completions{};
    std::uint32_t task_flags{};
    std::function<std::optional<int>(int)> draw{};
};
struct WorldEventEntryCandidate {
    AiRewardState state;
    WorldMapFacts facts;
    WorldEventTask task; // Candidate k.g installation together with the candidate event owner.
    EventGateCandidate gate;
    std::optional<std::uint64_t> created;
    EncounterCreationDenial denial{EncounterCreationDenial::none};
    bool music2{};
    bool notice24{};
};
struct WorldEventEntryResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldEventEntryCandidate> candidate;
};
// F new-task branch: F stays true on expected creation denial; success sets quota/music/notice.
// No db bind in that same branch; an already existing task event is bound by a later F call.
WorldEventEntryResult prepare_world_event_entry(const AiRewardState &state,
                                                const WorldMapFacts &facts,
                                                const WorldEventEntryInput &input);
struct WorldEncounterUpdateCandidate {
    AiRewardState state;
    WorldMapFacts facts;
    std::vector<EncounterRequest> requests;
    bool removed{};
};
struct WorldEncounterUpdateResult {
    AiRewardError error{AiRewardError::none};
    std::optional<WorldEncounterUpdateCandidate> candidate;
};
// bn consumer uses the h.e field captured before all actor passes. Snapshot at original request
// position AFTER group/spawn. Map refresh runs at source requests before retiring event erasure.
WorldEncounterUpdateResult
prepare_world_encounter_update(const AiRewardState &state, const WorldMapFacts &facts,
                               EncounterCommitInput input,
                               const CombatInfluenceCandidate &start_of_round_field);
} // namespace ark::simulation::rules
