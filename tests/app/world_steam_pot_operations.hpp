#pragma once

#include "world_active_income_strategy.hpp"

namespace ark::test {
// Shared input/observation of the delivered pot and material pages. These values
// are test-player facts, never a second world or a replacement reward consumer.
struct SteamPotCommit {
    int purchases{}, deposits{};
    bool crafted{};
    int experience{-1};
    std::array<int, 4> paid{};
};
IncomeDecision next_steam_pot_command(
    const app::WorldState &state, const simulation::rules::WorldMagicPotRecipeDefinition &recipe,
    bool crafted, std::int64_t cash_budget, std::uint64_t &next_market_step, bool &leaving_market);
SteamPotCommit
observe_steam_pot_command(const app::WorldState &before, const app::WorldCommand &command,
                          const app::WorldCommandResult &result, const app::WorldState &after,
                          const simulation::rules::WorldMagicPotRecipeDefinition &recipe,
                          bool already_crafted, const CampaignReceiptSink &receipts = {});
} // namespace ark::test
