#pragma once

// Private real-source initial AI interval adapted from research d7ca763 prototype/startup_ai.
// Requires an unchanged first-visitor snapshot. No normal-window/calendar/map-edit admission.
#include "ark/app/game.hpp"
#include "ark/app/initial_ai_data.hpp"
#include "ark/economy/cash.hpp"
#include "ark/facilities/service.hpp"
#include "ark/people/departure.hpp"
#include "ark/people/hp.hpp"
namespace ark::app {
struct InitialAiTickets {
    int category{}, facility{}, satisfaction{};
    std::optional<int> attribute;
    std::optional<int> weapon;
};
struct InitialAiFacility {
    facilities::Placement placement;
    facilities::FacilityArrivalState sales;
    std::vector<people::ActorId> occupants;
};
struct InitialAiState {
    people::ActorId actor;
    people::ActorControlState control;
    people::ActorCounterState counters;
    people::ActorEffectState effects;
    people::HumanDefinitionStatsInput definition;
    people::HumanDerivedStats stats;
    people::CharacterHpState hp;
    facilities::FacilityArrivalState visits;
    std::map<int, facilities::FacilityUseProgress> uses;
    std::map<facilities::InstanceId, InitialAiFacility> facilities;
    economy::CashLedger accounting;
    int satisfaction{};
    std::vector<facilities::PopularityRequest> popularity_requests;
    std::vector<people::LegacyActorControl> presentation_requests;
    world::WorldPosition position;
    std::optional<people::FacilityDeparture> journey;
    std::optional<world::ArrivalTarget> active_facility;
    std::size_t waypoint{};
    int current_weapon{}, weapon_reselect_counter{};
    std::optional<int> selected_weapon;
    std::uint64_t rounds{}, next_cash_id{1};
    int arrivals{}, departures{}, completions{}, occupations{}, recoveries{};
    int attribute_commits{}, equipment_commits{};
};
enum class InitialAiError {
    none,
    invalid_start,
    invalid_input,
    unsupported_branch,
    preparation_failed
};
class InitialAiSession {
  public:
    // Select either published birth point for a repeatable conditional run, without changing Game.
    InitialAiSession(const Game &startup, world::Cell birth);
    const InitialAiState &state() const;
    // One admitted c/d round: any failure discards ALL effects, money, references and control
    // edits.
    InitialAiError round(const InitialAiTickets &tickets);

  private:
    InitialAiError decision(InitialAiState &, const InitialAiTickets &) const;
    InitialAiError execution(InitialAiState &, const InitialAiTickets &) const;
    InitialAiError depart(InitialAiState &, const InitialAiTickets &) const;
    InitialAiError arrive(InitialAiState &) const;
    InitialAiError exit(InitialAiState &, const InitialAiTickets &) const;
    world::RouteMap map_;
    std::vector<facilities::InstanceId> instance_order_;
    std::map<facilities::InstanceId, facilities::Neighbourhood> neighbourhoods_;
    InitialAiState state_;
};
} // namespace ark::app
