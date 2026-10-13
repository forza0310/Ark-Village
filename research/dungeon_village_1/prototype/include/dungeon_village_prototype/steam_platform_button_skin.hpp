#pragma once

#include <array>
#include <optional>

namespace dungeon_village_prototype {
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

// Kairolib._drawButtons未取得动态图像时的原SEB后备；不属于平台绑定/键盘入口按钮。
struct SteamControllerButtonResource {
    const char *image_path;
    const char *sprite_path;
    int image{}, sprite{}; // managed kairolib组的原INF索引，不是游戏common索引。
};
struct SteamControllerButtonFallbackInput {
    int input_type{}, frame{}, layer{}; // input_type为原Steam ESteamInputType值，未匹配者用Switch。
    bool auto_key_reverse{};
    std::optional<int> okay_frame, cancel_frame; // 仅原frame0/1实际消费对应Canvas映射时必填。
};
struct SteamControllerButtonFallbackPlan {
    SteamControllerButtonResource resource;
    int frame{}, layer{};
};
// 先按原frame互斥反转，再仅对原layer1的16..19映射32..35；所有层最终归0。
// 已出版SEB的维护范围为frame0..43，坏请求拒绝；不发现设备、不调用Steam或推测当前映射。
// 调用者须先完成源动态图像优先分支；frame7无控制器时的right_click路径不是此后备。
std::optional<SteamControllerButtonFallbackPlan> steam_controller_button_fallback(
    const SteamControllerButtonFallbackInput &input);
} // namespace dungeon_village_prototype
