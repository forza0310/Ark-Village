#pragma once

// 41–47仅操作唯一世界Owner；原13槽/user_flags复用，不在窗口持有第二份壶状态。
#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
enum class StartupMagicPotEntry { main_menu, development_menu };
enum class StartupMagicPotAction {
    confirm, cancel, previous, next, select, previous_tab, next_tab
};
struct StartupMagicPotView {
    int raw{};
    int phase{}; // 43两页，其他页0。
    int counter{};
    int selection{};
    int first_visible{};
    int binding{-1}; // 44道具定义；46/47配方定义，其他页无绑定。
    std::vector<int> entries;
};
struct StartupMagicPotMenuInformation {
    int processed{}; // 原IsOpenMPotDevelWindow返回值；0隐藏，不是已结算件数。
    int pending_materials{}; // 原n[1]，期间块分母；不是壶容量。
};
// 只读主菜单期间显示。无元素输出即隐藏，不使用真正处理的满容量补1分支。
// 坏13槽、日期和32位乘积溢出返回空；这些拒绝属于维护约束。
std::optional<StartupMagicPotMenuInformation>
startup_magic_pot_menu_information(const StartupWorldRuntimeState &state);
// 两个已证入口处理M后插41；main清u bit2，development保留bit2。
StartupWorldRuntimeError open_startup_world_magic_pot(StartupWorldRuntimeState &state,
                                                      StartupMagicPotEntry entry);
bool initialize_startup_world_magic_pot_pages(StartupWorldRuntimeState &state);
bool valid_startup_world_magic_pot_page(const StartupWorldRuntimeState &state,
                                        const ref::WorldScriptPage &page);
std::optional<StartupMagicPotView>
inspect_startup_world_magic_pot_page(const StartupWorldRuntimeState &state, std::uint64_t page);
StartupWorldRuntimeError act_startup_world_magic_pot_page(
    StartupWorldRuntimeState &state, std::uint64_t page, StartupMagicPotAction action,
    int selection = 0);
std::optional<StartupWorldRuntimeState>
update_startup_world_magic_pot_page(const StartupWorldRuntimeState &state, std::uint64_t page);
// 只供村办53的私有候选使用：先扣q的共同顺序由村办Owner维护。
bool apply_startup_world_magic_pot_activity(StartupWorldRuntimeState &state, int kind);
} // namespace dungeon_village_prototype
