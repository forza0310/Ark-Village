#pragma once

#include "ark/app/session/world_session.hpp"
#include "world_active_trade_evidence.hpp"
#include <iosfwd>

namespace ark::test {
struct ActivePotVillageStats {
    ActiveTradeEvidence trade;
    std::uint64_t commands{}, ticks{}, healed_actor{};
    std::int64_t minimum_cash{}, facility_income{}, purchase_cost{}, use_income{};
    int purchases{}, deposits{}, processed{}, discoveries{}, crafted{}, used{}, healed{};
    int recipe{7}, item{29}, recipient{-1}, hp_before{}, hp_after{};
    int craft_month{-1}, use_month{-1};
};

// A player route from a verified second-star save. Only decisions and receipts live
// here; the production Owner and commands retain all money, stock, pot and HP state.
class ActivePotVillageStrategy {
  public:
    std::optional<app::WorldCommand> next(const app::WorldState &state);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after);
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    void reconcile(const app::WorldState &state);
    bool checkpoint(const app::WorldState &state) const;
    bool complete(const app::WorldState &state) const;
    std::string diagnose(const app::WorldState &state) const;
    const ActivePotVillageStats &stats() const { return stats_; }
    void encode(std::ostream &stream) const;
    static ActivePotVillageStrategy decode(std::istream &stream);

  private:
    void observe_world(const app::WorldState &before, const app::WorldState &after);
    ActivePotVillageStats stats_;
    std::uint64_t next_market_tick_{};
    // Only reconciling decoded checkpoint evidence enables crafting. Fresh stage A
    // must return to the scene and save after discovery, before spending elements.
    bool initialized_{}, leaving_market_{}, decoded_evidence_{}, craft_allowed_{};
};
} // namespace ark::test
