#pragma once

// Private real-source initial AI interval adapted from research d7ca763 prototype/startup_ai.
// Requires an unchanged first-visitor snapshot. Explicit window preview only, no calendar/map
// edits.
#include "ark/app/initial_ai_data.hpp"
#include "ark/app/startup_data.hpp"
#include "ark/economy/cash.hpp"
#include "ark/facilities/neighbourhood.hpp"
#include "ark/facilities/service.hpp"
#include "ark/people/departure.hpp"
#include "ark/people/hp.hpp"
#include <random>
namespace ark::app {
class Game;
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
    // Desktop preview policy: draw only at the rule's consumption point. A failed round also
    // discards RNG consumption; this is not a replay of the APK's random generator.
    InitialAiError round_random(std::mt19937 &random);

  private:
    InitialAiError prepare_round(const InitialAiTickets &, std::mt19937 *);
    InitialAiError decision(InitialAiState &, const InitialAiTickets &) const;
    InitialAiError execution(InitialAiState &, const InitialAiTickets &, std::mt19937 *) const;
    InitialAiError depart(InitialAiState &, const InitialAiTickets &, std::mt19937 *) const;
    InitialAiError arrive(InitialAiState &) const;
    InitialAiError exit(InitialAiState &, const InitialAiTickets &, std::mt19937 *) const;
    world::RouteMap map_;
    std::vector<facilities::InstanceId> instance_order_;
    std::map<facilities::InstanceId, facilities::Neighbourhood> neighbourhoods_;
    InitialAiState state_;
};
} // namespace ark::app
