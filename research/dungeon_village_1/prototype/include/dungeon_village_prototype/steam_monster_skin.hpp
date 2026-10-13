#pragma once

#include "dungeon_village_prototype/steam_human_skin.hpp"

namespace dungeon_village_prototype {
// 怪物窗口的正常显示身体。图片、SEB和帧是独立身份，不按PNG列数推断。
struct SteamMonsterBody {
    int image{}, sprite{}, frame{};
    std::array<int,2> position{};
};
using SteamMonsterOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamHumanOverviewText,SteamHumanClip,SteamMonsterBody,SteamStartupTouch>;
struct SteamMonsterSkinPlan {
    std::string title;
    std::vector<SteamMonsterOperation> operations;
    std::array<int,2> soft_labels{0,0};
};
struct SteamMonsterSkinOptions {
    int view_y{};
    bool japanese{}; // false采用固定汉化APK字面，不认证Steam中文翻译。
    std::optional<std::array<int,2>> title_widths;
};
struct SteamMonsterIntroductionInput {
    int counter{},body{},variant{},shadow{};
    std::string name; // 调用方真实定义名／已核翻译，不能由图片身份猜文字。
};
// raw89正常非调试显示；仅生成有序计划，不更新共享scratch、世界或随机。
// 完整共享显示附效的适用前提见rules/STEAM_MONSTER_PROGRESSION.md。
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const SteamMonsterIntroductionInput &,const SteamMonsterSkinOptions &);
// 严格读取真实栈顶89与现主钟；未Init、缺定义／载荷及已退休页显式拒绝。
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const StartupWorldRuntimeState &,std::uint64_t page,const SteamMonsterSkinOptions &);
} // namespace dungeon_village_prototype
