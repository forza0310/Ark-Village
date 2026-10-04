// Adapted from research d7ca763 prototype/startup_ai.cpp; private owner only.
#include "initial_ai_internal.hpp"
#include <limits>
namespace ark::app {
// Arrival posts income once and installs the use queue; weapon price replaces service price.
InitialAiError InitialAiSession::arrive(InitialAiState &s) const {
    const auto binding = s.journey->binding;
    auto &f = s.facilities.at(binding.instance);
    const auto &d = initial_definition(binding.definition_id);
    auto economy_input = facilities::with_neighbours({}, neighbourhoods_.at(binding.instance));
    economy_input.level = s.uses.at(d.id).level;
    const auto values = facilities::derive_economy(d.economy, economy_input);
    if (values.instance[0] > std::numeric_limits<int>::max())
        return InitialAiError::preparation_failed;
    facilities::ResolvedArrivalInput i;
    i.arrival = {s.actor,
                 binding.instance,
                 d.id,
                 d.kind,
                 d.activity_category,
                 d.activity_detail,
                 0,
                 s.control.flags,
                 -1,
                 3,
                 static_cast<int>(values.instance[0])};
    i.statistics = s.visits;
    i.statistics.current_month_facility_sales = f.sales.current_month_facility_sales;
    if (d.activity_detail == 1) {
        std::vector<people::WeaponChoiceDefinition> catalogue;
        for (const auto &w : initial_ai_rules().weapons)
            catalogue.push_back(w.selection);
        // Only positive A0 is admitted here; later reselect tickets are a separate input chain.
        const auto selected =
            people::prepare_weapon_choice(catalogue, s.current_weapon, s.weapon_reselect_counter);
        if (!selected.candidate)
            return InitialAiError::preparation_failed;
        i.equipment_id = selected.candidate->weapon_id;
        const auto &w = initial_ai_rules().weapons.at(*i.equipment_id);
        i.equipment_price = w.price;
    }
    const auto arrival = facilities::prepare_resolved_arrival(i);
    if (!arrival.candidate)
        return InitialAiError::preparation_failed;
    facilities::FacilityUsePlanInput use;
    use.control = s.control;
    use.category = d.activity_category;
    use.detail = d.activity_detail;
    use.definition_wait = initial_ai_rules().services.at(d.id).wait;
    const auto plan = facilities::prepare_facility_use_plan(use);
    if (!plan.candidate)
        return InitialAiError::preparation_failed;
    if (arrival.candidate->arrival.cash_income > 0 &&
        s.accounting.post_cash({s.next_cash_id++, s.rounds + 1, economy::CashCategory::facilities,
                                economy::CashDirection::income,
                                arrival.candidate->arrival.cash_income}) !=
            economy::CashError::none)
        return InitialAiError::preparation_failed;
    s.visits = arrival.candidate->arrival.state;
    f.sales.current_month_facility_sales = s.visits.current_month_facility_sales;
    s.selected_weapon = arrival.candidate->selected_equipment;
    s.control = plan.candidate->control;
    s.counters.state = 0;
    s.active_facility = binding;
    s.journey.reset();
    ++s.arrivals;
    return InitialAiError::none;
}
// Validate exit and delayed tail before applying shared use, release and satisfaction together.
InitialAiError InitialAiSession::exit(InitialAiState &s, const InitialAiTickets &tickets,
                                      std::mt19937 *random) const {
    if (!s.active_facility)
        return InitialAiError::preparation_failed;
    const auto binding = *s.active_facility;
    auto &f = s.facilities.at(binding.instance);
    const auto &d = initial_definition(binding.definition_id);
    const auto cell = people::world_cell(s.position);
    facilities::FacilityServiceExitInput i;
    i.actor = s.actor;
    i.control = s.control;
    i.binding_valid = cell && world::arrival_matches(map_, binding, *cell);
    i.map = map_;
    i.facility = f.placement;
    i.position = s.position;
    i.category = d.activity_category;
    i.detail = d.activity_detail;
    i.progress = s.uses.at(d.id);
    i.upgrade_uses = d.economy.upgrade_uses;
    i.occupants = f.occupants;
    auto economy_input = facilities::with_neighbours({}, neighbourhoods_.at(binding.instance));
    const auto values = facilities::derive_economy(d.economy, economy_input);
    if (values.instance[1] > std::numeric_limits<int>::max())
        return InitialAiError::preparation_failed;
    i.satisfaction = {s.actor,
                      binding.instance,
                      d.id,
                      s.satisfaction,
                      initial_ai_rules().satisfaction_thresholds,
                      static_cast<int>(values.instance[1]),
                      tickets.satisfaction};
    i.effects = initial_ai_rules().services.at(d.id).effects;
    i.effect_ticket = tickets.attribute;
    if (random && d.activity_category == 1 && d.activity_detail == 0) {
        i.satisfaction.ticket = std::uniform_int_distribution<int>(0, 9)(*random);
        if (!i.effects.empty())
            i.effect_ticket = std::uniform_int_distribution<int>(
                0, static_cast<int>(i.effects.size()) - 1)(*random);
    }
    i.equipment.old_weapon = s.current_weapon;
    i.equipment.new_weapon = s.selected_weapon.value_or(s.current_weapon);
    const auto result = facilities::prepare_facility_service_exit(i);
    if (!result.candidate || result.candidate->cleanup)
        return InitialAiError::preparation_failed;
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
    return InitialAiError::none;
}
// The d pass advances counters/effects before interpreting controls. Successful8 ends this pass.
InitialAiError InitialAiSession::execution(InitialAiState &s, const InitialAiTickets &tickets,
                                           std::mt19937 *random) const {
    s.counters.action = s.control.action_counter;
    s.counters.alternate = s.control.alternate_counter;
    const auto counters = people::advance_actor_counters(s.counters);
    const auto effects = people::advance_actor_effects(s.effects);
    const auto hp = people::advance_hp_animation(s.hp);
    if (!counters || !effects.candidate || !hp.candidate)
        return InitialAiError::preparation_failed;
    s.counters = *counters;
    s.effects = effects.candidate->state;
    s.hp = *hp.candidate;
    s.control.action_counter = s.counters.action;
    s.control.alternate_counter = s.counters.alternate;
    for (int budget = 0; budget < 128; ++budget) {
        const auto prefix = people::prepare_local_control_prefix(s.control);
        if (!prefix.candidate)
            return InitialAiError::preparation_failed;
        s.control = prefix.candidate->state;
        if (prefix.candidate->flow != people::ActorControlFlow::delegated)
            return InitialAiError::none;
        const auto command = s.control.queue.front();
        if (command[0] == 8) {
            if (command[1] != 0)
                return InitialAiError::unsupported_branch;
            const auto error = depart(s, tickets, random);
            if (error != InitialAiError::none)
                return error;
            const auto submitted =
                people::prepare_local_control_prefix(s.control, {std::nullopt, true});
            if (!submitted.candidate)
                return InitialAiError::preparation_failed;
            s.control = submitted.candidate->state;
            return InitialAiError::none; // Successful8 early stop, pending tail remains.
        }
        if (command[0] == 24) {
            const auto error = exit(s, tickets, random);
            if (error != InitialAiError::none)
                return error;
            continue;
        }
        s.control.queue.erase(s.control.queue.begin());
        if (command[0] == 21) {
            if (!s.active_facility)
                return InitialAiError::preparation_failed;
            s.facilities.at(s.active_facility->instance).occupants.push_back(s.actor);
            ++s.occupations;
        } else if (command[0] == 19) {
            if (command[2] < 0 || command[2] >= 6)
                return InitialAiError::invalid_input;
            auto &value = s.definition.extra[command[2]];
            const auto sum = static_cast<std::int64_t>(value) + command[3];
            if (sum < std::numeric_limits<int>::min() || sum > std::numeric_limits<int>::max())
                return InitialAiError::preparation_failed;
            value = static_cast<int>(sum);
            // a.e.b(attribute,delta) only writes z; it does NOT eagerly recalculate x/w.
            // A subsequent equip/job/growth recalculation observes the accumulated value.
            s.effects.display.insert(s.effects.display.begin(),
                                     {13, -command[1], command[2], command[3]});
            ++s.attribute_commits;
        } else if (command[0] == 28) {
            const auto commit = people::prepare_equipment_commit(command);
            if (!commit ||
                static_cast<std::size_t>(commit->equipment) >= initial_ai_rules().weapons.size())
                return InitialAiError::preparation_failed;
            s.definition.equipment[0] = initial_ai_rules().weapons[commit->equipment].combat;
            const auto stats =
                people::derive_human_stats(s.definition, initial_ai_rules().professions);
            if (!stats.candidate)
                return InitialAiError::preparation_failed;
            s.stats = *stats.candidate;
            s.current_weapon = commit->equipment;
            s.weapon_reselect_counter = commit->reselect_counter;
            ++s.equipment_commits;
        } else if (command[0] == 18 || command[0] == 27) {
            // Keep visual requests explicit: no invented language-variant count or pixel payload.
            s.presentation_requests.push_back(command);
        } else
            return InitialAiError::unsupported_branch;
    }
    return InitialAiError::preparation_failed;
}
} // namespace ark::app
