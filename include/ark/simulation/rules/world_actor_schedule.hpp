#pragma once

#include "ark/simulation/rules/world_actor_routes.hpp"
#include "ark/simulation/rules/world_schedule.hpp"

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
};
template <class Owner> struct WorldActorScheduleResult {
    WorldScheduleError error{WorldScheduleError::none};
    std::optional<Owner> state;
    std::optional<WorldScheduleCandidate> audit;
    std::vector<WorldActorDecisionCandidate> decisions;
    std::vector<WorldActorControlCandidate> controls;
};
// 共同c前段→全状态路由；共同d/成长→携物表情→全码v→原d尾部/删除点。
// 审计候选只保存读取时点，不是可写世界，不能把它们长期作为第二个Owner。
template <class Owner>
WorldActorScheduleResult<Owner>
prepare_world_actor_schedule(const Owner &state, const WorldScheduleInput &input,
                             const WorldActorScheduleAdapter<Owner> &adapter) {
    if (!adapter.read_common || !adapter.write_common || !adapter.read_routes ||
        !adapter.write_routes || (input.admitted && (!adapter.decision || !adapter.other)))
        return {WorldScheduleError::missing_consumer, {}, {}, {}, {}};
    WorldActorScheduleResult<Owner> output;
    OwnedWorldScheduleAdapter<Owner> owned;
    owned.read = adapter.read_common;
    owned.write = adapter.write_common;
    owned.projected_facing = adapter.projected_facing;
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
            return adapter.other(current, call, field);
        if (!call.id)
            return {};
        const CharacterId actor{*call.id};
        Owner next = current;
        auto routes = adapter.read_routes(current);
        const auto &common = adapter.read_common(current);
        routes.world = common.world;
        routes.facts = world_schedule_facts(common);
        routes.popularity_queue = common.popularity_queue;
        const auto publish = [&](const WorldActorRoutesState &r) {
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
            auto result = adapter.read_routes(next);
            const auto &latest = adapter.read_common(next);
            result.world = latest.world;
            result.facts = world_schedule_facts(latest);
            result.popularity_queue = latest.popularity_queue;
            return result;
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
            return adapter.read_routes(next);
        };
        if (call.stage == WorldScheduleStage::decision) {
            auto i = adapter.decision(current, actor);
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
                    return adapter.read_routes(next);
                };
            const auto r = prepare_world_actor_decision(routes, *i);
            if (!r.candidate)
                return {};
            routes = r.candidate->state;
            disposition = r.candidate->removed ? WorldScheduleDisposition::already_removed
                          : r.candidate->delete_requested
                              ? WorldScheduleDisposition::remove_requested
                              : WorldScheduleDisposition::keep;
            output.decisions.push_back(*r.candidate);
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
            const auto r = prepare_world_actor_control(routes, actor, command);
            if (!r.candidate)
                return {};
            routes = r.candidate->state;
            if (r.candidate->flow == WorldControlFlow::delete_requested)
                disposition = WorldScheduleDisposition::remove_requested;
            output.controls.push_back(*r.candidate);
        } else {
            if (!adapter.primary_expression_table || !routes.world.ai.contexts.count(actor))
                return {};
            const auto r = prepare_world_random_expression(
                routes.random, routes.world.ai.contexts.at(actor).effects, 17, 0,
                *adapter.primary_expression_table);
            if (!r.candidate)
                return {};
            routes.world.ai.contexts.at(actor).effects = r.candidate->state;
        }
        if (!publish(routes))
            return {};
        return OwnedWorldScheduleStep<Owner>{std::move(next), disposition};
    };
    auto schedule_input = input;
    schedule_input.publish_actor_tail = static_cast<bool>(adapter.tail_cache);
    auto result = prepare_owned_world_schedule(state, schedule_input, owned);
    if (!result.state)
        return {result.error, {}, {}, {}, {}};
    output.error = WorldScheduleError::none;
    output.state = std::move(result.state);
    output.audit = std::move(result.audit);
    return output;
}
} // namespace ark::simulation::rules
