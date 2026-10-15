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
// For an extra plot, remove its retiring upkeep from the post-admission budget.
// The reference route keeps its original buffer because it rebuilds the fixed plot.
std::int64_t steam_residence_reserve(const app::WorldState &state,
                                     std::optional<std::uint64_t> replacing_plot = {});
// Full source plot cost plus transitional/house maintenance; admission fee is
// separately checked by the same eligible_homeless selector used at actual input.
std::int64_t steam_residence_plot_reserve(const app::WorldState &state);
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
// Source catalogue only. The player applies this coverage target after its
// reference layout and special progression goals, preserving other point reserves.
std::optional<SteamProgressPointGoal> steam_unheld_activity_goal(const app::WorldState &state);
std::optional<int> steam_required_facility(const app::WorldState &state);
struct SteamShopExpansion {
    int definition{}, points{};
    std::int64_t price{}, construction{}, maintenance{};
};
// Entries must come from the actual85 or21 catalogue. Quoted unit price is a
// player priority, never a guaranteed return or a replacement source catalogue.
std::optional<SteamShopExpansion> steam_select_shop_expansion(const app::WorldState &state,
                                                              const std::vector<int> &entries,
                                                              std::int64_t cash_budget);
// One shared reserve for pending layout blueprints, expansion, progression and the next shop.
// Passing raw51's frozen entries preserves its exact catalogue; other callers query the source.
int steam_point_reserve(const app::WorldState &state,
                        const std::vector<int> *offered_activities = nullptr);
// A completed task leaves the intervening calendar month available for normal business.
int steam_task_recovery_month(const app::WorldState &state);
void inspect_steam_expenses(const app::WorldState &before, const app::WorldState &after,
                            std::string_view phase, std::ostream &output);
void steam_budget_contract();
} // namespace ark::test
