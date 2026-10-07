#pragma once

// 41–47仅操作唯一世界Owner；原13槽/user_flags复用，不在窗口持有第二份壶状态。
#include "ark/simulation/startup_world_runtime.hpp"

namespace ark::simulation {
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
} // namespace ark::simulation
