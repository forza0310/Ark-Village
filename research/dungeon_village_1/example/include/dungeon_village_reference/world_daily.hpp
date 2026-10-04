#pragma once

#include "dungeon_village_reference/object_commit.hpp"
#include "dungeon_village_reference/world_departure.hpp"
#include "dungeon_village_reference/world_lifecycle.hpp"

namespace dungeon_village_reference {
// 这是一次状态分支的临时投影；外层世界与任务/地图事实只有一个持久所有者。
struct WorldDailyState {
    RescueWorldState world;
    WorldMapFacts facts;
    WorldEventTask task;
};
using WorldDailyEventConsumer =
    std::function<std::optional<RescueWorldState>(const RescueWorldState &, int)>;
struct WorldDailyInput {
    CharacterId actor;
    std::vector<WorldExpressionTicket> expressions;
    std::optional<int> boost_ticket;
    std::optional<int> spawn_ticket; // L真实需要时才读取，不因上限/状态0概率0免抽。
    std::optional<EncounterCreationInput> spawn_creation;
    std::vector<Position> task_centers; // 原bq全任务，不只active task。
    std::optional<WorldEventEntryInput> task_creation;
    std::optional<WorldPathInput> path;
    std::optional<CollisionBox> actor_box;
    std::optional<CollisionBox> object_box;
    WorldDailyEventConsumer event;       // 登场90必须同步执行到返回/续体点，不仅排ID。
    EncounterCreationConsumer encounter; // 各介绍/89在spawn之前，同步Owner事务。
    WorldExpressionDraw expression_draw{};
    std::function<std::optional<int>(int)> draw{};
};
enum class WorldDailyError {
    none,
    invalid_input,
    stale_actor,
    missing_fact,
    missing_ticket,
    missing_consumer,
    consumer_failed,
    preparation_failed,
    unsupported_state
};
struct WorldDailyCandidate {
    WorldDailyState state;
    std::size_t consumed_expressions{};
    std::size_t consumed_variants{};
    bool consumed_boost{};
    bool consumed_spawn{};
    std::optional<EncounterCreationCandidate> spawn;
    std::optional<WorldEventEntryCandidate> task_entry;
    std::optional<WorldPathCandidate> path;
    std::optional<PickupCommitCandidate> pickup;
    bool ground_effect21{};          // 登场旧B65，位置为旧u，不伪造为当前n/s。
    std::vector<int> event_requests; // setter新116；90已由同步消费者提交。
};
struct WorldDailyResult {
    WorldDailyError error{WorldDailyError::none};
    std::optional<WorldDailyCandidate> candidate;
};
// 共同c前段/引用抢占以后，实际0:L→P，5:表情/任务/F/救援/重选→L，
// 8/9:au登场→事件90→c17/队列，11:F→H/碰撞→拾取或6.7追物。
// 无共同计数、无缓存格刷新、无默认成功外部消费者；晚期失败整段无候选。
WorldDailyResult prepare_world_daily_c(const WorldDailyState &state, const WorldDailyInput &input);
} // namespace dungeon_village_reference
