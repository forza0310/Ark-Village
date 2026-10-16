#pragma once

#include "dungeon_village_prototype/steam_human_skin.hpp"

namespace dungeon_village_prototype {
// Steam raw12 非日文目录：值始终为P12/P13/J3/J4的实际存储值，不传反向显示索引。
struct SteamConfigSkinInput {
    std::array<int,4> values{};
    int selection{};
    bool speed_unlocked{}; // 原sysSave.int[9]!=0，仅控制高速选项的颜色和注册。
};
// 已解析的当前语言文字。说明两行须对应当前selection；不猜字体宽或私自补默认设置。
struct SteamConfigTextInput {
    std::string title;
    std::array<std::string,4> names;
    std::array<std::array<std::string,2>,2> choices;
    std::array<std::string,2> explanation;
    std::optional<std::array<int,2>> title_widths; // 阴影/正文两次实际StringWidth，缺值拒绝。
};
struct SteamConfigSkinOptions {
    int view_y{};
    std::array<int,2> graphics_origin{}; // 只进入原Drag的绝对坐标，不重复平移图元。
    std::array<std::array<bool,2>,2> arrow_selected{}; // BGM/SE × 左21/右20的CheckTouch结果。
    bool steam_platform{}, on_top{}, platform_keyboard_touched{};
    SteamConfigTextInput text;
};
// GameView.IMAGE_DATA的索引及原DrawImage锚；不是common图片ID。
struct SteamConfigImage { int image{}; std::array<int,2> anchor{}; };
struct SteamConfigImagePlan {
    std::array<int,4> crop{};
    std::array<float,4> destination{};
};
// 仅本页实际消费的4/5/6/7/11；image_width为已加载controls图实际宽度。
std::optional<SteamConfigImagePlan> steam_config_image_skin(const SteamConfigImage &image,
                                                          int image_width);
struct SteamConfigTouch {
    SteamStartupTouch touch; // 已经过本页_addTouch修正的真正注册矩形。
    std::array<int,4> source_rectangle{}; // 调用者传入的原基矩形；滑杆不等于thumb热区。
    std::optional<int> extended_value;
    std::optional<std::array<int,6>> drag; // x/y/w/h/xcells/ycells；Flag见touch.option。
};
// 原helper请求：不将平台键盘图标或浏览器动作伪装成游戏内设置项。
struct SteamConfigPlatformKeyboard { std::array<int,2> anchor{21,51}; int component{2}; bool touched{}; };
using SteamConfigOperation=std::variant<SteamStartupFrameDraw,SteamHumanOverviewText,
    SteamConfigImage,SteamConfigTouch,SteamConfigPlatformKeyboard>;
struct SteamConfigSkinPlan {
    std::string title;
    std::array<int,2> soft_labels{0,2};
    std::vector<SteamConfigOperation> operations;
};
// 有序纯计划：窗口/内框→逐行底色、文字及组件→说明→平台请求。
// 不读取Owner、不修改设置或时钟、不写系统文件；原页面main不参与此绘制。
std::optional<SteamConfigSkinPlan> steam_config_skin(const SteamConfigSkinInput &input,
                                                    const SteamConfigSkinOptions &options);

// 仅已准入的Jp=false且En=true四行Update主体；不模拟通用wait/双钟或Touch即时setter。
struct SteamConfigUpdateInput {
    SteamConfigSkinInput state;
    bool up_repeat{}, down_repeat{}, left_repeat{}, right_repeat{}, back_soft_label{};
};
struct SteamConfigSettingRequest { int row{}, before{}, after{}; }; // 实际存储值，不是反向显示索引。
enum class SteamConfigReturnRequest { save_system, reset_offscreen, pop };
using SteamConfigUpdateOperation=std::variant<SteamConfigSettingRequest,SteamConfigReturnRequest>;
struct SteamConfigUpdatePlan {
    SteamConfigSkinInput candidate;
    std::array<bool,5> checked_keys{}; // 上、下、左、右、soft2；原分支没有确认键查询。
    std::vector<SteamConfigUpdateOperation> operations; // setter在前，再SaveSystem→ResetOffscreen→Pop。
};
// 有序纯请求：真正setter负责声音/画质/速度输出；本接口不重复试听或执行文件/平台操作。
// 返回并非撤销当轮设置；实际执行失败策略仍由应用Owner独立设计，不复制原异常后继续Pop。
std::optional<SteamConfigUpdatePlan> steam_config_english_update(const SteamConfigUpdateInput &input);
} // namespace dungeon_village_prototype
