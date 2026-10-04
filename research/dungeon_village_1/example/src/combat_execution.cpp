#include "dungeon_village_reference/combat_execution.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dungeon_village_reference {
namespace {
bool point_valid(CombatPoint p) {
    return std::isfinite(p.x) && std::isfinite(p.height) && std::isfinite(p.z) &&
           std::abs(p.x) <= 1000000 && std::abs(p.height) <= 1000000 && std::abs(p.z) <= 1000000;
}
float distance(CombatPoint a, CombatPoint b) {
    const float dx = a.x - b.x, dz = a.z - b.z;
    return static_cast<float>(std::sqrt(static_cast<double>(dx * dx + dz * dz)));
}
int map_int(int value, int low, int high, int begin, int end) {
    value = std::clamp(value, low, high);
    return static_cast<int>(begin +
                            static_cast<std::int64_t>(value - low) * (end - begin) / (high - low));
}
float launch_velocity(float span, int duration) {
    return duration == 1 ? 0.0F : 2.0F * span / (duration - 1);
}
float launch_acceleration(float span, int duration) {
    return duration <= 1 ? 0.0F : -2.0F * span / ((duration - 1) * duration);
}
float curved_span(float span, int duration, int tick) {
    return launch_velocity(span, duration) * tick +
           launch_acceleration(span, duration) * tick * (tick + 1) / 2.0F;
}
float outward_height(int tick) {
    return curved_span(20.0F, 3, tick); // n.h init: d.a(20,7,i) divides7 by2.
}
float return_height(int tick) {
    const float initial = outward_height(5);
    return initial + std::min(tick, 4) * (0.0F - initial) / 4.0F;
}
CombatPoint add(CombatPoint a, CombatPoint b) {
    return {a.x + b.x, a.height + b.height, a.z + b.z};
}
bool box_valid(CollisionBox b) {
    return point_valid({b.x_offset, b.width, b.z_offset}) && std::isfinite(b.depth) &&
           b.width >= 0 && b.depth >= 0 && b.depth <= 1000000;
}
bool collision(CombatPoint a, CollisionBox ab, CombatPoint b, CollisionBox bb) {
    const float ax = a.x + ab.x_offset, az = a.z + ab.z_offset;
    const float bx = b.x + bb.x_offset, bz = b.z + bb.z_offset;
    return ax + ab.width >= bx && ax <= bx + bb.width && az >= bz - bb.depth && az - ab.depth <= bz;
}
bool projectile_valid(const ProjectileState &s) {
    return (s.kind == ProjectileKind::arrow || s.kind == ProjectileKind::spell ||
            s.kind == ProjectileKind::delayed_damage) &&
           point_valid(s.position) && point_valid(s.velocity) && point_valid(s.acceleration) &&
           s.facing >= 0 && s.facing < 4 && s.counter >= 0 && s.counter <= 100 && s.delay >= 0 &&
           s.damage >= 0 && (s.kind != ProjectileKind::spell || (s.effect >= 4 && s.effect <= 9));
}
int magic_delay(int effect) {
    constexpr int first[] = {6, 12, 4}, total[] = {19, 24, 11};
    return effect >= 7 ? total[effect - 7] : first[effect - 4];
}
bool fits(std::int64_t n) {
    return n >= std::numeric_limits<int>::min() && n <= std::numeric_limits<int>::max();
}
} // namespace

