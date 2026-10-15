#pragma once

#include "world_active_trade_evidence.hpp"
#include "world_steam_people.hpp"

namespace ark::test {
// A temporary player of the existing pot/commerce pages. The normal Steam player
// owns equipment allocation; this module only observes its real resulting gift.
class SteamPotEquipmentPlayer {
  public:
    void reconcile(const app::WorldState &state);
    IncomeDecision next(const app::WorldState &state, std::int64_t budget, bool controller_ready,
                        const ActiveTradeEvidence &trade);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after,
                 const CampaignReceiptSink &receipts = {});
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    bool complete(const app::WorldState &state, const ActiveTradeEvidence &trade) const;
    void verify(const app::WorldState &state, const ActiveTradeEvidence &trade) const;
    // A new isolated segment retains the prior sidecar as its input. Replace the
    // controller only after certifying that input's gift and following business.
    bool begin_next(const app::WorldState &state, const ActiveTradeEvidence &trade);
    std::string diagnose() const;
    void encode(std::ostream &out) const;
    static SteamPotEquipmentPlayer decode(std::istream &in);
    int recipe() const { return recipe_; }

  private:
    int recipe_{-1}, kind_{}, definition_{}, slot_{}, initial_month_{-1}, initial_experience_{};
    int purchases_{}, deposits_{}, crafted_{}, equipped_{}, human_{-1}, equipped_month_{-1};
    int craft_experience_{-1};
    std::array<int, 4> paid_{};
    std::uint64_t actor_{}, next_market_step_{};
    bool leaving_market_{}; // Only an initialized83/84 visit; never persisted mid-page.
    void observe_equipment(const app::WorldState &before, const app::WorldState &after);
};
void steam_pot_equipment_contract();
} // namespace ark::test
