#include "ark/app/session/world_report.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
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
// A real source menu updates its own lifecycle while the world remains modal.
std::shared_ptr<const app::WorldFrame> menu_ready(app::WorldSession &session, int raw) {
    return await(session, [raw](const auto &f) {
        const auto &p = task_top(*f.state);
        return f.failed || (p.legacy_page == raw && p.lifecycle == 2 &&
                            sim::inspect_startup_world_menu_page(*f.state, p.id).has_value());
    });
}
std::shared_ptr<const app::WorldFrame> menu_tag(app::WorldSession &session, int raw, int tag) {
    const auto f = menu_ready(session, raw);
    check(!f->failed, "source navigation ready");
    const auto &p = task_top(*f->state);
    const auto view = sim::inspect_startup_world_menu_page(*f->state, p.id);
    const auto row = std::find(view->tags.begin(), view->tags.end(), tag);
    check(row != view->tags.end(), "source navigation tag is actually available");
    sim::StartupWorldMenuInput in;
    in.select_row = static_cast<int>(row - view->tags.begin());
    input_frame(session, session.act_menu(p.id, in));
    in = {};
    in.confirm = true;
    return input_frame(session, session.act_menu(p.id, in));
}
void main_menu_gate_and_pause() {
    using Outcome = app::WorldCommandOutcome;
    for (bool paused : {false, true}) {
        const auto state = initial(paused);
        app::WorldSession session(state);
        auto opened = input_frame(session, session.open_main_menu());
        if (paused) {
            check(!opened->failed && !opened->main_menu_open &&
                      input_result(*opened, opened->last_command_serial).outcome ==
                          Outcome::rejected,
                  "source menu refuses a paused framework without resuming it");
            same_world(*opened->state, state);
            session.stop();
            continue;
        }
        opened = menu_ready(session, 3);
        const auto ready = opened;
        const auto steps = ready->state->simulation_steps;
        const auto random = ready->state->scene.random.draws();
        const auto advanced = await(session, [&](const auto &f) {
            return f.outer_updates > ready->outer_updates || f.failed;
        });
        check(!advanced->failed && advanced->main_menu_open &&
                  advanced->state->simulation_steps == steps &&
                  advanced->state->scene.random.draws() == random,
              "real menu lifecycle publishes updates without advancing world or random");
        const auto duplicate = session.open_main_menu(), bypass = session.open_task_menu();
        const auto rejected = input_frame(session, bypass);
        check(!rejected->failed &&
                  input_result(*rejected, duplicate).outcome == Outcome::rejected &&
                  input_result(*rejected, bypass).outcome == Outcome::rejected,
              "duplicate menu and direct task shortcut cannot bypass raw3");
        input_frame(session, session.set_paused(true));
        auto denied = input_frame(session, session.close_main_menu());
        check(denied->main_menu_open && denied->state->scene.framework_paused &&
                  input_result(*denied, denied->last_command_serial).outcome == Outcome::rejected,
              "paused menu close rejects and preserves the player's pause");
        input_frame(session, session.set_paused(false));
        const auto closed = input_frame(session, session.close_main_menu());
        check(!closed->failed && !closed->main_menu_open,
              "real raw3 cancellation retires the menu");
        const auto resumed = await(
            session, [&](const auto &f) { return f.state->simulation_steps > steps || f.failed; });
        check(!resumed->failed && resumed->interval_seconds >= .047,
              "close resumes source pacing without catch-up");
        session.stop();
    }
}
void main_menu_task_transaction() {
    for (bool has_task : {false, true}) {
        auto state = has_task ? task_listing_fixture() : initial();
        state.scripts.user_flags |= 4U; // Explicit unlocked Adventure command fixture.
        app::WorldSession session(state);
        input_frame(session, session.open_main_menu());
        menu_tag(session, 3, 1);
        const auto submenu = menu_ready(session, 4);
        const auto steps = submenu->state->simulation_steps;
        const auto funds = submenu->state->scene.world.world.ai.accounting.funds();
        const auto result = menu_tag(session, 4, 7);
        check(!result->failed && result->state->simulation_steps == steps &&
                  result->state->scene.world.world.ai.accounting.funds() == funds,
              "real adventure parent and command do not advance business or charge a task");
        check(has_task ? task_top(*result->state).legacy_page == 22
                       : event_count(*result->state, 29) == 1,
              "actual adventure source selects catalog or original empty-task feedback");
        const auto stale = input_frame(session, session.open_menu_tasks());
        check(!stale->failed && input_result(*stale, stale->last_command_serial).outcome ==
                                    app::WorldCommandOutcome::rejected,
              "retired desktop shortcut cannot open a duplicate source page");
        session.stop();
    }
}
void main_menu_modal_rejections() {
    for (bool report : {false, true}) {
        auto state = report ? initial(false) : task_offer_fixture();
        if (report)
            state.report_state = 1;
        app::WorldSession session(state);
        const auto result = input_frame(session, session.open_main_menu());
        check(!result->failed && result->main_menu_open == report &&
                  input_result(*result, result->last_command_serial).outcome ==
                      (report ? app::WorldCommandOutcome::applied
                              : app::WorldCommandOutcome::rejected),
              "automatic report allows source menu while an actual modal refuses it");
        session.stop();
    }
    auto missing = initial(true);
    missing.rules = nullptr;
    app::WorldSession failed(missing);
    const auto result = input_frame(failed, failed.open_main_menu());
    check(result->failed && !result->main_menu_open &&
              input_result(*result, result->last_command_serial).runtime_error ==
                  sim::StartupWorldRuntimeError::missing_source,
          "missing source remains a fatal integration error");
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
    const auto requested = reporting.open_task_menu();
    frame = input_frame(reporting, reporting.set_paused(true));
    check(!frame->failed &&
              input_result(*frame, requested).outcome == app::WorldCommandOutcome::applied &&
              frame->outer_updates == 0 && frame->state->report_state == 1 &&
              event_count(*frame->state, 29) == 1,
          "Visible automatic report permits the source task-menu action without a report "
          "acknowledgement");
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

// These are page-callsite fixtures. They use the real initial shared definitions; no
// alternate reward calculation or long-running automatic player lives in transport tests.
app::WorldState managed_page_fixture(int raw, bool ready = true) {
    auto state = initial();
    if (raw == 60) {
        check(sim::open_startup_world_human_page(state, 1) == sim::StartupWorldRuntimeError::none,
              "Details fixture obtains its source0 context from the real scene entry");
    } else {
        state.scripts.pages.front().lifecycle = 3;
        rules::WorldScriptPage page;
        page.id = state.scripts.next_page_id++;
        page.kind = rules::WorldScriptPageKind::raw_page;
        page.legacy_page = raw;
        state.scripts.pages.push_back(page);
        state.page_human_bindings[page.id] = 1;
    }
    for (int event : {99, 111, 112, 119})
        state.scripts.event_calls[event] = 1;
    if (ready && raw >= 60 && raw <= 73)
        check(sim::initialize_startup_world_human_pages(state),
              "Managed fixture initializes source pages");
    state.scene.framework_paused = true;
    return state;
}
void human_command_transactions() {
    using Action = sim::StartupHumanPageAction;
    using Outcome = app::WorldCommandOutcome;
    auto state = initial(true);
    // Minimal clicked-actor identity fixture; no actor simulation is run in this case.
    rules::BattleActorRecord actor;
    actor.id = {501};
    actor.kind = rules::ActorKind::human;
    actor.definition = 1;
    state.scene.world.world.ai.battle.actors.emplace(actor.id, actor);
    state.scene.world.world.ai.human_order.push_back(actor.id);
    auto unavailable = actor;
    unavailable.id = {502};
    unavailable.control.state = 4;
    state.scene.world.world.ai.battle.actors.emplace(unavailable.id, unavailable);
    state.scene.world.world.ai.human_order.push_back(unavailable.id);
    state.human_presence.at(1) = 1;
    app::WorldSession session(state);
    const auto missing = session.open_human({999}, 1);
    const auto mismatched = session.open_human(actor.id, 2);
    session.set_paused(false);
    const auto stale_state = session.open_human(unavailable.id, 1);
    const auto opened = session.open_human(actor.id, 1);
    auto frame = input_frame(session, session.set_paused(true));
    check(!frame->failed && input_result(*frame, missing).outcome == Outcome::rejected &&
              input_result(*frame, mismatched).outcome == Outcome::rejected &&
              input_result(*frame, stale_state).outcome == Outcome::rejected &&
              input_result(*frame, stale_state).runtime_error ==
                  sim::StartupWorldRuntimeError::invalid_page &&
              input_result(*frame, opened).outcome == Outcome::applied &&
              task_top(*frame->state).legacy_page == 60 && frame->outer_updates == 0,
          "FIFO opens only a currently selectable actor with the observed definition; state4 "
          "rejects");
    const auto page = task_top(*frame->state).id;
    session.set_paused(false);
    const auto early = session.act_human(page, Action::view_tab, 1);
    const auto early_cancel = session.cancel_page(page);
    frame = input_frame(session, session.set_paused(true));
    check(!frame->failed && input_result(*frame, early).outcome == Outcome::rejected &&
              input_result(*frame, early_cancel).outcome == Outcome::rejected &&
              !frame->state->human_pages_initialized.count(page),
          "Input cannot initialize a new human page before the real framework update");
    session.stop();

    auto detail = managed_page_fixture(60);
    const auto id = task_top(detail).id;
    app::WorldSession details(detail);
    details.set_paused(false);
    const auto invalid = details.act_human(id, static_cast<Action>(999));
    const auto tab = details.act_human(id, Action::view_tab, 3);
    const auto close = details.act_human(id, Action::cancel);
    const auto stale = details.act_human(id, Action::gifts);
    frame = input_frame(details, details.set_paused(true));
    check(!frame->failed && input_result(*frame, invalid).outcome == Outcome::rejected &&
              input_result(*frame, tab).outcome == Outcome::applied &&
              input_result(*frame, close).outcome == Outcome::applied &&
              input_result(*frame, stale).outcome == Outcome::rejected &&
              frame->state->scene.random.draws() == detail.scene.random.draws(),
          "Human tab, close and stale input preserve FIFO and reject invalid action enums");
    details.stop();

    // One source frame must preserve independent UP/DOWN branch ordering; splitting it
    // into two FIFO actions would re-evaluate the newly changed phase and select a different slot.
    auto simultaneous = managed_page_fixture(60);
    const auto simultaneous_page = task_top(simultaneous).id;
    simultaneous.page_phases.at(simultaneous_page) = 2;
    simultaneous.human_page_selections.at(simultaneous_page) = 0;
    app::WorldSession composed(simultaneous);
    composed.set_paused(false);
    sim::StartupHumanCatalogueInput both;
    both.up = both.down = true;
    const auto combined = composed.input_human(simultaneous_page, both);
    auto bad = both;
    bad.selection = 999;
    const auto invalid_selection = composed.input_human(simultaneous_page, bad);
    app::WorldCommand old;
    old.kind = app::WorldCommandKind::human_input;
    old.page = simultaneous_page;
    old.human_input = both;
    old.generation = composed.frame()->generation + 1;
    const auto wrong_generation = composed.submit(old);
    frame = input_frame(composed, composed.set_paused(true));
    check(!frame->failed && input_result(*frame, combined).outcome == Outcome::applied &&
              input_result(*frame, invalid_selection).outcome == Outcome::rejected &&
              input_result(*frame, wrong_generation).outcome == Outcome::rejected &&
              frame->state->page_phases.at(simultaneous_page) == 1 &&
              frame->state->human_page_selections.at(simultaneous_page) == 1 &&
              frame->state->scene.random.draws() == simultaneous.scene.random.draws(),
          "Composite human frame retains branch ordering; invalid selection and generation roll back");
    composed.stop();

    auto report = state;
    report.report_state = 1;
    report.scene.framework_paused = false;
    app::WorldSession reporting(report);
    const auto requested = reporting.open_human(actor.id, 1);
    frame = input_frame(reporting, reporting.set_paused(true));
    check(!frame->failed && input_result(*frame, requested).outcome == Outcome::applied &&
              frame->outer_updates == 0 && frame->state->report_state == 1 &&
              task_top(*frame->state).legacy_page == 60,
          "Visible automatic report permits actor details while preserving source page ownership");
    reporting.stop();
}
void human_tracking_commands() {
    using Action = sim::StartupHumanPageAction;
    using Outcome = app::WorldCommandOutcome;
    auto state = initial();
    // Narrow arrival callsite fixture: the real producer creates all actor
    // ownership/context fields. This does not certify natural first-visit timing.
    state.arrival_counter = 1;
    for (int event : {89, 99, 111, 112, 119, 218})
        state.scripts.event_calls[event] = 1;
    for (int n = 0; n < 4 && state.scene.world.world.ai.human_order.empty(); ++n)
        state = advance(std::move(state));
    check(!state.scene.world.world.ai.human_order.empty() &&
              task_top(state).kind == rules::WorldScriptPageKind::scene,
          "Source arrival provides a real actor and stable information entry");
    const auto actor = state.scene.world.world.ai.human_order.front();
    const auto definition = state.scene.world.world.ai.battle.actors.at(actor).definition;
    check(sim::open_startup_world_information_menu(state) == sim::StartupWorldRuntimeError::none,
          "Tracking fixture enters the real information menu");
    state = advance(std::move(state));
    sim::StartupInformationInput confirm;
    confirm.confirm = true;
    check(sim::input_startup_world_information_page(state, task_top(state).id, confirm) ==
              sim::StartupWorldRuntimeError::none,
          "Information row0 opens the real adventurer directory");
    state = advance(std::move(state));
    const auto directory = task_top(state).id;
    const auto &list = state.information_page_data.at(directory).lists.front();
    const auto found = std::find(list.begin(), list.end(), definition);
    check(found != list.end(), "Actual live definition occurs in source35 catalogue");
    sim::StartupInformationInput row;
    row.select_row = static_cast<int>(found - list.begin());
    check(sim::input_startup_world_information_page(state, directory, row) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::input_startup_world_information_page(state, directory, confirm) ==
                  sim::StartupWorldRuntimeError::none,
          "Source35 creates source1 detail context without synthetic tracking flags");
    state = advance(std::move(state));
    const auto detail = task_top(state).id;
    const auto view = sim::inspect_startup_world_human_presentation(state, detail);
    check(view && view->tracking_available && task_top(state).lifecycle == 2,
          "Initialized source1 detail advertises actual tracking eligibility");
    state.scene.framework_paused = true;
    app::WorldSession session(state);
    session.set_paused(false);
    const auto tracking = session.act_human(detail, Action::track);
    const auto repeated = session.act_human(detail, Action::track);
    const auto focused = input_frame(session, session.set_paused(true));
    check(!focused->failed && input_result(*focused, tracking).outcome == Outcome::applied &&
              input_result(*focused, repeated).outcome == Outcome::rejected &&
              focused->state->scene.scene_state == 6 &&
              focused->state->scripts.selected_actor == actor.value &&
              task_top(*focused->state).kind == rules::WorldScriptPageKind::scene,
          "Tracking FIFO retires source35/details once and selects the actual scene6 actor");
    const auto scene = task_top(*focused->state).id;
    const auto paused = input_frame(session, session.confirm_tracking(scene, actor));
    check(!paused->failed &&
              input_result(*paused, paused->last_command_serial).outcome == Outcome::rejected &&
              !paused->state->confirm_input,
          "Paused tracking confirmation cannot plant a deferred input edge");
    session.set_paused(false);
    const auto wrong_page = session.confirm_tracking(scene + 1000, actor);
    const auto wrong_actor = session.confirm_tracking(scene, {actor.value + 1000});
    const auto accepted = session.confirm_tracking(scene, actor);
    const auto duplicate = session.confirm_tracking(scene, actor);
    const auto queued = input_frame(session, session.set_paused(true));
    check(!queued->failed && input_result(*queued, wrong_page).outcome == Outcome::rejected &&
              input_result(*queued, wrong_actor).outcome == Outcome::rejected &&
              input_result(*queued, accepted).outcome == Outcome::applied &&
              input_result(*queued, duplicate).outcome == Outcome::rejected,
          "Tracking edge binds page/actor and rejects stale or already pending confirmation");
    session.set_paused(false);
    const auto reopened = await(session, [](const auto &frame) {
        const auto &p = task_top(*frame.state);
        return frame.failed || (p.legacy_page == 60 && p.lifecycle == 2 &&
                                sim::startup_world_human_page_ready(*frame.state, p.id));
    });
    const auto current = task_top(*reopened->state).id;
    check(
        !reopened->failed && current != detail && reopened->state->scene.scene_state == 0 &&
            !reopened->state->confirm_input &&
            reopened->state->human_detail_contexts.at(current).chase_mode == 1 &&
            reopened->state->human_detail_contexts.at(current).actor == actor,
        "Normal source update consumes tracking edge once and reopens actor-bound source1 details");
    const auto closed = session.act_human(current, Action::cancel);
    const auto done = input_frame(session, session.set_paused(true));
    check(!done->failed && input_result(*done, closed).outcome == Outcome::applied &&
              task_top(*done->state).kind == rules::WorldScriptPageKind::scene &&
              done->state->scene.scene_state == 0,
          "Closing reopened source1 details completes tracking without returning to retired35");
    session.stop();
}

void human_gift_parent_transaction() {
    using Action = sim::StartupHumanPageAction;
    using Outcome = app::WorldCommandOutcome;
    auto state = managed_page_fixture(64);
    state.scene.framework_paused = false;
    const auto parent = task_top(state).id;
    const int weapon = state.equipment_page_catalogs.at(parent)[0].front();
    state.catalog.at({1, weapon}).free_purchases = 1;
    check(sim::act_startup_world_human_page(state, parent, Action::confirm) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_human_pages(state),
          "Actual equipment catalogue creates and initializes its bound question65");
    const auto question = task_top(state).id;
    const auto cash = state.scene.world.world.ai.accounting.funds();
    const auto draws = state.scene.random.draws();
    state.scene.framework_paused = true;
    app::WorldSession session(state);
    session.set_paused(false);
    const auto yes = session.act_human(question, Action::confirm);
    const auto duplicate = session.act_human(question, Action::confirm);
    const auto bypass = session.act_human(parent, Action::confirm);
    auto frame = input_frame(session, session.set_paused(true));
    check(!frame->failed && input_result(*frame, yes).outcome == Outcome::applied &&
              input_result(*frame, duplicate).outcome == Outcome::rejected &&
              input_result(*frame, bypass).outcome == Outcome::rejected &&
              frame->state->human_page_answers.at(parent) == 0 &&
              frame->state->catalog.at({1, weapon}).free_purchases == 1 &&
              frame->state->scene.world.world.ai.accounting.funds() == cash &&
              frame->state->scene.random.draws() == draws,
          "65 returns only its answer; duplicate and resumed-parent input cannot grant early");
    session.set_paused(false);
    await(session, [parent](const auto &f) {
        return f.failed || !f.state->human_page_answers.count(parent);
    });
    frame = input_frame(session, session.set_paused(true));
    check(!frame->failed && frame->state->catalog.at({1, weapon}).free_purchases == 0 &&
              frame->state->shop_humans.at(1).equipment[0] == weapon &&
              frame->state->scene.world.world.ai.accounting.funds() == cash &&
              frame->state->scene.random.draws() == draws + 1,
          "A real parent update consumes free stock and commits the gift and one dialogue draw");
    session.stop();
}
void tax_command_transactions() {
    using Action = sim::StartupWorldTaxAction;
    using Outcome = app::WorldCommandOutcome;
    auto state = managed_page_fixture(90, false);
    const auto page = task_top(state).id;
    state.human_presence.at(1) = 1;
    state.human_homes.at(1)[2] = 1;
    state.human_calendar.at(1).legacy_G = 100;
    state.scene.framework_paused = false;
    check(sim::initialize_startup_world_tax_page(state, page) ==
              sim::StartupWorldRuntimeError::none,
          "Tax fixture freezes the real eligible resident list");
    state.scene.framework_paused = true;
    app::WorldSession session(state);
    session.set_paused(false);
    const auto invalid = session.act_tax(page, static_cast<Action>(999));
    const auto select = session.act_tax(page, Action::select, 0);
    const auto confirm = session.act_tax(page, Action::confirm);
    const auto stale = session.act_tax(page, Action::confirm);
    const auto frame = input_frame(session, session.set_paused(true));
    check(!frame->failed && input_result(*frame, invalid).outcome == Outcome::rejected &&
              input_result(*frame, select).outcome == Outcome::applied &&
              input_result(*frame, confirm).outcome == Outcome::applied &&
              input_result(*frame, stale).outcome == Outcome::rejected &&
              frame->state->scene.world.world.ai.accounting.funds() ==
                  state.scene.world.world.ai.accounting.funds() &&
              frame->state->human_calendar.at(1).legacy_G == 100,
          "Tax selection and confirmation never pay; duplicate and invalid enums reject");
    session.stop();
    for (int raw : {60, 90, 98}) {
        app::WorldSession guarded(managed_page_fixture(raw, false));
        const auto id = task_top(*guarded.frame()->state).id;
        const auto result = input_frame(guarded, guarded.ack_page(id));
        check(result->failed && result->outer_updates == 0 && task_top(*result->state).id == id,
              "Generic acknowledgement cannot choose a human/tax action or execute automatic98");
        guarded.stop();
    }
}

} // namespace ark::test::world_session
