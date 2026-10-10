#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace ark::desktop {
struct WorldRenderSummary {
    std::size_t samples{};
    double interval_p50_ms{}, interval_p95_ms{}, cost_p95_ms{};
};

// Ordinary player sessions retain no per-frame history. Exact percentiles are only for a
// bounded --frames diagnostic; its launch budget caps storage even before capture is ready.
class WorldRenderStatistics {
  public:
    explicit WorldRenderStatistics(int frame_budget)
        : budget_(frame_budget > 0 ? static_cast<std::size_t>(frame_budget) : 0) {}
    void interval(int frame, double milliseconds) {
        if (frame > 20 && intervals_.size() < budget_)
            intervals_.push_back(milliseconds);
    }
    void cost(int frame, double milliseconds) {
        if (frame > 20 && costs_.size() < budget_)
            costs_.push_back(milliseconds);
    }
    WorldRenderSummary finish() {
        // Sort once in place: exit reporting neither copies histories nor sorts each percentile.
        std::sort(intervals_.begin(), intervals_.end());
        std::sort(costs_.begin(), costs_.end());
        return {intervals_.size(), percentile(intervals_, .5), percentile(intervals_, .95),
                percentile(costs_, .95)};
    }

  private:
    static double percentile(const std::vector<double> &values, double fraction) {
        return values.empty() ? 0.0
                              : values[static_cast<std::size_t>((values.size() - 1) * fraction)];
    }
    std::size_t budget_{};
    std::vector<double> intervals_, costs_;
};
} // namespace ark::desktop
