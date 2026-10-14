#pragma once

#include <array>
#include <optional>
#include <variant>
#include <vector>

namespace ark::simulation {
// Canvas.DrawScreenCursor的按下图选择，与合成Touch的按键资格分开。
// key为当前顶页映射的Unity KeyCode；布尔值来自已准入的平台查询，不发现设备。
struct SteamCursorPressInput {
    int a_key{-1}, b_key{-1}, y_key{-1}, gui_mode{};
    bool reverse{}, a_pressed{}, b_pressed{}, keyboard_y{}, steam_y{};
};
bool steam_cursor_pressed(const SteamCursorPressInput &input);
struct SteamCursorSkinInput {
    int draw_frame{}; // 本次调用前0..191；隐藏时原方法也先推进。
    bool joystick{};
    std::optional<std::array<float,2>> position;
    bool pressed{};
    int recover{}; // 已有平台状态0..12；本查询不递减。
    int application_type{};
    std::array<int,2> screen_size{}, canvas_size{}; // 仅type4消费；canvas宽来自native GetWidth。
};
struct SteamCursorDraw {
    std::array<int,2> anchor{};
    int frame{}, layer{1}; // 原SEB10/image12，显式帧不登记共享自动动画。
};
struct SteamCursorPushScale { float x{}, y{}; std::array<int,2> pivot{}; };
struct SteamCursorRenderMode { int operation{}, source{}, destination{}; };
struct SteamCursorPopMatrix {};
using SteamCursorOperation=std::variant<SteamCursorDraw,SteamCursorPushScale,
    SteamCursorRenderMode,SteamCursorPopMatrix>;
struct SteamCursorSkinPlan {
    int next_draw_frame{};
    // 基础Draw继承环境混合；恢复时PushScale→mode1→同锚Draw→默认mode0→Pop。
    std::vector<SteamCursorOperation> operations;
};
inline constexpr const char *steam_cursor_image_path="steam-platform/screen_cursor.png";
inline constexpr const char *steam_cursor_sprite_path="steam-platform/screen_cursor.seb";
// 纯调用计划，不合并外层矩阵、不产生Touch。Owner/平台仅在一次真实Draw准入提交next值。
// 维护预算：坐标±65536，实际消费尺寸1..65536；未消费的隐藏字段不校验。
std::optional<SteamCursorSkinPlan> steam_cursor_skin(const SteamCursorSkinInput &input);
} // namespace ark::simulation
