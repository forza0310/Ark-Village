#include "ark/simulation/facility_projection.hpp"

namespace ark::simulation {
std::optional<FacilityUseProjection>
project_facility_use_target(ark::simulation::rules::Position s, int category, int detail,
                            std::optional<int> direction) {
    if (s.x < 0 || s.x > 9999 || s.y < 0 || s.y > 9999)
        return {};
    int x = (s.x + s.y) * 30;
    int y = (s.y - s.x) * 15 + 15;
    if (category == 6 && detail == 3) {
        if (direction)
            return {};
        x += 22;
        y -= 14;
    } else if (category == 8 && detail == 2) {
        if (!direction || *direction < 0 || *direction > 3)
            return {};
        x += 30 + (1 - (*direction / 2) * 2) * 7;
        y += -15 + ((*direction == 0 || *direction == 3) ? 1 : -1);
    } else
        return {};
    // Keep the original float interpolation order; premature integer division changes targets.
    const float px = static_cast<float>(x), py = static_cast<float>(y);
    const float wx = ((px - py * 60.0F / 30.0F) * 100.0F) / 60.0F;
    const float wz = ((py + px * 30.0F / 60.0F) * 100.0F) / 30.0F;
    return FacilityUseProjection{{x, y}, {static_cast<int>(wx), static_cast<int>(wz)}};
}
} // namespace ark::simulation
