#include "dungeon_village_reference/actor_control.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_reference {
bool valid_actor_control(const LegacyActorControl &c) {
    static constexpr std::size_t sizes[34] = {3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1,
                                              1, 3, 4, 1, 1, 3, 1, 1, 2, 1, 3, 2, 3, 3, 1, 1, 1};
    if (c.empty() || c[0] < 0 || c[0] >= 34 || c.size() < sizes[c[0]])
        return false;
    if (c[0] == 1 && (c[1] < 0 || (c[2] != 0 && c[2] != 1)))
        return false;
    if (c[0] == 2 && (c[1] < 0 || c[1] > 20))
        return false;
    if (c[0] == 3 && (c[1] < 0 || c[1] > 11))
        return false;
    if (c[0] == 4 && (c[1] < 0 || c[1] > 3))
        return false;
    if (c[0] == 8 && (c[1] < 0 || c[1] > 8))
        return false;
    return true;
}
std::optional<ActorBaselineCandidate>
prepare_actor_baseline_restore(const ActorControlState &s, int baseline, bool human, int mode) {
    if (baseline < 0 || baseline > 20 || mode < 0 || mode > 4 || s.state < 0 || s.state > 20 ||
        s.action < 0 || s.action > 11 || s.action_counter < 0 || s.alternate_counter < 0 ||
        s.facing < 0 || s.facing > 3)
        return std::nullopt;
    ActorBaselineCandidate c{s, human};
    c.control.state = baseline;
    c.control.flags &= ~16U;
    c.control.queue.clear();
    c.control.action = c.control.action_counter = 0; // b() calls n(0), not opcode3's extra i=0.
    if (baseline == 5)
        c.control.queue = {{10, 0}};
    else if (baseline == 17 && mode == 0)
        c.control.queue = {{12}};
    else if (baseline == 17 && mode == 3)
        c.control.queue = {{13}};
    return c;
}
ActorControlResult prepare_local_control_prefix(const ActorControlState &s,
                                                const ActorControlContext &context) {
    if (s.state < 0 || s.state > 20 || s.action < 0 || s.action > 11 || s.action_counter < 0 ||
        s.alternate_counter < 0 || s.facing < 0 || s.facing > 3)
        return {ActorControlError::invalid_state, std::nullopt};
    for (const auto &command : s.queue)
        if (!valid_actor_control(command))
            return {ActorControlError::malformed_command, std::nullopt};
    ActorControlCandidate c;
    c.state = s;
    while (!c.state.queue.empty()) {
        auto &command = c.state.queue.front();
        switch (command[0]) {
        case 0:
            if (!context.move_arrived || c.consumed_motion_result) {
                c.flow = ActorControlFlow::delegated;
                return {ActorControlError::none, c};
            }
            c.consumed_motion_result = true;
            if (!*context.move_arrived) {
                c.flow = ActorControlFlow::moving;
                return {ActorControlError::none, c};
            }
            break;
        case 1:
            if (command[2] == 1)
                c.state.flags |= 1U;
            --command[1];
            if (command[1] > 0) {
                c.flow = ActorControlFlow::waiting;
                return {ActorControlError::none, c};
            }
            break;
        case 3:
            c.state.action = command[1];
            c.state.action_counter = 0;
            c.state.alternate_counter = 0;
            break;
        case 4:
            c.state.facing = command[1];
            break;
        case 5:
            c.state.flags = static_cast<std::uint32_t>(command[1]);
            break;
        case 6:
            c.state.flags |= static_cast<std::uint32_t>(command[1]);
            break;
        case 7:
            c.state.flags &= ~static_cast<std::uint32_t>(command[1]);
            break;
        case 8:
            if (context.departure_succeeded && *context.departure_succeeded) {
                c.state.queue.erase(c.state.queue.begin());
                ++c.removed_commands;
                c.state.state = 0; // Direct A write, NOT c(0): no B/D/queue reset here.
                c.flow = ActorControlFlow::departure_started;
                return {ActorControlError::none, c};
            }
            c.flow = ActorControlFlow::delegated;
            return {ActorControlError::none, c};
        case 9:
        case 11:
        case 20:
        case 31:
            break;
        default:
            c.flow = ActorControlFlow::delegated;
            return {ActorControlError::none, c};
        }
        c.state.queue.erase(c.state.queue.begin());
        ++c.removed_commands;
    }
    return {ActorControlError::none, c};
}

