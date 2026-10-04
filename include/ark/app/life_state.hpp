#pragma once

// Durable actor and facility-service state. Game owns these separately from static definitions,
// placement and shared facility progress; the initial interval reuses the same actor protocol.
#include "ark/facilities/service.hpp"
#include "ark/people/departure.hpp"
#include "ark/people/hp.hpp"
#include "ark/people/human_growth.hpp"

namespace ark::app {
enum class InitialAiError {
    none,
    invalid_start,
    invalid_input,
    unsupported_branch,
    preparation_failed
};
struct FacilityLifeState {
    facilities::FacilityArrivalState sales;
    std::vector<people::ActorId> occupants;
};
struct LifeActorState {
    people::ActorId actor;
    people::ActorControlState control;
    people::ActorCounterState counters;
    people::ActorEffectState effects;
    people::HumanDefinitionStatsInput definition;
    people::HumanDerivedStats stats;
    people::CharacterHpState hp;
    facilities::FacilityArrivalState visits; // Facility sales slot is scratch, restored from owner.
    int satisfaction{};
    std::vector<facilities::PopularityRequest> popularity_requests;
    std::vector<people::LegacyActorControl> presentation_requests;
    world::WorldPosition position;
    std::optional<people::FacilityDeparture> journey;
    std::optional<world::ArrivalTarget> active_facility;
    std::size_t waypoint{};
    int current_weapon{}, weapon_reselect_counter{};
    std::optional<int> selected_weapon;
    std::uint64_t rounds{}, route_revision{};
    int arrivals{}, departures{}, completions{}, occupations{}, recoveries{};
    int attribute_commits{}, equipment_commits{};
    // Product handoff: keep the actual selection and queue instead of rerolling missing rules.
    InitialAiError error{InitialAiError::none};
    std::optional<int> pending_category, pending_definition, pending_activity;
};
} // namespace ark::app
