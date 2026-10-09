#pragma once

#include "ark/simulation/rules/world_random.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace ark::simulation {
// APK b/h 的完整 q 槽：退休只清 active，age/y 仍影响以后更新和排序。
struct StartupTitleSlot {
    int active{}, definition{}, x{}, y{}, direction{}, age{};
};
struct StartupTitlePresentation {
    int l{}, f132f{}, s{}, t{};
    std::array<StartupTitleSlot, 20> slots{};
};
enum class StartupTitleAdmission { top_lifecycle_ready };
struct StartupTitleUpdateRequest {
    StartupTitleAdmission admission{StartupTitleAdmission::top_lifecycle_ready};
    bool confirm_pulse{};
};
struct StartupTitleUpdateCandidate {
    StartupTitlePresentation state;
    ark::simulation::rules::WorldRandomStream random;
    bool confirm_consumed{}, menu_confirm_ready{};
    std::size_t random_draws{};
};
struct StartupTitleUpdateResult {
    std::string error;
    std::optional<StartupTitleUpdateCandidate> candidate;
};
std::string validate_startup_title_presentation(const StartupTitlePresentation &state);
bool pristine_startup_title_presentation(const StartupTitlePresentation &state);
// 调用方 Owner 决定框架准入并联合提交候选；失败不会推进真实随机游标或吞输入。
StartupTitleUpdateResult prepare_startup_title_update(
    const StartupTitlePresentation &state,
    const ark::simulation::rules::WorldRandomStream &random,
    StartupTitleUpdateRequest request);

enum class StartupTitlePersonLayer { shadow, body };
struct StartupTitlePersonDraw {
    int slot{}, definition{}, age{}, step{}, facing{};
    std::array<int, 2> anchor{};
    std::array<StartupTitlePersonLayer, 2> layers{
        StartupTitlePersonLayer::shadow, StartupTitlePersonLayer::body};
};
struct StartupTitleProjection {
    std::string error;
    std::vector<StartupTitlePersonDraw> people;
};
// 只投影参数，不猜职业／服装 PNG，不更新计数或随机；surface_height 是绘制表面高度。
StartupTitleProjection project_startup_title_presentation(
    const StartupTitlePresentation &state, int surface_height);
} // namespace ark::simulation
