#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
// 平台已解析repeat/edge；显式行选择不能与按键混用。语言仅决定确认时子页存储位置。
struct StartupWorldMenuInput {
    bool up{}, down{}, left{}, right{}, confirm{}, cancel{};
    std::optional<int> select_row;
    bool english{};
    // 原raw3后继软键/浏览器须经应用消费者；尚未接入时显式拒绝，不自动打开外链。
    bool save_shortcut{}, browser_shortcut{};
};
struct StartupWorldMenuView {
    std::uint64_t page{};
    int raw{}, frame{}, selection{};
    std::array<int, 2> stored_position{};
    std::vector<int> tags;
};
StartupWorldRuntimeError open_startup_world_main_menu(StartupWorldRuntimeState &state);
// 原raw4的维护直达入口沿用；真实raw3确认根据父存储位置创建子菜单。
StartupWorldRuntimeError open_startup_world_navigation_submenu(
    StartupWorldRuntimeState &state, int raw, bool english = false);
bool initialize_startup_world_menu_pages(StartupWorldRuntimeState &candidate);
bool valid_startup_world_menu_page(const StartupWorldRuntimeState &state, std::uint64_t page);
std::optional<StartupWorldMenuView> inspect_startup_world_menu_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
StartupWorldRuntimeError input_startup_world_menu_page(
    StartupWorldRuntimeState &state, std::uint64_t page, const StartupWorldMenuInput &input);
std::optional<StartupWorldRuntimeState> update_startup_world_menu_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
// 仅借用当前回调执行锚；用于事件先插页后仍由原菜单创建目标的私有事务。
bool startup_world_menu_callback(const StartupWorldRuntimeState &state, int raw);
// 原RemoveAllMenuForms只退休菜单集合，实际Finish统一释放页载荷。
bool retire_startup_world_menu_pages(StartupWorldRuntimeState &candidate);
} // namespace dungeon_village_prototype
