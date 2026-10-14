#pragma once

#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "ark/simulation/presentation/startup_title_presentation.hpp"

namespace ark::simulation {
// 身体沿既有human SEB适配器绘制，不能误送common/weapon二分适配器。
struct StartupTitleBodyDraw {
    int image{}, sprite{}, frame{};
};
struct StartupTitleActorSkin {
    std::optional<StartupVisualDraw> shadow;
    std::optional<StartupVisualDraw> weapon;
    StartupTitleBodyDraw body;
};
// 固定APK、正常临时W、action0：按shadow(若请求)→weapon(若非-1)→body消费。
// 参数来自当前定义/草稿的职业、性别、主武器；不创建或修改W/世界人物。
// step为调用者已决定的0..3，facing为原0..3；不擅自从age重算标题/纪录不同周期。
// offset相对调用者人物锚点；SEB内部偏移由既有适配器另加一次。
// 不覆盖原W的调试、选中、携物、伤害或cd叠加；正常action0数组/裁片已独立交叉Steam，
// 此入口仍读取固定APK维护定义，不把它升级为Steam全定义配对或完整scratch恢复。
std::optional<StartupTitleActorSkin> startup_title_actor_skin(
    int profession, int sex, int weapon_definition, int step, int facing, bool include_shadow);

struct SteamTitleActorPlacement {
    StartupTitlePersonDraw person;
    // 边缘才设(op1,src,dst)且绘完清默认(op0,255,0)；中央继承环境，不额外reset。
    std::optional<std::array<int,3>> edge_mode;
};
struct SteamTitleActorProjection {
    std::vector<SteamTitleActorPlacement> people;
};
// 固定Steam只读Draw_titleAnime：全部20槽先按原交换序排序，再过滤inactive。
// 借用六整数DTO，不调用APK校验器/更新器；活跃x为-18..254，退休保留-19/255。
// 请求阴影及人物助手的实参，不声称普通scratch历史资格或深层附效已闭合。
// view_height来自原标题view_，与外层GameView.H分开；维护预算1..65536。
std::optional<SteamTitleActorProjection> steam_title_actor_projection(
    const std::array<StartupTitleSlot,20> &slots,int view_height);
} // namespace ark::simulation
