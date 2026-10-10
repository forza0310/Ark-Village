#pragma once

// 设施商品79、装备信息72与设施口碑演出82；同一Owner维护目录、页面和延迟人气。
#include "ark/simulation/world/startup_world_runtime.hpp"

namespace ark::simulation {
enum class StartupFacilityCatalogAction {
    confirm, cancel, previous, next, select, previous_tab, next_tab, inspect
};
struct StartupFacilityCatalogView {
    int raw{};
    int mode{}; // 79活动1/4/5，72装备信息0/1/2/3，82原展示模式0/1。
    int phase{}; // 79两页、72当前条目索引、82两段，不能互相替代。
    int counter{};
    int selection{};
    int first_visible{};
    int binding{-1}; // 79无绑定；72当前装备定义；82设施定义。
    std::vector<int> entries; // 79/72装备原ID，82共享人物定义ID。
};
bool initialize_startup_world_facility_catalog_pages(StartupWorldRuntimeState &state);
// 恢复与操作共用只读校验；合法待初始化页不被偷偷重建载荷。
bool valid_startup_world_facility_catalog_page(const StartupWorldRuntimeState &state,
                                             const ref::WorldScriptPage &page);
std::optional<StartupFacilityCatalogView>
inspect_startup_world_facility_catalog_page(const StartupWorldRuntimeState &state, std::uint64_t page);
StartupWorldRuntimeError act_startup_world_facility_catalog_page(
    StartupWorldRuntimeState &state, std::uint64_t page, StartupFacilityCatalogAction action,
    int selection = 0);
std::optional<StartupWorldRuntimeState>
update_startup_world_facility_catalog_page(const StartupWorldRuntimeState &state, std::uint64_t page);
} // namespace ark::simulation
