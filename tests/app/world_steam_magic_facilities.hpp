#pragma once

#include "world_campaign_receipts_test.hpp"
#include "world_steam_pot_operations.hpp"
#include <string>

namespace ark::test {
// Process-local player of47 ->107 ->93. The durable blueprint/claim count belongs
// to the source Owner; normal Steam construction resumes after the claim.
class SteamPotFacilityPlayer {
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
    int recipe_{-1}, definition_{-1}, purchases_{}, deposits_{};
    bool crafted_{}, claimed_{}, retired_{}, leaving_market_{};
    std::uint64_t next_market_step_{}, reward_page_{};
};
void steam_magic_facility_contract(const CampaignCommandConsumer &apply);
} // namespace ark::test
