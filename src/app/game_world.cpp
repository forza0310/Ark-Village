// Shared admitted round. This Game is already the private transaction made by update().
#include "ark/app/game.hpp"
#include "ark/app/world_schedule.hpp"
#include <stdexcept>

namespace ark::app {
void Game::step_normal_world() {
    WorldScheduleState common;
    // No second durable roster/map/cash: derive this protocol from the live Game each round.
    common.updates = static_cast<int>(state_.simulation_steps);
    if (state_.life)
        common.rosters[0].push_back(state_.life->actor.value);
    common.rosters[5] = state_.instance_order;
    ++state_.simulation_steps;
    bool first_visit{};
    std::optional<InitialAiSession> engine;
    auto candidate_random = random_;
    auto error = InitialAiError::none;
    const auto active = [&] {
        return engine && error == InitialAiError::none &&
               engine->state_.error == InitialAiError::none;
    };
    const auto result = prepare_world_schedule(
        common, {},
        [&](const WorldScheduleState &protocol,
            const WorldScheduleCall &call) -> std::optional<WorldScheduleStep> {
            WorldScheduleStep out{protocol, false};
            switch (call.stage) {
            case WorldScheduleStage::influence:
                // No combat/encounter consumer is populated in this first-person slice. Its field
                // remains unmaterialized; adding an enemy requires a real common field consumer.
                break;
            case WorldScheduleStage::arrival_front:
                first_visit = state_.event89_count == 0 && --state_.arrival_counter == 0;
                if (first_visit) {
                    const auto &data = startup_data();
                    const auto choice = std::uniform_int_distribution<std::size_t>(
                        0, data.spawn_points.size() - 1)(candidate_random);
                    state_.adventurer =
                        people::first_visit(data.first_character, data.spawn_points[choice]);
                    ++state_.event89_count;
                    state_.mode = Mode::tutorial;
                    start_village_life();
                    out.state.rosters[0].push_back(state_.life->actor.value);
                }
                if (state_.life && state_.life->error == InitialAiError::none)
                    engine = InitialAiSession::from_village(*this, *state_.life);
                break;
            case WorldScheduleStage::rescue_query:
                for (const auto id : state_.instance_order) {
                    const auto &f = state_.facilities.at(id);
                    if (f.remaining_ticks == 0 &&
                        definition(f.definition_id).activity_category == 2)
                        out.state.rescue_available = true;
                }
                break;
            case WorldScheduleStage::decision_prefix:
                // live_decision consumes its c prefix and state branch together, with no second c.
                break;
            case WorldScheduleStage::decision:
                if (active())
                    error = engine->live_decision(engine->state_, &candidate_random);
                break;
            case WorldScheduleStage::execution_prefix:
                if (active())
                    error = engine->execution_prefix(engine->state_);
                break;
            case WorldScheduleStage::carry_expression:
                // First visitor has no carried-character reference; this stage consumes no draw.
                break;
            case WorldScheduleStage::control:
                if (active()) {
                    error = engine->execution(engine->state_, {}, &candidate_random);
                    out.remove_requested = error == InitialAiError::none && engine->state_.removed;
                }
                break;
            case WorldScheduleStage::actor_tail:
                if (active()) {
                    error = engine->live_tail(engine->state_);
                    out.remove_requested = error == InitialAiError::none && engine->state_.removed;
                }
                break;
            case WorldScheduleStage::remove_actor:
                // Actual release/retirement is part of the same final owner commit below.
                break;
            case WorldScheduleStage::facility: {
                auto &f = state_.facilities.at(*call.id);
                if (f.remaining_ticks > 0 && --f.remaining_ticks == 0)
                    ++state_.layout_revision;
                break;
            }
            case WorldScheduleStage::finalize:
                // Exactly one or zero live actors: L's pair loops contain no pair, hence no draw.
                break;
            case WorldScheduleStage::popularity:
            case WorldScheduleStage::projectile:
            case WorldScheduleStage::object:
            case WorldScheduleStage::encounter:
                return {}; // These domains have no owner yet; never accept a fake successful
                           // consumer.
            }
            return out;
        });
    if (!result.candidate)
        throw std::logic_error("Current world violates its schedule contract");
    if (engine) {
        if (error == InitialAiError::none) {
            ++engine->state_.rounds;
            commit_village_life(engine->state_);
        } else {
            state_.life->error = error;
            ai_error_ = error;
            // Late actor failure discards its full candidate, including random choices and cash.
        }
    }
    if (error == InitialAiError::none)
        random_ = candidate_random;
    if (!first_visit) {
        auto &date = state_.calendar;
        date[3] += 27;
        if (date[3] >= 10800) {
            date[3] -= 10800;
            ++date[2];
        }
    }
}
} // namespace ark::app
