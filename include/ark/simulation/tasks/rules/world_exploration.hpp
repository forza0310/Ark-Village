#pragma once

#include "ark/simulation/tasks/rules/world_dungeon_finish.hpp"
#include "ark/simulation/map/rules/world_map_refresh.hpp"
#include "ark/simulation/village/rules/world_scripts.hpp"

namespace ark::simulation::rules {
// 仅保存共同地图尚未具备的字段，绝不长期保存另一份map/surface/设施。
struct WorldExplorationMapExtra {
    std::map<int, WorldMapDefinition> definitions;
    int special_ground_definition{-1};
    std::vector<int> base_variants;
    int fence_level{-1};
    std::vector<std::array<Position, 2>> fence_levels;
    std::vector<std::uint64_t> facility_order; // 原g，移除后仍保留其他实例顺序。
    std::map<std::uint64_t, WorldMapNeighbourCache> neighbours;
    std::vector<bool> road_quad;
    std::vector<bool> edge_road_pair;
    bool refresh_pending{};
};
// aM由finish持有，f215e由finish.dungeon.world.ai持有，页面和I另有唯一所有者。
struct WorldExplorationScriptExtra {
    std::vector<WorldScriptContinuation> continuations;
    int context{-1};
    std::string village_name;
    std::optional<std::uint64_t> executing_page;
    std::uint64_t next_page_id{1};
    bool page_mutations_locked{};
    std::optional<std::uint64_t> selected_actor;
    std::optional<std::uint64_t> selected_facility;
    std::optional<std::uint64_t> selected_monster;
};
struct WorldExplorationSummary {
    int legacy_page{};    // 30或32。
    std::uint64_t task{}; // aa引用；清h后仍可访问原任务对象。
    int definition{};     // q引用，名称/目录由表现层按此查真实目录。
    // f/g只保存于对应WorldScriptPage.legacy_f/g，不重复两份页面参数。
    std::vector<std::array<int, 2>> rewards; // 页32的X，摘要不再发放。
};
struct WorldExplorationGroundDisplay {
    Position cell; // 必须由表现层转换原h.j，绝不猜屏幕坐标。
    int kind{10};
    int delay{};
    int x_offset{30};
    int y_offset{-15};
};
struct WorldExplorationMessage {
    std::string text;
    std::array<int, 3> state{}; // 原S消息载荷：[ID,-az[ID],80]。
};
struct WorldExplorationUiState {
    std::vector<WorldScriptPage> pages; // 脚本与成果页共用框架栈。
    std::map<std::uint64_t, WorldExplorationSummary> summaries;
    std::vector<WorldExplorationGroundDisplay> ground_displays; // 未渲染的明确请求，不是占位成功。
    std::vector<WorldExplorationMessage> messages;
};
struct WorldExplorationState {
    DungeonFinishState finish;
    WorldExplorationMapExtra map;
    WorldExplorationScriptExtra scripts;
    WorldExplorationUiState ui;
    std::vector<std::array<int, 3>> popularity_queue; // UserData.I唯一所有者。
};
enum class WorldExplorationError {
    none,
    invalid_input,
    finish_failed,
    map_failed,
    script_failed,
    page_failed,
    display_failed
};
struct WorldExplorationCandidate {
    WorldExplorationState state;
    std::vector<DungeonCompletionRequest> requests;
    std::vector<DungeonFinishEffect> effects;
    std::vector<WorldMapRefreshStep> map_steps;
    std::vector<WorldScriptTrace> script_trace;
    std::size_t consumed_summary_tickets{};
    bool site_restored{};
    bool task_cleared{};
};
struct WorldExplorationResult {
    WorldExplorationError error{WorldExplorationError::none};
    DungeonFinishError finish_error{DungeonFinishError::none};
    WorldMapRefreshError map_error{WorldMapRefreshError::none};
    WorldScriptError script_error{WorldScriptError::none};
    std::optional<WorldExplorationCandidate> candidate;
};
// 阶段2整个共同事务：真实奖励队列/页面/脚本/地图/邻接/统计，错误整体丢弃。
// 不执行c前缀、人物AI、全场景或真实绘制；金额与目录必须来自现存共同所有者。
// input.draw仅借用外层私有随机候选；晚期失败时外层一并丢弃其游标，不提交真实流。
WorldExplorationResult prepare_world_exploration_finish(const WorldScriptCatalog &catalog,
                                                        const WorldExplorationState &state,
                                                        const DungeonFinishInput &input);
// 主场景真正获准推进时调用一次续体段，不推进AI或延迟人气队列。
WorldExplorationResult prepare_world_exploration_continuations(const WorldScriptCatalog &catalog,
                                                               const WorldExplorationState &state,
                                                               bool admitted);
} // namespace ark::simulation::rules
