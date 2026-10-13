#pragma once

#include "ark/simulation/presentation/steam_facility_effect_skin.hpp"

namespace ark::simulation {
// Steam51—54的纯绘制合同。业务、随机抽取、两提交时点均留在唯一Owner。
enum class SteamVillageActivityAsset { season, activity_icon, activity_picture, picture_background };
std::optional<SteamHumanResource> steam_village_activity_resource(SteamVillageActivityAsset asset);
struct SteamVillageActivityImage {
    SteamVillageActivityAsset asset;
    std::array<int,2> position{};
    int frame{};
    std::optional<std::array<int,4>> crop;
};
// rich仅原DrawString2颜色标记，普通文字及TextLayout不解析标记。
struct SteamVillageActivityText {
    std::string value;
    std::array<int,4> rectangle{};
    int anchor{}, font_size{};
    bool layout{}, rich{};
    std::array<int,3> rgb{92,51,31};
};
using SteamVillageActivityOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamFacilityImage,SteamFacilityNumber,SteamHumanOverviewText,SteamHumanPortrait,
    SteamFacilityClip,SteamStartupTouch,SteamHumanEquipmentRow,SteamHumanCatalogueScroll,
    SteamVillageActivityImage,SteamVillageActivityText,SteamFacilityEffectTouch>;
struct SteamVillageActivityPlan {
    std::vector<SteamVillageActivityOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{0,2};
};
struct SteamVillageActivityDefinition {
    int identity{}, picture{}, icon{}, kind{}, attribute{}, cost{}, magnitude{};
    std::string name, detail, description;
    std::uint32_t flags{};
    bool newly_unlocked{};
};
struct SteamVillageActivityHuman {
    int identity{}, profession{}, sex{}, current{}, previous{};
    std::string name;
};
struct SteamVillageActivityOptions {
    int view_y{}, finger_frame{}, arrow_count{};
    bool japanese{}, english{}, thai{}, original_language{}, scroll_first_touch{};
    std::optional<std::array<int,2>> title_widths;
    // 51属性提示/53属性结果中前缀、属性名称、后缀的三次实际测宽；未知不猜。
    std::array<std::optional<int>,3> attribute_widths;
    std::optional<int> description_width, increase_suffix_width;
    std::array<std::optional<int>,2> button_widths;
};
struct SteamVillageActivitySkinInput {
    int raw{51}, selection{}, first{}, counter{}, season{}, quarter_remaining{}, points{}, display_page{};
    std::vector<SteamVillageActivityDefinition> activities;
    std::optional<SteamVillageActivityDefinition> activity; // 52/53/54冻结绑定。
    std::vector<SteamVillageActivityHuman> humans; // 54原序冻结目录。
    std::array<SteamVillageActivityHuman,2> display_humans; // 51/54原有放回抽签身份。
    SteamVillageActivityOptions options;
};
// 单纯投影与有序helper请求；不推进页面/角色计数，不消费声音或清NEW。
std::optional<SteamVillageActivityPlan> steam_village_activity_skin(const SteamVillageActivitySkinInput &input);
struct StartupWorldRuntimeState;
std::optional<SteamVillageActivityPlan> steam_village_activity_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamVillageActivityOptions &options);
} // namespace ark::simulation
