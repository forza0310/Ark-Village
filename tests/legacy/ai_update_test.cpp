// Explicit conditional fixtures: cross-module ordering, not original new-game combat actors.
#include "ark/app/ai_schedule.hpp"
#include "ark/people/actor_housekeeping.hpp"
#include "ark/people/combat_ai.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace ark;
using namespace ark::people;
void check(bool pass, const char *why) {
    if (!pass)
        throw std::runtime_error(why);
}
struct SampleOwner {
    ActorControlState control;
    world::WorldPosition position{250, 250};
    int blocked_steps{}, cooldown{3}, town_steps{};
    bool facility_bound{true};
    std::vector<app::AiSchedulePhase> effects;
};
// The schedule owns traversal, while the caller owns and commits one private state copy.
bool sample_round(SampleOwner &live, bool reject_final) {
    auto next = live;
    const auto old_position = live.position;
    app::AiScheduleInput input;
    input.rosters[0] = {0}; // Original roster ID0, distinct from nonzero AI snapshot IDs.
    const auto result = app::prepare_ai_schedule(input, [&](const auto &visit, const auto &) {
        app::AiScheduleResponse response;
        next.effects.push_back(visit.phase);
        if (visit.phase == app::AiSchedulePhase::human_decision) {
            const auto prefix = prepare_actor_decision_prefix(
                next.control.flags, next.blocked_steps, next.cooldown, 100, 100);
            if (!prefix) {
                response.accepted = false;
                return response;
            }
            next.control.flags = prefix->flags;
            next.cooldown = prefix->attack_cooldown;
            next.blocked_steps = prefix->blocked_battle_steps;
            CombatStrategyInput policy;
            policy.flags = next.control.flags;
            policy.in_move_area = policy.sensed_enemy = policy.same_town_side = policy.fresh_enemy =
                true;
            policy.weapon_range = 100;
            policy.sensed_distance = 500;
            policy.group_tick = 1;
            policy.attack_slot = 10;
            const auto choice = prepare_combat_strategy(policy);
            if (!choice.candidate || choice.candidate->decision != CombatDecision::approach) {
                response.accepted = false;
                return response;
            }
            std::array<CombatMoveSample, 9> samples{};
            for (auto &sample : samples)
                sample = {true, true, 0, 100};
            samples[5].enemy_distance = 50;
            const auto move = prepare_combat_step(samples, false);
            if (!move.candidate || move.candidate->selected != 5) {
                response.accepted = false;
                return response;
            }
            next.position = advance_motion(next.position, {325, 275}, next.control.flags).position;
        } else if (visit.phase == app::AiSchedulePhase::human_execution) {
            ActorPhysicsInput physics;
            physics.state = 1;
            physics.flags = next.control.flags;
            physics.position = next.position;
            physics.decision_start = old_position;
            physics.blocked_battle_steps = next.blocked_steps;
            physics.area_after = false;
            const auto motion = prepare_actor_physics(physics);
            if (!motion) {
                response.accepted = false;
                return response;
            }
            next.position = motion->state.position;
            next.blocked_steps = motion->state.blocked_battle_steps;
            ActorRetentionInput retention;
            retention.state.state = 1;
            retention.state.flags = next.control.flags;
            retention.state.town_updates = next.town_steps;
            retention.old_cell_inside_town = true;
            retention.area_before = true; // Old c query, distinct from physics.area_after=false.
            retention.has_encounter = true;
            retention.route_cells = 3;
            const auto tail = prepare_actor_retention(retention);
            if (!tail || tail->delete_instance || !tail->requests.empty()) {
                response.accepted = false;
                return response;
            }
            next.town_steps = tail->state.town_updates;
        } else if (visit.phase == app::AiSchedulePhase::finalize) {
            response.accepted = !reject_final;
        }
        return response;
    });
    if (!result.candidate)
        return false;
    live = std::move(next);
    return true;
}
void movement_and_rollback() {
    SampleOwner live;
    live.control.flags = 128U;
    check(!sample_round(live, true) && live.position.x == 250 && live.position.z == 250 &&
              live.cooldown == 3 && live.town_steps == 0 && live.effects.empty(),
          "late scheduler failure cannot expose physics, counters or effects from private copy");
    check(sample_round(live, false) && live.position.x == 250 && live.position.z == 250 &&
              live.blocked_steps == 1 && live.cooldown == 2 && live.town_steps == 1 &&
              (live.control.flags & 2U),
          "strategy movement precedes physics rollback; old area controls retention separately");
    check(live.effects == std::vector<app::AiSchedulePhase>{app::AiSchedulePhase::human_decision,
                                                            app::AiSchedulePhase::human_execution,
                                                            app::AiSchedulePhase::finalize},
          "common update consumers remain in real c/d schedule order");
    const auto prefix = prepare_actor_decision_prefix(live.control.flags, 150, 2, 100, 100);
    check(prefix && prefix->restore_baseline && prefix->blocked_battle_steps == 0,
          "blocked150 explicitly requests baseline restoration");
    live.control.alternate_counter = 20;
    const auto baseline = prepare_actor_baseline_restore(live.control, 5, true, 0);
    check(baseline && baseline->control.alternate_counter == 20 &&
              baseline->control.queue == std::vector<LegacyActorControl>{{10, 0}},
          "blocked recovery connects to b without destroying alternate counter");
}
void deletion_release() {
    SampleOwner live;
    auto next = live;
    app::AiScheduleInput input;
    input.rosters[0] = {0};
    const auto result = app::prepare_ai_schedule(input, [&](const auto &visit, const auto &) {
        app::AiScheduleResponse response;
        next.effects.push_back(visit.phase);
        if (visit.phase == app::AiSchedulePhase::human_execution) {
            ActorRetentionInput retention;
            retention.state.flags = 2;
            retention.state.no_path_updates = 59;
            retention.area_before = true;
            const auto tail = prepare_actor_retention(retention);
            check(tail && tail->reason == ActorDeletionReason::empty_route,
                  "published no-path60 requests removal");
            response.remove = tail->delete_instance;
            response.release_current_facility = response.remove && next.facility_bound;
        } else if (visit.phase == app::AiSchedulePhase::release_human_facility) {
            next.facility_bound = false;
        }
        return response;
    });
    check(result.candidate && result.candidate->rosters[0].empty() && !next.facility_bound &&
              live.facility_bound &&
              next.effects[2] == app::AiSchedulePhase::release_human_facility,
          "retention deletion releases bound facility before erase, leaving original untouched");
    ActorRetentionInput stuck;
    stuck.state.flags = 2;
    stuck.state.bad_area_updates = 29;
    stuck.route_cells = 3;
    stuck.has_encounter = true;
    const auto tail = prepare_actor_retention(stuck);
    const auto cleanup = prepare_actor_cleanup(ActorKind::human, stuck.state.flags);
    check(tail && cleanup && tail->state.state == cleanup->state && (tail->state.flags & 32768U) &&
              tail->requests[0] == ActorRetentionRequest::clear_path &&
              tail->requests[1] == ActorRetentionRequest::cleanup,
          "bad-area recovery uses existing r then escape; it is not ordinary deletion");
}
} // namespace
int main() {
    try {
        movement_and_rollback();
        deletion_release();
        std::cout << "PASS AI common-update/control/motion/physics/retention schedule contracts\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
