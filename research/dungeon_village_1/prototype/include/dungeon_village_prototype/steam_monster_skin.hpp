#pragma once

#include "dungeon_village_prototype/steam_human_skin.hpp"
#include "dungeon_village_prototype/steam_information_skin.hpp"
#include "dungeon_village_prototype/steam_commerce_skin.hpp"

namespace dungeon_village_prototype {
// 怪物窗口的正常显示身体。图片、SEB和帧是独立身份，不按PNG列数推断。
struct SteamMonsterBody {
    int image{}, sprite{}, frame{};
    std::array<int,2> position{};
};
enum class SteamMonsterAsset { money };
std::optional<SteamFacilityResource> steam_monster_resource(SteamMonsterAsset);
struct SteamMonsterImage {
    SteamMonsterAsset asset;
    std::array<int,2> position{};
    int frame{};
};
using SteamMonsterOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamHumanOverviewText,SteamHumanClip,SteamMonsterBody,SteamStartupTouch,
    SteamFacilityImage,SteamFacilityNumber,SteamInformationNumber,SteamMonsterImage,SteamCommerceImage>;
struct SteamMonsterSkinPlan {
    std::string title;
    std::vector<SteamMonsterOperation> operations;
    std::array<int,2> soft_labels{0,0};
};
struct SteamMonsterSkinOptions {
    int view_y{};
    bool japanese{}; // 标题等false采用APK中文适配；92统计后备始终是原静态日文。
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> name_width; // raw92仅Lv>=2消费，当前Font实际测宽，禁止按字数估算。
    // 原非JP先LT再按空格分段；省略/缺两段时保留Steam静态日文后备及其几何。
    std::optional<std::array<std::string,2>> statistic_labels;
};
struct SteamMonsterIntroductionInput {
    int counter{},body{},variant{},shadow{};
    std::string name; // 调用方真实定义名／已核翻译，不能由图片身份猜文字。
};
// raw92当前共享定义查询；等级与收益由现有成长规则投影，非世界实例HP缓存。
struct SteamMonsterDetailInput {
    int counter{}, body{}, variant{}, shadow{};
    int defeats{}, human_downs{}, level{1}, experience{}, cash{}, points{}; // defeats为累计k.v，不是月计k.u。
    std::string name, description;
};
// raw89正常非调试显示；仅生成有序计划，不更新共享scratch、世界或随机。
// 完整共享显示附效的适用前提见rules/STEAM_MONSTER_PROGRESSION.md。
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const SteamMonsterIntroductionInput &,const SteamMonsterSkinOptions &);
// 严格读取真实栈顶89与现主钟；未Init、缺定义／载荷及已退休页显式拒绝。
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const StartupWorldRuntimeState &,std::uint64_t page,const SteamMonsterSkinOptions &);
std::optional<SteamMonsterSkinPlan> steam_monster_detail_skin(
    const SteamMonsterDetailInput &,const SteamMonsterSkinOptions &);
std::optional<SteamMonsterSkinPlan> steam_monster_detail_skin(
    const StartupWorldRuntimeState &,std::uint64_t page,const SteamMonsterSkinOptions &);
} // namespace dungeon_village_prototype
