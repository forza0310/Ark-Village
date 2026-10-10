#pragma once

#include "dungeon_village_prototype/steam_facility_skin.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include <string_view>

namespace dungeon_village_prototype {
// raw3读取的实际查询结果。建设/任务NEW须保留原缓存时点，不能拿数量代替。
struct SteamMainMenuNotices {
    bool construction{}, quest{}, selected_quest{}, equipment{};
    bool commerce_open{}, commerce_items{}, commerce_facilities{}, activities{};
    bool adventurers{}, magic_pot{};
};
// 唯一Owner的只读NEW/GET投影，保留原目录次序与短路；仅实际读取的缺源返回nullopt。
// 不刷新建设缓存、不清已阅标志，不使用商会目录策略或装备页筛选代替原谓词。
std::optional<SteamMainMenuNotices> steam_main_menu_notices(const StartupWorldRuntimeState &state);
enum class SteamMainMenuPackage { common, common2 };
struct SteamMainMenuImage {
    SteamMainMenuPackage package{SteamMainMenuPackage::common};
    int image{}, sprite{-1}, frame{}; // 手形frame=-1取外部当前SEB帧，不推进动画。
    std::array<int,4> crop{}; // sprite<0时为缩短后的源裁片，禁止改作纹理拉伸。
    std::array<int,2> position{};
};
struct SteamMainMenuText {
    int row{}, tag{}; // MENU_STR[tag]，不是子页标题或定义ID。
    std::array<int,2> position{};
    std::array<int,3> rgb{};
};
using SteamMainMenuDraw = std::variant<SteamMainMenuImage,SteamMainMenuText,SteamFacilityNumber>;
struct SteamMainMenuSkinPlan {
    std::array<int,2> origin{};
    std::vector<SteamMainMenuDraw> draws;
    std::vector<SteamStartupTouch> touches;
};
struct SteamMainMenuSkinInput {
    int frame{}, selection{};
    bool magic_unlocked{}; // 原flag1，Init决定是否插入tag3；不由期间栏可见性决定。
    std::optional<SteamMainMenuNotices> notices; // frame>=3必需，缺来源不能补全false。
    std::optional<StartupMagicPotMenuInformation> magic_period; // tag3绘制时必需，0表示隐藏。
};
struct SteamMainMenuSkinOptions {
    std::array<int,2> canvas{240,240}, origin{}; // 实际Graphics原点，不能当父页存储位置。
    int safe_left{};
    bool japanese{}, english{}, on_top{true}, top_is_main_menu{true};
    bool covered_by_nonmenu_subform{};
};
// Steam DrawMenu2(type0)逐行局部计划。真实目录/选择/计数及NEW由控制器提供。
// 不创建raw3、不消费光标请求、声音或随机；HUD、Review与共尾KEYCLICK由外层依原序追加。
// top_is_main_menu控制ID8注册，on_top只控制手形，两种原资格不得合并。
std::optional<SteamMainMenuSkinPlan> steam_main_menu_skin(
    const SteamMainMenuSkinInput &input,const SteamMainMenuSkinOptions &options);
// 仅raw4/7/9/10父子定位：使用raw3存储位置，不使用safe-left/居中后的绘制原点。
std::optional<std::array<int,2>> steam_main_menu_child_position(
    std::array<int,2> stored_parent,int selection,bool magic_unlocked,bool english);
// 固定Steam条目路径。common2/image0有独立差异PNG，其它明确复用同字节副本。
std::optional<std::string_view> steam_main_menu_image(SteamMainMenuPackage package,int image);
} // namespace dungeon_village_prototype
