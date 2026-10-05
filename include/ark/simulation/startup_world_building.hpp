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

enum class StartupFacilityPageAction { previous, next, confirm, cancel };
StartupWorldRuntimeError open_startup_world_facility_page(StartupWorldRuntimeState &state,
                                                          std::uint64_t facility);
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
