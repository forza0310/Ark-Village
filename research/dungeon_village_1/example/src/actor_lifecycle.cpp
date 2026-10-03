#include "dungeon_village_reference/actor_lifecycle.hpp"

#include <algorithm>
#include <cmath>

namespace dungeon_village_reference {
TimedLifecycleResult prepare_timed_lifecycle(const TimedLifecycleInput &i) {
    if ((i.kind != ActorKind::human && i.kind != ActorKind::monster) || i.old_counter < 0 ||
        i.monster_mode < 0 || i.monster_mode > 4 || i.hp_capacity <= 0 || i.hp_slot1 < 0 ||
        i.hp_slot1 > i.hp_capacity || !character_world_cell(i.position) ||
        !std::isfinite(i.horizontal_velocity.x) || !std::isfinite(i.horizontal_velocity.z) ||
        !std::isfinite(i.height))
        return {LifecycleError::invalid_input, std::nullopt};
    TimedLifecycleCandidate c;
    c.position = i.position;
    c.horizontal_velocity = i.horizontal_velocity;
    c.height = i.height;
    c.flags = i.flags;
    const auto request = [&](LifecycleRequestKind kind, int value = 0) {
        c.requests.push_back({kind, value});
    };
    const auto transition = [&](int state) {
        c.flags &= ~16U;
        if (state == 1)
            c.flags &= ~4U;
        request(LifecycleRequestKind::state, state);
    };
    const auto move = [&] {
        c.position.x += c.horizontal_velocity.x;
        c.position.z += c.horizontal_velocity.z;
    };
    switch (i.state) {
    case 2: {
        request(LifecycleRequestKind::expression, 3);
        const auto recovery =
            static_cast<std::int64_t>(i.hp_slot1) + std::max(i.hp_capacity / 900, 1);
        c.write_hp_slot1_and3 = static_cast<int>(std::min<std::int64_t>(recovery, i.hp_capacity));
        if (i.old_counter >= 900) {
            request(LifecycleRequestKind::expression, 4);
            c.reset_all_hp_to_capacity = true;
            c.flags &= ~16U;
            request(LifecycleRequestKind::restore_baseline);
        }
        break;
    }
    case 3:
        if (i.death_parameter < 0 || i.death_parameter > 1)
            return {LifecycleError::invalid_input, std::nullopt};
        if (i.old_counter >= 12) {
            request(LifecycleRequestKind::remove_event_member);
            request(i.death_parameter == 0 ? LifecycleRequestKind::normal_death_rewards
                                           : LifecycleRequestKind::cancelled_death_effect);
            c.delete_instance = true;
        }
        break;
    case 4:
        for (float *v : {&c.horizontal_velocity.x, &c.horizontal_velocity.z}) {
            *v *= 0.6F;
            if (std::abs(*v) < 0.01F)
                *v = 0;
        }
        move();
        if (i.old_counter >= 6) {
            if (i.has_object) {
                c.flags &= ~16U;
                request(LifecycleRequestKind::restore_baseline);
            } else
                transition(1);
        }
        break;
    case 8:
    case 9:
        if (i.old_counter == 65)
            request(LifecycleRequestKind::ground_effect, 21);
        if (i.old_counter >= 73) {
            c.flags &= ~1U;
            if (!i.event90_seen)
                request(LifecycleRequestKind::trigger_event, 90);
            transition(17);
            if (i.monster_mode == 0)
                request(LifecycleRequestKind::wander_event);
            else if (i.monster_mode == 1)
                request(LifecycleRequestKind::activity, 7);
            else if (i.monster_mode == 3)
                request(LifecycleRequestKind::follow_actor);
            else if (i.monster_mode == 4)
                request(LifecycleRequestKind::activity, 8);
        }
        break;
    case 10:
        if (i.old_counter >= 44) {
            c.flags &= ~16U;
            request(LifecycleRequestKind::restore_baseline);
        }
        break;
    case 12:
        if (i.old_counter >= 65) {
            c.flags = (c.flags & ~64U) | 2U;
            request(LifecycleRequestKind::clear_path);
            transition(0);
            request(LifecycleRequestKind::activity, 0);
        }
        break;
    case 15:
        move();
        if (i.old_counter == 15)
            request(LifecycleRequestKind::expression, 16);
        if (i.old_counter == 60) {
            c.height = 0;
            c.horizontal_velocity = {};
            c.zero_vertical_velocity = true;
        }
        if (i.old_counter >= 70) {
            if (i.kind == ActorKind::human) {
                transition(0);
                if (i.inside_town)
                    request(LifecycleRequestKind::activity, 0);
            } else {
                c.monster_mode = 1;
                transition(17);
                request(LifecycleRequestKind::activity, 7);
            }
        }
        break;
    case 20:
        move();
        if (i.old_counter >= 36) {
            c.height = 0;
            c.horizontal_velocity = {};
            c.zero_vertical_velocity = true;
            transition(0);
            request(LifecycleRequestKind::immediate_activity, 0);
            transition(2);
        }
        break;
    default:
        return {LifecycleError::unsupported_state, std::nullopt};
    }
    if (!character_world_cell(c.position))
        return {LifecycleError::invalid_input, std::nullopt};
    return {LifecycleError::none, c};
}
RescueBindingResult prepare_rescue_binding(const RescueBindingInput &i) {
    if (i.rescuer.value == 0 || i.rescuer_state != 13 || i.object_slot < -2 ||
        !character_world_cell(i.target_position) ||
        (i.target && (i.target->value == 0 || *i.target == i.rescuer)))
        return {LifecycleError::invalid_input, std::nullopt};
    RescueBindingCandidate c;
    if (!i.target || i.object_slot != -1 || !i.rescue_enabled) {
        c.requests.push_back({LifecycleRequestKind::restore_baseline, 0});
        return {LifecycleError::none, c};
    }
    if (i.target_state != 2)
        return {LifecycleError::stale_target, std::nullopt};
    if (!i.touching) {
        c.action = RescueBindingAction::chase;
        return {LifecycleError::none, c};
    }
    c.action = RescueBindingAction::bind;
    c.rescuer_reference = i.target;
    c.target_reference = i.rescuer;
    c.object_slot = -2;
    c.target_state = 16;
    c.sound = 7;
    c.requests = {{LifecycleRequestKind::clear_path, 0},
                  {LifecycleRequestKind::state, 0},
                  {LifecycleRequestKind::activity, 4},
                  {LifecycleRequestKind::expression, 13}};
    return {LifecycleError::none, c};
}
std::optional<CleanupCandidate> prepare_actor_cleanup(ActorKind kind, std::uint32_t flags) {
    if (kind != ActorKind::human && kind != ActorKind::monster)
        return std::nullopt;
    CleanupCandidate c;
    c.state = kind == ActorKind::human ? 19 : 0;
    c.flags = ((flags & ~97U) | 514U) & ~16U;
    c.waiting_updates = kind == ActorKind::human && (flags & 1024U) ? 120 : 0;
    return c;
}
} // namespace dungeon_village_reference
