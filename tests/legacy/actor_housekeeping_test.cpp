// Published research e8bd81c guards, thresholds and physics regressions.
#include "ark/people/actor_housekeeping.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::people;
namespace world = ark::world;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
void preparation() {
    for (bool battle : {false, true})
        for (bool inmap : {false, true})
            for (bool event : {false, true})
                for (bool bound : {false, true})
                    for (int match = 0; match < 4; ++match)
                        for (bool exit : {false, true}) {
                            BattlePreparationInput i;
                            i.battle_gate = battle;
                            i.cell_in_map = inmap;
                            i.cell_event_flag = event;
                            if (bound)
                                i.encounter = BattlePreparationEncounter{0, {4, 5}};
                            i.live_encounters = {{match & 1 ? 0 : 1, match & 2
                                                                         ? world::Cell{4, 5}
                                                                         : world::Cell{5, 4}}};
                            i.flags = exit ? 512U : 0U;
                            const auto c = prepare_battle_preparation(i);
                            const bool early_baseline = (inmap && !event) || !bound;
                            const bool matched = match == 3;
                            const auto expected = battle ? BattlePreparationAction::battle
                                                  : early_baseline || !matched || exit
                                                      ? BattlePreparationAction::restore_baseline
                                                      : BattlePreparationAction::keep;
                            check(c.action == expected &&
                                      c.clear_encounter ==
                                          (!battle && !early_baseline && (!matched || exit)),
                                  "state18 G priority, map asymmetry, zeroID+center identity,512 "
                                  "last");
                        }
    for (int blocked : {0, 149, 150, 151})
        for (int cooldown : {0, 1, 5}) {
            const auto c = prepare_actor_decision_prefix(0, blocked, cooldown, 4, 9);
            check(c && c->flags == 2 && c->blocked_battle_steps == (blocked >= 150 ? 0 : blocked) &&
                      c->restore_baseline == (blocked >= 150) &&
                      c->attack_cooldown == (cooldown > 0 ? cooldown - 1 : 0) && !c->low_hp,
                  "c prefix at150 reset,aj decrement, HP strict half integer");
        }
}
void physics() {
    for (int state = 0; state <= 20; ++state)
        for (int pause : {0, 1, 2})
            for (bool area : {false, true})
                for (bool escaped : {false, true}) {
                    ActorPhysicsInput i;
                    i.state = state;
                    i.pause = pause;
                    i.height = 20;
                    i.vertical_velocity = 2;
                    i.position = {3, 4};
                    i.decision_start = {1, 2};
                    i.area_after = area;
                    i.previous_area_after = true;
                    i.blocked_battle_steps = 4;
                    i.flags = escaped ? 32768U : 0U;
                    const auto c = prepare_actor_physics(i);
                    const bool admitted = pause == 0 && state != 16;
                    check(c && c->query_area_after == admitted &&
                              c->state.pause == (pause ? pause - 1 : 0),
                          "P1 decrements0 but gravity/K waits next d; rescued16 ignores physics");
                    if (!admitted) {
                        check(c->state.height == 20 && c->state.vertical_velocity == 2 &&
                                  c->state.previous_area_after,
                              "skipped physics preserves height,velocity and old aB1");
                        continue;
                    }
                    const float g = state == 10   ? -28.0F / 42.0F
                                    : state == 15 ? -240.0F / 870.0F
                                    : state == 20 ? -80.0F / 306.0F
                                                  : -20.0F / 6.0F;
                    check(c->state.vertical_velocity == 2 + g && c->state.height == 20 + (2 + g) &&
                              c->state.previous_area_after == area,
                          "four source gravity families and new velocity-first integration");
                    const bool reverted = !escaped && !area && (state == 1 || state == 4);
                    check(c->state.position.x == (reverted ? 1 : 3) &&
                              c->diagnostic.has_value() == reverted,
                          "battle/knockback only restore horizontal bu; escape32768 bypasses");
                    check(c->state.blocked_battle_steps ==
                              (state == 1 && !escaped ? (area ? 0 : 5) : 4),
                          "at only battle success/failure, not generic K or knockback");
                }
    ActorPhysicsInput i;
    i.height = 1;
    i.vertical_velocity = 0;
    auto c = prepare_actor_physics(i);
    check(c && c->state.height == 0 && c->state.vertical_velocity == 0,
          "negative ground clamps both height and velocity");
    i.height = std::numeric_limits<float>::infinity();
    check(!prepare_actor_physics(i), "nonfinite physics cannot expose partial position");
}
ActorRetentionInput retention_fixture() {
    ActorRetentionInput i;
    i.state.flags = 2;
    i.area_before = true;
    i.has_encounter = true;
    i.route_cells = 3;
    return i;
}
void retention() {
    for (int state = 0; state <= 20; ++state)
        for (unsigned flags : {0U, 2U, 16U, 18U, 64U, 66U, 512U, 514U, 1026U, 32770U})
            for (int old = 0; old <= 201; ++old) {
                auto i = retention_fixture();
                i.state.state = state;
                i.state.flags = flags;
                i.state.blocked_updates = old;
                i.old_cell_inside_town = old % 2;
                const auto c = prepare_actor_retention(i);
                const bool blocked = (flags & 64U) || !(flags & 2U);
                const bool cleaned = blocked && old >= 199;
                check(c && c->state.blocked_updates == (blocked ? old + 1 : 0) &&
                          c->state.state == (cleaned ? 19 : state) &&
                          c->requests.size() == (cleaned ? 1U : 0U),
                      "ab threshold200 cleanup occurs before subsequent guards, nonblocked reset0");
                check(
                    c->state.town_updates == (i.old_cell_inside_town ? 1 : 0) &&
                        c->state.outside_updates ==
                            (!i.old_cell_inside_town && state != 0 && !(flags & 16U) ? 1 : 0),
                    "L/M use old cell and old state before cleanup, not later projected location");
            }
    for (unsigned flags : {2U, 514U, 1026U})
        for (int old = 0; old <= 100; ++old) {
            auto i = retention_fixture();
            i.at_spawn_after_projection = true;
            i.state.flags = flags;
            i.state.spawn_updates = old;
            i.state.no_path_updates = 9;
            i.state.short_exit_updates = 8;
            const auto c = prepare_actor_retention(i);
            const bool deletes = old + 1 >= ((flags & (512U | 1024U)) ? 30 : 100);
            check(c && c->delete_instance == deletes && c->state.spawn_updates == old + 1 &&
                      c->state.no_path_updates == (deletes ? 9 : 0) &&
                      c->reason == (deletes ? ActorDeletionReason::spawn_timeout
                                            : ActorDeletionReason::none),
                  "spawn guard30/100 early return retains later guard counters");
        }
    auto i = retention_fixture();
    i.route_cells = 0;
    i.state.no_path_updates = 59;
    i.state.short_exit_updates = 9;
    auto c = prepare_actor_retention(i);
    check(c && c->reason == ActorDeletionReason::empty_route && c->state.short_exit_updates == 9,
          "empty route60 before short-exit guard");
    i = retention_fixture();
    i.state.flags |= 512;
    i.route_cells = 1;
    i.state.short_exit_updates = 9;
    c = prepare_actor_retention(i);
    check(c && c->reason == ActorDeletionReason::short_exit,
          "512 state0 path<=1 ten updates delete");
    i = retention_fixture();
    i.state.kind = ActorKind::monster;
    i.has_encounter = false;
    c = prepare_actor_retention(i);
    check(c && c->reason == ActorDeletionReason::unbound_monster,
          "unbound monster removed except state3");
    i.state.state = 3;
    check(!prepare_actor_retention(i)->delete_instance, "corpse retained until c death lifecycle");
    i = retention_fixture();
    i.state.kind = ActorKind::monster;
    i.state.blocked_updates = 199;
    i.state.flags |= 64;
    c = prepare_actor_retention(i);
    check(c && c->reason == ActorDeletionReason::unbound_monster && c->requests.size() == 1,
          "ab cleanup clears live db then same d removes monster, no stale has_encounter reuse");
    i = retention_fixture();
    i.area_before = false;
    i.state.bad_area_updates = 29;
    i.reported_hp = 0;
    c = prepare_actor_retention(i);
    check(c && !c->delete_instance && c->state.state == 19 && (c->state.flags & 32768U) &&
              c->requests ==
                  std::vector<ActorRetentionRequest>{
                      ActorRetentionRequest::clear_path, ActorRetentionRequest::cleanup,
                      ActorRetentionRequest::mark_escape32768, ActorRetentionRequest::assign_hp1,
                      ActorRetentionRequest::reset_action},
          "bad area30 clear route,r,escape,HP1 if g0,action0 order; recovery not deletion");
    i.state.flags |= 32768;
    c = prepare_actor_retention(i);
    check(c && c->state.bad_area_updates == 0 && c->requests.empty(),
          "escape or aB0 true resets bad area");
    i = retention_fixture();
    i.old_cell_inside_town = true;
    i.state.town_updates = std::numeric_limits<int>::max() - 1;
    check(prepare_actor_retention(i)->state.town_updates == 0, "L modulo max wrap is not overflow");
    i.state.spawn_updates = -1;
    check(!prepare_actor_retention(i), "negative counters reject whole tail");
    for (bool already : {false, true})
        for (bool town : {false, true})
            for (int old_state : {0, 1, 19})
                for (unsigned flags : {2U, 18U}) {
                    i = retention_fixture();
                    i.location_counters_already_advanced = already;
                    i.old_cell_inside_town = town;
                    i.state.state = old_state;
                    i.state.flags = flags;
                    i.state.town_updates = 12;
                    i.state.outside_updates = 34;
                    c = prepare_actor_retention(i);
                    check(c && c->state.town_updates == 12 + (!already && town ? 1 : 0) &&
                              c->state.outside_updates ==
                                  34 + (!already && !town && old_state != 0 && !(flags & 16U) ? 1
                                                                                              : 0),
                          "world prefix already advanced L/M once; tail must not double count");
                }
}
} // namespace
int main() {
    try {
        preparation();
        physics();
        retention();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
