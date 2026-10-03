#include "dungeon_village_reference/actor_lifecycle.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
bool has(const TimedLifecycleCandidate &c, LifecycleRequestKind kind, int parameter = 0) {
    for (const auto &r : c.requests)
        if (r.kind == kind && r.parameter == parameter)
            return true;
    return false;
}
void timers() {
    TimedLifecycleInput i;
    i.state = 2;
    for (int capacity : {1, 2, 899, 900, 1800, std::numeric_limits<int>::max()})
        for (int hp : {0, 1, capacity})
            for (int counter : {0, 898, 899, 900, 901}) {
                i.hp_capacity = capacity;
                i.hp_slot1 = hp;
                i.old_counter = counter;
                const auto r = prepare_timed_lifecycle(i);
                const auto expected = std::min<std::int64_t>(
                    capacity, static_cast<std::int64_t>(hp) + std::max(capacity / 900, 1));
                check(r.candidate && r.candidate->write_hp_slot1_and3 == expected &&
                          r.candidate->reset_all_hp_to_capacity == (counter >= 900) &&
                          has(*r.candidate, LifecycleRequestKind::restore_baseline) ==
                              (counter >= 900),
                      "old B recovery minimum1 and delayed full-capacity baseline");
            }
    i = {};
    i.state = 3;
    for (int counter : {0, 11, 12, 13})
        for (int parameter : {0, 1}) {
            i.old_counter = counter;
            i.death_parameter = parameter;
            auto r = prepare_timed_lifecycle(i);
            check(r.candidate && r.candidate->delete_instance == (counter >= 12),
                  "death deletion at old B12");
            if (counter >= 12)
                check(r.candidate->requests.size() == 2 &&
                          r.candidate->requests[0].kind ==
                              LifecycleRequestKind::remove_event_member &&
                          r.candidate->requests[1].kind ==
                              (parameter == 0 ? LifecycleRequestKind::normal_death_rewards
                                              : LifecycleRequestKind::cancelled_death_effect),
                      "cancelled death does not produce normal rewards");
        }
    for (int state : {8, 9})
        for (int mode = 0; mode <= 4; ++mode)
            for (int counter : {54, 55, 64, 65, 72, 73, 74}) {
                i.state = state;
                i.monster_mode = mode;
                i.old_counter = counter;
                i.flags = 1 | 16;
                auto r = prepare_timed_lifecycle(i);
                check(r.candidate &&
                          has(*r.candidate, LifecycleRequestKind::ground_effect, 21) ==
                              (counter == 65) &&
                          has(*r.candidate, LifecycleRequestKind::state, 17) == (counter >= 73) &&
                          has(*r.candidate, LifecycleRequestKind::trigger_event, 90) ==
                              (counter >= 73),
                      "appearance dust65 and state17/event90 at73");
                if (counter >= 73)
                    check(
                        !(r.candidate->flags & 17) &&
                            has(*r.candidate, LifecycleRequestKind::wander_event) == (mode == 0) &&
                            has(*r.candidate, LifecycleRequestKind::activity, 7) == (mode == 1) &&
                            has(*r.candidate, LifecycleRequestKind::follow_actor) == (mode == 3) &&
                            has(*r.candidate, LifecycleRequestKind::activity, 8) == (mode == 4),
                        "T0..4 appearance queue exact mapping");
            }
    i = {};
    i.state = 10;
    i.old_counter = 43;
    check(!has(*prepare_timed_lifecycle(i).candidate, LifecycleRequestKind::restore_baseline),
          "win43 waits");
    i.old_counter = 44;
    check(has(*prepare_timed_lifecycle(i).candidate, LifecycleRequestKind::restore_baseline),
          "win44 restores");
    i.state = 12;
    i.old_counter = 65;
    i.flags = 64 | 16;
    auto r = prepare_timed_lifecycle(i);
    check(r.candidate && r.candidate->flags == 2 && r.candidate->requests.size() == 3 &&
              r.candidate->requests[0].kind == LifecycleRequestKind::clear_path &&
              r.candidate->requests[1].kind == LifecycleRequestKind::state &&
              r.candidate->requests[2].kind == LifecycleRequestKind::activity,
          "pickup tail clears path before state0 then queued activity0");
}
void movement_and_exit() {
    TimedLifecycleInput i;
    i.state = 4;
    i.position = {10, 20};
    i.horizontal_velocity = {1, -0.01F};
    i.old_counter = 5;
    auto r = prepare_timed_lifecycle(i);
    check(r.candidate && std::abs(r.candidate->position.x - 10.6F) < 0.0001F &&
              r.candidate->horizontal_velocity.z == 0 && r.candidate->requests.empty(),
          "knockback damping occurs before horizontal move and old B guard");
    i.old_counter = 6;
    i.has_object = true;
    check(has(*prepare_timed_lifecycle(i).candidate, LifecycleRequestKind::restore_baseline),
          "knockback carrying object includes rescue sentinel");
    i.has_object = false;
    check(has(*prepare_timed_lifecycle(i).candidate, LifecycleRequestKind::state, 1),
          "noncarrying knockback enters battle");
    i.state = 15;
    i.old_counter = 60;
    i.height = 40;
    r = prepare_timed_lifecycle(i);
    check(r.candidate && r.candidate->position.x == 11 && r.candidate->height == 0 &&
              r.candidate->horizontal_velocity.x == 0 && r.candidate->zero_vertical_velocity,
          "parabola horizontal move before B60 land and velocity clear");
    i.old_counter = 70;
    check(has(*prepare_timed_lifecycle(i).candidate, LifecycleRequestKind::state, 0) &&
              !has(*prepare_timed_lifecycle(i).candidate, LifecycleRequestKind::activity, 0),
          "human outside town changes state but does not queue activity");
    i.inside_town = true;
    check(has(*prepare_timed_lifecycle(i).candidate, LifecycleRequestKind::activity, 0),
          "human inside town queues activity0");
    i.kind = ActorKind::monster;
    r = prepare_timed_lifecycle(i);
    check(r.candidate->monster_mode == 1 && has(*r.candidate, LifecycleRequestKind::state, 17) &&
              has(*r.candidate, LifecycleRequestKind::activity, 7),
          "parabola monster changes T1 before state17 activity7");
    i.kind = ActorKind::human;
    i.state = 20;
    i.old_counter = 36;
    r = prepare_timed_lifecycle(i);
    check(r.candidate && r.candidate->requests.size() == 3 &&
              r.candidate->requests[0].kind == LifecycleRequestKind::state &&
              r.candidate->requests[0].parameter == 0 &&
              r.candidate->requests[1].kind == LifecycleRequestKind::immediate_activity &&
              r.candidate->requests[2].kind == LifecycleRequestKind::state &&
              r.candidate->requests[2].parameter == 2,
          "dungeon exit must execute o0 between c0 and c2, not append activity0 after down");
    i.state = 11;
    check(prepare_timed_lifecycle(i).error == LifecycleError::unsupported_state,
          "unhandled target-driven state explicitly unsupported");
}
void rescue_and_cleanup() {
    RescueBindingInput i{{1}, 13, -1, true, CharacterId{2}, 2, {30, 40}, false};
    auto r = prepare_rescue_binding(i);
    check(r.candidate && r.candidate->action == RescueBindingAction::chase &&
              !r.candidate->object_slot,
          "non-touching target creates no half binding");
    i.touching = true;
    r = prepare_rescue_binding(i);
    check(r.candidate && r.candidate->action == RescueBindingAction::bind &&
              r.candidate->rescuer_reference == CharacterId{2} &&
              r.candidate->target_reference == CharacterId{1} && r.candidate->object_slot == -2 &&
              r.candidate->target_state == 16 && r.candidate->sound == 7 &&
              r.candidate->requests[2].parameter == 4,
          "two refs, sentinel, rescued state16 and rescuer activity4 in one candidate");
    i.target_state = 0;
    check(prepare_rescue_binding(i).error == LifecycleError::stale_target,
          "stale down target cannot bind");
    i.object_slot = -2;
    check(prepare_rescue_binding(i).candidate->action == RescueBindingAction::baseline,
          "occupied rescue sentinel causes baseline before target use");
    for (auto kind : {ActorKind::human, ActorKind::monster})
        for (unsigned flags : {0U, 1U, 16U, 32U, 64U, 1024U, 32768U, 65535U}) {
            auto c = prepare_actor_cleanup(kind, flags);
            check(c && c->state == (kind == ActorKind::human ? 19 : 0) &&
                      c->flags == (((flags & ~97U) | 514U) & ~16U) &&
                      c->waiting_updates ==
                          (kind == ActorKind::human && (flags & 1024) ? 120 : 0) &&
                      c->activity == 5,
                  "r cleanup is staged leave not immediate removal or universal120 wait");
        }
}
} // namespace
int main() {
    timers();
    movement_and_exit();
    rescue_and_cleanup();
    std::cout << checks << " checks passed\n";
}
