// 村办51—54的原型管理入口与只读页面投影。
#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
enum class StartupVillageActivityAction { previous, next, select, confirm, cancel };
struct StartupVillageActivityView {
    int raw{};
    int selection{};
    int first_visible{};
    int counter{};
    std::optional<int> activity;
    std::vector<int> entries; // 51是活动定义，54是冻结的开放人物定义。
    std::array<int, 2> display_humans{};
};
StartupWorldRuntimeError open_startup_world_village_activities(StartupWorldRuntimeState &state);
// 原框架入口初始化全部新活动页；53完成时的效果、随机与页插入属于同一候选Owner。
bool initialize_startup_world_village_activity_pages(StartupWorldRuntimeState &candidate);
std::optional<StartupVillageActivityView>
inspect_startup_world_village_activity_page(const StartupWorldRuntimeState &state,
                                            std::uint64_t page);
StartupWorldRuntimeError
act_startup_world_village_activity_page(StartupWorldRuntimeState &state, std::uint64_t page,
                                        StartupVillageActivityAction action, int selection = 0);
std::optional<StartupWorldRuntimeState>
update_startup_world_village_activity_page(const StartupWorldRuntimeState &state,
                                           std::uint64_t page);
} // namespace dungeon_village_prototype
