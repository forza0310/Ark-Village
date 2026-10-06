#include "ark/app/world_save.hpp"
#include "ark/simulation/startup_world_building.hpp"
#include "ark/simulation/startup_world_runtime_tasks.hpp"
#include "support/world_fixture.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace ref = sim::rules;
using State = sim::StartupWorldRuntimeState;
int checks{};
void check(bool value, const std::string &message) {
    ++checks;
    if (!value)
        throw std::runtime_error("save restore: " + message);
}
void advance(State &s) {
    const auto result = sim::prepare_startup_world_runtime(s);
    check(result.candidate.has_value(), "real source world update");
    s = *result.candidate;
    check(sim::update_startup_world_render_cache(s), "real source render cache update");
    s.sound_requests.clear();
    const auto raw = s.scripts.pages.back().legacy_page;
    if (s.scripts.pages.back().kind == ref::WorldScriptPageKind::dialogue ||
        s.scripts.pages.back().kind == ref::WorldScriptPageKind::simple_message ||
        s.scripts.pages.back().kind == ref::WorldScriptPageKind::newspaper ||
        (s.scripts.pages.back().kind == ref::WorldScriptPageKind::raw_page &&
         (raw == 11 || raw == 59 || raw == 67 || raw == 88 || raw == 96))) {
        const auto page = s.scripts.pages.back();
        // This is explicit diagnostic input, only for pages naturally created by the source.
        const auto error = sim::acknowledge_startup_world_runtime_page(s, page.id);
        check(error == sim::StartupWorldRuntimeError::none ||
                  error == sim::StartupWorldRuntimeError::invalid_page,
              "source acknowledgement or readiness rejection page=" +
                  std::to_string(page.legacy_page) +
                  " error=" + std::to_string(static_cast<int>(error)));
    }
}
State restored(const State &saved) {
    const auto image = app::capture_world_save(saved);
    check(image.image.has_value(), image.message);
    const auto decoded = app::decode_world_save(image.image->bytes);
    check(decoded.state.has_value(), decoded.message);
    auto candidate = *decoded.state;
    std::string reason;
    check(app::prepare_world_save_candidate(candidate, saved, reason) == app::WorldSaveError::none,
          reason);
    return candidate;
}
void discard_nonpersistent_effects(State &baseline) {
    for (auto &context : baseline.scene.world.world.ai.contexts)
        context.second.effects = {};
    baseline.sound_requests.clear();
    baseline.scripts.notices.clear();
    baseline.visual_effects.clear();
    baseline.delayed_effects.clear();
    baseline.global_effects.clear();
    baseline.scene.world.hints.clear();
    baseline.scene.world.floating_notes.clear();
    baseline.exploration_displays.clear();
    baseline.dungeon_labels.clear();
}
void same_durable(const State &a, const State &b, const char *scenario) {
    check(ark::test::same_world_clock(a, b), std::string(scenario) + " source clocks");
    check(a.scene.random.draws() == b.scene.random.draws(),
          std::string(scenario) + " controlled random consumption");
    const auto left = app::capture_world_save(a);
    const auto right = app::capture_world_save(b);
    check(left.image.has_value(), left.message);
    check(right.image.has_value(), right.message);
    check(left.image->bytes == right.image->bytes,
          std::string(scenario) + " durable field equality");
}
void invalid_candidates() {
    auto initial = ark::test::initial_world();
    std::string reason;
    check(app::validate_world_save_candidate(initial, reason) == app::WorldSaveError::none, reason);
    const auto reject = [&](const State &bad, const char *scenario) {
        auto candidate = bad;
        const auto cash = candidate.scene.world.world.ai.accounting.funds();
        const auto draws = candidate.scene.random.draws();
        const auto steps = candidate.simulation_steps;
        check(app::prepare_world_save_candidate(candidate, initial, reason) !=
                  app::WorldSaveError::none,
              std::string("reject ") + scenario);
        check(candidate.scene.world.world.ai.accounting.funds() == cash &&
                  candidate.scene.random.draws() == draws && candidate.simulation_steps == steps,
              std::string("no partial restore for ") + scenario);
    };
    auto bad = initial;
    bad.scene.world.facility_order.push_back(bad.scene.world.facility_order.front());
    reject(bad, "duplicate facility order identity");
    bad = initial;
    bad.scene.world.world.facilities.begin()->second.placement.instance_id.value += 100;
    reject(bad, "mismatched facility stable identity");
    bad = initial;
    bad.scene.world.world.facilities.begin()->second.occupants.push_back({999999});
    reject(bad, "dangling facility occupant");
    bad = initial;
    const auto first = bad.scene.world.world.facilities.begin()->second.placement.anchor;
    bad.scene.world.world.map.cells
        .at(static_cast<std::size_t>(first.y) * bad.scene.world.world.map.width + first.x)
        .facility.reset();
    reject(bad, "missing occupied tile binding");
    bad = initial;
    bad.scene.calendar.units = 10800;
    reject(bad, "unnormalized week clock");
    bad = initial;
    bad.scene.calendar.year = std::numeric_limits<int>::max();
    reject(bad, "year display overflow");
    bad = initial;
    bad.camera[0] = std::numeric_limits<float>::infinity();
    reject(bad, "nonfinite projection coordinate");
    bad = initial;
    bad.scene.world.world.ai.growth.begin()->second.definition.current_profession = 99999;
    reject(bad, "unknown current profession");
    bad = initial;
    bad.shop_humans.begin()->second.equipment[0] = 99999;
    reject(bad, "unknown equipment");
    bad = initial;
    ref::WorldScriptContinuation invalid_continuation;
    invalid_continuation.event = 99999;
    invalid_continuation.remaining_updates = 1;
    bad.scripts.continuations.push_back(invalid_continuation);
    reject(bad, "unknown delayed script identity");
    bad = initial;
    bad.scripts.facilities.erase(bad.scripts.facilities.begin());
    reject(bad, "incomplete shared facility directory");
    bad = initial;
    bad.scene.world.world.human_spending.erase(bad.scene.world.world.human_spending.begin());
    reject(bad, "incomplete shared human spending");
    bad = initial;
    bad.task_progress.definitions.erase(bad.task_progress.definitions.begin());
    reject(bad, "incomplete shared task definitions");
    bad = initial;
    bad.task_progress.definitions.begin()->second.kind = 999;
    reject(bad, "unknown shared task kind");
    bad = initial;
    bad.catalog.emplace(std::pair<int, int>{1, 99999}, ref::ObjectCatalogRecord{});
    bad.shop_humans.begin()->second.equipment[0] = 99999;
    reject(bad, "fabricated equipment catalog key");
    bad = initial;
    bad.scene.world.town.right = bad.scene.world.town.left;
    reject(bad, "empty town bounds");
    bad = initial;
    bad.scene.world.spawn_cells.clear();
    reject(bad, "empty arrival spawn cells");
    bad = initial;
    bad.scene.world.updates = std::numeric_limits<int>::max();
    reject(bad, "world update counter overflow");
    bad = initial;
    bad.dungeon_facilities.begin()->second.updates = std::numeric_limits<int>::max();
    reject(bad, "facility update counter overflow");
    bad = initial;
    bad.shop_item_stock.begin()->second.definition = 99999;
    reject(bad, "mismatched shop item value identity");
}
void task_and_combat_roundtrip(const State &natural) {
    auto task = natural;
    const auto generated =
        ref::prepare_world_task_creation(sim::startup_world_runtime_factory(task), 0);
    check(generated.candidate && generated.candidate->created_task &&
              sim::write_startup_world_runtime_factory(task, generated.candidate->state),
          "task fixture calls the actual source factory without injecting people or funds");
    const auto identity = *generated.candidate->created_task;
    check(sim::open_startup_world_runtime_task_menu(task) == sim::StartupWorldRuntimeError::none,
          "source task menu opens");
    const auto menu = task.scripts.pages.back().id;
    const auto &list = task.task_page_lists.at(menu);
    const auto selected = std::find(list.begin(), list.end(), identity);
    check(selected != list.end() && sim::act_startup_world_runtime_task_page(
                                        task, menu, sim::StartupWorldTaskAction::confirm,
                                        static_cast<int>(selected - list.begin()))
                                            .error == sim::StartupWorldRuntimeError::none,
          "source menu selects the generated task identity");
    const auto accepted = sim::act_startup_world_runtime_task_page(
        task, task.scripts.pages.back().id, sim::StartupWorldTaskAction::confirm);
    check(accepted.error == sim::StartupWorldRuntimeError::none && accepted.accepted,
          "source task offer consumes the real recruitment fee once");
    for (int n = 0; n < 1000 && !(task.active_task && app::world_save_eligible(task)); ++n) {
        advance(task);
        const auto page = task.scripts.pages.back();
        if (page.lifecycle != 2)
            continue;
        if (page.legacy_page == 25)
            check(sim::act_startup_world_runtime_task_page(task, page.id,
                                                           sim::StartupWorldTaskAction::depart)
                          .error == sim::StartupWorldRuntimeError::none,
                  "actual recruited team requests departure");
        else if (page.legacy_page == 28 && task.page_phases.at(page.id) == 0)
            check(sim::act_startup_world_runtime_task_page(task, page.id,
                                                           sim::StartupWorldTaskAction::confirm)
                          .error == sim::StartupWorldRuntimeError::none,
                  "actual departure animation is confirmed");
    }
    check(task.active_task == identity && app::world_save_eligible(task),
          "actual recruitment and departure reach a stable active task");
    auto loaded_task = restored(task);
    same_durable(task, loaded_task, "active task capture");
    auto baseline_task = task;
    discard_nonpersistent_effects(baseline_task);
    for (int n = 0; n < 20; ++n) {
        advance(loaded_task);
        advance(baseline_task);
    }
    check(loaded_task.active_task == baseline_task.active_task &&
              loaded_task.participants == baseline_task.participants &&
              loaded_task.scene.world.world.ai.accounting.funds() ==
                  baseline_task.scene.world.world.ai.accounting.funds() &&
              loaded_task.scene.random.draws() == baseline_task.scene.random.draws() &&
              ark::test::same_world_clock(loaded_task, baseline_task),
          "loaded active task does not repeat recruitment or lose actual crew");

    // Source encounter call-point fixture, matching the maintained runtime composition case.
    ref::EncounterCreationInput input;
    input.kind = 0;
    input.center = {10, 18};
    input.year_index = 0;
    input.month_index = 3;
    const auto created = sim::prepare_startup_world_runtime_encounter(natural, input);
    check(created && !created->scene.world.world.ai.monster_order.empty(),
          "actual encounter consumer creates real monster instances and caches");
    auto combat = *created;
    auto &ai = combat.scene.world.world.ai;
    const auto human = ai.human_order.front();
    const auto monster = ai.monster_order.back();
    const auto encounter = *ai.battle.actors.at(monster).encounter;
    const auto joined = ref::prepare_battle_group_join(ai, encounter, human, monster);
    check(joined.candidate.has_value(), "source battle group binds real opposing actors");
    ai = joined.candidate->state;
    const auto projectile = ref::prepare_projectile(ref::ProjectileKind::arrow, human, monster,
                                                    ai.battle.actors.at(human).position,
                                                    ai.battle.actors.at(monster).position, 0);
    check(projectile.candidate.has_value(),
          "source projectile constructor binds real caster and target");
    const auto projectile_id = ai.next_projectile_id++;
    ai.projectiles.emplace(projectile_id, *projectile.candidate);
    ai.projectile_order.push_back(projectile_id);
    check(sim::update_startup_world_render_cache(combat),
          "explicit encounter fixture includes the real transaction-tail render cache");
    auto loaded_combat = restored(combat);
    same_durable(combat, loaded_combat, "combat group and projectile capture");
    auto baseline_combat = combat;
    discard_nonpersistent_effects(baseline_combat);
    advance(loaded_combat);
    advance(baseline_combat);
    check(loaded_combat.scene.world.world.ai.projectile_order ==
                  baseline_combat.scene.world.world.ai.projectile_order &&
              loaded_combat.scene.world.world.ai.accounting.funds() ==
                  baseline_combat.scene.world.world.ai.accounting.funds() &&
              loaded_combat.scene.random.draws() == baseline_combat.scene.random.draws() &&
              ark::test::same_world_clock(loaded_combat, baseline_combat),
          "combat/projectile next update uses restored references without extra rewards");
    auto invalid = combat;
    invalid.scene.world.world.ai.encounters.at(encounter).group.cycle = 3;
    std::string reason;
    check(app::validate_world_save_candidate(invalid, reason) == app::WorldSaveError::invalid_world,
          "invalid battle group cycle rejected before next update");
    invalid = combat;
    invalid.actor_metadata.erase(monster);
    check(app::validate_world_save_candidate(invalid, reason) == app::WorldSaveError::invalid_world,
          "projectile target without required runtime cache rejected");
}
void natural_operation_roundtrip() {
    auto state = ark::test::initial_world();
    bool service{}, walking{}, construction{}, captured_service{}, continuation{};
    for (int update = 0; update < 1200; ++update) {
        advance(state);
        if (!app::world_save_eligible(state))
            continue;
        if (!continuation && !state.scripts.continuations.empty()) {
            const auto copy = restored(state);
            same_durable(state, copy, "natural delayed script capture");
            auto waiting = copy;
            auto uninterrupted = state;
            discard_nonpersistent_effects(uninterrupted);
            const auto event = state.scripts.continuations.front().event;
            const auto calls = state.scripts.event_calls.at(event);
            for (int remaining = state.scripts.continuations.front().remaining_updates + 2;
                 remaining > 0; --remaining) {
                advance(waiting);
                advance(uninterrupted);
            }
            check(waiting.scripts.event_calls.at(event) == calls &&
                      waiting.scripts.continuations.empty() &&
                      uninterrupted.scripts.continuations.empty() &&
                      waiting.scene.world.world.ai.accounting.funds() ==
                          uninterrupted.scene.world.world.ai.accounting.funds() &&
                      waiting.scene.random.draws() == uninterrupted.scene.random.draws() &&
                      ark::test::same_world_clock(waiting, uninterrupted),
                  "actual first-arrival delayed script resumes without a second event call");
            continuation = true;
        }
        if (!construction && !state.scene.world.world.ai.human_order.empty()) {
            const auto selected = sim::begin_startup_world_build(state, 30);
            check(selected.denial == sim::StartupBuildDenial::none, "real construction selection");
            bool built{};
            for (int y = 0; y < state.scene.world.world.map.height && !built; ++y)
                for (int x = 0; x < state.scene.world.world.map.width && !built; ++x) {
                    auto candidate = state;
                    const auto result = sim::confirm_startup_world_build(
                        candidate, {x, y}, ref::FacilityOrientation::first);
                    if (result.denial == sim::StartupBuildDenial::none && result.created) {
                        state = std::move(candidate);
                        sim::cancel_startup_world_build(state);
                        construction = true;
                        built = true;
                        const auto copy = restored(state);
                        same_durable(state, copy, "construction capture");
                        check(copy.scene.world.world.ai.accounting.funds() ==
                                  state.scene.world.world.ai.accounting.funds(),
                              "construction save does not charge again");
                    }
                }
            check(built, "natural construction finds legal actual placement");
        }
        for (const auto id : state.scene.world.world.ai.human_order) {
            const auto &actor = state.scene.world.world.ai.battle.actors.at(id);
            walking |= actor.control.action == 1;
            if (actor.control.state != 14 || captured_service)
                continue;
            auto loaded = restored(state);
            same_durable(state, loaded, "inn service capture");
            const auto &before = actor.hp;
            const auto &after = loaded.scene.world.world.ai.battle.actors.at(id).hp;
            check(before.requested_delta == after.requested_delta &&
                      before.displayed == after.displayed && before.origin == after.origin &&
                      before.target == after.target && before.animating == after.animating &&
                      before.legacy_tick == after.legacy_tick,
                  "inn service keeps all six HP fields without recovery");
            check(loaded.scene.world.world.facilities.size() ==
                          state.scene.world.world.facilities.size() &&
                      loaded.scene.world.world.actors.at(id).binding->instance_id ==
                          state.scene.world.world.actors.at(id).binding->instance_id,
                  "inn binding and facilities round-trip");
            service = captured_service = true;
        }
    }
    check(walking && service && construction && continuation,
          "real walking, charged inn service, construction and delayed script observed");
    for (int n = 0; n < 600 && !app::world_save_eligible(state); ++n)
        advance(state);
    check(app::world_save_eligible(state),
          "stable source capture found within the diagnostic budget");
    task_and_combat_roundtrip(state);
    auto loaded = restored(state);
    auto baseline = state;
    std::string reason;
    // Clear only the published nonpersistent effects in the independent reference side.
    // Its durable fields and map caches do not go through the restoration implementation.
    discard_nonpersistent_effects(baseline);
    same_durable(loaded, baseline, "natural restore");
    for (int n = 0; n < 80; ++n) {
        advance(loaded);
        advance(baseline);
    }
    if (app::world_save_eligible(loaded) && app::world_save_eligible(baseline))
        same_durable(loaded, baseline, "natural continuation");
    else
        check(loaded.scene.world.world.ai.accounting.funds() ==
                      baseline.scene.world.world.ai.accounting.funds() &&
                  ark::test::same_world_clock(loaded, baseline) &&
                  loaded.scene.random.draws() == baseline.scene.random.draws(),
              "natural continuation retains source modal boundary and financial outcome");
    auto departing = state;
    const auto id = departing.scene.world.world.ai.human_order.front();
    departing.scene.world.world.ai.battle.actors.at(id).control.flags |= 512U;
    check(!app::world_save_eligible(departing), "unsupported exit state is explicitly ineligible");
    auto bad = state;
    bad.scene.world.world.ai.battle.actors.at(id).rescue = ref::CharacterId{999999};
    check(app::validate_world_save_candidate(bad, reason) == app::WorldSaveError::invalid_world,
          "dangling real actor rescue reference rejected");
    bad = state;
    bad.scene.world.world.ai.battle.actors.at(id).control.queue.push_back({14});
    bad.scene.world.world.ai.battle.actors.at(id).control.queue.push_back({99999});
    check(app::validate_world_save_candidate(bad, reason) == app::WorldSaveError::invalid_world,
          "unknown control opcode rejected without executing the valid prefix");
    auto month_boundary = state;
    // Clock-only boundary fixture: all people, money, services and construction remain real.
    month_boundary.scene.calendar.subperiod = 3;
    month_boundary.scene.calendar.units = 10773;
    auto month_loaded = restored(month_boundary);
    auto month_baseline = month_boundary;
    discard_nonpersistent_effects(month_baseline);
    const auto previous_month = month_boundary.scene.calendar.month;
    advance(month_loaded);
    advance(month_baseline);
    check(month_loaded.scene.calendar.month == (previous_month + 1) % 12 &&
              month_loaded.scene.world.world.ai.accounting.funds() ==
                  month_baseline.scene.world.world.ai.accounting.funds() &&
              month_loaded.monthly_cash == month_baseline.monthly_cash &&
              month_loaded.village_points == month_baseline.village_points &&
              month_loaded.scene.random.draws() == month_baseline.scene.random.draws() &&
              ark::test::same_world_clock(month_loaded, month_baseline),
          "manual restore across month does not replay world prefix, fees or points");
}
} // namespace

void run_restore_tests() {
    invalid_candidates();
    natural_operation_roundtrip();
    std::cout << "PASS world save restore " << checks << " checks\n";
}
