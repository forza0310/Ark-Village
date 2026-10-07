#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
// b/g.a(o,x,y,image)：15×14裁剪，walk01帧0，脚底相对裁剪原点(8,21)。
// 坐标仅属于原版表现计划，不进入地图、人物AI或服务规则。
struct StartupPortrait {
    int image{};
    int sprite{1};
    int frame{};
    int clip_width{15};
    int clip_height{14};
    int anchor_x{8};
    int anchor_y{21};
};
std::optional<StartupPortrait> startup_world_portrait(const StartupWorldRuntimeState &state,
                                                      int human_definition);
struct StartupInnRow {
    ref::CharacterId actor;
    StartupPortrait portrait;
    int row{}; // 前四占用引用中有flag32者紧凑排列；不是扫描前四个可见人物。
    bool healing{};
    int bar_width{};
    int capacity{};
};
// 只读Tenant.o、B及am1；绘制查询不推进休息/回血、通知、资金或随机。
std::optional<std::vector<StartupInnRow>>
startup_world_inn_rows(const StartupWorldRuntimeState &state, std::uint64_t facility);

enum class StartupVisualResource { common, weapon };
// 源资源索引的只读绘制命令；sprite>=0使用SEB指定层，否则裁image的矩形。
// offset相对人物脚底/设施f()原投影，SEB内部offset另由画图适配器应用一次。
struct StartupVisualDraw {
    StartupVisualResource resource{StartupVisualResource::common};
    int sprite{-1}, image{-1}, frame{}, layer{};
    std::array<int, 4> crop{};
    std::array<int, 2> offset{};
};
// 只消费既有cd15/21/22举物事实及设施队首kind1..6；不推进计数、装配、邻接、随机或声音。
std::optional<std::vector<StartupVisualDraw>>
startup_world_equipment_lift_draws(const StartupWorldRuntimeState &state, ref::CharacterId actor);
std::optional<std::vector<StartupVisualDraw>>
startup_world_facility_growth_draws(const StartupWorldRuntimeState &state, std::uint64_t facility);
} // namespace dungeon_village_prototype
