#pragma once

#include "world_active_income_strategy.hpp"
#include <iosfwd>
#include <map>

namespace ark::test {
// Player policy only. Commands consume real award pages and reward qualifications;
// this controller never changes character, inventory or random state itself.
class SteamPeopleStrategy {
  public:
    IncomeDecision next(const app::WorldState &state, int reserved_points,
                        std::int64_t cash_reserve, bool start);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after);
    void observe_tick(const app::WorldState &before, const app::WorldState &after);
    // The layout controller must still build/open a real recruitment plot and
    // submit its actual residence candidate. This query only selects a person.
    static std::optional<int> eligible_homeless(const app::WorldState &state,
                                                std::int64_t cash_reserve);
    // The caller saves only at a stable main-scene boundary. Mid-gift saves fail
    // explicitly, rather than discarding a pending selection or fairness history.
    void encode(std::ostream &out) const;
    static SteamPeopleStrategy decode(std::istream &in);
    bool active() const { return gift_.has_value(); }
    int awarded() const { return awarded_; }
    int medals() const { return awarded_; }
    int gifts() const {
        int count{};
        for (const auto &[person, received] : equipment_gifts_) {
            (void)person;
            count += received;
        }
        return count;
    }
    const std::map<int, int> &equipment_gifts() const { return equipment_gifts_; }

  private:
    struct Gift {
        int human{}, slot{}, definition{};
    };
    std::optional<Gift> gift_;
    bool consumed_{};
    int awarded_{};
    std::map<int, int> equipment_gifts_;
    void observe_world(const app::WorldState &before, const app::WorldState &after);
};
void steam_people_contract();
} // namespace ark::test
