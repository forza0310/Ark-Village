#include "dungeon_village_reference/world_nonactor_schedule.hpp"

#include <algorithm>

namespace dungeon_village_reference {
namespace {
bool apply_globals(AiRewardState &state, const EncounterExternalWriteback &fields) {
    if (fields.pending_completion < 0)
        return false;
    state.battle.events.insert(fields.events.begin(), fields.events.end());
    state.pending_completion = fields.pending_completion;
    state.task_active = fields.task_active;
    state.task_completed = fields.task_completed;
    state.feature16 = fields.feature16;
    state.external_actor_roots = fields.external_actor_roots;
    state.external_encounter_roots = fields.external_encounter_roots;
    if (fields.monster_availability)
        for (const auto &entry : *fields.monster_availability) {
            const auto current = state.monster_growth.find(entry.first);
            if (current == state.monster_growth.end() || entry.second[0] < 0 ||
                entry.second[0] > 1 || entry.second[1] < 0 || entry.second[1] > 1)
                return false;
            current->second.status = entry.second[0];
            current->second.newly_unlocked = entry.second[1] != 0;
        }
    return true;
}
bool external_object(ObjectCommitRequestKind kind) {
    return kind == ObjectCommitRequestKind::ground_effect ||
           kind == ObjectCommitRequestKind::notice || kind == ObjectCommitRequestKind::event;
}
bool external_hit(HitRequestKind kind) {
    return kind == HitRequestKind::face_attacker || kind == HitRequestKind::attack_sound;
}
} // namespace
WorldNonactorResult prepare_world_nonactor_stage(const WorldNonactorScheduleState &state,
                                                 const WorldNonactorStageInput &input,
                                                 const CombatInfluenceCandidate &field,
                                                 const WorldNonactorConsumer &consumer) {
    const auto fail = [](WorldNonactorError error) { return WorldNonactorResult{error, {}}; };
    if (!valid_world_schedule_owner(state.common))
        return fail(WorldNonactorError::invalid_owner);
    WorldNonactorCandidate candidate{state, WorldScheduleDisposition::keep, {}};
    auto &routes = candidate.state;
    WorldNonactorError error{WorldNonactorError::none};
    const auto draw = [&](int bound) -> std::optional<int> {
        const auto result = routes.random.draw(bound);
        if (result.error != WorldRandomError::none) {
            error = WorldNonactorError::random_failed;
            return {};
        }
        return result.ticket;
    };
    const auto consume = [&](const WorldNonactorRequest &request) -> bool {
        if (!consumer) {
            error = WorldNonactorError::missing_consumer;
            return false;
        }
        const auto fields = consumer(routes, request);
        if (!fields || !apply_globals(routes.common.world.ai, fields->globals)) {
            error = WorldNonactorError::external_failed;
            return false;
        }
        if (fields->popularity_queue)
            routes.common.popularity_queue = *fields->popularity_queue;
        if (fields->globals.monster_availability) {
            if (!request.encounter ||
                request.encounter->kind != EncounterRequestKind::mark_task_complete) {
                error = WorldNonactorError::external_failed;
                return false;
            }
        }
        if (request.hit && request.hit->kind == HitRequestKind::face_attacker) {
            if (!request.target || !fields->target_facing || *fields->target_facing < 0 ||
                *fields->target_facing > 3) {
                error = WorldNonactorError::external_failed;
                return false;
            }
            auto &ai = routes.common.world.ai;
            const auto live = ai.battle.actors.find(*request.target);
            const auto retired = ai.retired_actors.find(*request.target);
            if (live == ai.battle.actors.end() && retired == ai.retired_actors.end()) {
                error = WorldNonactorError::external_failed;
                return false;
            }
            (live != ai.battle.actors.end() ? live->second : retired->second).control.facing =
                *fields->target_facing;
        } else if (fields->target_facing) {
            error = WorldNonactorError::external_failed;
            return false;
        }
        const bool target_visual = request.kind == WorldNonactorRequestKind::projectile_contact ||
            (request.kind == WorldNonactorRequestKind::projectile_visual && request.visual >= 4 &&
             request.visual <= 9);
        if (target_visual) {
            if (!request.target || !fields->target_effects ||
                !valid_actor_effect_state(*fields->target_effects) ||
                !routes.common.world.ai.contexts.count(*request.target)) {
                error = WorldNonactorError::external_failed;
                return false;
            }
            routes.common.world.ai.contexts.at(*request.target).effects = *fields->target_effects;
        } else if (fields->target_effects) {
            error = WorldNonactorError::external_failed;
            return false;
        }
        candidate.consumed.push_back(request);
        return true;
    };
    const auto failure = [&] {
        return fail(error != WorldNonactorError::none ? error : WorldNonactorError::domain_failed);
    };
    const auto &call = input.call;
    if (call.stage == WorldScheduleStage::finalize) {
        if (call.id)
            return fail(WorldNonactorError::missing_input);
        const auto overlap = prepare_world_schedule_overlap(routes.common, {}, draw);
        if (!overlap)
            return failure();
        routes.common = *overlap;
    } else if (call.stage == WorldScheduleStage::projectile) {
        if (!call.id || !input.projectile || input.projectile->projectile != *call.id)
            return fail(WorldNonactorError::missing_input);
        auto projectile = *input.projectile;
        projectile.draw = draw;
        projectile.expression = [&](CharacterId, const ActorEffectState &effects, int expression,
                                    int delay) -> std::optional<ActorEffectState> {
            if (!input.primary_expression_table)
                return {};
            const auto result = prepare_world_random_expression(
                routes.random, effects, expression, delay, *input.primary_expression_table);
            if (!result.candidate) {
                if (result.random_error != WorldRandomError::none)
                    error = WorldNonactorError::random_failed;
                return {};
            }
            return result.candidate->state;
        };
        projectile.event = [&](const AiRewardState &ai,
                               int event) -> std::optional<WorldCombatExternalWriteback> {
            routes.common.world.ai = ai;
            const HitRequest request{
                event == 131 ? HitRequestKind::event131 : HitRequestKind::event217, event};
            if (!consume({WorldNonactorRequestKind::projectile_hit,
                          *call.id,
                          {},
                          {},
                          0,
                          request,
                          {},
                          {}}))
                return {};
            return WorldCombatExternalWriteback{
                encounter_external_writeback(routes.common.world.ai),
                routes.common.popularity_queue};
        };
        if (projectile.drop_selection)
            projectile.drop_selection->draw = draw;
        const auto old = routes.common.world.ai.projectiles.find(*call.id);
        if (old == routes.common.world.ai.projectiles.end())
            return fail(WorldNonactorError::invalid_owner);
        const auto caster = old->second.caster;
        const auto result = prepare_world_projectile(routes.common.world.ai, projectile);
        if (!result.candidate)
            return failure();
        routes.common.world.ai = result.candidate->state;
        if (result.candidate->popularity_queue)
            routes.common.popularity_queue = *result.candidate->popularity_queue;
        for (const auto id : result.candidate->spawned_objects)
            routes.common.world.object_order.push_back(id);
        if (result.candidate->step.remove)
            candidate.disposition = WorldScheduleDisposition::already_removed;
        const auto target = result.candidate->step.damage_target;
        if (result.candidate->hit)
            for (const auto &hit : result.candidate->hit->requests)
                if (external_hit(hit.kind) && !consume({WorldNonactorRequestKind::projectile_hit,
                                                        *call.id,
                                                        caster,
                                                        target,
                                                        0,
                                                        hit,
                                                        {},
                                                        {}}))
                    return failure();
        const auto &step = result.candidate->step;
        if (step.contact_effect) {
            WorldNonactorRequest request{WorldNonactorRequestKind::projectile_contact,
                                         *call.id, caster, target, 0, {}, {}, {}};
            request.source_position = step.state.position;
            if (!consume(request))
                return failure();
        }
        for (const int effect : {step.ground_effect22 ? 22 : 0, step.visual_effect})
            if (effect) {
                // c/j.d：法术对实际碰撞者先画4..9，再给同一对象创建延迟伤害。
                // 此轮damage_target尚空；不能借最初瞄准对象或提前提交延迟HP。
                const auto visual_target = effect >= 4 && effect <= 9 && step.spawned
                    ? std::optional<CharacterId>{step.spawned->original_target} : target;
                WorldNonactorRequest request{WorldNonactorRequestKind::projectile_visual,
                    *call.id, caster, visual_target, effect, {}, {}, {}};
                request.source_position = step.state.position;
                if (!consume(request))
                    return failure();
            }
    } else if (call.stage == WorldScheduleStage::object) {
        if (!call.id || !routes.common.world.ai.battle.objects.count(*call.id))
            return fail(WorldNonactorError::missing_input);
        auto projected = routes.objects;
        projected.objects = routes.common.world.ai.battle.objects;
        projected.events = routes.common.world.ai.battle.events;
        const auto &object = projected.objects.at(*call.id);
        const auto result = prepare_object_update(
            projected, {*call.id}, inside_town(object.cached_cell, routes.common.town));
        if (!result.candidate)
            return failure();
        routes.objects = result.candidate->state;
        routes.common.world.ai.battle.objects = routes.objects.objects;
        routes.common.world.ai.battle.events = routes.objects.events;
        if (result.candidate->remove) {
            auto &order = routes.common.world.object_order;
            const auto found = std::find(order.begin(), order.end(), *call.id);
            if (found == order.end())
                return fail(WorldNonactorError::invalid_owner);
            order.erase(found);
            candidate.disposition = WorldScheduleDisposition::already_removed;
        }
        for (const auto &request : result.candidate->requests)
            if (external_object(request.kind)) {
                WorldNonactorRequest routed{WorldNonactorRequestKind::object, *call.id, {}, {}, 0,
                                            {}, request, {}};
                routed.source_position = object.position;
                if (!consume(routed))
                    return failure();
            }
    } else if (call.stage == WorldScheduleStage::encounter) {
        if (!call.id || !input.encounter || input.encounter->encounter != *call.id)
            return fail(WorldNonactorError::missing_input);
        auto encounter = *input.encounter;
        encounter.draw = draw;
        encounter.random_request =
            [&](const AiRewardState &ai,
                const EncounterRequest &request) -> std::optional<AiRewardState> {
            if (!input.primary_expression_table || !request.actor ||
                !ai.contexts.count(*request.actor))
                return {};
            auto next = ai;
            const auto result = prepare_world_random_expression(
                routes.random, next.contexts.at(*request.actor).effects, request.parameter,
                request.delay, *input.primary_expression_table);
            if (!result.candidate) {
                if (result.random_error != WorldRandomError::none)
                    error = WorldNonactorError::random_failed;
                return {};
            }
            next.contexts.at(*request.actor).effects = result.candidate->state;
            return next;
        };
        encounter.external_request =
            [&](const AiRewardState &ai,
                const EncounterRequest &request) -> std::optional<EncounterExternalWriteback> {
            routes.common.world.ai = ai;
            if (request.kind == EncounterRequestKind::refresh_map) {
                const auto refreshed =
                    prepare_world_event_map(ai, world_schedule_facts(routes.common));
                if (!refreshed.facts)
                    return {};
                routes.common.world.map = refreshed.facts->map;
                routes.common.surface = refreshed.facts->surface;
                routes.common.map_flags = refreshed.facts->flags;
                routes.common.town = refreshed.facts->town;
                return encounter_external_writeback(ai);
            }
            if (!consume(
                    {WorldNonactorRequestKind::encounter, *call.id, {}, {}, 0, {}, {}, request}))
                return {};
            auto fields = encounter_external_writeback(routes.common.world.ai);
            if (request.kind == EncounterRequestKind::mark_task_complete) {
                fields.monster_availability.emplace();
                for (const auto &entry : routes.common.world.ai.monster_growth) {
                    const auto old = ai.monster_growth.find(entry.first);
                    if (old == ai.monster_growth.end())
                        return {};
                    if (old->second.status != entry.second.status ||
                        old->second.newly_unlocked != entry.second.newly_unlocked)
                        fields.monster_availability->emplace(entry.first,
                            std::array<int, 2>{entry.second.status, entry.second.newly_unlocked ? 1 : 0});
                }
            }
            return fields;
        };
        const auto result = prepare_world_encounter_update(
            routes.common.world.ai, world_schedule_facts(routes.common), encounter, field);
        if (!result.candidate)
            return failure();
        routes.common.world.ai = result.candidate->state;
        routes.common.world.map = result.candidate->facts.map;
        routes.common.surface = result.candidate->facts.surface;
        routes.common.map_flags = result.candidate->facts.flags;
        routes.common.town = result.candidate->facts.town;
        for (const auto id : routes.common.world.ai.monster_order)
            if (!routes.common.world.actors.count(id)) {
                RescueActorContext initial;
                initial.destination = Position{}; // Character新O数组全零；不是伪造出生地目标。
                routes.common.world.actors.emplace(id, initial);
            }
        if (result.candidate->removed)
            candidate.disposition = WorldScheduleDisposition::already_removed;
    } else {
        return fail(WorldNonactorError::unsupported_stage);
    }
    // 丢弃重复objects/events投影，只返回唯一common中的真实对象；扩展只保留目录与店提示。
    routes.objects.objects.clear();
    routes.objects.events.clear();
    if (!valid_world_schedule_owner(routes.common))
        return fail(WorldNonactorError::invalid_owner);
    return {WorldNonactorError::none, std::move(candidate)};
}
} // namespace dungeon_village_reference
