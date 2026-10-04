// Adapted from research d7ca763 prototype/startup_ai.cpp; private owner only.
#include "ark/app/ai_schedule.hpp"
#include "ark/app/game.hpp"
#include "initial_ai_internal.hpp"
namespace ark::app {
// Admit only the published static first-visitor state; this is not a general save importer.
InitialAiSession::InitialAiSession(const Game &startup, world::Cell birth) {
    const auto &e = startup_data();
    const auto &initial = startup.state();
    if (!initial.adventurer || initial.event89_count != 1 || initial.simulation_steps != 420 ||
        initial.adventurer->definition_id != e.first_character.definition_id ||
        std::find(e.spawn_points.begin(), e.spawn_points.end(), birth) == e.spawn_points.end() ||
        !initial.expenses.empty() || initial.facilities.size() != e.seeds.size() ||
        initial.instance_order.size() != e.seeds.size() || initial.money != e.money)
        throw std::invalid_argument("Initial AI requires unchanged actual first-visitor snapshot");
    // The isolated interval does not admit player construction/rotation or arbitrary live saves.
    for (std::size_t n = 0; n < e.seeds.size(); ++n) {
        const auto &seed = e.seeds[n];
        const auto found = initial.facilities.find(seed.id);
        if (found == initial.facilities.end() || initial.instance_order.at(n) != seed.id ||
            found->second.definition_id != seed.definition_id ||
            !(found->second.anchor == seed.anchor) ||
            found->second.orientation != seed.orientation || found->second.remaining_ticks != 0)
            throw std::invalid_argument("Initial AI cannot consume a modified map");
        facilities::Placement p{seed.id, seed.definition_id,
                                initial_definition(seed.definition_id).shape, seed.orientation,
                                seed.anchor};
        state_.facilities.emplace(seed.id, InitialAiFacility{p, {}, {}});
        state_.uses.emplace(seed.definition_id, facilities::FacilityUseProgress{});
        neighbourhoods_.emplace(seed.id, startup.neighbourhood(seed.id));
    }
    map_ = startup.route_map();
    instance_order_ = initial.instance_order;
    state_.actor = {static_cast<std::uint64_t>(e.first_character.uid) + 1};
    state_.control.flags = initial.adventurer->flags;
    state_.control.queue = {{8, 0}};
    state_.position = {birth.x * 100.0F + 50.0F, birth.y * 100.0F + 50.0F};
    state_.accounting = economy::CashLedger(initial.money);
    state_.definition = initial_ai_rules().first_definition;
    const auto stats =
        people::derive_human_stats(state_.definition, initial_ai_rules().professions);
    if (!stats.candidate || stats.candidate->attributes != e.first_character.attributes ||
        stats.candidate->combat != e.first_character.combat)
        throw std::invalid_argument("Initial AI source stats disagree with actual first visitor");
    state_.stats = *stats.candidate;
    state_.hp = {
        0, e.first_character.hp[0], e.first_character.hp[1], e.first_character.hp[2], false, 0};
    state_.current_weapon = e.first_character.equipment[0];
    state_.weapon_reselect_counter = initial_ai_rules().weapon_reselect_counter;
    state_.satisfaction = e.first_character.satisfaction;
}
const InitialAiState &InitialAiSession::state() const { return state_; }

// Prepare c/d against one private copy; even a late exit/choice rejection discards all owners.
InitialAiError InitialAiSession::round(const InitialAiTickets &tickets) {
    return prepare_round(tickets, nullptr);
}
InitialAiError InitialAiSession::round_random(std::mt19937 &random) {
    auto next_random = random;
    const auto error = prepare_round({}, &next_random);
    if (error == InitialAiError::none)
        random = next_random;
    return error;
}
InitialAiError InitialAiSession::prepare_round(const InitialAiTickets &tickets,
                                               std::mt19937 *random) {
    if (live_ && state_.error != InitialAiError::none)
        return state_.error; // A retained handoff must not redraw its actual selection.
    auto next = state_;
    AiScheduleInput schedule;
    schedule.rosters[0] = {next.actor.value};
    schedule.rosters[5] = instance_order_;
    InitialAiError error = InitialAiError::none;
    const auto candidate = prepare_ai_schedule(schedule, [&](const auto &v, const auto &) {
        if (v.phase == AiSchedulePhase::human_decision)
            error = decision(next, tickets);
        else if (v.phase == AiSchedulePhase::human_execution)
            error = execution(next, tickets, random);
        AiScheduleResponse response;
        response.accepted = error == InitialAiError::none;
        return response;
    });
    if (!candidate.candidate)
        return error == InitialAiError::none ? InitialAiError::preparation_failed : error;
    ++next.rounds;
    state_ = std::move(next);
    return InitialAiError::none;
}
} // namespace ark::app
