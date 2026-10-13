#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"

namespace ark::simulation {
// 设施普通道具75目录、76投放演出及77结果；74入口仍由设施模块负责。
enum class StartupFacilityItemAction { confirm, cancel, previous, next, select };
struct StartupFacilityItemInput {
    bool up{},down{},confirm{},cancel{};
    std::optional<int> selection;
    struct Scroll { bool absolute{}; int value{}; };
    std::optional<Scroll> scroll;
};
// 同轮方向／选择合成后提交一次；确认不携带行号时消费Owner当前选择。
// 首行由Owner持有；DTO仅传本轮滚动请求，不保存设备手势。
StartupWorldRuntimeError input_startup_world_facility_item_page(StartupWorldRuntimeState &state,
    std::uint64_t page,const StartupFacilityItemInput &input);
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
} // namespace ark::simulation
