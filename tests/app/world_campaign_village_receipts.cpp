// Real village transaction observations; catalogue registration belongs to coverage.
#include "ark/simulation/village/rules/world_village_activity.hpp"
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

std::optional<CampaignReceipt> activity_popularity_queue_receipt(const State &before,
                                                                 const State &after) {
    const auto page = std::find_if(before.scripts.pages.rbegin(), before.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    if (page == before.scripts.pages.rend() || page->kind != ref::WorldScriptPageKind::raw_page ||
        page->legacy_page != 53)
        return {};
    const auto retired = std::find_if(after.scripts.pages.begin(), after.scripts.pages.end(),
                                      [&](const auto &p) { return p.id == page->id; });
    if (retired != after.scripts.pages.end() && retired->lifecycle != 4)
        return {};
    const auto need = [](bool value) {
        if (!value) throw std::runtime_error("Activity popularity lost its source queue/quarter binding");
    };
    need(before.rules && before.rules == after.rules &&
         before.activity_pages_initialized.count(page->id) && before.page_counters.at(page->id) >= 120);
    const int definition = before.activity_page_bindings.at(page->id);
    const auto &activity = before.rules->activities.at(definition);
    if (activity.parameters[2] != 2)
        return {};
    const auto &old = before.scene.world.popularity_queue, &now = after.scene.world.popularity_queue;
    need(now.size() == old.size() + 1 &&
         now.front() == std::array<int, 3>{10, activity.parameters[5], 1} &&
         std::equal(old.begin(), old.end(), now.begin() + 1) &&
         after.quarter_counter == before.quarter_counter - 1 &&
         after.activity_counts.at(definition) == before.activity_counts.at(definition) &&
         after.village_points == before.village_points && after.popularity == before.popularity &&
         after.maximum_popularity == before.maximum_popularity &&
         after.scene.world.world.ai.accounting.funds() == before.scene.world.world.ai.accounting.funds() &&
         after.scene.random.draws() == before.scene.random.draws());
    return CampaignReceipt{CampaignReceiptKind::activity_popularity_queued, definition,
                            after.simulation_steps, page->id, 0, -1,
                            {before.quarter_counter, before.popularity, static_cast<std::int64_t>(old.size()), activity.parameters[5]},
                            {after.quarter_counter, after.popularity, static_cast<std::int64_t>(now.size()), activity.parameters[5]}};
}

std::vector<CampaignReceipt> activity_human_effect_receipts(const State &before,
                                                            const State &after) {
    const auto page = std::find_if(before.scripts.pages.rbegin(), before.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    if (page == before.scripts.pages.rend() || page->kind != ref::WorldScriptPageKind::raw_page ||
        page->legacy_page != 53)
        return {};
    const auto retired = std::find_if(after.scripts.pages.begin(), after.scripts.pages.end(),
                                      [&](const auto &p) { return p.id == page->id; });
    if (retired != after.scripts.pages.end() && retired->lifecycle != 4)
        return {}; // An early confirmation or unchanged observation applies no effect.
    const auto need = [](bool condition) {
        if (!condition)
            throw std::runtime_error("Activity effect lost its completion/human binding");
    };
    need(before.rules && before.rules == after.rules &&
         before.activity_pages_initialized.count(page->id) &&
         before.page_counters.at(page->id) >= 120);
    const int id = before.activity_page_bindings.at(page->id);
    const auto &source = before.rules->activities.at(id);
    const int kind = source.parameters[2];
    if (kind != 0 && kind != 1)
        return {}; // Other effects have their own receipt consumers.
    const ref::WorldVillageActivityDefinition activity{id,
                                                       before.scripts.activities.at(id).status,
                                                       before.activity_flags.at(id),
                                                       before.activity_counts.at(id),
                                                       kind,
                                                       source.parameters[3],
                                                       source.parameters[4],
                                                       source.parameters[5]};
    std::vector<const ref::WorldScriptPage *> displays;
    for (const auto &p : after.scripts.pages)
        if (p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == 54 &&
            p.lifecycle != 4 &&
            std::none_of(before.scripts.pages.begin(), before.scripts.pages.end(),
                         [&](const auto &old) { return old.id == p.id; }))
            displays.push_back(&p);
    need(displays.size() == 1 && before.quarter_counter != std::numeric_limits<int>::min() &&
         after.activity_page_bindings.at(displays.front()->id) == id &&
         after.quarter_counter == before.quarter_counter - 1 &&
         before.village_points == after.village_points && before.events_held == after.events_held &&
         before.activity_counts == after.activity_counts &&
         before.activity_flags == after.activity_flags &&
         before.human_presence == after.human_presence &&
         before.scene.world.world.ai.accounting.funds() ==
             after.scene.world.world.ai.accounting.funds() &&
         after.scene.random.draws() == before.scene.random.draws() + 2);
    const auto same_hp = [](const auto &a, const auto &b) {
        return std::tie(a.requested_delta, a.displayed, a.origin, a.target, a.animating,
                        a.legacy_tick) == std::tie(b.requested_delta, b.displayed, b.origin,
                                                   b.target, b.animating, b.legacy_tick);
    };
    const auto &old_ai = before.scene.world.world.ai, &new_ai = after.scene.world.world.ai;
    std::vector<CampaignReceipt> result;
    for (const auto &human : before.rules->humans) {
        if (!before.human_presence.at(human.identity))
            continue;
        const int h = human.identity;
        const auto &old = old_ai.growth.at(h), &now = new_ai.growth.at(h);
        const int old_satisfaction = before.shop_humans.at(h).satisfaction;
        // The frozen rule suite owns the float/integer arithmetic. Here its tested
        // value oracle checks the Owner connection, roster and actual commit timing.
        const auto expected = ref::prepare_world_village_human_effect(
            activity, old.definition, old_ai.professions, old.derived, old_satisfaction);
        need(expected.candidate.has_value());
        const auto &effect = *expected.candidate;
        const auto &stats = effect.stats ? *effect.stats : old.derived;
        need(now.definition.extra == effect.definition.extra &&
             now.definition.current_profession == old.definition.current_profession &&
             now.definition.base == old.definition.base &&
             now.definition.legacy_u == old.definition.legacy_u &&
             now.definition.equipment == old.definition.equipment &&
             now.definition.learned_spells == old.definition.learned_spells &&
             now.definition.spell_professions == old.definition.spell_professions &&
             now.definition.profession_levels == old.definition.profession_levels &&
             after.shop_humans.at(h).satisfaction == effect.satisfaction &&
             after.human_activity_previous.at(h) == effect.previous_value &&
             now.derived.attributes == stats.attributes && now.derived.combat == stats.combat &&
             now.derived.growth == stats.growth &&
             now.derived.available_spells == stats.available_spells);
        const auto check_actors = [&](const auto &a, const auto &b) {
            need(a.size() == b.size());
            for (const auto &[actor_id, actor] : a)
                if (actor.kind == ref::ActorKind::human && actor.definition == h) {
                    const auto &changed = b.at(actor_id);
                    need(actor.kind == changed.kind && actor.definition == changed.definition &&
                         same_hp(actor.hp, changed.hp) &&
                         changed.capacity == (effect.stats ? stats.combat[0] : actor.capacity));
                }
        };
        check_actors(old_ai.battle.actors, new_ai.battle.actors);
        check_actors(old_ai.retired_actors, new_ai.retired_actors);
        const auto &focus = before.focus_actor.actor;
        if (focus.kind == ref::ActorKind::human && focus.definition == h)
            need(same_hp(focus.hp, after.focus_actor.actor.hp) &&
                 after.focus_actor.actor.capacity ==
                     (effect.stats ? stats.combat[0] : focus.capacity));
        const int attribute = kind == 0 ? -1 : activity.attribute;
        result.push_back({kind == 0 ? CampaignReceiptKind::activity_satisfaction_applied
                                    : CampaignReceiptKind::activity_attribute_applied,
                          id,
                          after.simulation_steps,
                          page->id,
                          0,
                          h,
                          {kind == 0 ? old_satisfaction : old.definition.extra.at(attribute),
                           kind == 0 ? old_satisfaction : old.derived.attributes.at(attribute),
                           attribute, old.definition.current_profession},
                          {kind == 0 ? effect.satisfaction : now.definition.extra.at(attribute),
                           kind == 0 ? effect.satisfaction : now.derived.attributes.at(attribute),
                           attribute, now.definition.current_profession}});
    }
    need(!result.empty());
    const auto &portraits = after.activity_page_display_humans.at(displays.front()->id);
    for (const int h : portraits)
        need(before.human_presence.at(h) != 0);
    if (kind == 0) {
        need(after.scene.world.popularity_queue.size() ==
                 before.scene.world.popularity_queue.size() + 1 &&
             after.scene.world.popularity_queue.front() ==
                 std::array<int, 3>{10, activity.magnitude, 1} &&
             std::equal(before.scene.world.popularity_queue.begin(),
                        before.scene.world.popularity_queue.end(),
                        after.scene.world.popularity_queue.begin() + 1));
    } else
        need(before.scene.world.popularity_queue == after.scene.world.popularity_queue);
    return result; // Publish only after every open definition and post-effect display validates.
}

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