AttackSetupResult prepare_human_attack(const AttackSetupInput &i) {
    if (i.weapon_kind < 0 || i.weapon_kind > 3 || i.weapon_combo < 1 || i.weapon_combo > 1000000 ||
        i.miss_low < 0 || i.miss_low > 100 || i.miss_high < 0 || i.miss_high > 100)
        return {CombatAiError::invalid_input, std::nullopt};
    auto tickets = i.tickets;
    for (auto &ticket : tickets) {
        if (i.draw) {
            const auto drawn = i.draw(100);
            if (!drawn)
                return {CombatAiError::missing_ticket, std::nullopt};
            ticket = *drawn;
        }
        if (ticket < 0 || ticket >= 100)
            return {CombatAiError::invalid_ticket, std::nullopt};
    }
    const int dexterity = map_int(i.dexterity, 0, 500, 0, 100);
    int count = map_int(dexterity, 0, 100, 1, i.weapon_combo);
    count += tickets[0] < 10 ? 2 : 0;
    count += tickets[1] < 50 ? 1 : 0;
    count -= tickets[2] < 30 ? 1 : 0;
    count -= tickets[3] < 10 ? 2 : 0;
    count = std::max(count, 1);
    if (i.boosted)
        count = count == 1 ? 2 : static_cast<int>(count * 1.7F);
    const bool miss = tickets[4] < map_int(dexterity, 0, 100, i.miss_low, i.miss_high);
    if (miss)
        count = 1;
    constexpr int actions[] = {1, 2, 3, 1};
    return {CombatAiError::none,
            AttackSetupCandidate{actions[i.weapon_kind],
                                 count,
                                 miss,
                                 {{3, actions[i.weapon_kind]}, {14}, {3, 0}, {1, 10, 0}, {7, 4}}}};
}
HumanAttackResult prepare_human_attack_frame(const HumanAttackFrame &i) {
    if (i.weapon_kind < 0 || i.weapon_kind > 3 || i.counter < 0 || i.counter > 1000000 ||
        i.combo_index < 0 || i.combo_count < 1 || i.combo_index >= i.combo_count ||
        !std::isfinite(i.enemy_distance) || i.enemy_distance < 0 || i.weapon_range < 0 ||
        (i.action && (*i.action < 0 || *i.action > 11)))
        return {CombatAiError::invalid_input, std::nullopt};
    HumanAttackCandidate c{i.counter + (i.combo_index > 0 ? 1 : 0), i.combo_index, i.armed};
    constexpr int actions[] = {1, 2, 3, 1};
    constexpr int first[] = {4, 2, 8, 6, 26, 1, 24, 60, 4, 300, 12, 300};
    constexpr int last[] = {16, 12, 14, 12, 42, 16, 24, 60, 16, 300, 18, 300};
    const int action = i.action.value_or(actions[i.weapon_kind]);
    c.query_enemy = i.weapon_kind != 1 ? c.armed && c.counter >= 5 && c.counter <= 8
                                       : c.counter == first[action];
    if (i.weapon_kind != 1 && c.query_enemy && i.enemy_found && i.enemy_distance < i.weapon_range) {
        c.armed = false;
        c.request_damage = true;
    }
    if (i.weapon_kind == 1 && c.counter == first[action]) {
        if (!i.enemy_found)
            return {CombatAiError::none, c}; // L535 bypasses completion/restart on this one frame.
        c.request_arrow = true;
    }
    if (c.combo_index >= i.combo_count - 1)
        c.completed = c.counter >= last[action];
    else if (c.counter >= last[action] - 5) {
        c.counter = 0;
        ++c.combo_index;
        c.armed = true;
    }
    return {CombatAiError::none, c};
}
std::optional<SpellFrameCandidate> prepare_spell_frame(int counter, bool target_found, int action) {
    if (counter < 0 || counter > 1000000 || action < 0 || action > 11)
        return std::nullopt;
    constexpr int last[] = {16, 12, 14, 12, 42, 16, 24, 60, 16, 300, 18, 300};
    return SpellFrameCandidate{counter == 1, counter == 26, counter == 26 && target_found,
                               counter >= last[action] && (counter != 26 || target_found)};
}
DamageResult prepare_healing_amount(int magic, std::optional<int> ticket,
                                    const CombatRandomDraw &draw) {
    if (magic < 0 || magic > 1000000)
        return {CombatAiError::invalid_input, std::nullopt};
    const int span = std::max(magic * 2 / 10, 2);
    if (!ticket && draw)
        ticket = draw(span);
    if (!ticket)
        return {CombatAiError::missing_ticket, std::nullopt};
    if (*ticket < 0 || *ticket >= span)
        return {CombatAiError::invalid_ticket, std::nullopt};
    return {CombatAiError::none, DamageCandidate{magic, span, magic + *ticket - span / 2}};
}
MonsterAttackResult prepare_monster_attack_frame(const MonsterAttackFrame &i) {
    if ((i.action != 3 && i.action != 6) || i.counter < 0 || i.counter > 1000000 || i.range < 0 ||
        !point_valid(i.origin) || !point_valid(i.destination) || !point_valid(i.previous_position))
        return {CombatAiError::invalid_input, std::nullopt};
    for (const auto &h : i.humans)
        if (!point_valid(h.position))
            return {CombatAiError::invalid_input, std::nullopt};
    MonsterAttackCandidate c{i.origin, i.armed, std::nullopt,
                             i.counter >= (i.action == 3 ? 56 : 30)};
    const int start = i.action == 3 ? 34 : 12, end = i.action == 3 ? 42 : 20;
    if (i.armed && i.counter >= start && i.counter <= end) {
        if (i.preferred)
            for (const auto &h : i.humans)
                if (h.id == *i.preferred && distance(i.previous_position, h.position) < i.range) {
                    c.damage_target = h.id;
                    break;
                }
        if (!c.damage_target)
            for (const auto &h : i.humans) {
                const auto manhattan =
                    std::abs(static_cast<std::int64_t>(h.half_cell.x) - i.half_cell.x) +
                    std::abs(static_cast<std::int64_t>(h.half_cell.y) - i.half_cell.y);
                if (h.eligible && manhattan <= 5 &&
                    distance(i.previous_position, h.position) < i.range) {
                    c.damage_target = h.id;
                    break;
                }
            }
        if (c.damage_target)
            c.armed = false;
    }
    // au movement is AFTER hit lookup; n itself is left unchanged.
    if (i.action == 3 && i.counter < 10)
        c.position.height =
            static_cast<float>(std::max(static_cast<int>(curved_span(20.0F, 5, i.counter)), 0));
    const int outbound = i.action == 3 ? 28 : 6;
    if (i.counter >= outbound && i.counter < start) {
        const int t = i.counter - outbound, duration = start - outbound;
        if (i.action == 3) {
            c.position.x += (i.destination.x - i.origin.x) * t / duration;
            c.position.z += (i.destination.z - i.origin.z) * t / duration;
        } else {
            c.position.x += curved_span(i.destination.x - i.origin.x, duration, t);
            c.position.z += curved_span(i.destination.z - i.origin.z, duration, t);
        }
        c.position.height = outward_height(t);
    } else if (i.counter >= start && i.counter < end) {
        const int t = i.counter - start, duration = end - start;
        c.position = i.destination;
        if (i.action == 3) {
            c.position.x += (i.origin.x - i.destination.x) * t / duration;
            c.position.z += (i.origin.z - i.destination.z) * t / duration;
        } else {
            c.position.x += curved_span(i.origin.x - i.destination.x, duration, t);
            c.position.z += curved_span(i.origin.z - i.destination.z, duration, t);
        }
        c.position.height = return_height(t);
    }
    if (!point_valid(c.position))
        return {CombatAiError::invalid_input, std::nullopt};
    return {CombatAiError::none, c};
}
ProjectileResult prepare_projectile(ProjectileKind kind, CharacterId caster, CharacterId target,
                                    CombatPoint origin, CombatPoint destination, int facing,
                                    int effect, int damage, int delay) {
    ProjectileState s;
    s.kind = kind;
    s.caster = caster;
    s.original_target = target;
    s.facing = facing;
    s.effect = effect;
    s.damage = damage;
    s.delay = delay;
    if (!projectile_valid(s) || !point_valid(origin) || !point_valid(destination))
        return {CombatAiError::invalid_input, std::nullopt};
    if (kind != ProjectileKind::delayed_damage) {
        const float dx = destination.x - origin.x, dz = destination.z - origin.z;
        const float length = distance(origin, destination);
        if (length == 0)
            return {CombatAiError::invalid_input, std::nullopt};
        const float speed = kind == ProjectileKind::arrow ? 17.0F : 10.0F;
        constexpr int heights[] = {4, 12, 12, 4};
        s.position = {origin.x,
                      kind == ProjectileKind::arrow ? static_cast<float>(heights[facing]) : 10.0F,
                      origin.z};
        s.velocity = {dx * speed / length, 0, dz * speed / length};
        if (kind == ProjectileKind::spell) {
            const int travel = s.velocity.x == 0 ? 0 : static_cast<int>(dx / s.velocity.x);
            const int duration = (std::max(travel, 4) + 4) / 2;
            if (duration > 46340)
                return {CombatAiError::invalid_input, std::nullopt};
            s.velocity.height = launch_velocity(30.0F, duration);
            s.acceleration.height = launch_acceleration(30.0F, duration);
        }
    }
    return {CombatAiError::none, s};
}
ProjectileStepResult advance_projectile(const ProjectileState &s, const ProjectileContext &i) {
    if (!projectile_valid(s))
        return {CombatAiError::invalid_input, std::nullopt};
    ProjectileStepCandidate c;
    c.state = s;
    if (!i.caster_reference || !i.original_target_reference) {
        c.remove = true;
        return {CombatAiError::none, c};
    }
    if (!box_valid(i.box))
        return {CombatAiError::invalid_input, std::nullopt};
    for (const auto &m : i.monsters)
        if (!point_valid(m.position) || !box_valid(m.box))
            return {CombatAiError::invalid_input, std::nullopt};
    c.state.velocity = add(s.velocity, s.acceleration);
    c.state.position = add(s.position, c.state.velocity);
    if (!point_valid(c.state.velocity) || !point_valid(c.state.position))
        return {CombatAiError::invalid_input, std::nullopt};
    const bool landed = s.kind == ProjectileKind::spell && c.state.position.height < 0;
    if (landed)
        c.state.position.height = 0;
    if (s.kind == ProjectileKind::arrow || landed)
        for (const auto &m : i.monsters)
            if (collision(c.state.position, i.box, m.position, m.box)) {
                c.remove = true;
                if (s.kind == ProjectileKind::arrow) {
                    c.damage_target = m.id;
                    c.physical_damage = true;
                    c.contact_effect = true;
                } else {
                    auto spawned =
                        prepare_projectile(ProjectileKind::delayed_damage, s.caster, m.id, {}, {},
                                           0, 0, s.damage, magic_delay(s.effect));
                    c.spawned = spawned.candidate;
                    c.visual_effect = s.effect;
                }
                return {CombatAiError::none, c};
            }
    if (landed) {
        c.ground_effect22 = true;
        c.remove = true;
    } else if (s.kind == ProjectileKind::delayed_damage && s.counter >= s.delay) {
        c.damage_target = s.original_target;
        c.remove = true;
    } else if (s.kind == ProjectileKind::arrow && c.state.position.height < 0) {
        c.remove = true;
    } else {
        ++c.state.counter;
        c.remove = c.state.counter >= 100;
    }
    return {CombatAiError::none, c};
}
HitResult prepare_hit(const HitTargetState &s, int damage, const HitContext &i) {
    if (damage < 0 || s.capacity < 0 || s.damage_total < 0 || s.hit_count < 0 ||
        s.label_timer < 0 || i.global_down_count < 0 || i.weapon_kind < 0 || i.weapon_kind > 3 ||
        i.monster_stat1 < 0 || i.monster_statF < 0 || i.target_action < 0 || i.target_action > 11 ||
        i.victim_participant_matches < 0 || i.killer_participant_matches < 0 ||
        (s.kind != ActorKind::human && s.kind != ActorKind::monster) ||
        (i.attacker_kind != ActorKind::human && i.attacker_kind != ActorKind::monster))
        return {CombatAiError::invalid_input, std::nullopt};
    HitCandidate c{s, false, false, false, {}};
    c.target.hit_flash = 7;
    auto request = [&c](HitRequestKind k, int p = 0) { c.requests.push_back({k, p}); };
    if (i.attacker_miss) {
        if (s.label_timer <= 0) {
            c.target.label_timer = 16;
            c.target.miss_label = true;
        }
        request(HitRequestKind::face_attacker);
        return {CombatAiError::none, c};
    }
    const auto hp = prepare_hp_change(s.hp, -damage, s.capacity);
    if (!hp.candidate || !fits(static_cast<std::int64_t>(s.damage_total) + damage) ||
        !fits(static_cast<std::int64_t>(s.hit_count) + 1))
        return {CombatAiError::invalid_input, std::nullopt};
    c.target.hp = *hp.candidate;
    c.target.damage_total += damage;
    ++c.target.hit_count;
    c.target.label_timer = 16;
    c.target.miss_label = false;
    c.landed = true;
    if (i.attacker_visible) {
        const int sound = i.attacker_kind == ActorKind::monster ? 13
                          : i.weapon_kind == 0                  ? 14
                          : i.weapon_kind == 2                  ? 15
                                                                : 12;
        request(HitRequestKind::attack_sound, sound);
    }
    c.lethal = c.target.hp.target <= 0; // NOT interpolated g().
    if (c.lethal) {
        c.target.flags &= ~128U;
        if (s.kind == ActorKind::human) {
            if (!fits(static_cast<std::int64_t>(i.global_down_count) + 1))
                return {CombatAiError::invalid_input, std::nullopt};
            request(HitRequestKind::state, 2);
            request(HitRequestKind::expression, 2); // Delay16 documented at owner boundary.
            request(HitRequestKind::reset_down_timer);
            if (i.target_carries_rescued_actor) {
                if (i.rescue_reference)
                    request(HitRequestKind::drop_rescued_actor, 2);
                request(HitRequestKind::clear_rescue_links);
            }
            c.target.flags &= ~2048U;
            if (i.victim_participant_matches > 0)
                request(HitRequestKind::participant_down_count, i.victim_participant_matches);
            request(HitRequestKind::global_down_count, 1);
            if (i.global_down_count + 1 >= 5 && !i.event131_present)
                request(HitRequestKind::event131, 131);
            request(HitRequestKind::clear_recent_reward_and_kills);
            request(HitRequestKind::monster_human_kills, 1);
        } else {
            const auto drop_ticket = i.drop_ticket ? i.drop_ticket
                                     : i.draw      ? i.draw(100)
                                                   : std::optional<int>{};
            if (!drop_ticket)
                return {CombatAiError::missing_ticket, std::nullopt};
            if (*drop_ticket < 0 || *drop_ticket >= 100)
                return {CombatAiError::invalid_ticket, std::nullopt};
            if (!i.target_attack_locked)
                request(HitRequestKind::copy_attack_position);
            request(HitRequestKind::state, 3);
            request(HitRequestKind::kill_count, 1);
            request(HitRequestKind::kill_stat1, i.monster_stat1);
            request(HitRequestKind::kill_statF, i.monster_statF);
            request(HitRequestKind::record_monster);
            if (i.task_encounter && i.killer_participant_matches > 0)
                request(HitRequestKind::participant_task_kills, i.killer_participant_matches);
            c.consumed_drop_ticket = true;
            if (*drop_ticket < 6 && !i.attacker_first_visit)
                request(HitRequestKind::spawn_drop);
            if (i.boss_flags4 && i.monster_rank == 5 && !i.event217_present)
                request(HitRequestKind::event217, 217);
            request(HitRequestKind::global_monster_record);
        }
    } else {
        c.target.flags &= ~16U;
        if (s.kind == ActorKind::monster && (s.flags & 16384U) &&
            (i.target_action == 7 ? 0 : c.target.hp.target) < s.capacity / 3)
            c.target.flags |= 4096U;
    }
    return {CombatAiError::none, c};
}
} // namespace dungeon_village_reference
