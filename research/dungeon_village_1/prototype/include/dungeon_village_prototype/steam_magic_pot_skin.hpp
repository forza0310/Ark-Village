#pragma once

#include "dungeon_village_prototype/steam_facility_effect_skin.hpp"

namespace dungeon_village_prototype {
// Steam魔法壶41—43有序只读表现；投入、处理、发现、炼制仍由唯一世界Owner提交。
enum class SteamMagicPotAsset { background, character, pot, bar, fire, ladder, element };
std::optional<SteamHumanResource> steam_magic_pot_resource(SteamMagicPotAsset asset);
struct SteamMagicPotImage {
    SteamMagicPotAsset asset;
    std::array<int,2> position{};
    int frame{};
    std::optional<std::array<int,4>> crop;
};
struct SteamMagicPotText {
    std::string value;
    std::array<int,4> rectangle{};
    int anchor{}, font_size{};
    bool layout{}, rich{};
    std::array<int,3> rgb{92,51,31};
};
// 原壶helper会替换clip，不能把SetClip当作栈上交集。
enum class SteamMagicPotClipKind { push_intersect, replace, pop };
struct SteamMagicPotClip { SteamMagicPotClipKind kind; std::array<int,4> rectangle{}; };
using SteamMagicPotOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamFacilityImage,SteamFacilityNumber,SteamHumanPortrait,
    SteamFacilityEffectImage,SteamMagicPotImage,SteamMagicPotText,SteamMagicPotClip,
    SteamStartupTouch,SteamFacilityEffectTouch,SteamHumanCatalogueScroll,SteamFacilityArrowTouch>;
struct SteamMagicPotPlan {
    std::vector<SteamMagicPotOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{0,2};
};
struct SteamMagicPotOptions {
    int view_y{}, finger_frame{}, arrow_count{};
    bool japanese{}, scroll_first_touch{};
    bool english{}; // Steam42另分en-US：素材标签追加" Left"，其它非JP不追加。
    std::optional<std::array<int,2>> title_widths;
    std::array<std::optional<int>,2> button_widths; // 41真实StringWidth，helper夹60。
    std::optional<int> hint_width; // 41底部说明真实测宽，helper夹200。
};
struct SteamMagicPotItemRow {
    int identity{}, inventory{};
    std::string name;
    std::vector<StartupVisualDraw> icon;
    std::array<int,4> elements{};
};
struct SteamMagicPotRecipeRow {
    int identity{}, reward_kind{}, reward_definition{}, inventory{};
    bool discovered{}, owned{}, affordable{};
    std::string name;
    std::vector<StartupVisualDraw> icon;
    std::array<int,4> cost{};
};
struct SteamMagicPotSkinInput {
    int raw{41}, selection{}, first{}, tab{}, counter{};
    std::array<int,13> parameters{}; // 唯一Owner原n，不另持久存壶状态。
    std::vector<SteamMagicPotItemRow> items;
    std::vector<SteamMagicPotRecipeRow> recipes;
    SteamMagicPotOptions options;
};
// Draw_magicPot(speed0)的已证mode0全景／mode1投入壶身，供41与44／45共用。
std::optional<SteamMagicPotPlan> steam_magic_pot_body_skin(
    int level,int frame,std::array<int,2> position,int mode);
std::optional<SteamMagicPotPlan> steam_magic_pot_skin(const SteamMagicPotSkinInput &input);
struct StartupWorldRuntimeState;
// 仅投影完整且已初始化的真实栈顶41／42／43，不处理菜单入口或补齐坏载荷。
std::optional<SteamMagicPotPlan> steam_magic_pot_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamMagicPotOptions &options);
} // namespace dungeon_village_prototype
