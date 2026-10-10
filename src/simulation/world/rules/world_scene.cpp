#include "ark/simulation/world/rules/world_scene.hpp"

#include <limits>

namespace ark::simulation::rules {
namespace {
bool same_calendar(const WorldCalendarState &a, const WorldCalendarState &b) {
    return a.year == b.year && a.month == b.month && a.subperiod == b.subperiod &&
           a.units == b.units && a.previous_units == b.previous_units &&
           a.month_ticks == b.month_ticks;
}
bool valid_scene(const WorldSceneState &state) {
    return state.scene_state >= 0 && state.scene_state <= 7 && state.frame_counter >= 0 &&
           state.frame_counter < std::numeric_limits<int>::max() && state.scene_counter >= 0 &&
           state.scene_counter < std::numeric_limits<int>::max() &&
           valid_world_calendar_state(state.calendar);
}
bool permits_branch_return(WorldSceneStage stage, WorldSceneDisposition disposition) {
    if (disposition == WorldSceneDisposition::continue_round)
        return true;
    if (disposition != WorldSceneDisposition::skip_round &&
        disposition != WorldSceneDisposition::end_frame)
        return false;
    switch (stage) {
    case WorldSceneStage::normal_condition_scripts:
    case WorldSceneStage::normal_delayed_scripts:
    case WorldSceneStage::build_input:
    case WorldSceneStage::facility_camera_input:
        return disposition == WorldSceneDisposition::skip_round;
    case WorldSceneStage::normal_input:
    case WorldSceneStage::common_menu_gate:
        return disposition == WorldSceneDisposition::end_frame;
    case WorldSceneStage::task_list_input:
    case WorldSceneStage::actor_camera_input:
        return true;
    default:
        return false;
    }
}
} // namespace
WorldSceneResult prepare_world_scene(const WorldSceneState &state, const WorldSceneInput &input,
                                     const WorldSceneConsumer &consumer) {
    if (!valid_scene(state) || input.calendar_advance < 0)
        return {WorldSceneError::invalid_state, {}, WorldCalendarError::none};
    WorldSceneCandidate candidate{state, 0, 0, {}};
    if (!input.framework_admitted || !state.top_is_main || state.framework_paused)
        return {WorldSceneError::none, std::move(candidate), WorldCalendarError::none};
    if (!consumer)
        return {WorldSceneError::missing_consumer, {}, WorldCalendarError::none};
    auto error = WorldSceneError::none;
    auto disposition = WorldSceneDisposition::continue_round;
    const auto call = [&](WorldSceneStage stage, int round,
                          std::optional<WorldCalendarStage> calendar_stage = {}) {
        const WorldSceneCall request{stage, round, calendar_stage};
        const auto before_calendar = candidate.state.calendar;
        auto next = consumer(candidate.state, request);
        if (!next) {
            error = WorldSceneError::consumer_failed;
            return false;
        }
        if (!valid_scene(next->state)) {
            error = WorldSceneError::invalid_state;
            return false;
        }
        if (!same_calendar(before_calendar, next->state.calendar)) {
            error = WorldSceneError::invalid_calendar_mutation;
            return false;
        }
        if (!permits_branch_return(stage, next->disposition)) {
            error = WorldSceneError::invalid_disposition;
            return false;
        }
        candidate.state = std::move(next->state);
        disposition = next->disposition;
        candidate.calls.push_back(request);
        return true;
    };
    if (!call(WorldSceneStage::entry_task_result, -1))
        return {error, {}, WorldCalendarError::none};
    candidate.state.frame_counter =
        (candidate.state.frame_counter + 1) % std::numeric_limits<int>::max();
    candidate.scheduled_rounds =
        reference_world_frame_rounds(candidate.state.scene_state, candidate.state.speed_setting);
    candidate.state.processing_phase = 1;
    candidate.state.processing_subphase = 0;
    if (!call(WorldSceneStage::frame_view_sync, -1))
        return {error, {}, WorldCalendarError::none};
    for (int round = 0; round < candidate.scheduled_rounds; ++round) {
        ++candidate.begun_rounds;
        candidate.state.scene_counter =
            (candidate.state.scene_counter + 1) % std::numeric_limits<int>::max();
        if (!call(WorldSceneStage::global_display, round))
            return {error, {}, WorldCalendarError::none};
        std::vector<WorldSceneStage> branch;
        switch (candidate.state.scene_state) {
        case 0:
            if (candidate.state.scene_counter == 1)
                candidate.state.first_normal_refresh = true;
            branch = {WorldSceneStage::normal_condition_scripts,
                      WorldSceneStage::normal_delayed_scripts, WorldSceneStage::normal_world,
                      WorldSceneStage::normal_input};
            break;
        case 1:
            branch = {WorldSceneStage::build_update, WorldSceneStage::build_input};
            break;
        case 2:
            branch = {WorldSceneStage::focus_world, WorldSceneStage::focus_actor,
                      WorldSceneStage::focus_input};
            break;
        case 3:
            branch = {WorldSceneStage::wait_input};
            break;
        case 4:
            branch = {WorldSceneStage::task_list_input};
            break;
        case 5:
            branch = {WorldSceneStage::task_camera_input};
            break;
        case 6:
            branch = {WorldSceneStage::actor_camera_input};
            break;
        case 7:
            branch = {WorldSceneStage::facility_camera_input};
            break;
        }
        for (const auto stage : branch) {
            if (!call(stage, round))
                return {error, {}, WorldCalendarError::none};
            if (disposition != WorldSceneDisposition::continue_round)
                break;
        }
        if (disposition == WorldSceneDisposition::end_frame)
            break;
        if (disposition == WorldSceneDisposition::skip_round)
            continue;
        for (const auto stage :
             {WorldSceneStage::common_display_tail, WorldSceneStage::common_global_flag,
              WorldSceneStage::common_menu_gate}) {
            if (!call(stage, round))
                return {error, {}, WorldCalendarError::none};
            if (disposition != WorldSceneDisposition::continue_round)
                break;
        }
        if (disposition == WorldSceneDisposition::end_frame)
            break;
        if (disposition == WorldSceneDisposition::skip_round)
            continue;
        // 已进入b内部，脚本改变栈顶不自行终止保存的轮数；日期只读实时scene状态。
        if (candidate.state.scene_state == 0) {
            const auto before_calendar = candidate.state.calendar;
            const auto calendar = prepare_world_calendar(
                before_calendar, input.calendar_advance,
                [&](const WorldCalendarState &date,
                    WorldCalendarStage stage) -> std::optional<WorldCalendarState> {
                    candidate.state.calendar = date;
                    if (!call(WorldSceneStage::calendar_call, round, stage))
                        return {};
                    return candidate.state.calendar;
                });
            if (!calendar.candidate)
                return {error == WorldSceneError::none ? WorldSceneError::calendar_failed : error,
                        {},
                        calendar.error};
            candidate.state.calendar = calendar.candidate->state;
        }
    }
    candidate.state.processing_phase = -1;
    return {WorldSceneError::none, std::move(candidate), WorldCalendarError::none};
}
WorldRenderGateResult prepare_world_render_gate(const WorldRenderClock &clock,
                                                std::int64_t observed_now_ms) {
    if (clock.parameter < 0 || clock.parameter == std::numeric_limits<int>::max())
        return {WorldRenderGateError::invalid_parameter, 0, {}};
    const auto interval = 1000 / (clock.parameter + 1);
    if (!clock.bypass_wait) {
        if (observed_now_ms >= clock.last_gate_ms) {
            const auto elapsed = static_cast<std::uint64_t>(observed_now_ms) -
                                 static_cast<std::uint64_t>(clock.last_gate_ms);
            if (elapsed > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
                return {WorldRenderGateError::overflow, 0, {}};
            if (elapsed < static_cast<std::uint64_t>(interval))
                return {
                    WorldRenderGateError::none, interval - static_cast<std::int64_t>(elapsed), {}};
        } else {
            const auto backwards = static_cast<std::uint64_t>(clock.last_gate_ms) -
                                   static_cast<std::uint64_t>(observed_now_ms);
            if (backwards >
                static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max() - interval))
                return {WorldRenderGateError::overflow, 0, {}};
            return {
                WorldRenderGateError::none, interval + static_cast<std::int64_t>(backwards), {}};
        }
    }
    auto result = clock;
    result.last_gate_ms = observed_now_ms;
    return {WorldRenderGateError::none, 0, result};
}
} // namespace ark::simulation::rules
