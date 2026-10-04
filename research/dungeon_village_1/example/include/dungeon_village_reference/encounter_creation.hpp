#pragma once

// Event creation/probe owner. Source catalog order, expected denials and malformed input differ.
#include "dungeon_village_reference/ai_perception.hpp"
#include "dungeon_village_reference/ai_rewards.hpp"

namespace dungeon_village_reference {
struct EncounterCreationDraw {
    std::optional<int> definition_ticket;
    std::array<int, 2> offset_tickets{};
};
struct EncounterCreationProbe {
    CharacterId actor;
    bool destination_inside_town{};
    int minimum_y{};
    std::optional<int> ticket;
    int logical_state{};
    std::vector<Position> task_centers; // k.a probes all bq, not merely the active task.
};
struct EncounterCreationInput {
    int kind{}; // f.g0 ordinary,1 retained,2 quest; NOT runtime k or monster T.
    Position center;
    std::array<bool, 3> upper_band_town{};       // Fresh h.c(x-1..x+1,y-1), source common guard.
    std::optional<EncounterCreationProbe> probe; // L->f.b->k.a, or direct f.a when absent.
    int year_index{};
    int month_index{};
    std::optional<int> count_ticket;
    std::optional<int> nearby_ticket;
    std::vector<EncounterCreationDraw> monsters;
    bool source_force_definition13{}; // f.v==1 debug guard; default source global is0.
};
enum class EncounterCreationDenial { none, probe, cell, task_overlap, town, limit, event_overlap };
enum class EncounterCreationRequestKind { definition_script, page89, refresh_map };
struct EncounterCreationRequest {
    EncounterCreationRequestKind kind{};
    int definition{};
};
struct EncounterCreationCandidate {
    AiRewardState state;
    std::optional<std::uint64_t> created;
    EncounterCreationDenial denial{
        EncounterCreationDenial::none}; // Expected no mutation, not error.
    bool consumed_probe{};
    bool consumed_count{};
    bool consumed_nearby{};
    std::size_t consumed_definitions{};
    std::size_t consumed_offsets{};
    std::vector<EncounterCreationRequest> requests;
};
struct EncounterCreationResult {
    AiRewardError error{AiRewardError::none};
    std::optional<EncounterCreationCandidate> candidate;
};
// Atomic event allocation -> count -> each definition unlock/intro/spawn -> state -> refresh.
// Gates return an unchanged candidate with denial; missing/invalid late draws return NO candidate.
// Quest quota remains0 here; F's caller installs m.c() only after successful creation.
EncounterCreationResult prepare_encounter_creation(const AiRewardState &state,
                                                   const EncounterCreationInput &input);
} // namespace dungeon_village_reference
