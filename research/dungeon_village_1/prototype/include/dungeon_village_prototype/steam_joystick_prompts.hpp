#pragma once

#include "dungeon_village_prototype/steam_platform_button_skin.hpp"
#include "dungeon_village_prototype/steam_cursor_skin.hpp"

namespace dungeon_village_prototype {
// 固定Steam x86样本Joystick.IsShown；不是HasController或提示登记表是否非空。
// 原Config.get_Platform恒4：编辑器直接true，否则需真实Canvas.lastInputDevice_。
// 缺Canvas观测明确拒绝；0(Unknown)/4(Joystick)真，其余完整signed32值假。
std::optional<bool> steam_joystick_is_shown(
    bool unity_editor,const std::optional<std::int32_t> &last_input_device);
// Canvas平台提示登记表，独立于世界/人物/游戏存档；按原插入顺序保留旧项。
struct SteamJoystickPromptEntry {
    int button{}, registration{-1};
    bool displayed{};
    std::array<int,2> anchor{};
    int life{4}, scale{100};
};
struct SteamJoystickPromptState {
    bool table_present{};
    std::vector<SteamJoystickPromptEntry> entries;
};
struct SteamJoystickRegisterInput {
    SteamPlatformJoystickRequest request;
    int application_type{};
    bool has_controller{}, steam_offset_enabled{}, top_form_present{};
    std::optional<std::string> right_soft_label;
    std::optional<std::array<int,2>> screen_size, application_size;
};
struct SteamJoystickRegisterResult {
    SteamJoystickPromptState state;
    bool registered{};
    std::optional<int> auto_cancel_button; // 仅新项且顶页存在，0..3的真实后继请求。
};
// 维护预算4096项、button0..43、registerID>=-1、life0..4、scale0..10000；非原硬上限。
// 实际type4尺寸1..65536；坐标加偏移溢出拒绝。合法早退不读取后续字段。
std::optional<SteamJoystickRegisterResult> steam_joystick_register(
    const SteamJoystickPromptState &,const SteamJoystickRegisterInput &);
std::optional<SteamJoystickPromptState> steam_joystick_reset(const SteamJoystickPromptState &);
std::optional<SteamJoystickPromptState> steam_joystick_raise(const SteamJoystickPromptState &);
// 原已解析GetJoystickUp(0..23)结果；相同button的所有可见registration都减少，不消费OS输入。
std::optional<SteamJoystickPromptState> steam_joystick_release(
    const SteamJoystickPromptState &,const std::array<bool,24> &released);

struct SteamJoystickDrawInput {
    int application_type{}, cursor_draw_frame{};
    std::optional<float> scale_ratio; // 仅type4读取；空表仍有外层缩放请求。
    bool use_steam{}, xbox{}, has_controller{};
    std::optional<std::string> right_soft_label;
    std::optional<std::array<int,2>> right_click_counts; // button7的返回/菜单有效life，不覆盖登记life。
};
struct SteamJoystickDrawButton {
    std::array<int,2> anchor{};
    int button{}, layer{};
    bool auto_key_reverse{true};
};
// 借用光标模块的通用图形操作载体，不调用光标更新或重排矩阵。
// PushScale作用局部图元；DrawButtons的anchor由原worldMatrix最后平移，不随局部缩放。
using SteamJoystickDrawOperation=std::variant<SteamCursorPushScale,SteamCursorRenderMode,
    SteamJoystickDrawButton,SteamCursorPopMatrix>;
struct SteamJoystickDrawPlan { std::vector<SteamJoystickDrawOperation> operations; };
// 只交有序DrawButtons请求；动态图、无设备跳过和原包后备仍由原下层provider决定。
// 所有入口仅返回私有候选，不改调用者状态、World Owner、随机或持久格式。
std::optional<SteamJoystickDrawPlan> steam_joystick_draw(
    const SteamJoystickPromptState &,const SteamJoystickDrawInput &);
} // namespace dungeon_village_prototype
