#pragma once

#include "dungeon_village_prototype/steam_task_selection_skin.hpp"
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
// 无Owner桥；调用方须提供真实游标、完整冻结名单和已解析图标，不在绘制时补页面字段。
std::optional<SteamTaskResultPlan> steam_task_result_skin(const SteamTaskResultInput &);
} // namespace dungeon_village_prototype
