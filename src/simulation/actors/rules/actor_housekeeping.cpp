#include "ark/simulation/actors/rules/actor_housekeeping.hpp"

#include <cmath>
#include <limits>

namespace ark::simulation::rules {
namespace {
bool count(int v) { return v >= 0 && v < std::numeric_limits<int>::max(); }
float acceleration(float height, int duration) {
    return (-2.0F * height) / ((duration - 1) * duration);
}
} // namespace
BattlePreparationCandidate prepare_battle_preparation(const BattlePreparationInput &i) {
    if (i.battle_gate)
        return {BattlePreparationAction::battle, false};
    if (i.cell_in_map && !i.cell_event_flag)
        return {BattlePreparationAction::restore_baseline, false};
    if (!i.encounter)
        return {BattlePreparationAction::restore_baseline, false};
    bool found{};
    for (const auto &e : i.live_encounters)
        if (e.legacy_id == i.encounter->legacy_id && e.center == i.encounter->center) {
            found = true;
            break;
        }
    if (!found || (i.flags & 512U))
        return {BattlePreparationAction::restore_baseline, true};
    return {};
}
std::optional<ActorPhysicsCandidate> prepare_actor_physics(const ActorPhysicsInput &i) {
    if (i.state < 0 || i.state > 20 || !count(i.pause) || !count(i.blocked_battle_steps) ||
        !std::isfinite(i.height) || !std::isfinite(i.vertical_velocity) ||
        !std::isfinite(i.position.x) || !std::isfinite(i.position.z) ||
        !std::isfinite(i.decision_start.x) || !std::isfinite(i.decision_start.z))
        return std::nullopt;
    ActorPhysicsCandidate c{i, false, std::nullopt};
    if (i.pause > 0) {
        --c.state.pause;
        return c;
    }
    if (i.state == 16)
        return c;
    const float gravity = i.state == 10   ? acceleration(14, 7)
                          : i.state == 15 ? acceleration(120, 30)
                          : i.state == 20 ? acceleration(40, 18)
                                          : acceleration(10, 3);
    c.state.vertical_velocity = i.vertical_velocity + gravity;
    c.state.height = i.height + c.state.vertical_velocity;
    if (!std::isfinite(c.state.height) || !std::isfinite(c.state.vertical_velocity))
        return std::nullopt;
    if (c.state.height < 0)
        c.state.height = c.state.vertical_velocity = 0;
    c.query_area_after = true;
    c.state.previous_area_after = i.area_after;
    if (i.state != 0 && !(i.flags & 32768U)) {
        if (i.state == 1) {
            if (i.area_after)
                c.state.blocked_battle_steps = 0;
            else {
                c.diagnostic = 7;
                c.state.position = i.decision_start;
                ++c.state.blocked_battle_steps;
            }
        } else if (i.state == 4 && !i.area_after) {
            c.diagnostic = 8;
            c.state.position = i.decision_start;
        }
    }
    return c;
}
std::optional<ActorDecisionPrefix> prepare_actor_decision_prefix(std::uint32_t flags, int blocked,
                                                                 int cooldown, int hp,
                                                                 int capacity) {
    if (!count(blocked) || !count(cooldown) || capacity < 0)
        return std::nullopt;
    return ActorDecisionPrefix{flags | 2U, blocked >= 150 ? 0 : blocked,
                               cooldown > 0 ? cooldown - 1 : 0, hp < capacity / 2, blocked >= 150};
}
std::optional<ActorRetentionCandidate> prepare_actor_retention(const ActorRetentionInput &i) {
    const auto &s = i.state;
    if ((s.kind != ActorKind::human && s.kind != ActorKind::monster) || s.state < 0 ||
        s.state > 20 || !count(s.town_updates) || !count(s.outside_updates) ||
        !count(s.blocked_updates) || !count(s.spawn_updates) || !count(s.no_path_updates) ||
        !count(s.short_exit_updates) || !count(s.bad_area_updates))
        return std::nullopt;
    ActorRetentionCandidate c{s, false, ActorDeletionReason::none, {}};
    auto &n = c.state;
    bool has_encounter = i.has_encounter;
    const auto cleanup = [&] {
        const auto p = prepare_actor_cleanup(n.kind, n.flags);
        if (!p)
            return false;
        n.flags = p->flags;
        n.state = p->state;
        has_encounter = false;
        c.requests.push_back(ActorRetentionRequest::cleanup);
        return true;
    };
    const auto remove = [&](ActorDeletionReason reason) {
        c.delete_instance = true;
        c.reason = reason;
    };
    if (!i.location_counters_already_advanced) {
        if (i.old_cell_inside_town)
            n.town_updates = (n.town_updates + 1) % std::numeric_limits<int>::max();
        else if (n.state != 0 && !(n.flags & 16U))
            ++n.outside_updates;
    }
    if ((n.flags & 64U) || !(n.flags & 2U)) {
        ++n.blocked_updates;
        if (n.blocked_updates >= 200 && !cleanup())
            return std::nullopt;
    } else
        n.blocked_updates = 0;
    if (i.at_spawn_after_projection) {
        ++n.spawn_updates;
        const int timeout = n.flags & (512U | 1024U) ? 30 : 100;
        if (n.spawn_updates >= timeout) {
            remove(ActorDeletionReason::spawn_timeout);
            return c;
        }
    } else
        n.spawn_updates = 0;
    if (n.state == 0 && i.route_cells == 0) {
        ++n.no_path_updates;
        if (n.no_path_updates >= 60) {
            remove(ActorDeletionReason::empty_route);
            return c;
        }
    } else
        n.no_path_updates = 0;
    if (n.state == 0 && (n.flags & 512U) && i.route_cells <= 1) {
        ++n.short_exit_updates;
        if (n.short_exit_updates >= 10) {
            remove(ActorDeletionReason::short_exit);
            return c;
        }
    } else
        n.short_exit_updates = 0;
    if (n.kind == ActorKind::monster && !has_encounter && n.state != 3) {
        remove(ActorDeletionReason::unbound_monster);
        return c;
    }
    if (i.area_before || (n.flags & 32768U)) {
        n.bad_area_updates = 0;
        return c;
    }
    ++n.bad_area_updates;
    if (n.bad_area_updates >= 30) {
        c.requests.push_back(ActorRetentionRequest::clear_path);
        if (!cleanup())
            return std::nullopt;
        n.flags |= 32768U;
        c.requests.push_back(ActorRetentionRequest::mark_escape32768);
        if (i.reported_hp == 0)
            c.requests.push_back(ActorRetentionRequest::assign_hp1);
        c.requests.push_back(ActorRetentionRequest::reset_action);
    }
    return c;
}
} // namespace ark::simulation::rules
