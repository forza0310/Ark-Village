#pragma once

#include "ark/app/world_session.hpp"
#include "world_active_trade_evidence.hpp"
#include <iosfwd>
#include <set>

namespace ark::test {
struct ActiveLateVillageStats {
    ActiveTradeEvidence trade;
    std::uint64_t commands{}, ticks{}, departed_task{};
    std::int64_t minimum_cash{}, facility_income{}, new_shop_income{}, construction_cost{},
        gift_cost{};
    int initial_successes{}, task_successes{}, task_departures{}, activities{}, gifts{},
        admissions{};
    int promoted_month{-1}, pot_month{-1};
    std::int64_t pot_income{};
    bool second_star_conditions{}, pot_paid{};
    int target_rank{2}, school_activity_month{-1};
    std::uint64_t western{}, school{};
    std::int64_t western_initial_sales{}, school_sales{};
    bool third_star_conditions{}, school_activity_paid{};
    int western_unlock_points{};
    bool western_unlock_paid{}, western_unlock_claimed{};
    std::uint64_t layout_old_shop{}, layout_moved_shop{};
    int layout_road_cells{};
    std::int64_t layout_cost{};
    bool layout_complete{};
    std::set<std::uint64_t> buildings, successful_tasks;
    std::set<int> residents;
};

// Player decisions after an independently verified first-star save. This owns only
// observations and selections: all construction, gifts, admissions and rewards go through
// WorldCommand. The runner supplies the real update and bounded wall/step budgets.
class ActiveLateVillageStrategy {
  public:
    ActiveLateVillageStrategy() = default;
    explicit ActiveLateVillageStrategy(int target_rank);
    std::optional<app::WorldCommand> next(const app::WorldState &state);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after);
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    void reconcile(const app::WorldState &state);
    bool checkpoint(const app::WorldState &state) const;
    bool complete(const app::WorldState &state) const;
    std::string diagnose(const app::WorldState &state) const;
    std::string diagnose_construction(const app::WorldState &state) const;
    const ActiveLateVillageStats &stats() const { return stats_; }
    void encode(std::ostream &stream) const;
    static ActiveLateVillageStrategy decode(std::istream &stream);

  private:
    void observe_world(const app::WorldState &before, const app::WorldState &after);
    ActiveLateVillageStats stats_;
    std::uint64_t requested_task_{}, recruitment_{}, next_management_tick_{}, next_activity_tick_{};
    int next_task_month_{}, recipient_{-1}, build_definition_{-1};
    simulation::rules::Position build_anchor_{};
    bool initialized_{}, build_committed_{}, gifting_{};
};
} // namespace ark::test
