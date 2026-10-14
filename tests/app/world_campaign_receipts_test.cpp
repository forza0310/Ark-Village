// Bounded qualification fixtures certify source transactions, not natural unlocks.
// Process orchestration, files and long-route strategy history remain in world_campaign_test.
#include "world_campaign_receipts_test.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/map/startup_world_expansion.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include "ark/simulation/village/startup_world_tax.hpp"
#include "ark/simulation/actors/rules/human_management.hpp"
#include "world_active_late_strategy.hpp"
#include "world_steam_budget.hpp"
#include "world_steam_professions.hpp"
#include "world_steam_strategy.hpp"
#include <algorithm>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ark::test {
namespace {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace ref = sim::rules;
using State = app::WorldState;
using Kind = app::WorldCommandKind;
using LateStrategy = ActiveLateVillageStrategy;
void require(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
const ref::WorldScriptPage *top(const State &state) {
    for (auto page = state.scripts.pages.rbegin(); page != state.scripts.pages.rend(); ++page)
        if (page->lifecycle != 4)
            return &*page;
    return nullptr;
}
// Source-page transactions use isolated eligibility fixtures; they certify receipts,
// not natural star progression. Keep them separate from driver/sidecar contracts.
// A short source-consumer fixture, distinct from the saved natural business route.
// Reuse its candidate/update lifecycle for transaction and people receipt checks.
template <class Complete>
void run_steam_receipt_fixture(const CampaignCommandConsumer &apply, State &state,
                               ark::test::SteamVillageStrategy &strategy, Complete complete,
                               const ark::test::CampaignReceiptSink &receive) {
    for (int step = 0; step < 600 && !complete(); ++step) {
        const auto action = strategy.next(state);
        const auto *page = top(state);
        if (page && sim::startup_world_human_page_ready(state, page->id) &&
            (page->legacy_page == 66 || page->legacy_page == 68)) {
            // Formal 5708375: human consume_display waits at66/40..84;
            // world_award_page waits at68/67..72. Observe actual source updates only.
            const int counter = state.page_counters.at(page->id);
            const bool waits = page->legacy_page == 66 ? counter >= 40 && counter < 85
                                                      : counter >= 67 && counter < 73;
            require(waits ? !action
                          : action && action->kind == Kind::human_action &&
                                action->human_action == sim::StartupHumanPageAction::confirm &&
                                action->page == page->id,
                    "Gift input violates observed source timing: raw=" +
                        std::to_string(page->legacy_page) + " counter=" + std::to_string(counter));
        }
        if (action) {
            auto candidate = state;
            const auto result = apply(candidate, *action);
            strategy.observe(state, *action, result, candidate, receive);
            state = std::move(candidate);
        }
        if (complete())
            break;
        auto update = sim::prepare_startup_world_runtime(state);
        require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                "Steam receipt fixture update failed; " + strategy.diagnose(state));
        strategy.observe_tick(state, *update.candidate, receive);
        state = std::move(*update.candidate);
    }
    if (!complete()) {
        std::ostringstream detail;
        detail << strategy.diagnose(state) << " world_steps=" << state.simulation_steps;
        for (const auto &[id, facility] : state.scene.world.world.facilities)
            detail << " facility=" << id << ':' << facility.placement.definition_id << ':'
                   << facility.status << ':' << state.facility_flags.at(id);
        detail << " satisfaction1=" << state.shop_humans.at(1).satisfaction;
        throw std::runtime_error("Steam receipt fixture did not reach its bound effect; " +
                                 detail.str());
    }
}

// Prepare arrival eligibility only; the real arrival consumer allocates the actor,
// definition bindings and introductory pages. It is not a natural unlock claim.
State steam_people_fixture() {
    auto state = ark::test::initial_world();
    state.rank = 1;
    state.arrival_counter = 1;
    state.human_presence.at(1) = 1;
    state.human_calendar.at(1).absent_months = 100;
    const auto arrived = sim::startup_world_runtime_adapter().arrival(state);
    const auto visitor = arrived ? sim::startup_world_human_details(*arrived, 1) : std::nullopt;
    require(visitor && visitor->live_actor,
            "People receipt fixture lacks its source-created first visitor");
    state = *arrived;
    // Finish the real introductory pages before installing isolated qualifications;
    // their normal script writeback must not be confused with gift consumption.
    for (int step = 0; step < 40; ++step) {
        const auto *page = top(state);
        if (page && page->kind == ref::WorldScriptPageKind::scene && page->lifecycle == 2 &&
            state.scene.scene_state == 0)
            return state;
        if (page && page->kind == ref::WorldScriptPageKind::dialogue && page->lifecycle == 2)
            require(sim::acknowledge_startup_world_runtime_page(state, page->id) ==
                        sim::StartupWorldRuntimeError::none,
                    "People fixture introduction did not close through its source consumer");
        auto update = sim::prepare_startup_world_runtime(state);
        require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                "People fixture introduction initialization failed");
        state = std::move(*update.candidate);
    }
    throw std::runtime_error("People fixture has not reached its initialized main scene");
}

