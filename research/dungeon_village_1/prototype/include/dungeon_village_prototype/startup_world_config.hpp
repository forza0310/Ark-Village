#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/steam_config_skin.hpp"

namespace dungeon_village_prototype {
// 跨局J3/J4/J9由应用Owner提供；世界不另持系统设置副本。
struct StartupConfigSystemValues {
    int bgm{3}, se{3};
    bool speed_unlocked{};
};
struct StartupConfigInput {
    bool up_repeat{}, down_repeat{}, left_repeat{}, right_repeat{}, back_soft_label{};
};
struct StartupWorldConfigView {
    std::uint64_t page{};
    int selection{}, frame{}, entry_quality{};
};
struct StartupWorldConfigResult {
    StartupWorldRuntimeError error{StartupWorldRuntimeError::none};
    std::optional<StartupWorldRuntimeState> candidate;
    std::array<int, 2> system_volumes{{3, 3}};
    std::vector<SteamConfigSetterOperation> effects;
    std::array<bool, 5> checked_keys{};
    bool save_system{}, reset_offscreen{}, popped{};
};
StartupWorldRuntimeError open_startup_world_config_page(StartupWorldRuntimeState &state);
bool valid_startup_world_config_page(const StartupWorldRuntimeState &state, std::uint64_t page);
// 同时核孤立选择及退休页；Finish前不能用擦键修复坏载荷。
bool valid_startup_world_config_pages(const StartupWorldRuntimeState &state);
bool initialize_startup_world_config_pages(StartupWorldRuntimeState &candidate);
std::optional<StartupWorldConfigView> inspect_startup_world_config_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
std::optional<StartupWorldRuntimeState> update_startup_world_config_page(
    const StartupWorldRuntimeState &state, std::uint64_t page);
// 已准入非日文四行的一整轮：公共主钟→raw12模20→方向→setter→返回。
// 候选仅在应用联合系统提交及表现输出成功准备后安装；本层不写文件或播放。
StartupWorldConfigResult prepare_startup_world_config_input(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const StartupConfigSystemValues &system, const StartupConfigInput &input);
// Touch即时setter不推进页面钟、不借键盘的J9门控；before按当前Owner读取。
StartupWorldConfigResult prepare_startup_world_config_setting(
    const StartupWorldRuntimeState &state, std::uint64_t page,
    const StartupConfigSystemValues &system, int row, int value);
StartupWorldRuntimeError set_startup_world_config_selection(
    StartupWorldRuntimeState &state, std::uint64_t page, int selection);
} // namespace dungeon_village_prototype
