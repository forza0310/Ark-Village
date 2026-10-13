#pragma once

#include "ark/simulation/presentation/steam_human_skin.hpp"

namespace ark::simulation {
// Steam75/76/77的有序只读计划。目录光标由Owner/输入适配提供，绘制不消费道具或声音。
enum class SteamFacilityEffectAsset { response, throw_flash };
struct SteamFacilityEffectResource {
    const char *group;
    int image{}, sprite{};
    const char *published_image, *published_sprite;
};
std::optional<SteamFacilityEffectResource> steam_facility_effect_resource(SteamFacilityEffectAsset asset);
struct SteamFacilityEffectImage {
    SteamFacilityEffectAsset asset;
    std::array<int,2> position{};
    int frame{};
};
struct SteamFacilityEffectText {
    std::string value;
    std::array<int,4> rectangle{};
    int anchor{}, font_size{};
    bool layout{};
    std::array<int,3> rgb{92,51,31};
};
struct SteamFacilityEffectTouch {
    SteamStartupTouch touch;
    std::array<int,4> margin{};
};
struct SteamFacilityEffectScroll {
    SteamFacilityScrollSkinPlan skin;
    SteamStartupTouch touch;
    std::array<int,4> margin{3,20,0,0};
    std::array<int,3> arguments{}; // count、visible、0x20000；组件12绑定0x40000滚动。
    SteamStartupTouch track_touch;
};
using SteamFacilityEffectOperation=std::variant<SteamStartupFrameDraw,SteamHumanImage,
    SteamFacilityImage,SteamFacilityNumber,SteamFacilityMapchip2,SteamFacilityClip,
    SteamHumanPortrait,SteamFacilityEffectImage,SteamFacilityEffectText,
    SteamStartupTouch,SteamFacilityEffectTouch,SteamFacilityEffectScroll>;
struct SteamFacilityEffectPlan {
    std::vector<SteamFacilityEffectOperation> operations;
    std::array<int,2> soft_labels{0,2};
    std::string title{"设施强化"};
};
struct SteamFacilityItemRow {
    int definition{}, inventory{};
    std::string name;
    std::vector<StartupVisualDraw> icon;
    std::array<int,3> improvements{}; // 原k三槽，不按当前适配倍率重算。
};
struct SteamFacilityItemSkinInput {
    std::vector<SteamFacilityItemRow> items; // 已初始化的原序正库存目录。
    SteamFacilityBonusCursor cursor;
    int finger_frame{}, arrow_count{}, view_y{};
    bool japanese{}, scroll_first_touch{};
    std::optional<std::array<int,2>> title_widths;
};
// 原75先合成up/down，再只修选择窗；允许已合法载荷的尾窗不足五行。
std::optional<SteamFacilityBonusCursor> steam_facility_item_cursor(
    int count,SteamFacilityBonusCursor cursor,bool up,bool down);
std::optional<SteamFacilityEffectPlan> steam_facility_item_skin(const SteamFacilityItemSkinInput &input);
struct SteamFacilityImprovementSkinInput {
    int raw_page{76}, frame{}, mapchip{}, profession{}, sex{}, response{};
    std::vector<StartupVisualDraw> item_icon; // 76需要；77不读取旧道具库存。
    std::array<std::array<int,3>,3> attributes{}; // 每属性[前、后、差]，冻结Owner载荷。
    std::array<int,3> limits{};
    int view_y{};
    bool japanese{};
    std::optional<std::array<int,2>> title_widths;
};
std::optional<SteamFacilityEffectPlan> steam_facility_improvement_skin(const SteamFacilityImprovementSkinInput &input);
struct SteamFacilityEffectOptions {
    int finger_frame{}, arrow_count{}, view_y{};
    bool japanese{}, scroll_first_touch{};
    std::optional<std::array<int,2>> title_widths;
};
// 仅接受真实已初始化75/76/77，拒绝缺载荷/错误绑定；75首行只读Owner。
std::optional<SteamFacilityEffectPlan> steam_facility_effect_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamFacilityEffectOptions &options);
} // namespace ark::simulation