// Qualification fixture: factory identity and actor allocation are genuine; the
// arrived crew and reached reward threshold are supplied, then the real grant runs.
void task_loot_receipt_contract() {
    struct Case {
        int kind, definition, grants, held, status;
    };
    const Case cases[]{{0, 0, 1, 0, 0},  {0, 0, 2, 998, 1}, {1, 1, 1, 0, 0},
                       {2, 33, 1, 0, 0}, {3, 0, 1, 0, 0},   {1, 1, 1, 0, 1}};
    for (const auto &entry : cases) {
        auto before = steam_people_fixture();
        const auto created =
            ref::prepare_world_task_creation(sim::startup_world_runtime_factory(before), 0);
        require(created.candidate && created.candidate->created_task &&
                    sim::write_startup_world_runtime_factory(before, created.candidate->state),
                "Loot receipt fixture must use a real factory task and site");
        const auto task = *created.candidate->created_task;
        const auto facility = *before.tasks.at(task).facility;
        before.active_task = task;
        before.scene.world.world.ai.task_active = true;
        const auto human = sim::startup_world_human_details(before, 1);
        require(human && human->live_actor, "Loot receipt fixture lost its allocated visitor");
        const auto actor = *human->live_actor;
        auto &site = before.scene.world.world.facilities.at(facility);
        const auto cell = site.placement.anchor;
        site.status = 1;
        site.occupants = {actor};
        before.scene.world.world.ai.battle.actors.at(actor).control.state = 14;
        before.scene.world.world.ai.battle.actors.at(actor).position = {float(cell.x * 100 + 50), 0,
                                                                        float(cell.y * 100 + 50)};
        before.scene.world.world.ai.contexts.at(actor) =
            ref::RewardActorContext{cell, false, {}, {}};
        before.scene.world.world.actors.at(actor).binding =
            ref::ArrivalBinding{cell, {facility}, site.placement.definition_id};
        before.dungeon_actors.at(actor) = ref::DungeonActorProgress{};
        auto &progress = before.dungeon_facilities.at(facility);
        progress.extent = *ref::dungeon_extent(1, 80); // Source Q80; not a wall-clock duration.
        progress.challenges.assign(entry.grants, {0, 0, 0, 0, entry.kind, entry.definition});
        auto &stock = before.catalog.at({entry.kind, entry.definition});
        stock.status = entry.status;
        stock.free_purchases = 0;
        if (entry.kind == 0) {
            stock.inventory = entry.held;
            before.items.at(entry.definition) = stock;
        }
        const auto result = sim::prepare_startup_world_runtime_dungeon_crew(before, facility);
        require(result.has_value(), "Loot fixture direct source grant was rejected");
        const auto observed = task_loot_receipts(before, *result);
        require(!observed.gap && observed.receipts.size() == 1,
                "Source direct grant lost its task/catalog receipt");
        const auto &receipt = observed.receipts.front();
        require(receipt.definition == entry.definition && receipt.subject == task && receipt.page &&
                    receipt.before[1] == entry.status && receipt.before[2] == entry.grants &&
                    receipt.before[3] == before.tasks.at(task).definition &&
                    receipt.after[0] == (entry.kind == 0 ? std::min(999, entry.held + entry.grants)
                                         : entry.status == 0 ? 1
                                                             : 0),
                "Direct loot receipt confused item inventory, equipment qualification or task ID");
        const auto repeated = sim::prepare_startup_world_runtime_dungeon_crew(*result, facility);
        require(repeated && task_loot_receipts(*result, *repeated).receipts.empty() &&
                    task_loot_receipts(*result, *result).receipts.empty(),
                "Unchanged/aged challenge or loaded snapshot was re-granted by observation");
        auto bad = *result;
        bad.catalog.at({entry.kind, entry.definition}).status = 3;
        const auto rejected = task_loot_receipts(before, bad);
        require(rejected.gap && rejected.receipts.empty(),
                "Unaccounted catalog mutation earned a receipt");
        if (entry.kind == 0) {
            auto conflict_before = before, conflict_after = *result;
            ref::GroundObjectState object;
            object.id = ref::ObjectId{900};
            object.state = 5;
            object.counter = 19;
            object.kind = entry.kind;
            object.definition = entry.definition;
            conflict_before.scene.world.world.ai.battle.objects.emplace(900, object);
            object.counter = 20;
            conflict_after.scene.world.world.ai.battle.objects.emplace(900, object);
            const auto ambiguous = task_loot_receipts(conflict_before, conflict_after);
            require(ambiguous.gap && ambiguous.receipts.empty(),
                    "Same-definition timed pickup was attributed to the task");

            // A different ground item shares the source E counter, but must not
            // gain a task receipt merely because its pickup happened this round.
            object.definition = 1;
            object.counter = 19;
            conflict_before.scene.world.world.ai.battle.objects.at(900) = object;
            object.counter = 20;
            conflict_after.scene.world.world.ai.battle.objects.at(900) = object;
            ++conflict_after.item_rewards;
            ++conflict_after.catalog.at({0, 1}).inventory;
            conflict_after.items.at(1) = conflict_after.catalog.at({0, 1});
            const auto isolated = task_loot_receipts(conflict_before, conflict_after);
            require(!isolated.gap && isolated.receipts.size() == 1 &&
                        isolated.receipts.front().definition == entry.definition,
                    "Independent ground pickup confused the shared counter or earned task credit");
        }
    }
}

