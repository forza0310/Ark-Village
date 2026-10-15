#pragma once

#include "world_steam_pot_operations.hpp"
#include <string>

namespace ark::test {
// One process-local player of the existing pot and ordinary-item gift pages.
// Incomplete work cannot be saved; durable learning belongs to the real human.
class SteamPotSpellBookPlayer {
  public:
    void reconcile(const app::WorldState &state);
    IncomeDecision next(const app::WorldState &state, std::int64_t cash_budget);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after,
                 const CampaignReceiptSink &receipts = {});
    bool complete(const app::WorldState &state) const;
    bool active() const { return !retired_; }
    std::string diagnose() const;

  private:
    int recipe_{-1}, item_{-1}, spell_{-1}, human_{-1};
    int purchases_{}, deposits_{}, experience_{-1};
    bool crafted_{}, consumed_{}, retired_{}, leaving_market_{};
    std::uint64_t next_market_step_{}, actor_{};
    std::array<int, 4> paid_{};
};
void steam_magic_book_contract();
} // namespace ark::test
