#pragma once

#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"

namespace dungeon_village_prototype {
// 已解析的平台输入；显式行选择不得与按键混用，原按键优先级由Owner消费。
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
struct StartupHumanInformation {
    StartupHumanDetails details;
    int presence{}, contribution{}, yearly_town_points{}, yearly_spending{};
    bool newly_unlocked{};
};
struct StartupInformationPageView {
    std::uint64_t page_id{};
    int raw{};
    int selection_or_period{};
    int frame{};
    std::array<StartupInformationEntry, 5> entries;
    std::optional<StartupIncomeInformation> income;
    int selection{}, first_visible{}; // raw35/37/38/40当前列表，selection_or_period仍为页签。
    std::optional<std::vector<StartupItemInformation>> items;
    std::optional<StartupEquipmentInformation> equipment;
    std::optional<std::vector<StartupHumanInformation>> humans;
    std::optional<StartupTownInformation> town;
    std::string village_name; // 34读取实际应用安装到Owner的村名，不借最高资金纪录名称。
    std::optional<std::vector<StartupFacilityInformation>> facilities;
};
// 稳定scene是维护适配入口；raw3须由已验证执行锚进入，保存实际父位置与行号。
StartupWorldRuntimeError open_startup_world_information_menu(StartupWorldRuntimeState &state,
                                                             bool english = false);
// 原raw4赠礼入口40；仅接受已验证的真实raw4执行锚，不借装备目录38。
StartupWorldRuntimeError open_startup_world_present_directory(StartupWorldRuntimeState &state);
// 仅栈顶生命周期2接受动作；不支持的子页显式拒绝并保留完整Owner。
StartupWorldRuntimeError input_startup_world_information_page(
    StartupWorldRuntimeState &state, std::uint64_t page, const StartupInformationInput &input);
// 框架初始化：仅生命周期0且全部载荷均不存在时创建；空37经真实事件15后退休。
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
