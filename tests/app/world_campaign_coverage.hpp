#pragma once
#include "ark/app/session/world_session.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace ark::test {
// Emitted only after the strategy's independent business assertions have passed.
// Values are actual before/after observations; queue requests are never paid effects.
enum class CampaignReceiptKind {
    activity_points_paid,
    activity_effect_queued,
    item_consumed,
    item_attribute_applied,
    item_obtained,
    recipe_discovered,
    recipe_crafted,
    item_hp_restored,
    facility_blueprint_paid,
    facility_blueprint_claimed,
    facility_construction_paid,
    activity_pot_opened,
    weapon_reward_equipped,
    armour_reward_equipped,
    accessory_reward_equipped,
    weapon_combat_applied,
    armour_combat_applied,
    accessory_combat_applied,
    human_medal_awarded,
    human_home_admitted,
    task_victory_committed,
    item_commerce_bought,
    item_facility_consumed,
    facility_item_improved,
    item_task_loot_claimed,
    weapon_task_loot_claimed,
    armour_task_loot_claimed,
    accessory_task_loot_claimed
};
struct CampaignReceipt {
    CampaignReceiptKind kind{};
    int definition{};
    std::uint64_t step{}, page{}, subject{};
    int recipient{-1};
    std::array<std::int64_t, 4> before{}, after{};
};
using CampaignReceiptSink = std::function<void(const CampaignReceipt &)>;

// Capture fields only after a strategy has verified its real source transaction.
// This shares the wire mapping, not the payment/unlock oracle or player decisions.
CampaignReceipt facility_transaction_receipt(CampaignReceiptKind kind, int definition,
                                             const app::WorldState &before,
                                             const app::WorldState &after, std::uint64_t page,
                                             std::uint64_t instance = 0);
// Field mapping only; the caller verifies admission, its fee and the actual residence instance.
CampaignReceipt residence_transaction_receipt(int human, const app::WorldState &before,
                                              const app::WorldState &after, std::uint64_t page,
                                              std::uint64_t facility);
// Bind an already-observed victory to the submitted task and its new raw30 page.
// This checks the receipt's source identity, not loot, combat rewards or successor generation.
std::optional<CampaignReceipt> task_victory_receipt(const app::WorldState &before,
                                                    const app::WorldState &after,
                                                    std::uint64_t departed_task);
// Direct exploration grants only: source challenge0->2 plus matching catalog effects.
// A same-frame competing origin remains unverified instead of receiving invented credit.
struct CampaignLootObservation {
    std::vector<CampaignReceipt> receipts;
    const char *gap{};
};
CampaignLootObservation task_loot_receipts(const app::WorldState &before,
                                           const app::WorldState &after);

// Per-process evidence inventory. It deliberately does not reconstruct a command history
// from a loaded save, or turn an inventory decrease into a verified item effect.
class CampaignCoverage {
  public:
    void observe(const app::WorldState &before, const app::WorldState &after);
    void record(const CampaignReceipt &receipt, std::ostream &log);
    void write(std::ostream &out, const app::WorldState &state) const;
    void save(const std::filesystem::path &directory, const app::WorldState &state) const;

  private:
    using Key = std::pair<std::string, int>;
    void mark(const char *kind, int id, const char *event);
    std::map<Key, std::set<std::string>> observations_;
    std::map<Key, std::set<std::string>>
        receipts_; // Bounded stages per definition, no history queue.
    std::string segment_;
    std::uint64_t first_step_{}, last_step_{};
};
void campaign_coverage_contract();
} // namespace ark::test
