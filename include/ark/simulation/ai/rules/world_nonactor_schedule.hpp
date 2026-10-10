#pragma once

#include "ark/simulation/combat/rules/object_commit.hpp"
#include "ark/simulation/combat/rules/world_encounters.hpp"
#include "ark/simulation/world/rules/world_random_consumers.hpp"
#include "ark/simulation/ai/rules/world_schedule.hpp"

namespace ark::simulation::rules {
// 共同Owner调用点临时投影。objects.objects/events从common重建，不可作为第二个持久所有者。
struct WorldNonactorScheduleState {
    WorldScheduleState common;
    WorldRandomStream random;
    ObjectCommitState objects;
};
enum class WorldNonactorRequestKind {
    projectile_contact,
    projectile_visual,
    projectile_hit,
    object,
    encounter
};
struct WorldNonactorRequest {
    WorldNonactorRequestKind kind{};
    std::uint64_t identity{};
    std::optional<CharacterId> caster;
    std::optional<CharacterId> target;
    int visual{}; // 22落地，其他为真实法术effect；接触是独立cd10请求，不是效果20。
    std::optional<HitRequest> hit;
    std::optional<ObjectCommitRequest> object;
    std::optional<EncounterRequest> encounter;
    std::optional<CombatPoint> source_position{}; // 被移除投射/物体仍需原调用点n。
};
struct WorldNonactorWriteback {
    EncounterExternalWriteback globals;
    std::optional<int> target_facing; // face_attacker必须给真实当前方向；不能任意回填人物。
    std::optional<std::vector<std::array<int, 3>>> popularity_queue{}; // 脚本22的真实I提交。
    std::optional<ActorEffectState> target_effects{}; // 仅contact/命中法术目标cd/ce，非任意AI写回。
};
using WorldNonactorConsumer = std::function<std::optional<WorldNonactorWriteback>(
    const WorldNonactorScheduleState &, const WorldNonactorRequest &)>;
struct WorldNonactorStageInput {
    WorldScheduleCall call;
    std::optional<WorldProjectileInput> projectile;
    std::optional<EncounterCommitInput> encounter;
    std::optional<bool> primary_expression_table;
};
enum class WorldNonactorError {
    none,
    invalid_owner,
    unsupported_stage,
    missing_input,
    missing_consumer,
    domain_failed,
    external_failed,
    random_failed
};
struct WorldNonactorCandidate {
    WorldNonactorScheduleState state;
    WorldScheduleDisposition disposition{WorldScheduleDisposition::keep};
    std::vector<WorldNonactorRequest> consumed;
};
struct WorldNonactorResult {
    WorldNonactorError error{WorldNonactorError::none};
    std::optional<WorldNonactorCandidate> candidate;
};
// 只接真实bo/bp/bn/L。到访、人气、设施、人物必须交各自实际消费者，未支持直接拒绝。
// expression在事件原req位置使用同一私有random；外部脚本/任务/表现必须消费不能只排通知。
WorldNonactorResult prepare_world_nonactor_stage(const WorldNonactorScheduleState &state,
                                                 const WorldNonactorStageInput &input,
                                                 const CombatInfluenceCandidate &start_field,
                                                 const WorldNonactorConsumer &consumer);
template <class Owner> struct WorldNonactorScheduleAdapter {
    std::function<const WorldScheduleState &(const Owner &)> read_common;
    std::function<WorldScheduleState &(Owner &)> write_common;
    std::function<WorldNonactorScheduleState(const Owner &)> read_routes;
    // 只写目录/随机等本路由扩展；共同数据由模板强制写回最新候选，禁止恢复旧事实。
    std::function<bool(Owner &, const WorldNonactorScheduleState &)> write_routes;
    std::function<std::optional<WorldProjectileInput>(const Owner &, std::uint64_t)> projectile;
    std::function<std::optional<EncounterCommitInput>(const Owner &, std::uint64_t)> encounter;
    std::optional<bool> primary_expression_table;
    // callback接收已发布当前候选的私有Owner，实际消费scripts/pages/task/渲染输出。
    // 需要更广AI字段写回而不在typed writeback中时必须拒绝；不能静默丢掉脚本真实效果。
    std::function<std::optional<WorldNonactorWriteback>(Owner &, const WorldNonactorRequest &)>
        request;
    std::function<std::optional<OwnedWorldScheduleStep<Owner>>(
        const Owner &, const WorldScheduleCall &, const CombatInfluenceCandidate &)>
        other;
};
// 直接用作WorldActorScheduleAdapter.other，或供prepare_owned_world_schedule消费非人物阶段。
template <class Owner>
std::optional<OwnedWorldScheduleStep<Owner>>
prepare_owned_world_nonactor_stage(const Owner &state, const WorldScheduleCall &call,
                                   const CombatInfluenceCandidate &field,
                                   const WorldNonactorScheduleAdapter<Owner> &adapter) {
    if (call.stage == WorldScheduleStage::arrival_front ||
        call.stage == WorldScheduleStage::popularity || call.stage == WorldScheduleStage::facility)
        return adapter.other ? adapter.other(state, call, field) : std::nullopt;
    if (!adapter.read_common || !adapter.write_common || !adapter.read_routes ||
        !adapter.write_routes)
        return {};
    Owner next = state;
    auto routes = adapter.read_routes(state);
    routes.common = adapter.read_common(state);
    WorldNonactorStageInput input{call, {}, {}, adapter.primary_expression_table};
    if (call.stage == WorldScheduleStage::projectile && call.id && adapter.projectile)
        input.projectile = adapter.projectile(state, *call.id);
    if (call.stage == WorldScheduleStage::encounter && call.id && adapter.encounter)
        input.encounter = adapter.encounter(state, *call.id);
    WorldNonactorConsumer consume;
    if (adapter.request)
        consume =
            [&](const WorldNonactorScheduleState &current,
                const WorldNonactorRequest &request) -> std::optional<WorldNonactorWriteback> {
            if (!adapter.write_routes(next, current))
                return {};
            adapter.write_common(next) = current.common;
            auto fields = adapter.request(next, request);
            if (!fields ||
                (adapter.read_common(next).popularity_queue != current.common.popularity_queue &&
                 !fields->popularity_queue))
                return {};
            return fields;
        };
    const auto result = prepare_world_nonactor_stage(routes, input, field, consume);
    if (!result.candidate || !adapter.write_routes(next, result.candidate->state))
        return {};
    adapter.write_common(next) = result.candidate->state.common;
    return OwnedWorldScheduleStep<Owner>{std::move(next), result.candidate->disposition};
}
} // namespace ark::simulation::rules
