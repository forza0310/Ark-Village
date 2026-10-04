#pragma once

// Durable actor and facility-service state. Game owns these separately from static definitions,
// placement and shared facility progress; the initial interval reuses the same actor protocol.
#include "ark/facilities/service.hpp"
#include "ark/people/actor_housekeeping.hpp"
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
// Shared definition D must be supplied from evidence; absence is not an invented empty home.
struct LifeHome {
    world::Cell cell;
    int state{};
    int fourth_slot{}; // Preserve shared definition D[3]; this slice does not consume it.
};
enum class LifeHandoff { none, home_projection, encounter_creation, facility_consumer };
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
    // Original s/O/G are independent: c moves n, only the d tail refreshes cached_cell.
    world::Cell cached_cell{}, destination{};
    std::optional<world::Route> unbound_route;
    std::optional<world::ArrivalTarget>
        destination_binding; // O identity survives r/O path cleanup.
    std::optional<LifeHome> home;
    people::ActorRetentionState retention;
    int baseline{};
    bool move_area_before{};
    bool removed{}; // Scheduler removes the live projection only after successful submission.
    bool definition_departed{}; // Control26 writes shared definition m=1.
    LifeHandoff handoff{LifeHandoff::none};
    std::optional<int> spawn_ticket; // Actual L ticket for an unhandled encounter request.
    std::optional<world::Cell> spawn_center;
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
