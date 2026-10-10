#pragma once

#include "ark/simulation/actors/rules/world_actor_routes.hpp"
#include "ark/simulation/ai/rules/world_schedule.hpp"

namespace ark::simulation::rules {
// 外层Owner只保存一次真实world。read_routes按本次共同状态生成值投影，write_routes
// 将当前世界/目录/随机及其他扩展一起写回私有Owner，不恢复过时共同字段。
template <class Owner> struct WorldActorScheduleAdapter {
    std::function<const WorldScheduleState &(const Owner &)> read_common;
    std::function<WorldScheduleState &(Owner &)> write_common;
    std::function<WorldActorRoutesState(const Owner &)> read_routes;
    std::function<bool(Owner &, const WorldActorRoutesState &)> write_routes;
    std::function<std::optional<WorldActorDecisionInput>(const Owner &, CharacterId)> decision;
    WorldActorCommandProvider command;
    // 跨域事件可改住宅/职业/邻接，必须在当前队首发布后再读外层事实。
    std::function<std::optional<WorldActorCommandInput>(
        const Owner &, const WorldActorRoutesState &, CharacterId, const LegacyActorControl &)>
        owned_command;
    std::optional<bool> primary_expression_table;
    // 事件在路由实际调用点先发布当前候选，再同步消费脚本/页栈等外层事实。
    // 不能只回传AiRewardState后丢弃Owner中的续体、页面与目录修改。
    std::function<std::optional<Owner>(const Owner &, int)> event;
    std::function<std::optional<Owner>(const Owner &, CharacterId, const BattleActorRecord &)>
        tail_cache;
    std::function<std::optional<int>(const Owner &, CharacterId, const BattleActorRecord &)>
        projected_facing{};
    std::function<std::optional<Owner>(const Owner &, const EncounterCreationRequest &)> encounter;
    std::function<std::optional<Owner>(const Owner &, const WorldActorPresentationRequest &)>
        presentation;
    // 到访/人气/投射/物体/事件/设施/最后L必须是实际消费者，不能默认成功。
    std::function<std::optional<OwnedWorldScheduleStep<Owner>>(
        const Owner &, const WorldScheduleCall &, const CombatInfluenceCandidate &)>
        other;
    // 以下可选消费者只借用共同调度的私有Owner；不得保留引用。
    // other_private启用复用草稿路径；跨域事件沿用上方完整值接口。
    std::function<std::optional<WorldScheduleDisposition>(Owner &, const WorldScheduleCall &,
                                                          const CombatInfluenceCandidate &)>
        other_private{};
    std::function<bool(Owner &, CharacterId, const BattleActorRecord &)> tail_cache_private{};
    // 完整当前投影：world/facts/人气与当前共同Owner一致；扩展字段仍须完整。
    // 适配者负责原任务旗标等投影校验，不能靠跳过共同覆盖改变规则输入。
    std::function<WorldActorRoutesState(const Owner &)> read_current_routes{};
    // 完整写回及原验证/规范化：成功时共同与扩展字段都已经发布。
    std::function<bool(Owner &, const WorldActorRoutesState &)> write_current_routes{};
    // 复用本调用点刚取得的完整路线投影；不缓存或提前读取下一人物输入。
    std::function<std::optional<WorldActorDecisionInput>(
        const Owner &, const WorldActorRoutesState &, CharacterId)>
        decision_from_routes{};
    // 只读非common扩展与参数actor；不依赖共同Owner的after-control副本。
    std::function<std::optional<int>(const Owner &, CharacterId, const BattleActorRecord &)>
        projected_facing_without_common{};
};
template <class Owner> struct WorldActorScheduleResult {
    WorldScheduleError error{WorldScheduleError::none};
    std::optional<Owner> state;
    std::optional<WorldScheduleCandidate> audit;
    std::vector<WorldActorDecisionCandidate> decisions;
    std::vector<WorldActorControlCandidate> controls;
    std::optional<WorldScheduleFailure> failure{};
};
// 共同c前段→全状态路由；共同d/成长→携物表情→全码v→原d尾部/删除点。
// 审计候选只保存读取时点，不是可写世界，不能把它们长期作为第二个Owner。
template <class Owner>
WorldActorScheduleResult<Owner>
prepare_world_actor_schedule(const Owner &state, const WorldScheduleInput &input,
                             const WorldActorScheduleAdapter<Owner> &adapter) {
    if (!adapter.read_common || !adapter.write_common ||
        (!adapter.read_routes && !adapter.read_current_routes) ||
        (!adapter.write_routes && !adapter.write_current_routes) ||
        (input.admitted && ((!adapter.decision && !adapter.decision_from_routes) ||
                            (!adapter.other && !adapter.other_private))))
        return {WorldScheduleError::missing_consumer, {}, {}, {}, {}};
    WorldActorScheduleResult<Owner> output;
    OwnedWorldScheduleAdapter<Owner> owned;
    owned.read = adapter.read_common;
    owned.write = adapter.write_common;
    owned.projected_facing = adapter.projected_facing;
    owned.projected_facing_without_common = adapter.projected_facing_without_common;
    // 旧presentation/encounter回调原本直接读取routes；保留该默认接口的语义。
    const auto read_routes = [&](const Owner &owner, bool refresh_common) {
        if (adapter.read_current_routes && (refresh_common || !adapter.read_routes))
            return adapter.read_current_routes(owner);
        auto routes = adapter.read_routes(owner);
        if (refresh_common) {
            const auto &common = adapter.read_common(owner);
            routes.world = common.world;
            routes.facts = world_schedule_facts(common);
            routes.popularity_queue = common.popularity_queue;
        }
        return routes;
    };
    // current与next只在私有路径中别名；公开值路径保留一次Owner复制。
    const auto consume_actor =
        [&](const Owner &current, Owner &next,
            const WorldScheduleCall &call) -> std::optional<WorldScheduleDisposition> {
        if (!call.id)
            return {};
        const CharacterId actor{*call.id};
        auto routes = read_routes(current, true);
        const auto publish = [&](const WorldActorRoutesState &r) {
            if (adapter.write_current_routes)
                return adapter.write_current_routes(next, r);
            if (!adapter.write_routes(next, r))
                return false;
            auto &published = adapter.write_common(next);
            published.world = r.world;
            published.surface = r.facts.surface;
            published.map_flags = r.facts.flags;
            published.town = r.facts.town;
            published.popularity_queue = r.popularity_queue;
            return true;
        };
        const WorldActorEventConsumer consume_event =
            [&](const WorldActorRoutesState &r, int code) -> std::optional<WorldActorRoutesState> {
            if (!adapter.event || !publish(r))
                return {};
            auto consumed = adapter.event(next, code);
            if (!consumed)
                return {};
            next = std::move(*consumed);
            return read_routes(next, true);
        };
        WorldScheduleDisposition disposition{WorldScheduleDisposition::keep};
        const WorldActorPresentationConsumer present =
            [&](const WorldActorRoutesState &r, const WorldActorPresentationRequest &request)
            -> std::optional<WorldActorRoutesState> {
            if (!adapter.presentation || !publish(r))
                return {};
            auto consumed = adapter.presentation(next, request);
            if (!consumed)
                return {};
            next = std::move(*consumed);
            return read_routes(next, false);
        };
        if (call.stage == WorldScheduleStage::decision) {
            auto i = adapter.decision_from_routes
                         ? adapter.decision_from_routes(current, routes, actor)
                         : adapter.decision(current, actor);
            if (!i || !(i->actor == actor))
                return {};
            if (adapter.event)
                i->event = consume_event;
            if (adapter.presentation)
                i->presentation = present;
            if (adapter.encounter)
                i->encounter = [&](const WorldActorRoutesState &r,
                                   const EncounterCreationRequest &request)
                    -> std::optional<WorldActorRoutesState> {
                    if (!publish(r))
                        return {};
                    auto consumed = adapter.encounter(next, request);
                    if (!consumed)
                        return {};
                    next = std::move(*consumed);
                    return read_routes(next, false);
                };
            auto r = prepare_world_actor_decision_consuming(std::move(routes), *i);
            if (!r.candidate) {
                output.failure = WorldScheduleFailure{call.stage, call.id, "actor.decision",
                                                      static_cast<int>(r.error)};
                return {};
            }
            disposition = r.candidate->removed ? WorldScheduleDisposition::already_removed
                          : r.candidate->delete_requested
                              ? WorldScheduleDisposition::remove_requested
                              : WorldScheduleDisposition::keep;
            // 最后一次发布直接读取完整candidate，再整值移入独立审计。
            // 不建立只为发布而存在的第二份完整routes，也不移走审计中的state。
            if (!publish(r.candidate->state))
                return {};
            output.decisions.push_back(std::move(*r.candidate));
            return disposition;
        } else if (call.stage == WorldScheduleStage::control) {
            const WorldActorCommandProvider command =
                [&](const WorldActorRoutesState &r, CharacterId id,
                    const LegacyActorControl &op) -> std::optional<WorldActorCommandInput> {
                if (!adapter.command && !adapter.owned_command)
                    return {};
                if (adapter.owned_command && !publish(r))
                    return {};
                auto input = adapter.owned_command ? adapter.owned_command(next, r, id, op)
                                                   : adapter.command(r, id, op);
                if (input && adapter.event)
                    input->event = consume_event;
                if (input && adapter.presentation)
                    input->presentation = present;
                return input;
            };
            auto r = prepare_world_actor_control_consuming(std::move(routes), actor, command);
            if (!r.candidate) {
                output.failure = WorldScheduleFailure{call.stage, call.id, "actor.control",
                                                      static_cast<int>(r.error)};
                return {};
            }
            if (r.candidate->flow == WorldControlFlow::delete_requested)
                disposition = WorldScheduleDisposition::remove_requested;
            if (!publish(r.candidate->state))
                return {};
            output.controls.push_back(std::move(*r.candidate));
            return disposition;
        } else {
            if (!adapter.primary_expression_table || !routes.world.ai.contexts.count(actor))
                return {};
            auto r = prepare_world_random_expression(routes.random,
                                                     routes.world.ai.contexts.at(actor).effects, 17,
                                                     0, *adapter.primary_expression_table);
            if (!r.candidate) {
                output.failure = WorldScheduleFailure{call.stage, call.id,
                                                      r.random_error != WorldRandomError::none
                                                          ? "actor.expression_random"
                                                          : "actor.expression",
                                                      r.random_error != WorldRandomError::none
                                                          ? static_cast<int>(r.random_error)
                                                          : static_cast<int>(r.expression_error)};
                return {};
            }
            routes.world.ai.contexts.at(actor).effects = std::move(r.candidate->state);
        }
        if (!publish(routes))
            return {};
        return disposition;
    };
    owned.consume =
        [&](const Owner &current, const WorldScheduleCall &call,
            const CombatInfluenceCandidate &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        if (call.stage == WorldScheduleStage::actor_tail_cache) {
            if (!call.id || !call.projected_actor || !adapter.tail_cache)
                return {};
            auto cached = adapter.tail_cache(current, CharacterId{*call.id}, *call.projected_actor);
            return cached ? std::optional<OwnedWorldScheduleStep<Owner>>{{std::move(*cached)}}
                          : std::nullopt;
        }
        if (call.stage != WorldScheduleStage::decision &&
            call.stage != WorldScheduleStage::control &&
            call.stage != WorldScheduleStage::carry_expression)
            return adapter.other ? adapter.other(current, call, field) : std::nullopt;
        Owner next = current;
        const auto disposition = consume_actor(current, next, call);
        if (!disposition)
            return {};
        return OwnedWorldScheduleStep<Owner>{std::move(next), *disposition};
    };
    if (adapter.other_private)
        owned.consume_private =
            [&](Owner &current, const WorldScheduleCall &call,
                const CombatInfluenceCandidate &field) -> std::optional<WorldScheduleDisposition> {
            if (call.stage == WorldScheduleStage::actor_tail_cache) {
                if (!call.id || !call.projected_actor)
                    return {};
                if (adapter.tail_cache_private) {
                    if (!adapter.tail_cache_private(current, CharacterId{*call.id},
                                                    *call.projected_actor))
                        return {};
                    return WorldScheduleDisposition::keep;
                }
                if (!adapter.tail_cache)
                    return {};
                auto cached =
                    adapter.tail_cache(current, CharacterId{*call.id}, *call.projected_actor);
                if (!cached)
                    return {};
                current = std::move(*cached);
                return WorldScheduleDisposition::keep;
            }
            if (call.stage != WorldScheduleStage::decision &&
                call.stage != WorldScheduleStage::control &&
                call.stage != WorldScheduleStage::carry_expression)
                return adapter.other_private(current, call, field);
            return consume_actor(current, current, call);
        };
    auto schedule_input = input;
    schedule_input.publish_actor_tail = static_cast<bool>(adapter.tail_cache) ||
                                        (adapter.other_private && adapter.tail_cache_private);
    auto result = prepare_owned_world_schedule(state, schedule_input, owned);
    if (!result.state)
        return {result.error, {}, {}, {}, {}, output.failure ? output.failure : result.failure};
    output.error = WorldScheduleError::none;
    output.state = std::move(result.state);
    output.audit = std::move(result.audit);
    return output;
}
} // namespace ark::simulation::rules