// Explicit first-star qualifications still consume the actual paid building,
// source Init and confirmation. Current values cannot replace the original cache.
void rank_promotion_receipt_contract(const CampaignCommandConsumer &apply) {
    auto qualified = initial_world();
    const int definition = 35; // First-star condition3; pay/build through the real consumer.
    qualified.facility_presence.at(definition) = 2;
    const auto selected = sim::begin_startup_world_build(qualified, definition);
    require(selected.error == sim::StartupWorldRuntimeError::none &&
                selected.denial == sim::StartupBuildDenial::none,
            "Rank fixture cannot select its prerequisite building");
    std::optional<State> built;
    const auto &map = qualified.scene.world.world.map;
    for (int y = 0; y < map.height && !built; ++y)
        for (int x = 0; x < map.width && !built; ++x) {
            auto candidate = qualified;
            const auto result = sim::confirm_startup_world_build(candidate, {x, y},
                                                                 ref::FacilityOrientation::first);
            if (result.error == sim::StartupWorldRuntimeError::none && result.created)
                built = std::move(candidate);
        }
    require(built && sim::cancel_startup_world_build(*built) == sim::StartupWorldRuntimeError::none,
            "Rank fixture must legally place and leave its paid construction");
    qualified = std::move(*built);
    // Numerical qualification only. This is not a natural first-star certificate.
    qualified.popularity = 300;
    qualified.maximum_income = 5000;
    qualified.events_held = 2;
    ref::WorldScriptPage page;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 48;
    const auto created =
        ref::prepare_world_script_page(sim::startup_world_runtime_scripts(qualified), page);
    require(created.candidate &&
                sim::write_startup_world_runtime_scripts(qualified, created.candidate->state),
            "Rank fixture cannot install its actual confirmation page");
    require(sim::initialize_startup_world_town_progress_pages(qualified),
            "Rank fixture admits its complete source selection before update");
    const auto initialized = sim::update_startup_world_runtime_page(qualified);
    require(initialized.has_value(), "Source rank Init failed");
    qualified = *initialized;
    require(std::all_of(qualified.rank_met.begin(), qualified.rank_met.end(),
                        [](bool value) { return value; }),
            "Source rank Init did not find the four qualified first-star conditions");
    for (const bool current_changed : {false, true}) {
        auto before = qualified;
        if (current_changed)
            before.popularity = 299; // Source confirmation still consumes the initialized cache.
        auto after = before;
        app::WorldCommand confirm;
        confirm.kind = Kind::rank_action;
        confirm.page = top(before)->id;
        const auto result = apply(after, confirm);
        require(result.outcome == app::WorldCommandOutcome::applied && after.rank == 1,
                "Actual rank confirmation failed or rejudged its initialized conditions");
        const auto receipt = rank_promotion_receipt(before, after);
        require(receipt && receipt->definition == 1 && receipt->page == confirm.page &&
                    receipt->before[0] == 0 && receipt->after[0] == 1 && receipt->before[1] == 15 &&
                    receipt->after[1] == 15 && !rank_promotion_receipt(after, after),
                "Rank receipt lost its actual transition or reconstructed cold history");
        CampaignCoverage coverage;
        std::ostringstream log;
        coverage.record(*receipt, log);
        require(log.str().find("\trank\t1\tconditions_promoted\t") != std::string::npos,
                "First display star must keep its own source identity");
        for (int fault = 0; fault < 8; ++fault) {
            auto old = before, bad = after;
            switch (fault) {
            case 0:
                old.rank_met[0] = false;
                break;
            case 1:
                --old.rank_values[1];
                break;
            case 2:
                old.scripts.pages.back().legacy_page = 74;
                break;
            case 3:
                bad.rank = 2;
                break;
            case 4:
                ++bad.rank_history.at(0)[1];
                break;
            case 5:
                bad.scripts.user_flags &= ~8U;
                break;
            case 6:
                for (auto &p : bad.scripts.pages)
                    if (p.legacy_page == 50)
                        p.lifecycle = 4;
                break;
            case 7:
                for (const auto &activity : before.rules->activities)
                    if (activity.parameters[6] == 1)
                        bad.scripts.activities.at(activity.identity).status = 0;
                break;
            }
            bool rejected{};
            try {
                (void)rank_promotion_receipt(old, bad);
            } catch (const std::runtime_error &) {
                rejected = true;
            }
            require(rejected, "Contradictory rank commit produced a success receipt");
        }
        auto invalid = *receipt;
        invalid.before[1] = 7;
        bool rejected{};
        try {
            coverage.record(invalid, log);
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        require(rejected, "Wire formatter accepted an incomplete rank-condition mask");
    }
    auto cancelled = qualified;
    app::WorldCommand cancel;
    cancel.kind = Kind::rank_action;
    cancel.page = top(cancelled)->id;
    cancel.cancel = true;
    require(apply(cancelled, cancel).outcome == app::WorldCommandOutcome::applied &&
                !rank_promotion_receipt(qualified, cancelled),
            "Cancellation must not earn a promotion receipt");
}

// Qualification inputs are explicit. The real page scheduler pays/reset once;
// this verifies evidence wiring, not a naturally earned house or annual tax route.
void residence_tax_receipt_contract() {
    for (const int amount : {140, 0, -10}) {
        auto before = steam_people_fixture();
        before.human_presence.at(0) = 1;
        before.human_homes.at(0)[2] = 1;
        before.human_homes.at(1)[2] = 1;
        const int first_amount = amount == 0 ? 0 : 20;
        before.human_calendar.at(0).legacy_G = first_amount;
        before.human_calendar.at(1).legacy_G = amount;
        before.human_presence.at(2) = 1; // Open nonresident: reset but no contribution.
        before.human_homes.at(2)[2] = 0;
        before.human_calendar.at(2).legacy_G = 90;
        before.scene.world.world.ai.battle.humans.at(2).battle_reward_stat = 700;
        before.human_presence.at(3) = 0; // Absent resident: excluded by p, still reset.
        before.human_homes.at(3)[2] = 1;
        before.human_calendar.at(3).legacy_G = 50;
        const auto install = [](State &state, int raw) {
            ref::WorldScriptPage page;
            page.kind = ref::WorldScriptPageKind::raw_page;
            page.legacy_page = raw;
            const auto created =
                ref::prepare_world_script_page(sim::startup_world_runtime_scripts(state), page);
            require(created.candidate &&
                        sim::write_startup_world_runtime_scripts(state, created.candidate->state),
                    "Tax receipt fixture did not install its source page");
            return state.scripts.pages.back().id;
        };
        auto displayed = before;
        const auto list = install(displayed, 90);
        for (int n = 0; n < 4 && top(displayed) && top(displayed)->lifecycle != 2; ++n) {
            auto initialized = sim::prepare_startup_world_runtime(displayed);
            require(initialized.candidate.has_value(),
                    "Tax receipt fixture source initialization failed");
            displayed = std::move(*initialized.candidate);
        }
        require(top(displayed) && top(displayed)->id == list && top(displayed)->lifecycle == 2,
                "Tax receipt fixture must admit90 before confirmation");
        const auto old_list = displayed;
        require(
            sim::act_startup_world_tax_page(displayed, list, sim::StartupWorldTaxAction::confirm) ==
                    sim::StartupWorldRuntimeError::none &&
                residence_tax_receipts(old_list, displayed).empty(),
            "Tax list confirmation was certified as income");
        const auto page = install(before, 98);
        const auto paused = before;
        require(residence_tax_receipts(paused, paused).empty(),
                "Unconsumed automatic tax page earned a receipt");
        auto update = sim::prepare_startup_world_runtime(before);
        require(update.candidate.has_value(), "Tax receipt source scheduler rejected its page");
        auto receipts = residence_tax_receipts(before, *update.candidate);
        if (receipts.empty()) {
            before = std::move(*update.candidate); // Source initialization may precede its Update.
            update = sim::prepare_startup_world_runtime(before);
            require(update.candidate.has_value(), "Tax receipt source Update was rejected");
            receipts = residence_tax_receipts(before, *update.candidate);
        }
        const auto &after = *update.candidate;
        require(receipts.size() == 2 && receipts[0].definition == 0 &&
                    receipts[1].definition == 1 && receipts[1].recipient == 1 &&
                    receipts[1].page == page && receipts[1].before[1] == amount &&
                    receipts[1].after[1] == 0 &&
                    receipts[1].after[0] - receipts[1].before[0] == first_amount + amount &&
                    receipts[1].after[2] - receipts[1].before[2] == first_amount + amount &&
                    residence_tax_receipts(after, after).empty(),
                "Source tax receipt confused eligible contribution, total income or replay");
        const auto rejected = [&](State bad) {
            try {
                residence_tax_receipts(before, bad);
            } catch (const std::runtime_error &) {
                return;
            }
            throw std::runtime_error("Unaccounted tax candidate earned a receipt");
        };
        auto bad = after;
        ++bad.monthly_cash.at(before.scene.calendar.month)[4][0];
        rejected(bad);
        bad = after;
        bad.human_calendar.at(2).legacy_G = 90;
        rejected(bad);
        bad = after;
        bad.scripts.notices.back().message = 2;
        rejected(bad);
        bad = after;
        bad.scene.random.draw(7);
        rejected(bad);
    }
}

// Source-call fixture: real encounter allocation, then explicit corpse/tier qualifications.
// Death results come from the maintained consumer; this is not a natural defeat or full tick.
void monster_retirement_receipt_contract() {
    ref::EncounterCreationInput input;
    input.center = {10, 18};
    input.month_index = 3;
    const auto spawned = sim::prepare_startup_world_runtime_encounter(initial_world(), input);
    require(spawned && !spawned->scene.world.world.ai.monster_order.empty(),
            "retirement fixture needs a real allocated monster");
    auto before = *spawned;
    auto &ai = before.scene.world.world.ai;
    const auto actor = ai.monster_order.front();
    const int definition = ai.battle.actors.at(actor).definition;
    ai.battle.actors.at(actor).control.state = 3;
    ai.battle.actors.at(actor).hp = {};
    ai.battle.actors.at(actor).state_counter = 12;
    ai.battle.actors.at(actor).state_parameter = 0;
    // Independent published oracle: ordinary growth99 ->100 pays120 for base100.
    ai.monster_growth.at(definition).growth = 99;
    ai.monster_growth.at(definition).base_cash_reward = 100;
    ai.battle.monsters.at(definition).flags &= ~4U;
    const auto consume = [&](const State &source) {
        const auto death = ref::prepare_monster_death_commit(source.scene.world.world.ai, actor);
        require(death.candidate.has_value(), "actual source corpse consumer rejected fixture");
        auto result = source;
        result.scene.world.world.ai = death.candidate->state;
        return result;
    };
    const auto after = consume(before);
    const auto receipt = monster_retirement_receipts(before, after);
    require(
        !receipt.gap && receipt.receipts.size() == 1 &&
            receipt.receipts.front().kind == CampaignReceiptKind::monster_retirement_cash &&
            receipt.receipts.front().definition == definition &&
            receipt.receipts.front().subject == actor.value &&
            receipt.receipts.front().before[1] == 99 && receipt.receipts.front().after[1] == 100 &&
            receipt.receipts.front().after[2] == 120 && receipt.receipts.front().after[3] == 1 &&
            after.scene.world.world.ai.accounting.funds() == ai.accounting.funds() + 120 &&
            ai.battle.actors.count(actor) && ai.monster_growth.at(definition).growth == 99,
        "receipt must bind actual retirement, tier boundary and posted120, without input mutation");
    require(monster_retirement_receipts(after, after).receipts.empty(),
            "cold or unchanged holdings cannot reconstruct a retirement reward");
    // Monthly k.u resets after actor consumers; lifetime k.v and the posted entry remain.
    const auto reset_month = [&](const State &source) {
        const auto adapter = sim::startup_world_runtime_adapter();
        auto date = source.scene.calendar;
        if (++date.month == 12) {
            date.month = 0;
            ++date.year;
        }
        auto dated = source; // The real calendar caller installs the normalized date first.
        dated.scene.calendar = date;
        const auto reset = ref::prepare_owned_world_runtime_calendar(
            dated, date, ref::WorldCalendarStage::month_kill_reset, adapter);
        require(reset.has_value(), "actual monthly kill reset consumer rejected fixture");
        return *reset;
    };
    const auto monthly = monster_retirement_receipts(before, reset_month(after));
    require(!monthly.gap && monthly.receipts.size() == 1 &&
                monthly.receipts.front().kind ==
                    CampaignReceiptKind::monster_retirement_cash_month_reset &&
                monthly.receipts.front().after[0] == 0 &&
                monthly.receipts.front().after[1] == 100 &&
                monthly.receipts.front().after[2] == 120,
            "same-frame monthly reset must retain actual cash/growth and final zero tally");
    const auto quiet_month = monster_retirement_receipts(after, reset_month(after));
    require(!quiet_month.gap && quiet_month.receipts.empty(),
            "a monthly tally reset alone is neither a cash grant nor a counter failure");
    auto early = before;
    early.scene.world.world.ai.battle.actors.at(actor).state_counter = 11;
    require(monster_retirement_receipts(early, consume(early)).receipts.empty(),
            "B11 cannot certify retirement cash");
    auto cancelled = before;
    cancelled.scene.world.world.ai.battle.actors.at(actor).state_parameter = 1;
    const auto unpaid = consume(cancelled);
    require(!unpaid.scene.world.world.ai.battle.actors.count(actor) &&
                monster_retirement_receipts(cancelled, unpaid).receipts.empty() &&
                unpaid.scene.world.world.ai.accounting.funds() == ai.accounting.funds(),
            "C1 removal has no definition growth or retirement cash");
    // A second corpse is a minimal call-point roster qualification, not another
    // simulated encounter. Two real death commits test same-definition aggregation.
    auto multiple = before;
    auto &pair = multiple.scene.world.world.ai;
    const ref::CharacterId second{pair.next_actor_id++};
    auto clone = pair.battle.actors.at(actor);
    clone.id = second;
    clone.legacy_id += 1;
    pair.battle.actors.emplace(second, clone);
    pair.monster_order.push_back(second);
    pair.contexts.emplace(second, pair.contexts.at(actor));
    if (clone.encounter)
        pair.encounters.at(*clone.encounter).members.push_back(second);
    auto both = consume(multiple);
    const auto second_death = ref::prepare_monster_death_commit(both.scene.world.world.ai, second);
    require(second_death.candidate.has_value(), "second source corpse commit failed");
    both.scene.world.world.ai = second_death.candidate->state;
    const auto combined = monster_retirement_receipts(multiple, both);
    require(!combined.gap && combined.receipts.size() == 1 &&
                combined.receipts.front().before[3] == 2 &&
                combined.receipts.front().after[1] == 101 &&
                combined.receipts.front().after[2] == 240,
            "two corpses share one definition batch, with two actual120 entries");
    const auto gap = [&](const State &invalid) {
        const auto result = monster_retirement_receipts(before, invalid);
        require(result.gap && result.receipts.empty(),
                "bad binding must not produce partial retirement success");
    };
    auto bad = after;
    bad.scene.world.world.ai.monster_growth.at(definition).growth -= 1;
    gap(bad);
    bad = after;
    bad.scene.world.world.ai.monster_order.push_back(actor);
    gap(bad);
    bad = after;
    auto ledger = bad.scene.world.world.ai.accounting.snapshot();
    ledger.entries.at(ai.next_cash_id).amount = 100; // XP-side old tier is not the cash amount.
    ledger.funds -= 20;
    const auto changed = ref::PeriodAccounting::from_snapshot(ledger);
    require(changed.has_value(), "valid contradictory cash fixture could not be prepared");
    bad.scene.world.world.ai.accounting = *changed;
    gap(bad);
    CampaignCoverage coverage;
    std::ostringstream log;
    coverage.record(receipt.receipts.front(), log);
    require(log.str().find("\tretirement_cash_committed\t") != std::string::npos,
            "bound retirement receipt must retain its distinct stage");
    const auto saved_log = log.str();
    auto invalid = receipt.receipts.front();
    invalid.after[0] -= 1;
    bool rejected{};
    try {
        coverage.record(invalid, log);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    require(rejected && log.str() == saved_log,
            "invalid retirement evidence cannot alter the emitted log");
}

void steam_people_receipt_contract(const CampaignCommandConsumer &apply) {
    using Receipt = ark::test::CampaignReceiptKind;
    // Explicit reward/paid qualifications test consumption; not natural acquisition.
    // Reuse each namespace's independent arrival Owner and actual source gift flow.
    for (const bool purchase : {false, true})
      for (int source_kind : {1, 2, 3}) {
        auto state = steam_people_fixture();
        for (auto &[key, stock] : state.catalog)
            if (key.first >= 1 && key.first <= 3) {
                stock.free_purchases = 0; // Each case isolates one source namespace.
                if (purchase) stock.status = 0;
            }
        for (const auto &definition : state.rules->equipment)
            if (definition.shop.kind == source_kind &&
                std::any_of(definition.shop.combat.begin(), definition.shop.combat.end(),
                            [](int value) { return value > 0; })) {
                auto &stock = state.catalog.at({source_kind, definition.shop.id});
                stock.status = 1;
                stock.free_purchases = purchase ? 0 : 1;
            }
        if (purchase)
            state.scene.world.world.ai.accounting = ref::PeriodAccounting(
                steam_operating_reserve(state) + 10000);
        ark::test::SteamVillageStrategy strategy;
        std::vector<ark::test::CampaignReceipt> receipts;
        const auto initial = sim::startup_world_human_details(state, 1);
        std::cout << "PEOPLE_FIXTURE kind=" << source_kind << " equipment=";
        for (const auto &entry : initial->equipment)
            std::cout << entry.value_or(-1) << ',';
        std::cout << std::endl;
        const Receipt equipped = purchase
                                     ? (source_kind == 1 ? Receipt::weapon_paid_equipped
                                        : source_kind == 2 ? Receipt::armour_paid_equipped
                                                           : Receipt::accessory_paid_equipped)
                                     : (source_kind == 1 ? Receipt::weapon_reward_equipped
                                        : source_kind == 2 ? Receipt::armour_reward_equipped
                                                           : Receipt::accessory_reward_equipped);
        const Receipt combat = source_kind == 1   ? Receipt::weapon_combat_applied
                               : source_kind == 2 ? Receipt::armour_combat_applied
                                                  : Receipt::accessory_combat_applied;
        run_steam_receipt_fixture(
            apply, state, strategy,
            [&] {
                const auto *page = top(state);
                return receipts.size() == 2 && page &&
                       page->kind == ref::WorldScriptPageKind::scene && state.scene.scene_state == 0;
            },
            [&](const auto &receipt) {
                if (receipt.kind == equipped || receipt.kind == combat)
                    receipts.push_back(receipt);
            });
        const auto &paid = receipts[0], &effect = receipts[1];
        const auto profile = sim::startup_world_human_details(state, 1);
        require(paid.kind == equipped && effect.kind == combat &&
                    paid.definition == effect.definition && paid.recipient == 1 &&
                    effect.recipient == 1 && paid.page == effect.page &&
                    paid.subject == effect.subject && profile && profile->live_actor &&
                    paid.subject == profile->live_actor->value &&
                    (purchase ? paid.before[0] == 0 && paid.after[0] == 0 &&
                                    paid.before[1] > paid.after[1]
                              : paid.before[0] == paid.after[0] + 1 && paid.before[1] == paid.after[1]) &&
                    paid.after[3] == paid.definition &&
                    profile->equipment.at(paid.after[2]) == paid.definition &&
                    std::equal(effect.after.begin(), effect.after.end(), profile->combat.begin()),
                "Source equipment consumption, actor binding and attributes were not independently "
                "recorded");
        if (purchase) {
            const auto item = std::find_if(state.rules->equipment.begin(), state.rules->equipment.end(),
                                           [&](const auto &e) { return e.shop.kind == source_kind && e.shop.id == paid.definition; });
            require(item != state.rules->equipment.end(), "paid gift lost its original definition");
            const auto quote = ref::human_equipment_gift_cost(
                {item->shop.id, int(paid.after[2]), 1, item->gift_order, item->reward_difficulty,
                 item->shop.price, item->gift_rating, 0, item->shop.combat});
            require(quote && *quote > 0 && paid.before[1] - paid.after[1] == *quote,
                    "paid equipment did not charge the independently queried source gift price");
        }
    }
    auto awarded = steam_people_fixture();
    ref::WorldScriptPage annual;
    annual.kind = ref::WorldScriptPageKind::raw_page;
    annual.legacy_page = 87; // Explicit annual-entry fixture, initialized by the real runtime.
    const auto page =
        ref::prepare_world_script_page(sim::startup_world_runtime_scripts(awarded), annual);
    require(page.candidate &&
                sim::write_startup_world_runtime_scripts(awarded, page.candidate->state),
            "Annual receipt fixture did not install its source page");
    ark::test::SteamVillageStrategy awards;
    std::optional<ark::test::CampaignReceipt> medal;
    run_steam_receipt_fixture(
        apply, awarded, awards, [&] { return medal.has_value(); },
        [&](const auto &receipt) {
            if (receipt.kind == Receipt::human_medal_awarded)
                medal = receipt;
        });
    require(medal->definition == 1 && medal->recipient == 1 && medal->page &&
                medal->before[0] == medal->after[0] + 1 &&
                medal->after[1] == medal->before[1] + 1 &&
                awarded.human_calendar.at(1).celebrations == medal->after[1],
            "Source medal receipt lacks its actual recipient and one-use qualification");

    auto housed = steam_people_fixture();
    // Existing expanded geometry is an explicit admission prerequisite, not a paid
    // activity certificate. Use the source map consumer to keep its caches consistent.
    const auto plot = std::find_if(
        ark::test::steam_village_layout().begin(), ark::test::steam_village_layout().end(),
        [](const auto &target) { return target.role == ark::test::SteamLayoutRole::recruitment; });
    require(plot != ark::test::steam_village_layout().end(),
            "Source layout lacks its recruitment goal");
    while (!ark::test::steam_layout_inside_fence(housed, *plot)) {
        require(housed.fence_level + 1 < int(housed.rules->fences.size()) &&
                    sim::expand_startup_world_map(housed),
                "Residence fixture cannot prepare its source-expanded geometry");
    }
    // Provide a legally paid recruitment plot before the autonomous layout policy
    // spends the small fixture budget on a new inn. The real admission remains its decision.
    const auto selection = sim::begin_startup_world_build(housed, 24);
    require(selection.error == sim::StartupWorldRuntimeError::none &&
                selection.denial == sim::StartupBuildDenial::none,
            "Residence fixture cannot select its recruitment plot; denial=" +
                std::to_string(int(selection.denial)) +
                " presence=" + std::to_string(housed.facility_presence.at(24)));
    app::WorldCommand build;
    build.kind = Kind::confirm_build;
    build.definition = 24;
    build.anchor = plot->anchor;
    const auto quote = sim::startup_world_build_quote(housed, 24);
    const auto money = housed.scene.world.world.ai.accounting.funds();
    const auto constructed = apply(housed, build);
    require(constructed.outcome == app::WorldCommandOutcome::applied && constructed.created &&
                quote &&
                money - housed.scene.world.world.ai.accounting.funds() == quote->construction_cost,
            "Residence fixture must install and pay for its actual recruitment plot; denial=" +
                std::to_string(int(constructed.build_denial)) +
                " error=" + std::to_string(int(constructed.runtime_error)));
    build.kind = Kind::cancel_build;
    require(apply(housed, build).outcome == app::WorldCommandOutcome::applied,
            "Residence fixture construction mode did not return to the main scene");
    for (int step = 0;
         step < 600 && housed.scene.world.world.facilities.at(*constructed.created).status != 1;
         ++step) {
        const auto *page = top(housed);
        if (page && page->kind == ref::WorldScriptPageKind::dialogue && page->lifecycle == 2)
            require(sim::acknowledge_startup_world_runtime_page(housed, page->id) ==
                        sim::StartupWorldRuntimeError::none,
                    "Residence fixture construction dialogue did not close");
        auto update = sim::prepare_startup_world_runtime(housed);
        require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                "Residence fixture construction did not advance normally");
        housed = std::move(*update.candidate);
    }
    require(housed.scene.world.world.facilities.at(*constructed.created).status == 1,
            "Source recruitment plot must be ready before testing admission");
    housed.shop_humans.at(1).satisfaction = housed.rules->humans.at(1).residence_threshold;
    // Admission now reserves the new house's maintenance as well. Qualify only
    // this short decision fixture; real campaigns must earn their own cash.
    auto ledger = housed.scene.world.world.ai.accounting.snapshot();
    ledger.funds = steam_residence_reserve(housed) + housed.rules->humans.at(1).residence_fee;
    const auto qualified_cash = ref::PeriodAccounting::from_snapshot(ledger);
    require(qualified_cash.has_value(), "Residence fixture cannot qualify its operating budget");
    housed.scene.world.world.ai.accounting = *qualified_cash;
    ark::test::SteamVillageStrategy homes;
    std::optional<ark::test::CampaignReceipt> admission;
    run_steam_receipt_fixture(
        apply, housed, homes, [&] { return admission.has_value(); },
        [&](const auto &receipt) {
            if (receipt.kind == Receipt::human_home_admitted)
                admission = receipt;
        });
    require(admission->definition == 1 && admission->recipient == 1 && admission->subject &&
                admission->before[0] - admission->after[0] ==
                    housed.rules->humans.at(1).residence_fee &&
                admission->before[1] == 0 && admission->after[1] == 1 &&
                housed.human_homes.at(1)[2] == 1 &&
                housed.scene.world.world.facilities.at(admission->subject).kind == 12 &&
                housed.facility_residents.at(admission->subject) == 1,
            "Actual admission lacks its one-time source fee and bound residential instance");
}

void steam_profession_receipt_contract(const CampaignCommandConsumer &apply) {
    // Arrival is source-created; mastery/opening/medals are explicit qualifications,
    // not evidence of naturally unlocking this profession or completing the campaign.
    auto state = steam_people_fixture();
    constexpr int human = 1, target = 5;
    auto &growth = state.scene.world.world.ai.growth.at(human);
    const int old_job = growth.definition.current_profession;
    growth.definition.profession_levels.at(old_job) = 10;
    state.scripts.professions.at(target).status = 1;
    state.human_calendar.at(human).celebrations = 99;
    state.village_points = 999;
    const auto source_stats = ref::derive_human_stats(growth.definition,
                                                     state.scene.world.world.ai.professions);
    require(source_stats.candidate && !source_stats.candidate->available_spells[0] &&
                growth.definition.spell_professions.at(0) == target,
            "Profession qualification must start without the source mage spell");
    growth.derived = *source_stats.candidate;
    require(sim::synchronize_startup_world_human_capacity(state, human),
            "Profession qualification lost the real actor's capacity");
    state.scene.world.world.ai.accounting = ref::PeriodAccounting(steam_operating_reserve(state) + 10000);
    const auto initial = sim::startup_world_human_details(state, human);
    require(initial && initial->live_actor, "Profession receipt fixture lost its live actor");
    const auto learned = growth.definition.learned_spells;
    const int quote = state.rules->jobs.at(target).change_points;
    SteamProfessionStrategy flow;
    CampaignCoverage coverage;
    std::ostringstream log;
    std::vector<CampaignReceipt> receipts;
    const auto receive = [&](const CampaignReceipt &receipt) {
        coverage.record(receipt, log);
        receipts.push_back(receipt);
    };
    for (int step = 0; step < 600; ++step) {
        const auto decision = flow.next(state, 0, steam_operating_reserve(state), receipts.empty());
        auto input = decision.command;
        if (!decision.handled) {
            const auto *p = top(state);
            require(p && (p->kind == ref::WorldScriptPageKind::dialogue ||
                           p->legacy_page == 15 || p->legacy_page == 67 || p->legacy_page == 70),
                    "Profession fixture reached an unrelated source page");
            if (p->lifecycle == 2 &&
                (p->legacy_page != 70 || sim::startup_world_human_page_ready(state, p->id))) {
                app::WorldCommand acknowledge;
                acknowledge.kind = Kind::acknowledge_page;
                acknowledge.page = p->id;
                if (p->legacy_page == 70) {
                    acknowledge.kind = Kind::human_action;
                    acknowledge.human_action = sim::StartupHumanPageAction::confirm;
                    if (state.page_phases.at(p->id) == 2 && state.human_page_selections.at(p->id) != 1) {
                        acknowledge.human_action = sim::StartupHumanPageAction::select;
                        acknowledge.selection = 1;
                    }
                }
                input = acknowledge;
            }
        }
        if (input) {
            auto candidate = state;
            const auto result = apply(candidate, *input);
            require(result.outcome == app::WorldCommandOutcome::applied,
                    "Profession fixture's real source input was rejected");
            flow.observe(state, *input, result, candidate, receive);
            state = std::move(candidate);
        }
        if (receipts.size() == 4 && !flow.active())
            break;
        auto update = sim::prepare_startup_world_runtime(state);
        require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                "Profession fixture failed its real source update");
        flow.observe_tick(state, *update.candidate, receive);
        state = std::move(*update.candidate);
    }
    const auto final = sim::startup_world_human_details(state, human);
    require(receipts.size() == 4 && !flow.active() && final && final->profession == target &&
                state.scene.world.world.ai.growth.at(human).definition.learned_spells == learned,
            "Profession fixture did not finish its source flow with permanent learning unchanged");
    const auto &paid = receipts[0], &set = receipts[1], &stats = receipts[2], &spell = receipts[3];
    using Receipt = CampaignReceiptKind;
    require(paid.kind == Receipt::profession_points_paid && paid.definition == target &&
                paid.before[0] - paid.after[0] == quote && paid.after[1] == paid.before[1] + 1 &&
                paid.before[2] == old_job && paid.after[2] == old_job && paid.before[3] == paid.after[3] &&
                set.kind == Receipt::profession_set && set.definition == target && set.page != paid.page &&
                set.before[0] == old_job && set.after[0] == target && set.before[2] == 54 && set.after[2] == 55 &&
                stats.kind == Receipt::profession_stats_applied && stats.definition == target && stats.page == set.page &&
                std::equal(stats.after.begin(), stats.after.end(), final->combat.begin()) &&
                spell.kind == Receipt::spell_available && spell.definition == 0 && spell.page == stats.page &&
                spell.step == stats.step && spell.before[0] == 0 && spell.after[0] == 1 &&
                spell.before[1] == spell.after[1] && final->spells[0],
            "Real62 payment,55 setter,197 stats or separate spell availability lacks its numeric receipt");
    for (const auto &receipt : receipts)
        require(receipt.recipient == human && receipt.subject == initial->live_actor->value,
                "Profession/spell stages were joined across unrelated source humans or actors");
    const auto saved = log.str();
    flow.observe_tick(state, state, receive);
    require(log.str() == saved && receipts.size() == 4,
            "An unchanged profession observation republished a source receipt");
}

