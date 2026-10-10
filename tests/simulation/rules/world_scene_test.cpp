#include "ark/simulation/world/rules/world_scene.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
WorldSceneState fixture(int state = 0, int speed = 0) {
    WorldSceneState result;
    result.scene_state = state;
    result.speed_setting = speed;
    result.random = WorldRandomStream::from_raw({10, -20, 30, -40, 50, -60, 70, -80});
    return result;
}
// 全部消费者为显式调度夹具，只验证组合，不把空域操作称为真实世界/月界闭环。
std::optional<WorldSceneStep> fixture_consumer(const WorldSceneState &state,
                                               const WorldSceneCall &) {
    return WorldSceneStep{state};
}
std::size_t count(const WorldSceneCandidate &result, WorldSceneStage stage) {
    return static_cast<std::size_t>(
        std::count_if(result.calls.begin(), result.calls.end(),
                      [&](const WorldSceneCall &call) { return call.stage == stage; }));
}
void normal_order() {
    const auto result = prepare_world_scene(fixture(), {27, true}, fixture_consumer);
    check(result.candidate && result.candidate->scheduled_rounds == 1 &&
              result.candidate->begun_rounds == 1 && result.candidate->state.frame_counter == 1 &&
              result.candidate->state.scene_counter == 1 &&
              result.candidate->state.first_normal_refresh &&
              result.candidate->state.processing_phase == -1 &&
              result.candidate->state.calendar.units == 27 &&
              result.candidate->state.calendar.month_ticks == 1,
          "one normal frame increments separate frame/scene/calendar counters");
    const std::vector<WorldSceneStage> expected{
        WorldSceneStage::entry_task_result,      WorldSceneStage::frame_view_sync,
        WorldSceneStage::global_display,         WorldSceneStage::normal_condition_scripts,
        WorldSceneStage::normal_delayed_scripts, WorldSceneStage::normal_world,
        WorldSceneStage::normal_input,           WorldSceneStage::common_display_tail,
        WorldSceneStage::common_global_flag,     WorldSceneStage::common_menu_gate};
    check(result.candidate->calls.size() == expected.size(),
          "ordinary date needs no rollover callback");
    for (std::size_t i = 0; i < expected.size(); ++i)
        check(result.candidate->calls[i].stage == expected[i] &&
                  result.candidate->calls[i].round == (i < 2 ? -1 : 0) &&
                  !result.candidate->calls[i].calendar_stage,
              "scripts precede world, inputs and common q/k/menu tail follow world");
    auto max_counter = fixture();
    max_counter.frame_counter = std::numeric_limits<int>::max() - 1;
    max_counter.scene_counter = std::numeric_limits<int>::max() - 1;
    const auto wrapped = prepare_world_scene(max_counter, {0, true}, fixture_consumer);
    check(wrapped.candidate && wrapped.candidate->state.frame_counter == 0 &&
              wrapped.candidate->state.scene_counter == 0,
          "stable source counters wrap at Integer.MAX_VALUE without C++ overflow");
}
void branch_matrix() {
    for (int state = 0; state <= 7; ++state)
        for (int speed : {0, 1, 2}) {
            const auto result =
                prepare_world_scene(fixture(state, speed), {27, true}, fixture_consumer);
            const auto rounds = state == 0 && speed == 1 ? 2 : 1;
            check(result.candidate && result.candidate->scheduled_rounds == rounds &&
                      result.candidate->begun_rounds == rounds &&
                      count(*result.candidate, WorldSceneStage::global_display) ==
                          static_cast<std::size_t>(rounds),
                  "all known states advance global display once per saved round");
            const auto &r = *result.candidate;
            check(count(r, WorldSceneStage::normal_world) == (state == 0 ? rounds : 0u) &&
                      count(r, WorldSceneStage::focus_world) == (state == 2 ? 1u : 0u) &&
                      count(r, WorldSceneStage::build_update) == (state == 1 ? 1u : 0u) &&
                      count(r, WorldSceneStage::focus_actor) == (state == 2 ? 1u : 0u),
                  "normal/focus world and build-only facility update are distinct consumers");
            check(r.state.calendar.month_ticks == (state == 0 ? rounds : 0),
                  "unchanged nonnormal state does not advance date even if focus executes world");
        }
    for (int guard = 0; guard < 3; ++guard) {
        auto state = fixture(0, 1);
        WorldSceneInput input{27, true};
        if (guard == 0)
            state.top_is_main = false;
        if (guard == 1)
            state.framework_paused = true;
        if (guard == 2)
            input.framework_admitted = false;
        const auto result = prepare_world_scene(state, input, {});
        check(result.candidate && result.candidate->begun_rounds == 0 &&
                  result.candidate->calls.empty() && result.candidate->state.frame_counter == 0 &&
                  result.candidate->state.calendar.month_ticks == 0,
              "unadmitted framework never enters MainScene.b or requires unused consumers");
    }
}
void saved_rounds_and_stack() {
    const auto changed = prepare_world_scene(
        fixture(0, 1), {27, true}, [](const WorldSceneState &state, const WorldSceneCall &call) {
            auto next = state;
            if (call.stage == WorldSceneStage::normal_input && call.round == 0) {
                next.scene_state = 1;
                next.scene_counter = 0;
                next.speed_setting = 0;
            }
            return std::optional<WorldSceneStep>{{std::move(next)}};
        });
    check(changed.candidate && changed.candidate->begun_rounds == 2 &&
              count(*changed.candidate, WorldSceneStage::normal_world) == 1 &&
              count(*changed.candidate, WorldSceneStage::build_update) == 1 &&
              changed.candidate->state.scene_counter == 1 &&
              changed.candidate->state.calendar.month_ticks == 0,
          "second saved round follows realtime new build state, not new speed nor initial normal "
          "route");
    const auto delayed = prepare_world_scene(
        fixture(0, 1), {27, true}, [](const WorldSceneState &state, const WorldSceneCall &call) {
            auto next = state;
            if (call.stage == WorldSceneStage::normal_delayed_scripts && call.round == 0) {
                next.top_is_main = false;
                return std::optional<WorldSceneStep>{
                    {std::move(next), WorldSceneDisposition::skip_round}};
            }
            return std::optional<WorldSceneStep>{{std::move(next)}};
        });
    check(delayed.candidate && delayed.candidate->begun_rounds == 2 &&
              count(*delayed.candidate, WorldSceneStage::normal_world) == 1 &&
              count(*delayed.candidate, WorldSceneStage::common_display_tail) == 1 &&
              delayed.candidate->state.calendar.month_ticks == 1 &&
              !delayed.candidate->state.top_is_main,
          "script push skips only current round; changed top does not preempt remaining native b "
          "body");
    const auto opened = prepare_world_scene(
        fixture(0, 1), {27, true}, [](const WorldSceneState &state, const WorldSceneCall &call) {
            return std::optional<WorldSceneStep>{
                {state, call.stage == WorldSceneStage::normal_input
                            ? WorldSceneDisposition::end_frame
                            : WorldSceneDisposition::continue_round}};
        });
    check(opened.candidate && opened.candidate->begun_rounds == 1 &&
              count(*opened.candidate, WorldSceneStage::normal_world) == 1 &&
              count(*opened.candidate, WorldSceneStage::common_display_tail) == 0 &&
              opened.candidate->state.calendar.month_ticks == 0,
          "explicit a(page)==true ends whole frame before q/date and suppresses second round");
    const auto menu = prepare_world_scene(
        fixture(0, 1), {27, true}, [](const WorldSceneState &state, const WorldSceneCall &call) {
            return std::optional<WorldSceneStep>{
                {state, call.stage == WorldSceneStage::common_menu_gate
                            ? WorldSceneDisposition::end_frame
                            : WorldSceneDisposition::continue_round}};
        });
    check(menu.candidate && menu.candidate->begun_rounds == 1 &&
              count(*menu.candidate, WorldSceneStage::common_global_flag) == 1 &&
              menu.candidate->state.calendar.month_ticks == 0,
          "menu opens only after q/k but before same-round date");
}
void realtime_date() {
    for (auto initial : {1, 2, 3, 7}) {
        const auto result =
            prepare_world_scene(fixture(initial, 1), {27, true},
                                [](const WorldSceneState &state, const WorldSceneCall &call) {
                                    auto next = state;
                                    if (call.stage == WorldSceneStage::build_input ||
                                        call.stage == WorldSceneStage::focus_input ||
                                        call.stage == WorldSceneStage::wait_input ||
                                        call.stage == WorldSceneStage::facility_camera_input) {
                                        next.scene_state = 0;
                                        next.scene_counter = 0;
                                    }
                                    return std::optional<WorldSceneStep>{{std::move(next)}};
                                });
        check(result.candidate && result.candidate->scheduled_rounds == 1 &&
                  result.candidate->state.calendar.units == 27 &&
                  result.candidate->state.calendar.month_ticks == 1,
              "build/focus/wait/facility-camera switching to0 at input admits same-round date");
    }
    const auto empty_camera = prepare_world_scene(
        fixture(7), {27, true}, [](const WorldSceneState &state, const WorldSceneCall &call) {
            auto next = state;
            if (call.stage == WorldSceneStage::facility_camera_input) {
                next.scene_state = 0;
                return std::optional<WorldSceneStep>{
                    {std::move(next), WorldSceneDisposition::skip_round}};
            }
            return std::optional<WorldSceneStep>{{std::move(next)}};
        });
    check(empty_camera.candidate && empty_camera.candidate->state.scene_state == 0 &&
              empty_camera.candidate->state.calendar.month_ticks == 0 &&
              count(*empty_camera.candidate, WorldSceneStage::common_display_tail) == 0,
          "null facility target uses L159 and skips tail/date despite switching state0");
}
void calendar_and_rollback() {
    auto original = fixture(0, 1);
    original.calendar = {0, 0, 3, 10773, 10746, 1599};
    const auto result = prepare_world_scene(
        original, {27, true}, [](const WorldSceneState &state, const WorldSceneCall &call) {
            auto next = state;
            if (call.stage == WorldSceneStage::normal_world)
                ++next.world.updates;
            if (call.stage == WorldSceneStage::global_display)
                next.random.draw(100);
            if (call.stage == WorldSceneStage::calendar_call &&
                call.calendar_stage == WorldCalendarStage::month_equipment)
                next.random.draw(100);
            return std::optional<WorldSceneStep>{{std::move(next)}};
        });
    check(result.candidate && result.candidate->state.calendar.month == 1 &&
              result.candidate->state.calendar.units == 27 &&
              result.candidate->state.calendar.month_ticks == 1 &&
              result.candidate->state.world.updates == 2 &&
              result.candidate->state.random.draws() == 3,
          "world then actual calendar normalization share one candidate and random owner across "
          "rounds");
    check(count(*result.candidate, WorldSceneStage::calendar_call) == 19,
          "nested rollover requires each calendar domain stage, not one completion notification");
    const auto failed = prepare_world_scene(
        original, {27, true},
        [](const WorldSceneState &state,
           const WorldSceneCall &call) -> std::optional<WorldSceneStep> {
            auto next = state;
            if (call.stage == WorldSceneStage::global_display)
                next.random.draw(100);
            if (call.stage == WorldSceneStage::normal_world)
                ++next.world.updates;
            if (call.stage == WorldSceneStage::calendar_call &&
                call.calendar_stage == WorldCalendarStage::subperiod_capacity_hint)
                return {};
            return WorldSceneStep{std::move(next)};
        });
    check(failed.error == WorldSceneError::consumer_failed && !failed.candidate &&
              failed.calendar_error == WorldCalendarError::consumer_failed &&
              original.calendar.month == 0 && original.random.draws() == 0 &&
              original.world.updates == 0,
          "late month consumer failure exposes none of earlier world/scene/random/date candidate");
    const auto mutation = prepare_world_scene(
        original, {27, true}, [](const WorldSceneState &state, const WorldSceneCall &) {
            auto next = state;
            ++next.calendar.month_ticks;
            return std::optional<WorldSceneStep>{{std::move(next)}};
        });
    check(mutation.error == WorldSceneError::invalid_calendar_mutation && !mutation.candidate,
          "scene consumer cannot eagerly advance date ahead of world");
    check(prepare_world_scene(original, {27, true}, {}).error == WorldSceneError::missing_consumer,
          "no default success scene consumer exists");
}
void disposition_guards() {
    for (auto forbidden : {WorldSceneStage::entry_task_result, WorldSceneStage::global_display,
                           WorldSceneStage::normal_world, WorldSceneStage::common_display_tail}) {
        const auto result = prepare_world_scene(
            fixture(), {27, true}, [&](const WorldSceneState &state, const WorldSceneCall &call) {
                return std::optional<WorldSceneStep>{
                    {state, call.stage == forbidden ? WorldSceneDisposition::skip_round
                                                    : WorldSceneDisposition::continue_round}};
            });
        check(result.error == WorldSceneError::invalid_disposition && !result.candidate,
              "nonbranch source call cannot invent a skip/end return");
    }
    const auto end_condition = prepare_world_scene(
        fixture(), {27, true}, [](const WorldSceneState &state, const WorldSceneCall &call) {
            return std::optional<WorldSceneStep>{
                {state, call.stage == WorldSceneStage::normal_condition_scripts
                            ? WorldSceneDisposition::end_frame
                            : WorldSceneDisposition::continue_round}};
        });
    check(end_condition.error == WorldSceneError::invalid_disposition,
          "condition script goes L159, not native page-return frame end");
}
struct FixtureOwner {
    WorldSceneState only_common;
    int external_effects{};
};
void owned_atomic() {
    const FixtureOwner original{fixture(), 0};
    OwnedWorldSceneAdapter<FixtureOwner> adapter;
    adapter.read = [](const FixtureOwner &owner) { return owner.only_common; };
    adapter.write = [](FixtureOwner &owner, const WorldSceneState &state) {
        owner.only_common = state;
    };
    adapter.consume =
        [](const FixtureOwner &owner,
           const WorldSceneCall &call) -> std::optional<OwnedWorldSceneStep<FixtureOwner>> {
        auto next = owner;
        ++next.external_effects;
        if (call.stage == WorldSceneStage::global_display)
            next.only_common.random.draw(100);
        if (call.stage == WorldSceneStage::common_menu_gate)
            return {};
        return OwnedWorldSceneStep<FixtureOwner>{std::move(next)};
    };
    const auto failed = prepare_owned_world_scene(original, {27, true}, adapter);
    check(!failed.state && !failed.audit && original.external_effects == 0 &&
              original.only_common.random.draws() == 0,
          "owned adapter rolls back extra domains together with shared projection");
    adapter.consume = [](const FixtureOwner &owner, const WorldSceneCall &call) {
        auto next = owner;
        ++next.external_effects;
        if (call.stage == WorldSceneStage::global_display)
            next.only_common.random.draw(100);
        return std::optional<OwnedWorldSceneStep<FixtureOwner>>{{std::move(next)}};
    };
    const auto success = prepare_owned_world_scene(original, {27, true}, adapter);
    check(success.state && success.audit && success.state->external_effects == 10 &&
              success.state->only_common.random.draws() == 1 &&
              success.state->only_common.calendar.month_ticks == 1,
          "owned success atomically returns domain effects and post-calendar projection");
}
void render_clock() {
    WorldRenderClock clock{1000, 20, false};
    for (int elapsed = 0; elapsed < 47; ++elapsed) {
        const auto result = prepare_world_render_gate(clock, 1000 + elapsed);
        check(result.error == WorldRenderGateError::none && !result.clock &&
                  result.wait_remaining_ms == 47 - elapsed,
              "render path integer1000/21 gate waits without advancing or logic ticks");
    }
    const auto boundary = prepare_world_render_gate(clock, 1047);
    check(boundary.clock && boundary.clock->last_gate_ms == 1047 && boundary.wait_remaining_ms == 0,
          "gate accepts exact47ms boundary and records current observation");
    const auto late = prepare_world_render_gate(clock, 5000);
    check(late.clock && late.clock->last_gate_ms == 5000 &&
              prepare_world_render_gate(*late.clock, 5001).wait_remaining_ms == 46,
          "late draw records now, has no catch-up iterations or old deadline accumulation");
    const auto backwards = prepare_world_render_gate(clock, 900);
    check(!backwards.clock && backwards.wait_remaining_ms == 147,
          "wall clock reversal waits for original source threshold, not forced logic update");
    clock.bypass_wait = true;
    check(prepare_world_render_gate(clock, 900).clock->last_gate_ms == 900,
          "source u skips wait but still overwrites s with observed timestamp");
    clock.parameter = -1;
    check(prepare_world_render_gate(clock, 1000).error == WorldRenderGateError::invalid_parameter,
          "zero divisor v0 not admitted by explicit maintenance contract");
    clock = {0, std::numeric_limits<int>::max(), false};
    check(prepare_world_render_gate(clock, 1000).error == WorldRenderGateError::invalid_parameter,
          "parameter+1 overflow rejected");
    clock = {std::numeric_limits<std::int64_t>::min(), 20, false};
    check(prepare_world_render_gate(clock, std::numeric_limits<std::int64_t>::max()).error ==
              WorldRenderGateError::overflow,
          "extreme subtraction rejected without signed C++ overflow");
    clock = {std::numeric_limits<std::int64_t>::max(), 20, false};
    check(prepare_world_render_gate(clock, std::numeric_limits<std::int64_t>::min()).error ==
              WorldRenderGateError::overflow,
          "extreme reversed clock rejected without signed C++ overflow");
}
} // namespace
int main() {
    try {
        normal_order();
        branch_matrix();
        saved_rounds_and_stack();
        realtime_date();
        calendar_and_rollback();
        disposition_guards();
        owned_atomic();
        render_clock();
        std::cout << "world_scene: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_scene: " << e.what() << '\n';
        return 1;
    }
}
