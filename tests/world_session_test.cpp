// Transport tests use the real maintained runtime; no alternative AI/tick consumer is injected.
#include "ark/app/world_report.hpp"
#include "ark/app/world_session.hpp"

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
    sim::StartupSession startup;
    sim::StartupWorldRuntimeSession source(startup.state(),
                                           rules::WorldRandomStream::from_java_seed(1));
    auto state = source.state();
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
    std::cout << "PASS world session " << checks << " checks\n";
}
