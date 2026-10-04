// Adapted from published research 1e50a60/c4ce4b2; independent product build.
#pragma once

// Pure arithmetic for the fixed APK's render/input gate; placement is a caller contract.
#include <cstdint>
#include <optional>

namespace ark::app {
struct OriginalLoopPacing {
    std::int64_t last_start_ms{}; // s committed after render/input wait; scene already updated.
    int parameter{20};            // e()==v-1; constructor v21. Not the measured x/w FPS estimate.
};
struct OriginalLoopWait {
    int minimum_period_ms{};
    std::int64_t remaining_ms{};
};
// Source pumps platform events with sleep(0) until the minimum start interval expires.
// Returns a wait query, not a recommendation to busy-wait in raylib or an actual sleep call.
std::optional<OriginalLoopWait> query_original_loop_wait(OriginalLoopPacing clock,
                                                         std::int64_t observed_ms);
// Advance from a fresh observation AFTER the wait. Slow updates adopt NOW, no catch-up debt.
std::optional<OriginalLoopPacing> start_original_loop(OriginalLoopPacing clock,
                                                      std::int64_t observed_ms);
// MainScene snapshots this outer loop bound: only mode0 and speed slot13 EXACTLY1 produce2.
// Each iteration must still re-evaluate scene/domain gates; this is not two unconditional AI ticks.
int original_scene_iterations(int mode, int speed_slot13);
} // namespace ark::app
