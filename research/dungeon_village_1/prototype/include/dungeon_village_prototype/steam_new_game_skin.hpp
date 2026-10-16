#pragma once

#include "dungeon_village_prototype/steam_human_skin.hpp"
#include "dungeon_village_prototype/steam_manual_skin.hpp"

namespace dungeon_village_prototype {
// Steam raw91只读配置绘制，显式选择/主钟不从标题计数或draft缺字段补造。
struct SteamNewGameInput {
    std::string village, name;
    int sex{}, selection{}, main{};
    int adventure_profession{-1}; // 绑定原JobData.adventureJobData，不是定义0当前职业。
};
struct SteamNewGameOptions {
    int view_y{}, finger_frame{-1}; // 当前SEB21帧由表现消费者提供，不取main模数。
    bool english{}, japanese{};
    std::optional<std::array<int,2>> title_widths;
    std::optional<int> start_width; // 当前Font真实StringWidth；原clampW=-1，不夹200。
    std::array<bool,2> arrow_selected{}; // 原CheckTouch(18,value)，不能按悬停猜选中。
};
// 与raw13相同的AddTouch箭头绘制helper，不同组件/value/margin保留在本页请求。
struct SteamNewGameArrow {
    SteamManualArrowRequest source;
    int component{18}, value{};
    std::array<int,4> margin{}; // left/right/top/bottom；Steam本页底边4，不借APK20。
};
using SteamNewGameOperation = std::variant<SteamStartupFrameDraw,StartupSkinDraw,
    SteamHumanOverviewText,SteamHumanPortrait,SteamFacilityImage,SteamStartupTouch,SteamNewGameArrow>;
struct SteamNewGamePlan {
    std::vector<SteamNewGameOperation> operations;
    std::string title;
    std::array<int,2> soft_labels{0,2};
};
// 标题/四项中文沿已见Steam窗口；布局/资源/字号资格独立核验，字体与度量仍为维护适配。
// 维护来源预算4096字节/字宽；缺真实冒险者职业、测宽、手形帧或非法选择显式拒绝。
// 草稿空村名/姓名原样留空，不添加原平台尚未闭合的非空门槛。
// 不创建世界、修改草稿、推进随机/时钟或执行开始/取消。
std::optional<SteamNewGamePlan> steam_new_game_skin(
    const SteamNewGameInput &input,const SteamNewGameOptions &options);

// 已进入raw91 Update2分支后的逻辑查询结果；通用Update主钟已经由外层推进。
// Pulse/Repeat/soft2分别供给，不把物理按键或Touch值直接当作原查询结果。
struct SteamNewGameUpdateInput {
    int selection{}, sex{};
    bool change_name{};
    std::array<bool,4> early_pulses{}; // 原键值2、8、128、512，依序短路。
    bool up_pulse{}, down_pulse{}, left_repeat{}, right_repeat{}, confirm_pulse{}, back_soft_label{};
};
enum class SteamNewGameAction { none, edit_village, edit_name, start_new_game, pop };
struct SteamNewGameUpdatePlan {
    int selection{}, sex{};
    bool reset_default_name{}; // 先按最终sex取原默认名，再执行action；不在计划中猜译名。
    // 依序为四个early、上、下、左、右、确认、soft2；未查询项保持false。
    std::array<bool,10> checked_keys{};
    SteamNewGameAction action{SteamNewGameAction::none};
    std::optional<int> editor_max_length; // MainPanel原参数14/12，不认证字符/字节算法。
    std::optional<int> page_result; // 仅返回动作请求先置-1，再调用真实Pop。
    std::optional<bool> immediate_return{true}; // 有外部动作时为空，不把请求当作执行成功。
};
// 纯候选：不改草稿/Owner、不调用编辑器/NewGame/Pop、不抽随机、不推进主钟。
// 编辑及开始还须执行原标签/面板/切主Form副作用；其异常与结果由应用层负责。
std::optional<SteamNewGameUpdatePlan> steam_new_game_update(const SteamNewGameUpdateInput &input);
} // namespace dungeon_village_prototype
