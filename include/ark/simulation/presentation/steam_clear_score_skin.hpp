#pragma once

#include "ark/simulation/presentation/steam_human_skin.hpp"
#include "ark/simulation/village/startup_world_clear_score.hpp"

namespace ark::simulation {
class StartupApplication;
// Steam raw17 Draw_clearPoint的纯表现输入；不计算/累加分数，不推进钟或提交纪录。
// stage_counter是clearPoint_cnt，main_counter是frame_，不能由同一个阶段钟填两项。
struct SteamClearScoreInput {
    int stage{}, stage_counter{}, main_counter{}, row{};
    std::int64_t sum{}, captured_high_score{};
    // 仅阶段1/3实际显示当前行时读取；结尾row6不访问数组，也不制造第七行。
    std::optional<StartupClearScoreRows> rows;
};
struct SteamClearScoreOptions {
    int view_y{};
    bool english{}; // 只选择原English当前累计标签的几何，不冒充已核英文译文。
    std::optional<std::array<int,2>> title_widths; // 原标题阴影/正文分别实测。
    // 阶段3先测score定位组，再测score定位后缀；固定后缀留宽48不能用测宽替换。
    std::array<std::optional<int>,2> score_widths;
};
using SteamClearScorePlan = SteamHumanOverviewPlan;
// 仅已维护非负long；负数/原有符号回绕不在维护计分来源范围，显式拒绝。
// 数字为CommaSeparate普通Font文字，不替换成SEB数字或P单位。
std::optional<std::string> steam_clear_score_number(std::int64_t value);
// 中文文案沿固定APK适配；布局、角色、门槛、空数量单位及Touch独立来自Steam。
// 只核绘制载荷，不把条件显示用的sum/score反算成另一份业务验收。
std::optional<SteamClearScorePlan> steam_clear_score_skin(
    const SteamClearScoreInput &input, const SteamClearScoreOptions &options);
// 只读已初始化的应用计分页；独立主钟、阶段镜像和六行关联不全时拒绝，不在查询中初始化。
std::optional<SteamClearScoreInput> steam_clear_score_input(const StartupApplication &application,
                                                          std::uint64_t page);
} // namespace ark::simulation
