#pragma once

#include "ark/simulation/startup_world_runtime.hpp"

namespace ark::simulation {
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
} // namespace ark::simulation