std::optional<ActorStateTransitionCandidate>
prepare_actor_state_transition(const ActorStateTransitionInput &i) {
    const auto &s = i.control;
    if (i.next_state < 0 || i.next_state > 20 || i.baseline < 0 || i.baseline > 20 || s.state < 0 ||
        s.state > 20 || s.action < 0 || s.action > 11 || s.action_counter < 0 ||
        s.alternate_counter < 0 || s.facing < 0 || s.facing > 3 || i.legacy_u < 0 ||
        i.legacy_u > 100)
        return std::nullopt;
    ActorStateTransitionCandidate c;
    c.control = s;
    c.baseline =
        (i.next_state == 0 || i.next_state == 5 || i.next_state == 17) ? i.next_state : i.baseline;
    c.clear_encounter = i.human && i.next_state != 1 && i.next_state != 18;
    c.control.state = i.next_state;
    c.control.alternate_counter = 0;
    c.control.flags &= ~16U;
    c.control.queue.clear();
    const auto action = [&](int value) {
        c.control.action = value;
        c.control.action_counter = 0;
    };
    switch (i.next_state) {
    case 0:
    case 4:
    case 5:
    case 11:
    case 12:
    case 13:
        action(0);
        break;
    case 1:
        c.control.flags &= ~4U;
        break;
    case 2:
        action(7);
        break;
    case 3:
        c.copy_attack_position = s.action == 3 || s.action == 6;
        action(9);
        break;
    case 10:
        if (i.current_facility_category == 2) {
            c.request_cleanup = true;
            break;
        }
        c.control.flags &= ~2048U;
        c.control.queue = {{1, 5, 0}, {32}, {3, 6}, {1, 14, 0}, {3, 0}};
        break;
    case 18:
        c.reset_attack_count = true;
        action(0);
        c.control.queue = {{10, 1}};
        if (!(c.control.flags & 2048U)) {
            if (!i.boost_ticket || *i.boost_ticket < 0 || *i.boost_ticket >= 100)
                return std::nullopt;
            c.consumed_boost_ticket = true;
            if (*i.boost_ticket < i.legacy_u * 12 / 100) {
                c.control.flags |= 2048U;
                c.request_boost_event116 = !i.boost_event116_seen;
            }
        }
        break;
    default:
        break; // These states preserve k/l after resetting i/B/C and the queue.
    }
    return c;
}
FailedActivityCandidate prepare_failed_activity(std::uint32_t flags) {
    FailedActivityCandidate c{flags, false, false, true};
    if (flags & 1024U) {
        c.expression18 = true;
        if (flags & 32768U) {
            c.delete_instance = true;
            c.cleanup = false;
            return c;
        }
    }
    if (flags & 512U) {
        c.flags |= 1024U;
        c.flags &= ~66U;
    }
    return c;
}

