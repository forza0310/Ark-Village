#include "dungeon_village_reference/ai_perception.hpp"

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
void event_gate() {
    EventGateInput i;
    i.actor = {ActorKind::human, 1024, 5, true};
    i.cell = {10, 10};
    i.cell_in_map = i.cell_event_flag = true;
    i.encounters = {{3, {11, 11}}, {4, {10, 10}}};
    auto r = prepare_event_gate(i);
    check(r.candidate && r.candidate->ready && r.candidate->bind_encounter == 3,
          "F ignores1024 and first source-order matching center wins");
    i.active_task = i.definition_task_flag = true;
    i.task_encounter = 4;
    check(prepare_event_gate(i).candidate->bind_encounter == 4,
          "task actor scans only task event identity");
    i.task_encounter.reset();
    check(!prepare_event_gate(i).candidate->ready, "task with no event cannot match any event");
    i.previous_encounter = 99;
    r = prepare_event_gate(i);
    check(r.candidate->ready && !r.candidate->bind_encounter,
          "no new match retains stale db and returns true");
    i.cell_event_flag = false;
    i.task_kind = 1;
    i.task_center = {9, 9};
    r = prepare_event_gate(i);
    check(r.candidate->ready && r.candidate->request_task_encounter && !r.candidate->bind_encounter,
          "creation requested but no same-call db binding");
    i.task_encounter = 8;
    check(prepare_event_gate(i).candidate->bind_encounter == 8,
          "existing task event binds without creation");
    for (int x = -3; x <= 3; ++x)
        for (int y = -3; y <= 3; ++y) {
            i.cell = {9 + x, 9 + y};
            check(prepare_event_gate(i).candidate->ready ==
                      (x >= -1 && x <= 1 && y >= -1 && y <= 1),
                  "inclusive +/-1 task rectangle");
        }
    i = {};
    i.actor = {ActorKind::monster, 1024, 5, true, true, true, false, false};
    check(prepare_event_gate(i).candidate->ready, "monster F ignores carrying and map bounds");
    i.actor.flags = 512;
    check(!prepare_event_gate(i).candidate->ready, "F512 preempts all actors");
    i.encounters = {{0, {}}, {0, {1, 1}}};
    check(prepare_event_gate(i).error == ActorAiError::invalid_input,
          "original zero event ID allowed but duplicate identity refused");
}
void move_area() {
    std::array<MoveAreaProbe, 4> p;
    p.fill({true, 1, 4, true});
    for (int state = 0; state <= 20; ++state)
        for (bool event : {false, true})
            for (std::size_t probe = 0; probe < 4; ++probe)
                for (int bad = 0; bad < 4; ++bad) {
                    auto sample = p;
                    auto &s = sample[probe];
                    if (bad == 0)
                        s.cell_in_map = false;
                    if (bad == 1)
                        s.legacy_surface = 3;
                    if (bad == 2)
                        s.logical_state = 0;
                    if (bad == 3)
                        s.in_encounter_square = false;
                    const auto r = prepare_move_area(state, event, sample);
                    const int diagnostic = bad == 0                              ? 1
                                           : bad < 3                             ? 2
                                           : event && (state == 1 || state == 4) ? 3
                                                                                 : 0;
                    check(r && r->allowed == (diagnostic == 0) &&
                              r->legacy_diagnostic == diagnostic,
                          "four probes each retain K guard state order");
                }
    check(!prepare_move_area(21, false, p), "unknown actor state rejected");
    p[0].in_encounter_square = false;
    p[1].cell_in_map = false;
    check(prepare_move_area(1, true, p)->legacy_diagnostic == 3,
          "earlier probe region failure precedes later out-of-bounds");
}
void rescue_and_objects() {
    PreemptionInput i;
    i.rescue_enabled = true;
    i.cell = {10, 10};
    i.people = {{{1}, 2, {0, 0}, {10, 10}, {0, 0}, false, false, false}};
    i.objects = {{9, 3, {0, 0}, {10, 10}}};
    check(select_idle_preemption(i) == 13, "rescue preempts object scan");
    i.definition_task_flag = true;
    check(select_idle_preemption(i) == 11, "task-assigned actor can pick objects but not rescue");
    i.definition_task_flag = false;
    for (int state = 0; state <= 20; ++state) {
        i.state = state;
        check(select_idle_preemption(i) ==
                  ((state == 0 || state == 5) ? std::optional<int>{13} : std::nullopt),
              "preemption only0/5");
    }
    i.state = 5;
    for (int x = -4; x <= 4; ++x)
        for (int y = -4; y <= 4; ++y) {
            i.people[0].cell = i.objects[0].cell = {10 + x, 10 + y};
            // Independent closed-rectangle intersection, not a reused within() helper.
            const bool intersects =
                11 >= (9 + x) && 10 <= (12 + x) && 10 >= (8 + y) && 9 <= (11 + y);
            check(bool(select_idle_preemption(i)) == intersects,
                  "1x1 against3x3 gives inclusive +/-2 not +/-1");
        }
    i.people[0].cell = {10, 10};
    i.people[0].inside_town = true;
    i.objects.clear();
    check(!select_idle_preemption(i), "preemption same town side required");
    auto result = select_rescue_target({0, 0}, i.people);
    check(result.candidate && result.candidate->id.value == 1,
          "I later does not reapply town side condition");
    i.people[0].on_event_cell = true;
    check(!select_rescue_target({0, 0}, i.people).candidate, "I excludes event-bound down target");
    i.people = {{{1}, 2, {4, 0}, {}, {4, 0}, true, false, false},
                {{2}, 2, {1, 0}, {}, {0, 0}, false, false, true},
                {{3}, 2, {0, 1}, {}, {0, 0}, false, false, true}};
    check(select_rescue_target({0, 0}, i.people).candidate->id.value == 3,
          "rescue reverse-scan tie matches combat sorting");
    check(select_healing_target({0, 0}, i.people)->value == 1,
          "J first roster target at distance4 not nearest");
    i.people[0].half_cell = {4, 1};
    check(select_healing_target({0, 0}, i.people)->value == 2,
          "J Manhattan5 excluded and self distance0 allowed");
}
void spawn_probe() {
    for (int state = 0; state <= 20; ++state)
        for (int ticket = 0; ticket < 1000; ++ticket) {
            SpawnProbeInput i{ActorKind::human, state, false, false, 10, 10, 0, 5, ticket};
            const auto r = prepare_spawn_probe(i);
            check(r.candidate && r.candidate->consumes_ticket &&
                      r.candidate->request_event_probe == (state == 5 && ticket < 13),
                  "L draws even for zero probability post-reselection states");
            i.monster_count = 5;
            check(prepare_spawn_probe(i).candidate->consumes_ticket &&
                      !prepare_spawn_probe(i).candidate->request_event_probe,
                  "monster cap checked after random draw");
        }
    SpawnProbeInput i;
    i.destination_inside_town = true;
    check(!prepare_spawn_probe(i).candidate->consumes_ticket,
          "destination town guard before random requirement");
    i.destination_inside_town = false;
    check(prepare_spawn_probe(i).error == ActorAiError::invalid_input,
          "eligible probe cannot silently invent randomness");
}
void departure_override() {
    DepartureOverrideInput i;
    i.self = {1};
    i.reachable = {{0, 0}, {1, 1}, {4, 4}, {5, 5}};
    i.nearest_down = Position{1, 1};
    i.nearest_object = Position{0, 0};
    i.nearest_encounter = Position{5, 4};
    i.active_task = i.definition_task_flag = true;
    i.task_center = {5, 5};
    auto r = prepare_departure_override(i);
    check(r && r->kind == DepartureOverrideKind::task && r->destination == Position{5, 5} &&
              r->request_task_attribute,
          "reachable assigned task has priority over all special targets");
    i.has_object = true;
    check(prepare_departure_override(i)->kind == DepartureOverrideKind::ordinary,
          "carrying task actor skips task and rescue/object/event overrides");
    i.definition_task_flag = false;
    check(prepare_departure_override(i)->kind == DepartureOverrideKind::rescue,
          "o rescue branch itself has no additional carrying guard");
    i.people = {{{1}, {1, 1}}};
    check(prepare_departure_override(i)->kind == DepartureOverrideKind::rescue,
          "own previous destination not a reservation");
    i.people.push_back({{2}, {1, 1}});
    check(prepare_departure_override(i)->kind == DepartureOverrideKind::object,
          "other destination equality reserves nearest down then falls to object");
    i.nearest_object = Position{9, 9};
    for (int activity = 0; activity < 9; ++activity) {
        i.activity = activity;
        r = prepare_departure_override(i);
        check(r->kind == (activity == 6 ? DepartureOverrideKind::encounter
                                        : DepartureOverrideKind::ordinary),
              "only activity6 gets event override (decompiler inverted this)");
        if (activity == 6)
            check(r->destination == Position{4, 4},
                  "event first reachable original-order square cell");
    }
    i.flags = 512;
    check(prepare_departure_override(i)->kind == DepartureOverrideKind::ordinary,
          "leaving guard bypasses human override block");
    i.flags = 0;
    i.kind = ActorKind::monster;
    check(prepare_departure_override(i)->kind == DepartureOverrideKind::ordinary,
          "monster uses activity-specific selector not human pre-policy block");
}
} // namespace
int main() {
    event_gate();
    move_area();
    rescue_and_objects();
    spawn_probe();
    departure_override();
    std::cout << checks << " checks passed\n";
}
