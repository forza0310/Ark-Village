#include "ark/simulation/world/loop_pacing.hpp"

#include <limits>

namespace ark::simulation {
std::optional<OriginalLoopWait> query_original_loop_wait(OriginalLoopPacing c, std::int64_t now) {
    if (c.last_start_ms < 0 || now < 0 || c.parameter < 0 || c.parameter >= 1000)
        return {};
    const int period = 1000 / (c.parameter + 1);
    if (c.last_start_ms > std::numeric_limits<std::int64_t>::max() - period)
        return {};
    const auto deadline = c.last_start_ms + period;
    return OriginalLoopWait{period, now < deadline ? deadline - now : 0};
}
std::optional<OriginalLoopPacing> start_original_loop(OriginalLoopPacing c, std::int64_t now) {
    const auto query = query_original_loop_wait(c, now);
    if (!query || query->remaining_ms != 0)
        return {};
    c.last_start_ms = now;
    return c;
}
int original_scene_iterations(int mode, int speed) { return mode == 0 && speed == 1 ? 2 : 1; }
} // namespace ark::simulation
