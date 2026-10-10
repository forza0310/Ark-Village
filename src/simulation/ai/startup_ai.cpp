#include "ark/simulation/ai/startup_ai.hpp"
#include "ark/simulation/ai/rules/ai_perception.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ark::simulation {
namespace {
const StartupDefinition &definition(int id) {
    const auto &defs = startup_evidence().definitions;
    const auto it =
        std::find_if(defs.begin(), defs.end(), [&](const auto &d) { return d.id == id; });
    if (it == defs.end())
        throw std::invalid_argument("unknown startup definition");
    return *it;
}
} // namespace
StartupAiSession::StartupAiSession(const StartupSession &startup, ref::Position birth)
    : map_(startup_route_map(startup.state().loaded_map)), loaded_(startup.state().loaded_map) {
    const auto &e = startup_evidence();
    if (!startup.state().character || startup.state().event89_count != 1 ||
        startup.state().character->definition_id != e.first_character.definition_id ||
        std::find(e.spawn_points.begin(), e.spawn_points.end(), birth) == e.spawn_points.end() ||
        !startup.state().terrain_edits.empty() ||
        startup.state().facilities.size() != loaded_.instances.size())
        throw std::invalid_argument("requires unchanged actual first-visitor snapshot");
    state_.actor = {static_cast<std::uint64_t>(e.first_character.uid) + 1};
    state_.control.flags = startup.state().character->flags;
    state_.control.queue = {{8, 0}};
    state_.position = {birth.x * 100.0F + 50.0F, birth.y * 100.0F + 50.0F};
    state_.accounting = startup.state().accounting;
    state_.next_cash_id = startup.state().next_cash_id;
    state_.definition = startup_ai_rules().first_definition;
    const auto stats = ref::derive_human_stats(state_.definition, startup_ai_rules().professions);
    if (!stats.candidate || stats.candidate->attributes != e.first_character.attributes ||
        stats.candidate->combat != e.first_character.combat)
        throw std::invalid_argument("source growth projection disagrees with actual first visitor");
    state_.stats = *stats.candidate;
    state_.hp = {
        0, e.first_character.hp[0], e.first_character.hp[1], e.first_character.hp[2], false, 0};
    state_.current_weapon = e.first_character.equipment[0];
    state_.weapon_reselect_counter = e.first_character.weapon_reselect_counter;
    state_.satisfaction = e.first_character.satisfaction;
    std::vector<ref::FacilityPlacement> placements;
    std::vector<ref::NeighbourDefinition> definitions;
    std::vector<ref::Position> roads;
    for (const auto &d : e.definitions)
        definitions.push_back(
            {d.id, static_cast<ref::FacilityShape>(d.shape), d.kind, d.neighbour_effects});
    for (const auto &f : loaded_.instances) {
        const auto id = static_cast<std::uint64_t>(f.legacy_id) + 1;
        const auto &d = definition(f.definition_id);
        ref::FacilityPlacement p{{id},
                                 f.definition_id,
                                 static_cast<ref::FacilityShape>(d.shape),
                                 ref::FacilityOrientation::first,
                                 f.anchor};
        placements.push_back(p);
        state_.facilities.emplace(id, StartupAiFacility{p, {}, {}});
        state_.uses.emplace(f.definition_id, ref::FacilityUseProgress{});
    }
    for (int y = 0; y < loaded_.height; ++y)
        for (int x = 0; x < loaded_.width; ++x)
            if (map_.cells[y * loaded_.width + x].legacy_state == 3)
                roads.push_back({x, y});
    const auto neighbours = ref::derive_facility_neighbourhood(definitions, placements, roads,
                                                               loaded_.width, loaded_.height);
    if (neighbours.error != ref::NeighbourhoodError::none)
        throw std::invalid_argument("invalid actual first-play neighbourhood");
    for (const auto &n : neighbours.facilities)
        neighbourhoods_.emplace(n.instance_id.value, n);
}
const StartupAiState &StartupAiSession::state() const { return state_; }
StartupAiError StartupAiSession::depart(StartupAiState &s, const StartupAiTickets &tickets) const {
    const auto cell = ref::character_world_cell(s.position);
    if (!cell)
        return StartupAiError::invalid_input;
    const auto search = ref::search_legacy_map(map_, *cell);
    if (!search.field)
        return StartupAiError::preparation_failed;
    ref::ActivityCandidateInput input;
    input.legacy_activity = 0;
    input.town = {6, 17, 2, 10};
    input.last_visited_instance = s.visits.last_visited_instance;
    for (const auto &c : loaded_.cells)
        input.cell_definition_ids.push_back(c.definition_id);
    for (const auto &d : startup_evidence().definitions)
        input.definitions.push_back({d.id, d.category, d.definition_charm});
    for (const auto &f : loaded_.instances)
        input.instances.push_back(
            {{static_cast<std::uint64_t>(f.legacy_id) + 1}, f.definition_id, 1});
    const auto candidates = ref::collect_activity_candidates(*search.field, input);
    if (!candidates.snapshot)
        return StartupAiError::preparation_failed;
    // The projection has no tasks, down actors, objects or encounters. Resolve priority explicitly
    // rather than interpreting all activity0 requests as ordinary in a populated world.
    ref::DepartureOverrideInput priority;
    priority.self = s.actor;
    priority.flags = s.control.flags;
    for (const auto &c : candidates.snapshot->cells)
        priority.reachable.push_back(c.position);
    const auto override = ref::prepare_departure_override(priority);
    if (!override || override->kind != ref::DepartureOverrideKind::ordinary)
        return StartupAiError::unsupported_branch;
    ref::FacilityDepartureInput departure_input;
    departure_input.legacy_flags = s.control.flags;
    departure_input.category_ticket = tickets.category;
    departure_input.facility_ticket = tickets.facility;
    std::copy(s.visits.legacy_visit_counts.begin(), s.visits.legacy_visit_counts.end(),
              departure_input.legacy_visit_counts.begin());
    const auto departure =
        ref::prepare_facility_departure(*search.field, *candidates.snapshot, departure_input);
    if (!departure.departure)
        return StartupAiError::preparation_failed;
    const auto &d = definition(departure.departure->binding.definition_id);
    if ((d.category != 1 && d.category != 2) || (d.detail != 0 && d.detail != 1))
        return StartupAiError::unsupported_branch;
    s.journey = departure.departure;
    s.waypoint = 0;
    if (s.journey->legacy_direction)
        s.control.facing = *s.journey->legacy_direction;
    ++s.departures;
    return StartupAiError::none;
}
StartupAiError StartupAiSession::arrive(StartupAiState &s) const {
    const auto binding = s.journey->binding;
    auto &f = s.facilities.at(binding.instance_id.value);
    const auto &d = definition(binding.definition_id);
    auto economy_input =
        ref::neighbourhood_economy_input(neighbourhoods_.at(binding.instance_id.value));
    if (!economy_input)
        return StartupAiError::preparation_failed;
    economy_input->level = s.uses.at(d.id).level;
    const auto values = ref::derive_facility_economy(d.economy, *economy_input);
    if (!values.values || values.values->instance_attributes[0] > std::numeric_limits<int>::max())
        return StartupAiError::preparation_failed;
    ref::ResolvedArrivalInput i;
    i.arrival = {s.actor,
                 binding.instance_id,
                 d.id,
                 d.kind,
                 d.category,
                 d.detail,
                 0,
                 s.control.flags,
                 -1,
                 3,
                 static_cast<int>(values.values->instance_attributes[0])};
    i.statistics = s.visits;
    i.statistics.current_month_facility_sales = f.sales.current_month_facility_sales;
    if (d.detail == 1) {
        std::vector<ref::WeaponChoiceDefinition> catalogue;
        for (const auto &w : startup_ai_rules().weapons)
            catalogue.push_back(w.selection);
        // Only positive A0 is admitted here; later reselect tickets are a separate input chain.
        const auto selected =
            ref::prepare_weapon_choice(catalogue, s.current_weapon, s.weapon_reselect_counter);
        if (!selected.candidate)
            return StartupAiError::preparation_failed;
        i.equipment_id = selected.candidate->weapon_id;
        const auto &w = startup_ai_rules().weapons.at(*i.equipment_id);
        i.equipment_price = w.price;
    }
    const auto arrival = ref::prepare_resolved_arrival(i);
    if (!arrival.candidate)
        return StartupAiError::preparation_failed;
    ref::FacilityUsePlanInput use;
    use.control = s.control;
    use.category = d.category;
    use.detail = d.detail;
    use.definition_wait = d.use_wait;
    const auto plan = ref::prepare_facility_use_plan(use);
    if (!plan.candidate)
        return StartupAiError::preparation_failed;
    if (arrival.candidate->arrival.cash_income > 0 &&
        s.accounting.post_cash({s.next_cash_id++, s.rounds + 1, ref::CashCategory::facilities,
                                ref::CashDirection::income,
                                arrival.candidate->arrival.cash_income}) !=
            ref::AccountingError::none)
        return StartupAiError::preparation_failed;
    s.visits = arrival.candidate->arrival.state;
    f.sales.current_month_facility_sales = s.visits.current_month_facility_sales;
    s.selected_weapon = arrival.candidate->selected_equipment;
    s.control = plan.candidate->control;
    s.counters.state = 0;
    s.active_facility = binding;
    s.journey.reset();
    ++s.arrivals;
    return StartupAiError::none;
}
StartupAiError StartupAiSession::decision(StartupAiState &s, const StartupAiTickets &) const {
    if (s.control.state == 14 && s.active_facility) {
        if (definition(s.active_facility->definition_id).category == 2 && s.counters.state == 170) {
            const auto hp = ref::prepare_hp_change(s.hp, s.stats.combat[0], s.stats.combat[0]);
            if (!hp.candidate)
                return StartupAiError::preparation_failed;
            s.hp = *hp.candidate;
            ++s.recoveries;
        }
        return StartupAiError::none;
    }
    if (s.control.state != 0 || !s.journey)
        return StartupAiError::none;
    const auto status = ref::inspect_facility_entry(map_, s.journey->binding, s.position, true);
    if (status == ref::FacilityEntryStatus::ready)
        return arrive(s);
    if (status != ref::FacilityEntryStatus::not_entered)
        return StartupAiError::preparation_failed;
    const auto &route = s.journey->route.steps;
    if (route.empty() || s.waypoint >= route.size())
        return StartupAiError::preparation_failed;
    const auto cell = route[s.waypoint];
    const auto &tile = loaded_.cells.at(static_cast<std::size_t>(cell.y * loaded_.width + cell.x));
    const auto target =
        ref::character_waypoint(cell, tile.legacy_state, definition(tile.definition_id).direction);
    if (!target.target)
        return StartupAiError::preparation_failed;
    const auto motion = ref::advance_character_motion(s.position, *target.target, s.control.flags);
    if (!motion.step)
        return StartupAiError::preparation_failed;
    s.position = motion.step->position;
    if (motion.step->waypoint_overlap && s.waypoint + 1 < route.size())
        ++s.waypoint;
    return StartupAiError::none;
}
StartupAiError StartupAiSession::exit(StartupAiState &s, const StartupAiTickets &tickets) const {
    if (!s.active_facility)
        return StartupAiError::preparation_failed;
    const auto binding = *s.active_facility;
    auto &f = s.facilities.at(binding.instance_id.value);
    const auto &d = definition(binding.definition_id);
    const auto cell = ref::character_world_cell(s.position);
    ref::FacilityServiceExitInput i;
    i.actor = s.actor;
    i.control = s.control;
    i.binding_valid = cell && ref::arrival_binding_matches(map_, binding, *cell);
    i.map = map_;
    i.facility = f.placement;
    i.position = s.position;
    i.category = d.category;
    i.detail = d.detail;
    i.progress = s.uses.at(d.id);
    i.upgrade_uses = d.economy.upgrade_uses;
    i.occupants = f.occupants;
    auto economy_input =
        ref::neighbourhood_economy_input(neighbourhoods_.at(binding.instance_id.value));
    if (!economy_input)
        return StartupAiError::preparation_failed;
    const auto values = ref::derive_facility_economy(d.economy, *economy_input);
    if (!values.values || values.values->instance_attributes[1] > std::numeric_limits<int>::max())
        return StartupAiError::preparation_failed;
    i.satisfaction = {s.actor,
                      binding.instance_id,
                      d.id,
                      s.satisfaction,
                      startup_evidence().first_character.job_satisfaction_thresholds,
                      static_cast<int>(values.values->instance_attributes[1]),
                      tickets.satisfaction};
    i.effects = d.exit_effects;
    i.effect_ticket = tickets.attribute;
    i.equipment.old_weapon = s.current_weapon;
    i.equipment.new_weapon = s.selected_weapon.value_or(s.current_weapon);
    const auto result = ref::prepare_facility_service_exit(i);
    if (!result.candidate || result.candidate->cleanup)
        return StartupAiError::preparation_failed;
    s.position = result.candidate->position->position;
    s.uses[d.id] = result.candidate->shared_use->progress;
    f.occupants = result.candidate->occupants;
    s.control = result.candidate->control;
    s.counters.state = 0;
    if (result.candidate->satisfaction) {
        s.satisfaction = result.candidate->satisfaction->satisfaction;
        s.popularity_requests.push_back(result.candidate->satisfaction->popularity);
    }
    s.active_facility.reset();
    ++s.completions;
    return StartupAiError::none;
}
StartupAiError StartupAiSession::execution(StartupAiState &s,
                                           const StartupAiTickets &tickets) const {
    s.counters.action = s.control.action_counter;
    s.counters.alternate = s.control.alternate_counter;
    const auto counters = ref::advance_actor_counters(s.counters);
    const auto effects = ref::advance_actor_effects(s.effects);
    const auto hp = ref::advance_hp_animation(s.hp);
    if (!counters || !effects.candidate || !hp.candidate)
        return StartupAiError::preparation_failed;
    s.counters = *counters;
    s.effects = effects.candidate->state;
    s.hp = *hp.candidate;
    s.control.action_counter = s.counters.action;
    s.control.alternate_counter = s.counters.alternate;
    for (int budget = 0; budget < 128; ++budget) {
        const auto prefix = ref::prepare_local_control_prefix(s.control);
        if (!prefix.candidate)
            return StartupAiError::preparation_failed;
        s.control = prefix.candidate->state;
        if (prefix.candidate->flow != ref::ActorControlFlow::delegated)
            return StartupAiError::none;
        const auto command = s.control.queue.front();
        if (command[0] == 8) {
            if (command[1] != 0)
                return StartupAiError::unsupported_branch;
            const auto error = depart(s, tickets);
            if (error != StartupAiError::none)
                return error;
            const auto submitted =
                ref::prepare_local_control_prefix(s.control, {std::nullopt, true});
            if (!submitted.candidate)
                return StartupAiError::preparation_failed;
            s.control = submitted.candidate->state;
            return StartupAiError::none; // Successful8 early stop, pending tail remains.
        }
        if (command[0] == 24) {
            const auto error = exit(s, tickets);
            if (error != StartupAiError::none)
                return error;
            continue;
        }
        s.control.queue.erase(s.control.queue.begin());
        if (command[0] == 21) {
            if (!s.active_facility)
                return StartupAiError::preparation_failed;
            s.facilities.at(s.active_facility->instance_id.value).occupants.push_back(s.actor);
            ++s.occupations;
        } else if (command[0] == 19) {
            if (command[2] < 0 || command[2] >= 6)
                return StartupAiError::invalid_input;
            auto &value = s.definition.extra[command[2]];
            const auto sum = static_cast<std::int64_t>(value) + command[3];
            if (sum < std::numeric_limits<int>::min() || sum > std::numeric_limits<int>::max())
                return StartupAiError::preparation_failed;
            value = static_cast<int>(sum);
            // a.e.b(attribute,delta) only writes z; it does NOT eagerly recalculate x/w.
            // A subsequent equip/job/growth recalculation observes the accumulated value.
            s.effects.display.insert(s.effects.display.begin(),
                                     {13, -command[1], command[2], command[3]});
            ++s.attribute_commits;
        } else if (command[0] == 28) {
            const auto commit = ref::prepare_equipment_commit(command);
            if (!commit ||
                static_cast<std::size_t>(commit->equipment) >= startup_ai_rules().weapons.size())
                return StartupAiError::preparation_failed;
            s.definition.equipment[0] = startup_ai_rules().weapons[commit->equipment].combat;
            const auto stats =
                ref::derive_human_stats(s.definition, startup_ai_rules().professions);
            if (!stats.candidate)
                return StartupAiError::preparation_failed;
            s.stats = *stats.candidate;
            s.current_weapon = commit->equipment;
            s.weapon_reselect_counter = commit->reselect_counter;
            ++s.equipment_commits;
        } else if (command[0] == 18 || command[0] == 27) {
            // Keep visual requests explicit: no invented language-variant count or pixel payload.
            s.presentation_requests.push_back(command);
        } else
            return StartupAiError::unsupported_branch;
    }
    return StartupAiError::preparation_failed;
}
StartupAiError StartupAiSession::round(const StartupAiTickets &tickets) {
    auto next = state_;
    ref::AiScheduleInput schedule;
    schedule.rosters[0] = {next.actor.value};
    for (const auto &f : loaded_.instances)
        schedule.rosters[5].push_back(static_cast<std::uint64_t>(f.legacy_id) + 1);
    StartupAiError error = StartupAiError::none;
    const auto candidate = ref::prepare_ai_schedule(schedule, [&](const auto &v, const auto &) {
        if (v.phase == ref::AiSchedulePhase::human_decision)
            error = decision(next, tickets);
        else if (v.phase == ref::AiSchedulePhase::human_execution)
            error = execution(next, tickets);
        ref::AiScheduleResponse response;
        response.accepted = error == StartupAiError::none;
        return response;
    });
    if (!candidate.candidate)
        return error == StartupAiError::none ? StartupAiError::preparation_failed : error;
    ++next.rounds;
    state_ = std::move(next);
    return StartupAiError::none;
}
} // namespace ark::simulation
