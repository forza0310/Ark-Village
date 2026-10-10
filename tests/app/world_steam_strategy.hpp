#pragma once
#include "world_active_trade_evidence.hpp"
#include "world_steam_layout.hpp"
#include "world_steam_people.hpp"

namespace ark::test {
struct SteamVillageStats {
    ActiveTradeEvidence trade;
    std::uint64_t ticks{}, commands{};
    int builds{}, moves{}, removals{}, roads{}, admissions{}, victories{}, activities{};
    int initial_month{-1}, completed_month{-1};
    std::int64_t minimum_cash{}, construction_cost{}, village_points_spent{};
};
// A legal player implementing the user's Steam layout/loot allocation policy. The
// blueprint is a goal, never a replacement Owner, source rule or restored Steam save.
class SteamVillageStrategy {
  public:
    void reconcile(const app::WorldState &state);
    std::optional<app::WorldCommand> next(const app::WorldState &state);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after);
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    const SteamVillageStats &stats() const { return stats_; }
    bool checkpoint(const app::WorldState &state) const;
    bool complete(const app::WorldState &state) const;
    void continue_campaign() { checkpoint_phase_ = false; }
    std::string diagnose(const app::WorldState &state) const;
    void encode(std::ostream &out) const;
    static SteamVillageStrategy decode(std::istream &in);

  private:
    enum class Operation { none, build, move, remove, road };
    Operation operation_{Operation::none};
    SteamLayoutTarget target_{};
    std::uint64_t moving_{}, recruitment_{}, departed_task_{};
    int unlock_{-1}, resident_{-1}, next_task_month_{};
    bool operation_committed_{}, checkpoint_phase_{true};
    std::uint64_t next_management_{}, next_activity_{}, next_layout_{};
    SteamVillageStats stats_;
    SteamPeopleStrategy people_;
    ActiveIncomeStrategy income_{3600, true};
    std::set<std::uint64_t> won_tasks_;
    std::set<std::size_t> traded_slots_;
    std::optional<app::WorldCommand> layout(const app::WorldState &state);
    void observe_world(const app::WorldState &before, const app::WorldState &after);
};
void steam_strategy_contract();
} // namespace ark::test
