#pragma once

#include "world_active_income_strategy.hpp"
#include "world_campaign_coverage.hpp"
#include "world_steam_professions.hpp"
#include <iosfwd>
#include <map>
#include <vector>

namespace ark::test {
// Prospective test-player eligibility only: reuse the gift policy for an equipment
// recipe's possible free grant. This neither grants stock nor certifies consumption.
bool steam_equipment_reward_usable(const app::WorldState &state, int kind, int definition);

// Player policy only. Commands consume real award pages, reward qualifications and quotes;
// this controller never changes character, inventory or random state itself.
class SteamPeopleStrategy {
  public:
    IncomeDecision next(const app::WorldState &state, int reserved_points,
                        std::int64_t cash_reserve, bool start);
    void observe(const app::WorldState &before, const app::WorldCommand &command,
                 const app::WorldCommandResult &result, const app::WorldState &after,
                 const CampaignReceiptSink &receipts = {});
    void observe_tick(const app::WorldState &before, const app::WorldState &after,
                      const CampaignReceiptSink &receipts = {});
    // The layout controller must still build/open a real recruitment plot and
    // submit its actual residence candidate. This query only selects a person.
    static std::optional<int> eligible_homeless(const app::WorldState &state,
                                                std::int64_t cash_reserve);
    // The caller saves only at a stable main-scene boundary. Mid-management saves fail
    // explicitly, rather than discarding a pending selection or fairness history.
    void encode(std::ostream &out) const;
    static SteamPeopleStrategy decode(std::istream &in);
    bool active() const { return gift_.has_value() || professions_.active(); }
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
        int charge{}, stock{}; // Source quote/qualification captured before the actual gift flow.
    };
    std::optional<Gift> gift_;
    bool consumed_{};
    int awarded_{};
    std::map<int, int> equipment_gifts_;
    SteamProfessionStrategy professions_;
    void observe_world(const app::WorldState &before, const app::WorldState &after,
                       std::vector<CampaignReceipt> &pending);
};
void steam_people_contract();
} // namespace ark::test
