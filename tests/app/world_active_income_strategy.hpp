#pragma once

#include "ark/app/session/world_session.hpp"
#include <ostream>
#include <set>

namespace ark::test {
// Optional player heuristic, never a rule or world mutation. Prices are forecast
// through the delivered pure rule; payment, consumption and effects use commands.
struct IncomeInvestment {
    std::uint64_t facility{};
    int definition{}, item{};
    std::int64_t cost{}, price_delta{}, monthly_gain{};
    bool purchase{};
    bool introduction{};
    bool amenities{}; // Free quality/charm improvement; no inferred cash return.
    std::array<std::int64_t, 3> visible_deltas{};
};
std::optional<IncomeInvestment> choose_income_investment(const app::WorldState &state,
                                                         std::int64_t available,
                                                         std::optional<std::uint64_t> new_shop = {},
                                                         bool inventory_only = false);
void inspect_income(const app::WorldState &state, std::ostream &out);
struct IncomeDecision {
    bool handled{};
    std::optional<app::WorldCommand> command;
};
class ActiveIncomeStrategy {
  public:
    explicit ActiveIncomeStrategy(int stop_month, bool reward_only = false)
        : stop_month_(stop_month), reward_only_(reward_only) {}
    IncomeDecision next(const app::WorldState &state, std::int64_t reserve, bool start);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldState &after);
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    bool active() const { return choice_.has_value(); }
    int uses() const { return uses_; }
    std::int64_t spent() const { return spent_; }
    bool reward_only() const { return reward_only_; }
    void register_new_shop(std::uint64_t id) { new_shops_.insert(id); }
    // Only stable controller boundaries are durable: no partially selected,
    // purchased or consumed gift may silently disappear on reload.
    void encode(std::ostream &out) const;
    static ActiveIncomeStrategy decode(std::istream &in);

  private:
    std::optional<IncomeInvestment> choice_;
    bool purchased_{}, consumed_{}, improved_{};
    int uses_{};
    int stop_month_{};
    bool reward_only_{};
    std::int64_t spent_{};
    std::set<std::uint64_t> new_shops_;
};
void income_strategy_contract();
} // namespace ark::test
