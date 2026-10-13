#pragma once

#include "ark/simulation/presentation/steam_human_skin.hpp"

namespace ark::simulation {
// raw82仅展开已核口碑演出，不执行脚本40、声音4或人气I队列。
enum class SteamFacilityReputationAsset { secretary };
std::optional<SteamHumanResource> steam_facility_reputation_resource(SteamFacilityReputationAsset asset);
struct SteamFacilityReputationImage {
    SteamFacilityReputationAsset asset;
    std::array<int,2> position{};
    int frame{};
};
using SteamFacilityReputationOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamFacilityImage,SteamFacilityNumber,SteamFacilityMapchip2,SteamFacilityClip,
    SteamHumanPortrait,SteamHumanOverviewText,SteamFacilityReputationImage,SteamStartupTouch>;
struct SteamFacilityReputationPlan {
    std::vector<SteamFacilityReputationOperation> operations;
    std::array<int,2> soft_labels{0,0};
    std::string title{"传说中的商店"};
};
struct SteamFacilityReputationParticipant {
    int definition{}, body_image{}; // 当前职业／性别的图片；冻结的是定义引用，不冻结职业。
};
struct SteamFacilityReputationInput {
    int definition{}, mapchip{}, phase{}, frame{}, frame2{}, view_y{};
    std::string name;
    std::vector<SteamFacilityReputationParticipant> participants; // 原序1..3，不能重新抽签／排序。
    bool japanese{}, language8{}; // Steam GetLanguage()==8独立分支，不将其推定为APK非JP布局。
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> increase_suffix_width; // phase1实际StringWidth；数字宽度是GetFig(20)*10。
};
// 帧0仍生成零高clip；frame2与phase计数独立。人物无武器，固定影子先于身体。
// 不接共享scratch的历史附加效果，不把本基础计划宣称完整Draw_human子树。
std::optional<SteamFacilityReputationPlan> steam_facility_reputation_skin(
    const SteamFacilityReputationInput &input);
struct SteamFacilityReputationOptions {
    int view_y{};
    bool japanese{}, language8{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> increase_suffix_width;
};
// 只接受真实已初始化顶页82，并读取Owner独立page_secondary_counters；缺键明确拒绝。
// 不初始化、推进计数、发声音／随机或提前提交+20。
std::optional<SteamFacilityReputationPlan> steam_facility_reputation_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamFacilityReputationOptions &options);
} // namespace ark::simulation
