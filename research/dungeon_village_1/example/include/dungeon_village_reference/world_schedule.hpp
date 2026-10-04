#pragma once

// 共同世界调度：这里只持有一个人物/设施所有者，控制与领域消费者不重复推进共同前段。
#include "dungeon_village_reference/ai_schedule.hpp"
#include "dungeon_village_reference/world_actor_tail.hpp"
#include "dungeon_village_reference/world_overlap.hpp"

#include <utility>

namespace dungeon_village_reference {
struct WorldScheduleState {
    RescueWorldState world;
    std::vector<int> surface;
    std::vector<std::uint32_t> map_flags;
    TownBounds town;
    std::vector<Position> spawn_cells;
    std::vector<std::uint64_t> facility_order;
    int updates{};                                    // UserData.p，不是日历、帧数或人物B。
    std::vector<std::vector<int>> hints;              // aI：只推进队首下标1，完整载荷原样保留。
    std::vector<std::vector<int>> floating_notes;     // aW：反向推进下标0，8时删除。
    std::vector<std::array<int, 3>> popularity_queue; // I：先减，再在<=0时提交。
    bool rescue_available{}; // S：本轮人物c前重算，设施status恰1且category2。
};
enum class WorldScheduleStage {
    arrival_front, // h.e之后、提示和人气之前；允许当前真实到访消费者创建人物。
    popularity,
    decision,         // 已完成共同c前段与R修复/抢占，只执行状态分支。
    carry_expression, // 已完成共同d前段/成长，N=-2的c17在v之前。
    control,          // 只执行一次v；普通hold仍执行尾部，delete跳过尾部。
    projectile,
    object,
    encounter,
    facility,
    finalize // 实际L重叠分离，必须显式提供消费者，不当作空收尾。
};
struct WorldScheduleCall {
    WorldScheduleStage stage{};
    std::optional<std::uint64_t> id;
    std::optional<std::array<int, 2>> popularity; // delta、flag==1；不是金币/点数。
};
enum class WorldScheduleDisposition { keep, remove_requested, already_removed };
struct WorldScheduleStep {
    WorldScheduleState state;
    WorldScheduleDisposition disposition{WorldScheduleDisposition::keep};
};
using WorldScheduleConsumer = std::function<std::optional<WorldScheduleStep>(
    const WorldScheduleState &, const WorldScheduleCall &, const CombatInfluenceCandidate &)>;
struct WorldScheduleInput {
    bool admitted{true};                 // 主场景/日期/菜单资格已由调用方判定；镜头延迟不是AI守卫。
    std::size_t dispatch_limit{1000000}; // 维护保护，不是原作人数/时间上限。
};
enum class WorldScheduleError {
    none,
    invalid_owner,
    missing_consumer,
    consumer_failed,
    invalid_mutation,
    common_segment_failed,
    dispatch_limit
};
struct WorldScheduleEffects {
    CharacterId actor;
    std::vector<ActorEffectSound> sounds;
    std::vector<HumanGrowthRequest> growth;
};
struct WorldScheduleCandidate {
    WorldScheduleState state;
    std::optional<CombatInfluenceCandidate> start_field;
    std::vector<AiScheduleVisit> visits;
    std::vector<WorldScheduleCall> calls;
    std::vector<WorldScheduleEffects> effects;
};
struct WorldScheduleResult {
    WorldScheduleError error{WorldScheduleError::none};
    std::optional<WorldScheduleCandidate> candidate;
};
WorldMapFacts world_schedule_facts(const WorldScheduleState &state);
bool valid_world_schedule_owner(const WorldScheduleState &state);
// 共同前置→两遍实时名单→共同尾部/原时点释放退休→bo/bp/bn/g→L。
// 实际删除消费者只删除一次，未实现域/随机/表现适配须返回失败；没有默认演示消费者。
// 回调只读取私有候选并返回新值；失败不暴露名单、扣款、队列或表现请求的部分结果。
WorldScheduleResult prepare_world_schedule(const WorldScheduleState &state,
                                           const WorldScheduleInput &input,
                                           const WorldScheduleConsumer &consumer);
// 将实际L纯规则写回唯一n位置，不刷新s/t/ax；票号必须按本轮真实pair顺序供给。
std::optional<WorldScheduleState> prepare_world_schedule_overlap(const WorldScheduleState &state,
                                                                 const std::vector<int> &tickets);

// 商店/探索/任务拥有额外数据时，仍只有外层Owner拥有事实。临时投影仅在私有副本中
// 进行，成功后共同数据和扩展数据一并返回，不能捕获并修改真实目录或随机生成器。
template <class Owner> struct OwnedWorldScheduleStep {
    Owner state;
    WorldScheduleDisposition disposition{WorldScheduleDisposition::keep};
};
template <class Owner> struct OwnedWorldScheduleAdapter {
    std::function<const WorldScheduleState &(const Owner &)> read;
    std::function<WorldScheduleState &(Owner &)> write;
    std::function<std::optional<OwnedWorldScheduleStep<Owner>>(
        const Owner &, const WorldScheduleCall &, const CombatInfluenceCandidate &)>
        consume;
};
template <class Owner> struct OwnedWorldScheduleResult {
    WorldScheduleError error{WorldScheduleError::none};
    std::optional<Owner> state;
    std::optional<WorldScheduleCandidate> audit;
};
template <class Owner>
OwnedWorldScheduleResult<Owner>
prepare_owned_world_schedule(const Owner &state, const WorldScheduleInput &input,
                             const OwnedWorldScheduleAdapter<Owner> &adapter) {
    if (!adapter.read || !adapter.write || (input.admitted && !adapter.consume))
        return {WorldScheduleError::missing_consumer, {}, {}};
    Owner scratch = state;
    const auto result = prepare_world_schedule(
        adapter.read(state), input,
        [&](const WorldScheduleState &common, const WorldScheduleCall &call,
            const CombatInfluenceCandidate &field) -> std::optional<WorldScheduleStep> {
            adapter.write(scratch) = common;
            auto step = adapter.consume(scratch, call, field);
            if (!step)
                return {};
            scratch = std::move(step->state);
            return WorldScheduleStep{adapter.read(scratch), step->disposition};
        });
    if (!result.candidate)
        return {result.error, {}, {}};
    adapter.write(scratch) = result.candidate->state;
    return {WorldScheduleError::none, std::move(scratch), result.candidate};
}
} // namespace dungeon_village_reference
