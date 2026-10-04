// Adapted from research d7ca763 prototype/startup_ai.cpp; private owner only.
#include "ark/people/ai_perception.hpp"
#include "initial_ai_internal.hpp"
#include <limits>
namespace ark::app {
// Resolve ordinary priority first, then consume separate category/facility tickets on this map.
InitialAiError InitialAiSession::depart(InitialAiState &s, const InitialAiTickets &tickets,
                                        RandomStream *random) const {
    if (live_)
        return live_depart(s, s.control.queue.front()[1], tickets, random);
    const auto cell = people::world_cell(s.position);
    if (!cell)
        return InitialAiError::invalid_input;
    const auto search = world::search(map_, *cell);
    if (!search.field)
        return InitialAiError::preparation_failed;
    people::ActivityCandidateInput input;
    input.legacy_activity = 0;
    const auto bounds = startup_data().build_bounds;
    input.town = {bounds.min_x - 1, bounds.max_x + 1, bounds.min_y - 1, bounds.max_y + 1};
    input.last_visited_instance = s.visits.last_visited_instance;
    for (const auto &c : map_.cells)
        input.cell_definition_ids.push_back(c.definition_id);
    for (const auto &d : startup_data().definitions)
        input.definitions.push_back({d.id, d.activity_category, d.economy.attributes[2].first});
    for (const auto id : instance_order_)
        input.instances.push_back(
            {id, s.facilities.at(id).placement.definition_id, s.facilities.at(id).phase});
    const auto candidates = people::collect_activity_candidates(*search.field, input);
    if (!candidates.snapshot)
        return InitialAiError::preparation_failed;
    // The projection has no tasks, down actors, objects or encounters. Resolve priority explicitly
    // rather than interpreting all activity0 requests as ordinary in a populated world.
    people::DepartureOverrideInput priority;
    priority.self = s.actor;
    priority.flags = s.control.flags;
    for (const auto &c : candidates.snapshot->cells)
        priority.reachable.push_back(c.position);
    const auto override = people::prepare_departure_override(priority);
    if (!override || override->kind != people::DepartureOverrideKind::ordinary)
        return InitialAiError::unsupported_branch;
    people::FacilityDepartureInput departure_input;
    departure_input.legacy_flags = s.control.flags;
    departure_input.category_ticket = tickets.category;
    departure_input.facility_ticket = tickets.facility;
    std::copy(s.visits.legacy_visit_counts.begin(), s.visits.legacy_visit_counts.end(),
              departure_input.legacy_visit_counts.begin());
    if (random || live_) {
        const auto plan = people::plan_activity_categories({0, candidates.snapshot->category_counts,
                                                            departure_input.legacy_visit_counts,
                                                            s.control.flags});
        if (!plan.plan)
            return InitialAiError::preparation_failed;
        if (!plan.plan->forced_category && plan.plan->total_weight <= 0) {
            if (live_) {
                s.error = InitialAiError::unsupported_branch;
                s.pending_activity = 0;
                return InitialAiError::none;
            }
            return InitialAiError::unsupported_branch;
        }
        int category{};
        if (plan.plan->forced_category) {
            category = *plan.plan->forced_category;
        } else {
            if (plan.plan->total_weight > std::numeric_limits<int>::max())
                return InitialAiError::invalid_input;
            if (random) {
                const auto draw = random->draw(static_cast<int>(plan.plan->total_weight));
                if (draw.error != RandomError::none)
                    return InitialAiError::preparation_failed;
                departure_input.category_ticket = draw.ticket;
            }
            std::vector<std::int64_t> weights;
            for (const auto &option : plan.plan->options)
                weights.push_back(option.weight);
            const auto chosen =
                people::select_weighted_ticket(weights, departure_input.category_ticket);
            if (!chosen.index)
                return InitialAiError::preparation_failed;
            category = plan.plan->options[*chosen.index].category;
        }
        // An original exit/special choice ends this finite preview; never reroll or remove it.
        if (category != 1 && category != 2) {
            if (live_) {
                s.error = InitialAiError::unsupported_branch;
                s.pending_category = category;
                return InitialAiError::none;
            }
            return InitialAiError::unsupported_branch;
        }
        std::int64_t total{};
        for (const auto &cell : candidates.snapshot->cells)
            if (cell.instance && cell.instance->legacy_phase == 1 &&
                cell.definition.legacy_category == category)
                total += cell.definition.definition_charm;
        if (total <= 0 || total > std::numeric_limits<int>::max())
            return InitialAiError::preparation_failed;
        if (random) {
            const auto draw = random->draw(static_cast<int>(total));
            if (draw.error != RandomError::none)
                return InitialAiError::preparation_failed;
            departure_input.facility_ticket = draw.ticket;
        }
    }
    const auto departure =
        people::prepare_facility_departure(*search.field, *candidates.snapshot, departure_input);
    if (!departure.departure)
        return InitialAiError::preparation_failed;
    const auto &d = initial_definition(departure.departure->binding.definition_id);
    if ((d.activity_category != 1 && d.activity_category != 2) ||
        (d.activity_detail != 0 && d.activity_detail != 1)) {
        if (live_) {
            s.error = InitialAiError::unsupported_branch;
            s.pending_definition = d.id;
            return InitialAiError::none;
        }
        return InitialAiError::unsupported_branch;
    }
    s.journey = departure.departure;
    s.waypoint = 0;
    s.route_revision = layout_revision_;
    if (s.journey->legacy_direction)
        s.control.facing = *s.journey->legacy_direction;
    ++s.departures;
    return InitialAiError::none;
}
// The c pass checks entry before motion; inn recovery reads B before the d pass increments it.
InitialAiError InitialAiSession::decision(InitialAiState &s, const InitialAiTickets &) const {
    if (s.control.state == 14 && s.active_facility) {
        if (initial_definition(s.active_facility->definition_id).activity_category == 2 &&
            s.counters.state == 170) {
            const auto hp = people::prepare_hp_change(s.hp, s.stats.combat[0], s.stats.combat[0]);
            if (!hp.candidate)
                return InitialAiError::preparation_failed;
            s.hp = *hp.candidate;
            ++s.recoveries;
        }
        return InitialAiError::none;
    }
    if (s.control.state != 0 || !s.journey)
        return InitialAiError::none;
    // Construction refreshes the route against the live layout without changing the selected
    // target or consuming another choice ticket. The original target binding is revalidated.
    if (live_ && s.route_revision != layout_revision_) {
        const auto cell = people::world_cell(s.position);
        if (!cell || !world::arrival_matches(map_, s.journey->binding, s.journey->binding.cell))
            return InitialAiError::preparation_failed;
        const auto field = world::search(map_, *cell);
        if (!field.field)
            return InitialAiError::preparation_failed;
        auto route = world::trace(*field.field, s.journey->binding.cell);
        if (route.error != world::RouteError::none)
            return InitialAiError::preparation_failed;
        s.journey->route = std::move(route);
        s.waypoint = 0;
        s.route_revision = layout_revision_;
    }
    const auto status = people::inspect_entry(map_, s.journey->binding, s.position, true);
    if (status == people::EntryStatus::ready)
        return arrive(s);
    if (status != people::EntryStatus::not_entered)
        return InitialAiError::preparation_failed;
    const auto &route = s.journey->route.steps;
    if (route.empty() || s.waypoint >= route.size())
        return InitialAiError::preparation_failed;
    const auto cell = route[s.waypoint];
    const auto &tile = map_.cells.at(map_.index(cell));
    const auto target = people::waypoint(cell, tile.legacy_state, tile.direction);
    const auto motion = people::advance_motion(s.position, target, s.control.flags);
    s.position = motion.position;
    if (motion.waypoint_overlap && s.waypoint + 1 < route.size())
        ++s.waypoint;
    return InitialAiError::none;
}
} // namespace ark::app