// Inventory eligibility is explicit; selection,75 consumption and76 shared improvement
// still run through the real strategy and Owner. This does not prove natural acquisition.
void steam_investment_receipt_contract(const CampaignCommandConsumer &apply) {
    using Receipt = ark::test::CampaignReceiptKind;
    for (int item : {0, 18}) {
        auto state = steam_people_fixture();
        for (auto &[key, stock] : state.catalog)
            if (key.first >= 1 && key.first <= 3)
                stock.free_purchases = 0;
        for (auto &[id, stock] : state.items) {
            stock.inventory = id == item ? 1 : 0;
            state.catalog.at({0, id}).inventory = stock.inventory;
        }
        ark::test::SteamVillageStrategy strategy;
        std::optional<ark::test::CampaignReceipt> consumed, improved;
        run_steam_receipt_fixture(
            apply, state, strategy, [&] { return improved.has_value(); },
            [&](const auto &receipt) {
                if (receipt.kind == Receipt::item_facility_consumed) {
                    require(!consumed && !improved && receipt.definition == item &&
                                receipt.before[0] == 1 && receipt.after[0] == 0 &&
                                receipt.before[1] == receipt.after[1] && receipt.subject,
                            "Facility inventory receipt lacks its one-use binding or actual cash");
                    consumed = receipt;
                } else if (receipt.kind == Receipt::facility_item_improved) {
                    require(consumed && !improved && receipt.page != consumed->page &&
                                receipt.subject == consumed->subject &&
                                receipt.definition == consumed->after[3] &&
                                receipt.before[3] == item && receipt.after[3] == item,
                            "Facility improvement must follow its separate item consumption");
                    improved = receipt;
                }
            });
        const auto values = sim::startup_world_facility_values(state, improved->subject);
        require(values && state.items.at(item).inventory == 0 &&
                    std::equal(values->instance_attributes.begin(),
                               values->instance_attributes.begin() + 3, improved->after.begin()) &&
                    (item == 0 ? improved->after[0] == improved->before[0] &&
                                     improved->after[1] > improved->before[1]
                               : improved->after[0] > improved->before[0] &&
                                     improved->after[2] > improved->before[2]),
                "Source quality-only or price/charm improvement was not applied to its instance");
        std::vector<ark::test::CampaignReceipt> duplicate;
        strategy.observe_tick(state, state,
                              [&](const auto &receipt) { duplicate.push_back(receipt); });
        require(duplicate.empty(), "Unchanged investment observation republished its receipt");
    }
    // New-shop introduction eligibility belongs to the controller fixture. The real
    // chamber must still supply the selected stock and execute its cash transaction.
    auto paid_state = steam_people_fixture();
    paid_state.rank = 2; // Explicit eligibility for the existing two-star paid experiment.
    paid_state.scripts.user_flags |= 16U;
    for (auto &[id, stock] : paid_state.items) {
        stock.inventory = 0;
        paid_state.catalog.at({0, id}).inventory = 0;
    }
    // Source item18 is a500G introduction with a six-unit shop cap. One available
    // vendor unit is explicit eligibility, not a natural restock/acquisition claim.
    require(paid_state.rules->items.at(18).commerce_price == 500 &&
                paid_state.shop_item_stock.at(18).maximum_quantity == 6,
            "Paid introduction fixture differs from the fixed item18 source row");
    paid_state.shop_item_stock.at(18).quantity = 1;
    ark::test::ActiveIncomeStrategy paid(3600);
    for (const auto &[id, facility] : paid_state.scene.world.world.facilities)
        if (facility.status == 1)
            paid.register_new_shop(id);
    std::optional<ark::test::CampaignReceipt> bought;
    for (int step = 0; step < 80 && !bought; ++step) {
        const auto decision = paid.next(paid_state, 0, true);
        auto input = decision.command;
        if (!decision.handled) {
            const auto *page = top(paid_state);
            require(page && (page->kind == ref::WorldScriptPageKind::dialogue ||
                             page->legacy_page == 15),
                    "Paid fixture has an uncovered parent page: " +
                        std::to_string(page ? page->legacy_page : -1));
            if (page->lifecycle == 2) {
                app::WorldCommand acknowledge;
                acknowledge.kind = Kind::acknowledge_page;
                acknowledge.page = page->id;
                input = acknowledge;
            }
        }
        if (input) {
            auto candidate = paid_state;
            const auto result = apply(candidate, *input);
            require(result.outcome == app::WorldCommandOutcome::applied,
                    "Paid investment fixture command rejected");
            paid.observe(paid_state, *input, candidate, [&](const auto &receipt) {
                require(receipt.kind == Receipt::item_commerce_bought && !bought,
                        "Paid introduction emitted an unrelated or duplicate transaction");
                bought = receipt;
            });
            paid_state = std::move(candidate);
        }
        if (bought)
            break;
        auto update = sim::prepare_startup_world_runtime(paid_state);
        require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                "Paid investment fixture failed its source update");
        paid.observe_tick(paid_state, *update.candidate);
        paid_state = std::move(*update.candidate);
    }
    require(bought && bought->before[0] + 1 == bought->after[0] &&
                bought->before[1] == bought->after[1] + 1 &&
                bought->before[2] - bought->after[2] ==
                    paid_state.rules->items.at(bought->definition).commerce_price &&
                paid_state.items.at(bought->definition).inventory == 1,
            "Real item purchase did not retain its source price, stock and inventory receipt");
}

