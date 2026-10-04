#pragma once

// A bounded, private first-play AI owner. It is not wired to the normal window loop or calendar.
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_reference/actor_effects.hpp"
#include "dungeon_village_reference/ai_schedule.hpp"
#include "dungeon_village_reference/character_hp.hpp"
#include "dungeon_village_reference/facility_departure.hpp"
#include "dungeon_village_reference/facility_service.hpp"
#include "dungeon_village_reference/human_growth.hpp"
#include "dungeon_village_reference/weapon_choice.hpp"

namespace dungeon_village_prototype {
struct StartupWeaponRule {
    ref::WeaponChoiceDefinition selection;
    int price{};
    std::array<int, 4> combat{};
};
struct StartupAiRules {
    ref::HumanDefinitionStatsInput first_definition;
    std::vector<ref::HumanProfessionRule> professions;
    std::vector<StartupWeaponRule> weapons;
};
const StartupAiRules &startup_ai_rules(); // Generated from fixed source tables, not demo values.
struct StartupAiTickets {
    int category{};
    int facility{};
    int satisfaction{};
    std::optional<int> attribute;
    std::optional<int> weapon;
};
struct StartupAiFacility {
    ref::FacilityPlacement placement;
    ref::FacilityArrivalState sales; // Only current_month_facility_sales consumed here.
    std::vector<ref::CharacterId> occupants;
};
struct StartupAiState {
    ref::CharacterId actor;
    ref::ActorControlState control;
    ref::ActorCounterState counters;
    ref::ActorEffectState effects;
    ref::HumanDefinitionStatsInput definition;
    ref::HumanDerivedStats stats;
    ref::CharacterHpState hp;
    ref::FacilityArrivalState visits;
    std::map<int, ref::FacilityUseProgress> uses; // Definition-shared, not instance-shared.
    std::map<std::uint64_t, StartupAiFacility> facilities;
    ref::PeriodAccounting accounting;
    int satisfaction{};
    std::vector<ref::PopularityRequest> popularity_requests;
    std::vector<ref::LegacyActorControl> presentation_requests; // No fake render coordinates.
    ref::WorldPosition position;
    std::optional<ref::FacilityDeparture> journey;
    std::optional<ref::ArrivalBinding> active_facility;
    std::size_t waypoint{};
    int current_weapon{};
    int weapon_reselect_counter{};
    std::optional<int> selected_weapon;
    std::uint64_t rounds{};
    std::uint64_t next_cash_id{1};
    int arrivals{};
    int departures{};
    int completions{};
    int occupations{};
    int recoveries{};
    int attribute_commits{};
    int equipment_commits{};
};
enum class StartupAiError {
    none,
    invalid_start,
    invalid_input,
    unsupported_branch,
    preparation_failed
};
// Requires an already-installed actual first visitor; accepts either source birth point.
// Each round clones all owners; any failed command discards the whole round including accounting.
// Only the closed initial category1/2 facility interval is admitted. Unknown world-event branches
// reject explicitly, rather than silently running a demonstration policy in the default game.
class StartupAiSession {
  public:
    StartupAiSession(const StartupSession &startup, ref::Position birth);
    const StartupAiState &state() const;
    StartupAiError round(const StartupAiTickets &tickets);

  private:
    StartupAiError decision(StartupAiState &next, const StartupAiTickets &tickets) const;
    StartupAiError execution(StartupAiState &next, const StartupAiTickets &tickets) const;
    StartupAiError depart(StartupAiState &next, const StartupAiTickets &tickets) const;
    StartupAiError arrive(StartupAiState &next) const;
    StartupAiError exit(StartupAiState &next, const StartupAiTickets &tickets) const;
    ref::LegacyMap map_;
    LoadedStartupMap loaded_;
    std::map<std::uint64_t, ref::FacilityNeighbourhood> neighbourhoods_;
    StartupAiState state_;
};
} // namespace dungeon_village_prototype
