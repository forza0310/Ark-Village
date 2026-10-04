// Window-visible adapter for the published initial life interval, not the full world AI loop.
#include "ark/app/game.hpp"
#include <stdexcept>

namespace ark::app {
void Game::start_ai_preview() {
    ai_.emplace(*this, state_.adventurer->cell);
    // The first c/d pair runs in the arrival step before the tutorial blocks subsequent rounds.
    step_ai_preview();
}
void Game::step_ai_preview() {
    if (ai_->state().rounds >= 1000) {
        state_.mode = Mode::research_boundary; // Product preview limit, not a gameplay duration.
        return;
    }
    ai_error_ = ai_->round_random(random_);
    if (ai_error_ != InitialAiError::none) {
        // Retain the last successful round. Unknown exit/fallback is an explicit preview boundary.
        state_.mode = Mode::research_boundary;
        return;
    }
    project_ai_preview();
}
// These fields are the existing UI read model. The private session remains the sole AI owner;
// preview construction is disabled, so there is no competing cash/equipment writer.
void Game::project_ai_preview() {
    const auto &s = ai_->state();
    auto &actor = *state_.adventurer;
    const auto cell = people::world_cell(s.position);
    if (!cell)
        throw std::logic_error("AI produced an invalid world position");
    actor.position = s.position;
    actor.cell = *cell;
    actor.flags = s.control.flags;
    actor.satisfaction = s.satisfaction;
    actor.attributes = s.stats.attributes;
    actor.combat = s.stats.combat;
    actor.equipment[0] = s.current_weapon;
    actor.hp = {s.hp.displayed, s.hp.origin, s.hp.target};
    actor.pending_activity.reset();
    state_.money = s.accounting.funds();
    state_.accounting = s.accounting; // Diagnostic read projection; preview rejects construction.
    state_.next_cash_id = s.next_cash_id;
    for (const auto &[id, uses] : s.uses) {
        auto &progress = state_.definition_progress.at(id);
        progress.level = uses.level;
        progress.completed_uses = static_cast<std::uint64_t>(uses.completed_uses);
        progress.upgrade_pending = uses.upgrade_pending;
    }
}
} // namespace ark::app
