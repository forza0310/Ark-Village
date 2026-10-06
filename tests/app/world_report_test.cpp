// Automatic display and diagnostic skips reuse the real fee/snapshot/close transaction. Defeat
// and visitor values below are explicit contract fixtures, not a claimed natural game trajectory.
#include "ark/app/world_report.hpp"
#include "support/checks.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>

namespace {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace rules = sim::rules;
using State = sim::StartupWorldRuntimeState;
State prepared_report(ark::test::Checks &check) {
    auto state = ark::test::initial_world();
    state.scene.calendar.month_ticks = state.clock_parameter * 20 - 143;
    const auto monster =
        std::find_if(state.rules->monsters.begin(), state.rules->monsters.end(),
                     [](const auto &value) { return value.points_per_defeat > 0; });
    check(monster != state.rules->monsters.end(), "Source defines a point-bearing monster");
    state.scene.world.world.ai.monster_growth.at(monster->identity).defeats = 3;
    rules::BattleActorRecord actor;
    actor.id = {81};
    actor.definition = state.rules->humans.front().identity;
    actor.position = {155, 3, 255};
    actor.state_counter = 101;
    actor.control.state = 14;
    state.scene.world.world.ai.battle.actors.emplace(actor.id, actor);
    state.scene.world.world.ai.human_order.push_back(actor.id);
    const auto adapter = sim::startup_world_runtime_adapter();
    const auto input = adapter.report_input(state);
    check(input.has_value(), "Source projects actual monthly report input");
    const auto before_cash = state.scene.world.world.ai.accounting.funds();
    const auto before_points = state.village_points;
    const auto result = rules::prepare_world_month_report(adapter.report.read(state), *input);
    check(result.candidate && adapter.report.write(state, result.candidate->state),
          "Actual source report prepares and commits maintenance fees plus immutable totals");
    check(state.report_state == 1 && state.report_counter == 0 && state.report_snapshot[0] == 3 &&
              state.report_snapshot[1] == 3 * monster->points_per_defeat &&
              state.scene.world.world.ai.accounting.funds() < before_cash &&
              state.village_points == before_points,
          "Fixture begins after fee payment and before report point redemption");
    return state;
}
void unchanged_world(ark::test::Checks &check, const State &state, const State &before) {
    check(ark::test::same_world_clock(state, before) &&
              state.scene.world.world.map.cells.size() == before.scene.world.world.map.cells.size(),
          "Report input never advances world, calendar, arrival or map ownership");
    const auto &actor = state.scene.world.world.ai.battle.actors.at({81});
    const auto &old_actor = before.scene.world.world.ai.battle.actors.at({81});
    check(actor.state_counter == old_actor.state_counter &&
              actor.control.state == old_actor.control.state &&
              actor.position.x == old_actor.position.x &&
              actor.position.z == old_actor.position.z &&
              actor.position.height == old_actor.position.height &&
              state.scene.world.world.ai.human_order == before.scene.world.world.ai.human_order,
          "Report input does not drive or recreate the current visitor");
    check(state.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds() &&
              state.monthly_cash == before.monthly_cash &&
              state.facility_monthly_cash == before.facility_monthly_cash &&
              state.report_snapshot == before.report_snapshot &&
              state.report_records == before.report_records &&
              state.report_new_records == before.report_new_records &&
              state.maximum_income == before.maximum_income &&
              state.scripts.notices.size() == before.scripts.notices.size(),
          "Report phases neither repeat maintenance/notice nor recalculate or repay the snapshot");
    check(state.scene.random.draws() == before.scene.random.draws(),
          "Confirmation does not consume shared random input");
    auto random = state.scene.random, old_random = before.scene.random;
    for (int i = 0; i < 5; ++i)
        check(random.draw(103).ticket == old_random.draw(103).ticket,
              "Report input preserves the future shared random sequence");
}
void automatic_transitions(ark::test::Checks &check) {
    auto state = prepared_report(check);
    const auto before = state;
    const auto adapter = sim::startup_world_runtime_adapter();
    for (int tick = 1; tick <= 140; ++tick) {
        auto input = adapter.report_input(state);
        check(input && input->admitted && !input->skip_display,
              "Automatic report uses source-admitted input without a synthesized confirmation");
        const auto result = rules::prepare_world_month_report(adapter.report.read(state), *input);
        check(result.candidate && adapter.report.write(state, result.candidate->state),
              "Each admitted automatic display step commits through the actual Owner adapter");
        const int phase = tick < 70 ? 1 : tick < 140 ? 2 : 0;
        check(state.report_state == phase && state.report_counter == tick % 70 &&
                  state.village_points ==
                      (tick < 140
                           ? before.village_points
                           : std::min(999, before.village_points + before.report_snapshot[1])),
              "Source 70/70 display ordering settles positive points only when phase2 closes");
        if (tick == 70 || tick == 140)
            unchanged_world(check, state, before);
    }
    // Move away from the phase0 fee trigger as the ordinary world consumer does. A closed
    // report must not repay its retained snapshot on subsequent automatic updates.
    const auto closed = state;
    auto input = adapter.report_input(state);
    ++input->old_month_tick;
    const auto result = rules::prepare_world_month_report(adapter.report.read(state), *input);
    check(result.candidate && adapter.report.write(state, result.candidate->state) &&
              state.report_state == 0 && state.village_points == closed.village_points,
          "A later automatic step cannot redeem the same retained report snapshot twice");
    unchanged_world(check, state, closed);
}
void diagnostic_skips(ark::test::Checks &check) {
    auto state = prepared_report(check);
    const auto before = state;
    check(app::world_report_visible(state), "Prepared report is a visible automatic overlay");
    check(app::acknowledge_world_report(state, 1) && state.report_state == 2 &&
              state.report_counter == 0 && state.village_points == before.village_points,
          "Diagnostic skip advances from defeats to financial results without early points");
    unchanged_world(check, state, before);
    const auto phase2 = state;
    check(!app::acknowledge_world_report(state, 1) && state.report_state == phase2.report_state &&
              state.report_counter == phase2.report_counter,
          "A repeated stale first-phase click cannot close the financial report");
    unchanged_world(check, state, phase2);
    check(app::acknowledge_world_report(state, 2) && state.report_state == 0 &&
              state.report_counter == 0 && !app::world_report_waiting(state) &&
              state.village_points ==
                  std::min(999, before.village_points + before.report_snapshot[1]),
          "Second diagnostic skip closes and awards actual snapshot points exactly once");
    unchanged_world(check, state, before);
    const auto closed = state;
    for (int phase : {0, 1, 2, 3, 4})
        check(!app::acknowledge_world_report(state, phase) &&
                  state.village_points == closed.village_points && state.report_state == 0,
              "Closed report rejects all repeated confirmations, even at the original fee tick");
    unchanged_world(check, state, closed);
}
void gates(ark::test::Checks &check) {
    auto state = prepared_report(check);
    const auto before = state;
    state.scene.framework_paused = true;
    check(!app::acknowledge_world_report(state, 1) && state.report_state == 1 &&
              state.report_counter == 0 && state.scene.framework_paused,
          "Diagnostic skip cannot override the user's explicit pause or auto-resume the world");
    state.scene.framework_paused = false;
    for (const int mode : {1, 3, 4, 5, 6, 7}) {
        state.scene.scene_state = mode;
        check(app::world_report_visible(state) && !app::acknowledge_world_report(state, 1) &&
                  state.report_state == 1 && state.report_counter == 0,
              "A visible overlay cannot bypass source non-world camera/build/wait update "
              "eligibility");
    }
    state.scene.scene_state = before.scene.scene_state;
    for (int phase : {-1, 0, 2, 3, 4})
        check(!app::acknowledge_world_report(state, phase) && state.report_state == 1,
              "Only the currently displayed source report phase is accepted");
    rules::WorldScriptPage modal;
    modal.id = 999;
    modal.kind = rules::WorldScriptPageKind::dialogue;
    modal.lifecycle = 1;
    state.scripts.pages.push_back(modal);
    check(!app::world_report_waiting(state) && !app::acknowledge_world_report(state, 1),
          "A script modal takes input precedence over an underlying report");
    state.scripts.pages.back().lifecycle = 4;
    check(app::world_report_waiting(state), "Closed modal does not obscure the main-scene report");
    state.scripts.pages.clear();
    check(!app::world_report_waiting(state) && !app::acknowledge_world_report(state, 1),
          "Missing scene root cannot be treated as an exposed report");
    state = before;
    state.report_state = 3; // Explicit existing-special-phase fixture; does not invent its entry.
    state.village_points = 999;
    check(app::acknowledge_world_report(state, 3) && state.report_state == 0 &&
              state.village_points == 999,
          "Existing phase3 closes through the source and retains its point cap");
    unchanged_world(check, state, before);
    state = before;
    state.report_counter = -1;
    check(!app::acknowledge_world_report(state, 1) && state.report_state == 1 &&
              state.report_counter == -1 && state.village_points == before.village_points,
          "Rejected source candidate leaves the original report owner unchanged");
    unchanged_world(check, state, before);
}
} // namespace
namespace ark::test {
void world_report() {
    Checks check{"world_report"};
    automatic_transitions(check);
    diagnostic_skips(check);
    gates(check);
    std::cout << "PASS automatic world report " << check.count() << " checks\n";
}
} // namespace ark::test
