// Read-only battle evidence. Corpse retirement, lethal-hit statistics and event XP have
// different consumers; this query certifies only definition growth and posted village cash.
#include "ark/simulation/combat/rules/combat_ai.hpp"
#include "world_campaign_coverage.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <vector>

namespace ark::test {
namespace {
namespace ref = simulation::rules;
using State = app::WorldState;
struct RetirementBatch {
    std::vector<ref::CharacterId> actors;
    std::int64_t cash{};
};
} // namespace

CampaignRewardObservation monster_retirement_receipts(const State &before, const State &after) {
    const auto &old = before.scene.world.world.ai, &now = after.scene.world.world.ai;
    const auto old_month =
        std::int64_t(before.scene.calendar.year) * 12 + before.scene.calendar.month;
    const auto new_month =
        std::int64_t(after.scene.calendar.year) * 12 + after.scene.calendar.month;
    if (new_month < old_month || new_month > old_month + 1)
        return {{}, "monster_calendar_binding"};
    const bool month_reset = new_month != old_month;
    std::map<int, RetirementBatch> batches;
    std::set<ref::CharacterId> visited;
    for (const auto id : old.monster_order) {
        if (!visited.insert(id).second || !old.battle.actors.count(id))
            return {{}, "monster_roster_binding"};
        const auto &actor = old.battle.actors.at(id);
        if (now.battle.actors.count(id) || actor.control.state != 3 || actor.state_parameter != 0 ||
            actor.state_counter < 12)
            continue;
        if (!(actor.id == id) || !id.value || actor.kind != ref::ActorKind::monster ||
            std::find(now.monster_order.begin(), now.monster_order.end(), id) !=
                now.monster_order.end() ||
            !old.monster_growth.count(actor.definition) ||
            !old.battle.monsters.count(actor.definition))
            return {{}, "corpse_retirement_binding"};
        batches[actor.definition].actors.push_back(id);
    }
    // The maintained runtime's only monster-category income producer is the C0
    // retirement consumer. Compare every new entry, including zero-valued rewards.
    std::multiset<std::int64_t> posted, expected;
    for (auto entry = now.accounting.entries().lower_bound(old.next_cash_id);
         entry != now.accounting.entries().end(); ++entry) {
        const auto &cash = entry->second;
        if (cash.category != ref::CashCategory::monsters)
            continue;
        if (old.accounting.entries().count(entry->first) || cash.event_id != entry->first ||
            cash.direction != ref::CashDirection::income || cash.amount < 0)
            return {{}, "monster_cash_entry_binding"};
        posted.insert(cash.amount);
    }
    for (const auto &[definition, growth] : old.monster_growth) {
        const auto updated = now.monster_growth.find(definition);
        const auto found = batches.find(definition);
        const auto count = found == batches.end() ? 0 : std::int64_t(found->second.actors.size());
        if (updated == now.monster_growth.end() ||
            std::int64_t(updated->second.defeats) !=
                (month_reset ? 0 : std::int64_t(growth.defeats) + count) ||
            std::int64_t(updated->second.growth) != std::int64_t(growth.growth) + count)
            return {{}, "monster_retirement_counter_binding"};
        if (!count)
            continue;
        if (growth.base_cash_reward != updated->second.base_cash_reward ||
            !now.battle.monsters.count(definition) ||
            old.battle.monsters.at(definition).flags != now.battle.monsters.at(definition).flags)
            return {{}, "monster_reward_definition_binding"};
        auto &batch = batches.at(definition);
        // XP uses the old growth; cash uses successive new growth values. Do not
        // attach the cash amount to a killer, or infer XP distribution from it.
        for (std::int64_t n = 1; n <= count; ++n) {
            const auto cash = ref::prepare_monster_growth(
                growth.base_cash_reward, int(std::int64_t(growth.growth) + n), 1,
                (old.battle.monsters.at(definition).flags & 4U) != 0);
            if (!cash || batch.cash > std::numeric_limits<std::int64_t>::max() - *cash)
                return {{}, "monster_reward_amount_binding"};
            expected.insert(*cash);
            batch.cash += *cash;
        }
    }
    if (posted != expected)
        return {{}, "monster_cash_not_isolated"};
    if (batches.empty())
        return {};
    std::uint64_t page{};
    for (auto p = before.scripts.pages.rbegin(); p != before.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4) {
            page = p->id;
            break;
        }
    if (!page || !before.rules || before.rules != after.rules)
        return {{}, "monster_frame_binding"};
    CampaignRewardObservation result;
    for (const auto &[definition, batch] : batches) {
        const auto &a = old.monster_growth.at(definition), &b = now.monster_growth.at(definition);
        const auto count = std::int64_t(batch.actors.size());
        // k.u is the monthly tally. Calendar reset runs after the world consumers;
        // its final zero is observed directly, never reconstructed as an increment.
        result.receipts.push_back({month_reset
                                       ? CampaignReceiptKind::monster_retirement_cash_month_reset
                                       : CampaignReceiptKind::monster_retirement_cash,
                                   definition,
                                   after.simulation_steps,
                                   page,
                                   batch.actors.front().value,
                                   -1,
                                   {a.defeats, a.growth, 0, count},
                                   {b.defeats, b.growth, batch.cash, count}});
    }
    return result;
}
} // namespace ark::test
