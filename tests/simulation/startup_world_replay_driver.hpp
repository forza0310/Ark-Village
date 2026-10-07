#pragma once

#include "ark/simulation/startup_world_runtime.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace ark::simulation::test {

// 自然玩家是世界之外的维护测试控制器；完整轮边界持久化所有跨轮策略和统计。
// 不把这些字段当作原 APK 存档，也不保存可从当前世界重新取得的引用或迭代器。
struct StartupWorldReplayDriver {
    std::uint64_t seed{1};
    int speed{};
    bool expansion{};
    int next_frame{}, observed_frame{}, checks{};
    bool terminal{};
    std::optional<std::uint64_t> bakery;
    std::set<int> upgraded;
    std::set<std::uint64_t> seen_upgrades;
    int last_month{-1}, promoted_month{-1}, unlocked_month{-1};
    std::int64_t unlocked_income{};
    int edited_month{-1};
    std::int64_t edited_income{};
    std::array<std::uint64_t, 2> edited_retired{};
    bool progression_complete{};
    int expanded_month{-1}, expanded_road_month{-1};
    std::int64_t expanded_income{};
    std::optional<ref::Position> expanded_road;
    int expanded_road_definition{-1};
    std::size_t sounds{}, peak_sounds{}, peak_payloads{}, peak_pages{}, peak_effects{},
        peak_actors{}, peak_cash{}, peak_tasks{}, peak_retired_actors{}, peak_retired_encounters{};
    int next_activity_attempt{}, completed{}, unlocked_completed{}, next_task_month{};
    std::optional<std::uint64_t> previous_task;
};

struct StartupWorldReplayOptions {
    std::string save_file, load_file, trace_file, save_directory;
    std::string producer_revision{"unspecified"}; // 生成可执行对应的研究checkpoint，仅作来源追溯。
    std::optional<int> save_at, stop_at, trace_from, save_every;
    bool seed_explicit{}, speed_explicit{};
};

// 独立的版本化测试载荷；拒绝缺字段、尾随字节、重复集合项和错误轮边界。
std::vector<std::uint8_t> encode_startup_world_replay_driver(const StartupWorldReplayDriver &);
StartupWorldReplayDriver decode_startup_world_replay_driver(const std::vector<std::uint8_t> &);
void validate_startup_world_replay_driver_world(const StartupWorldReplayDriver &,
                                               const StartupWorldRuntimeState &);
void validate_startup_world_replay_options(const StartupWorldReplayOptions &, int next_frame,
                                         int frame_limit);
bool startup_world_periodic_snapshot_due(const StartupWorldReplayOptions &, int completed_frame);

} // namespace ark::simulation::test
