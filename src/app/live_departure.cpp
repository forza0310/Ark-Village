// Current-owner o(activity), adapted from maintained WORLD_DEPARTURE. No alternate world or RNG.
#include "ark/people/ai_perception.hpp"
#include "initial_ai_internal.hpp"
#include <limits>

namespace ark::app {
InitialAiError InitialAiSession::live_depart(InitialAiState &s, int activity,
                                             const InitialAiTickets &tickets,
                                             std::mt19937 *random) const {
    if (s.control.queue.empty() || s.control.queue.front()[0] != 8 || activity < 0 || activity > 8)
        return InitialAiError::invalid_input;
    // Control8 consumes itself BEFORE o; normal failure keeps G/H cleanup and replaces FIFO via r.
    s.control.queue.erase(s.control.queue.begin());
    s.journey.reset();
    s.unbound_route.reset();
    s.waypoint = 0;
    const auto bounds = startup_data().build_bounds;
    people::TownBounds town{bounds.min_x - 1, bounds.max_x + 1, bounds.min_y - 1, bounds.max_y + 1};
    world::SearchLimits limits;
    limits.reverse_equal_cost = true;
    if (activity == 6 && !people::inside_town(s.cached_cell, town))
        limits.max_expanded_cost = 500;
    const auto field = world::search(map_, s.cached_cell, limits);
    if (!field.field)
        return InitialAiError::preparation_failed;
    people::ActivityCandidateInput input;
    input.legacy_activity = activity;
    input.town = town;
    input.last_visited_instance = s.visits.last_visited_instance;
    for (const auto &c : map_.cells)
        input.cell_definition_ids.push_back(c.definition_id);
    for (const auto &d : startup_data().definitions)
        input.definitions.push_back({d.id, d.activity_category, d.economy.attributes[2].first});
    for (const auto id : instance_order_)
        input.instances.push_back(
            {id, s.facilities.at(id).placement.definition_id, s.facilities.at(id).phase});
    const auto collected = people::collect_activity_candidates(*field.field, input);
    if (!collected.snapshot)
        return InitialAiError::preparation_failed;
    const auto &snapshot = *collected.snapshot;
    const auto draw = [&](std::int64_t bound, int fixed) -> std::optional<int> {
        if (bound <= 0 || bound > std::numeric_limits<int>::max())
            return {};
        const int v =
            random ? std::uniform_int_distribution<int>(0, static_cast<int>(bound) - 1)(*random)
                   : fixed;
        return v >= 0 && v < bound ? std::optional<int>(v) : std::nullopt;
    };
    std::optional<world::Cell> goal;
    std::optional<int> category;
    // This owner's populated domains currently contain one human and no tasks/objects/events.
    // Keep priority evaluation real, with those actual empty lists, before ordinary selection.
    people::DepartureOverrideInput priority;
    priority.self = s.actor;
    priority.flags = s.control.flags;
    priority.activity = activity;
    for (const auto &c : snapshot.cells)
        priority.reachable.push_back(c.position);
    const auto override = people::prepare_departure_override(priority);
    if (!override)
        return InitialAiError::preparation_failed;
    goal = override->destination;
    if (!snapshot.cells.empty() && !goal) {
        if (activity == 0 || activity == 6) {
            people::ActivityChoiceInput choice;
            choice.legacy_activity = activity;
            choice.available_category_counts = snapshot.category_counts;
            std::copy(s.visits.legacy_visit_counts.begin(), s.visits.legacy_visit_counts.end(),
                      choice.legacy_visit_counts.begin());
            choice.legacy_flags = s.control.flags;
            const auto plan = people::plan_activity_categories(choice);
            if (!plan.plan)
                return InitialAiError::preparation_failed;
            category = plan.plan->forced_category;
            if (!category && plan.plan->total_weight > 0) {
                const auto ticket = draw(plan.plan->total_weight, tickets.category);
                if (!ticket)
                    return InitialAiError::invalid_input;
                std::vector<std::int64_t> weights;
                for (const auto &option : plan.plan->options)
                    weights.push_back(option.weight);
                const auto selected = people::select_weighted_ticket(weights, *ticket);
                if (!selected.index)
                    return InitialAiError::preparation_failed;
                category = plan.plan->options[*selected.index].category;
            }
        } else if (activity == 5)
            category = 3;
        else if (activity == 4) {
            for (const auto &c : snapshot.cells)
                if (c.definition.legacy_category == 2 && c.cost &&
                    (!goal || *c.cost < *field.field->distances[map_.index(*goal)]))
                    goal = c.position;
        } else if (activity == 7) {
            const auto ticket = draw(snapshot.cells.size(), tickets.facility);
            if (!ticket)
                return InitialAiError::invalid_input;
            goal = snapshot.cells[*ticket].position;
        } else if (activity != 2 && activity != 3) {
            s.pending_activity = activity;
            s.error = InitialAiError::unsupported_branch;
            return InitialAiError::none;
        }
        if (category == 3) {
            // D is shared-definition evidence, not inferred from absence of a house on the map.
            if (!s.home) {
                s.pending_category = 3;
                s.handoff = LifeHandoff::home_projection;
                s.error = InitialAiError::unsupported_branch;
                return InitialAiError::none;
            }
            if (s.home->state < 0 || !map_.contains(s.home->cell))
                return InitialAiError::invalid_input;
            if (s.home->state == 1) {
                const auto t = draw(100, tickets.facility);
                if (!t)
                    return InitialAiError::invalid_input;
                const auto &tile = map_.cells[map_.index(s.home->cell)];
                if (*t < 90 && s.cached_cell.x != s.home->cell.x &&
                    s.cached_cell.y != s.home->cell.y && tile.facility &&
                    s.facilities.at(tile.facility->instance).phase != 0)
                    goal = s.home->cell;
            }
            if (!goal) {
                const auto &exits = startup_data().spawn_points;
                if (exits.size() < 2)
                    return InitialAiError::invalid_input;
                for (std::size_t n = 0; n < 2; ++n)
                    if (s.cached_cell == exits[n])
                        goal = exits[1 - n];
                if (!goal) {
                    const auto t = draw(exits.size(), tickets.facility);
                    if (!t)
                        return InitialAiError::invalid_input;
                    goal = exits[*t];
                }
            }
        } else if (category == 4) {
            if (snapshot.category_counts[4] > 0) {
                const auto t = draw(snapshot.category_counts[4], tickets.facility);
                if (!t)
                    return InitialAiError::invalid_input;
                const auto selected = people::select_counted_category_four(snapshot, *t);
                if (!selected.target)
                    return InitialAiError::preparation_failed;
                goal = selected.target->cell.position;
            }
        } else if (category == -1) {
            for (int attempt = 0; attempt < 6; ++attempt) {
                const auto t = draw(snapshot.cells.size(), tickets.facility);
                if (!t)
                    return InitialAiError::invalid_input;
                const auto p = snapshot.cells[*t].position;
                if (attempt == 5 || p.y >= town.bottom + 2) {
                    goal = p;
                    break;
                }
            }
        } else if (category) {
            std::int64_t weight{};
            for (const auto &c : snapshot.cells)
                if (c.instance && c.instance->legacy_phase == 1 &&
                    c.definition.legacy_category == *category)
                    weight += c.definition.definition_charm;
            if (weight > 0) {
                const auto t = draw(weight, tickets.facility);
                if (!t)
                    return InitialAiError::invalid_input;
                const auto selected = people::select_snapshot_facility(snapshot, *category, *t);
                if (!selected.target)
                    return InitialAiError::preparation_failed;
                goal = selected.target->goal.position;
            }
        }
    }
    if (goal) {
        auto route = world::trace(*field.field, *goal);
        if (route.error == world::RouteError::none) {
            s.destination = *goal;
            s.destination_binding.reset();
            const auto &tile = map_.cells[map_.index(*goal)];
            if (tile.facility) {
                const auto &d = initial_definition(tile.definition_id);
                if (d.activity_category != 4 &&
                    ((d.activity_category != 1 && d.activity_category != 2) ||
                     (d.activity_detail != 0 && d.activity_detail != 1))) {
                    s.error = InitialAiError::unsupported_branch;
                    s.pending_definition = d.id;
                    s.handoff = LifeHandoff::facility_consumer;
                    return InitialAiError::none;
                }
                people::FacilityDeparture journey;
                journey.category = d.activity_category;
                journey.binding = {*goal, tile.facility->instance, tile.definition_id};
                s.destination_binding = journey.binding;
                journey.route = std::move(route);
                s.journey = std::move(journey);
            } else
                s.unbound_route = std::move(route);
            const auto &steps = s.journey ? s.journey->route.steps : s.unbound_route->steps;
            if (!steps.empty()) {
                const auto p = steps.front();
                s.control.facing = p.x > s.cached_cell.x   ? 1
                                   : p.x < s.cached_cell.x ? 3
                                   : p.y > s.cached_cell.y ? 0
                                                           : 2;
            }
            s.control.state = 0; // NOT c0: preserve B/C/D/k/l/i and remaining tail.
            s.route_revision = layout_revision_;
            ++s.departures;
            return InitialAiError::none;
        }
    }
    const auto failure = people::prepare_failed_activity(s.control.flags);
    if (failure.expression18) {
        // Platform expression variants have not been delivered to this owner. Preserve the branch.
        s.error = InitialAiError::unsupported_branch;
        s.pending_activity = activity;
        return InitialAiError::none;
    }
    if (failure.delete_instance) {
        s.removed = true;
        return InitialAiError::none;
    }
    s.control.flags = failure.flags;
    return cleanup(s);
}
} // namespace ark::app
