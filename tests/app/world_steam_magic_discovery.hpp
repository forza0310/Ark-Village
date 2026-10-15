#pragma once

#include "world_steam_pot_operations.hpp"
#include <string>

namespace ark::test {
// A process-local player; pending pieces and recipe progress remain in the real
// Owner/player save. Each cold segment reconstructs its next source objective.
class SteamPotDiscoveryPlayer {
  public:
    void reconcile(const app::WorldState &state);
    IncomeDecision next(const app::WorldState &state, std::int64_t cash_budget);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after,
                 const CampaignReceiptSink &receipts = {});
    bool active() const { return !retired_; }
    std::string diagnose(const app::WorldState &state) const;

  private:
    int recipe_{-1}, purchases_{}, deposits_{}, processed_{}, last_open_week_{-1};
    std::uint64_t next_market_step_{};
    bool leaving_market_{}, retired_{};
};
void steam_magic_discovery_contract();
} // namespace ark::test
