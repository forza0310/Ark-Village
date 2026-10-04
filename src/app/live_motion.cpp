// Real old-s path/ground/FIFO integration for the currently populated normal village domains.
#include "ark/people/ai_perception.hpp"
#include "initial_ai_internal.hpp"
#include <algorithm>

namespace ark::app {
namespace {
people::TownBounds town_bounds() {
    const auto b = startup_data().build_bounds;
    return {b.min_x - 1, b.max_x + 1, b.min_y - 1, b.max_y + 1};
}
bool move_area(const world::RouteMap &map, const LifeActorState &s) {
    constexpr world::Cell offsets[]{{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    std::array<people::MoveAreaProbe, 4> probes;
    for (std::size_t n = 0; n < 4; ++n) {
        // Source half-grid truncation precedes integer /2, including just outside map edges.
        const world::Cell cell{static_cast<int>((s.position.x + offsets[n].x) / 50) / 2,
                               static_cast<int>((s.position.z + offsets[n].y) / 50) / 2};
        auto &p = probes[n];
        p.cell_in_map = map.contains(cell);
        if (p.cell_in_map) {
            const auto &tile = map.cells[map.index(cell)];
            p.legacy_surface = static_cast<int>(tile.category); // i.g, not the display sprite.
            p.logical_state = tile.legacy_state;
        }
    }
    return people::prepare_move_area(s.control.state, false, probes)->allowed;
}
} // namespace
InitialAiError InitialAiSession::cleanup(InitialAiState &s) const {
    const auto r = people::prepare_actor_cleanup(people::ActorKind::human, s.control.flags);
    if (!r)
        return InitialAiError::preparation_failed;
    if (s.destination_binding &&
        world::arrival_matches(map_, *s.destination_binding, s.cached_cell)) {
        auto &occupants = s.facilities.at(s.destination_binding->instance).occupants;
        const auto found = std::find(occupants.begin(), occupants.end(), s.actor);
        if (found != occupants.end())
            occupants.erase(found);
    }
    s.control.flags = r->flags;
    const auto changed = people::prepare_actor_state_transition(
        {s.control, true, r->state, s.baseline, 0, {}, false, {}});
    if (!changed)
        return InitialAiError::preparation_failed;
    s.control = changed->control;
    s.baseline = changed->baseline;
    s.counters.state = 0;
    if (r->waiting_updates)
        s.control.queue.push_back({1, r->waiting_updates, 0});
    s.control.queue.push_back({8, r->activity});
    s.active_facility.reset();
    return InitialAiError::none;
}
InitialAiError InitialAiSession::live_decision(InitialAiState &s, RandomStream *random) const {
    if (!people::world_cell(s.position) || s.control.state < 0 || s.control.state > 20)
        return InitialAiError::invalid_input;
    const auto prefix = people::prepare_actor_decision_prefix(s.control.flags, 0, 0, s.hp.target,
                                                              s.stats.combat[0]);
    if (!prefix)
        return InitialAiError::preparation_failed;
    s.control.flags = prefix->flags;
    s.move_area_before = move_area(map_, s);
    const bool inside = people::inside_town(s.cached_cell, town_bounds());
    // This empty event/task F has no side effects and cannot succeed. When those owners are
    // populated, move F into the researched P/c5 positions; do not feed live rosters here.
    people::EventGateInput gate;
    gate.actor.flags = s.control.flags;
    gate.actor.state_counter = s.counters.state;
    gate.actor.in_move_area = s.move_area_before;
    gate.actor.inside_town = inside;
    gate.cell = s.cached_cell;
    gate.cell_in_map = map_.contains(s.cached_cell);
    const auto event = people::prepare_event_gate(gate);
    if (!event.candidate)
        return InitialAiError::preparation_failed;
    if (event.candidate->ready)
        return InitialAiError::unsupported_branch;
    if (s.control.state == 14)
        return decision(s, {});
    if (s.control.state == 5)
        return live_idle(s, random); // Never fall through to P after a c0 reselection.
    if (s.control.state != 0)
        return InitialAiError::none;
    const auto probe = live_spawn(s, random); // Original L precedes P, even in state0.
    if (probe != InitialAiError::none || s.error != InitialAiError::none)
        return probe;
    const auto *route = s.journey         ? &s.journey->route
                        : s.unbound_route ? &*s.unbound_route
                                          : nullptr;
    if (!route || route->steps.empty() || !map_.contains(s.cached_cell))
        return InitialAiError::none;
    if (s.cached_cell == s.destination) {
        if (s.journey) {
            if (!world::arrival_matches(map_, s.journey->binding, s.cached_cell))
                return cleanup(s);
            const auto arrived = arrive(s);
            if (arrived == InitialAiError::none)
                s.waypoint = 0;
            return arrived;
        }
        s.unbound_route.reset();
        s.waypoint = 0;
        const auto &exits = startup_data().spawn_points;
        if (std::find(exits.begin(), exits.end(), s.cached_cell) != exits.end()) {
            s.control.queue.push_back({0, s.cached_cell.x * 100 + 50, s.cached_cell.y * 100});
            s.control.queue.push_back({26});
        } else {
            const auto changed = people::prepare_actor_state_transition(
                {s.control, true, 5, s.baseline, 0, {}, false, {}});
            if (!changed)
                return InitialAiError::preparation_failed;
            s.control = changed->control;
            s.baseline = changed->baseline;
            s.counters.state = 0;
            s.control.queue.push_back({10, 0});
        }
        return InitialAiError::none;
    }
    if (s.route_revision != layout_revision_) {
        world::SearchLimits limits;
        limits.reverse_equal_cost = true;
        const auto field = world::search(map_, s.cached_cell, limits);
        if (!field.field)
            return InitialAiError::preparation_failed;
        auto refreshed = world::trace(*field.field, s.destination);
        if (refreshed.error != world::RouteError::none)
            return cleanup(s);
        if (s.journey)
            s.journey->route = std::move(refreshed);
        else
            s.unbound_route = std::move(refreshed);
        s.waypoint = 0;
        s.route_revision = layout_revision_;
        route = s.journey ? &s.journey->route : &*s.unbound_route;
        if (route->steps.empty())
            return InitialAiError::none;
    }
    if (s.waypoint >= route->steps.size())
        return InitialAiError::invalid_input;
    // Construction cannot turn an unbound destination into a shop. Path identity remains original.
    const auto cell = route->steps[s.waypoint];
    if (!map_.contains(cell))
        return InitialAiError::invalid_input;
    const auto &tile = map_.cells[map_.index(cell)];
    const auto motion = people::advance_motion(
        s.position, people::waypoint(cell, tile.legacy_state, tile.direction), s.control.flags);
    s.position = motion.position;
    if (motion.waypoint_overlap && s.waypoint + 1 < route->steps.size())
        ++s.waypoint;
    return InitialAiError::none;
}
InitialAiError InitialAiSession::live_wander(InitialAiState &s, RandomStream *random) const {
    people::ActorWanderInput input;
    input.parameter = s.control.queue.front()[1];
    input.actor = s.cached_cell;
    input.width = map_.width;
    input.height = map_.height;
    for (int y = 0; y < map_.height; ++y)
        for (int x = 0; x < map_.width; ++x)
            input.cells.push_back({map_.cells[map_.index({x, y})].legacy_state,
                                   people::inside_town({x, y}, town_bounds()), 0});
    // A zero-ticket dry preparation discovers legal neighbours only; no state/RNG is consumed.
    input.tickets = {0, 0, 0, 0, 0, 0, 0};
    const auto preview = people::prepare_actor_wander(input);
    if (!preview)
        return InitialAiError::preparation_failed;
    input.tickets.clear();
    const auto draw = [&](int bound) -> bool {
        if (!random) {
            input.tickets.push_back(0); // Explicit ticket-driven fixture, not normal RNG policy.
            return true;
        }
        const auto result = random->draw(bound);
        if (result.error != RandomError::none)
            return false;
        input.tickets.push_back(result.ticket);
        return true;
    };
    if (preview->cells.empty()) {
        if (!draw(100))
            return InitialAiError::preparation_failed;
    } else
        for (const auto bound : {static_cast<int>(preview->cells.size()), 80, 80, 100, 100, 4, 20})
            if (!draw(bound))
                return InitialAiError::preparation_failed;
    const auto planned = people::prepare_actor_wander(input);
    if (!planned)
        return InitialAiError::preparation_failed;
    s.control.queue.erase(s.control.queue.begin());
    s.control.queue.insert(s.control.queue.end(), planned->append.begin(), planned->append.end());
    return InitialAiError::none;
}
InitialAiError InitialAiSession::live_tail(InitialAiState &s) const {
    people::ActorRetentionInput input;
    input.state = s.retention;
    input.state.state = s.control.state;
    input.state.flags = s.control.flags;
    input.old_cell_inside_town = people::inside_town(s.cached_cell, town_bounds());
    const auto cell = people::world_cell(s.position);
    if (!cell)
        return InitialAiError::invalid_input;
    s.cached_cell = *cell; // Original d projection; never performed in c or FIFO motion.
    const auto &exits = startup_data().spawn_points;
    input.at_spawn_after_projection = std::find(exits.begin(), exits.end(), *cell) != exits.end();
    input.area_before = s.move_area_before;
    input.route_cells = s.journey         ? s.journey->route.steps.size()
                        : s.unbound_route ? s.unbound_route->steps.size()
                                          : 0;
    input.reported_hp = s.control.action == 7 ? 0 : s.hp.target;
    const auto retention = people::prepare_actor_retention(input);
    if (!retention)
        return InitialAiError::preparation_failed;
    for (const auto request : retention->requests) {
        if (request == people::ActorRetentionRequest::cleanup) {
            const auto result = cleanup(s);
            if (result != InitialAiError::none)
                return result;
        } else if (request == people::ActorRetentionRequest::clear_path) {
            s.journey.reset();
            s.unbound_route.reset();
            s.waypoint = 0;
        } else if (request == people::ActorRetentionRequest::mark_escape32768)
            s.control.flags |= 32768U;
        else if (request == people::ActorRetentionRequest::reset_action)
            s.control.action = s.control.action_counter = 0;
        else if (request == people::ActorRetentionRequest::assign_hp1) {
            const auto hp = people::prepare_hp_assignment(s.hp, 1);
            if (!hp.candidate)
                return InitialAiError::preparation_failed;
            s.hp = *hp.candidate;
        }
    }
    s.retention = retention->state;
    s.removed = retention->delete_instance;
    return InitialAiError::none;
}
} // namespace ark::app
