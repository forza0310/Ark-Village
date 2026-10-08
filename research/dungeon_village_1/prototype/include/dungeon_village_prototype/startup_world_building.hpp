#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
enum class StartupBuildDenial {
    none,
    unavailable,
    insufficient_funds,
    outside_map,
    outside_town,
    occupied
};
struct StartupBuildResult {
    StartupWorldRuntimeError error{StartupWorldRuntimeError::none};
    StartupBuildDenial denial{StartupBuildDenial::none};
    std::optional<std::uint64_t> created{};
};
// 原页21普通可建子集（含募集、排除私人住宅/道路），按原定义顺序与j分组。
std::optional<std::array<std::vector<int>, 3>>
startup_world_build_catalog(const StartupWorldRuntimeState &state);
std::optional<ref::FacilityEconomyValues>
startup_world_build_quote(const StartupWorldRuntimeState &state, int definition);
int startup_world_facility_page_count(const StartupWorldRuntimeState &state,
                                      const ref::WorldScriptPage &page);
StartupWorldRuntimeError open_startup_world_build_menu(StartupWorldRuntimeState &state);
StartupBuildResult select_startup_world_build_menu(StartupWorldRuntimeState &state,
                                                   std::uint64_t page, int definition);
StartupWorldRuntimeError cancel_startup_world_build_menu(StartupWorldRuntimeState &state,
                                                         std::uint64_t page);
StartupBuildResult begin_startup_world_build(StartupWorldRuntimeState &state, int definition);
// 正常建设保持state1以支持连续放置；取消才返回0。拒绝无实例/扣款/随机部分提交。
StartupBuildResult confirm_startup_world_build(StartupWorldRuntimeState &state,
                                               ref::Position anchor,
                                               ref::FacilityOrientation orientation);
StartupWorldRuntimeError cancel_startup_world_build(StartupWorldRuntimeState &state);
// 原m.n连接警告；从原h.f[0]计算当前地图距离，不重建或覆盖人物既有G路线。
bool refresh_startup_world_connections(StartupWorldRuntimeState &state);
// 新局a/o.a(false)已计算的s/w/G投影，不把缺失w误作没有加成，不排变化提示。
bool initialize_startup_world_neighbours(StartupWorldRuntimeState &state);
// 唯一Owner候选上的原创建/刷新原语；移动只复用安装，不支付设施原造价。
StartupBuildResult install_startup_world_facility(StartupWorldRuntimeState &candidate,
                                                  int definition, ref::Position anchor,
                                                  ref::FacilityOrientation orientation);
bool refresh_startup_world_map(StartupWorldRuntimeState &candidate, bool notices);
// 原c→d：只重建显示、道路及围栏，不改变邻接G/s/w/H或通知。
bool refresh_startup_world_surface(StartupWorldRuntimeState &candidate);
// 原地图g内部设施创建：限kind4/5，不走玩家镇界、造价、施工和连接提示。
// refresh=true仅c→d；false保留源稍后统一刷新时点。调用者持有整次事务候选。
StartupBuildResult install_startup_world_map_facility(StartupWorldRuntimeState &candidate,
                                                      int definition, ref::Position anchor,
                                                      bool refresh);
// 只退休实例及其占地/辅助缓存，不重写人物既有目标、路线或合法任务历史。
bool retire_startup_world_facility(StartupWorldRuntimeState &candidate, std::uint64_t facility);
// 原o.f()只重算kind3共享经营缓存；转职最终确认调用，不在预览/中点执行。
bool refresh_startup_world_profession_economy(StartupWorldRuntimeState &state);

enum class StartupFacilityPageAction { previous, next, confirm, cancel };
StartupWorldRuntimeError open_startup_world_facility_page(StartupWorldRuntimeState &state,
                                                          std::uint64_t facility);
// 原85按钮7：raw74只读定义，无实例/邻接；返回保留商会父页选择及资金。
StartupWorldRuntimeError open_startup_world_facility_definition(StartupWorldRuntimeState &state,
                                                                int definition);
StartupWorldRuntimeError act_startup_world_facility_page(StartupWorldRuntimeState &state,
                                                         std::uint64_t page,
                                                         StartupFacilityPageAction action);
bool valid_startup_world_facility_page(const StartupWorldRuntimeState &state,
                                       const ref::WorldScriptPage &page);
// 详情只读当前共享成长/职业及实例邻接；不刷新提示、不扣款、不升级。
std::optional<ref::FacilityEconomyValues>
startup_world_facility_values(const StartupWorldRuntimeState &state, std::uint64_t facility);
// raw74第二页读初始化Y的原序，不重新扫描邻接或从累计s分摊贡献。
// 显示标签按原来源kind固定，value按原y位置取；不按规则attribute_slot重排。
struct StartupFacilityBonusValue {
    int attribute{}; // 显示0价格、1品质、2魅力，独立于原x规则槽。
    std::string label;
    std::int64_t value{};
    std::string text; // 原串字面量“+”再接signed整数，负值保留“+-3”。
};
struct StartupFacilityBonusRow {
    std::uint64_t instance{};
    int definition{}, ordinal{}, icon{};
    std::string name; // 原定义名称 + (同定义最小未用序号+1)，不是共享等级。
    std::vector<StartupFacilityBonusValue> values;
};
// 返回全部有序行供原5行视窗消费；空来源合法，坏引用/字段显式拒绝。查询不改Owner。
std::optional<std::vector<StartupFacilityBonusRow>>
startup_world_facility_bonus_rows(const StartupWorldRuntimeState &state, std::uint64_t page);
struct StartupFacilityBonusWindow {
    std::size_t first{}, total{};
    std::vector<StartupFacilityBonusRow> rows; // 原最多5个可见项，first为原bJ；不存入Owner。
};
std::optional<StartupFacilityBonusWindow>
startup_world_facility_bonus_window(const StartupWorldRuntimeState &state, std::uint64_t page,
                                   std::size_t first);
// raw80实际住户名单和替换事务；入住收费与住宅建设造价不是两笔扣款。
StartupBuildResult act_startup_world_residence_page(StartupWorldRuntimeState &state,
                                                    std::uint64_t page, int human,
                                                    bool cancel = false);
std::optional<StartupWorldRuntimeState>
prepare_startup_world_residence_completion(const StartupWorldRuntimeState &state,
                                           std::uint64_t facility);
std::optional<StartupWorldRuntimeState>
refresh_startup_world_residence_requests(const StartupWorldRuntimeState &state, bool notify);
bool consume_startup_world_facility_upgrade(StartupWorldRuntimeState &state, std::uint64_t page,
                                            bool confirm);
} // namespace dungeon_village_prototype
