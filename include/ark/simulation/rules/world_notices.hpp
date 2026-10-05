#pragma once

#include "ark/simulation/rules/world_scripts.hpp"

namespace ark::simulation::rules {
struct WorldNoticeCandidate {
    std::vector<WorldScriptNotice> notices;
    std::vector<int> sounds;
};
// d/a.q：只按原索引1、0推进，删除之后不在同轮补推进第三项。
std::optional<WorldNoticeCandidate>
prepare_world_notices(const std::vector<WorldScriptNotice> &notices);
struct WorldNoticePlacement {
    std::size_t index{};
    int offset{}; // 相对于底边，原绘制顺序1再0；不是平台屏幕坐标。
    int height{};
};
// 只读显示计划，8进入/80持有/8退出；不推进计数、声音或随机。
std::optional<std::vector<WorldNoticePlacement>>
world_notice_placements(const std::vector<WorldScriptNotice> &notices);
} // namespace ark::simulation::rules