void steam_transaction_receipt_contract(const CampaignCommandConsumer &apply) {
    auto state = ark::test::initial_world();
    // Source-menu eligibility only. This bounded fixture must still pay/build/claim
    // through production commands; it is not a natural second-star certificate.
    state.rank = 2;
    state.village_points = 120;
    state.scripts.user_flags |= 16U;
    ark::test::SteamVillageStrategy strategy;
    std::set<ark::test::CampaignReceiptKind> stages;
    int receipt_count{};
    const auto receive = [&](const ark::test::CampaignReceipt &receipt) {
        ++receipt_count;
        using Receipt = ark::test::CampaignReceiptKind;
        require(receipt.page != 0, "Steam receipt lost its actual source page");
        if (receipt.kind == Receipt::facility_blueprint_paid)
            require(receipt.before[0] > receipt.after[0] && receipt.before[1] == receipt.after[1],
                    "Steam blueprint payment must precede its unlock");
        if (receipt.kind == Receipt::facility_blueprint_claimed)
            require(receipt.before[0] == receipt.after[0] && receipt.before[1] == 0 &&
                        receipt.after[1] == 2 && receipt.after[2] == receipt.before[2] + 1,
                    "Steam claim lost its separate unlock/free-build effect");
        if (receipt.kind == Receipt::facility_construction_paid)
            require(receipt.subject &&
                        state.scene.world.world.facilities.count(receipt.subject) == 0,
                    "Steam construction must bind a new actual instance");
        stages.insert(receipt.kind);
    };
    using Receipt = ark::test::CampaignReceiptKind;
    const auto achieved = [&] {
        return stages.count(Receipt::facility_blueprint_paid) &&
               stages.count(Receipt::facility_blueprint_claimed) &&
               stages.count(Receipt::facility_construction_paid);
    };
    run_steam_receipt_fixture(apply, state, strategy, achieved, receive);
    const auto count = receipt_count;
    app::WorldCommandResult rejected;
    rejected.outcome = app::WorldCommandOutcome::rejected;
    bool refused{};
    try {
        strategy.observe(state, {}, rejected, state, receive);
    } catch (const std::runtime_error &) {
        refused = true;
    }
    require(refused && receipt_count == count, "Rejected Steam command published a receipt");
}

