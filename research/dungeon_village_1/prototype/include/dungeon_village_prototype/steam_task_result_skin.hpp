#pragma once

#include "dungeon_village_prototype/steam_task_selection_skin.hpp"
#include "dungeon_village_prototype/steam_task_roster_skin.hpp"
#include "dungeon_village_prototype/startup_world_visuals.hpp"

namespace dungeon_village_prototype {
// Steam任务31战绩／32宝物：只消费初始化后的原序载荷，不重结算、不抽人物。
struct SteamTaskResultHuman {
    int identity{},profession{},sex{};
    std::string name;
};
struct SteamTaskResultCrew {
    SteamTaskResultHuman human;
    int kills{},downs{}; // 原H/I；特殊任务H归一已由Owner初始化完成。
};
struct SteamTaskResultReward {
    int kind{},definition{},value{}; // 原X的kind0道具、1武器、2防具、3饰品；value为展示价格。
    std::string name;
    std::vector<StartupVisualDraw> icon; // 沿现item/equipment_icon_draws，无库存／领取操作。
};
struct SteamTaskResultOptions {
    int view_y{},finger_frame{};
    bool japanese{},scroll_first_touch{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> bottom_width; // 32及日文31真实StringWidth；非日文31字号10直接居中。
};
struct SteamTaskResultInput {
    int raw{31},selection{},first{},difficulty{};
    std::string task_name; // 原QuestData.GetName(true)，不是无条件原表基本名称。
    std::vector<SteamTaskResultCrew> crew; // X可重复引用同一人物，必须保留原序。
    std::vector<SteamTaskResultReward> rewards;
    std::array<SteamTaskResultHuman,2> footer; // 页面冻结f/g，不能取当前列表首尾替代。
    SteamTaskResultOptions options;
};
using SteamTaskResultOperation=std::variant<SteamStartupFrameDraw,SteamTaskSelectionText,
    SteamTaskSelectionClip,SteamTaskSelectionImage,SteamHumanImage,SteamHumanPortrait,
    SteamFacilityImage,SteamFacilityNumber,SteamHumanCatalogueScroll,SteamStartupTouch>;
struct SteamTaskResultPlan {
    std::vector<SteamTaskResultOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{0,2};
};
// 条件纯计划；正式Owner桥只消费真实冻结名单和游标，不在绘制时补页面字段。
std::optional<SteamTaskResultPlan> steam_task_result_skin(const SteamTaskResultInput &);
std::optional<std::string> steam_task_display_name(const StartupWorldRuntimeState &,
    const StartupWorldTask &,bool after_completion);
// 供后端按真实文案测宽；返回值未安装字体options，不推进或初始化页面。
std::optional<SteamTaskResultInput> steam_task_result_input(const StartupWorldRuntimeState &,
    std::uint64_t page);
std::optional<SteamTaskResultPlan> steam_task_result_skin(const StartupWorldRuntimeState &,
    std::uint64_t page,const SteamTaskResultOptions &);

// 30沿实时m、当前职业／性别／主武器绘演员，仅经验share与q绑定冻结于页面。
struct SteamTaskVictoryHuman { int identity{},profession{},sex{},weapon{-1}; };
struct SteamTaskVictoryActor {
    int profession{},sex{},weapon{-1},action{};
    std::array<int,2> position{};
};
// 30仅action0／6，step0、facing2；不创建W或修改人物。
std::optional<StartupTitleActorSkin> steam_task_victory_actor_skin(const SteamTaskVictoryActor &);
struct SteamTaskVictoryOptions {
    int view_y{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> reputation_suffix_width; // 原Font.StringWidth("上升了!")，不是数字测宽。
};
struct SteamTaskVictoryInput {
    int phase{},counter{},secondary{},experience{},reputation{};
    std::string task_name; // QuestData.GetName(true)，与列表GetName(false)后缀时点不同。
    std::vector<SteamTaskVictoryHuman> humans; // 实时m原序，重复身份合法。
    SteamTaskVictoryOptions options;
};
using SteamTaskVictoryOperation=std::variant<SteamStartupFrameDraw,SteamTaskSelectionText,
    SteamTaskSelectionClip,SteamHumanImage,SteamFacilityNumber,SteamTaskRosterImage,
    SteamTaskVictoryActor,SteamStartupTouch>;
struct SteamTaskVictoryPlan {
    std::vector<SteamTaskVictoryOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{0,2}; // 原另注册无显式矩形的组件2／option2。
};
std::optional<SteamTaskVictoryPlan> steam_task_victory_skin(const SteamTaskVictoryInput &);
std::optional<SteamTaskVictoryPlan> steam_task_victory_skin(
    const StartupWorldRuntimeState &,std::uint64_t page,const SteamTaskVictoryOptions &);
} // namespace dungeon_village_prototype
