// Transport tests use the real maintained runtime; no alternative AI/tick consumer is injected.
#include "ark/app/world_report.hpp"
#include "ark/app/world_session.hpp"
#include "ark/simulation/startup_world_runtime_tasks.hpp"
#include "support/world_fixture.hpp"
#include "world_session_test_support.hpp"

#include <algorithm>
#include <atomic>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace ark::test::world_session {
using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
app::WorldState initial(bool paused) {
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
              a.scene.calendar.month_ticks == b.scene.calendar.month_ticks &&
              a.report_state == b.report_state && a.report_counter == b.report_counter &&
              a.report_snapshot == b.report_snapshot && a.village_points == b.village_points &&
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
    auto paused_reference = state;
    check(sim::acknowledge_startup_world_runtime_page(paused_reference, page_id) ==
              sim::StartupWorldRuntimeError::invalid_page,
          "Published 2b479f6 freezes every page callback while paused");
    same_world(paused_reference, state);
    app::WorldSession paused_session(state);
    const auto paused_input = paused_session.ack_page(page_id);
    const auto paused_failure =
        await(paused_session, [](const auto &frame) { return frame.failed; });
    check(paused_failure->last_command_serial == paused_input && paused_failure->outer_updates == 0,
          "Paused page refusal cannot execute a callback or advance the worker");
    same_world(*paused_failure->state, state);
    paused_session.stop();

    // Confirm while running, then freeze the committed publication before probing stale
    // input. This never depends on input arriving within the 47ms tick gate.
    state.scene.framework_paused = false;
    auto expected = state;
    check(sim::acknowledge_startup_world_runtime_page(expected, page_id) ==
              sim::StartupWorldRuntimeError::none,
          "Source accepts the first actual page confirmation");
    check(sim::acknowledge_startup_world_runtime_page(expected, page_id) ==
              sim::StartupWorldRuntimeError::invalid_page,
          "The committed page identity is stale before testing worker rollback");
    state.scene.framework_paused = true;
    app::WorldSession session(state);
    session.set_paused(false);
    const auto accepted = session.ack_page(page_id);
    const auto committed = await(session, [accepted](const auto &frame) {
        return frame.last_command_serial >= accepted || frame.failed;
    });
    check(!committed->failed &&
              std::none_of(
                  committed->state->scripts.pages.begin(), committed->state->scripts.pages.end(),
                  [&](const auto &page) { return page.id == page_id && page.lifecycle != 4; }),
          "Worker commits the actual source confirmation before rejecting stale input");
    const auto frozen = input_frame(session, session.set_paused(true));
    check(!frozen->failed && frozen->state->scene.framework_paused,
          "The last successful commit is frozen for deterministic rollback comparison");
    expected = *frozen->state;
    const auto stale = session.ack_page(page_id);
    const auto duplicate = session.ack_page(page_id);
    check(stale > accepted && duplicate == stale + 1,
          "Explicit confirmations retain FIFO identity");
    const auto failed = await(session, [](const auto &frame) { return frame.failed; });
    check(failed->last_command_serial == stale && !failed->error.empty() &&
              failed->outer_updates == frozen->outer_updates && failed->previous == failed->state,
          "Stale confirmation fails explicitly without automatically confirming a new "
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
void automatic_report_and_pause() {
    auto state = initial();
    // Explicit already-seen introduction fixture keeps the actual normal scene/calendar
    // consumer eligible. Source event7 has an empty automatic condition at new-game entry.
    state.scripts.event_calls[7] = 1;
    state.scene.calendar.month_ticks = state.clock_parameter * 20 - 143;
    const auto monster =
        std::find_if(state.rules->monsters.begin(), state.rules->monsters.end(),
                     [](const auto &value) { return value.points_per_defeat > 0; });
    check(monster != state.rules->monsters.end(),
          "Report fixture has a point-bearing source monster");
    state.scene.world.world.ai.monster_growth.at(monster->identity).defeats = 3;
    state = advance(std::move(state));
    check(app::world_report_visible(state) && state.report_snapshot[1] > 0,
          "Source world update creates an exposed report with unsettled positive points");
    // The consumer's entire 70/70 boundary is covered by world_report. Start at its last
    // first-phase tick here so this real-thread test focuses on transport, pause and resume.
    state.report_counter = 69;
    app::WorldSession session(state);
    const auto first = session.frame();
    const auto phase2 = await(
        session, [](const auto &frame) { return frame.state->report_state == 2 || frame.failed; });
    check(
        !phase2->failed && phase2->outer_updates > 0 && phase2->last_command_serial == 0 &&
            phase2->state->scene.world.updates > state.scene.world.updates &&
            phase2->state->scene.calendar.month_ticks > state.scene.calendar.month_ticks &&
            phase2->state->village_points == state.village_points &&
            first->state->report_state == 1 && first->state->report_counter == 69,
        "Automatic report advances the shared world/date and phase without input or early points");
    const auto paused = input_frame(session, session.set_paused(true));
    check(!paused->failed && paused->state->report_state == 2 &&
              paused->state->scene.framework_paused &&
              session.wait_for_frame_after(paused->revision, 120ms) == paused,
          "Explicit user pause freezes the report and all world publication until explicit resume");
    auto expected = state;
    for (std::uint64_t i = 0; i < paused->outer_updates; ++i)
        expected = advance(std::move(expected));
    expected.scene.framework_paused = true;
    same_world(*paused->state, expected);
    session.set_paused(false);
    const auto closed = await(
        session, [](const auto &frame) { return frame.state->report_state == 0 || frame.failed; });
    check(!closed->failed && !closed->state->scene.framework_paused &&
              closed->state->village_points ==
                  std::min(999, state.village_points + state.report_snapshot[1]),
          "After explicit resume, phase2 ends naturally and source awards its positive snapshot "
          "once");
    session.stop();
    const auto final = session.frame();
    expected = state;
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

    // Ordinary ack remains an explicit programming error. New award inputs use the same
    // recoverable decision protocol as task/building inputs, including stale prompts.
    app::WorldSession ordinary(source);
    ordinary.ack_page(page);
    check(await(ordinary, [](const auto &f) { return f.failed; })->failed,
          "Ordinary confirmation cannot select an annual award decision");
    ordinary.stop();
    for (int mode = 0; mode < 4; ++mode) {
        auto state = source;
        if (mode == 2)
            state.scene.framework_paused = true;
        app::WorldSession blocked(state);
        const auto input = blocked.act_award(mode == 3 ? page + 1000 : page,
                                             mode == 0   ? Action::confirm_termination
                                             : mode == 1 ? static_cast<Action>(999)
                                                         : Action::request_termination);
        const auto rejected = await(
            blocked, [input](const auto &f) { return f.last_command_serial >= input || f.failed; });
        check(!rejected->failed && rejected->last_command_serial == input &&
                  rejected->command_results.back().outcome == app::WorldCommandOutcome::rejected &&
                  !rejected->state->award_termination_pending.at(page) &&
                  rejected->state->medal_count == 1 && event_count(*rejected->state, 22) == 0 &&
                  rejected->state->scene.random.draws() == source.scene.random.draws(),
              "Absent prompt, invalid action, pause and stale award ID reject without world "
              "mutation");
        check(blocked.set_view({}, {0, 24, 384, 211}) != 0,
              "Rejected annual input retains a usable command transport");
        blocked.stop();
    }
    app::WorldSession rewarding(source);
    const auto request = rewarding.act_award(page, Action::request_award, 0);
    const auto no = rewarding.act_award(page, Action::reject_award);
    const auto yes_request = rewarding.act_award(page, Action::request_award, 0);
    const auto yes = rewarding.act_award(page, Action::confirm_award);
    const auto pause = rewarding.set_paused(true);
    const auto rewarded = await(
        rewarding, [pause](const auto &f) { return f.last_command_serial >= pause || f.failed; });
    const auto human = source.award_rankings.at(page).front();
    check(no == request + 1 && yes_request == no + 1 && yes == yes_request + 1 &&
              !rewarded->failed && rewarded->state->medal_count == 0 &&
              rewarded->state->shop_humans.at(human).satisfaction ==
                  source.shop_humans.at(human).satisfaction + 10 &&
              rewarded->state->scene.random.draws() == source.scene.random.draws(),
          "Explicit ranked selection, reject, reselect and confirm grant one actual reward in FIFO "
          "order");
    rewarding.stop();
}
void report_below_timed_page() {
    auto state = initial();
    state.scene.scene_state = 2;
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
    const auto progressed = await(session, [&](const auto &frame) {
        return frame.outer_updates > revealed->outer_updates || frame.failed;
    });
    check(!progressed->failed &&
              progressed->state->report_counter > revealed->state->report_counter,
          "Revealing the report resumes its source counter with normal shared-world updates");
    session.stop();
    const auto final = session.frame();
    auto expected = state;
    for (std::uint64_t i = 0; i < final->outer_updates; ++i)
        expected = advance(std::move(expected));
    same_world(*final->state, expected);
}
void sound_output_sink() {
    auto state = initial(true);
    using Operation = sim::StartupAudioOperation;
    state.sound_requests = {{Operation::jingle, 4}, {Operation::ordinary_play, 11}};
    state.scripts.pages.front().lifecycle = 3;
    rules::WorldScriptPage page;
    page.id = state.scripts.next_page_id++;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 30; // The source requests sound4 exactly at counter1.
    state.scripts.pages.push_back(page);
    app::WorldSession session(state);
    const auto first = session.frame();
    check(first->consumed_sound_requests == 2 && first->state->sound_requests.empty() &&
              first->previous == first->state,
          "Prewarmed sounds are taken once before the first immutable publication");
    for (int i = 0; i < 10; ++i)
        check(session.frame() == first, "Repeated frame reads cannot consume or replay outputs");
    const auto metadata = input_frame(session, session.set_view({11, 20}, {0, 24, 384, 211}));
    check(metadata->consumed_sound_requests == 2 && metadata->state->sound_requests.empty(),
          "Camera publication does not replay the initial sound queue");
    session.set_paused(false);
    await(session, [](const auto &f) { return f.failed || f.outer_updates >= 2; });
    const auto ticked = input_frame(session, session.set_paused(true));
    check(!ticked->failed && ticked->consumed_sound_requests == 3 &&
              ticked->state->sound_requests.empty() && ticked->previous->sound_requests.empty() &&
              first->consumed_sound_requests == 2,
          "Successful source counter1 sound is consumed once without mutating retained snapshots");
    check(session.take_audio_requests() ==
              std::vector<sim::StartupAudioRequest>{
                  {Operation::jingle, 4}, {Operation::ordinary_play, 11}, {Operation::jingle, 4}},
          "Skipped frames retain operation, duplicate ID and original output order");
    check(session.take_audio_requests().empty(), "Second audio drain is empty");
    const auto rejected =
        input_frame(session, session.act_tax(page.id, sim::StartupWorldTaxAction::confirm));
    check(!rejected->failed && rejected->consumed_sound_requests == 3,
          "Rejected decision publishes no new sound consumption");
    const auto failed = input_frame(session, session.ack_page(page.id + 1000));
    check(failed->failed && failed->consumed_sound_requests == 3 &&
              failed->state->sound_requests.empty(),
          "Failure publication preserves the last successful output-consumption count");
    check(session.take_audio_requests().empty(), "Rejected and failed commands emit no audio");
    session.stop();
}

} // namespace ark::test::world_session
int main() {
    using namespace ark::test::world_session;
    paused_commands();
    sequential_equivalence();
    page_failure_rollback();
    runtime_failure_rollback();
    automatic_report_and_pause();
    input_flood_fairness();
    annual_page_commands();
    report_below_timed_page();
    main_menu_gate_and_pause();
    main_menu_task_transaction();
    main_menu_modal_rejections();
    task_inputs_and_denials();
    recruitment_held_transport();
    task_menu_report_and_departure();
    building_command_transactions();
    facility_and_rank_commands();
    facility_catalog_commands();
    residence_replacement_command();
    human_command_transactions();
    human_gift_parent_transaction();
    tax_command_transactions();
    sound_output_sink();
    save_command_transactions();
    system_command_transactions();
    village_command_transactions();
    commerce_command_transactions();
    magic_pot_commands();
    facility_item_command_transactions();
    editing_command_transactions();
    std::cout << "PASS world session " << checks << " checks\n";
}
