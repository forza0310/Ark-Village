#pragma once

#include "world_steam_pot_operations.hpp"
#include <string>

namespace ark::test {
// A process-local player joining the original pot47 and facility75-77 consumers.
// Only a completed return to the main scene permits a player checkpoint.
class SteamPotItemPlayer {
  public:
    void reconcile(const app::WorldState &state, int recipe);
    IncomeDecision next(const app::WorldState &state, std::int64_t cash_reserve);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after,
                 const CampaignReceiptSink &receipts = {});
    void observe_tick(const app::WorldState &before, const app::WorldState &after,
                      const CampaignReceiptSink &receipts = {});
    bool complete(const app::WorldState &state) const;
    bool active() const { return !retired_; }
    std::string diagnose() const;

  private:
    ActiveIncomeStrategy use_{0, true}; // Explicit targets do not use the forecast horizon.
    int recipe_{-1}, item_{-1}, purchases_{}, deposits_{}, experience_{-1};
    bool crafted_{}, armed_{}, retired_{}, leaving_market_{};
    std::uint64_t next_market_step_{};
    std::array<int, 4> paid_{};
};
void steam_magic_item_contract();
} // namespace ark::test
