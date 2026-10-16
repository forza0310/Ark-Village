#pragma once

#include "ark/simulation/presentation/steam_monster_skin.hpp"
#include "ark/simulation/presentation/startup_world_presentation.hpp"

namespace ark::simulation {
// Steam UserData.DungeonBar_draw 的正常绑定路径；调用者先向唯一Owner请求表现，
// 再把同次冻结抖动与显示事实交给本模块。无任务／非探索任务返回空计划。
struct SteamDungeonFooterChallenge {
    std::array<int,6> record{};
    // 仅类型1读取：来自实际MonsterData，不由第五槽ID推图片或猜faceIconSeb_。
    int monster_image{-1},monster_sprite{-1};
};
struct SteamDungeonFooterHuman {
    int progress{},state{},counter{},body_image{-1};
};
struct SteamDungeonFooterTask {
    int kind{},background_row{},counter{},progress{},maximum{};
    std::uint64_t facility{};
    std::vector<SteamDungeonFooterChallenge> challenges;
    std::vector<SteamDungeonFooterHuman> humans;
    std::vector<std::array<int,3>> effects; // phase,x,y；y已经是显示坐标。
};
struct SteamDungeonFooterInput {
    std::array<int,4> view{}; // VIEW_X/Y/W/H，W/H为原宽高，不是右下端点。
    int side_width{};
    bool japanese{};
    std::uint64_t page{};
    std::optional<SteamDungeonFooterTask> task;
    StartupPresentationPlan presentation; // 原序且仅消费本page/facility的jitter。
};
using SteamDungeonFooterOperation=std::variant<StartupSkinDraw,StartupSkinRect,
    SteamHumanClip,SteamHumanPortrait,SteamMonsterBody,SteamFacilityNumber>;
struct SteamDungeonFooterPlan {
    std::vector<SteamDungeonFooterOperation> operations;
};
// 源矩形保留Steam未消费ClampMax返回值的事实，允许负sx；不得擅自修成APK min。
// 这里只交原请求，越界最终像素和共享scratch附效仍未认证。
// 几何/向量预算是维护拒绝边界，不是原游戏上限；失败不返回部分图元。
std::optional<SteamDungeonFooterPlan> steam_dungeon_footer_skin(const SteamDungeonFooterInput &);
// 当前只绑定已核Steam根资源；语言替换由宿主另选，不能把APK中文当Steam语言资源。
std::optional<SteamFacilityResource> steam_dungeon_footer_resource(int common_image);
// 同次Owner表现提交后的真实事实；视口/侧软键宽度/语言仍为显式平台上下文。
// 不补缺绑定、不消费dungeon_labels、不重建随机；调用方不得夹入后续世界更新。
std::optional<SteamDungeonFooterPlan> steam_dungeon_footer_skin(
    const StartupWorldRuntimeState &,std::uint64_t page,const StartupPresentationPlan &,
    std::array<int,4> view,int side_width,bool japanese);
} // namespace ark::simulation
