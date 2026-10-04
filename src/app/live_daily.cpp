// Current empty-task/empty-encounter village's supported daily c0/c5 branches.
// Adapted from maintained world_daily; path P and execution d remain separate phases.
#include "ark/app/initial_ai.hpp"
#include "ark/people/ai_perception.hpp"

namespace ark::app {
namespace {
people::TownBounds town() {
    const auto b = startup_data().build_bounds;
    return {b.min_x - 1, b.max_x + 1, b.min_y - 1, b.max_y + 1};
}
} // namespace
InitialAiError InitialAiSession::live_spawn(InitialAiState &s, RandomStream *random) const {
    people::SpawnProbeInput input;
    input.state = s.control.state;
    input.inside_town = people::inside_town(s.cached_cell, town());
    input.destination_inside_town = people::inside_town(s.destination, town());
    input.cell_y = s.cached_cell.y;
    input.minimum_y = startup_data().character_spawn_minimum_y;
    input.monster_count = 0; // Actual populated roster, not a pretend cap reached.
    input.monster_limit = 4; // Research initial common-world cap; no monster owner yet.
    // Probe qualification first. Ticket0 is a dry input; it does not consume the global stream.
    input.ticket = 0;
    const auto admission = people::prepare_spawn_probe(input);
    if (!admission.candidate)
        return InitialAiError::preparation_failed;
    if (!admission.candidate->consumes_ticket)
        return InitialAiError::none;
    if (random) {
        const auto draw = random->draw(1000);
        if (draw.error != RandomError::none)
            return InitialAiError::preparation_failed;
        input.ticket = draw.ticket;
    } else
        input.ticket = 999; // Explicit path/FIFO fixture excludes creation, not a new-game value.
    const auto probe = people::prepare_spawn_probe(input);
    if (!probe.candidate)
        return InitialAiError::preparation_failed;
    if (probe.candidate->request_event_probe) {
        s.handoff = LifeHandoff::encounter_creation;
        s.error = InitialAiError::unsupported_branch;
        s.spawn_ticket = input.ticket;
        s.spawn_center = s.cached_cell;
        s.pending_activity = 6;
    }
    return InitialAiError::none;
}
InitialAiError InitialAiSession::live_idle(InitialAiState &s, RandomStream *random) const {
    // Expression8 always draws probability before bit16. Both original platform tables have4
    // variants here; consume the variant only if insertion really needs one (not if suppressed).
    people::ActorExpressionInput expression{s.effects, 8, 0, 999, 4, {}};
    if (random) {
        const auto probability = random->draw(1000);
        if (probability.error != RandomError::none)
            return InitialAiError::preparation_failed;
        expression.probability_ticket = probability.ticket;
    }
    auto effect = people::prepare_actor_expression(expression);
    if (effect.error == people::ActorEffectError::missing_ticket) {
        if (!random)
            return InitialAiError::preparation_failed;
        const auto variant = random->draw(4);
        if (variant.error != RandomError::none)
            return InitialAiError::preparation_failed;
        expression.variant_ticket = variant.ticket;
        effect = people::prepare_actor_expression(expression);
    }
    if (!effect.candidate)
        return InitialAiError::preparation_failed;
    s.effects = std::move(effect.candidate->state);
    const auto idle = people::prepare_human_idle(
        {s.control.flags, false, false, false, false, false, s.retention.outside_updates,
         s.control.action == 7 ? 0 : s.hp.target, s.stats.combat[0], s.definition.legacy_u});
    if (!idle.candidate)
        return InitialAiError::preparation_failed;
    if (idle.candidate->activity) {
        const auto changed = people::prepare_actor_state_transition(
            {s.control, true, 0, s.baseline, 0, {}, false, {}});
        if (!changed)
            return InitialAiError::preparation_failed;
        s.control = changed->control;
        s.baseline = changed->baseline;
        s.counters.state = 0;
        s.control.queue.push_back({8, *idle.candidate->activity});
    }
    return idle.candidate->run_spawn_probe ? live_spawn(s, random) : InitialAiError::none;
}
} // namespace ark::app
