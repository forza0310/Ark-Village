#pragma once

#include "dungeon_village_prototype/steam_task_selection_skin.hpp"

namespace dungeon_village_prototype {
// Steam任务25／26参加者、27追加征集、28出发：只读冻结页面输入。
enum class SteamTaskRosterAsset { departure_background, secretary_face, shadow };
std::optional<SteamFacilityResource> steam_task_roster_resource(SteamTaskRosterAsset);
struct SteamTaskRosterImage {
    SteamTaskRosterAsset asset;
    std::array<int,2> position{};
    std::optional<std::array<int,4>> crop;
    int frame{};
};
struct SteamTaskRosterTouch {
    SteamStartupTouch touch;
    std::array<int,4> margin{}; // left/right/top/bottom，25/26为0,-22,0,0；27为29,29,0,0。
};
struct SteamTaskDepartureActor {
    int profession{},sex{},weapon{-1},action{},step{};
    std::array<int,2> position{};
};
// 28只请求action0或action4/step0/facing2；阴影是独立且先消费的图元。
std::optional<StartupTitleActorSkin> steam_task_departure_actor_skin(const SteamTaskDepartureActor &);
using SteamTaskRosterOperation=std::variant<SteamStartupFrameDraw,SteamTaskSelectionText,
    SteamTaskSelectionClip,SteamHumanImage,SteamHumanPortrait,SteamFacilityImage,
    SteamFacilityNumber,SteamHumanCatalogueScroll,SteamFacilityArrowTouch,SteamStartupTouch,
    SteamTaskRosterImage,SteamTaskRosterTouch,SteamTaskDepartureActor>;
struct SteamTaskRosterPlan {
    std::vector<SteamTaskRosterOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{};
};
struct SteamTaskRosterHuman {
    int identity{},profession{},sex{},weapon{-1},level{},effort{},medals{},fee{};
    std::string name;
    std::array<int,4> combat{}; // 原w[0..3]／HP、攻击、防御、魔法，不能套六基础属性。
    bool affordable{};
};
struct SteamTaskRosterOptions {
    int view_y{},finger_frame{};
    bool scroll_first_touch{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> prediction_width; // 28phase0的原Font实际StringWidth，不按字数估算。
    std::array<std::optional<int>,2> button_widths; // 28的dx/dy重载不夹200，测宽后加16。
};
struct SteamTaskRosterInput {
    int raw{25},selection{},first{},tab{},phase{},counter{},secondary{},prediction{};
    int participant_count{}; // 实时m.size，与25/26冻结X独立。
    std::vector<int> frozen_humans; // 25/26末行-1；27无哨兵；28沿真实m顺序。
    std::vector<SteamTaskRosterHuman> humans;
    std::string task_name;
    bool ongoing{}; // raw26原Quest.IsDungeon()，不是根据页面号补实施中文字。
    SteamTaskRosterOptions options;
};
// 坏名单、游标、依赖和测宽显式拒绝；不补报名、收费、页面或随机。
std::optional<SteamTaskRosterPlan> steam_task_roster_skin(const SteamTaskRosterInput &);
} // namespace dungeon_village_prototype
