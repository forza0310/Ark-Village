#pragma once

#include "dungeon_village_prototype/steam_startup_skin.hpp"
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include "dungeon_village_prototype/startup_title_actor_skin.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace dungeon_village_prototype {
struct StartupWorldRuntimeState;
// 译文由平台按角色/分类/页签提供，金额单独沿原YEN口径传入已格式化字符串。
enum class SteamInformationTextRole {
    title, period, income_header, expense_header, category, income, expense,
    profit_label, profit_value, description, name_header, inventory_header,
    row_name, unknown_row, item_description, empty_directory, known_count, attribute_placeholder,
    satisfaction_header, effort_header, equipment_header, contribution_header,
    town_points_header, spending_header, adventurer_count,
    village_name, town_stat_label, town_adventurer_count, town_equipment_count,
    town_equipment_kinds, facility_income_list, facility_profit_header,
    facility_name, facility_tracking_hint
};
enum class SteamInformationTextMode { plain, layout, rich_text };
struct SteamInformationText {
    SteamInformationTextRole role;
    int slot{-1}; // 收支分类0..4，目录为绝对行索引；不是定义ID。
    int period{}; // 收支0月/1年，装备0..3；翻译不得改变业务页签。
    std::array<int,2> position{};
    std::optional<int> anchor;
    std::array<int,3> rgb{};
    int font_size{}; // 0保留当前字体；10是该条临时请求，绘完恢复。
    std::string value; // 原说明/名称、已格式化金额或字面占位；空值由role提供译文。
    SteamInformationTextMode mode{SteamInformationTextMode::plain};
    std::array<int,2> extent{}; // 仅layout使用；原TextLayout宽/高，不按字符数测量。
    std::optional<int> line_space{}; // layout显式0；rich_text空值沿原TextLayout全局行距。
    int argument{}; // 数量角色传计数；facility_name传ordinal+1，value仍为未拼接原定义名。
};
// 保留Steam DrawLine原端点及lineWidth；后端不可把端点差擅自当FillRect尺寸。
struct SteamInformationLine {
    std::array<int,2> from{}, to{};
    int width{1};
    std::array<int,3> rgb{};
};
enum class SteamInformationNumberKind { inventory_count, positive_attribute };
struct SteamInformationNumber {
    SteamInformationNumberKind kind;
    int value{};
    std::array<int,2> position{}; // 原Draw_count或Draw_plusValue的实参，偏移由展开器处理。
};
// 目录层有界合法输入：库存1..999，正属性1..INT_MAX。数字与单位保留原绘序。
std::optional<std::vector<StartupSkinDraw>> steam_information_number_draws(
    const SteamInformationNumber &number);
struct SteamInformationTouch : SteamStartupTouch {
    std::array<int,4> margin{}; // 保留原TouchOption.Margin，不提前混成OS热区。
    std::optional<std::array<int,3>> scroll_arguments; // count、可见行数、0x20000。
};
// Steam35的静态职业身体：在有序clip内部消费human包SEB，内部offset仅加一次。
// 不创建W，不附武器/HP，不把共享scratch未知附加效果伪装成已还原。
struct SteamInformationHumanBody {
    StartupTitleBodyDraw body;
    std::array<int,2> position{};
};
using SteamInformationDraw = std::variant<StartupSkinRect,StartupSkinDraw,
    SteamInformationText,SteamInformationLine,SteamInformationNumber,
    SteamFacilityClip,SteamInformationHumanBody,SteamFacilityNumber>;
struct SteamInformationSkinPlan {
    std::vector<SteamInformationDraw> draws;
    // image_draw引用有序draws下标；目录行及两项滚动组件保留原矩形和附加参数。
    std::vector<SteamInformationTouch> touches;
    std::array<int,2> soft_labels{0,2};
    int raw{36};
};
struct SteamInformationSkinOptions {
    int view_y{};
    bool japanese{};
    // 标题阴影/正文的两次真实StringWidth，必需；0宽仍请求空串两遍。
    std::optional<std::array<int,2>> title_widths;
    bool scroll_first_touch{}; // 平台实际CheckFirstTouch(12,0x40000)，非“鼠标悬停”。
    bool english{}; // Steam35属性标题独立En分支，不能由!japanese推导。
    std::optional<int> bottom_width{}; // 仅34必需：当前Font对底部原字符串的真实StringWidth。
};
// Steam34统计/星级/真实测宽底栏及39设施目录/利润；只读现有页面，不执行入页副作用。
std::optional<SteamInformationSkinPlan> steam_town_information_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamInformationSkinOptions &options);
std::optional<SteamInformationSkinPlan> steam_facility_information_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamInformationSkinOptions &options);
// Steam35四页完整局部计划；按有序draws执行clip/身体/文字/数字，不推进Owner。
std::optional<SteamInformationSkinPlan> steam_adventurer_information_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamInformationSkinOptions &options);
// 仅Steam raw36局部绘制：调用方安装SubForm原点，不能再给每条图元重复叠VIEW_Y。
// VIEW_Y只进入原窗口/内框helper，不替代画布尺寸或OS/DPI变换。
// 读取已初始化生命周期1/2/3，复用Owner月年投影；不推进计数、随机、声音或页栈。
std::optional<SteamInformationSkinPlan> steam_income_information_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamInformationSkinOptions &options);
// Steam37/38完整局部图元与注册计划；手形sprite21/frame=-1保留独立SEB当前帧，
// 不用Owner页面计数代替绘制动画。TextLayout/富文本仍由真实文字后端执行。
std::optional<SteamInformationSkinPlan> steam_item_information_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamInformationSkinOptions &options);
std::optional<SteamInformationSkinPlan> steam_equipment_information_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const SteamInformationSkinOptions &options);
// 仅本模块消费的common图片；路径相对assets，显式选择Steam差异图，不按同名猜复用。
std::optional<std::string_view> steam_information_image(int common_image);
} // namespace dungeon_village_prototype
