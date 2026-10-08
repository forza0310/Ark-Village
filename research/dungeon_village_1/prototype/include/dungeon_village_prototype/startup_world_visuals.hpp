#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

#include <functional>

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
    std::optional<std::size_t> record_index{}; // 人物cd绘制保留原索引；设施/静态图标无此索引。
};
// 只消费既有cd15/21/22举物事实及设施队首kind1..6；不推进计数、装配、邻接、随机或声音。
std::optional<std::vector<StartupVisualDraw>>
startup_world_equipment_lift_draws(const StartupWorldRuntimeState &state, ref::CharacterId actor);
std::optional<std::vector<StartupVisualDraw>>
startup_world_facility_growth_draws(const StartupWorldRuntimeState &state, std::uint64_t facility);

// cd13气泡：背景/图标→蓝色属性名→数字/加号；各命令offset相对身体绘制锚点H+bl。
// 与举物按record_index归并，不能把所有13集中放到所有15/21/22之后。
struct StartupAttributeGainDraw {
    std::size_t record_index{};
    int age{}, attribute{}, delta{}, width{};
    std::vector<StartupVisualDraw> before_text;
    std::string text;
    std::array<int,2> text_offset{};
    std::array<int,3> text_rgb{0,100,255};
    std::vector<StartupVisualDraw> after_text;
};
// measure只读当前表现字体，分别测原属性名及原始有符号十进制串；不存入Owner/schema。
// 查询不推进cd、重复累计属性、重新派生HP、消费随机或发声音；退休由逻辑更新完成。
std::optional<std::vector<StartupAttributeGainDraw>>
startup_world_attribute_gain_draws(const StartupWorldRuntimeState &state, ref::CharacterId actor,
                                  const std::function<int(const std::string &)> &measure);

// 原b/g.c type1：先分类底框(-1,-1)，再普通道具g.g的16×16前景(0,0)。
// 返回相对调用者前景左上角的计划；不读/扣库存，不把定义ID或效果类别当图标。
std::optional<std::vector<StartupVisualDraw>>
startup_world_item_icon_draws(const StartupWorldRuntimeState &state, int item_definition);

// a/j五参数绘制的完整分片，offset相对候选光标／64×32缩略图的(2,10)锚点。
// 与建设准入分开：只核原定义/形状/朝向，地图边缘候选仍可绘制越界分片。
struct StartupBuildingDraw {
    std::string sprite;
    int frame{};
    std::array<int, 2> offset{};
};
std::optional<std::vector<StartupBuildingDraw>>
startup_world_building_draws(const StartupWorldRuntimeState &state, int definition,
                             ref::FacilityOrientation orientation);
// 原state1、mode0/7、全地图光标资格及scene_counter%20<10；不检查占用/金币或推进计数。
std::optional<std::vector<StartupBuildingDraw>>
startup_world_building_preview_draws(const StartupWorldRuntimeState &state, ref::Position cursor,
                                     ref::FacilityOrientation orientation);
} // namespace dungeon_village_prototype
