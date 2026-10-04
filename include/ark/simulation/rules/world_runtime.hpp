#pragma once

#include "ark/simulation/rules/world_actor_schedule.hpp"
#include "ark/simulation/rules/world_calendar_maintenance.hpp"
#include "ark/simulation/rules/world_calendar_tasks.hpp"
#include "ark/simulation/rules/world_facility_update.hpp"
#include "ark/simulation/rules/world_month_report.hpp"
#include "ark/simulation/rules/world_nonactor_schedule.hpp"
#include "ark/simulation/rules/world_popularity.hpp"
#include "ark/simulation/rules/world_scene.hpp"
#include "ark/simulation/rules/world_task_creation.hpp"
#include "ark/simulation/rules/world_world_entry.hpp"

#include <stdexcept>

namespace ark::simulation::rules {
// read只生成调用点值投影；write提交全部扩展与共享字段到同一私有Owner。
// 投影类型不是长期持久状态；不能把审计结果重新当作可写世界。
template <class Owner, class Projection> struct WorldRuntimeProjection {
    std::function<Projection(const Owner &)> read;
    std::function<bool(Owner &, const Projection &)> write;
    explicit operator bool() const { return read && write; }
};

template <class Owner> struct OwnedWorldRuntimeCreation {
    Owner state;
    std::optional<std::uint64_t> created;
    EncounterCreationDenial denial{EncounterCreationDenial::none};
};
template <class Owner> struct WorldRuntimeAdapter {
    WorldRuntimeProjection<Owner, WorldSceneState> scene;
    WorldRuntimeProjection<Owner, WorldScriptState> scripts;
    WorldRuntimeProjection<Owner, WorldMonthReportState> report;
    WorldRuntimeProjection<Owner, WorldPopularityState> popularity;
    WorldRuntimeProjection<Owner, WorldFacilityUpdateState> facilities;
    WorldRuntimeProjection<Owner, WorldCalendarMaintenanceState> maintenance;
    WorldRuntimeProjection<Owner, WorldCalendarTasksState> tasks;
    WorldRuntimeProjection<Owner, WorldTaskCreationState> factory;
    WorldRuntimeProjection<Owner, WorldWorldEntryState> entry;
    WorldActorScheduleAdapter<Owner> actors;
    WorldNonactorScheduleAdapter<Owner> nonactors;
    // 全部随机消费者读写这个Owner字段，不捕获真实全局生成器。
    std::function<const WorldRandomStream &(const Owner &)> read_random;
    std::function<WorldRandomStream &(Owner &)> write_random;
    std::function<std::optional<WorldMonthReportInput>(const Owner &)> report_input;
    WorldScriptCatalog catalog;
    // 真实f.a0创建消费者；V/aK/概率/20落点由已维护entry强制消费。
    std::function<std::optional<OwnedWorldRuntimeCreation<Owner>>(
        const Owner &, const EncounterCreationInput &)> create_encounter;
    // 镜头l.h只影响前置到访，不以高层JADX包围形态额外暂停整段AI。
    std::function<std::optional<Owner>(const Owner &)> before_common;
    std::function<std::optional<Owner>(const Owner &)> arrival;
    std::function<std::optional<Owner>(const Owner &, const WorldScheduleEffects &)> prefix_effects;
    // 住宅、探索crew/phase2必须在当前私有Owner同步消费，不仅保存ID。
    std::function<std::optional<Owner>(const Owner &, const WorldFacilityUpdateRequest &)> facility;
    std::function<std::optional<Owner>(const Owner &, const CalendarMaintenanceRequest &)>
        calendar_request;
    std::function<std::optional<Owner>(const Owner &)> endgame_checkpoint;
    // 资金/库存等原前置条件先于aL扫描；该入口只处理此前分支，不扫描脚本目录。
    std::function<std::optional<OwnedWorldSceneStep<Owner>>(const Owner &)> normal_conditions;
    // 保存资格、年度刷新、子周期刷新，以及框架/镜头/输入的实际适配。
    // 缺失拒绝；其余已提取领域不能通过该函数跳过。
    std::function<std::optional<Owner>(const Owner &, WorldCalendarStage)> calendar_other;
    std::function<std::optional<OwnedWorldSceneStep<Owner>>(const Owner &, const WorldSceneCall &)>
        scene_other;
};

namespace world_runtime_detail {
inline bool task_stage(WorldCalendarStage stage) {
    switch (stage) {
    case WorldCalendarStage::month_special_scripts:
    case WorldCalendarStage::month_rank_check:
    case WorldCalendarStage::subperiod_task_deadline:
    case WorldCalendarStage::subperiod_task_midpoint:
    case WorldCalendarStage::subperiod_task_generation:
    case WorldCalendarStage::subperiod_capacity_hint:
        return true;
    default:
        return false;
    }
}
inline bool external_calendar_stage(WorldCalendarStage stage) {
    return stage == WorldCalendarStage::checkpoint_before_normalize ||
           stage == WorldCalendarStage::year_refresh ||
           stage == WorldCalendarStage::subperiod_refresh;
}
} // namespace world_runtime_detail

// 真实任务工厂接在日历原调用点。普通无地点保留消费；任何后段失败丢弃整个Owner。
template <class Owner>
std::optional<Owner>
prepare_owned_world_runtime_calendar(const Owner &state, const WorldCalendarState &date,
                                     WorldCalendarStage stage,
                                     const WorldRuntimeAdapter<Owner> &adapter) {
    if (!adapter.read_random || !adapter.write_random)
        return {};
    if (world_runtime_detail::external_calendar_stage(stage))
        return adapter.calendar_other ? adapter.calendar_other(state, stage) : std::nullopt;
    Owner next = state;
    if (world_runtime_detail::task_stage(stage)) {
        if (!adapter.tasks)
            return {};
        auto tasks = adapter.tasks.read(next);
        tasks.random = adapter.read_random(next);
        const auto result = prepare_world_calendar_tasks(
            tasks, date, stage, adapter.catalog,
            [&](const WorldCalendarTasksState &current, const CalendarTaskExternalRequest &request)
                -> std::optional<CalendarTaskExternalCandidate> {
                if (!adapter.tasks.write(next, current))
                    return {};
                adapter.write_random(next) = current.random;
                std::optional<std::uint64_t> created;
                if (request.kind == CalendarTaskExternalKind::create_task) {
                    if (!adapter.factory)
                        return {};
                    auto creation = adapter.factory.read(next);
                    creation.finish = current.finish;
                    creation.random = adapter.read_random(next);
                    const auto factory = prepare_world_task_creation(creation, request.task_kind);
                    if (!factory.candidate ||
                        !adapter.factory.write(next, factory.candidate->state))
                        return {};
                    adapter.write_random(next) = factory.candidate->state.random;
                    created = factory.candidate->created_task;
                } else {
                    if (!adapter.endgame_checkpoint)
                        return {};
                    auto checkpoint = adapter.endgame_checkpoint(next);
                    if (!checkpoint)
                        return {};
                    next = std::move(*checkpoint);
                }
                auto published = adapter.tasks.read(next);
                published.random = adapter.read_random(next);
                return CalendarTaskExternalCandidate{std::move(published), created};
            });
        if (!result.candidate || !adapter.tasks.write(next, result.candidate->state))
            return {};
        adapter.write_random(next) = result.candidate->state.random;
        return next;
    }
    if (!adapter.maintenance)
        return {};
    auto maintenance = adapter.maintenance.read(next);
    maintenance.random = adapter.read_random(next);
    const auto result = prepare_world_calendar_maintenance(maintenance, date, stage);
    if (!result.candidate || !adapter.maintenance.write(next, result.candidate->state))
        return {};
    adapter.write_random(next) = result.candidate->state.random;
    // 请求必须消费后再重投影，不拿maintenance旧快照覆盖脚本产生的目录/资金/随机。
    for (const auto &request : result.candidate->pending_requests) {
        if (!adapter.calendar_request)
            return {};
        auto consumed = adapter.calendar_request(next, request);
        if (!consumed)
            return {};
        next = std::move(*consumed);
    }
    return next;
}

template <class Owner>
std::optional<OwnedWorldScheduleStep<Owner>>
prepare_owned_world_runtime_domain(const Owner &state, const WorldScheduleCall &call,
                                   const CombatInfluenceCandidate &field,
                                   const WorldRuntimeAdapter<Owner> &adapter) {
    Owner next = state;
    if (call.stage == WorldScheduleStage::prefix_effects) {
        if (!call.effects || !adapter.prefix_effects)
            return {};
        auto consumed = adapter.prefix_effects(next, *call.effects);
        return consumed ? std::optional<OwnedWorldScheduleStep<Owner>>{{std::move(*consumed)}}
                        : std::nullopt;
    }
    if (call.stage == WorldScheduleStage::arrival_front) {
        if (!adapter.arrival)
            return {};
        const auto arrived = adapter.arrival(next);
        return arrived ? std::optional<OwnedWorldScheduleStep<Owner>>{{*arrived}} : std::nullopt;
    }
    if (call.stage == WorldScheduleStage::popularity) {
        if (!call.popularity || !adapter.popularity)
            return {};
        const auto result =
            prepare_world_popularity(adapter.catalog, adapter.popularity.read(next),
                                     (*call.popularity)[0], (*call.popularity)[1] == 1);
        if (!result.candidate || !adapter.popularity.write(next, result.candidate->state))
            return {};
        return OwnedWorldScheduleStep<Owner>{std::move(next)};
    }
    if (call.stage == WorldScheduleStage::facility) {
        if (!call.id || !adapter.facilities || !adapter.read_random || !adapter.write_random)
            return {};
        auto facilities = adapter.facilities.read(next);
        facilities.random = adapter.read_random(next);
        const auto result = prepare_world_facility_update(
            facilities, *call.id, adapter.catalog,
            [&](const WorldFacilityUpdateState &current, const WorldFacilityUpdateRequest &request)
                -> std::optional<WorldFacilityUpdateState> {
                if (!adapter.facilities.write(next, current) || !adapter.facility)
                    return {};
                adapter.write_random(next) = current.random;
                auto consumed = adapter.facility(next, request);
                if (!consumed)
                    return {};
                next = std::move(*consumed);
                auto published = adapter.facilities.read(next);
                published.random = adapter.read_random(next);
                return published;
            });
        if (!result.candidate || !adapter.facilities.write(next, result.candidate->state))
            return {};
        adapter.write_random(next) = result.candidate->state.random;
        return OwnedWorldScheduleStep<Owner>{std::move(next)};
    }
    auto nonactors = adapter.nonactors;
    nonactors.other = {}; // 到访/人气/设施已在上方实际路由，禁止互相递归兜底。
    if (!adapter.read_random || !adapter.write_random || !nonactors.read_routes ||
        !nonactors.write_routes)
        return {};
    const auto read_routes = nonactors.read_routes;
    const auto write_routes = nonactors.write_routes;
    nonactors.read_routes = [&](const Owner &owner) {
        auto routes = read_routes(owner);
        routes.random = adapter.read_random(owner);
        return routes;
    };
    nonactors.write_routes = [&](Owner &owner, const WorldNonactorScheduleState &routes) {
        if (!write_routes(owner, routes))
            return false;
        adapter.write_random(owner) = routes.random;
        return true;
    };
    return prepare_owned_world_nonactor_stage(state, call, field, nonactors);
}

template <class Owner> struct WorldRuntimeResult {
    WorldSceneError error{WorldSceneError::none};
    WorldScheduleError world_error{WorldScheduleError::none};
    std::optional<Owner> state;
    std::optional<WorldSceneCandidate> scene;
    std::vector<WorldScheduleCandidate> worlds; // 审计，不是可写副本。
};

// MainScene的共同组合入口。只推进已获框架资格的场景；保存的1/2轮数不因脚本推页重算。
// 最后一轮/最后一个日历域失败也不向调用者暴露此前AI、随机、金钱或页栈的部分提交。
template <class Owner>
WorldRuntimeResult<Owner> prepare_owned_world_runtime(const Owner &state,
                                                      const WorldSceneInput &input,
                                                      const WorldRuntimeAdapter<Owner> &adapter) {
    WorldRuntimeResult<Owner> output;
    if (!adapter.scene || !adapter.scripts) {
        output.error = WorldSceneError::missing_consumer;
        return output;
    }
    OwnedWorldSceneAdapter<Owner> scene;
    scene.read = adapter.scene.read;
    scene.write = [&](Owner &owner, const WorldSceneState &value) {
        if (!adapter.scene.write(owner, value))
            throw std::runtime_error("共同场景投影写回失败");
    };
    scene.consume = [&](const Owner &current,
                        const WorldSceneCall &call) -> std::optional<OwnedWorldSceneStep<Owner>> {
        Owner next = current;
        if (call.stage == WorldSceneStage::normal_condition_scripts ||
            call.stage == WorldSceneStage::normal_delayed_scripts) {
            if (call.stage == WorldSceneStage::normal_condition_scripts) {
                if (!adapter.normal_conditions)
                    return {};
                auto conditions = adapter.normal_conditions(next);
                if (!conditions)
                    return {};
                if (conditions->disposition != WorldSceneDisposition::continue_round)
                    return conditions;
                next = std::move(conditions->state);
            }
            auto scripts = adapter.scripts.read(next);
            WorldScriptResult result;
            if (call.stage == WorldSceneStage::normal_condition_scripts) {
                const auto date = adapter.scene.read(next).calendar;
                result = prepare_world_script_automatic(
                    adapter.catalog, scripts, {date.year, date.month, date.subperiod, true});
            } else
                result = prepare_world_script_continuations(adapter.catalog, scripts, true);
            if (!result.candidate || !adapter.scripts.write(next, result.candidate->state))
                return {};
            const auto disposition = result.candidate->entered_program
                                         ? WorldSceneDisposition::skip_round
                                         : WorldSceneDisposition::continue_round;
            return OwnedWorldSceneStep<Owner>{std::move(next), disposition};
        }
        if (call.stage == WorldSceneStage::normal_world ||
            call.stage == WorldSceneStage::focus_world) {
            if (!adapter.entry || !adapter.read_random || !adapter.write_random ||
                !adapter.before_common || !adapter.report ||
                !adapter.report_input)
                return {};
            auto initial_entry = adapter.entry.read(next);
            initial_entry.random = adapter.read_random(next);
            const auto entry = prepare_world_world_entry(
                initial_entry, adapter.scene.read(next).calendar, adapter.catalog,
                [&](const WorldWorldEntryState &current, const EncounterCreationInput &creation)
                    -> std::optional<WorldWorldEntryCreation> {
                    if (!adapter.entry.write(next, current) || !adapter.create_encounter)
                        return {};
                    adapter.write_random(next) = current.random;
                    const auto created = adapter.create_encounter(next, creation);
                    if (!created)
                        return {};
                    next = created->state;
                    auto published = adapter.entry.read(next);
                    published.random = adapter.read_random(next);
                    return WorldWorldEntryCreation{std::move(published), created->created,
                                                    created->denial};
                });
            if (!entry.candidate || !adapter.entry.write(next, entry.candidate->state))
                return {};
            adapter.write_random(next) = entry.candidate->state.random;
            const auto report_input = adapter.report_input(next);
            if (!report_input)
                return {};
            const auto report =
                prepare_world_month_report(adapter.report.read(next), *report_input);
            if (!report.candidate || !adapter.report.write(next, report.candidate->state))
                return {};
            auto prefix = adapter.before_common(next);
            if (!prefix)
                return {};
            next = std::move(*prefix);
            auto actors = adapter.actors;
            if (!adapter.read_random || !adapter.write_random || !actors.read_routes ||
                !actors.write_routes)
                return {};
            const auto read_routes = actors.read_routes;
            const auto write_routes = actors.write_routes;
            actors.read_routes = [&](const Owner &owner) {
                auto routes = read_routes(owner);
                routes.random = adapter.read_random(owner);
                return routes;
            };
            actors.write_routes = [&](Owner &owner, const WorldActorRoutesState &routes) {
                if (!write_routes(owner, routes))
                    return false;
                adapter.write_random(owner) = routes.random;
                return true;
            };
            actors.event = [&](const Owner &owner, int code) -> std::optional<Owner> {
                auto consumed = owner;
                const auto script = prepare_world_script(
                    adapter.catalog, adapter.scripts.read(consumed), {code, {}, {}});
                if (!script.candidate || !adapter.scripts.write(consumed, script.candidate->state))
                    return {};
                return consumed;
            };
            actors.other = [&](const Owner &owner, const WorldScheduleCall &request,
                               const CombatInfluenceCandidate &field) {
                return prepare_owned_world_runtime_domain(owner, request, field, adapter);
            };
            const auto world = prepare_world_actor_schedule(next, {true}, actors);
            if (!world.state) {
                output.world_error = world.error;
                return {};
            }
            if (world.audit)
                output.worlds.push_back(*world.audit);
            return OwnedWorldSceneStep<Owner>{*world.state};
        }
        if (call.stage == WorldSceneStage::calendar_call) {
            if (!call.calendar_stage)
                return {};
            const auto date = adapter.scene.read(next).calendar;
            const auto calendar =
                prepare_owned_world_runtime_calendar(next, date, *call.calendar_stage, adapter);
            return calendar ? std::optional<OwnedWorldSceneStep<Owner>>{{*calendar}} : std::nullopt;
        }
        return adapter.scene_other ? adapter.scene_other(next, call) : std::nullopt;
    };
    try {
        const auto result = prepare_owned_world_scene(state, input, scene);
        output.error = result.error;
        if (!result.state) {
            output.worlds.clear();
            return output;
        }
        output.state = result.state;
        output.scene = result.audit;
        return output;
    } catch (...) {
        output.error = WorldSceneError::consumer_failed;
        output.worlds.clear();
        return output;
    }
}
} // namespace ark::simulation::rules
