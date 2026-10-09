#pragma once

#include "dungeon_village_prototype/startup_skin.hpp"
#include <array>
#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

namespace dungeon_village_prototype {
// Steam逻辑坐标与资源身份；不混用APK窗口展开、OS像素或原游戏内部指针。
enum class SteamStartupAsset { title_menu, title_cursor, save_icon, arrows, menu, finger_left, finger_right };
struct SteamStartupResource {
    const char *package;
    const char *image_name;
    const char *sprite_name; // nullptr表示整图/裁片。
    int image{}, sprite{-1};
    bool language_variant{}; // 当前仅saveload；实际路径须由已核语言选择结果提供。
};
std::optional<SteamStartupResource> steam_startup_resource(SteamStartupAsset asset);
struct SteamStartupImage {
    SteamStartupAsset asset;
    std::array<int, 2> position;
    std::optional<std::array<int, 4>> crop;
    int frame{}; // -1明确表示使用外部当前SEB帧；纯查询不推进动画。
    int scale{1000};
};
struct SteamStartupFill {
    std::array<double, 4> rectangle;
    std::array<int, 3> rgb;
    int source_ratio{256};
};
enum class SteamStartupTextRole {
    title_menu, slot_title, slot_type, slot_name, slot_date, slot_cash, empty_slot,
    choose_data, save_menu, message_title, message_body, answer
};
// 保留原helper的实参与标题语义；不以假造的标题文字坐标代替窗口helper。
struct SteamStartupWindow {
    int width{}, height{}, vertical{}, style{};
    SteamStartupTextRole title;
    int title_value{};
};
// DrawBox后两项为边界，不是宽高。调用者使用Steam helper适配器。
struct SteamStartupBox { int left{}, top{}, right{}, bottom{}; };
enum class SteamStartupTextPlacement { source_point, centered, right, layout };
struct SteamStartupText {
    SteamStartupTextRole role;
    int row{}; // 标题/菜单/按钮行或中断0、手动1；实际译文/数值由只读文本适配器提供。
    std::array<double, 4> rectangle; // w/h为0代表点文字；否则TextLayout矩形。
    std::optional<int> anchor; // 仅已核显式anchor；未具名重载不猜内部位值。
    std::optional<std::array<int, 3>> rgb; // 空值交源样式适配器，不编造未核颜色。
    int line_space{}, font_size{}; // font_size=0保留当前字号；11为该次请求，画后恢复。
    SteamStartupTextPlacement placement{SteamStartupTextPlacement::source_point};
};
// 复用已核同几何/common资源载体，但保持Steam独立helper及原调用顺序。
// rect为变换前逻辑半开边界，outline表示一逻辑单位内边环，不许诺DPI物理像素。
using SteamStartupFrameDraw = std::variant<StartupSkinRect, StartupSkinDraw, SteamStartupText>;
struct SteamStartupFramePlan {
    std::vector<SteamStartupFrameDraw> draws;
};
// VIEW_Y与外层页面origin不同，只在helper原位置使用；本函数不安装或修改clip。
// measured_title_widths按阴影、正文两次真实StringWidth提供，不能缓存为一次测宽。
// nullopt对应null标题；{0,0}仍画两次空串。保留原零宽尾图块请求。
std::optional<SteamStartupFramePlan> steam_startup_window_skin(const SteamStartupWindow &window,
    int view_y, std::optional<std::array<int, 2>> measured_title_widths = {});
// mode0/common30，mode1/common121白角；其他mode仍画填充/边环，但不画角。
std::optional<SteamStartupFramePlan> steam_startup_box_skin(const SteamStartupBox &box,
    int view_y, int mode = 0);
// 两个独立原helper：Window2只有木纹；Window3显式坐标、双边环及H+1木纹。
// 尺寸保护为维护接口约束，不是原Steam规则；不增加Owner/字体/随机/持久化状态。
std::optional<SteamStartupFramePlan> steam_startup_window2_skin(int width, int height,
    int vertical, int view_y);
std::optional<SteamStartupFramePlan> steam_startup_window3_skin(int width, int height,
    int left, int top);
using SteamStartupDraw = std::variant<SteamStartupImage, SteamStartupFill,
    SteamStartupWindow, SteamStartupBox, SteamStartupText>;
struct SteamStartupTouch {
    int component{}, value{};
    std::optional<std::array<int, 4>> rectangle;
    std::optional<std::size_t> image_draw; // 箭头由原SEB helper注册，引用其图块，不猜热区尺寸。
    int option{}; // 普通行无option；raw20全局KEYCLICK明确为2。
};
struct SteamStartupSkinPlan {
    std::array<int, 2> origin{};
    std::vector<SteamStartupDraw> draws; // 原局部绘制顺序；文字与图片不能各自重排。
    std::vector<SteamStartupTouch> touches; // 原AddTouch基矩形，尚未经过margin/clip/scale/hit分流。
};
// 只返回已可见的菜单局部，不负责背景、Logo、平台按钮或菜单Owner。
std::optional<SteamStartupSkinPlan> steam_title_menu_skin(int width, int height,
    int title_frame, int selection);
struct SteamSaveSelectorSkinInput {
    int width{240}, height{240}, frame{}, slot{}, row{1}, title_frame{75};
    std::array<bool, 2> present{}; // 来自原目录日期!=-1，不是文件是否存在。
    bool japanese{}, title_on_top{true}, focused{true}, covered_by_dialog{};
};
// 覆盖询问时整块选档窗隐藏；菜单覆盖时保留窗体，但没有箭头触摸及选中手形。
std::optional<SteamStartupSkinPlan> steam_save_selector_skin(const SteamSaveSelectorSkinInput &input);
struct SteamSaveMenuSkinInput {
    int width{240}, height{240}, frame{}, selection{};
    bool japanese{}, on_top{true}, covered_by_dialog{};
    std::array<int, 3> measured_text_widths{}; // 原StringWidth整数测宽，不自行估字宽。
};
std::optional<SteamStartupSkinPlan> steam_save_menu_skin(const SteamSaveMenuSkinInput &input);
// raw1局部；调用者按返回origin恰一次转换。确认/取消及默认选择1由Owner维护。
// 两个实测StringWidthF决定共同按钮宽度，不能由字符数推导。
std::optional<SteamStartupSkinPlan> steam_save_confirmation_skin(int width, int height,
    int selection, const std::array<float, 2> &measured_button_widths);
} // namespace dungeon_village_prototype
