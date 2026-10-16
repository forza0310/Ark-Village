#pragma once

#include "world_active_income_strategy.hpp"
#include "world_active_trade_evidence.hpp"
#include "world_steam_people.hpp"

namespace ark::test {
// Cold-prefix diagnostics separate actual recipe/resources from this player's
// conservative equipment policy. No acquisition, crafting, or reachability claim.
void inspect_steam_magic_pot(const app::WorldState &state, std::ostream &out);

// A temporary player of the existing pot/commerce pages. The normal Steam player
// owns equipment allocation; this module only observes its real resulting gift.
class SteamPotEquipmentPlayer {
  public:
    void reconcile(const app::WorldState &state);
    // Legacy prefixes wait for trade after gifting; active business retains just the worn slot.
    IncomeDecision next(const app::WorldState &state, std::int64_t budget, bool wait_for_trade,
                        const ActiveTradeEvidence &trade);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after,
                 const CampaignReceiptSink &receipts = {});
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    bool complete(const app::WorldState &state, const ActiveTradeEvidence &trade) const;
    void verify(const app::WorldState &state, const ActiveTradeEvidence &trade) const;
    // Archive the strict live endpoint before ordinary play may replace this gift.
    void certify(const app::WorldState &state, const ActiveTradeEvidence &trade);
    bool certified() const { return certified_; }
    // A new isolated segment retains the prior sidecar as its input. Replace the
    // controller only after certifying that input's gift and following business.
    bool begin_next(const app::WorldState &state, const ActiveTradeEvidence &trade);
    std::string diagnose() const;
    void encode(std::ostream &out) const;
    static SteamPotEquipmentPlayer decode(std::istream &in);
    int recipe() const { return recipe_; }
    std::optional<SteamEquipmentSlot> retained_slot() const {
        return equipped_ ? std::optional<SteamEquipmentSlot>{{human_, slot_}} : std::nullopt;
    }

  private:
    int recipe_{-1}, kind_{}, definition_{}, slot_{}, initial_month_{-1}, initial_experience_{};
    int purchases_{}, deposits_{}, crafted_{}, equipped_{}, human_{-1}, equipped_month_{-1};
    int craft_experience_{-1};
    std::array<int, 4> paid_{};
    std::uint64_t actor_{}, next_market_step_{};
    bool leaving_market_{}; // Only an initialized83/84 visit; never persisted mid-page.
    bool certified_{};      // Test-sidecar history; never a player-world field.
    void observe_equipment(const app::WorldState &before, const app::WorldState &after);
};
void steam_pot_equipment_contract();
} // namespace ark::test
