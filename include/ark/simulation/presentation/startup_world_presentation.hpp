#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"

namespace ark::simulation {
enum class StartupPresentationMode { full_redraw, top_only };
// 独立控制器记录调用序号与夹入位置；Owner从真实页栈核准expected_pages。
// 声音守卫是本次原平台上下文输入，不能从暂停世界/逻辑倍速推断。
struct StartupPresentationRequest {
    std::uint64_t ordinal{};
    StartupPresentationMode mode{StartupPresentationMode::top_only};
    std::vector<std::uint64_t> expected_pages;
    bool application_preview{}; // 原aQ的ap前缀。
    bool sound_paused{};         // 原SoundPlayer.h，独立于世界暂停。
    // 原66包装的!hidden/L<=0未映射为Owner字段。调用者显式证明本次栈顶66准入；
    // false不调用66出口，不能由human_pages_initialized推定true。
    bool gift_wrapper_ready{};
};
struct StartupDungeonJitter {
    std::uint64_t page{}, facility{};
    std::size_t challenge{}; // 原k索引；结果按原逆序，不按稳定怪物身份排序。
    int offset{};
};
// 成功返回的当次冻结事实；再次画它不调用Owner，没有随机/清理/声音副作用。
// 即时返回，不保存等待输出或第二份世界；调用序号由replay控制器持久化。
struct StartupPresentationPlan {
    std::uint64_t ordinal{};
    std::vector<std::uint64_t> pages;
    std::vector<StartupDungeonJitter> dungeon_jitters;
    std::optional<std::uint64_t> cleared_task;
    std::size_t sound_requests{};
    std::size_t random_before{}, random_after{};
};
struct StartupPresentationResult {
    StartupWorldRuntimeError error{StartupWorldRuntimeError::none};
    std::optional<StartupWorldRuntimeState> candidate;
    std::optional<StartupPresentationPlan> plan;
};
std::optional<std::vector<std::uint64_t>> startup_world_presentation_pages(
    const StartupWorldRuntimeState &state, StartupPresentationMode mode);
// 原e(canvas)缺绑定清理与66计数45声音出口，均在同一私有候选提交。
// 只维护本专题副作用，不宣称完整像素、隐藏包装或Android重绘调度等价。
StartupPresentationResult prepare_startup_world_presentation(
    const StartupWorldRuntimeState &state, const StartupPresentationRequest &request);
} // namespace ark::simulation
