#include "dungeon_village_reference/actor_control.hpp"

#include <iostream>
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
} // namespace
int main() {
    shapes();
    local_queue();
    std::cout << checks << " checks passed\n";
}
