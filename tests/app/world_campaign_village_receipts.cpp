// Real village transaction observations; catalogue registration belongs to coverage.
#include "world_campaign_coverage.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace ark::test {
namespace {
using State = app::WorldState;
namespace ref = simulation::rules;
} // namespace

std::optional<CampaignReceipt> rank_promotion_receipt(const State &before, const State &after) {
    if (before.rank == after.rank)
        return {};
    const auto need = [](bool value) {
        if (!value)
            throw std::runtime_error("Rank promotion lost its conditions/page/commit binding");
    };
    need(before.rules && before.rules == after.rules && before.rank >= 0 && before.rank < 5 &&
         after.rank == before.rank + 1);
    const auto page = std::find_if(before.scripts.pages.rbegin(), before.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    need(page != before.scripts.pages.rend() && page->id &&
         page->kind == ref::WorldScriptPageKind::raw_page && page->legacy_page == 48 &&
         before.page_counters.count(page->id));
    const auto terms = ref::fixed_calendar_task_rank_terms().at(before.rank);
    need(terms.size() == 4);
    for (std::size_t n = 0; n < terms.size(); ++n) {
        // The original checks cached flags; type3 is a presence flag with value0.
        need(before.rank_met[n] &&
             (terms[n].type == 3 || before.rank_values[n] >= terms[n].threshold));
    }
    const auto closed = std::find_if(after.scripts.pages.begin(), after.scripts.pages.end(),
                                     [&](const auto &p) { return p.id == page->id; });
    need(closed == after.scripts.pages.end() || closed->lifecycle == 4);
    const auto &a = before.scene.calendar, &b = after.scene.calendar;
    need(a.year == b.year && a.month == b.month && a.subperiod == b.subperiod &&
         a.units == b.units &&
         after.rank_history.at(after.rank - 1) == std::array<int, 2>{b.year, b.month} &&
         (after.scripts.user_flags & 8U));
    int celebrations{};
    for (const auto &p : after.scripts.pages)
        if (p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == 50 &&
            p.lifecycle != 4 &&
            std::none_of(before.scripts.pages.begin(), before.scripts.pages.end(),
                         [&](const auto &old) { return old.id == p.id; }))
            ++celebrations;
    need(celebrations == 1);
    for (const auto &activity : before.rules->activities)
        if (activity.parameters[6] == after.rank) {
            const auto &old = before.scripts.activities.at(activity.identity);
            const auto &opened = after.scripts.activities.at(activity.identity);
            need(opened.status == 1 && (old.status != 0 || opened.pending_notice));
        }
    return CampaignReceipt{CampaignReceiptKind::rank_conditions_promoted,
                           after.rank,
                           after.simulation_steps,
                           page->id,
                           0,
                           -1,
                           {before.rank, 15, before.maximum_income, before.popularity},
                           {after.rank, 15, after.maximum_income, after.popularity}};
}

std::vector<CampaignReceipt> residence_tax_receipts(const State &before, const State &after) {
    const auto page = std::find_if(before.scripts.pages.rbegin(), before.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    if (page == before.scripts.pages.rend() || page->kind != ref::WorldScriptPageKind::raw_page ||
        page->legacy_page != 98)
        return {};
    const auto closed = std::find_if(after.scripts.pages.begin(), after.scripts.pages.end(),
                                     [&](const auto &p) { return p.id == page->id; });
    if (closed != after.scripts.pages.end() && closed->lifecycle != 4)
        return {}; // Paused, not initialized, or not yet consumed: no money receipt.
    const auto need = [](bool value) {
        if (!value)
            throw std::runtime_error("Residence tax lost its raw98 income/reset binding");
    };
    const auto &old_date = before.scene.calendar, &new_date = after.scene.calendar;
    need(before.rules && after.rules && old_date.month >= 0 && old_date.month < 12 &&
         std::tie(old_date.year, old_date.month, old_date.subperiod, old_date.units,
                  old_date.previous_units, old_date.month_ticks) ==
             std::tie(new_date.year, new_date.month, new_date.subperiod, new_date.units,
                      new_date.previous_units, new_date.month_ticks));
    const int month = old_date.month;
    const auto old_cash = before.scene.world.world.ai.accounting.funds();
    const auto new_cash = after.scene.world.world.ai.accounting.funds();
    std::int64_t total{};
    std::vector<int> residents;
    for (const auto &human : before.rules->humans) {
        const int id = human.identity;
        need(after.human_presence.at(id) == before.human_presence.at(id) &&
             after.human_homes.at(id) == before.human_homes.at(id));
        if (before.human_presence.at(id) != 0 && before.human_homes.at(id)[2] == 1) {
            total += before.human_calendar.at(id).legacy_G;
            residents.push_back(id);
        }
        // The source resets all definitions, including absent/nonresident humans.
        need(after.human_calendar.at(id).legacy_F == 0 &&
             after.human_calendar.at(id).legacy_G == 0 &&
             after.scene.world.world.ai.battle.humans.at(id).battle_reward_stat == 0);
    }
    const auto old_random = before.scene.random.snapshot(),
               new_random = after.scene.random.snapshot();
    need(!residents.empty() &&
         !(total > 0 && old_cash > std::numeric_limits<std::int64_t>::max() - total) &&
         !(total < 0 && old_cash < std::numeric_limits<std::int64_t>::min() - total) &&
         new_cash == old_cash + total &&
         std::int64_t(after.monthly_cash[month][4][0]) - before.monthly_cash[month][4][0] ==
             total &&
         std::tie(old_random.engine_state, old_random.tape, old_random.cursor,
                  old_random.tape_mode) == std::tie(new_random.engine_state, new_random.tape,
                                                    new_random.cursor, new_random.tape_mode) &&
         before.sound_requests == after.sound_requests &&
         before.village_points == after.village_points && before.popularity == after.popularity);
    for (int m = 0; m < 12; ++m)
        for (int category = 0; category < 5; ++category)
            for (int side = 0; side < 2; ++side)
                if (m != month || category != 4 || side != 0)
                    need(before.monthly_cash[m][category][side] ==
                         after.monthly_cash[m][category][side]);
    need(after.scripts.notices.size() == before.scripts.notices.size() + 1 &&
         after.scripts.notices.back().message == 28);
    std::vector<CampaignReceipt> receipts;
    for (const int id : residents)
        receipts.push_back({CampaignReceiptKind::human_home_tax_collected,
                            id,
                            after.simulation_steps,
                            page->id,
                            0,
                            id,
                            {old_cash, before.human_calendar.at(id).legacy_G,
                             before.monthly_cash[month][4][0], month},
                            {new_cash, 0, after.monthly_cash[month][4][0], month}});
    return receipts;
}

} // namespace ark::test
