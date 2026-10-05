// Transport tests use the real maintained runtime; no alternative AI/tick consumer is injected.
#include "ark/app/world_report.hpp"
#include "ark/app/world_session.hpp"
#include "ark/simulation/startup_world_runtime_tasks.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <atomic>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
namespace app = ark::app;
namespace sim = ark::simulation;
namespace rules = sim::rules;
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
app::WorldState initial(bool paused = false) {
    auto state = ark::test::initial_world();
    state.scene.framework_paused = paused;
    return state;
}
std::shared_ptr<const app::WorldFrame>
await(app::WorldSession &session, const std::function<bool(const app::WorldFrame &)> &predicate) {
    const auto limit = Clock::now() + 5s;
    auto frame = session.frame();
    while (!predicate(*frame)) {
        if (Clock::now() >= limit)
            throw std::runtime_error("Timed out awaiting a world worker publication");
        frame = session.wait_for_frame_after(frame->revision, 100ms);
    }
    return frame;
}
void same_world(const app::WorldState &a, const app::WorldState &b) {
    check(a.scene.world.world.ai.accounting.funds() == b.scene.world.world.ai.accounting.funds() &&
              a.scene.world.updates == b.scene.world.updates &&
              a.simulation_steps == b.simulation_steps && a.arrival_counter == b.arrival_counter &&
              a.scene.frame_counter == b.scene.frame_counter &&
              a.scene.scene_counter == b.scene.scene_counter &&
              a.scene.scene_state == b.scene.scene_state &&
              a.scene.speed_setting == b.scene.speed_setting &&
              a.scene.framework_paused == b.scene.framework_paused &&
              a.scene.calendar.units == b.scene.calendar.units &&
              a.scene.calendar.year == b.scene.calendar.year &&
              a.scene.calendar.month == b.scene.calendar.month &&
              a.scene.world.world.ai.human_order == b.scene.world.world.ai.human_order &&
              a.scene.world.world.ai.monster_order == b.scene.world.world.ai.monster_order &&
              a.camera == b.camera && a.reference_viewport == b.reference_viewport,
          "Worker and sequential source differ in world/date/input state");
    check(a.scripts.event_calls == b.scripts.event_calls &&
              a.scripts.user_flags == b.scripts.user_flags &&
              a.scripts.pages.size() == b.scripts.pages.size() &&
              a.page_counters == b.page_counters,
          "Worker and sequential source differ in event/page progression");
    for (std::size_t i = 0; i < a.scripts.pages.size(); ++i) {
        const auto &left = a.scripts.pages[i];
        const auto &right = b.scripts.pages[i];
        check(left.id == right.id && left.kind == right.kind && left.lifecycle == right.lifecycle &&
                  left.legacy_page == right.legacy_page && left.title == right.title &&
                  left.paragraphs == right.paragraphs,
              "Worker changed the source page identity or content");
    }
    check(a.scene.random.draws() == b.scene.random.draws(),
          "Worker changed the shared random cursor");
    auto ra = a.scene.random, rb = b.scene.random;
    for (int i = 0; i < 8; ++i)
        check(ra.draw(197).ticket == rb.draw(197).ticket,
              "Worker changed future shared random values");
}
app::WorldState advance(app::WorldState state) {
    auto result = sim::prepare_startup_world_runtime(state);
    check(result.candidate && sim::update_startup_world_render_cache(*result.candidate),
          "Reference source step failed");
    return std::move(*result.candidate);
}
void paused_commands() {
    app::WorldSession session(initial(true));
    const auto first = session.frame();
    check(first && first->state && first->previous == first->state && first->revision == 0 &&
              first->outer_updates == 0 && !first->failed && first->interval_seconds == .047,
          "Constructor publishes a complete immutable initial world before ticking");
    // Waiting covers several source gates without introducing a busy loop or a test clock.
    const auto still = session.wait_for_frame_after(first->revision, 120ms);
    check(still == first, "Paused gates do not publish fake updates or advance revision");
    const auto a = session.set_speed(1);
    const auto b = session.set_view({13.F, -24.F}, {0, 24, 384, 211});
    const auto c = session.set_speed(0);
    const auto d = session.set_view({31.F, 42.F}, {1, 25, 500, 300});
    check(a > 0 && b == a + 1 && c == b + 1 && d == c + 1,
          "Input FIFO assigns consecutive serials in submission order");
    const auto changed = await(
        session, [d](const auto &frame) { return frame.last_command_serial >= d || frame.failed; });
    check(!changed->failed && changed->outer_updates == 0 &&
              changed->state->scene.speed_setting == 0 &&
              changed->state->camera == std::array<float, 2>{31.F, 42.F} &&
              changed->state->reference_viewport == std::array<int, 4>{1, 25, 500, 300} &&
              changed->previous == changed->state,
          "Paused FIFO commits current view/speed without ticking or replaying interpolation");
    check(first->state->camera != changed->state->camera && first->state->scene.framework_paused &&
              first->revision == 0 && first->state->simulation_steps == 0,
          "Readers holding an old publication never observe worker mutations");
    const auto duplicate =
        session.set_view(changed->state->camera, changed->state->reference_viewport);
    const auto same = await(
        session, [duplicate](const auto &frame) { return frame.last_command_serial >= duplicate; });
    check(same->state == changed->state && same->revision == changed->revision + 1,
          "Unchanged view acknowledges input without cloning the entire world");
    const auto stopped = Clock::now();
    session.stop();
    session.stop();
    check(Clock::now() - stopped < 1s && session.set_paused(false) == 0 &&
              session.wait_for_frame_after(same->revision, 5s) == session.frame(),
          "Stop wakes waits, joins idempotently and refuses further input");
}
void sequential_equivalence() {
    for (int speed : {0, 1}) {
        auto start = initial();
        start.scene.speed_setting = speed;
        auto paused_start = start;
        paused_start.scene.framework_paused = true;
        app::WorldSession session(std::move(paused_start));
        const auto first = session.frame();
        const auto resume = session.set_paused(false);
        check(resume != 0, "Paused constructor accepts the first resume command");
        const auto progressing = await(
            session, [](const auto &frame) { return frame.outer_updates >= 4 || frame.failed; });
        check(!progressing->failed && progressing->previous != progressing->state &&
                  progressing->max_update_ms > 0 && progressing->interval_seconds >= .047 &&
                  progressing->published > first->published,
              "Real updates publish distinct immutable endpoints and measured timing");
        const auto pause = session.set_paused(true);
        const auto paused = await(session, [pause](const auto &frame) {
            return frame.last_command_serial >= pause || frame.failed;
        });
        check(!paused->failed && paused->previous == paused->state,
              "Pause publication freezes interpolation at the current endpoint");
        session.stop();
        const auto result = session.frame();
        for (std::uint64_t i = 0; i < result->outer_updates; ++i)
            start = advance(std::move(start));
        start.scene.framework_paused = true;
        same_world(*result->state, start);
        check(result->published - first->published >= 47ms * result->outer_updates,
              "Runtime starts respect the minimum interval with no FPS/speed catch-up loop");
        check(first->state->simulation_steps == 0 && first->state->scene.random.draws() == 0,
              "Published initial state remains immutable after active worker updates");
    }
}
void page_failure_rollback() {
    auto state = initial();
    const auto active = [&] {
        return std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                            [](const auto &page) { return page.lifecycle != 4; });
    };
    // Reach a real source page rather than assuming a fixed new-game frame of arrival.
    for (int i = 0; i < 100; ++i) {
        const auto page = active();
        if (page != state.scripts.pages.rend() && page->kind != rules::WorldScriptPageKind::scene &&
            page->legacy_page != 16 && page->legacy_page != 56 && page->legacy_page != 57)
            break;
        state = advance(std::move(state));
    }
    const auto top = active();
    check(top != state.scripts.pages.rend() && top->kind != rules::WorldScriptPageKind::scene,
          "Bounded source startup provides a real confirmable page");
    const auto page_id = top->id;
    state.scene.framework_paused = true;
    auto expected = state;
    check(sim::acknowledge_startup_world_runtime_page(expected, page_id) ==
              sim::StartupWorldRuntimeError::none,
          "Source accepts the first actual page confirmation");
    app::WorldSession session(state);
    const auto accepted = session.ack_page(page_id);
    const auto duplicate = session.ack_page(page_id);
    check(accepted > 0 && duplicate == accepted + 1, "Explicit confirmations retain FIFO identity");
    const auto failed = await(session, [](const auto &frame) { return frame.failed; });
    check(failed->last_command_serial == duplicate && !failed->error.empty() &&
              failed->outer_updates == 0 && failed->previous == failed->state,
          "Duplicate stale confirmation fails explicitly without automatically confirming a new "
          "page");
    same_world(*failed->state, expected);
    const auto unchanged = session.wait_for_frame_after(failed->revision, 80ms);
    check(unchanged == failed && session.set_paused(false) == 0,
          "Failure freezes the last committed state and has no automatic clear/retry");
    session.stop();
}
void runtime_failure_rollback() {
    auto invalid = initial();
    invalid.scene.scene_state = 2;
    invalid.scene.world.map_flags.pop_back(); // Same maintained invalid-owner regression input.
    app::WorldSession session(invalid);
    const auto failed = await(session, [](const auto &frame) { return frame.failed; });
    check(failed->outer_updates == 0 && failed->max_update_ms > 0 &&
              failed->error.find("World update rejected") != std::string::npos,
          "Failed runtime candidate produces a measured error rather than partial publication");
    same_world(*failed->state, invalid);
    session.stop();
}
void report_gate_and_commands() {
    auto state = initial();
    // An explicit focus-state fixture reaches the same source common-world report consumer
    // without the new-game introduction intercepting the first normal scene iteration.
    state.scene.scene_state = 2;
    state.scene.calendar.month_ticks = state.clock_parameter * 20 - 143;
    state = advance(std::move(state));
    check(app::world_report_waiting(state), "Source world update creates an exposed report");
    app::WorldSession session(state);
    const auto first = session.frame();
    const auto waiting = session.wait_for_frame_after(first->revision, 120ms);
    check(waiting == first && waiting->outer_updates == 0,
          "Visible monthly report freezes world publication pending explicit input");
    const auto next = session.ack_report(1);
    const auto phase2 = await(session, [next](const auto &frame) {
        return frame.last_command_serial >= next || frame.failed;
    });
    check(!phase2->failed && phase2->state->report_state == 2 && phase2->outer_updates == 0 &&
              phase2->state->scene.calendar.month_ticks == state.scene.calendar.month_ticks,
          "First manual report command advances only the source display phase");
    check(session.wait_for_frame_after(phase2->revision, 80ms) == phase2,
          "Financial phase also remains frozen until the second confirmation");
    const auto pause = session.set_paused(true);
    const auto close = session.ack_report(2);
    check(close == pause + 1, "Pause/report commands retain common FIFO order");
    const auto closed = await(session, [close](const auto &frame) {
        return frame.last_command_serial >= close || frame.failed;
    });
    check(!closed->failed && closed->state->report_state == 0 && closed->outer_updates == 0 &&
              closed->state->scene.framework_paused &&
              closed->state->scene.world.world.ai.accounting.funds() ==
                  state.scene.world.world.ai.accounting.funds(),
          "Closing a report preserves explicit user pause and never recharges maintenance");
    const auto resume = session.set_paused(false);
    check(resume != 0, "Report close leaves the session ready to resume");
    const auto resumed =
        await(session, [](const auto &frame) { return frame.outer_updates > 0 || frame.failed; });
    check(!resumed->failed, "Closing the report releases normal source world updates");
    session.stop();
    const auto final = session.frame();
    auto expected = *closed->state;
    expected.scene.framework_paused = false;
    for (std::uint64_t i = 0; i < final->outer_updates; ++i)
        expected = advance(std::move(expected));
    same_world(*final->state, expected);
}
void input_flood_fairness() {
    app::WorldSession session(initial());
    std::atomic<bool> finish{};
    std::atomic<bool> producer_active{true};
    std::atomic<std::uint64_t> submitted{};
    std::thread producer([&] {
        const auto limit = Clock::now() + 5s;
        std::uint64_t count{};
        while (!finish.load() && Clock::now() < limit) {
            const auto serial =
                session.set_view({static_cast<float>(count % 1000), -24.F}, {0, 24, 384, 211});
            if (!serial)
                break;
            ++count;
            submitted.store(count);
            if (count % 64 == 0)
                std::this_thread::yield();
        }
        producer_active = false;
    });
    std::shared_ptr<const app::WorldFrame> progressed;
    try {
        progressed = await(
            session, [](const auto &frame) { return frame.outer_updates >= 3 || frame.failed; });
    } catch (...) {
        finish = true;
        producer.join();
        throw;
    }
    const bool still_submitting = producer_active.load();
    finish = true;
    producer.join();
    check(still_submitting && submitted.load() > 3 && !progressed->failed &&
              progressed->outer_updates >= 3,
          "Continuous camera input cannot starve due source runtime updates");
    // Pause is an ordering barrier. The final following view must be acknowledged intact,
    // with no later source camera movement obscuring which queued view actually committed.
    const auto pause = session.set_paused(true);
    const auto last = session.set_view({123.F, 456.F}, {2, 25, 400, 240});
    check(last == pause + 1, "Final view retains its position after the pause input barrier");
    const auto final = await(session, [last](const auto &frame) {
        return frame.last_command_serial >= last || frame.failed;
    });
    check(!final->failed && final->last_command_serial == last &&
              final->state->scene.framework_paused &&
              final->state->camera == std::array<float, 2>{123.F, 456.F} &&
              final->state->reference_viewport == std::array<int, 4>{2, 25, 400, 240},
          "Coalesced views acknowledge their final serial and preserve the final camera");
    session.stop();
    check(session.set_view({}, {0, 0, 100, 100}) == 0,
          "Input flood leaves a stoppable session with no abandoned producer");
}
app::WorldState award_fixture() {
    // Explicit raw87 fixture from the published page contract, not an annual startup trace.
    auto state = initial();
    state.scripts.pages.front().lifecycle = 3;
    rules::WorldScriptPage page;
    page.id = state.scripts.next_page_id++;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 87;
    state.scripts.pages.push_back(page);
    state.scripts.executing_page = page.id;
    state.human_presence.at(1) = 1;
    state = advance(std::move(state));
    check(state.medal_count == 1 && state.award_rankings.count(page.id) &&
              !state.award_termination_pending.at(page.id),
          "Source initializes the annual page once without granting a medal to any actor");
    return state;
}
int event_count(const app::WorldState &state, int event) {
    const auto found = state.scripts.event_calls.find(event);
    return found == state.scripts.event_calls.end() ? 0 : found->second;
}
void annual_page_commands() {
    using Action = rules::WorldAwardAction;
    const auto source = award_fixture();
    const auto page = source.scripts.pages.back().id;
    app::WorldSession session(source);
    auto serial = session.act_award(page, Action::request_termination);
    auto shown = await(session, [serial](const auto &frame) {
        return frame.last_command_serial >= serial || frame.failed;
    });
    check(!shown->failed && shown->state->award_termination_pending.at(page) &&
              shown->state->medal_count == 1 && event_count(*shown->state, 22) == 0,
          "Termination request opens confirmation without ending the ceremony or spending medals");
    serial = session.act_award(page, Action::reject_termination);
    auto rejected = await(session, [serial](const auto &frame) {
        return frame.last_command_serial >= serial || frame.failed;
    });
    check(!rejected->failed && !rejected->state->award_termination_pending.at(page) &&
              rejected->state->scripts.pages.back().id == page &&
              rejected->state->scripts.pages.back().lifecycle != 4 &&
              event_count(*rejected->state, 22) == 0,
          "Explicit no answer preserves the actual annual page and allows another decision");
    const auto requested = session.act_award(page, Action::request_termination);
    const auto confirmed = session.act_award(page, Action::confirm_termination);
    const auto paused = session.set_paused(true);
    check(confirmed == requested + 1 && paused == confirmed + 1,
          "Annual request, affirmative answer and pause share the ordered command FIFO");
    const auto closed = await(session, [paused](const auto &frame) {
        return frame.last_command_serial >= paused || frame.failed;
    });
    check(!closed->failed && !closed->state->award_termination_pending.at(page) &&
              closed->state->medal_count == 1 && event_count(*closed->state, 22) == 1 &&
              std::none_of(
                  closed->state->scripts.pages.begin(), closed->state->scripts.pages.end(),
                  [page](const auto &item) { return item.id == page && item.lifecycle != 4; }),
          "Affirmative answer runs the source termination once and retains the unused medal");
    check(closed->state->scene.random.draws() == source.scene.random.draws() &&
              closed->state->scene.calendar.units == source.scene.calendar.units &&
              closed->state->scene.world.world.ai.accounting.funds() ==
                  source.scene.world.world.ai.accounting.funds() &&
              closed->state->simulation_steps == source.simulation_steps &&
              closed->state->human_presence == source.human_presence,
          "Annual input does not advance date, AI, shared randomness or fabricate cash/visitors");
    session.stop();

    for (int mode = 0; mode < 5; ++mode) {
        auto state = source;
        if (mode == 3)
            state.scene.framework_paused = true;
        app::WorldSession blocked(state);
        const auto before = blocked.frame();
        const auto input = mode == 0 ? blocked.ack_page(page)
                                     : blocked.act_award(mode == 4 ? page + 1000 : page,
                                                         mode == 1   ? Action::confirm_termination
                                                         : mode == 2 ? Action::request_award
                                                                     : Action::request_termination);
        const auto failed = await(blocked, [](const auto &frame) { return frame.failed; });
        check(
            input != 0 && failed->last_command_serial == input &&
                !failed->state->award_termination_pending.at(page) &&
                failed->state->medal_count == 1 && event_count(*failed->state, 22) == 0 &&
                failed->state->scene.random.draws() == source.scene.random.draws(),
            "Ordinary confirmation, absent question, unsupported award, pause and stale ID reject");
        if (mode == 3)
            check(failed->state == before->state && failed->outer_updates == 0,
                  "Paused annual command fails without even cloning a committed replacement");
        check(blocked.act_award(page, Action::request_termination) == 0,
              "A failed annual action cannot automatically retry or clear the session error");
        blocked.stop();
    }
}
void report_below_timed_page() {
    auto state = initial();
    state.report_state = 1; // Explicit overlay fixture beneath a source-owned automatic page.
    state.scripts.pages.front().lifecycle = 3;
    rules::WorldScriptPage timer;
    timer.id = state.scripts.next_page_id++;
    timer.kind = rules::WorldScriptPageKind::raw_page;
    timer.legacy_page = 16;
    timer.legacy_l = 2;
    state.scripts.pages.push_back(timer);
    app::WorldSession session(state);
    const auto revealed =
        await(session, [](const auto &frame) { return frame.outer_updates >= 2 || frame.failed; });
    check(!revealed->failed && revealed->state->page_counters.at(timer.id) == 2 &&
              revealed->state->report_state == 1 && app::world_report_waiting(*revealed->state) &&
              revealed->last_command_serial == 0,
          "Timed page above a report updates and closes itself without confirmation input");
    check(session.wait_for_frame_after(revealed->revision, 120ms) == revealed &&
              revealed->state->scene.calendar.units == state.scene.calendar.units &&
              revealed->state->scene.random.draws() == state.scene.random.draws(),
          "Only the revealed main-scene report holds subsequent world updates");
    session.stop();
}

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
    auto active = initial(true);
    active.active_task = 7; // Eligibility fixture only; no task update is allowed while menu holds.
    app::WorldSession session(active);
    input_frame(session, session.open_main_menu());
    session.set_paused(false);
    const auto result = input_frame(session, session.open_menu_tasks());
    check(!result->failed && result->main_menu_open && result->outer_updates == 0 &&
              result->state->active_task == active.active_task &&
              input_result(*result, result->last_command_serial).outcome == Outcome::rejected,
          "An active task rejects Adventure without closing the menu or losing its world gate");
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
} // namespace
int main() {
    paused_commands();
    sequential_equivalence();
    page_failure_rollback();
    runtime_failure_rollback();
    report_gate_and_commands();
    input_flood_fairness();
    annual_page_commands();
    report_below_timed_page();
    main_menu_gate_and_pause();
    main_menu_task_transaction();
    main_menu_modal_rejections();
    task_inputs_and_denials();
    recruitment_held_transport();
    task_menu_report_and_departure();
    std::cout << "PASS world session " << checks << " checks\n";
}
