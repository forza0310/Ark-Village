#pragma once

#include "ark/simulation/village/rules/world_calendar.hpp"
#include "ark/simulation/world/rules/world_random.hpp"
#include "ark/simulation/ai/rules/world_schedule.hpp"

namespace ark::simulation::rules {
// 这是外层唯一Owner的临时公共投影，不能再持久保存第二份world/随机/日历事实。
struct WorldSceneState {
    WorldScheduleState world;
    WorldRandomStream random;
    WorldCalendarState calendar;
    int scene_state{};           // MainScene.f102a，已证0..7。
    int speed_setting{};         // P.b[13]，仅恰1具有双轮意义。
    int frame_counter{};         // f104d，每次MainScene.b入口推进一次。
    int scene_counter{};         // f103b，每逻辑轮推进，a(state)会清0。
    int processing_phase{-1};    // aF，逻辑更新1、结束-1；不是绘制相位2。
    int processing_subphase{};   // aG，入口清0。
    bool first_normal_refresh{}; // aS，state0且新scene_counter1时置true。
    bool top_is_main{true};      // 真实框架栈顶资格，仅在进入b之前读取。
    bool framework_paused{};     // 框架m，只绘制、不调用栈顶b。
};
enum class WorldSceneStage {
    entry_task_result,        // aI!=null的上次任务结果分支，在入口计数和轮数之前。
    frame_view_sync,          // 按初始state刷新选择/观察镜头/图集21。
    global_display,           // av.f()，每逻辑轮最先推进。
    normal_condition_scripts, // 资金条件与aL脚本，触发可L159跳过本轮余下部分。
    normal_delayed_scripts,   // aN队列扫描，到期脚本同样可L159。
    normal_world,             // aH.e：月报前段与共同世界，先于输入/日历。
    normal_input,
    build_update, // a.o.c与g各设施d，不执行完整世界。
    build_input,
    focus_world, // state2：aH.e，仍处理完整世界/月报。
    focus_actor, // aH.g和W.d，保持原state2额外推进。
    focus_input,
    wait_input,            // state3，scene_counter>=10才可能确认切0。
    task_list_input,       // state4，选择任务/页面/取消。
    task_camera_input,     // state5，镜头+任务输入。
    actor_camera_input,    // state6，镜头+角色详情输入，不调用aH.e。
    facility_camera_input, // state7，镜头逼近可能切0；空目标直接L159。
    common_display_tail,   // av.q，只有到L2df的分支执行。
    common_global_flag,    // n.k，紧随q。
    common_menu_gate,      // 尾部菜单检查，打开菜单使a(page)==true→立即结束帧。
    calendar_call          // 已经正常走到尾部且实时state0，具体calendar_stage必须消费。
};
struct WorldSceneCall {
    WorldSceneStage stage{};
    int round{-1}; // 入口两个stage为-1；其余为0/1。
    std::optional<WorldCalendarStage> calendar_stage;
};
// continue_round：走到本分支下一点；skip_round：L159，仍可能进入保存的第二轮；
// end_frame：原a(page)返回true→Lebf，不把任意脚本推页自动改成这个返回。
enum class WorldSceneDisposition { continue_round, skip_round, end_frame };
struct WorldSceneStep {
    WorldSceneState state;
    WorldSceneDisposition disposition{WorldSceneDisposition::continue_round};
};
using WorldSceneConsumer =
    std::function<std::optional<WorldSceneStep>(const WorldSceneState &, const WorldSceneCall &)>;
struct WorldSceneInput {
    std::int32_t calendar_advance{}; // d.a.R，调用者提供真实值，默认0不是新局27认证。
    bool framework_admitted{true};   // 已通过活动、焦点、生命周期bB2等真实框架资格。
};
enum class WorldSceneError {
    none,
    invalid_state,
    missing_consumer,
    consumer_failed,
    invalid_disposition,
    invalid_calendar_mutation,
    calendar_failed
};
struct WorldSceneCandidate {
    WorldSceneState state;
    int scheduled_rounds{};
    int begun_rounds{};
    std::vector<WorldSceneCall> calls;
};
struct WorldSceneResult {
    WorldSceneError error{WorldSceneError::none};
    std::optional<WorldSceneCandidate> candidate;
    WorldCalendarError calendar_error{WorldCalendarError::none};
};
// 本方法只组合MainScene.b，不在每逻辑轮假加47ms，也不做固定步长补算。
// 所有域、页面、脚本和随机适配必须返回同一私有Owner；缺失不得当作成功通知。
WorldSceneResult prepare_world_scene(const WorldSceneState &state, const WorldSceneInput &input,
                                     const WorldSceneConsumer &consumer);
template <class Owner> struct OwnedWorldSceneStep {
    Owner state;
    WorldSceneDisposition disposition{WorldSceneDisposition::continue_round};
};
template <class Owner> struct OwnedWorldSceneAdapter {
    std::function<WorldSceneState(const Owner &)> read; // 值投影，不持有第二个世界。
    std::function<void(Owner &, const WorldSceneState &)> write;
    std::function<std::optional<OwnedWorldSceneStep<Owner>>(const Owner &, const WorldSceneCall &)>
        consume;
};
template <class Owner> struct OwnedWorldSceneResult {
    WorldSceneError error{WorldSceneError::none};
    std::optional<Owner> state;
    std::optional<WorldSceneCandidate> audit;
    WorldCalendarError calendar_error{WorldCalendarError::none};
};
template <class Owner>
OwnedWorldSceneResult<Owner>
prepare_owned_world_scene(const Owner &state, const WorldSceneInput &input,
                          const OwnedWorldSceneAdapter<Owner> &adapter) {
    if (!adapter.read || !adapter.write)
        return {WorldSceneError::missing_consumer, {}, {}, WorldCalendarError::none};
    Owner scratch = state;
    WorldSceneConsumer consumer;
    if (adapter.consume) {
        consumer = [&](const WorldSceneState &common,
                       const WorldSceneCall &call) -> std::optional<WorldSceneStep> {
            adapter.write(scratch, common);
            auto next = adapter.consume(scratch, call);
            if (!next)
                return {};
            scratch = std::move(next->state);
            return WorldSceneStep{adapter.read(scratch), next->disposition};
        };
    }
    auto result = prepare_world_scene(adapter.read(state), input, consumer);
    if (!result.candidate)
        return {result.error, {}, {}, result.calendar_error};
    adapter.write(scratch, result.candidate->state);
    return {WorldSceneError::none, std::move(scratch), std::move(result.candidate),
            WorldCalendarError::none};
}

// b/b.d是框架绘制路径门槛，kairo/android/a/b.g的栈顶b()路径先return false。
// Main循环再次g才到d→输入刷新/绘制；这个时钟不能绑定每个MainScene逻辑轮。
struct WorldRenderClock {
    std::int64_t last_gate_ms{};
    int parameter{20};  // v=parameter+1，初始v21，1000/21=47ms。
    bool bypass_wait{}; // b/b.u。
};
enum class WorldRenderGateError { none, invalid_parameter, overflow };
struct WorldRenderGateResult {
    WorldRenderGateError error{WorldRenderGateError::none};
    std::int64_t wait_remaining_ms{};
    std::optional<WorldRenderClock> clock; // 尚须等待时没有推进后的时钟候选。
};
// 调用方等待并泵平台事件后，以新观测now重试；达到门槛记录now，超时不补算。
WorldRenderGateResult prepare_world_render_gate(const WorldRenderClock &clock,
                                                std::int64_t observed_now_ms);
} // namespace ark::simulation::rules
