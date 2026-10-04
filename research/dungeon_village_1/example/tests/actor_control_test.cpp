#include "dungeon_village_reference/actor_control.hpp"
#include "dungeon_village_reference/actor_lifecycle.hpp"

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
void shapes() {
    const std::size_t expected[]{3, 3, 2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1,
                                 1, 3, 4, 1, 1, 3, 1, 1, 2, 1, 3, 2, 3, 3, 1, 1, 1};
    for (int opcode = 0; opcode < 34; ++opcode)
        for (std::size_t size = 1; size <= 6; ++size) {
            LegacyActorControl c(size);
            c[0] = opcode;
            check(valid_actor_control(c) == (size >= expected[opcode]),
                  "all34 minimum payload widths, trailing fields preserved");
        }
    for (const LegacyActorControl &c : {LegacyActorControl{},
                                        {-1},
                                        {34},
                                        {1, -1, 0},
                                        {1, 1, 2},
                                        {2, 21},
                                        {3, 12},
                                        {4, 4},
                                        {8, 9}})
        check(!valid_actor_control(c), "invalid command rejected explicitly");
}
void local_queue() {
    ActorControlState s;
    s.queue = {{6, 32}, {1, 2, 1}, {7, 1}, {3, 4}, {4, 2}, {8, 0}, {19, 6, 0, 5}};
    auto r = prepare_local_control_prefix(s, {std::nullopt, true});
    check(r.candidate && r.candidate->flow == ActorControlFlow::waiting &&
              r.candidate->state.flags == 33 && r.candidate->removed_commands == 1 &&
              r.candidate->state.queue[0][1] == 1 && s.queue[1][1] == 2,
          "wait subtracts same round while original queue remains untouched");
    r = prepare_local_control_prefix(r.candidate->state, {std::nullopt, true});
    check(r.candidate && r.candidate->flow == ActorControlFlow::departure_started &&
              r.candidate->state.action == 4 && r.candidate->state.facing == 2 &&
              r.candidate->state.flags == 32 && r.candidate->state.queue.size() == 1 &&
              r.candidate->state.queue[0][0] == 19 && r.candidate->state.state == 0,
          "wait reaches0 then executes local tail, successful8 stops before attribute19");
    r = prepare_local_control_prefix(r.candidate->state);
    check(r.candidate && r.candidate->flow == ActorControlFlow::delegated &&
              r.candidate->removed_commands == 0 && r.candidate->state.queue[0][0] == 19,
          "next d requires actual attribute handler, never discarded");
    s.queue = {{1, 0, 0}, {9}, {11}, {20, 1}, {31}, {5, 7}, {7, 6}};
    r = prepare_local_control_prefix(s);
    check(r.candidate && r.candidate->flow == ActorControlFlow::empty &&
              r.candidate->removed_commands == 7 && r.candidate->state.flags == 1,
          "zero wait, original no-ops and bitmask replacement finish same round");
    s.queue = {{0, 100, 200}, {0, 300, 400}};
    r = prepare_local_control_prefix(s, {false, std::nullopt});
    check(r.candidate && r.candidate->flow == ActorControlFlow::moving &&
              !r.candidate->removed_commands,
          "unreached motion keeps front");
    r = prepare_local_control_prefix(s, {true, std::nullopt});
    check(r.candidate && r.candidate->flow == ActorControlFlow::delegated &&
              r.candidate->removed_commands == 1 && r.candidate->state.queue[0][1] == 300,
          "second motion needs fresh target-specific result not reused true");
    s.queue = {{8, 0}, {19, 6, 0, 5}};
    r = prepare_local_control_prefix(s, {std::nullopt, false});
    check(r.candidate && r.candidate->flow == ActorControlFlow::delegated &&
              !r.candidate->removed_commands && r.candidate->state.queue.size() == 2,
          "failed8 delegates complete failure/cleanup instead of running attr");
    s.queue = {{6, 32}, {999}};
    r = prepare_local_control_prefix(s);
    check(r.error == ActorControlError::malformed_command && !r.candidate && s.flags == 0,
          "safety preflight malformed tail rejects without partial flags");
}
void transitions_and_failure() {
    ActorStateTransitionInput i;
    i.control = {16U | 4U | 2048U, 1, 6, 37, 25, 2, {{8, 0}, {19, 6, 0, 5}}};
    i.baseline = 5;
    for (bool human : {false, true})
        for (int state = 0; state <= 20; ++state) {
            i.human = human;
            i.next_state = state;
            auto c = prepare_actor_state_transition(i);
            check(c && c->control.state == state && c->control.alternate_counter == 0 &&
                      !(c->control.flags & 16) && c->reset_state_counter_and_parameter,
                  "every setter clears16, B/C/i and old commands");
            check(c->clear_encounter == (human && state != 1 && state != 18),
                  "only humans clear db outside1/18, dc not included");
            check(c->baseline == ((state == 0 || state == 5 || state == 17) ? state : 5),
                  "setter only updates baseline0/5/17");
            const bool reset_action = state == 0 || state == 2 || state == 3 || state == 4 ||
                                      state == 5 || state == 11 || state == 12 || state == 13 ||
                                      state == 18;
            check(c->control.action_counter == (reset_action ? 0 : 37),
                  "other states keep action counter, state1 does not set action");
            if (state == 10)
                check(c->control.queue ==
                              std::vector<LegacyActorControl>{
                                  {1, 5, 0}, {32}, {3, 6}, {1, 14, 0}, {3, 0}} &&
                          !(c->control.flags & 2048),
                      "state10 queued jump, no immediate action6");
            if (state == 18)
                check(c->reset_attack_count && c->control.queue[0] == LegacyActorControl{10, 1} &&
                          !c->consumed_boost_ticket,
                      "already boosted18 skips random draw");
        }
    i.next_state = 10;
    i.current_facility_category = 2;
    auto c = prepare_actor_state_transition(i);
    check(c && c->request_cleanup && c->control.queue.empty() && (c->control.flags & 2048),
          "state10 inn cleanup occurs before normal jump/boost clearing");
    i.current_facility_category.reset();
    i.next_state = 18;
    i.control.flags = 0;
    for (int u = 0; u <= 100; ++u)
        for (int ticket = 0; ticket < 100; ++ticket) {
            i.legacy_u = u;
            i.boost_ticket = ticket;
            c = prepare_actor_state_transition(i);
            check(c && c->consumed_boost_ticket &&
                      static_cast<bool>(c->control.flags & 2048) == (ticket < u * 12 / 100) &&
                      c->request_boost_event116 == (ticket < u * 12 / 100),
                  "state18 boost integer threshold0..12, strict less and draw even0");
        }
    i.boost_ticket.reset();
    check(!prepare_actor_state_transition(i), "missing mandatory state18 ticket rejects candidate");
    for (std::uint32_t flags : {0U, 512U, 1024U, 32768U, 512U | 32768U, 512U | 1024U | 32768U}) {
        const auto f = prepare_failed_activity(flags | 66U);
        check(f.expression18 == static_cast<bool>(flags & 1024U) &&
                  f.delete_instance == ((flags & 1024U) && (flags & 32768U)) &&
                  f.cleanup == !f.delete_instance,
              "failure deletion tests OLD1024 before adding it from512");
        if (f.cleanup) {
            const auto clean = prepare_actor_cleanup(ActorKind::human, f.flags);
            ActorControlState s;
            s.flags = clean->flags;
            s.state = clean->state;
            if (clean->waiting_updates)
                s.queue.push_back({1, clean->waiting_updates, 0});
            s.queue.push_back({8, clean->activity});
            const auto resumed = prepare_local_control_prefix(s, {std::nullopt, true});
            check(resumed.candidate &&
                      resumed.candidate->flow == ((f.flags & 1024U)
                                                      ? ActorControlFlow::waiting
                                                      : ActorControlFlow::departure_started),
                  "failed8 cleanup can resume new queue same d, 1024 waits120");
        }
    }
}
void wandering() {
    ActorWanderInput i;
    i.width = i.height = 3;
    i.actor = {1, 1};
    i.center = Position{1, 1};
    i.cells.resize(9);
    for (int opcode : {10, 12, 13}) {
        i.opcode = opcode;
        i.tickets = opcode == 13 ? std::vector<int>{0, 0, 79, 9}
                                 : std::vector<int>{0, 0, 79, 99, 99, 3, 19};
        auto c = prepare_actor_wander(i);
        const Position first = opcode == 10 ? Position{1, 2} : Position{0, 2};
        check(c && c->cells.front() == first && c->cells.size() == (opcode == 10 ? 4 : 8),
              "four/eight candidate order preserves original neighbor arrays");
        check(c->append[0] == LegacyActorControl{0, first.x * 100 + 10, first.y * 100 + 89} &&
                  c->append[1] == LegacyActorControl{1, opcode == 13 ? 14 : 119, 0} &&
                  c->consumed_tickets == (opcode == 13 ? 4 : 7),
              "direct random world target, offsets10..89 and exact wait draw counts");
        check(c->append.back()[0] == opcode, "self wander appended after movement/wait, not front");
        i.cells[7].inside_town = true;
        c = prepare_actor_wander(i);
        check(c && c->cells.size() == (opcode == 13   ? 8
                                       : opcode == 10 ? 3
                                                      : 7),
              "follow13 retains town,10/12 filter town");
        i.cells[7].inside_town = false;
    }
    i.opcode = 10;
    i.parameter = 1;
    i.tickets = {0};
    auto c = prepare_actor_wander(i);
    check(c && c->cells.empty() &&
              c->append == std::vector<LegacyActorControl>{{1, 20, 0}, {10, 1}} &&
              c->consumed_tickets == 1,
          "event wander10 parameter1 only flags2, empty consumes only100");
    i.cells[3].flags = 2;
    i.tickets = {0, 79, 0, 0, 99, 0, 0};
    c = prepare_actor_wander(i);
    check(c && c->cells == std::vector<Position>{{0, 1}}, "flags2 filter retains exact cell");
    i.tickets.back() = 20;
    check(!prepare_actor_wander(i), "invalid last draw rejects entire appended sequence");
    i.opcode = 12;
    i.center.reset();
    i.tickets.clear();
    c = prepare_actor_wander(i);
    check(c && c->append.empty() && !c->consumed_tickets,
          "missing encounter/follow removes original12 without requeue or draws");
    i.opcode = 10;
    i.parameter = 0;
    i.actor = {-1, 1};
    i.tickets = {0, 0, 0, 0, 0, 0, 0};
    c = prepare_actor_wander(i);
    check(c && c->cells == std::vector<Position>{{0, 1}} &&
              c->append.front() == LegacyActorControl{0, 10, 110},
          "source tests NEIGHBORS in bounds, outside cached center remains usable");
    for (const int opcode : {10, 12, 13})
        for (const int extreme :
             {std::numeric_limits<int>::min(), std::numeric_limits<int>::max()}) {
            i.opcode = opcode;
            i.actor = {extreme, extreme};
            i.center = i.actor;
            i.tickets = {99};
            c = prepare_actor_wander(i);
            check(c && c->cells.empty() && c->consumed_tickets == 1 &&
                      c->append.front() == LegacyActorControl{1, 119, 0},
                  "wide neighbor offsets skip extreme signed outside cells without overflow");
        }
}
void equipment_tail() {
    EquipmentExitTailInput i;
    i.old_weapon = 2;
    i.new_weapon = 7;
    i.armor = 4;
    i.accessory = 8;
    for (int detail : {1, 4, 5}) {
        i.detail = detail;
        for (int type : {0, 1, 2, 3}) {
            i.armor_type = type;
            const auto tail = prepare_equipment_exit_tail(i);
            check(tail && tail->front() == LegacyActorControl{8, 0} &&
                      tail->back() == LegacyActorControl{18, 9, 0},
                  "equipment tail starts activity0, final expression9 remains queued");
            ActorControlState s;
            s.queue = *tail;
            auto r = prepare_local_control_prefix(s, {std::nullopt, true});
            check(r.candidate && r.candidate->state.queue.size() == tail->size() - 1 &&
                      r.candidate->state.queue.front() == LegacyActorControl{20, 1},
                  "successful activity stops before equipment display, no eager commit");
            const auto commit = prepare_equipment_commit((*tail)[9]);
            check(commit &&
                      commit->slot == (detail == 1   ? 0
                                       : detail == 5 ? 3
                                       : type == 2   ? 1
                                                     : 2) &&
                      commit->equipment == (detail == 1   ? 7
                                            : detail == 5 ? 8
                                                          : 4) &&
                      commit->update_actor_weapon == (detail == 1) && commit->reselect_counter == 6,
                  "28 updates actor ae and v0,30 only v slot, both reselect6/recalculate");
            check(!prepare_equipment_commit((*tail)[5]), "27/29 display cannot mutate equipment");
            check((*tail)[6][1] == (detail == 1 ? 42 : 32), "weapon waits42 armor/accessory32");
        }
    }
    i.category = 2;
    check(!prepare_equipment_exit_tail(i), "nonshop does not get equipment tail");
    check(!prepare_equipment_commit({30, 4, 1}) && !prepare_equipment_commit({28, -1}),
          "illegal equipment slot or ID rejected");
}
} // namespace
void baseline_restore() {
    for (int baseline = 0; baseline <= 20; ++baseline)
        for (int mode = 0; mode <= 4; ++mode)
            for (bool human : {false, true}) {
                ActorControlState s;
                s.state = 12;
                s.action = 7;
                s.action_counter = 42;
                s.alternate_counter = 20;
                s.flags = 16U | 2048U | 128U;
                s.queue = {{1, 10, 0}};
                const auto c = prepare_actor_baseline_restore(s, baseline, human, mode);
                check(c && c->control.state == baseline && c->control.action == 0 &&
                          c->control.action_counter == 0 && c->control.alternate_counter == 20 &&
                          c->control.flags == (2048U | 128U) && c->clear_encounter == human,
                      "b restores D directly, clears16/n0/db only, does not use c(D)");
                std::vector<LegacyActorControl> expected;
                if (baseline == 5)
                    expected = {{10, 0}};
                else if (baseline == 17 && mode == 0)
                    expected = {{12}};
                else if (baseline == 17 && mode == 3)
                    expected = {{13}};
                check(c->control.queue == expected, "only5/17 mode0/3 enqueue source wander");
            }
}
int main() {
    shapes();
    local_queue();
    transitions_and_failure();
    wandering();
    equipment_tail();
    baseline_restore();
    std::cout << checks << " checks passed\n";
}
