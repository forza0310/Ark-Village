#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
// 设施普通道具75目录、76投放演出及77结果；74入口仍由设施模块负责。
enum class StartupFacilityItemAction { confirm, cancel, previous, next, select };
StartupWorldRuntimeError open_startup_world_facility_items(StartupWorldRuntimeState &state,
                                                           std::uint64_t facility_page);
StartupWorldRuntimeError act_startup_world_facility_item_page(StartupWorldRuntimeState &state,
                                                              std::uint64_t page,
                                                              StartupFacilityItemAction action,
                                                              int selection = -1);
bool valid_startup_world_facility_item_page(const StartupWorldRuntimeState &state,
                                            const ref::WorldScriptPage &page);
// 框架按栈内原序初始化，包括被同步实例脚本盖住的76；只传入候选Owner。
bool initialize_startup_world_facility_item_pages(StartupWorldRuntimeState &candidate);
// 每次获准页面更新自行增加原counter；仅候选成功才提交首次改良、脚本及换页。
std::optional<StartupWorldRuntimeState>
prepare_startup_world_facility_item_page(const StartupWorldRuntimeState &state);
} // namespace dungeon_village_prototype
