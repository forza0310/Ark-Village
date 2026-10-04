#pragma once

// One wall-clock adapter for source pacing or an explicitly requested fixed-rate experiment.
#include "ark/app/fixed_step_clock.hpp"
#include "ark/app/original_loop.hpp"

namespace ark::app {
class SimulationClock {
  public:
    explicit SimulationClock(int fixed_tick_rate = 0);
    // The original gate keeps running in modals; only eligible work is returned, never backlog.
    // Seconds are monotonic platform observations, converted to integer ms for the original gate.
    int advance(double observed_seconds, bool eligible);
    double remaining_seconds(double observed_seconds) const;
    void reset();
    bool original_pacing() const { return !fixed_; }

  private:
    std::optional<FixedStepClock> fixed_;
    OriginalLoopPacing original_;
    std::optional<double> last_observation_;
    bool eligible_{};
};
} // namespace ark::app
