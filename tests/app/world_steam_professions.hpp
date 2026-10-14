#pragma once

#include "world_active_income_strategy.hpp"
#include "world_campaign_coverage.hpp"

namespace ark::test {
// One disposable player choice, never another world or a persisted training history.
// The real pages own selection, payment, rewards and the 55/197 animation clocks.
class SteamProfessionStrategy {
  public:
    IncomeDecision next(const app::WorldState &state, int reserved_points,
                        std::int64_t cash_reserve, bool start);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after,
                 const CampaignReceiptSink &receipts = {});
    void observe_tick(const app::WorldState &before, const app::WorldState &after,
                      const CampaignReceiptSink &receipts = {});
    bool active() const { return flow_.has_value(); }

  private:
    struct Flow {
        int human{}, job{}, old_job{}, charge{};
        simulation::rules::CharacterId actor;
        std::uint64_t animation{};
        bool paid{}, changed{}, finished{}, cancelled{};
    };
    std::optional<Flow> flow_;
};
} // namespace ark::test
