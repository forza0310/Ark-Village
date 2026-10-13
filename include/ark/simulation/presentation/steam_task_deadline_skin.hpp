#pragma once

#include "ark/simulation/presentation/steam_task_roster_skin.hpp"

namespace ark::simulation {
// Steam raw33期限继续确认。只投影页面及当前定义／实例；续费、中止由父Owner消费答案。
enum class SteamTaskDeadlineAsset { progress_label };
std::optional<SteamFacilityResource> steam_task_deadline_resource(SteamTaskDeadlineAsset);
struct SteamTaskDeadlineImage {
    SteamTaskDeadlineAsset asset;
    std::array<int,2> position{};
    int frame{};
};
struct SteamTaskDeadlineMonster {
    int image{},sprite{},frame{};
    std::array<int,2> position{};
    int requested_step{}; // 页面传入末人物步长；实际walk帧另由main与怪物速度产生。
};
using SteamTaskDeadlineOperation=std::variant<SteamStartupFrameDraw,SteamTaskSelectionText,
    SteamTaskSelectionClip,SteamHumanImage,SteamHumanPortrait,SteamFacilityImage,
    SteamFacilityNumber,SteamStartupTouch,SteamTaskRosterImage,SteamFacilityMapchipDraw,
    SteamTaskDeadlineImage,SteamTaskDeadlineMonster>;
struct SteamTaskDeadlinePlan {
    std::vector<SteamTaskDeadlineOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{0,0};
};
struct SteamTaskDeadlineHuman {
    int identity{},profession{},weapon{-1},downs{};
};
struct SteamTaskDeadlineLiveHuman {
    int definition{},action{},hp_after{},hp_maximum{}; // 原seb_+24，不能填state_+64。
};
struct SteamTaskDeadlineMonsterDefinition {
    int identity{},body{},variant{},shadow{}; // 已解析的原调用输入，不按body猜shadow。
};
struct SteamTaskDeadlineOptions {
    int view_y{},finger_frame{};
    bool japanese{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> down_number_width; // 原Font.StringWidth(全m倒下次数十进制串)，不是SEB宽。
    std::optional<int> prediction_width;
    std::optional<int> bubble_width; // phase1仅测本次答案的原句子。
    std::array<std::optional<int>,2> button_widths; // 实际字宽+16，不夹200。
};
struct SteamTaskDeadlineInput {
    int phase{},selection{},counter{},fee{},grade{},kind{};
    std::optional<int> answer; // phase1必有0／1；phase0不消费。
    bool affordable{};
    std::string task_name;
    std::vector<int> participants; // 原m保序；只画前3，合计全部，合法重复不去重。
    std::vector<SteamTaskDeadlineHuman> humans;
    std::vector<SteamTaskDeadlineLiveHuman> active_humans; // 原bl顺序，可只筛参与定义，按定义找首次匹配。
    std::optional<int> mapchip; // kind0原h为空时允许缺省；不是坏绑定的兜底。
    std::optional<int> progress; // kind0已有Dungeon消费者派生百分比，0..100。
    std::optional<int> remaining; // kind1既有quota／spawned／linked计算后的剩余数。
    std::optional<int> main_monster;
    std::vector<int> encounter_monsters;
    std::vector<SteamTaskDeadlineMonsterDefinition> monsters;
    SteamTaskDeadlineOptions options;
};
// 不修改Owner／scratch／页面／随机。坏定义、实例、测宽及依赖显式拒绝。
std::optional<SteamTaskDeadlinePlan> steam_task_deadline_skin(const SteamTaskDeadlineInput &);
} // namespace ark::simulation
