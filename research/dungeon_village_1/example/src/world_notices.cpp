#include "dungeon_village_reference/world_notices.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
namespace {
bool valid(const std::vector<WorldScriptNotice> &notices) {
    return std::all_of(notices.begin(), notices.end(), [](const auto &n) {
        return n.duration >= 0 && n.duration <= std::numeric_limits<int>::max() - 16;
    });
}
} // namespace
std::optional<WorldNoticeCandidate>
prepare_world_notices(const std::vector<WorldScriptNotice> &notices) {
    if (!valid(notices))
        return {};
    WorldNoticeCandidate next{notices, {}};
    const auto count = std::min<std::size_t>(notices.size(), 2);
    for (auto index = count; index > 0; --index) {
        auto &n = next.notices[index - 1];
        if (n.counter == std::numeric_limits<int>::max())
            return {};
        ++n.counter;
        if (n.counter == 1)
            next.sounds.push_back(11);
        if (n.counter > n.duration + 16)
            next.notices.erase(next.notices.begin() + static_cast<std::ptrdiff_t>(index - 1));
    }
    return next;
}
std::optional<std::vector<WorldNoticePlacement>>
world_notice_placements(const std::vector<WorldScriptNotice> &notices) {
    if (!valid(notices))
        return {};
    std::vector<WorldNoticePlacement> plan;
    int total{};
    for (auto index = std::min<std::size_t>(notices.size(), 2); index > 0; --index) {
        const auto &n = notices[index - 1];
        if (n.counter <= 0)
            continue;
        const int full = index == 1 ? 21 : 19;
        // 过期记录仍可能被只读查询；先在宽整数内丢弃非正高度，避免窄化回绕。
        const std::int64_t raw_height =
            n.counter <= 8 ? full * n.counter / 8
            : n.counter >= n.duration + 8
                ? full - (static_cast<std::int64_t>(n.counter) - n.duration - 8) * full / 8
                : full;
        if (raw_height <= 0)
            continue;
        const auto height = static_cast<int>(raw_height);
        total += height;
        plan.push_back({index - 1, 0, height});
    }
    int offset = -total;
    for (auto &p : plan) {
        p.offset = offset;
        offset += p.height;
    }
    return plan;
}
} // namespace dungeon_village_reference
