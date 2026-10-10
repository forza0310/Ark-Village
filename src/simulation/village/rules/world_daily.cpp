#include "ark/simulation/village/rules/world_daily.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace ark::simulation::rules {
namespace {
bool inside(Position p, TownBounds b) {
    return p.x > b.left && p.x < b.right && p.y > b.top && p.y < b.bottom;
}
bool same_map(const LegacyMap &a, const LegacyMap &b) {
    if (a.width != b.width || a.height != b.height || a.cells.size() != b.cells.size())
        return false;
    for (std::size_t n = 0; n < a.cells.size(); ++n) {
        const auto &x = a.cells[n];
        const auto &y = b.cells[n];
        if (x.legacy_state != y.legacy_state || x.category != y.category ||
            x.facility.has_value() != y.facility.has_value())
            return false;
        if (x.facility && (!(x.facility->instance_id == y.facility->instance_id) ||
                           x.facility->definition_id != y.facility->definition_id ||
                           x.facility->fragment_index != y.facility->fragment_index))
            return false;
    }
    return true;
}
bool box_valid(CollisionBox b) {
    return std::isfinite(b.x_offset) && std::isfinite(b.z_offset) && std::isfinite(b.width) &&
           std::isfinite(b.depth) && b.width > 0 && b.depth > 0 && std::abs(b.x_offset) < 1000000 &&
           std::abs(b.z_offset) < 1000000 && b.width < 1000000 && b.depth < 1000000;
}
bool boxes_touch(CombatPoint a, CollisionBox ab, CombatPoint b, CollisionBox bb) {
    const float ax = a.x + ab.x_offset, az = a.z + ab.z_offset;
    const float bx = b.x + bb.x_offset, bz = b.z + bb.z_offset;
    return ax <= bx + bb.width && bx <= ax + ab.width && az - ab.depth <= bz && bz - bb.depth <= az;
}
float source_arc(float height, int duration, int tick) {
    const int half = duration / 2;
    const float v = half == 1 ? 0.0F : 2.0F * height / (half - 1);
    const float acceleration = half <= 1 ? 0.0F : -2.0F * height / ((half - 1) * half);
    const float t = static_cast<float>(tick);
    return std::max(0.0F, v * t + acceleration * t * (t + 1.0F) / 2.0F);
}
} // namespace
WorldDailyResult prepare_world_daily_c(const WorldDailyState &s, const WorldDailyInput &i) {
    const auto fail = [](WorldDailyError error) { return WorldDailyResult{error, {}}; };
    const auto found = s.world.ai.battle.actors.find(i.actor);
    if (!i.actor.value || found == s.world.ai.battle.actors.end() ||
        !(found->second.id == i.actor) || !s.world.ai.contexts.count(i.actor) ||
        !s.world.actors.count(i.actor))
        return fail(WorldDailyError::stale_actor);
    const auto &old = found->second;
    if (old.kind != ActorKind::human && old.kind != ActorKind::monster)
        return fail(WorldDailyError::invalid_input);
    const auto &roster =
        old.kind == ActorKind::human ? s.world.ai.human_order : s.world.ai.monster_order;
    if (std::count(roster.begin(), roster.end(), i.actor) != 1 ||
        !world_control_detail::valid_control(old.control) || old.state_counter < 0 ||
        !valid_world_map_facts(s.facts) || !same_map(s.world.map, s.facts.map))
        return fail(WorldDailyError::invalid_input);
    WorldDailyCandidate c;
    c.state = s;
    WorldDailyError error{WorldDailyError::none};
    const auto expression = [&](int type) {
        auto &effects = c.state.world.ai.contexts.at(i.actor).effects;
        std::optional<WorldExpressionTicket> supplied;
        if (c.consumed_expressions < i.expressions.size())
            supplied = i.expressions[c.consumed_expressions];
        else if (i.expression_draw) {
            try {
                supplied = i.expression_draw(effects, type);
            } catch (...) {
                error = WorldDailyError::preparation_failed;
                return false;
            }
        }
        if (!supplied) {
            error = WorldDailyError::missing_ticket;
            return false;
        }
        const auto &ticket = *supplied;
        const auto prepared = prepare_actor_expression(
            {effects, type, 0, ticket.probability, ticket.variant_count, ticket.variant});
        if (!prepared.candidate) {
            error = WorldDailyError::preparation_failed;
            return false;
        }
        effects = prepared.candidate->state;
        ++c.consumed_expressions;
        c.consumed_variants += prepared.candidate->consumed_variant ? 1U : 0U;
        return true;
    };
    const auto transition = [&](int state) {
        auto boost = i.boost_ticket;
        if (state == 18 && !(c.state.world.ai.battle.actors.at(i.actor).control.flags & 2048U) &&
            !boost && i.draw) {
            try {
                boost = i.draw(100);
            } catch (...) {
                error = WorldDailyError::preparation_failed;
                return false;
            }
        }
        const auto result = prepare_world_state_transition(c.state.world, {i.actor, state, boost});
        if (!result.candidate) {
            error = WorldDailyError::preparation_failed;
            return false;
        }
        c.state.world = result.candidate->state;
        c.consumed_boost |= result.candidate->consumed_boost_ticket;
        c.event_requests.insert(c.event_requests.end(), result.candidate->event_requests.begin(),
                                result.candidate->event_requests.end());
        return true;
    };
    const auto clear_path = [&] {
        auto &path = c.state.world.actors.at(i.actor);
        path.journey.reset();
        path.unbound_route.reset();
        path.waypoint = 0;
        path.path_pending = false;
    };
    const auto activity = [&](int parameter) {
        c.state.world.ai.battle.actors.at(i.actor).control.queue.push_back({8, parameter});
    };
    const auto event_gate = [&]() -> std::optional<bool> {
        auto task = c.state.task;
        task.definition_task_flag = c.state.world.actors.at(i.actor).definition_task_flag;
        const auto gate = prepare_world_event_gate(c.state.world.ai, i.actor, c.state.facts, task);
        if (!gate.candidate) {
            error = WorldDailyError::preparation_failed;
            return {};
        }
        c.state.world.ai = gate.candidate->state;
        if (gate.candidate->gate.request_task_encounter) {
            if (!i.task_creation || !(i.task_creation->actor == i.actor) ||
                i.task_creation->task.kind != task.kind ||
                !(i.task_creation->task.center == task.center) ||
                i.task_creation->task.encounter != task.encounter ||
                i.task_creation->task.definition_task_flag != task.definition_task_flag) {
                error = WorldDailyError::missing_fact;
                return {};
            }
            auto task_creation = *i.task_creation;
            if (i.draw)
                task_creation.draw = i.draw;
            const auto entry =
                prepare_world_event_entry(c.state.world.ai, c.state.facts, task_creation);
            if (!entry.candidate) {
                error = WorldDailyError::preparation_failed;
                return {};
            }
            c.state.world.ai = entry.candidate->state;
            c.state.facts = entry.candidate->facts;
            c.state.world.map = c.state.facts.map;
            c.state.task = entry.candidate->task;
            c.task_entry = entry.candidate;
        }
        return gate.candidate->gate.ready;
    };
    const auto spawn = [&] {
        const auto &a = c.state.world.ai.battle.actors.at(i.actor);
        const auto &ctx = c.state.world.ai.contexts.at(i.actor);
        const auto &path = c.state.world.actors.at(i.actor);
        const auto destination = path.destination ? path.destination
                                 : path.binding   ? std::optional<Position>(path.binding->goal)
                                                  : std::optional<Position>(Position{});
        if (c.state.world.ai.monster_order.size() >
            static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            error = WorldDailyError::invalid_input;
            return false;
        }
        SpawnProbeInput probe_input{a.kind,
                                    a.control.state,
                                    ctx.inside_town,
                                    inside(*destination, c.state.facts.town),
                                    ctx.cell.y,
                                    c.state.facts.town.top,
                                    static_cast<int>(c.state.world.ai.monster_order.size()),
                                    c.state.world.ai.monster_limit,
                                    i.spawn_ticket};
        auto probe = prepare_spawn_probe(probe_input);
        if (!probe.candidate && !probe_input.ticket && probe_input.monster_limit >= 0 && i.draw) {
            try {
                probe_input.ticket = i.draw(1000);
            } catch (...) {
                error = WorldDailyError::preparation_failed;
                return false;
            }
            probe = prepare_spawn_probe(probe_input);
        }
        if (!probe.candidate) {
            error = i.spawn_ticket ? WorldDailyError::preparation_failed
                                   : WorldDailyError::missing_ticket;
            return false;
        }
        c.consumed_spawn = probe.candidate->consumes_ticket;
        if (!probe.candidate->request_event_probe)
            return true;
        if (!i.spawn_creation) {
            error = WorldDailyError::missing_fact;
            return false;
        }
        auto creation = *i.spawn_creation;
        if (i.draw)
            creation.draw = i.draw;
        creation.kind = 0;
        creation.center = ctx.cell;
        if (ctx.cell.x < 0 || ctx.cell.y < 0 || ctx.cell.x >= c.state.world.map.width ||
            ctx.cell.y >= c.state.world.map.height) {
            error = WorldDailyError::invalid_input;
            return false;
        }
        creation.probe = EncounterCreationProbe{
            i.actor,
            inside(*destination, c.state.facts.town),
            c.state.facts.town.top,
            probe_input.ticket,
            c.state.world.map
                .cells[static_cast<std::size_t>(ctx.cell.y * c.state.world.map.width + ctx.cell.x)]
                .legacy_state,
            i.task_centers};
        for (int n = 0; n < 3; ++n) {
            const Position p{ctx.cell.x + n - 1, ctx.cell.y - 1};
            const auto b = c.state.facts.town;
            creation.upper_band_town[static_cast<std::size_t>(n)] =
                p.x >= b.left && p.x <= b.right && p.y >= b.top && p.y <= b.bottom;
        }
        const auto result = prepare_encounter_creation(c.state.world.ai, creation, i.encounter);
        if (!result.candidate) {
            error = WorldDailyError::preparation_failed;
            return false;
        }
        c.state.world.ai = result.candidate->state;
        c.spawn = result.candidate;
        if (result.candidate->created) {
            const auto refreshed = prepare_world_event_map(c.state.world.ai, c.state.facts);
            if (!refreshed.facts) {
                error = WorldDailyError::preparation_failed;
                return false;
            }
            c.state.facts = *refreshed.facts;
            // 新怪物必须同时拥有共享设施/运动context，不能只追加bm。
            for (const auto id : c.state.world.ai.monster_order)
                if (!c.state.world.actors.count(id)) {
                    if (std::find(s.world.ai.monster_order.begin(), s.world.ai.monster_order.end(),
                                  id) != s.world.ai.monster_order.end()) {
                        error = WorldDailyError::missing_fact;
                        return false;
                    }
                    RescueActorContext context;
                    context.destination = Position{}; // 原fresh O数组全0，不是猜测出口。
                    c.state.world.actors.emplace(id, context);
                }
        }
        return true;
    };
    switch (old.control.state) {
    case 0: {
        if (!spawn())
            return fail(error);
        if (!i.path || !(i.path->actor == i.actor))
            return fail(WorldDailyError::missing_fact);
        auto input = *i.path;
        input.facts = c.state.facts;
        input.task = c.state.task;
        input.task.definition_task_flag = c.state.world.actors.at(i.actor).definition_task_flag;
        const auto path = prepare_world_path_c(c.state.world, input);
        if (!path.candidate)
            return fail(WorldDailyError::preparation_failed);
        c.state.world = path.candidate->state;
        c.state.facts = path.candidate->facts;
        c.state.task = path.candidate->task;
        c.path = path.candidate;
        if (path.candidate->event116)
            c.event_requests.push_back(116);
        break;
    }
    case 5: {
        if (old.kind != ActorKind::human)
            return fail(WorldDailyError::unsupported_state);
        if (!expression(8))
            return fail(error);
        const auto definition = c.state.world.ai.growth.find(old.definition);
        if (definition == c.state.world.ai.growth.end())
            return fail(WorldDailyError::missing_fact);
        const int effort = definition->second.definition.legacy_u;
        if (old.control.flags & 16U)
            break;
        if (c.state.world.ai.task_active && c.state.world.actors.at(i.actor).definition_task_flag) {
            if (!transition(0))
                return fail(error);
            activity(1);
            break;
        }
        const auto ready = event_gate();
        if (!ready)
            return fail(error);
        if (*ready) {
            if (!expression(6) || !transition(18))
                return fail(error);
            break;
        }
        if (old.rescue) {
            clear_path();
            if (!transition(0))
                return fail(error);
            activity(4);
            break;
        }
        const auto cell = c.state.world.ai.contexts.at(i.actor).cell;
        if (c.state.world.actors.at(i.actor).outside_updates % 100 == 0)
            for (const auto id : c.state.world.ai.encounter_order) {
                const auto encounter = c.state.world.ai.encounters.find(id);
                if (encounter == c.state.world.ai.encounters.end())
                    return fail(WorldDailyError::missing_fact);
                const auto center = encounter->second.runtime.center;
                if (std::abs(static_cast<std::int64_t>(cell.x) - center.x) <= 3 &&
                    std::abs(static_cast<std::int64_t>(cell.y) - center.y) <= 3) {
                    if (!transition(0))
                        return fail(error);
                    activity(6);
                    break;
                }
            }
        const auto &current = c.state.world.ai.battle.actors.at(i.actor);
        const auto idle = prepare_human_idle(
            {current.control.flags, false, false, false, false, false,
             c.state.world.actors.at(i.actor).outside_updates,
             current.control.action == 7 ? 0 : current.hp.target, current.capacity, effort});
        if (!idle.candidate)
            return fail(WorldDailyError::preparation_failed);
        if (idle.candidate->activity) {
            if (!transition(0))
                return fail(error);
            activity(*idle.candidate->activity);
        }
        if (!spawn())
            return fail(error);
        break;
    }
    case 8:
    case 9: {
        auto &a = c.state.world.ai.battle.actors.at(i.actor);
        a.attack_position = {a.position.x, source_arc(20, 12, old.state_counter - 55),
                             a.position.z};
        c.ground_effect21 = old.state_counter == 65;
        if (old.state_counter < 73)
            break;
        a.control.flags &= ~1U;
        if (!c.state.world.ai.battle.events.count(90)) {
            if (!i.event)
                return fail(WorldDailyError::missing_consumer);
            std::optional<RescueWorldState> event;
            try {
                event = i.event(c.state.world, 90);
            } catch (...) {
                return fail(WorldDailyError::consumer_failed);
            }
            if (!event || !event->ai.battle.events.count(90) ||
                !event->ai.battle.actors.count(i.actor) || !event->actors.count(i.actor))
                return fail(WorldDailyError::consumer_failed);
            c.state.world = std::move(*event);
        }
        const int mode = c.state.world.actors.at(i.actor).monster_mode;
        if (mode < 0 || mode > 4)
            return fail(WorldDailyError::invalid_input);
        if (!transition(17))
            return fail(error);
        auto &queue = c.state.world.ai.battle.actors.at(i.actor).control.queue;
        if (mode == 0)
            queue.push_back({12});
        else if (mode == 1)
            activity(7);
        else if (mode == 3)
            queue.push_back({13});
        else if (mode == 4)
            activity(8);
        break;
    }
    case 11: {
        if (old.kind != ActorKind::human)
            return fail(WorldDailyError::unsupported_state);
        const auto ready = event_gate();
        if (!ready)
            return fail(error);
        if (*ready) {
            if (!expression(6) || !transition(18))
                return fail(error);
            break;
        }
        std::vector<ObjectProbe> objects;
        std::set<std::uint64_t> seen;
        for (const auto id : c.state.world.object_order) {
            const auto object = c.state.world.ai.battle.objects.find(id);
            if (!seen.insert(id).second || object == c.state.world.ai.battle.objects.end() ||
                object->second.id.value != id)
                return fail(WorldDailyError::missing_fact);
            objects.push_back({object->second.id, object->second.state, object->second.position});
        }
        if (objects.size() != c.state.world.ai.battle.objects.size())
            return fail(WorldDailyError::missing_fact);
        const auto selected = select_ground_object(old.position, objects);
        if (selected.error != ObjectError::none)
            return fail(WorldDailyError::preparation_failed);
        bool touching{};
        if (selected.selected && old.object_slot == -1) {
            if (!i.actor_box || !i.object_box || !box_valid(*i.actor_box) ||
                !box_valid(*i.object_box))
                return fail(WorldDailyError::missing_fact);
            touching =
                boxes_touch(old.position, *i.actor_box,
                            c.state.world.ai.battle.objects.at(selected.selected->value).position,
                            *i.object_box);
        }
        const auto pickup = prepare_ground_pickup_commit(
            c.state.world.ai.battle, {i.actor, c.state.world.object_order, false, touching, 0, {}});
        if (!pickup.candidate)
            return fail(WorldDailyError::preparation_failed);
        c.state.world.ai.battle = pickup.candidate->state;
        c.pickup = pickup.candidate;
        if (pickup.candidate->action == PickupAction::chase) {
            const auto target = *pickup.candidate->chase_target;
            const auto motion = advance_character_motion({old.position.x, old.position.z},
                                                         {target.x, target.z}, old.control.flags);
            if (!motion.step)
                return fail(WorldDailyError::preparation_failed);
            auto &a = c.state.world.ai.battle.actors.at(i.actor);
            a.position.x = motion.step->position.x;
            a.position.z = motion.step->position.z;
            if (motion.step->velocity)
                c.state.world.actors.at(i.actor).horizontal_velocity = *motion.step->velocity;
        }
        break;
    }
    default:
        return fail(WorldDailyError::unsupported_state);
    }
    return {WorldDailyError::none, std::move(c)};
}
} // namespace ark::simulation::rules
