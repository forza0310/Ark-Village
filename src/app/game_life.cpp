// Normal startup's first-actor life loop. Game remains the only durable village owner.
#include "ark/app/game.hpp"
#include <stdexcept>

namespace ark::app {
void Game::start_village_life() {
    const auto &visitor = *state_.adventurer;
    const auto &rules = initial_ai_rules();
    LifeActorState actor;
    actor.actor = {static_cast<std::uint64_t>(visitor.uid) + 1};
    actor.control.flags = visitor.flags;
    actor.control.queue = {{8, 0}};
    actor.position = visitor.position;
    actor.definition = rules.first_definition;
    const auto stats = people::derive_human_stats(actor.definition, rules.professions);
    if (!stats.candidate || stats.candidate->combat != visitor.combat ||
        stats.candidate->attributes != visitor.attributes)
        throw std::logic_error("First visitor and researched life stats disagree");
    actor.stats = *stats.candidate;
    actor.hp = {0, visitor.hp[0], visitor.hp[1], visitor.hp[2], false, 0};
    actor.current_weapon = visitor.equipment[0];
    actor.weapon_reselect_counter = rules.weapon_reselect_counter;
    actor.satisfaction = visitor.satisfaction;
    state_.life = std::move(actor);
    step_village_life(); // Same arrival round c/d, before the tutorial gates subsequent updates.
}
void Game::step_village_life() {
    if (!state_.life || state_.life->error != InitialAiError::none)
        return; // An explicit actor handoff does not prevent dates or construction advancing.
    auto engine = InitialAiSession::from_village(*this, *state_.life);
    auto candidate_random = random_;
    const auto error = engine.round_random(candidate_random);
    if (error != InitialAiError::none) {
        state_.life->error = error;
        ai_error_ = error;
        return; // Failed numeric/structure round retains the last successful actor/world/RNG.
    }
    const auto &candidate = engine.state();
    state_.life = static_cast<const LifeActorState &>(candidate);
    state_.accounting = candidate.accounting;
    state_.next_cash_id = candidate.next_cash_id;
    state_.money = state_.accounting.funds();
    for (const auto &[id, service] : candidate.facilities)
        state_.facility_life.at(id) = {service.sales, service.occupants};
    for (const auto &[id, use] : candidate.uses) {
        auto &shared = state_.definition_progress.at(id);
        shared.level = use.level;
        shared.completed_uses = static_cast<std::uint64_t>(use.completed_uses);
        shared.upgrade_pending = use.upgrade_pending;
    }
    random_ = candidate_random;
    ai_error_ = state_.life->error;
    project_village_actor();
}
// Existing HUD, roster and renderer consume one read projection of the actual life actor.
void Game::project_village_actor() {
    const auto &s = *state_.life;
    auto &actor = *state_.adventurer;
    const auto cell = people::world_cell(s.position);
    if (!cell)
        throw std::logic_error("Life actor produced an invalid world position");
    actor.position = s.position;
    actor.cell = *cell;
    actor.flags = s.control.flags;
    actor.satisfaction = s.satisfaction;
    actor.attributes = s.stats.attributes;
    actor.combat = s.stats.combat;
    actor.equipment[0] = s.current_weapon;
    actor.hp = {s.hp.displayed, s.hp.origin, s.hp.target};
    actor.pending_activity.reset();
}
} // namespace ark::app