std::optional<ActorWanderCandidate> prepare_actor_wander(const ActorWanderInput &i) {
    if (i.opcode != 10 && i.opcode != 12 && i.opcode != 13)
        return std::nullopt;
    ActorWanderCandidate c;
    if (i.opcode != 10 && !i.center)
        return c;
    if (i.width <= 0 || i.height <= 0 ||
        static_cast<std::uint64_t>(i.width) * i.height != i.cells.size() ||
        i.cells.size() > 1000000)
        return std::nullopt;
    const Position center = i.opcode == 10 ? i.actor : *i.center;
    if (center.x < 0 || center.y < 0 || center.x >= i.width || center.y >= i.height)
        return std::nullopt;
    static constexpr Position four[]{{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    static constexpr Position eight[]{{-1, 1}, {0, 1},   {1, 1},  {-1, 0},
                                      {1, 0},  {-1, -1}, {0, -1}, {1, -1}};
    const auto consider = [&](Position offset) {
        const std::int64_t x = static_cast<std::int64_t>(center.x) + offset.x;
        const std::int64_t y = static_cast<std::int64_t>(center.y) + offset.y;
        if (x < 0 || y < 0 || x >= i.width || y >= i.height)
            return;
        const auto &cell = i.cells[static_cast<std::size_t>(y) * i.width + x];
        if (cell.state == 4 && (i.opcode == 13 || !cell.inside_town) &&
            (i.opcode != 10 || i.parameter != 1 || (cell.flags & 2U)))
            c.cells.push_back({static_cast<int>(x), static_cast<int>(y)});
    };
    if (i.opcode == 10)
        for (const auto offset : four)
            consider(offset);
    else
        for (const auto offset : eight)
            consider(offset);
    const auto draw = [&](int bound) -> std::optional<int> {
        if (c.consumed_tickets >= i.tickets.size())
            return std::nullopt;
        const int ticket = i.tickets[c.consumed_tickets++];
        if (ticket < 0 || ticket >= bound)
            return std::nullopt;
        return ticket;
    };
    if (!c.cells.empty()) {
        const auto cell = draw(static_cast<int>(c.cells.size()));
        const auto x = draw(80);
        const auto z = draw(80);
        if (!cell || !x || !z)
            return std::nullopt;
        const Position p = c.cells[static_cast<std::size_t>(*cell)];
        const float world_x = p.x * 100.0F + static_cast<float>(*x + 10);
        const float world_z = p.y * 100.0F + static_cast<float>(*z + 10);
        if (static_cast<double>(world_x) > std::numeric_limits<int>::max() ||
            static_cast<double>(world_z) > std::numeric_limits<int>::max())
            return std::nullopt;
        c.append.push_back({0, static_cast<int>(world_x), static_cast<int>(world_z)});
        const auto wait = draw(i.opcode == 13 ? 10 : 100);
        if (!wait)
            return std::nullopt;
        c.append.push_back({1, *wait + (i.opcode == 13 ? 5 : 20), 0});
        if (i.opcode != 13) {
            const auto always = draw(100); // The source '<100' is true, but still consumes a draw.
            const auto facing = draw(4);
            const auto turn_wait = draw(20);
            if (!always || !facing || !turn_wait)
                return std::nullopt;
            c.append.push_back({4, *facing});
            c.append.push_back({1, *turn_wait + 10, 0});
        }
    } else {
        const auto wait = draw(100);
        if (!wait)
            return std::nullopt;
        c.append.push_back({1, *wait + 20, 0});
    }
    c.append.push_back(i.opcode == 10 ? LegacyActorControl{10, i.parameter}
                                      : LegacyActorControl{i.opcode});
    return c;
}
std::optional<EquipmentCommitCandidate> prepare_equipment_commit(const LegacyActorControl &c) {
    if (!valid_actor_control(c))
        return std::nullopt;
    if (c[0] == 28 && c[1] >= 0)
        return EquipmentCommitCandidate{0, c[1], 6, true, true};
    if (c[0] == 30 && c[1] >= 0 && c[1] <= 3 && c[2] >= 0)
        return EquipmentCommitCandidate{c[1], c[2], 6, false, true};
    return std::nullopt;
}
std::optional<std::vector<LegacyActorControl>>
prepare_equipment_exit_tail(const EquipmentExitTailInput &i) {
    if (i.category != 1 || (i.detail != 1 && i.detail != 4 && i.detail != 5) || i.old_weapon < 0 ||
        i.new_weapon < 0 || i.armor < 0 || i.accessory < 0 || i.armor_type < 0)
        return std::nullopt;
    std::vector<LegacyActorControl> c{{8, 0}, {20, 1}, {1, 5, 0}, {6, 64}, {3, 9}};
    if (i.detail == 1) {
        c.push_back({27, i.old_weapon, i.new_weapon});
        c.push_back({1, 42, 0});
        c.push_back({7, 64});
        c.push_back({3, 0});
        c.push_back({28, i.new_weapon});
    } else {
        const bool armor = i.detail == 4;
        c.push_back({29, armor ? 21 : 22, armor ? i.armor : i.accessory});
        c.push_back({1, 32, 0});
        c.push_back({7, 64});
        c.push_back({3, 0});
        c.push_back({30, armor ? (i.armor_type == 2 ? 1 : 2) : 3, armor ? i.armor : i.accessory});
    }
    c.push_back({18, 9, 0});
    return c;
}
} // namespace dungeon_village_reference
