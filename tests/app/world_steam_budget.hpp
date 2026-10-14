#pragma once
#include "ark/app/session/world_session.hpp"
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <vector>

namespace ark::test {
// Test-player budgets read the real Owner and quotes; they never change source fees.
std::int64_t steam_operating_reserve(const app::WorldState &state);
std::int64_t steam_residence_reserve(const app::WorldState &state);
bool steam_task_affordable(const app::WorldState &state, std::uint64_t task);
bool steam_task_renewable(const app::WorldState &state, int displayed_fee);
// raw27 has already cached the source quote; rank eligible, affordable definitions
// by their current four combat values without recreating the hiring fee formula.
std::optional<int> steam_task_extra_human(const app::WorldState &state, std::uint64_t page);
bool steam_optional_activity_affordable(const app::WorldState &state);
enum class SteamProgressPurchase { activity, blueprint };
struct SteamProgressPointGoal {
    SteamProgressPurchase kind;
    int definition{}, points{};
};
// A player target from actual rank/availability/counts: save for the unconsumed
// rank4 event, then the next rank's required facility. This is no unlock rule.
std::optional<SteamProgressPointGoal> steam_progress_point_goal(const app::WorldState &state);
std::optional<int> steam_required_facility(const app::WorldState &state);
// One shared player reserve for pending layout blueprints, offered expansion and progression.
// Passing raw51's frozen entries preserves its exact catalogue; other callers query the source.
int steam_point_reserve(const app::WorldState &state,
                        const std::vector<int> *offered_activities = nullptr);
// A completed task leaves the intervening calendar month available for normal business.
int steam_task_recovery_month(const app::WorldState &state);
void inspect_steam_expenses(const app::WorldState &before, const app::WorldState &after,
                            std::string_view phase, std::ostream &output);
void steam_budget_contract();
} // namespace ark::test
