#include "ark/simulation/rules/world_random_consumers.hpp"

#include <array>

namespace ark::simulation::rules {
int reference_world_expression_variants(int expression, bool primary_table) {
    constexpr std::array<int, 19> first{7, 4, 2, 4, 2, 3, 3, 4, 4, 9, 2, 2, 1, 3, 1, 3, 2, 2, 1};
    constexpr std::array<int, 19> second{7, 4, 2, 4, 2, 3, 3, 4, 4, 9, 2, 2, 1, 3, 1, 1, 1, 2, 1};
    if (expression < 0 || expression >= 19)
        return 0;
    return (primary_table ? first : second)[static_cast<std::size_t>(expression)];
}
WorldExpressionDrawResult prepare_world_random_expression(WorldRandomStream &stream,
                                                          const ActorEffectState &state,
                                                          int expression, int delay,
                                                          bool primary_table) {
    const int count = reference_world_expression_variants(expression, primary_table);
    if (count == 0 || !valid_actor_effect_state(state) || delay < 0)
        return {WorldRandomError::none, ActorEffectError::invalid_input, {}, {}};
    auto scratch = stream;
    const auto probability = scratch.draw(1000);
    if (probability.error != WorldRandomError::none)
        return {probability.error, ActorEffectError::none, {}, {}};
    WorldExpressionTicket ticket{probability.ticket, count, {}};
    auto r = prepare_actor_expression({state, expression, delay, ticket.probability, count, {}});
    if (r.error == ActorEffectError::missing_ticket) {
        const auto variant = scratch.draw(count);
        if (variant.error != WorldRandomError::none)
            return {variant.error, ActorEffectError::none, {}, {}};
        ticket.variant = variant.ticket;
        r = prepare_actor_expression(
            {state, expression, delay, ticket.probability, count, ticket.variant});
    }
    if (!r.candidate)
        return {WorldRandomError::none, r.error, {}, {}};
    stream = std::move(scratch);
    return {WorldRandomError::none, ActorEffectError::none, ticket, r.candidate};
}
} // namespace ark::simulation::rules
