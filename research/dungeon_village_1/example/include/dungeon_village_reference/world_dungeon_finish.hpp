#pragma once

#include "dungeon_village_reference/world_dungeon.hpp"

namespace dungeon_village_reference {
// 阶段2的任务实例引用；definition可重复，identity不可替代为定义ID。
struct DungeonFinishTask {
    std::uint64_t identity{};
    int definition{};
    int difficulty{};
    int pending_completion_value{};
    std::optional<std::uint64_t> facility;
    std::optional<Position> site; // c.k.e；kind1也有坐标但无facility，已移除实例仍可被引用。
};
struct DungeonFinishSite {
    std::vector<Position> occupied_cells; // 原z顺序，首项是阶段2恢复调用格。
};
// LegacyMap只持有寻路字段；这些c/i字段由地图所有者按同一格索引保存。
struct DungeonFinishSurface {
    int definition{};         // b
    int updates{};            // f
    int instance{-1};         // 原c/i.m；边界/外部入口方向字段，不是维护facility identity。
    int fragment{-1};         // l，c/h也会重写，不等于LegacyMap绑定片号。
    int display_definition{}; // h
    int variant{};            // i
    int road_mask{};          // k
};
struct DungeonFinishState {
    DungeonWorldState dungeon;
    DungeonTaskSuccessState task_progress; // tasks definitions/date/statistics唯一所有者。
    std::map<std::uint64_t, DungeonFinishTask> tasks;
    std::vector<std::uint64_t> task_order;               // bq，保留重复引用和原序。
    std::optional<std::uint64_t> active_task;            // UserData.h
    std::vector<int> participants;                       // UserData.m，清h不清此表。
    std::map<int, std::uint32_t> human_definition_flags; // 全bv，不只bl或参与者。
    std::map<std::uint64_t, DungeonFinishSite> sites;
    std::vector<DungeonFinishSurface> surface;
    int ground_definition{};        // o.S.n，不补默认定义号。
    std::map<int, int> event_calls; // aM，判断201/92必须读取此前事件执行后的实时值。
    int raw_year{};
    int raw_month{};
};
enum class DungeonFinishEffectKind {
    actor_reward_display,
    summary30,
    summary32,
    event,
    site_success_display,
    rebuild_display,
    rebuild_roads_fences,
    refresh_scene,
    rebuild_neighbours,
    threshold_notice
};
struct DungeonFinishEffect {
    DungeonFinishEffectKind kind;
    std::optional<CharacterId> actor;
    std::optional<Position> cell;
    std::optional<std::uint64_t> task;
    int first{};
    int second{};
    std::vector<std::array<int, 2>> summary_rewards;
};
// 私有候选上的同步真实消费者；不能在候选外提交不可回滚副作用。
// 事件要执行脚本到返回/续体点并登记aM；
// 页面要提交真实页面栈，地图两个rebuild要重算全图，不能用排队通知替代。
// 没有默认成功回调；测试替身必须明确标识为夹具，不代表窗口/脚本已验收。
using DungeonFinishConsumer = std::function<std::optional<DungeonFinishState>(
    const DungeonFinishState &, const DungeonFinishEffect &)>;
struct DungeonFinishInput {
    std::uint64_t facility{};
    std::vector<int> summary_tickets; // 两次独立participants.size抽选。
    std::function<std::optional<int>(int)> draw{};
};
enum class DungeonFinishError {
    none,
    invalid_input,
    stale_facility,
    stale_task,
    missing_consumer,
    consumer_failed,
    numeric_overflow
};
struct DungeonFinishCandidate {
    DungeonFinishState state;
    std::vector<DungeonCompletionRequest> requests;
    std::vector<DungeonFinishEffect> effects;
    std::size_t consumed_summary_tickets{};
    bool site_restored{};
    bool task_cleared{};
};
struct DungeonFinishResult {
    DungeonFinishError error{DungeonFinishError::none};
    std::optional<DungeonFinishCandidate> candidate;
};
// c/m阶段2 AFTER计数前缀；不跑Tenant.d或人物AI。不变更输入，晚期失败整段回滚。
DungeonFinishResult prepare_world_dungeon_finish(const DungeonFinishState &state,
                                                 const DungeonFinishInput &input,
                                                 const DungeonFinishConsumer &consumer = {});
} // namespace dungeon_village_reference
