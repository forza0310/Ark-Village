#pragma once

#include "dungeon_village_prototype/startup_skin.hpp"
#include "dungeon_village_prototype/startup_title_actor_skin.hpp"
#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include <array>
#include <optional>
#include <variant>
#include <vector>

namespace dungeon_village_prototype {
// Steam raw60 的局部表现；资源身份独立核验，不按 APK 同名推定相等。
enum class SteamHumanAsset {
    medal, pinch, bubble, number05, portrait_background, home_background,
    experience_bar, experience_label, experience_remaining, smile, combat_icon,
    equipment_background, weapon_icon, armour_icon, accessory_icon
};
struct SteamHumanResource {
    int image{}, sprite{-1};
    const char *published_image;
    const char *published_sprite;
};
std::optional<SteamHumanResource> steam_human_resource(SteamHumanAsset asset);
struct SteamHumanImage {
    SteamHumanAsset asset;
    std::array<int,2> position{};
    int frame{};
    // 有裁片直接取 PNG；否则读取资源对应 SEB，内部偏移只加一次。
    std::optional<std::array<int,4>> crop;
};
enum class SteamHumanTextRole { down };
struct SteamHumanText {
    SteamHumanTextRole role;
    std::array<int,2> position{};
    int anchor{};
    std::array<int,3> rgb{};
};
using SteamHumanDraw = std::variant<StartupSkinRect,SteamHumanImage,SteamHumanText>;
struct SteamHumanSkinPlan { std::vector<SteamHumanDraw> draws; };
struct SteamHumanHpInput {
    int now{}, after{}, maximum{};
};
// 锚点已包含 MOVE_DATA 插值及住宅偏移。仅普通肖像调用；state7由父页另走倒下分支。
// 无实例必须传 nullopt；maximum由当前共享定义 GetHpMax 对应值提供，不改 HP 六槽。
// 保留有符号 HP，长度夹限但危险判断仍读取 after；不推进世界／表现计数。
std::optional<SteamHumanSkinPlan> steam_human_power_skin(
    std::array<int,2> position, int direction, int frame,
    std::optional<SteamHumanHpInput> live);
// 默认调用锚是(83,70)；5枚起复用已核 number05 数字 helper，不能写字体乘号。
std::optional<SteamHumanSkinPlan> steam_human_medal_skin(int medals,
    std::array<int,2> position = {83,70});
// 默认倒下文字锚基点(75,79)，宽度由实际字体 StringWidth 提供。
// 4096为维护输出预算，不是原字体或原游戏字符串上限。
std::optional<SteamHumanSkinPlan> steam_human_down_bubble_skin(int measured_width,
    std::array<int,2> position = {75,79});
struct SteamHumanDetailSkin {
    SteamHumanSkinPlan medals; // raw60共用区中，先奖章再人物助手。
    std::array<int,2> portrait_position{};
    StartupTitleActorSkin portrait; // 武器(若有)→身体；本页不加标题阴影。
    SteamHumanSkinPlan after_portrait; // 普通为HP/危险图标，倒下为气泡/文字。
};
// 当前职业/性别/武器和实际实例来自只读Owner view；显式W优先，否则取同定义首实例。
// 不推进页面或共享scratch。
// down_text_width只在state7使用，必须来自实际字体测宽；其它情况不需要测宽。
// 这里只展开已核基础肖像，不认证原共享scratch任意历史残留的附加效果。
std::optional<SteamHumanDetailSkin> steam_human_detail_skin(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    std::optional<int> down_text_width = {});
// raw60概览：共用区与页0有序输出；其它页独立后继，不伪造完整四页皮肤。
enum class SteamHumanOverviewTextRole { name, profession, satisfaction, effort, gift };
struct SteamHumanOverviewText {
    SteamHumanOverviewTextRole role;
    std::string value;
    std::array<int,4> rectangle{};
    int anchor{}, font_size{};
    bool layout{};
    std::array<int,3> rgb{63,44,4};
};
struct SteamHumanOverviewScroll {
    std::array<StartupSkinRect,2> rectangles;
    std::array<int,4> thumb_touch; // 12的基矩形，25仍使用(221,69,3,127)。
};
using SteamHumanOverviewOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamFacilityImage,SteamFacilityNumber,SteamHumanOverviewText,SteamHumanDetailSkin,
    SteamStartupTouch,SteamHumanOverviewScroll>;
struct SteamHumanOverviewPlan {
    std::vector<SteamHumanOverviewOperation> operations;
    std::array<int,2> soft_labels{0,2};
    bool recovery_gift{};
};
struct SteamHumanOverviewOptions {
    int view_y{}, finger_frame{};
    bool japanese{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> profession_width, profession_repeat_width; // 两次均在原改字号之前。
    std::array<std::optional<int>,2> bottom_widths; // 普通礼物／恢复提示的实际StringWidth。
    std::optional<int> down_text_width;
};
// 只读已初始化raw60页0；无W仍绘定义，坏装备/库存依赖显式拒绝。
// 不重算成长/HP、初始化、推进计数、抽随机、发声或提交追踪。
std::optional<SteamHumanOverviewPlan> steam_human_overview_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamHumanOverviewOptions &options);
} // namespace dungeon_village_prototype
