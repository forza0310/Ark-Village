#pragma once

#include "dungeon_village_prototype/steam_human_skin.hpp"

namespace dungeon_village_prototype {
// Steam任务选择22、费用提示23、募集演出24；业务与随机仅由原世界Owner推进。
enum class SteamTaskSelectionAsset { recruitment_background, treasure };
std::optional<SteamFacilityResource> steam_task_selection_resource(SteamTaskSelectionAsset);
struct SteamTaskSelectionImage {
    SteamTaskSelectionAsset asset;
    std::array<int,2> position{};
    int frame{};
    std::optional<std::array<int,4>> crop;
};
struct SteamTaskSelectionText {
    std::string value;
    std::array<int,4> rectangle{};
    int anchor{},font_size{};
    bool layout{};
    std::array<int,3> rgb{92,51,31};
};
struct SteamTaskSelectionClip { bool push{}; std::array<int,4> rectangle{}; };
using SteamTaskSelectionOperation=std::variant<SteamStartupFrameDraw,SteamTaskSelectionImage,
    SteamTaskSelectionText,SteamTaskSelectionClip,SteamHumanImage,SteamHumanPortrait,
    SteamFacilityImage,SteamFacilityNumber,SteamHumanCatalogueScroll,SteamStartupTouch>;
struct SteamTaskSelectionPlan {
    std::vector<SteamTaskSelectionOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{};
};
struct SteamTaskSelectionOptions {
    int view_y{},finger_frame{};
    bool japanese{},scroll_first_touch{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> bottom_width; // 22说明／23单按钮StringWidth，原helper夹200。
    std::array<std::optional<int>,6> phrase_widths; // 24只消费实际可见的原发言。
};
struct SteamTaskSelectionRow {
    std::uint64_t identity{};
    int kind{},difficulty{},fee{}; // difficulty沿现Owner字段；原UI读Quest.treasureLv_，不是怪物数。
    bool newly_available{},affordable{};
    std::string name;
};
struct SteamTaskRecruitmentHuman {
    int identity{},profession{},sex{},weapon{-1},level{},medals{};
    std::string name;
};
struct SteamTaskSelectionInput {
    int raw{22},selection{},first{},counter{},fee{};
    bool affordable{};
    std::vector<SteamTaskSelectionRow> rows;
    std::optional<ref::TaskRecruitmentAnimation> recruitment;
    std::vector<SteamTaskRecruitmentHuman> humans;
    SteamTaskSelectionOptions options;
};
// 有序只读图元；不补页面载荷、不重抽报名、不以展示人数替代真实X／Y。
std::optional<SteamTaskSelectionPlan> steam_task_selection_skin(const SteamTaskSelectionInput &);
std::optional<SteamTaskSelectionPlan> steam_task_selection_skin(
    const StartupWorldRuntimeState &,std::uint64_t page,const SteamTaskSelectionOptions &);
} // namespace dungeon_village_prototype
