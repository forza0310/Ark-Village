#pragma once

#include "ark/simulation/village/rules/world_scripts.hpp"

namespace ark::simulation::rules {
struct WorldPopularityReward {
    int definition{};
    int legacy_tag{};
    int threshold{};
    int human{};
    int facility{};
    WorldScriptProgram program;
    std::uint32_t flags{};
    int status{};
    bool pending_notice{};
};
// 只保存人气自己的事实；脚本、目录、页面必须是唯一外层Owner在调用点的临时投影。
struct WorldPopularityState {
    int popularity{};                           // f214d，最低-50，不封顶。
    int maximum{};                              // f216f，先更新，奖励判断仍保留旧最大值。
    bool reward_display{};                      // R，页97真正更新时才清，不以关闭任意页面清除。
    std::vector<std::array<int, 3>> pulses;     // bu，每次显示合并清全部旧项。
    std::vector<WorldPopularityReward> rewards; // bC原目录序，非threshold重排序。
    WorldScriptState scripts;
};
enum class WorldPopularityError { none, invalid_input, invalid_table, overflow, script_failed };
struct WorldPopularityTableResult {
    WorldPopularityError error{WorldPopularityError::none};
    std::optional<std::vector<WorldPopularityReward>> rewards;
};
WorldPopularityTableResult parse_world_popularity_rewards(const std::string &table);
struct WorldPopularityCandidate {
    WorldPopularityState state;
    std::optional<int> awarded_definition;
    std::vector<int> invoked_events;
    std::vector<WorldScriptTrace> trace;
};
struct WorldPopularityResult {
    WorldPopularityError error{WorldPopularityError::none};
    WorldScriptError script_error{WorldScriptError::none};
    std::optional<WorldPopularityCandidate> candidate;
};
// 原UserData.a(delta,show)：结算I队列后立即提交奖励程序与新闻续体，全部失败回滚。
// 每次至多发第一个满足的未领目录；超10000且旧/新peak跨百才重放最后目录程序。
WorldPopularityResult prepare_world_popularity(const WorldScriptCatalog &catalog,
                                               const WorldPopularityState &state, int delta,
                                               bool show_notice);
// b/g.java页97的实际更新：清R并标页面关闭，不推进世界或续体。
WorldPopularityResult prepare_world_popularity_unlock_page(const WorldPopularityState &state,
                                                           std::uint64_t page);
// 原av.g(identity)恢复所需程序注册；不把奖励程序混入aL事件计数。
std::optional<WorldScriptCatalog>
world_popularity_script_catalog(const WorldScriptCatalog &catalog,
                                const std::vector<WorldPopularityReward> &rewards);
} // namespace ark::simulation::rules
