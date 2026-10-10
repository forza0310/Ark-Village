#pragma once

#include "ark/simulation/presentation/startup_world_visuals.hpp"

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
// 不覆盖原W的调试、选中、携物、伤害或cd叠加，不声称Steam资源调用已交叉。
std::optional<StartupTitleActorSkin> startup_title_actor_skin(
    int profession, int sex, int weapon_definition, int step, int facing, bool include_shadow);
} // namespace ark::simulation
