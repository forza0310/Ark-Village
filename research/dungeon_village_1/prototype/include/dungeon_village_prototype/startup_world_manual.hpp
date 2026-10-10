#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"

namespace dungeon_village_prototype {
struct StartupManualInput { bool confirm{}, right{}, left{}, cancel{}; };
// 正文只读固定规则，关于页单独标识并保留旧正文；不生成未经认证的关于文案。
struct StartupManualPageView {
    std::uint64_t page{};
    int index{}, counter{};
    bool about{};
    std::string text;
    std::vector<int> decorations;
    int text_page{};
    bool localize_text{true};
};
StartupWorldRuntimeError open_startup_world_manual_page(StartupWorldRuntimeState &state);
bool valid_startup_world_manual_page(const StartupWorldRuntimeState &state, std::uint64_t page);
bool initialize_startup_world_manual_pages(StartupWorldRuntimeState &candidate);
std::optional<StartupManualPageView> inspect_startup_world_manual_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
StartupWorldRuntimeError input_startup_world_manual_page(StartupWorldRuntimeState &state,
    std::uint64_t page, const StartupManualInput &input);
std::optional<StartupWorldRuntimeState> update_startup_world_manual_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
} // namespace dungeon_village_prototype
