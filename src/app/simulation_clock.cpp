#include "ark/app/simulation_clock.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace ark::app {
namespace {
std::int64_t milliseconds(double seconds) {
    if (!std::isfinite(seconds) || seconds < 0 ||
        seconds >= static_cast<double>(std::numeric_limits<std::int64_t>::max() / 1000 - 1))
        throw std::invalid_argument("Invalid simulation clock observation");
    return static_cast<std::int64_t>(seconds * 1000);
}
} // namespace
SimulationClock::SimulationClock(int fixed_tick_rate) {
    if (fixed_tick_rate != 0)
        fixed_.emplace(fixed_tick_rate);
}
void SimulationClock::reset() {
    last_observation_.reset();
    original_ = {};
    eligible_ = false;
    if (fixed_)
        fixed_->reset();
}
int SimulationClock::advance(double now, bool eligible) {
    const auto now_ms = milliseconds(now);
    if (!last_observation_) {
        eligible_ = eligible;
        last_observation_ = now;
        original_.last_start_ms = now_ms;
        if (fixed_)
            fixed_->advance(0, eligible);
        return 0;
    }
    const auto elapsed = now - *last_observation_;
    if (fixed_) {
        const auto due = fixed_->advance(elapsed, eligible);
        eligible_ = eligible;
        last_observation_ = now;
        return due;
    }
    eligible_ = eligible;
    last_observation_ = now;
    const auto next = start_original_loop(original_, now_ms);
    if (!next)
        return 0;
    original_ = *next; // Commit actual observed start, including when the scene blocks work.
    return eligible ? 1 : 0;
}
double SimulationClock::remaining_seconds(double now) const {
    const auto now_ms = milliseconds(now);
    if (!last_observation_)
        return 0;
    if (fixed_) {
        if (!eligible_)
            return std::numeric_limits<double>::infinity();
        return std::max(0.0, fixed_->remaining_seconds() - (now - *last_observation_));
    }
    const auto wait = query_original_loop_wait(original_, now_ms);
    if (!wait)
        throw std::invalid_argument("Original loop deadline overflow");
    return wait->remaining_ms / 1000.0;
}
} // namespace ark::app
