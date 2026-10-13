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
} // namespace dungeon_village_prototype
