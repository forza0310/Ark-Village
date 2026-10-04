#include "ark/app/fixed_step_clock.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ark::app {
FixedStepClock::FixedStepClock(int ticks_per_second) {
    if (ticks_per_second < 1 || ticks_per_second > 240)
        throw std::invalid_argument("Logic tick rate must be 1..240 Hz");
    interval_ = 1.0 / ticks_per_second;
}
void FixedStepClock::reset() {
    pending_ = 0;
    running_ = false;
}
int FixedStepClock::advance(double elapsed_seconds, bool running) {
    if (!std::isfinite(elapsed_seconds) || elapsed_seconds < 0)
        throw std::invalid_argument("Invalid elapsed simulation time");
    if (!running) {
        reset();
        return 0;
    }
    if (!running_) {
        running_ = true;
        return 0;
    }
    // Bound debt before division, so a debugger stop cannot overflow the count or freeze input.
    pending_ += std::min(elapsed_seconds, interval_ * 8);
    const auto count =
        std::min(8, static_cast<int>(std::floor((pending_ + interval_ * 1e-9) / interval_)));
    pending_ = std::max(0.0, pending_ - count * interval_);
    return count;
}
} // namespace ark::app
