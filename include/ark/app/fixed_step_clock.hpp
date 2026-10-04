#pragma once

// Wall-clock adapter only. Game continues to accept discrete eligible updates, not seconds.
namespace ark::app {
class FixedStepClock {
  public:
    explicit FixedStepClock(int ticks_per_second);
    // Returns outer updates due this frame. Resume discards the interval spanning blocked time;
    // long stalls admit at most eight updates, discarding elapsed time beyond that budget.
    int advance(double elapsed_seconds, bool running);
    void reset();
    double remaining_seconds() const { return interval_ - pending_; }

  private:
    double interval_;
    double pending_{};
    bool running_{};
};
} // namespace ark::app
