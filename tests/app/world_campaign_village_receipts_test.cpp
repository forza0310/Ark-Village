// Village page/effect binding has its own fixtures; arithmetic remains in frozen rules.
// Reuse the campaign executable instead of extending the existing mixed 1,000-line fixture file.
#include "../support/world_fixture.hpp"
#include "ark/simulation/village/startup_world_village_activity.hpp"
#include "world_campaign_coverage.hpp"
#include "world_campaign_receipts_test.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace ark::test {
namespace {
namespace app = ark::app;
namespace sim = simulation;
namespace ref = sim::rules;
using State = app::WorldState;
void need(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(std::string("Village receipt: ") + message);
}
const ref::WorldScriptPage &top(const State &s) {
    const auto page = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    need(page != s.scripts.pages.rend(), "missing source page");
    return *page;
}
void applied_effect(const CampaignCommandConsumer &apply, int activity) {
    // Explicit menu/presence qualification, not claimed natural unlocking or arrival.
    auto state = initial_world();
    state.village_points = state.rules->activities.at(activity).parameters[4];
    state.quarter_counter = 1;
    for (auto &[id, definition] : state.scripts.activities)
        definition.status = id == activity ? 1 : 0;
    for (auto &[id, present] : state.human_presence)
        present = id < 2 ? 1 : 0;
    state.shop_humans.at(0).satisfaction = 20;
    state.shop_humans.at(1).satisfaction = 100;
    std::optional<State> ready, completed;
    bool early_confirmed{};
    for (int step = 0; step < 300 && !completed; ++step) {
        app::WorldCommand command;
        command.page = top(state).id;
        bool send{};
        if (top(state).kind == ref::WorldScriptPageKind::scene) {
            command.kind = app::WorldCommandKind::open_village_activities;
            send = true;
        } else if (top(state).kind == ref::WorldScriptPageKind::dialogue) {
            command.kind = app::WorldCommandKind::acknowledge_page;
            send = true;
        } else if (const auto view =
                       sim::inspect_startup_world_village_activity_page(state, command.page)) {
            command.kind = app::WorldCommandKind::village_activity_action;
            command.village_activity_action = sim::StartupVillageActivityAction::confirm;
            send = view->raw != 53 || view->counter >= 120 || !early_confirmed;
            if (view->raw == 53 && view->counter < 120)
                early_confirmed = true;
            if (view->raw == 53 && view->counter >= 120) {
                // Live, retired and focused instances must get capacity only; six HP slots stay
                // intact.
                ref::BattleActorRecord actor;
                actor.id = {8888};
                actor.definition = 0;
                actor.capacity = state.scene.world.world.ai.growth.at(0).derived.combat[0];
                actor.hp = {15, 30, 30, 45, true, 2};
                state.scene.world.world.ai.battle.actors.emplace(actor.id, actor);
                actor.id = {8889};
                state.scene.world.world.ai.retired_actors.emplace(actor.id, actor);
                state.focus_actor.actor.hp = actor.hp;
                ready = state;
            }
        }
        if (send) {
            const auto before = state;
            const auto outcome = apply(state, command);
            need(outcome.outcome == app::WorldCommandOutcome::applied,
                 "actual activity command rejected");
            if (ready)
                completed = state;
            else
                need(activity_human_effect_receipts(before, state).empty(),
                     "payment or early confirmation certified an effect");
        }
        if (!completed) {
            auto update = sim::prepare_startup_world_runtime(state);
            need(update.candidate && sim::update_startup_world_render_cache(*update.candidate),
                 "activity fixture update rejected");
            state = std::move(*update.candidate);
        }
    }
    need(ready && completed && early_confirmed, "actual raw53 completion was not reached");
    const auto before_digest = sim::startup_world_state_digest(*ready),
               after_digest = sim::startup_world_state_digest(*completed);
    const auto receipts = activity_human_effect_receipts(*ready, *completed);
    if (state.rules->activities.at(activity).parameters[2] == 2) {
        const auto queued = activity_popularity_queue_receipt(*ready, *completed);
        need(receipts.empty() && queued && queued->definition == activity &&
                 queued->before[0] == 1 && queued->after[0] == 0 &&
                 queued->before[1] == queued->after[1] && queued->after[3] == 40 &&
                 !activity_popularity_queue_receipt(*completed, *completed),
             "type2 completion must queue40 popularity, not claim human/applied effects");
        CampaignCoverage coverage;
        std::ostringstream log;
        coverage.record(*queued, log);
        need(log.str().find("\tpopularity_queued\t") != std::string::npos,
             "delayed popularity request lost its separate queue-only stage");
        for (int fault = 0; fault < 4; ++fault) {
            auto old = *ready, bad = *completed;
            if (fault == 0) ++bad.scene.world.popularity_queue.front()[1];
            if (fault == 1) ++bad.quarter_counter;
            if (fault == 2) ++bad.popularity;
            if (fault == 3) old.page_counters.at(top(old).id) = 119;
            bool refused{};
            try { (void)activity_popularity_queue_receipt(old, bad); }
            catch (const std::exception &) { refused = true; }
            need(refused, "corrupt/early delayed activity completion was certified");
        }
        return;
    }
    need(receipts.size() == 2 && receipts[0].recipient == 0 && receipts[1].recipient == 1,
         "effect did not bind all open definitions in source order");
    need(before_digest == sim::startup_world_state_digest(*ready) &&
             after_digest == sim::startup_world_state_digest(*completed),
         "effect inspection mutated an Owner");
    if (activity == 0)
        need(receipts[0].before[0] == 20 && receipts[0].after[0] == 22 &&
                 receipts[1].after[0] == 100,
             "real type0 effect or saturation differed from fixed source values");
    else {
        need(receipts[0].kind == CampaignReceiptKind::activity_attribute_applied,
             "type1 effect was labelled satisfaction");
        if (activity == 4)
            need(completed->scene.world.world.ai.battle.actors.at({8888}).capacity >
                         ready->scene.world.world.ai.battle.actors.at({8888}).capacity &&
                     completed->scene.world.world.ai.battle.actors.at({8888}).hp.target == 45,
                 "HP capacity improvement was mistaken for current HP recovery");
    }
    need(ready->scene.world.world.ai.growth.at(2).definition.extra ==
                 completed->scene.world.world.ai.growth.at(2).definition.extra &&
             ready->shop_humans.at(2).satisfaction == completed->shop_humans.at(2).satisfaction,
         "absent definition received an activity effect");
    CampaignCoverage coverage;
    std::ostringstream log;
    for (const auto &receipt : receipts)
        coverage.record(receipt, log);
    need(log.str().find(activity == 0 ? "\tsatisfaction_applied\t"
                                      : "\tattribute_applied_to_human\t") != std::string::npos,
         "verified applied effect lost its separate wire stage");
    for (int fault = 0; fault < 7; ++fault) {
        auto old = *ready, bad = *completed;
        if (fault == 0)
            ++bad.scene.world.world.ai.battle.actors.at({8888}).hp.target;
        if (fault == 1)
            ++bad.human_activity_previous.at(1);
        if (fault == 2)
            ++bad.scene.world.world.ai.growth.at(1).definition.extra[0];
        if (fault == 3)
            ++bad.quarter_counter;
        if (fault == 4)
            bad.activity_page_bindings.erase(top(bad).id);
        if (fault == 5)
            old.page_counters.at(top(old).id) = 119;
        if (fault == 6)
            ++bad.focus_actor.actor.hp.target;
        bool refused{};
        try {
            activity_human_effect_receipts(old, bad);
        } catch (const std::exception &) {
            refused = true;
        }
        need(refused, "corrupt or late human effect was accepted");
    }
    auto display = *completed;
    // The HP probes above are isolated effect instances, not complete AI roster entries.
    // Use raw54's source page consumer to initialize its display before transport input;
    // a full-world update would incorrectly require routing metadata for these probes.
    auto initialized = sim::update_startup_world_village_activity_page(display, top(display).id);
    need(initialized.has_value(),
         "raw54 first source update rejected");
    display = std::move(*initialized);
    app::WorldCommand close;
    close.kind = app::WorldCommandKind::village_activity_action;
    close.page = top(display).id;
    close.village_activity_action = sim::StartupVillageActivityAction::confirm;
    need(apply(display, close).outcome == app::WorldCommandOutcome::applied &&
             activity_human_effect_receipts(*completed, display).empty() &&
             display.scene.world.world.ai.growth.at(0).definition.extra ==
                 completed->scene.world.world.ai.growth.at(0).definition.extra,
         "raw54 repeated an effect or an applied receipt");
}
} // namespace
void campaign_village_effect_contract(const CampaignCommandConsumer &apply) {
    for (const int activity : {0, 4, 7, 19})
        applied_effect(apply, activity);
}
} // namespace ark::test
