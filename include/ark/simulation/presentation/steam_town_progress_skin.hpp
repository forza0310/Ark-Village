#pragma once

#include "ark/simulation/presentation/steam_facility_effect_skin.hpp"

namespace ark::simulation {
// Steam48/49条件、50三段晋级、95奖励的原序纯表现。领取、声音与随机只由Owner执行。
enum class SteamTownAsset { star, rank_star, rank_frame, award, president_carry, shadow,
    celebration_background, reward_background, mini1, mini2, move_facility, residence_icon, activity_icon };
struct SteamTownResource {
    const char *group;
    int image{}, sprite{-1};
    const char *published_image, *published_sprite;
};
std::optional<SteamTownResource> steam_town_resource(SteamTownAsset asset);
struct SteamTownImage {
    SteamTownAsset asset;
    std::array<int,2> position{};
    int frame{};
    std::optional<std::array<int,4>> crop;
};
// SetClip替换当前矩形，区别于ClipRect的栈上交集。执行器必须按顺序消费。
enum class SteamTownClipKind { push_intersect, replace, pop };
struct SteamTownClip { SteamTownClipKind kind; std::array<int,4> rectangle{}; };
struct SteamTownText {
    std::string value;
    std::array<int,4> rectangle{};
    int anchor{}, font_size{};
    bool rich{}, layout{};
    std::array<int,3> rgb{92,51,31};
};
using SteamTownOperation=std::variant<SteamStartupFrameDraw,SteamTownImage,SteamTownText,
    SteamTownClip,SteamFacilityImage,SteamFacilityNumber,SteamFacilityMapchip2,
    SteamHumanImage,SteamHumanPortrait,SteamStartupTouch,SteamFacilityEffectTouch>;
struct SteamTownProgressPlan {
    std::vector<SteamTownOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{0,2};
};
struct SteamTownProgressOptions {
    int view_y{}, finger_frame{};
    bool japanese{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> condition_suffix_width, bottom_width, promoted_suffix_width, reward_number_width;
    // 恭喜／太好／万岁／请；只要求当前实际绘制的字符串测宽，不能按汉字数猜宽。
    std::array<std::optional<int>,4> bubble_widths;
};
struct SteamTownConditionRow {
    int type{}, current{}, required{};
    bool met{};
    std::string facility_name; // type3从指定设施定义取得，不能从现地图猜。
};
struct SteamTownConditionsInput {
    int raw_page{48}, rank{}, selection{};
    std::string village_name;
    std::array<SteamTownConditionRow,4> rows;
    SteamTownProgressOptions options;
};
std::optional<int> steam_town_conditions_cursor(int raw_page,int selection,bool up,bool down);
std::optional<SteamTownProgressPlan> steam_town_conditions_skin(const SteamTownConditionsInput &input);
struct SteamTownCelebrationHuman {
    int definition{}, profession{}, sex{};
    std::array<int,4> frozen{}; // 原X[1..4]：已+150的x、y、方向、层。
};
struct SteamTownCelebrationInput {
    int rank{1}, phase{}, frame{}, frame2{};
    std::vector<SteamTownCelebrationHuman> participants;
    SteamTownProgressOptions options;
};
// X不随演出位移写回；phase2分别绘后层1、前层0。不认证共享scratch历史残留。
std::optional<SteamTownProgressPlan> steam_town_celebration_skin(const SteamTownCelebrationInput &input);
struct SteamTownGiftInput {
    int kind{}, value{}, sex{}, frame{}, mapchip{}, activity_icon{};
    std::uint32_t activity_flags{};
    std::string name;
    SteamTownProgressOptions options;
};
// 仅既有七类0/1/3/4/9/10/11，r4 value为职业定义；不接受尚未维护的其它奖励。
std::optional<SteamTownProgressPlan> steam_town_gift_skin(const SteamTownGiftInput &input);
// 只读真实已初始化栈顶。缺计数、错绑定、退休页均拒绝，不在查询中补初始化。
std::optional<SteamTownProgressPlan> steam_town_conditions_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamTownProgressOptions &options);
std::optional<SteamTownProgressPlan> steam_town_celebration_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamTownProgressOptions &options);
std::optional<SteamTownProgressPlan> steam_town_gift_skin(
    const StartupWorldRuntimeState &state,std::uint64_t page,const SteamTownProgressOptions &options);
} // namespace ark::simulation
