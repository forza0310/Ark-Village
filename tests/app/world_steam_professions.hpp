#pragma once

#include "world_active_income_strategy.hpp"
#include "world_campaign_coverage.hpp"
#include <iosfwd>
#include <utility>

namespace ark::test {
// Read-only cold-prefix qualifications. Live-actor and reserved-budget filters
// belong to this test player, not to the source profession catalogue.
void inspect_steam_professions(const app::WorldState &state, int reserved_points,
                               std::int64_t cash_reserve, std::ostream &out);

// One disposable player choice, never another world or a persisted training history.
// The real pages own selection, payment, rewards and the 55/197 animation clocks.
class SteamProfessionStrategy {
  public:
    // Explicit coverage uses the same source pages and budget. This transient
    // request also retains the chosen person's profession after the flow retires.
    void set_target(const app::WorldState &state, int human, int job);
    bool target_pending(const app::WorldState &state) const;
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
    std::optional<std::pair<int, int>> target_; // human/job; supplied again after a cold restart.
};
} // namespace ark::test
