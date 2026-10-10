#pragma once

#include "ark/app/session/world_session.hpp"
#include "world_active_trade_evidence.hpp"
#include <iosfwd>
#include <set>

namespace ark::test {
// Evidence retained across player-save restarts. These are observations, never world inputs.
struct ActiveVillageStats {
    ActiveTradeEvidence trade;
    std::uint64_t commands{}, ticks{}, bakery{}, departed_task{};
    std::int64_t construction_cost{}, minimum_cash{}, facility_income{}, bakery_income{};
    int activities{}, gifts{}, task_departures{}, task_successes{}, task_failures{};
    int promoted_month{-1}, exhibition_month{-1};
    std::int64_t exhibition_income{};
    bool bakery_completed{}, first_star_conditions{}, exhibition_paid{};
    std::set<std::uint64_t> successful_tasks;
    std::set<int> upgraded_definitions;
};

// Bounded P1 player: build bakery, give one real starting item, run affordable activities
// and adventures, apply for the first star, consume painting exhibition, then keep trading.
// The caller executes one command through the product consumer and one real update per round.
// This class owns only strategy/evidence; it never mutates or retains an Owner copy.
class ActiveVillageStrategy {
  public:
    std::optional<app::WorldCommand> next(const app::WorldState &state);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after);
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    // A cold player load may change random stream and clear transient page state. Rebind
    // identities from the loaded Owner, while retaining externally recorded business facts.
    void reconcile(const app::WorldState &state);
    const ActiveVillageStats &stats() const { return stats_; }
    bool construction_checkpoint(const app::WorldState &state) const;
    bool complete(const app::WorldState &state) const;
    std::string diagnose(const app::WorldState &state) const;
    void encode(std::ostream &stream) const;
    static ActiveVillageStrategy decode(std::istream &stream);

  private:
    void observe_world(const app::WorldState &before, const app::WorldState &after);
    ActiveVillageStats stats_;
    std::uint64_t next_activity_tick_{};
    std::uint64_t requested_task_{};
    int next_task_month_{}, recipient_{-1};
    bool initialized_{};
};
} // namespace ark::test
