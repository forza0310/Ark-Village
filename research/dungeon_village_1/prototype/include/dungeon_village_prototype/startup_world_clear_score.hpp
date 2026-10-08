#pragma once

// 通关计分的只读投影和raw17更新；系统写入／事件由应用Owner事务协调。
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace dungeon_village_prototype {
struct StartupWorldRuntimeState;
enum class StartupClearScoreError {
    none, invalid_input, missing_binding, numeric_overflow, already_finished
};
struct StartupClearScoreRow {
    std::int64_t count{};
    std::int64_t score{};
};
using StartupClearScoreRows = std::array<StartupClearScoreRow, 6>;
struct StartupClearScoreResult {
    StartupClearScoreError error{StartupClearScoreError::none};
    std::optional<StartupClearScoreRows> candidate;
};
StartupClearScoreResult startup_world_clear_score(const StartupWorldRuntimeState &state);

struct StartupClearScorePageState {
    int stage{}; // bo，0..7。
    int counter{}; // bp，仅本阶段。
    int row{}; // bq，完成后恰6。
    std::int64_t sum{}; // bs，stage3恰满只累加一次。
    std::int64_t captured_high_score{}; // bt，初始化捕获，不逐帧重读。
    bool finished{};
    bool new_record{};
    int trophy{}; // 仅完成且新高时返回1..4；非新高0，不覆盖旧系统档位。
};
struct StartupClearScorePageResult {
    StartupClearScoreError error{StartupClearScoreError::none};
    std::optional<StartupClearScorePageState> candidate;
    bool finished_pulse{};
    std::vector<int> sounds; // 原i()未直接c(sound)，目前为空。
};
// confirm是这一次Update的脉冲；先推进counter，随后才消费确认。
// 已完成显式拒绝，不把stage7的9999计数误作再次领奖。
StartupClearScorePageResult prepare_startup_clear_score_page(
    const StartupClearScoreRows &rows, const StartupClearScorePageState &state, bool confirm);
} // namespace dungeon_village_prototype
