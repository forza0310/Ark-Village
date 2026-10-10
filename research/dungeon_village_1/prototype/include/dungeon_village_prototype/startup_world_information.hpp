#pragma once

#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
// 已解析的平台输入；raw9显式行选择不得与按键混用，原按键优先级由Owner消费。
struct StartupInformationInput {
    bool up{}, down{}, left{}, right{}, confirm{}, cancel{};
    std::optional<int> select_row;
};
struct StartupInformationEntry {
    int tag{};
    std::string_view label;
    int target_raw{};
    bool implemented{};
};
struct StartupInformationPageView {
    std::uint64_t page_id{};
    int raw{};
    int selection_or_period{};
    int frame{};
    std::array<StartupInformationEntry, 5> entries;
    std::optional<StartupIncomeInformation> income;
};
// 稳定scene是维护主菜单适配入口；实际raw3父页也可进入，不伪造主菜单页。
StartupWorldRuntimeError open_startup_world_information_menu(StartupWorldRuntimeState &state);
// 仅栈顶生命周期2接受动作；不支持的子页显式拒绝并保留完整Owner。
StartupWorldRuntimeError input_startup_world_information_page(
    StartupWorldRuntimeState &state, std::uint64_t page, const StartupInformationInput &input);
// 框架初始化：仅生命周期0且两份载荷均不存在时创建；已有页缺字段不能补默认值。
bool initialize_startup_world_information_pages(StartupWorldRuntimeState &candidate);
std::optional<StartupWorldRuntimeState> update_startup_world_information_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
// 检查既有phase/counter与生命周期关系，不初始化、清理或推进状态。
bool valid_startup_world_information_page(const StartupWorldRuntimeState &state,
                                         std::uint64_t page);
// 可读已初始化生命周期1/2/3，含挂起父页；可绘制不意味着可交互。
std::optional<StartupInformationPageView> inspect_startup_world_information_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
} // namespace dungeon_village_prototype
