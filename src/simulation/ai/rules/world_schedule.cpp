#include "ark/simulation/ai/rules/world_schedule.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation::rules {
namespace {
AiRosters rosters(const WorldScheduleState &s) {
    AiRosters r;
    for (const auto id : s.world.ai.human_order)
        r[0].push_back(id.value);
    for (const auto id : s.world.ai.monster_order)
        r[1].push_back(id.value);
    r[2] = s.world.ai.projectile_order;
    r[3] = s.world.object_order;
    r[4] = s.world.ai.encounter_order;
    r[5] = s.facility_order;
    return r;
}
template <class Map> bool exact_order(const std::vector<std::uint64_t> &r, const Map &m) {
    std::set<std::uint64_t> seen;
    for (const auto id : r)
        if (!seen.insert(id).second || !m.count(id))
            return false;
    return seen.size() == m.size();
}
void refresh_roots(WorldScheduleState &s) {
    // 所有设施占用引用都是Java根，包括重复引用和已退出运行名单的人物。
    // 调用者显式登记的任务/UI根不能被设施根覆盖。
    s.world.ai.facility_actor_roots.clear();
    for (const auto &[id, facility] : s.world.facilities) {
        (void)id;
        for (const auto actor : facility.occupants)
            s.world.ai.facility_actor_roots.push_back(actor);
    }
}
bool valid_phase_mutation(const AiRosters &before, const AiRosters &after, std::size_t list,
                          std::uint64_t current, bool remove, std::vector<AiRosterAppend> &append) {
    for (std::size_t n = 0; n < before.size(); ++n) {
        auto expected = before[n];
        if (remove && n == list)
            expected.erase(std::find(expected.begin(), expected.end(), current));
        if (after[n].size() < expected.size() ||
            !std::equal(expected.begin(), expected.end(), after[n].begin()))
            return false;
        std::set<std::uint64_t> old(before[n].begin(), before[n].end());
        for (std::size_t at = expected.size(); at < after[n].size(); ++at) {
            if (old.count(after[n][at]))
                return false; // 不能在同次删除后以相同稳定身份重新入名单。
            append.push_back({static_cast<AiRosterKind>(n), after[n][at]});
        }
    }
    return true;
}
} // namespace
WorldMapFacts world_schedule_facts(const WorldScheduleState &s) {
    return {s.world.map, s.surface, s.map_flags, s.town};
}
std::optional<WorldScheduleState>
prepare_world_schedule_overlap(const WorldScheduleState &s, const std::vector<int> &tickets,
                               const std::function<std::optional<int>(int)> &draw) {
    if (!valid_world_schedule_owner(s))
        return {};
    WorldOverlapInput input;
    input.boundary_y = s.town.top; // 翻转后的h.l[n.o][1][1]为较小Y，不是[0][1]。
    input.direction_tickets = tickets;
    input.draw = draw;
    for (const auto kind : {ActorKind::human, ActorKind::monster}) {
        auto &list = kind == ActorKind::human ? input.humans : input.monsters;
        for (const auto id :
             kind == ActorKind::human ? s.world.ai.human_order : s.world.ai.monster_order) {
            const auto &a = s.world.ai.battle.actors.at(id);
            const auto &ctx = s.world.ai.contexts.at(id);
            list.push_back({id,
                            a.control.state,
                            ctx.cell,
                            ctx.inside_town,
                            ctx.move_area,
                            a.body,
                            {a.position.x, a.position.z},
                            {a.decision_start.x, a.decision_start.z}});
        }
    }
    const auto overlap = prepare_world_overlap(input);
    if (!overlap.candidate)
        return {};
    auto next = s;
    for (const auto *list : {&overlap.candidate->humans, &overlap.candidate->monsters})
        for (const auto &a : *list) {
            auto &position = next.world.ai.battle.actors.at(a.id).position;
            position.x = a.position.x;
            position.z = a.position.z;
        }
    return next;
}
bool valid_world_schedule_owner(const WorldScheduleState &s) {
    if (!valid_world_map_facts(world_schedule_facts(s)) || s.updates < 0 ||
        s.updates >= std::numeric_limits<int>::max())
        return false;
    const auto r = rosters(s);
    std::size_t total{};
    for (const auto &list : r) {
        if (list.size() > 1000000 - total)
            return false;
        total += list.size();
    }
    std::set<CharacterId> actors;
    for (std::size_t kind = 0; kind < 2; ++kind)
        for (const auto id : r[kind]) {
            const CharacterId actor{id};
            const auto found = s.world.ai.battle.actors.find(actor);
            if (!id || !actors.insert(actor).second || found == s.world.ai.battle.actors.end() ||
                !(found->second.id == actor) ||
                found->second.kind != (kind == 0 ? ActorKind::human : ActorKind::monster) ||
                !s.world.ai.contexts.count(actor) || !s.world.actors.count(actor) ||
                s.world.ai.retired_actors.count(actor))
                return false;
            const auto &path = s.world.actors.at(actor);
            if (path.journey && path.unbound_route)
                return false;
        }
    if (actors.size() != s.world.ai.battle.actors.size() ||
        !exact_order(r[2], s.world.ai.projectiles) ||
        !exact_order(r[3], s.world.ai.battle.objects) ||
        !exact_order(r[4], s.world.ai.encounters) || !exact_order(r[5], s.world.facilities))
        return false;
    for (const auto &[id, f] : s.world.facilities) {
        if (f.placement.instance_id.value != id)
            return false;
        for (const auto occupant : f.occupants)
            if (!s.world.ai.battle.actors.count(occupant) &&
                !s.world.ai.retired_actors.count(occupant))
                return false;
    }
    for (const auto &hint : s.hints)
        if (hint.size() < 2 || hint[1] < 0 || hint[1] == std::numeric_limits<int>::max())
            return false;
    for (const auto &note : s.floating_notes)
        if (note.empty() || note[0] < 0 || note[0] == std::numeric_limits<int>::max())
            return false;
    for (const auto &p : s.popularity_queue)
        if (p[0] == std::numeric_limits<int>::min())
            return false;
    return true;
}
WorldScheduleResult prepare_world_schedule(const WorldScheduleState &s,
                                           const WorldScheduleInput &input,
                                           const WorldScheduleConsumer &consumer) {
    const auto fail = [](WorldScheduleError error) -> WorldScheduleResult { return {error, {}}; };
    if (!valid_world_schedule_owner(s) || !input.dispatch_limit || input.dispatch_limit > 1000000)
        return fail(WorldScheduleError::invalid_owner);
    WorldScheduleCandidate c;
    c.state = s;
    if (!input.admitted)
        return {WorldScheduleError::none, c};
    if (!consumer)
        return fail(WorldScheduleError::missing_consumer);
    const auto influence = prepare_world_influence(s.world.ai, world_schedule_facts(s));
    if (!influence.candidate)
        return fail(WorldScheduleError::common_segment_failed);
    c.start_field = influence.candidate;
    // 根是当前私有世界的派生投影，不能每次追加而永久保留已经释放的设施根。
    const auto roots = [&] { refresh_roots(c.state); };
    WorldScheduleError error{WorldScheduleError::none};
    const auto invoke = [&](WorldScheduleCall call) -> std::optional<WorldScheduleDisposition> {
        if (c.calls.size() >= input.dispatch_limit) {
            error = WorldScheduleError::dispatch_limit;
            return {};
        }
        roots();
        c.calls.push_back(call);
        const auto before = rosters(c.state);
        auto step = consumer(c.state, call, *c.start_field);
        if (!step) {
            error = WorldScheduleError::consumer_failed;
            return {};
        }
        c.state = std::move(step->state);
        roots();
        if (!valid_world_schedule_owner(c.state)) {
            error = WorldScheduleError::invalid_owner;
            return {};
        }
        const auto after = rosters(c.state);
        if ((call.stage == WorldScheduleStage::popularity ||
             call.stage == WorldScheduleStage::carry_expression ||
             call.stage == WorldScheduleStage::finalize) &&
            before != after) {
            error = WorldScheduleError::invalid_mutation;
            return {};
        }
        if (call.stage == WorldScheduleStage::arrival_front) {
            // 原到访只追加人物，可clearAll(bo/bn)，不创建其他种类或重排旧名单。
            const bool human_prefix =
                after[0].size() >= before[0].size() &&
                std::equal(before[0].begin(), before[0].end(), after[0].begin());
            const bool unchanged =
                before[1] == after[1] && before[3] == after[3] && before[5] == after[5];
            const bool clearable = (after[2].empty() || before[2] == after[2]) &&
                                   (after[4].empty() || before[4] == after[4]);
            if (!human_prefix || !unchanged || !clearable) {
                error = WorldScheduleError::invalid_mutation;
                return {};
            }
        }
        return step->disposition;
    };
    auto disposition = invoke({WorldScheduleStage::arrival_front, {}, {}});
    if (!disposition || *disposition != WorldScheduleDisposition::keep)
        return fail(disposition ? WorldScheduleError::invalid_mutation : error);
    if (!c.state.hints.empty()) {
        auto &first = c.state.hints.front();
        ++first[1];
        if (first[1] >= (c.state.hints.size() >= 2 ? 20 : 60))
            c.state.hints.erase(c.state.hints.begin());
    }
    c.state.updates = static_cast<int>((static_cast<std::int64_t>(c.state.updates) + 1) %
                                       std::numeric_limits<int>::max());
    for (std::size_t n = c.state.floating_notes.size(); n-- > 0;)
        if (++c.state.floating_notes[n][0] >= 8)
            c.state.floating_notes.erase(c.state.floating_notes.begin() +
                                         static_cast<std::ptrdiff_t>(n));
    for (std::size_t n = c.state.popularity_queue.size(); n-- > 0;) {
        auto &p = c.state.popularity_queue[n];
        --p[0];
        if (p[0] > 0)
            continue;
        const auto queue = c.state.popularity_queue;
        disposition = invoke({WorldScheduleStage::popularity, {}, {{p[1], p[2] == 1 ? 1 : 0}}});
        if (!disposition)
            return fail(error);
        if (*disposition != WorldScheduleDisposition::keep || c.state.popularity_queue != queue)
            return fail(WorldScheduleError::invalid_mutation);
        c.state.popularity_queue.erase(c.state.popularity_queue.begin() +
                                       static_cast<std::ptrdiff_t>(n));
    }
    c.state.rescue_available = false;
    for (const auto id : c.state.facility_order) {
        const auto &f = c.state.world.facilities.at(id);
        if (f.status == 1 && f.category == 2) {
            c.state.rescue_available = true;
            break;
        }
    }
    const auto scheduled = prepare_ai_schedule(
        {rosters(c.state), true, input.dispatch_limit},
        [&](const AiScheduleVisit &visit, const AiRosters &current) -> AiScheduleResponse {
            const auto reject = [&](WorldScheduleError e) -> AiScheduleResponse {
                error = e;
                return {false, false, false, {}};
            };
            if (current != rosters(c.state))
                return reject(WorldScheduleError::invalid_mutation);
            const bool decision = visit.phase == AiSchedulePhase::human_decision ||
                                  visit.phase == AiSchedulePhase::monster_decision;
            const bool execution = visit.phase == AiSchedulePhase::human_execution ||
                                   visit.phase == AiSchedulePhase::monster_execution;
            const std::size_t list = decision || execution
                                         ? (visit.phase == AiSchedulePhase::human_decision ||
                                                    visit.phase == AiSchedulePhase::human_execution
                                                ? 0
                                                : 1)
                                     : visit.phase == AiSchedulePhase::projectile ? 2
                                     : visit.phase == AiSchedulePhase::object     ? 3
                                     : visit.phase == AiSchedulePhase::encounter  ? 4
                                                                                  : 5;
            bool remove{};
            if (decision) {
                const CharacterId actor{*visit.id};
                const auto prefix = prepare_world_perception_prefix(
                    c.state.world.ai, actor, world_schedule_facts(c.state),
                    c.state.world.actors.at(actor).monster_mode);
                if (!prefix.candidate)
                    return reject(WorldScheduleError::common_segment_failed);
                c.state.world.ai = prefix.candidate->state;
                const auto references = prepare_world_reference_preemption(
                    c.state.world.ai, actor, world_schedule_facts(c.state),
                    c.state.rescue_available, c.state.world.actors.at(actor).definition_task_flag,
                    c.state.world.object_order);
                if (!references.candidate)
                    return reject(WorldScheduleError::common_segment_failed);
                c.state.world.ai = references.candidate->state;
                disposition = invoke({WorldScheduleStage::decision, visit.id, {}});
            } else if (execution) {
                const CharacterId actor{*visit.id};
                const auto prefix = prepare_world_execution_prefix(c.state.world.ai, actor);
                if (!prefix.candidate)
                    return reject(WorldScheduleError::common_segment_failed);
                c.state.world.ai = prefix.candidate->state;
                c.effects.push_back(
                    {actor, prefix.candidate->sounds, prefix.candidate->growth_requests});
                if (!prefix.candidate->sounds.empty() ||
                    !prefix.candidate->growth_requests.empty()) {
                    WorldScheduleCall effects{WorldScheduleStage::prefix_effects, visit.id, {}};
                    effects.effects = c.effects.back();
                    disposition = invoke(effects);
                    if (!disposition || *disposition != WorldScheduleDisposition::keep)
                        return reject(disposition ? WorldScheduleError::invalid_mutation : error);
                }
                if (prefix.candidate->request_carry_expression) {
                    disposition = invoke({WorldScheduleStage::carry_expression, visit.id, {}});
                    if (!disposition || *disposition != WorldScheduleDisposition::keep)
                        return reject(disposition ? WorldScheduleError::invalid_mutation : error);
                }
                disposition = invoke({WorldScheduleStage::control, visit.id, {}});
                if (disposition && *disposition == WorldScheduleDisposition::keep) {
                    WorldActorTailInput tail_input{actor, world_schedule_facts(c.state),
                                                   c.state.spawn_cells};
                    if (input.projected_facing)
                        tail_input.facing_after_projection = [&](const BattleActorRecord &value) {
                            return input.projected_facing(actor, value);
                        };
                    const auto tail = prepare_world_actor_tail(c.state.world, tail_input);
                    if (!tail.candidate)
                        return reject(WorldScheduleError::common_segment_failed);
                    c.state.world = tail.candidate->state;
                    if (input.publish_actor_tail) {
                        WorldScheduleCall cache{WorldScheduleStage::actor_tail_cache, visit.id, {}};
                        cache.projected_actor = tail.candidate->projected_actor;
                        const auto published = invoke(cache);
                        if (!published || *published != WorldScheduleDisposition::keep)
                            return reject(published ? WorldScheduleError::invalid_mutation : error);
                    }
                    if (tail.candidate->delete_instance)
                        disposition = WorldScheduleDisposition::remove_requested;
                }
            } else {
                const auto stage =
                    visit.phase == AiSchedulePhase::projectile  ? WorldScheduleStage::projectile
                    : visit.phase == AiSchedulePhase::object    ? WorldScheduleStage::object
                    : visit.phase == AiSchedulePhase::encounter ? WorldScheduleStage::encounter
                    : visit.phase == AiSchedulePhase::facility  ? WorldScheduleStage::facility
                                                                : WorldScheduleStage::finalize;
                disposition = invoke({stage, visit.id, {}});
            }
            if (!disposition)
                return reject(error);
            remove = *disposition != WorldScheduleDisposition::keep;
            if (remove && (visit.phase == AiSchedulePhase::finalize ||
                           (visit.phase == AiSchedulePhase::facility &&
                            *disposition != WorldScheduleDisposition::already_removed)))
                return reject(WorldScheduleError::invalid_mutation);
            const auto now = rosters(c.state);
            const bool present = visit.id && std::find(now[list].begin(), now[list].end(),
                                                       *visit.id) != now[list].end();
            if (remove && (decision || execution)) {
                if (*disposition == WorldScheduleDisposition::remove_requested) {
                    const auto removed =
                        prepare_world_actor_remove(c.state.world, {*visit.id}, execution);
                    if (!removed.candidate)
                        return reject(WorldScheduleError::invalid_mutation);
                    c.state.world = removed.candidate->state;
                } else if (present || (execution && list == 0))
                    return reject(WorldScheduleError::invalid_mutation);
            } else if (remove &&
                       (*disposition != WorldScheduleDisposition::already_removed || present))
                return reject(WorldScheduleError::invalid_mutation);
            AiScheduleResponse response;
            response.remove = remove;
            if (!valid_phase_mutation(current, rosters(c.state), list, visit.id.value_or(0), remove,
                                      response.append) ||
                !valid_world_schedule_owner(c.state))
                return reject(WorldScheduleError::invalid_mutation);
            return response;
        });
    if (!scheduled.candidate)
        return fail(scheduled.error == AiScheduleError::dispatch_limit
                        ? WorldScheduleError::dispatch_limit
                    : error == WorldScheduleError::none ? WorldScheduleError::invalid_mutation
                                                        : error);
    if (scheduled.candidate->rosters != rosters(c.state))
        return fail(WorldScheduleError::invalid_mutation);
    c.visits = scheduled.candidate->visits;
    roots();
    return {WorldScheduleError::none, c};
}
} // namespace ark::simulation::rules
