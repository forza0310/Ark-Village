#pragma once

// 商会83—86与购买设施领取93：使用唯一世界Owner的金币、点数、库存和页面。
#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
enum class StartupCommerceAction {
    confirm,
    cancel,
    previous,
    next,
    select,
    previous_tab,
    next_tab,
    inspect
};
struct StartupCommerceView {
    int raw{};
    int mode{}; // 84/86：0买入，1出售。
    int tab{};  // 买入目录的价格/持有数量展示，不改变交易方向。
    int selection{};
    int first_visible{};
    int binding{-1};
    int feedback_counter{};
    int counter{};
    std::vector<int> entries;
};
StartupWorldRuntimeError open_startup_world_commerce(StartupWorldRuntimeState &state);
bool initialize_startup_world_commerce_pages(StartupWorldRuntimeState &state);
std::optional<StartupCommerceView>
inspect_startup_world_commerce_page(const StartupWorldRuntimeState &state, std::uint64_t page);
StartupWorldRuntimeError act_startup_world_commerce_page(StartupWorldRuntimeState &state,
                                                         std::uint64_t page,
                                                         StartupCommerceAction action,
                                                         int selection = 0);
std::optional<StartupWorldRuntimeState>
update_startup_world_commerce_page(const StartupWorldRuntimeState &state, std::uint64_t page);
} // namespace dungeon_village_prototype