void source_transaction_receipt_contract(const CampaignCommandConsumer &apply) {
    // A bounded decision fixture, not a natural second-star certificate. Keep 119
    // points for the real 120-point offer, then exercise actual 83/85/93 consumers.
    auto restaurant_state = ark::test::initial_world();
    restaurant_state.rank = 2;
    restaurant_state.village_points = 119;
    restaurant_state.quarter_counter = 1;
    // Explicit chamber-entry eligibility for this isolated decision fixture. The
    // real route must cold-load this flag from the independently verified prefix.
    restaurant_state.scripts.user_flags |= 16U;
    require(restaurant_state.rules->facility_initial.at(40).capacity == 120 &&
                restaurant_state.facility_presence.at(40) == 0,
            "Restaurant fixture differs from the published 120-point locked offer");
    LateStrategy restaurant_budget(3);
    restaurant_budget.reconcile(restaurant_state);
    const auto reserved = restaurant_budget.next(restaurant_state);
    require(!reserved || (reserved->kind != Kind::open_village_activities &&
                          reserved->kind != Kind::open_commerce),
            "Restaurant budget must not spend its reserved points or open an unaffordable offer");
    restaurant_state.village_points = 120;
    LateStrategy restaurant(3);
    restaurant.reconcile(restaurant_state);
    std::vector<ark::test::CampaignReceipt> restaurant_receipts;
    for (int step = 0; step < 300 && !restaurant.stats().western_unlock_claimed; ++step) {
        if (const auto action = restaurant.next(restaurant_state)) {
            auto candidate = restaurant_state;
            const auto result = apply(candidate, *action);
            restaurant.observe(restaurant_state, *action, result, candidate,
                               [&](const ark::test::CampaignReceipt &receipt) {
                                   restaurant_receipts.push_back(receipt);
                               });
            restaurant_state = std::move(candidate);
        }
        if (restaurant.stats().western_unlock_claimed)
            break;
        auto candidate = sim::prepare_startup_world_runtime(restaurant_state);
        require(candidate.candidate.has_value(), "Restaurant quick fixture update failed");
        require(sim::update_startup_world_render_cache(*candidate.candidate),
                "Restaurant quick fixture render cache failed");
        restaurant.observe_tick(restaurant_state, *candidate.candidate);
        restaurant_state = std::move(*candidate.candidate);
    }
    require(restaurant.stats().western_unlock_paid && restaurant.stats().western_unlock_claimed &&
                restaurant.stats().western_unlock_points == 120 &&
                restaurant_state.village_points == 0 &&
                restaurant_state.facility_presence.at(40) == 2,
            "Actual restaurant chamber purchase/claim did not complete exactly once");
    using Receipt = ark::test::CampaignReceiptKind;
    require(restaurant_receipts.size() == 2 &&
                restaurant_receipts[0].kind == Receipt::facility_blueprint_paid &&
                restaurant_receipts[0].definition == 40 && restaurant_receipts[0].page &&
                restaurant_receipts[0].before[0] == 120 && restaurant_receipts[0].after[0] == 0 &&
                restaurant_receipts[0].before[1] == restaurant_receipts[0].after[1] &&
                restaurant_receipts[1].kind == Receipt::facility_blueprint_claimed &&
                restaurant_receipts[1].definition == 40 && restaurant_receipts[1].page &&
                restaurant_receipts[1].before[1] == 0 && restaurant_receipts[1].after[1] == 2 &&
                restaurant_receipts[1].before[2] + 1 == restaurant_receipts[1].after[2] &&
                restaurant_receipts[1].before[0] == restaurant_receipts[1].after[0],
            "Actual blueprint payment and later claim lack separate source-bound receipts");
    auto construction_state = ark::test::initial_world();
    construction_state.rank = 1; // Explicit source-menu eligibility, not a natural first star.
    LateStrategy construction_route;
    construction_route.reconcile(construction_state);
    std::optional<ark::test::CampaignReceipt> construction_receipt;
    for (int step = 0; step < 150 && !construction_receipt; ++step) {
        if (const auto action = construction_route.next(construction_state)) {
            auto candidate = construction_state;
            const auto result = apply(candidate, *action);
            construction_route.observe(construction_state, *action, result, candidate,
                                       [&](const ark::test::CampaignReceipt &receipt) {
                                           if (receipt.kind == Receipt::facility_construction_paid)
                                               construction_receipt = receipt;
                                       });
            construction_state = std::move(candidate);
        }
        if (construction_receipt)
            break;
        auto update = sim::prepare_startup_world_runtime(construction_state);
        require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                "Construction-receipt fixture runtime failed");
        construction_route.observe_tick(construction_state, *update.candidate);
        construction_state = std::move(*update.candidate);
    }
    require(construction_receipt && construction_receipt->page && construction_receipt->subject &&
                construction_state.scene.world.world.facilities.at(construction_receipt->subject)
                        .placement.definition_id == construction_receipt->definition &&
                construction_receipt->before[3] > construction_receipt->after[3],
            "Real gold construction lacks its installed instance and actual charge receipt");
    // Isolated eligibility fixture: consume the real activity pages, not a synthetic
    // flag change. The natural second-star route remains a separate business acceptance.
    auto pot_open_state = ark::test::initial_world();
    pot_open_state.rank = 2;
    pot_open_state.village_points = 100;
    pot_open_state.quarter_counter = 1;
    pot_open_state.scripts.activities.at(30).status = 1;
    LateStrategy pot_open_route;
    std::vector<ark::test::CampaignReceipt> pot_open_receipts;
    ark::test::SteamVillageStrategy steam_pot_observer;
    std::vector<ark::test::CampaignReceipt> steam_pot_receipts;
    for (int step = 0; step < 300 && pot_open_route.stats().pot_month < 0; ++step) {
        std::optional<app::WorldCommand> action;
        const auto *page = top(pot_open_state);
        require(page != nullptr, "Pot-open fixture lost its source page");
        if (page->kind == ref::WorldScriptPageKind::scene) {
            action.emplace();
            action->kind = Kind::open_village_activities;
        } else if (page->kind == ref::WorldScriptPageKind::dialogue) {
            // The actual new-world introduction can precede this isolated menu path.
            action.emplace();
            action->kind = Kind::acknowledge_page;
            action->page = page->id;
        } else if (const auto view =
                       sim::inspect_startup_world_village_activity_page(pot_open_state, page->id)) {
            if (view->raw != 53 || view->counter >= 120) {
                action.emplace();
                action->kind = Kind::village_activity_action;
                action->page = page->id;
                action->village_activity_action = sim::StartupVillageActivityAction::confirm;
                if (view->raw == 51) {
                    const auto selected = std::find(view->entries.begin(), view->entries.end(), 30);
                    require(selected != view->entries.end(),
                            "Pot import not in eligible fixture catalogue");
                    action->selection = static_cast<int>(selected - view->entries.begin());
                    if (view->selection != action->selection)
                        action->village_activity_action = sim::StartupVillageActivityAction::select;
                }
            }
        }
        if (action) {
            auto candidate = pot_open_state;
            const auto result = apply(candidate, *action);
            pot_open_route.observe(pot_open_state, *action, result, candidate,
                                   [&](const ark::test::CampaignReceipt &receipt) {
                                       pot_open_receipts.push_back(receipt);
                                   });
            steam_pot_observer.observe(pot_open_state, *action, result, candidate,
                                       [&](const ark::test::CampaignReceipt &receipt) {
                                           steam_pot_receipts.push_back(receipt);
                                       });
            pot_open_state = std::move(candidate);
        }
        if (pot_open_route.stats().pot_month >= 0)
            break;
        auto update = sim::prepare_startup_world_runtime(pot_open_state);
        require(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                "Pot-open short fixture runtime failed");
        pot_open_route.observe_tick(pot_open_state, *update.candidate);
        steam_pot_observer.observe_tick(pot_open_state, *update.candidate);
        pot_open_state = std::move(*update.candidate);
    }
    require(pot_open_receipts.size() == 2 &&
                pot_open_receipts[0].kind == Receipt::activity_points_paid &&
                pot_open_receipts[0].definition == 30 && pot_open_receipts[0].before[0] == 100 &&
                pot_open_receipts[0].after[0] == 0 &&
                pot_open_receipts[1].kind == Receipt::activity_pot_opened &&
                pot_open_receipts[1].definition == 30 && pot_open_receipts[1].page &&
                !(pot_open_receipts[1].before[0] & 1) && (pot_open_receipts[1].after[0] & 1) &&
                (pot_open_state.scripts.user_flags & 1U),
            "Real pot import must expose separate payment and applied unlock receipts: count=" +
                std::to_string(pot_open_receipts.size()) +
                " raw=" + std::to_string(top(pot_open_state)->legacy_page) +
                " points=" + std::to_string(pot_open_state.village_points) +
                " flags=" + std::to_string(pot_open_state.scripts.user_flags));
    require(steam_pot_receipts.size() == pot_open_receipts.size() &&
                std::equal(steam_pot_receipts.begin(), steam_pot_receipts.end(),
                           pot_open_receipts.begin(),
                           [](const auto &a, const auto &b) {
                               return a.kind == b.kind && a.definition == b.definition &&
                                      a.page == b.page && a.step == b.step &&
                                      a.subject == b.subject && a.recipient == b.recipient &&
                                      a.before == b.before && a.after == b.after;
                           }),
            "Steam bridge lost the already-verified source activity payment/unlock receipt");
}

} // namespace

void campaign_receipt_contract(const CampaignCommandConsumer &apply) {
    if (!apply)
        throw std::invalid_argument("Receipt fixtures require the campaign command consumer");
    source_transaction_receipt_contract(apply);
    task_loot_receipt_contract();
    monster_retirement_receipt_contract();
    rank_promotion_receipt_contract(apply);
    residence_tax_receipt_contract();
    campaign_village_effect_contract(apply);
    steam_transaction_receipt_contract(apply);
    steam_people_receipt_contract(apply);
    steam_profession_receipt_contract(apply);
    steam_investment_receipt_contract(apply);
}
} // namespace ark::test
