#include "ark/app/world_report.hpp"
#include "ark/simulation/startup_world_runtime_tasks.hpp"
#include "world_session_test_support.hpp"

#include <algorithm>
#include <stdexcept>

namespace ark::test::world_session {
using namespace std::chrono_literals;
const rules::WorldScriptPage &task_top(const app::WorldState &state) {
    const auto p = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                [](const auto &page) { return page.lifecycle != 4; });
    if (p == state.scripts.pages.rend())
        throw std::runtime_error("Task transport fixture lost its source page");
    return *p;
}
std::shared_ptr<const app::WorldFrame> input_frame(app::WorldSession &session,
                                                   std::uint64_t serial) {
    return await(session, [serial](const auto &frame) {
        return frame.last_command_serial >= serial || frame.failed;
    });
}
const app::WorldCommandResult &input_result(const app::WorldFrame &frame, std::uint64_t serial) {
    const auto result = std::find_if(frame.command_results.begin(), frame.command_results.end(),
                                     [serial](const auto &r) { return r.serial == serial; });
    if (result == frame.command_results.end())
        throw std::runtime_error("Task command result was lost behind a later publication");
    return *result;
}
app::WorldState task_listing_fixture() {
    // Explicit task-generation callsite fixture, not a claim that the initial day naturally
    // generated a task. Map, task, fees, catalogue and offer pages use the actual consumers.
    auto state = initial();
    const auto created =
        rules::prepare_world_task_creation(sim::startup_world_runtime_factory(state), 0);
    check(created.candidate && created.candidate->created_task &&
              sim::write_startup_world_runtime_factory(state, created.candidate->state),
          "Transport fixture creates a task with the real current-world factory");
    return state;
}
app::WorldState task_offer_fixture() {
    auto state = task_listing_fixture();
    check(sim::open_startup_world_runtime_task_menu(state) == sim::StartupWorldRuntimeError::none &&
              task_top(state).legacy_page == 22,
          "Source task menu opens its real task list");
    const auto selected = sim::act_startup_world_runtime_task_page(
        state, task_top(state).id, sim::StartupWorldTaskAction::confirm, 0);
    check(selected.error == sim::StartupWorldRuntimeError::none &&
              task_top(state).legacy_page == 23,
          "Source task list selection opens the real offer");
    return state;
}
app::WorldState recruitment_fixture() {
    auto state = task_offer_fixture();
    const auto accepted = sim::act_startup_world_runtime_task_page(
        state, task_top(state).id, sim::StartupWorldTaskAction::confirm);
    check(accepted.error == sim::StartupWorldRuntimeError::none && accepted.accepted &&
              task_top(state).legacy_page == 24,
          "Actual affordable offer prepares the recruitment state");
    return state;
}
// The desktop overlay never enters the source page stack or replaces framework_paused.
void main_menu_gate_and_pause() {
    using Outcome = app::WorldCommandOutcome;
    for (bool paused : {false, true}) {
        const auto state = initial(paused);
        app::WorldSession session(state);
        const auto opened = input_frame(session, session.open_main_menu());
        check(!opened->failed && opened->main_menu_open && opened->outer_updates == 0 &&
                  opened->state->scene.framework_paused == paused,
              "Desktop menu opens above the source scene without changing explicit pause");
        same_world(*opened->state, state);
        check(session.wait_for_frame_after(opened->revision, 120ms) == opened,
              "Menu freezes world, calendar, recruitment and publications across source gates");
        const auto duplicate = session.open_main_menu();
        const auto bypass = session.open_task_menu();
        const auto rejected = input_frame(session, bypass);
        check(!rejected->failed && rejected->main_menu_open && rejected->outer_updates == 0 &&
                  input_result(*rejected, duplicate).outcome == Outcome::rejected &&
                  input_result(*rejected, bypass).outcome == Outcome::rejected,
              "Duplicate open and direct task shortcut reject without bypassing menu or failing");
        same_world(*rejected->state, state);
        const auto closed = input_frame(session, session.close_main_menu());
        check(!closed->failed && !closed->main_menu_open &&
                  closed->state->scene.framework_paused == paused,
              "Menu close restores update eligibility while retaining explicit pause");
        if (paused) {
            check(session.wait_for_frame_after(closed->revision, 120ms) == closed,
                  "Closing a menu opened during pause cannot resume the world");
            const auto stale = input_frame(session, session.close_main_menu());
            check(!stale->failed && !stale->main_menu_open &&
                      input_result(*stale, stale->last_command_serial).outcome == Outcome::rejected,
                  "A stale close is acknowledged as a recoverable rejection");
        } else {
            const auto resumed = await(session, [](const auto &frame) {
                return frame.outer_updates >= 2 || frame.failed;
            });
            check(!resumed->failed && resumed->interval_seconds >= .047,
                  "Menu close resumes source gates without catch-up or a new tick policy");
        }
        session.stop();
    }
}
void main_menu_task_transaction() {
    using Outcome = app::WorldCommandOutcome;
    // Both a real catalogue and event29's empty-catalogue feedback use the source transaction.
    for (bool has_task : {false, true}) {
        auto state = has_task ? task_listing_fixture() : initial();
        state.scene.framework_paused = true;
        app::WorldSession session(state);
        input_frame(session, session.open_main_menu());
        const auto denied = input_frame(session, session.open_menu_tasks());
        check(!denied->failed && denied->main_menu_open &&
                  input_result(*denied, denied->last_command_serial).outcome == Outcome::rejected,
              "Selecting Adventure during explicit pause preserves the desktop menu");
        const auto resume = session.set_paused(false);
        const auto selected = session.open_menu_tasks();
        const auto paused = session.set_paused(true);
        check(selected == resume + 1 && paused == selected + 1,
              "Resume, atomic Adventure selection and pause retain input FIFO ordering");
        const auto result = input_frame(session, paused);
        check(!result->failed && !result->main_menu_open &&
                  input_result(*result, selected).outcome == Outcome::applied &&
                  result->state->scene.framework_paused &&
                  task_top(*result->state).kind != rules::WorldScriptPageKind::scene &&
                  result->state->scene.random.draws() == state.scene.random.draws() &&
                  result->state->simulation_steps == state.simulation_steps &&
                  result->state->scene.world.world.ai.accounting.funds() ==
                      state.scene.world.world.ai.accounting.funds(),
              "One source task transaction closes the overlay without a main-scene update");
        check(has_task ? task_top(*result->state).legacy_page == 22
                       : event_count(*result->state, 29) == 1,
              "Adventure opens the actual list or retains source no-task message feedback");
        const auto stale = input_frame(session, session.open_menu_tasks());
        check(!stale->failed && !stale->main_menu_open &&
                  input_result(*stale, stale->last_command_serial).outcome == Outcome::rejected,
              "Stale Adventure selection cannot open a second page or fail the session");
        session.stop();
    }
    auto active = task_listing_fixture();
    active.scene.framework_paused = true;
    active.active_task = active.task_order.front(); // Explicit active-task entry fixture.
    app::WorldSession session(active);
    input_frame(session, session.open_main_menu());
    session.set_paused(false);
    const auto management = session.open_menu_tasks();
    const auto result = input_frame(session, session.set_paused(true));
    check(!result->failed && !result->main_menu_open && result->outer_updates == 0 &&
              result->state->active_task == active.active_task &&
              task_top(*result->state).legacy_page == 26 &&
              input_result(*result, management).outcome == Outcome::applied,
          "An active task opens its source management page in one menu transaction");
    session.stop();
}
void main_menu_modal_rejections() {
    using Outcome = app::WorldCommandOutcome;
    for (bool report : {false, true}) {
        auto state = report ? initial(true) : task_offer_fixture();
        state.scene.framework_paused = true;
        if (report)
            state.report_state = 1;
        app::WorldSession session(state);
        const auto result = input_frame(session, session.open_main_menu());
        check(!result->failed && !result->main_menu_open && result->outer_updates == 0 &&
                  input_result(*result, result->last_command_serial).outcome == Outcome::rejected,
              "Monthly report and a source modal page reject desktop menu opening recoverably");
        same_world(*result->state, state);
        session.stop();
    }
    auto missing = initial(true);
    missing.rules = nullptr;
    app::WorldSession failed(missing);
    const auto result = input_frame(failed, failed.open_main_menu());
    check(result->failed && !result->main_menu_open &&
              input_result(*result, result->last_command_serial).runtime_error ==
                  sim::StartupWorldRuntimeError::missing_source,
          "Missing source remains fatal; the recoverable menu policy cannot hide broken runtime");
    failed.stop();
}
void task_inputs_and_denials() {
    using Action = sim::StartupWorldTaskAction;
    using Outcome = app::WorldCommandOutcome;
    const auto offer = task_offer_fixture();
    const auto page = task_top(offer).id;
    auto expected = offer;
    const auto source = sim::act_startup_world_runtime_task_page(expected, page, Action::confirm);
    check(source.error == sim::StartupWorldRuntimeError::none && source.accepted,
          "Reference task offer accepts the current funds");
    app::WorldSession session(offer);
    const auto accepted = session.act_task_page(page, Action::confirm);
    const auto paused = session.set_paused(true);
    check(paused == accepted + 1, "Task accept and pause retain common FIFO order");
    const auto result = input_frame(session, paused);
    check(!result->failed && input_result(*result, accepted).task_accepted &&
              input_result(*result, accepted).outcome == Outcome::applied &&
              task_top(*result->state).legacy_page == 24 &&
              result->state->scene.world.world.ai.accounting.funds() ==
                  expected.scene.world.world.ai.accounting.funds() &&
              result->state->scene.random.draws() == expected.scene.random.draws() &&
              !result->state->active_task,
          "FIFO acceptance charges and recruits once without prematurely departing");
    const auto stale = session.act_task_page(page, Action::confirm);
    const auto invalid =
        session.act_task_page(task_top(*result->state).id, static_cast<Action>(999));
    const auto view = session.set_view({10, 20}, {0, 24, 384, 211});
    const auto rejected = input_frame(session, view);
    check(!rejected->failed && input_result(*rejected, stale).outcome == Outcome::rejected &&
              input_result(*rejected, stale).runtime_error ==
                  sim::StartupWorldRuntimeError::invalid_page &&
              input_result(*rejected, invalid).outcome == Outcome::rejected &&
              rejected->state->scene.world.world.ai.accounting.funds() ==
                  result->state->scene.world.world.ai.accounting.funds(),
          "Stale/invalid task inputs are recoverable and their serial results survive a view");
    session.stop();

    auto poor = offer;
    poor.scene.world.world.ai.accounting = rules::PeriodAccounting(0);
    app::WorldSession denied(poor);
    const auto refusal = denied.act_task_page(page, Action::confirm);
    const auto frozen = input_frame(denied, denied.set_paused(true));
    const auto &denial = input_result(*frozen, refusal);
    check(!frozen->failed && denial.outcome == Outcome::rejected &&
              denial.runtime_error == sim::StartupWorldRuntimeError::none &&
              denial.denial == rules::TaskCommandDenial::insufficient_funds &&
              !denial.task_accepted &&
              event_count(*frozen->state, 11) == event_count(poor, 11) + 1 &&
              frozen->state->scene.world.world.ai.accounting.funds() == 0 &&
              frozen->state->scene.random.draws() == poor.scene.random.draws() &&
              !frozen->state->active_task,
          "Funds denial preserves the real message11 transaction without paying, RNG or worker "
          "failure");
    check(denied.set_view({}, {0, 24, 384, 211}) != 0, "Denial leaves transport usable");
    denied.stop();

    auto missing = initial(true);
    missing.rules = nullptr;
    app::WorldSession broken(missing);
    const auto command = broken.open_task_menu();
    const auto failed = input_frame(broken, command);
    check(failed->failed &&
              input_result(*failed, command).runtime_error ==
                  sim::StartupWorldRuntimeError::missing_source &&
              broken.open_task_menu() == 0,
          "Missing task source is a fatal integration error, not a gameplay refusal");
    broken.stop();
}
void recruitment_held_transport() {
    for (int speed : {0, 1}) {
        auto state = recruitment_fixture();
        state.scene.speed_setting = speed;
        state.scene.framework_paused = true;
        const auto page = task_top(state).id;
        app::WorldSession session(state);
        const auto blocked = session.set_page_confirm_held(page, true);
        auto frame = input_frame(session, blocked);
        check(!frame->failed &&
                  input_result(*frame, blocked).outcome == app::WorldCommandOutcome::rejected &&
                  !frame->state->page_confirm_held,
              "A paused page cannot acquire held input");
        session.set_paused(false);
        const auto press = session.set_page_confirm_held(page, true);
        const auto held = input_frame(session, press);
        check(!held->failed && held->state->page_confirm_held,
              "Explicit raw24 press binds current page identity");
        // A late old-page release must not cancel this page's still-held press.
        const auto unrelated =
            input_frame(session, session.set_page_confirm_held(page + 1000, false));
        check(!unrelated->failed && unrelated->state->page_confirm_held,
              "Unrelated release cannot cancel another page's press");
        await(session, [&](const auto &f) {
            return f.outer_updates >= held->outer_updates + 3 || f.failed;
        });
        frame = input_frame(session, session.set_paused(true));
        check(!frame->failed && !frame->state->page_confirm_held &&
                  frame->state->page_counters.at(page) - held->state->page_counters.at(page) ==
                      2 * static_cast<int>(frame->outer_updates - held->outer_updates) &&
                  frame->state->scene.calendar.units == state.scene.calendar.units &&
                  frame->state->simulation_steps == state.simulation_steps,
              "Held advances twice per logical page update, independent of world1/2 speed and FPS");
        const auto pause_frame = frame;
        session.set_paused(false);
        await(session, [&](const auto &f) {
            return f.outer_updates >= pause_frame->outer_updates + 2 || f.failed;
        });
        frame = input_frame(session, session.set_paused(true));
        check(!frame->failed &&
                  frame->state->page_counters.at(page) -
                          pause_frame->state->page_counters.at(page) ==
                      static_cast<int>(frame->outer_updates - pause_frame->outer_updates),
              "Resume does not resurrect the pre-pause physical press");
        session.set_paused(false);
        frame = input_frame(session, session.set_page_confirm_held(page, true));
        check(!frame->failed && frame->state->page_confirm_held,
              "A fresh post-pause edge can acquire the current page");
        frame = input_frame(session, session.set_page_confirm_held(page, false));
        check(!frame->failed && !frame->state->page_confirm_held,
              "Release or focus-loss edge clears the current physical press");
        input_frame(session, session.set_paused(true));
        session.stop();
    }

    auto near_end = recruitment_fixture();
    const auto page = task_top(near_end).id;
    // Explicit late-animation fixture keeps the real recruitment entries/consumer; no wall-clock
    // long run.
    near_end.page_counters[page] = near_end.task_recruitment_pages.at(page).completion_tick - 1;
    app::WorldSession session(near_end);
    const auto press = session.set_page_confirm_held(page, true);
    input_frame(session, press);
    await(session, [page](const auto &f) { return task_top(*f.state).id != page || f.failed; });
    const auto frame = input_frame(session, session.set_paused(true));
    check(!frame->failed && !frame->state->page_confirm_held &&
              task_top(*frame->state).id != page && !frame->state->participants.empty(),
          "Source recruitment completion clears held before the next page can consume it");
    const auto stale = session.set_page_confirm_held(page, true);
    const auto rejected = input_frame(session, stale);
    check(!rejected->failed &&
              input_result(*rejected, stale).outcome == app::WorldCommandOutcome::rejected &&
              !rejected->state->page_confirm_held,
          "Late press cannot be retargeted onto the new page");
    session.stop();
}
void task_menu_report_and_departure() {
    app::WorldSession empty(initial());
    const auto opened = empty.open_task_menu();
    auto frame = input_frame(empty, empty.set_paused(true));
    check(!frame->failed &&
              input_result(*frame, opened).outcome == app::WorldCommandOutcome::applied &&
              frame->state->task_order.empty() && event_count(*frame->state, 29) == 1,
          "Opening an empty task list consumes original event29 instead of fabricating a task");
    empty.stop();

    auto report = initial();
    report.report_state = 1;
    app::WorldSession reporting(report);
    const auto denied = reporting.open_task_menu();
    frame = input_frame(reporting, denied);
    check(!frame->failed &&
              input_result(*frame, denied).outcome == app::WorldCommandOutcome::rejected &&
              frame->outer_updates == 0 && app::world_report_waiting(*frame->state),
          "Task menu input cannot bypass the desktop's manual month report gate");
    const auto confirmed = input_frame(reporting, reporting.ack_report(1));
    check(!confirmed->failed && confirmed->state->report_state == 2,
          "Rejected task input leaves the ordinary report protocol usable");
    reporting.stop();

    auto team = recruitment_fixture();
    const auto recruitment = task_top(team).id;
    team.page_counters[recruitment] =
        team.task_recruitment_pages.at(recruitment).completion_tick - 1;
    const auto recruited = sim::update_startup_world_runtime_task_page(team, recruitment);
    check(recruited.has_value(), "Late source recruitment fixture completes");
    team = *recruited;
    // Source event8/9 may be above the team page; explicitly acknowledge only its dialogue.
    for (int i = 0; i < 8 && task_top(team).legacy_page != 25; ++i) {
        check(task_top(team).kind != rules::WorldScriptPageKind::raw_page &&
                  sim::acknowledge_startup_world_runtime_page(team, task_top(team).id) ==
                      sim::StartupWorldRuntimeError::none,
              "Only source recruitment messages may precede the team fixture");
    }
    check(task_top(team).legacy_page == 25 && !team.participants.empty(),
          "Source team selection is available");
    const auto proposal = sim::act_startup_world_runtime_task_page(
        team, task_top(team).id, sim::StartupWorldTaskAction::depart);
    check(proposal.error == sim::StartupWorldRuntimeError::none &&
              task_top(team).legacy_page == 28 && !team.active_task,
          "Departure proposal opens source prediction without activating a task");
    const auto departure = task_top(team).id;
    const auto task = *task_top(team).task_identity;
    const auto started = sim::act_startup_world_runtime_task_page(
        team, departure, sim::StartupWorldTaskAction::confirm, 0);
    check(started.error == sim::StartupWorldRuntimeError::none &&
              team.page_phases.at(departure) == 1,
          "Explicit departure selection starts the source animation");
    team.page_counters[departure] =
        95; // Explicit final-frame transport fixture, not natural elapsed time.
    app::WorldSession departing(team);
    const auto finish = departing.act_task_page(departure, sim::StartupWorldTaskAction::confirm, 0);
    frame = input_frame(departing, departing.set_paused(true));
    check(!frame->failed && input_result(*frame, finish).departed &&
              frame->state->active_task == task && frame->state->scene.world.world.ai.task_active &&
              frame->state->task_subperiods == 0 && !frame->state->page_confirm_held,
          "Final source departure input installs one real task and returns to main");
    departing.stop();

    auto shop = initial();
    shop.scripts.pages.front().lifecycle = 3;
    rules::WorldScriptPage page;
    page.id = shop.scripts.next_page_id++;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 83;
    shop.scripts.pages.push_back(page);
    app::WorldSession shopping(shop);
    const auto cancel = shopping.cancel_page(page.id);
    frame = input_frame(shopping, shopping.set_paused(true));
    check(!frame->failed &&
              input_result(*frame, cancel).outcome == app::WorldCommandOutcome::applied &&
              task_top(*frame->state).kind == rules::WorldScriptPageKind::scene &&
              frame->state->scene.world.world.ai.accounting.funds() ==
                  shop.scene.world.world.ai.accounting.funds(),
          "Explicit shop83 Back closes only its page without purchasing");
    const auto duplicate = shopping.cancel_page(page.id);
    frame = input_frame(shopping, duplicate);
    check(!frame->failed &&
              input_result(*frame, duplicate).outcome == app::WorldCommandOutcome::rejected,
          "Old shop Back is a recoverable stale input");
    shopping.stop();
}

} // namespace ark::test::world_session
