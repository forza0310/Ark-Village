// Preserve integer truncation at each percentage group, rounding-before-cap for maintenance,
// and the second cap after instance addition. This query does not perform monthly accounting.
#include "ark/facilities/economy.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace ark::facilities {
namespace {
bool valid(Endpoints v) { return v.first >= 0 && v.fifth >= 0; }
std::int64_t interpolate(Endpoints v, int numerator, int denominator) {
    return v.first + static_cast<std::int64_t>(numerator) *
                         (static_cast<std::int64_t>(v.fifth) - v.first) / denominator;
}
void percent(std::int64_t &value, std::int64_t multiplier) {
    if (value > std::numeric_limits<std::int64_t>::max() / multiplier ||
        value < std::numeric_limits<std::int64_t>::min() / multiplier)
        throw std::overflow_error("Facility price multiplication overflow");
    value = value * multiplier / 100;
}
} // namespace
EconomyValues derive_economy(const EconomyDefinition &d, const EconomyInput &input) {
    if (input.level < 1 || input.level > 5 || d.construction_cost < 0 || d.construction_ticks < 0 ||
        !valid(d.upgrade_uses) || !std::all_of(d.attributes.begin(), d.attributes.end(), valid) ||
        std::any_of(input.job_counts.begin(), input.job_counts.end(), [](int v) { return v < 0; }))
        throw std::invalid_argument("Invalid facility economy input");
    EconomyValues result;
    for (std::size_t slot = 0; slot < 4; ++slot) {
        auto value = interpolate(d.attributes[slot], input.level - 1, 4) + input.improvements[slot];
        if (slot == 0) {
            const std::uint32_t primary_flags[] = {4096, 8192, 512, 2048, 65536};
            const int groups[] = {2, 3, 4, 6, 5};
            for (int i = 0; i < 5; ++i)
                if (d.flags & primary_flags[i]) {
                    percent(value, 100 + 20LL * input.job_counts[groups[i]]);
                    break;
                }
            const int second = (d.flags & 131072) ? 0 : (d.flags & 262144) ? 1 : -1;
            if (second >= 0)
                percent(value, 100 + 10LL * input.job_counts[second]);
        }
        if (slot == 3)
            value -= value % 10;
        const auto cap = static_cast<std::int64_t>(d.attributes[slot].fifth) * 2;
        result.definition[slot] = std::min(value, cap);
        result.instance[slot] =
            std::min(result.definition[slot] + (slot < 3 ? input.modifiers[slot] : 0), cap);
    }
    result.construction_cost = d.construction_cost;
    if (d.decoration) {
        result.construction_cost = interpolate({d.construction_cost, d.construction_cost / 4},
                                               std::min(input.job_counts[8], 3), 3);
        result.construction_cost -= result.construction_cost % 10;
    }
    result.construction_ticks = interpolate({d.construction_ticks, d.construction_ticks / 10},
                                            std::min(input.job_counts[9], 3), 3);
    result.upgrade_uses = interpolate(d.upgrade_uses, input.level - 1, 4);
    result.upgrade_ready = d.upgrade_uses.fifth > 0 && input.level < 5 &&
                           input.completed_uses >= static_cast<std::uint64_t>(result.upgrade_uses);
    return result;
}
} // namespace ark::facilities
