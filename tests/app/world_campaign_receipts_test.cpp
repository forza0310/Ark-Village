// Bounded qualification fixtures certify source transactions, not natural unlocks.
// Process orchestration, files and long-route strategy history remain in world_campaign_test.
#include "world_campaign_receipts_test.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/map/startup_world_expansion.hpp"
#include "world_active_late_strategy.hpp"
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
        if (const auto action = strategy.next(state)) {
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

void steam_people_receipt_contract(const CampaignCommandConsumer &apply) {
    using Receipt = ark::test::CampaignReceiptKind;
    // Explicit reward qualifications test consumption; they do not prove a natural
    // dungeon acquisition. Each namespace starts from an independent arrival Owner.
    for (int source_kind : {1, 2, 3}) {
        auto state = steam_people_fixture();
        for (auto &[key, stock] : state.catalog)
            if (key.first >= 1 && key.first <= 3)
                stock.free_purchases = 0; // Each case isolates one source namespace.
        for (const auto &definition : state.rules->equipment)
            if (definition.shop.kind == source_kind &&
                std::any_of(definition.shop.combat.begin(), definition.shop.combat.end(),
                            [](int value) { return value > 0; })) {
                auto &stock = state.catalog.at({source_kind, definition.shop.id});
                stock.status = stock.free_purchases = 1;
            }
        ark::test::SteamVillageStrategy strategy;
        std::vector<ark::test::CampaignReceipt> receipts;
        const auto initial = sim::startup_world_human_details(state, 1);
        std::cout << "PEOPLE_FIXTURE kind=" << source_kind << " equipment=";
        for (const auto &entry : initial->equipment)
            std::cout << entry.value_or(-1) << ',';
        std::cout << std::endl;
        const Receipt equipped = source_kind == 1   ? Receipt::weapon_reward_equipped
                                 : source_kind == 2 ? Receipt::armour_reward_equipped
                                                    : Receipt::accessory_reward_equipped;
        const Receipt combat = source_kind == 1   ? Receipt::weapon_combat_applied
                               : source_kind == 2 ? Receipt::armour_combat_applied
                                                  : Receipt::accessory_combat_applied;
        run_steam_receipt_fixture(
            apply, state, strategy, [&] { return receipts.size() == 2; },
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
                    paid.before[0] == paid.after[0] + 1 && paid.before[1] == paid.after[1] &&
                    paid.after[3] == paid.definition &&
                    profile->equipment.at(paid.after[2]) == paid.definition &&
                    std::equal(effect.after.begin(), effect.after.end(), profile->combat.begin()),
                "Source equipment consumption, actor binding and attributes were not independently "
                "recorded");
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
    steam_transaction_receipt_contract(apply);
    steam_people_receipt_contract(apply);
    steam_investment_receipt_contract(apply);
}
} // namespace ark::test
