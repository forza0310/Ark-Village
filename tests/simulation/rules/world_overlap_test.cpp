#include "ark/simulation/rules/world_overlap.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
bool near(float a, float b) { return std::abs(a - b) < 0.0001f; }
WorldOverlapActor actor(std::uint64_t id, float x = 0.0f, float z = 0.0f) {
    WorldOverlapActor a;
    a.id = {id};
    a.state = 1;
    a.cell = {0, 11};
    a.decision_area = true;
    a.position = {x, z};
    a.previous_position = {-100.0f, -200.0f};
    return a;
}
WorldOverlapInput fixture() {
    WorldOverlapInput i;
    i.boundary_y = 10;
    i.direction_tickets.assign(100, 0);
    return i;
}
void pair_order() {
    auto i = fixture();
    i.humans = {actor(0), actor(1), actor(2)};
    i.monsters = {actor(0), actor(1), actor(2)};
    const auto r = prepare_world_overlap(i);
    check(r.candidate && r.candidate->consumed_tickets == 18,
          "3 human pairs + 9 cross pairs + 6 ORDERED monster pairs");
    const auto &pairs = r.candidate->attempts;
    check(pairs[0].moved.kind == ActorKind::human && pairs[0].moved.id.value == 0 &&
              pairs[0].other.id.value == 1 && pairs[1].other.id.value == 2,
          "human forward outer, following human forward inner");
    check(pairs[2].moved.id.value == 0 && pairs[2].other.kind == ActorKind::monster &&
              pairs[2].other.id.value == 0 && pairs[4].other.id.value == 2,
          "cross pairs follow EACH human's human-human loop");
    check(pairs[12].moved.kind == ActorKind::monster && pairs[12].moved.id.value == 0 &&
              pairs[12].other.id.value == 1 && pairs[14].moved.id.value == 1 &&
              pairs[14].other.id.value == 0,
          "monster unordered pair occurs again in reversed outer/inner orientation");
    check(r.candidate->humans[0].cell.x == 0 && r.candidate->humans[0].cell.y == 11 &&
              r.candidate->humans[0].previous_position.x == -100.0f &&
              i.humans[0].position.x == 0.0f,
          "private positions change without reprojecting cached s or mutating input");
}
void guards() {
    for (int state = 0; state <= 20; ++state) {
        auto i = fixture();
        i.humans = {actor(0)};
        i.monsters = {actor(0)};
        i.monsters[0].state = state;
        const auto r = prepare_world_overlap(i);
        const bool eligible = state != 2 && state != 3 && state != 8 && state != 9 && state != 14 &&
                              state != 15 && state != 16;
        check(r.candidate && r.candidate->consumed_tickets == (eligible ? 1u : 0u),
              "monster b.a excludes only source seven states plus aB0");
        i.monsters[0].decision_area = false;
        const auto no_area = prepare_world_overlap(i);
        check(no_area.candidate && no_area.candidate->consumed_tickets == 0,
              "monster ineligible on decision aB0");
    }
    for (int state = 0; state <= 20; ++state) {
        auto i = fixture();
        i.humans = {actor(0)};
        i.monsters = {actor(0)};
        i.humans[0].state = state;
        const auto r = prepare_world_overlap(i);
        check(r.candidate && r.candidate->consumed_tickets == (state == 1 ? 1u : 0u),
              "human outer requires state1, not broad monster qualification");
    }
    auto i = fixture();
    i.humans = {actor(0), actor(1)};
    i.humans[0].inside_town = true;
    i.humans[0].decision_area = false;
    i.monsters = {actor(0)};
    const auto town = prepare_world_overlap(i);
    check(town.candidate && town.candidate->consumed_tickets == 2,
          "inside-town human skips human-human but NOT human-monster; aB0 not read for human");
    i.humans[0].cell.y = 10;
    i.humans[1].cell.y = 9;
    const auto boundary = prepare_world_overlap(i);
    check(boundary.candidate && boundary.candidate->consumed_tickets == 0,
          "human outer uses strict cached s.y > raw h.l boundary");
    i = fixture();
    i.humans = {actor(0)};
    i.monsters = {actor(0)};
    i.monsters[0].cell.x = 2;
    const auto far = prepare_world_overlap(i);
    check(far.candidate && far.candidate->consumed_tickets == 1 &&
              !far.candidate->attempts[0].adjacent && far.candidate->humans[0].position.x == 0.0f,
          "nonadjacent qualified pair still consumes random draw before small-function guard");
}
void geometry() {
    const auto human = reference_world_overlap_rectangle(ActorKind::human, 999);
    check(human && human->x_offset == -30 && human->z_offset == 30 && human->width == 30 &&
              human->height == 30,
          "human uses ai1 corner, not centered ah1 offset and not caller monster shape");
    constexpr int widths[]{30, 30, 30, 40, 60, 30, 240};
    constexpr int heights[]{30, 30, 30, 40, 60, 20, 240};
    for (int shape = 0; shape < 7; ++shape) {
        const auto r = reference_world_overlap_rectangle(ActorKind::monster, shape);
        check(r && r->width == widths[shape] && r->height == heights[shape] &&
                  r->x_offset == -widths[shape] && r->z_offset == heights[shape],
              "monster exact g+3 rectangle dimensions including nonsquare shape5");
    }
    check(!reference_world_overlap_rectangle(ActorKind::monster, -1) &&
              !reference_world_overlap_rectangle(ActorKind::monster, 7),
          "out-of-table monster shape rejected, not guessed or clamped");
    auto i = fixture();
    i.humans = {actor(0, 35.0f, 5.0f)};
    i.monsters = {actor(0)};
    i.monsters[0].monster_shape = 4;
    const auto large_second = prepare_world_overlap(i);
    check(large_second.candidate && large_second.candidate->attempts[0].overlapping &&
              large_second.candidate->attempts[0].shared_rectangle->width == 60 &&
              near(large_second.candidate->humans[0].position.x, 60.0f) &&
              near(large_second.candidate->humans[0].position.z, (5.0f * 60.0f) / 35.0f),
          "second lookup overwrites BOTH collision rectangles, moves first relative to second");
    i.direction_tickets[0] = 1;
    const auto small_second = prepare_world_overlap(i);
    check(small_second.candidate && !small_second.candidate->attempts[0].overlapping &&
              small_second.candidate->attempts[0].shared_rectangle->width == 30 &&
              small_second.candidate->monsters[0].position.x == 0.0f,
          "reversing random orientation changes shared rectangle and can prevent overlap");
    i = fixture();
    i.humans = {actor(0, 30.0f, 30.0f)};
    i.monsters = {actor(0)};
    const auto touching = prepare_world_overlap(i);
    check(touching.candidate && touching.candidate->attempts[0].overlapping &&
              !touching.candidate->attempts[0].rolled_back,
          "rectangles touching edges are overlapping, both axes equal take horizontal branch");
    i.humans[0].position = {5.0f, 20.0f};
    i.monsters[0].monster_shape = 5;
    const auto tall_axis = prepare_world_overlap(i);
    check(tall_axis.candidate && near(tall_axis.candidate->humans[0].position.x, 5.0f) &&
              near(tall_axis.candidate->humans[0].position.z, 20.0f),
          "vertical dominant separation uses HEIGHT20, not WIDTH30");
    i.humans[0].position = {-2.0f, -10.0f};
    const auto negative = prepare_world_overlap(i);
    check(negative.candidate && near(negative.candidate->humans[0].position.x, -4.0f) &&
              near(negative.candidate->humans[0].position.z, -20.0f),
          "negative coordinates preserve relative signs in vertical branch");
    i.humans[0].position = {20.0f, 0.0005f};
    const auto axis = prepare_world_overlap(i);
    check(
        axis.candidate && axis.candidate->attempts[0].rolled_back &&
            axis.candidate->humans[0].position.x == -100.0f &&
            axis.candidate->humans[0].position.z == -200.0f,
        "near-axis overlap rolls FIRST actor n back to bu instead of division or arbitrary nudge");
    i.humans[0].position = {20.0f, 0.001f};
    const auto threshold = prepare_world_overlap(i);
    check(threshold.candidate && !threshold.candidate->attempts[0].rolled_back,
          "0.001 axis threshold is strict, not <=");
    i = fixture();
    i.humans = {actor(0), actor(1), actor(2, 100.0f, 100.0f)};
    i.humans[0].previous_position = {90.0f, 90.0f};
    const auto successive = prepare_world_overlap(i);
    check(successive.candidate && successive.candidate->attempts[0].rolled_back &&
              successive.candidate->attempts[1].adjacent &&
              successive.candidate->attempts[1].overlapping &&
              successive.candidate->humans[0].position.x == 70.0f &&
              successive.candidate->humans[0].position.z == 70.0f &&
              successive.candidate->humans[0].cell.y == 11,
          "later pair reads changed live n but unchanged cached s, not independent snapshots");
}
void failure_atomicity() {
    auto i = fixture();
    i.monsters = {actor(0), actor(1)};
    i.direction_tickets = {0};
    const auto missing = prepare_world_overlap(i);
    check(missing.error == WorldOverlapError::missing_ticket && !missing.candidate &&
              i.monsters[0].position.x == 0.0f,
          "late second-pair missing RNG rejects candidate including previous position mutation");
    i.direction_tickets = {0, 2};
    const auto bad_ticket = prepare_world_overlap(i);
    check(bad_ticket.error == WorldOverlapError::invalid_input && !bad_ticket.candidate,
          "late draw outside a2 range rejects whole plan");
    i.direction_tickets = {0, 0};
    i.attempt_limit = 1;
    const auto budget = prepare_world_overlap(i);
    check(budget.error == WorldOverlapError::attempt_limit && !budget.candidate,
          "ordered pair loop cannot silently truncate at safety budget");
    i.attempt_limit = 0;
    check(prepare_world_overlap(i).error == WorldOverlapError::invalid_input,
          "zero safety budget explicitly invalid");
    i = fixture();
    i.monsters = {actor(0), actor(0)};
    check(prepare_world_overlap(i).error == WorldOverlapError::duplicate_id,
          "duplicate same-roster stable identity rejected");
    i.monsters = {actor(0)};
    i.humans = {actor(0)};
    i.monsters[0].monster_shape = 7;
    const auto shape = prepare_world_overlap(i);
    check(shape.error == WorldOverlapError::invalid_input && !shape.candidate,
          "invalid queried shape does not invent collision rectangle");
    i.monsters[0].monster_shape = 0;
    i.humans[0].position.x = std::numeric_limits<float>::infinity();
    check(prepare_world_overlap(i).error == WorldOverlapError::invalid_input,
          "nonfinite world geometry rejected before arithmetic");
    i = fixture();
    const auto empty = prepare_world_overlap(i);
    check(empty.candidate && empty.candidate->attempts.empty() &&
              empty.candidate->consumed_tickets == 0,
          "empty admitted world has no random consumption");
}
} // namespace
int main() {
    try {
        pair_order();
        guards();
        geometry();
        failure_atomicity();
        std::cout << "world overlap checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
