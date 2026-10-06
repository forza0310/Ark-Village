#include "ark/simulation/rules/world_departure.hpp"
#include "ark/simulation/rules/world_detached_actor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace ark::simulation::rules {
namespace {
bool within(const LegacyMap &map, Position p) {
    return p.x >= 0 && p.x < map.width && p.y >= 0 && p.y < map.height;
}
std::size_t index(const LegacyMap &map, Position p) {
    return static_cast<std::size_t>(p.y) * map.width + p.x;
}
bool live(const RescueWorldState &s, CharacterId id) {
    const auto a = s.ai.battle.actors.find(id);
    if (!id.value || a == s.ai.battle.actors.end() || !(a->second.id == id) ||
        !s.actors.count(id) || !s.ai.contexts.count(id))
        return false;
    const auto &roster = a->second.kind == ActorKind::human ? s.ai.human_order : s.ai.monster_order;
    return std::count(roster.begin(), roster.end(), id) == 1;
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
template <class Map> bool same_keys(const Map &a, const Map &b) {
    if (a.size() != b.size())
        return false;
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](const auto &x, const auto &y) { return x.first == y.first; });
}
bool valid_path_callback(const RescueWorldState &old, const RescueWorldState &next,
                         CharacterId self) {
    if (!same_map(old.map, next.map) || old.ai.human_order != next.ai.human_order ||
        old.ai.monster_order != next.ai.monster_order ||
        !same_keys(old.ai.battle.actors, next.ai.battle.actors) ||
        !same_keys(old.ai.contexts, next.ai.contexts) || !same_keys(old.actors, next.actors) ||
        !same_keys(old.facilities, next.facilities) ||
        !(old.ai.contexts.at(self).cell == next.ai.contexts.at(self).cell) ||
        !(old.ai.contexts.at(self).half_cell == next.ai.contexts.at(self).half_cell))
        return false;
    const auto &before_context = old.actors.at(self);
    const auto &after_context = next.actors.at(self);
    const auto &before_position = old.ai.battle.actors.at(self).position;
    const auto &after_position = next.ai.battle.actors.at(self).position;
    if (!(before_context.destination == after_context.destination) ||
        before_context.binding.has_value() != after_context.binding.has_value() ||
        (before_context.binding &&
         (!(before_context.binding->goal == after_context.binding->goal) ||
          !(before_context.binding->instance_id == after_context.binding->instance_id) ||
          before_context.binding->definition_id != after_context.binding->definition_id)) ||
        before_position.x != after_position.x || before_position.z != after_position.z ||
        before_position.height != after_position.height)
        return false;
    for (const auto &[id, before] : old.ai.battle.actors) {
        const auto &after = next.ai.battle.actors.at(id);
        if (!(after.id == before.id) || after.kind != before.kind ||
            after.definition != before.definition || after.legacy_id != before.legacy_id ||
            !live(next, id) || !prepare_local_control_prefix(after.control).candidate)
            return false;
    }
    return true;
}
bool path_transition(RescueWorldState &s, CharacterId id, int state, std::optional<int> boost = {},
                     WorldPathCandidate *out = nullptr) {
    auto &a = s.ai.battle.actors.at(id);
    ActorStateTransitionInput i;
    i.control = a.control;
    i.human = a.kind == ActorKind::human;
    i.next_state = state;
    i.baseline = a.baseline;
    i.boost_ticket = boost;
    i.boost_event116_seen = s.ai.battle.events.count(116) != 0;
    if (state == 18 && !(a.control.flags & 2048U)) {
        const auto definition = s.ai.growth.find(a.definition);
        if (definition == s.ai.growth.end())
            return false;
        i.legacy_u = definition->second.definition.legacy_u;
    }
    const auto r = prepare_actor_state_transition(i);
    if (!r || r->request_cleanup || r->copy_attack_position)
        return false;
    a.control = r->control;
    a.baseline = r->baseline;
    a.state_counter = a.state_parameter = 0;
    if (r->clear_encounter)
        a.encounter.reset();
    if (r->reset_attack_count)
        a.attack_count = 0;
    if (r->request_boost_event116)
        s.ai.battle.events.insert(116);
    if (out) {
        out->consumed_boost_ticket = r->consumed_boost_ticket;
        out->event116 = r->request_boost_event116;
    }
    return true;
}
bool path_cleanup(RescueWorldState &s, CharacterId id) {
    if (s.ai.battle.actors.at(id).kind == ActorKind::human) {
        const auto r = prepare_world_rescue_cleanup(s, id);
        if (!r.candidate)
            return false;
        s = r.candidate->state;
        return true;
    }
    auto &a = s.ai.battle.actors.at(id);
    const auto cleanup = prepare_actor_cleanup(a.kind, a.control.flags);
    if (!cleanup)
        return false;
    const auto &binding = s.actors.at(id).binding;
    if (binding && arrival_binding_matches(s.map, *binding, s.ai.contexts.at(id).cell)) {
        const auto f = s.facilities.find(binding->instance_id.value);
        if (f != s.facilities.end() &&
            f->second.placement.definition_id == binding->definition_id &&
            f->second.placement.instance_id == binding->instance_id) {
            auto &occupants = f->second.occupants;
            const auto it = std::find(occupants.begin(), occupants.end(), id);
            if (it != occupants.end())
                occupants.erase(it);
        }
    }
    a.rescue.reset();
    a.encounter.reset();
    a.group.reset();
    a.position.height = 0;
    a.control.flags = cleanup->flags;
    if (!path_transition(s, id, cleanup->state))
        return false;
    a.control.queue.push_back({8, cleanup->activity});
    return true;
}
struct Draws {
    const std::vector<std::int64_t> &tickets;
    const std::function<std::optional<std::int64_t>(int)> &draw;
    std::size_t used{};
    WorldDepartureError error{WorldDepartureError::none};
    std::optional<std::int64_t> take(std::int64_t bound) {
        if (bound <= 0) {
            error = WorldDepartureError::preparation_failed;
            return {};
        }
        std::optional<std::int64_t> next;
        if (used < tickets.size())
            next = tickets[used];
        else if (draw && bound <= std::numeric_limits<int>::max()) {
            try {
                next = draw(static_cast<int>(bound));
            } catch (...) {
                error = WorldDepartureError::preparation_failed;
                return {};
            }
        }
        if (!next) {
            error = WorldDepartureError::missing_ticket;
            return {};
        }
        const auto value = *next;
        ++used;
        if (value < 0 || value >= bound) {
            error = WorldDepartureError::invalid_ticket;
            return {};
        }
        return value;
    }
};
std::optional<Position> category_goal(int category, std::int64_t count,
                                      const ActivityCandidateSnapshot &snapshot,
                                      const RescueWorldState &s, const WorldDepartureInput &i,
                                      Draws &draws) {
    const auto cell = s.ai.contexts.at(i.actor).cell;
    const auto &actor = s.ai.battle.actors.at(i.actor);
    if (category == -1) {
        for (int attempt = 0; attempt < 6; ++attempt) {
            const auto ticket = draws.take(static_cast<std::int64_t>(snapshot.cells.size()));
            if (!ticket)
                return {};
            const auto goal = snapshot.cells[static_cast<std::size_t>(*ticket)].position;
            // 前五次偏好 y>=town.bottom+2；第六次无条件接受，不能减少真实抽号。
            if (attempt == 5 || static_cast<std::int64_t>(goal.y) >=
                                    static_cast<std::int64_t>(i.catalogue.town.bottom) + 2)
                return goal;
        }
    }
    if (category == 3) {
        if (!i.home || i.home->state < 0 || !within(s.map, i.home->cell)) {
            draws.error = WorldDepartureError::invalid_input;
            return {};
        }
        if (i.home->state == 1) {
            const auto ticket = draws.take(100);
            if (!ticket)
                return {};
            // 保留先抽号的短路顺序，以及原版两个坐标均不同的 AND 条件。
            if (*ticket < 90 && !actor.rescue && cell.x != i.home->cell.x &&
                cell.y != i.home->cell.y) {
                const auto &tile = s.map.cells[index(s.map, i.home->cell)];
                if (tile.facility) {
                    const auto f = s.facilities.find(tile.facility->instance_id.value);
                    if (f == s.facilities.end()) {
                        draws.error = WorldDepartureError::invalid_catalogue;
                        return {};
                    }
                    if (f->second.status != 0)
                        return i.home->cell;
                }
            }
        }
        if (i.exits.size() < 2 || std::any_of(i.exits.begin(), i.exits.end(),
                                              [&](Position p) { return !within(s.map, p); })) {
            draws.error = WorldDepartureError::invalid_input;
            return {};
        }
        for (std::size_t n = 0; n < 2; ++n)
            if (cell == i.exits[n])
                return i.exits[1 - n];
        const auto ticket = draws.take(static_cast<std::int64_t>(i.exits.size()));
        return ticket ? std::optional<Position>(i.exits[static_cast<std::size_t>(*ticket)])
                      : std::nullopt;
    }
    if (count == 0)
        return {};
    if (category == 4) {
        const auto ticket = draws.take(count);
        if (!ticket)
            return {};
        auto remaining = *ticket;
        // 活动 1 可以传入 dm5 作抽取上限，但原扫描仍是类别 4，不修正原错位。
        for (const auto &candidate : snapshot.cells)
            if (candidate.definition.legacy_category == 4 && remaining-- == 0)
                return candidate.position;
        return {};
    }
    if (category != 1 && category != 2 && category != 6 && category != 8) {
        draws.error = WorldDepartureError::invalid_input;
        return {};
    }
    std::int64_t weight{};
    for (const auto &candidate : snapshot.cells) {
        const auto &definition = candidate_instance_definition(candidate);
        if (candidate.instance && candidate.instance->legacy_phase == 1 &&
            definition.legacy_category == category) {
            if (definition.definition_charm > std::numeric_limits<int>::max() - weight) {
                draws.error = WorldDepartureError::invalid_catalogue;
                return {};
            }
            weight += definition.definition_charm;
        }
    }
    if (weight == 0)
        return {};
    const auto ticket = draws.take(weight);
    if (!ticket)
        return {};
    const auto selected = select_snapshot_facility(snapshot, category, *ticket);
    if (!selected.target) {
        draws.error = WorldDepartureError::preparation_failed;
        return {};
    }
    return selected.target->goal.position;
}
} // namespace
LegacySearchResult prepare_world_departure_field(const LegacyMap &map, Position start,
                                                 std::int64_t limit) {
    LegacySearchLimits limits;
    limits.max_expanded_cost = limit;
    limits.reverse_equal_cost = true;
    return search_legacy_map(map, start, limits);
}
static WorldDepartureResult prepare_departure_impl(const RescueWorldState &s,
                                                   const WorldDepartureInput &i, bool detached) {
    const auto fail = [](WorldDepartureError e) -> WorldDepartureResult { return {e, {}}; };
    if (!(detached ? valid_detached_human(s, i.actor) : live(s, i.actor)))
        return fail(WorldDepartureError::stale_actor);
    if (!valid_legacy_map(s.map) || i.activity < 0 || i.activity > 8 ||
        i.catalogue.town.left >= i.catalogue.town.right ||
        i.catalogue.town.top >= i.catalogue.town.bottom)
        return fail(WorldDepartureError::invalid_input);
    const auto cell = s.ai.contexts.at(i.actor).cell;
    if (!within(s.map, cell))
        return fail(WorldDepartureError::invalid_input);
    const auto &actor = s.ai.battle.actors.at(i.actor);
    const auto &context = s.actors.at(i.actor);
    if (std::any_of(context.visits.legacy_visit_counts.begin(),
                    context.visits.legacy_visit_counts.end(),
                    [](int value) { return value < 0; }) ||
        context.visits.legacy_category_one_count < 0)
        return fail(WorldDepartureError::invalid_input);
    const auto limit = i.activity == 6 && !inside_town(cell, i.catalogue.town)
                           ? 500
                           : std::numeric_limits<int>::max();
    const auto field = prepare_world_departure_field(s.map, cell, limit);
    if (!field.field)
        return fail(WorldDepartureError::preparation_failed);
    auto catalogue = i.catalogue;
    catalogue.legacy_activity = i.activity;
    catalogue.last_visited_instance = context.visits.last_visited_instance;
    catalogue.instances.clear();
    for (const auto &[key, facility] : s.facilities) {
        if (key != facility.placement.instance_id.value)
            return fail(WorldDepartureError::invalid_catalogue);
        const auto definition_id = facility.placement.definition_id;
        const auto definition = std::find_if(
            catalogue.definitions.begin(), catalogue.definitions.end(),
            [&](const CandidateDefinition &value) { return value.definition_id == definition_id; });
        if (definition == catalogue.definitions.end() ||
            definition->legacy_category != facility.category)
            return fail(WorldDepartureError::invalid_catalogue);
        catalogue.instances.push_back(
            {facility.placement.instance_id, facility.placement.definition_id, facility.status});
    }
    const auto candidates = collect_activity_candidates(*field.field, catalogue);
    if (!candidates.snapshot)
        return fail(WorldDepartureError::invalid_catalogue);
    WorldDepartureCandidate c;
    c.state = s;
    c.snapshot = *candidates.snapshot;
    auto &next_context = c.state.actors.at(i.actor);
    next_context.journey.reset();
    next_context.unbound_route.reset();
    next_context.path_pending = false;
    next_context.waypoint = 0;
    if (c.snapshot.cells.empty()) {
        c.denial = WorldDepartureDenial::no_candidates;
        return {WorldDepartureError::none, std::move(c)};
    }
    Draws draws{i.tickets, i.draw};
    const auto finish = [&]() -> WorldDepartureResult {
        if (draws.error != WorldDepartureError::none)
            return fail(draws.error);
        c.consumed_tickets = draws.used;
        if (!c.goal) {
            c.denial = WorldDepartureDenial::no_selection;
            return {WorldDepartureError::none, std::move(c)};
        }
        const auto route = trace_legacy_path(*field.field, *c.goal);
        if (route.error != MapAccessError::none) {
            c.denial = WorldDepartureDenial::no_route;
            return {WorldDepartureError::none, std::move(c)};
        }
        c.route = route;
        next_context.destination = c.goal;
        const auto &tile = s.map.cells[index(s.map, *c.goal)];
        next_context.binding.reset();
        if (tile.facility) {
            const auto f = s.facilities.find(tile.facility->instance_id.value);
            if (f == s.facilities.end() ||
                f->second.placement.definition_id != tile.facility->definition_id)
                return fail(WorldDepartureError::invalid_catalogue);
            const ArrivalBinding binding{*c.goal, tile.facility->instance_id,
                                         tile.facility->definition_id};
            next_context.binding = binding;
            const auto selected = std::find_if(c.snapshot.cells.begin(), c.snapshot.cells.end(),
                                               [&](const ActivityCandidateCell &candidate) {
                                                   return candidate.position == *c.goal;
                                               });
            ActivityCandidateCell target;
            std::size_t selected_index{};
            if (selected != c.snapshot.cells.end()) {
                target = *selected;
                selected_index = static_cast<std::size_t>(selected - c.snapshot.cells.begin());
            } else {
                const auto surface_id = catalogue.cell_definition_ids[index(s.map, *c.goal)];
                const auto d =
                    std::find_if(catalogue.definitions.begin(), catalogue.definitions.end(),
                                 [&](const CandidateDefinition &value) {
                                     return value.definition_id == surface_id;
                                 });
                const auto instance_definition =
                    std::find_if(catalogue.definitions.begin(), catalogue.definitions.end(),
                                 [&](const CandidateDefinition &value) {
                                     return value.definition_id == tile.facility->definition_id;
                                 });
                if (d == catalogue.definitions.end() ||
                    instance_definition == catalogue.definitions.end())
                    return fail(WorldDepartureError::invalid_catalogue);
                target = {*c.goal,
                          *d,
                          CandidateInstance{tile.facility->instance_id,
                                            tile.facility->definition_id, f->second.status},
                          route.cost,
                          CandidateOrigin::map_scan,
                          index(s.map, *c.goal),
                          *instance_definition,
                          tile.legacy_state,
                          tile.category};
            }
            next_context.journey =
                FacilityDeparture{f->second.category,
                                  {selected_index, selected_index, selected_index, target},
                                  binding,
                                  route,
                                  {}};
        } else {
            next_context.unbound_route = route;
            c.unbound_motion = true;
        }
        next_context.path_pending = true;
        if (!route.steps.empty()) {
            const auto first = route.steps.front();
            const int direction = first.x > cell.x   ? 1
                                  : first.x < cell.x ? 3
                                  : first.y > cell.y ? 0
                                                     : 2;
            c.state.ai.battle.actors.at(i.actor).control.facing = direction;
            if (next_context.journey)
                next_context.journey->legacy_direction = direction;
        }
        c.succeeded = true;
        return {WorldDepartureError::none, std::move(c)};
    };
    DepartureOverrideInput priority;
    priority.kind = actor.kind;
    priority.self = i.actor;
    priority.flags = actor.control.flags;
    priority.activity = i.activity;
    priority.has_object = actor.object_slot != -1;
    priority.active_task = s.ai.task_active;
    priority.definition_task_flag = context.definition_task_flag;
    priority.task_center = i.task_center;
    for (const auto &candidate : c.snapshot.cells)
        priority.reachable.push_back(candidate.position);
    const auto apply_override = [&](const DepartureOverrideCandidate &override) {
        c.priority = override.kind;
        c.goal = override.destination;
        if (override.request_task_attribute && !(actor.control.flags & 2048U)) {
            const auto definition = s.ai.growth.find(actor.definition);
            if (definition == s.ai.growth.end() || definition->second.definition.legacy_u < 0 ||
                definition->second.definition.legacy_u > 100)
                return fail(WorldDepartureError::invalid_input);
            const auto ticket = draws.take(100);
            if (ticket && *ticket < definition->second.definition.legacy_u * 20 / 100) {
                c.state.ai.battle.actors.at(i.actor).control.flags |= 2048U;
                c.event116 = c.state.ai.battle.events.insert(116).second;
            }
        }
        return finish();
    };
    auto override = prepare_departure_override(priority);
    if (!override)
        return fail(WorldDepartureError::preparation_failed);
    if (override->destination)
        return apply_override(*override);
    if (actor.kind == ActorKind::human && !(actor.control.flags & (512U | 1024U)) &&
        !context.definition_task_flag) {
        std::vector<RescueTargetSnapshot> people;
        std::set<CharacterId> seen;
        for (const auto id : s.ai.human_order) {
            if (!seen.insert(id).second || !live(s, id) ||
                s.ai.battle.actors.at(id).kind != ActorKind::human)
                return fail(WorldDepartureError::stale_actor);
            const auto &human = s.ai.battle.actors.at(id);
            const auto &cached = s.ai.contexts.at(id);
            const auto &extra = s.actors.at(id);
            people.push_back({id,
                              human.control.state,
                              {human.position.x, human.position.z},
                              cached.cell,
                              cached.half_cell,
                              cached.inside_town,
                              extra.on_event_cell,
                              cached.low_hp});
            if (human.legacy_id != actor.legacy_id)
                priority.people.push_back(
                    {id, extra.destination.value_or(extra.binding ? extra.binding->goal
                                                                  : Position{0, 0})});
        }
        const auto down = select_rescue_target({actor.position.x, actor.position.z}, people);
        if (down.error != ActorAiError::none)
            return fail(WorldDepartureError::preparation_failed);
        if (down.candidate)
            priority.nearest_down = s.ai.contexts.at(down.candidate->id).cell;
        override = prepare_departure_override(priority);
        if (!override)
            return fail(WorldDepartureError::preparation_failed);
        if (override->destination)
            return apply_override(*override);
        std::vector<ObjectProbe> objects;
        std::set<std::uint64_t> object_ids;
        if (s.object_order.size() != s.ai.battle.objects.size())
            return fail(WorldDepartureError::invalid_input);
        for (const auto id : s.object_order) {
            const auto o = s.ai.battle.objects.find(id);
            if (!object_ids.insert(id).second || o == s.ai.battle.objects.end())
                return fail(WorldDepartureError::invalid_input);
            objects.push_back({o->second.id, o->second.state, o->second.position});
        }
        const auto object = select_ground_object(actor.position, objects);
        if (object.error != ObjectError::none)
            return fail(WorldDepartureError::preparation_failed);
        if (object.selected)
            priority.nearest_object = s.ai.battle.objects.at(object.selected->value).cached_cell;
        override = prepare_departure_override(priority);
        if (!override)
            return fail(WorldDepartureError::preparation_failed);
        if (override->destination)
            return apply_override(*override);
        std::vector<std::pair<Position, int>> events;
        std::set<std::uint64_t> event_ids;
        if (i.activity == 6 && s.ai.encounter_order.size() != s.ai.encounters.size())
            return fail(WorldDepartureError::invalid_input);
        if (i.activity == 6)
            for (const auto id : s.ai.encounter_order) {
                const auto event = s.ai.encounters.find(id);
                if (!event_ids.insert(id).second || event == s.ai.encounters.end())
                    return fail(WorldDepartureError::invalid_input);
                const auto p = event->second.runtime.center;
                if (!within(s.map, p))
                    return fail(WorldDepartureError::invalid_input);
                const auto dx = static_cast<std::int64_t>(p.x) - cell.x;
                const auto dy = static_cast<std::int64_t>(p.y) - cell.y;
                const auto distance = std::sqrt(static_cast<double>(dx * dx + dy * dy));
                if (distance > std::numeric_limits<int>::max())
                    return fail(WorldDepartureError::invalid_input);
                events.push_back({p, static_cast<int>(distance)});
            }
        if (!events.empty()) {
            auto best = events.front();
            for (std::size_t n = events.size(); n > 1; --n)
                if (events[n - 1].second < best.second)
                    best = events[n - 1];
            priority.nearest_encounter = best.first;
        }
    }
    override = prepare_departure_override(priority);
    if (!override)
        return fail(WorldDepartureError::preparation_failed);
    if (override->destination)
        return apply_override(*override);
    const auto &counts = c.snapshot.category_counts;
    const auto goal_for = [&](int category, std::int64_t count) {
        c.goal = category_goal(category, count, c.snapshot, s, i, draws);
    };
    if (i.activity == 0 || i.activity == 6) {
        std::array<std::int64_t, 6> visits{};
        std::copy(context.visits.legacy_visit_counts.begin(),
                  context.visits.legacy_visit_counts.end(), visits.begin());
        const auto planned =
            plan_activity_categories({i.activity, counts, visits, actor.control.flags});
        if (!planned.plan)
            return fail(WorldDepartureError::preparation_failed);
        std::optional<int> category = planned.plan->forced_category;
        if (!category && planned.plan->total_weight > 0) {
            const auto ticket = draws.take(planned.plan->total_weight);
            if (ticket) {
                std::vector<std::int64_t> weights;
                for (const auto &option : planned.plan->options)
                    weights.push_back(option.weight);
                const auto selected = select_weighted_ticket(weights, *ticket);
                if (!selected.index)
                    return fail(WorldDepartureError::preparation_failed);
                category = planned.plan->options[*selected.index].category;
            }
        }
        if (category)
            goal_for(*category, *category >= 0 ? counts[static_cast<std::size_t>(*category)] : 0);
    } else if (i.activity == 1) {
        if (actor.object_slot == -1 && context.visits.legacy_visit_counts[1] == 0) {
            constexpr int before_rest[]{60, 50, 40};
            constexpr int after_rest[]{30, 20, 10, 0};
            const int k = context.visits.legacy_category_one_count;
            const int threshold =
                90 - (context.visits.legacy_visit_counts[2] == 0 ? before_rest[std::min(k, 2)]
                                                                 : after_rest[std::min(k, 3)]);
            const auto ticket = draws.take(100);
            if (ticket && *ticket < threshold)
                goal_for(4, counts[5]);
        }
    } else if (i.activity == 4) {
        for (const auto &candidate : c.snapshot.cells)
            if (candidate.definition.legacy_category == 2 && candidate.cost &&
                (!c.goal || *candidate.cost < *field.field->distances[index(s.map, *c.goal)]))
                c.goal = candidate.position;
    } else if (i.activity == 5) {
        goal_for(3, counts[3]);
    } else if (i.activity == 7) {
        const auto ticket = draws.take(static_cast<std::int64_t>(c.snapshot.cells.size()));
        if (ticket)
            c.goal = c.snapshot.cells[static_cast<std::size_t>(*ticket)].position;
    } else if (i.activity == 8) {
        std::vector<Position> special;
        for (const auto &candidate : c.snapshot.cells) {
            if (candidate.definition.legacy_category != 6)
                continue;
            const auto detail = i.definition_details.find(candidate.definition.definition_id);
            if (detail == i.definition_details.end())
                return fail(WorldDepartureError::invalid_catalogue);
            if (detail->second == 3)
                special.push_back(candidate.position);
        }
        const auto ticket = draws.take(
            static_cast<std::int64_t>(special.empty() ? c.snapshot.cells.size() : special.size()));
        if (ticket)
            c.goal = special.empty() ? c.snapshot.cells[static_cast<std::size_t>(*ticket)].position
                                     : special[static_cast<std::size_t>(*ticket)];
    }
    return finish();
}
WorldDepartureResult prepare_world_departure(const RescueWorldState &s,
                                             const WorldDepartureInput &i) {
    return prepare_departure_impl(s, i, false);
}
static WorldDepartureControlResult
departure_control(const RescueWorldState &s, const WorldDepartureControlInput &i, bool detached) {
    const auto fail = [](WorldDepartureError error) -> WorldDepartureControlResult {
        return {error, {}};
    };
    const auto id = i.departure.actor;
    if (!(detached ? valid_detached_human(s, id) : live(s, id)))
        return fail(WorldDepartureError::stale_actor);
    const auto &old = s.ai.battle.actors.at(id);
    if (old.control.queue.empty() ||
        std::any_of(
            old.control.queue.begin(), old.control.queue.end(),
            [](const LegacyActorControl &command) { return !valid_actor_control(command); }) ||
        old.control.queue.front()[0] != 8)
        return fail(WorldDepartureError::invalid_input);
    auto next = s;
    auto input = i.departure;
    input.activity = old.control.queue.front()[1];
    next.ai.battle.actors.at(id).control.queue.erase(
        next.ai.battle.actors.at(id).control.queue.begin());
    const auto departure = prepare_departure_impl(next, input, detached);
    if (!departure.candidate)
        return fail(departure.error);
    WorldDepartureControlCandidate c;
    c.state = departure.candidate->state;
    c.denial = departure.candidate->denial;
    c.goal = departure.candidate->goal;
    c.consumed_tickets = departure.candidate->consumed_tickets;
    c.departure_succeeded = departure.candidate->succeeded;
    if (c.departure_succeeded) {
        c.state.ai.battle.actors.at(id).control.state = 0; // 直接写 A，不调用 c0 重置其他状态。
        return {WorldDepartureError::none, std::move(c)};
    }
    const auto failure = prepare_failed_activity(c.state.ai.battle.actors.at(id).control.flags);
    // 旧 1024 的表情先于旧 32768 删除；后面新置的 1024 不能倒回来重做此判断。
    if (failure.expression18) {
        auto supplied = i.failure_expression;
        if (!supplied && i.expression_draw) {
            try {
                supplied = i.expression_draw(c.state.ai.contexts.at(id).effects, 18);
            } catch (...) {
                return fail(WorldDepartureError::preparation_failed);
            }
        }
        if (!supplied)
            return fail(WorldDepartureError::missing_ticket);
        const auto &ticket = *supplied;
        const auto expression =
            prepare_actor_expression({c.state.ai.contexts.at(id).effects, 18, 0, ticket.probability,
                                      ticket.variant_count, ticket.variant});
        if (!expression.candidate)
            return fail(expression.error == ActorEffectError::missing_ticket
                            ? WorldDepartureError::missing_ticket
                        : expression.error == ActorEffectError::invalid_ticket
                            ? WorldDepartureError::invalid_ticket
                            : WorldDepartureError::invalid_input);
        c.state.ai.contexts.at(id).effects = expression.candidate->state;
        c.consumed_expression = true;
        c.consumed_variant = expression.candidate->consumed_variant;
    }
    c.delete_instance = failure.delete_instance;
    if (c.delete_instance)
        return {WorldDepartureError::none, std::move(c)};
    c.state.ai.battle.actors.at(id).control.flags = failure.flags;
    if (old.kind == ActorKind::human) {
        const auto cleanup = detached ? prepare_world_detached_actor_cleanup(c.state, id)
                                      : prepare_world_rescue_cleanup(c.state, id);
        if (!cleanup.candidate)
            return fail(WorldDepartureError::preparation_failed);
        c.state = cleanup.candidate->state;
    } else {
        auto &actor = c.state.ai.battle.actors.at(id);
        const auto cleanup = prepare_actor_cleanup(actor.kind, actor.control.flags);
        if (!cleanup)
            return fail(WorldDepartureError::preparation_failed);
        const auto &binding = c.state.actors.at(id).binding;
        if (binding &&
            arrival_binding_matches(c.state.map, *binding, c.state.ai.contexts.at(id).cell)) {
            const auto facility = c.state.facilities.find(binding->instance_id.value);
            if (facility != c.state.facilities.end() &&
                facility->second.placement.definition_id == binding->definition_id &&
                facility->second.placement.instance_id == binding->instance_id) {
                auto &occupants = facility->second.occupants;
                const auto occupied = std::find(occupants.begin(), occupants.end(), id);
                if (occupied != occupants.end())
                    occupants.erase(occupied);
            }
        }
        actor.rescue.reset();
        actor.encounter.reset();
        actor.group.reset();
        actor.position.height = 0;
        actor.control.flags = cleanup->flags;
        ActorStateTransitionInput transition;
        transition.control = actor.control;
        transition.human = false;
        transition.baseline = actor.baseline;
        transition.next_state = cleanup->state;
        const auto reset = prepare_actor_state_transition(transition);
        if (!reset)
            return fail(WorldDepartureError::preparation_failed);
        actor.control = reset->control;
        actor.baseline = reset->baseline;
        actor.state_counter = actor.state_parameter = 0;
        actor.control.queue.push_back({8, cleanup->activity});
    }
    // r 产生的等待或第二条 8 归外层同次 FIFO 循环，不能推进下一次共同 d 前段。
    c.cleaned_up = c.continue_interpreter = true;
    return {WorldDepartureError::none, std::move(c)};
}
WorldDepartureControlResult prepare_world_departure_control(const RescueWorldState &s,
                                                            const WorldDepartureControlInput &i) {
    return departure_control(s, i, false);
}
WorldDepartureControlResult
prepare_world_detached_departure_control(const RescueWorldState &s,
                                         const WorldDepartureControlInput &i) {
    return departure_control(s, i, true);
}
WorldPathResult prepare_world_path_c(const RescueWorldState &s, const WorldPathInput &i) {
    const auto fail = [](WorldPathError e) -> WorldPathResult { return {e, {}}; };
    if (!live(s, i.actor))
        return fail(WorldPathError::stale_actor);
    const auto &old = s.ai.battle.actors.at(i.actor);
    if ((old.control.state != 0 && !(old.kind == ActorKind::monster && old.control.state == 17)) ||
        !prepare_local_control_prefix(old.control).candidate || !valid_world_map_facts(i.facts) ||
        !same_map(s.map, i.facts.map))
        return fail(WorldPathError::invalid_input);
    WorldPathCandidate c;
    c.state = s;
    c.facts = i.facts;
    c.task = i.task;
    // F先于越界/G/O判定；已执行的共同c前段和L不得在此重放。
    const auto gate = prepare_world_event_gate(s.ai, i.actor, i.facts, i.task);
    if (!gate.candidate)
        return fail(WorldPathError::preparation_failed);
    c.state.ai = gate.candidate->state;
    if (gate.candidate->gate.request_task_encounter) {
        if (!i.task_attempt)
            return fail(WorldPathError::missing_domain);
        std::optional<WorldEventEntryCandidate> attempted;
        try {
            attempted = i.task_attempt(c.state.ai, c.facts, i.actor, c.task);
        } catch (...) {
            return fail(WorldPathError::invalid_callback);
        }
        if (!attempted)
            return fail(WorldPathError::preparation_failed);
        auto next = c.state;
        next.ai = attempted->state;
        if (!valid_path_callback(c.state, next, i.actor) ||
            !valid_world_map_facts(attempted->facts) || !same_map(s.map, attempted->facts.map) ||
            attempted->facts.surface != i.facts.surface ||
            attempted->facts.town.left != i.facts.town.left ||
            attempted->facts.town.right != i.facts.town.right ||
            attempted->facts.town.top != i.facts.town.top ||
            attempted->facts.town.bottom != i.facts.town.bottom ||
            attempted->task.kind != i.task.kind ||
            attempted->task.definition_task_flag != i.task.definition_task_flag ||
            !(attempted->task.center == i.task.center) || !attempted->gate.ready ||
            !attempted->gate.request_task_encounter ||
            next.ai.battle.actors.at(i.actor).encounter !=
                c.state.ai.battle.actors.at(i.actor).encounter ||
            attempted->task.encounter != attempted->created ||
            attempted->music2 != attempted->created.has_value() ||
            attempted->notice24 != attempted->created.has_value())
            return fail(WorldPathError::invalid_callback);
        if (attempted->created) {
            const auto event = next.ai.encounters.find(*attempted->created);
            const auto refreshed = prepare_world_event_map(next.ai, c.facts);
            if (event == next.ai.encounters.end() ||
                event->second.runtime.id != *attempted->created ||
                !(event->second.runtime.center == i.task.center) || !refreshed.facts ||
                refreshed.facts->flags != attempted->facts.flags)
                return fail(WorldPathError::invalid_callback);
        } else if (attempted->facts.flags != i.facts.flags)
            return fail(WorldPathError::invalid_callback);
        c.state = std::move(next);
        c.facts = std::move(attempted->facts);
        c.task = attempted->task;
        c.created_task_encounter = attempted->created;
        c.task_creation_denial = attempted->denial;
        c.music2 = attempted->music2;
        c.notice24 = attempted->notice24;
        c.attempted_task_creation = true;
    }
    if (gate.candidate->gate.ready) {
        c.event_preempted = true;
        if (old.kind == ActorKind::human) {
            const auto cell = s.ai.contexts.at(i.actor).cell;
            bool nearby = false;
            for (const auto id : s.ai.human_order) {
                if (!live(s, id) || s.ai.battle.actors.at(id).kind != ActorKind::human)
                    return fail(WorldPathError::stale_actor);
                const auto p = s.ai.contexts.at(id).cell;
                nearby |= old.legacy_id != s.ai.battle.actors.at(id).legacy_id &&
                          std::abs(static_cast<std::int64_t>(cell.x) - p.x) <= 1 &&
                          std::abs(static_cast<std::int64_t>(cell.y) - p.y) <= 1;
            }
            if (nearby) {
                auto supplied = i.nearby_expression;
                if (!supplied && i.expression_draw) {
                    try {
                        supplied = i.expression_draw(c.state.ai.contexts.at(i.actor).effects, 7);
                    } catch (...) {
                        return fail(WorldPathError::invalid_callback);
                    }
                }
                if (!supplied)
                    return fail(WorldPathError::missing_ticket);
                const auto &t = *supplied;
                const auto expression =
                    prepare_actor_expression({c.state.ai.contexts.at(i.actor).effects, 7, 0,
                                              t.probability, t.variant_count, t.variant});
                if (!expression.candidate)
                    return fail(expression.error == ActorEffectError::missing_ticket
                                    ? WorldPathError::missing_ticket
                                : expression.error == ActorEffectError::invalid_ticket
                                    ? WorldPathError::invalid_ticket
                                    : WorldPathError::invalid_input);
                c.state.ai.contexts.at(i.actor).effects = expression.candidate->state;
                c.consumed_expression = true;
                c.consumed_variant = expression.candidate->consumed_variant;
            }
            auto boost = i.boost_ticket;
            if (!(c.state.ai.battle.actors.at(i.actor).control.flags & 2048U)) {
                if (!boost && i.draw) {
                    try {
                        boost = i.draw(100);
                    } catch (...) {
                        return fail(WorldPathError::invalid_callback);
                    }
                }
                if (!boost)
                    return fail(WorldPathError::missing_ticket);
                if (*boost < 0 || *boost >= 100)
                    return fail(WorldPathError::invalid_ticket);
                if (!c.state.ai.growth.count(old.definition))
                    return fail(WorldPathError::missing_fact);
            }
            if (!path_transition(c.state, i.actor, 18, boost, &c))
                return fail(WorldPathError::preparation_failed);
        } else if (old.kind == ActorKind::monster) {
            if (!path_transition(c.state, i.actor, 1))
                return fail(WorldPathError::preparation_failed);
        } else
            return fail(WorldPathError::invalid_input);
        return {WorldPathError::none, std::move(c)};
    }
    const auto cell = s.ai.contexts.at(i.actor).cell;
    if (!within(s.map, cell))
        return {WorldPathError::none, std::move(c)}; // 原P越界false，不清路线或伪造到达。
    const auto &ctx = s.actors.at(i.actor);
    if (ctx.journey && ctx.unbound_route)
        return fail(WorldPathError::invalid_input);
    const auto *route = ctx.journey         ? &ctx.journey->route
                        : ctx.unbound_route ? &*ctx.unbound_route
                                            : nullptr;
    if (!route || route->steps.empty())
        return {WorldPathError::none, std::move(c)}; // G空时，即使旧s==O也不进入。
    const auto destination = ctx.destination ? ctx.destination
                             : ctx.binding   ? std::optional<Position>(ctx.binding->goal)
                                             : std::nullopt;
    if (!ctx.path_pending || !destination || !within(s.map, *destination) ||
        route->error != MapAccessError::none || ctx.waypoint >= route->steps.size() ||
        std::any_of(route->steps.begin(), route->steps.end(),
                    [&](Position p) { return !within(s.map, p); }) ||
        (ctx.journey && (!ctx.binding || !(ctx.journey->binding.goal == *destination) ||
                         !(ctx.binding->goal == *destination) ||
                         !(ctx.journey->binding.instance_id == ctx.binding->instance_id) ||
                         ctx.journey->binding.definition_id != ctx.binding->definition_id)) ||
        (ctx.unbound_route && ctx.binding))
        return fail(WorldPathError::invalid_input);
    if (cell == *destination) {
        // j的O2=-1只比较坐标；后来该格出现建筑仍然走地面分支。
        if (ctx.binding && !arrival_binding_matches(s.map, *ctx.binding, cell)) {
            if (!path_cleanup(c.state, i.actor))
                return fail(WorldPathError::preparation_failed);
            c.cleaned_up = true;
            return {WorldPathError::none, std::move(c)};
        }
        auto &next_context = c.state.actors.at(i.actor);
        next_context.journey.reset();
        next_context.unbound_route.reset();
        next_context.path_pending = false;
        next_context.waypoint = 0; // O()只清G/H，保留全部目标身份。
        c.arrived = true;
        if (!ctx.binding) {
            if (!i.exits)
                return fail(WorldPathError::missing_fact);
            if (std::any_of(i.exits->begin(), i.exits->end(),
                            [&](Position p) { return !within(s.map, p); }))
                return fail(WorldPathError::invalid_input);
            auto &a = c.state.ai.battle.actors.at(i.actor);
            if (std::find(i.exits->begin(), i.exits->end(), cell) != i.exits->end()) {
                a.control.queue.push_back({0, cell.x * 100 + 50, cell.y * 100});
                a.control.queue.push_back({26});
                c.scheduled_exit = true;
            } else {
                if (!path_transition(c.state, i.actor, old.kind == ActorKind::human ? 5 : 17))
                    return fail(WorldPathError::preparation_failed);
                a.control.queue.push_back(old.kind == ActorKind::human ? LegacyActorControl{10, 0}
                                                                       : LegacyActorControl{8, 7});
            }
            return {WorldPathError::none, std::move(c)};
        }
        const auto f = c.state.facilities.find(ctx.binding->instance_id.value);
        if (f == c.state.facilities.end() ||
            !(f->second.placement.instance_id == ctx.binding->instance_id) ||
            f->second.placement.definition_id != ctx.binding->definition_id)
            return fail(WorldPathError::missing_fact);
        const auto facility = f->second; // 回调/结算可能替换整个候选，不能保留引用。
        if (i.facility_consumer) {
            std::optional<WorldPathFacilityCandidate> domain;
            try {
                domain = i.facility_consumer(c.state,
                                             {i.actor, *ctx.binding, old.kind == ActorKind::human});
            } catch (...) {
                return fail(WorldPathError::invalid_callback);
            }
            if (!domain)
                return fail(WorldPathError::preparation_failed);
            if (!valid_path_callback(c.state, domain->state, i.actor) ||
                domain->state.actors.at(i.actor).journey ||
                domain->state.actors.at(i.actor).unbound_route ||
                domain->state.actors.at(i.actor).path_pending ||
                !(domain->state.actors.at(i.actor).visits.last_visited_instance ==
                  std::optional<BuildingId>(ctx.binding->instance_id)))
                return fail(WorldPathError::invalid_callback);
            c.state = std::move(domain->state);
            c.ground_effect20 = domain->ground_effect20;
        } else {
            // 有界普通a(m,o)：装备抽选、递归救援和物体交付必须用真实领域回调。
            if (old.object_slot == -2 ||
                (old.object_slot >= 0 && (facility.category == 1 || facility.category == 7)) ||
                facility.detail == 1 || facility.detail == 4 || facility.detail == 5)
                return fail(WorldPathError::missing_domain);
            auto stats = c.state.actors.at(i.actor).visits;
            if (old.kind == ActorKind::human) {
                const auto spending = c.state.human_spending.find(old.definition);
                if (spending == c.state.human_spending.end())
                    return fail(WorldPathError::missing_fact);
                stats.legacy_actor_total = spending->second;
            }
            stats.current_month_facility_sales = facility.sales;
            // 原N>=0只在类别1/7交付；进入旅馆等普通设施仍保留携带物，不需物品目录。
            const auto resolved = prepare_resolved_arrival(
                {{i.actor, facility.placement.instance_id, facility.placement.definition_id,
                  facility.kind, facility.category, facility.detail,
                  old.kind == ActorKind::human ? 0 : 1, old.control.flags, old.object_slot,
                  s.month_index, facility.price},
                 stats,
                 {},
                 {},
                 false});
            if (!resolved.candidate)
                return fail(WorldPathError::preparation_failed);
            const auto &arrival = resolved.candidate->arrival;
            auto visits = arrival.state;
            c.state.facilities.at(ctx.binding->instance_id.value).sales =
                visits.current_month_facility_sales;
            if (old.kind == ActorKind::human)
                c.state.human_spending.at(old.definition) = visits.legacy_actor_total;
            visits.current_month_facility_sales = visits.legacy_actor_total = 0;
            c.state.actors.at(i.actor).visits = visits;
            const auto cash = arrival.cash_income;
            if (cash > 0) {
                auto &ai = c.state.ai;
                if (ai.next_cash_id == 0 ||
                    ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
                    ai.accounting.post_cash({ai.next_cash_id, ai.period, CashCategory::facilities,
                                             CashDirection::income, cash}) != AccountingError::none)
                    return fail(WorldPathError::preparation_failed);
                ++ai.next_cash_id;
            }
            if (old.kind == ActorKind::human) {
                auto &a = c.state.ai.battle.actors.at(i.actor);
                const int mode = (a.control.flags & 256U) ? 2 : 0;
                a.control.flags &= ~256U;
                const auto use = prepare_world_facility_use(
                    c.state, {i.actor, mode, i.use_world_target, i.use_direction_ticket, i.draw,
                              i.use_direction_target});
                if (!use.state)
                    return fail(WorldPathError::preparation_failed);
                c.state = *use.state;
                c.ground_effect20 = use.ground_effect20;
            }
        }
        if (old.kind == ActorKind::monster) {
            c.state.ai.battle.actors.at(i.actor).control.queue.clear(); // 原w，不是c状态重置。
            if (facility.category == 3) {
                c.path_returned_true = true;
                c.delete_instance = old.control.state == 0;
            } else if (facility.category == 6 && facility.detail == 3) {
                const auto use =
                    prepare_world_facility_use(c.state, {i.actor, 0, i.use_world_target, {}});
                if (!use.state)
                    return fail(WorldPathError::preparation_failed);
                c.state = *use.state;
                c.ground_effect20 = use.ground_effect20;
            } else {
                if (!path_transition(c.state, i.actor, 17))
                    return fail(WorldPathError::preparation_failed);
                c.state.ai.battle.actors.at(i.actor).control.queue.push_back({8, 7});
            }
        }
        return {WorldPathError::none, std::move(c)};
    }
    const auto waypoint_cell = route->steps[ctx.waypoint];
    const auto &tile = s.map.cells[index(s.map, waypoint_cell)];
    int direction = 0;
    if (tile.legacy_state == 6 || tile.legacy_state == 7) {
        if (!tile.facility)
            return fail(WorldPathError::missing_fact);
        const auto found = i.definition_directions.find(tile.facility->definition_id);
        if (found == i.definition_directions.end())
            return fail(WorldPathError::missing_fact);
        direction = found->second;
    }
    const auto waypoint = character_waypoint(waypoint_cell, tile.legacy_state, direction);
    if (!waypoint.target)
        return fail(WorldPathError::invalid_input);
    const auto step = advance_character_motion({old.position.x, old.position.z}, *waypoint.target,
                                               old.control.flags);
    if (!step.step)
        return fail(WorldPathError::invalid_input);
    auto &a = c.state.ai.battle.actors.at(i.actor);
    a.position.x = step.step->position.x;
    a.position.z = step.step->position.z;
    c.moved = a.position.x != old.position.x || a.position.z != old.position.z;
    if (step.step->velocity)
        c.state.actors.at(i.actor).horizontal_velocity = *step.step->velocity;
    if (step.step->waypoint_overlap && ctx.waypoint + 1 < route->steps.size()) {
        ++c.state.actors.at(i.actor).waypoint;
        c.advanced_waypoint = true;
    }
    return {WorldPathError::none, std::move(c)};
}
} // namespace ark::simulation::rules
