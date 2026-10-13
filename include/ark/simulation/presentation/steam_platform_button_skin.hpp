#pragma once

#include <array>
#include <optional>

namespace ark::simulation {
// SteamInputManager平台按钮，不占用游戏common图片命名空间。
enum class SteamPlatformButtonKind { controller_binding, keyboard_guide };
struct SteamPlatformButtonResource {
    const char *image_path;
    const char *sprite_path;
    int image{}, sprite{};
    std::array<int,4> crop{};
    std::array<int,2> offset{};
};
std::optional<SteamPlatformButtonResource> steam_platform_button_resource(SteamPlatformButtonKind kind);
struct SteamPlatformButtonInput {
    SteamPlatformButtonKind kind{};
    std::array<int,2> anchor{};
    int component{};
    bool desktop{}, xbox{}, touched{}; // 真实平台资格及CheckTouch，不从悬停猜测。
    std::optional<float> scale_ratio; // 仅controller附加glyph消费GetScaleRatio(false)，缺值拒绝。
};
struct SteamPlatformBlend {
    int mode{};
    std::array<int,4> rgba{};
};
struct SteamPlatformJoystickRequest {
    int button{}, registration{}, scale{};
    std::array<int,2> anchor{};
};
struct SteamPlatformButtonBody {
    SteamPlatformButtonResource resource;
    std::array<float,4> destination{};
    std::optional<SteamPlatformBlend> blend; // 空值继承原Graphics混色；高亮BLEND_COLOR1。
    int component{}, value{};
    std::array<int,4> touch_rectangle{}, margin{15,15,15,15};
    std::optional<SteamPlatformJoystickRequest> joystick;
    bool reset_origin{true}; // Draw/注册前PushOrigin(0,0,false)，结束恢复，不叠表单原点。
};
struct SteamPlatformButtonPlan {
    std::optional<SteamPlatformButtonBody> body; // 平台资格不满足是合法隐藏，不生成热区。
};
// 纯资源/几何/注册请求；不打开网页或绑定面板，不改Steam、图形栈、Owner或随机。
// 维护预算：坐标±65536、component0..65535、controller比例(0,10000]；不宣称原规则限值。
std::optional<SteamPlatformButtonPlan> steam_platform_button_skin(const SteamPlatformButtonInput &input);
} // namespace ark::simulation
