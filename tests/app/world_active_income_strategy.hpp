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
};
std::optional<IncomeInvestment>
choose_income_investment(const app::WorldState &state, std::int64_t available,
                         std::optional<std::uint64_t> new_shop = {});
void inspect_income(const app::WorldState &state, std::ostream &out);
struct IncomeDecision {
    bool handled{};
    std::optional<app::WorldCommand> command;
};
class ActiveIncomeStrategy {
  public:
    explicit ActiveIncomeStrategy(int stop_month) : stop_month_(stop_month) {}
    IncomeDecision next(const app::WorldState &state, std::int64_t reserve, bool start);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldState &after);
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    bool active() const { return choice_.has_value(); }
    int uses() const { return uses_; }
    std::int64_t spent() const { return spent_; }
    void register_new_shop(std::uint64_t id) { new_shops_.insert(id); }

  private:
    std::optional<IncomeInvestment> choice_;
    bool purchased_{}, consumed_{}, improved_{};
    int uses_{};
    int stop_month_{};
    std::int64_t spent_{};
    std::set<std::uint64_t> new_shops_;
};
void income_strategy_contract();
} // namespace ark::test
