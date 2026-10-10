#pragma once

#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include <cstdint>
#include <string>

namespace dungeon_village_prototype {
struct StartupWorldRuntimeState;
// 译文由平台按角色/分类/页签提供，金额单独沿原YEN口径传入已格式化字符串。
enum class SteamInformationTextRole {
    title, period, income_header, expense_header, category, income, expense,
    profit_label, profit_value, description
};
struct SteamInformationText {
    SteamInformationTextRole role;
    int slot{-1}; // 仅五个分类行为0..4，不是菜单可选行。
    int period{}; // 0月/1年；标题显示period+1，翻译不得改成另一个业务页签。
    std::array<int,2> position{};
    std::optional<int> anchor;
    std::array<int,3> rgb{};
    int font_size{}; // 0保留当前字体；10是该条临时请求，绘完恢复。
    std::string value; // 仅income/expense/profit_value含已格式化金额，非数字SEB。
};
// 保留Steam DrawLine原端点及lineWidth；后端不可把端点差擅自当FillRect尺寸。
struct SteamInformationLine {
    std::array<int,2> from{}, to{};
    int width{1};
    std::array<int,3> rgb{};
};
using SteamInformationDraw = std::variant<StartupSkinRect,StartupSkinDraw,
                                         SteamInformationText,SteamInformationLine>;
struct SteamInformationSkinPlan {
    std::vector<SteamInformationDraw> draws;
    // 仅标题两箭头；image_draw引用上方有序draws下标。底部说明不注册点击组件。
    std::vector<SteamStartupTouch> touches;
    std::array<int,2> soft_labels{0,2};
};
struct SteamInformationSkinOptions {
    int view_y{};
    bool japanese{};
    // 标题阴影/正文的两次真实StringWidth，必需；0宽仍请求空串两遍。
    std::optional<std::array<int,2>> title_widths;
};
// 仅Steam raw36局部绘制：调用方安装SubForm原点，不能再给每条图元重复叠VIEW_Y。
// VIEW_Y只进入原窗口/内框helper，不替代画布尺寸或OS/DPI变换。
// 读取已初始化生命周期1/2/3，复用Owner月年投影；不推进计数、随机、声音或页栈。
std::optional<SteamInformationSkinPlan> steam_income_information_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamInformationSkinOptions &options);
} // namespace dungeon_village_prototype
