// Conditional real startup integration, not an assertion of default full AI or original RNG.
#include "ark/app/game.hpp"
#include "ark/people/actor_control.hpp"
#include "ark/people/decision.hpp"
#include "legacy/support/people_test_support.hpp"
#include <algorithm>
#include <iostream>
#include <set>

namespace {
using namespace ark;
using namespace ark::people;
using namespace ark::test;
void real_scene() {
    app::Game game;
    const auto map = game.route_map();
    ActivityCandidateInput input;
    const auto &data = app::startup_data();
    input.town = {data.build_bounds.min_x, data.build_bounds.max_x, data.build_bounds.min_y,
                  data.build_bounds.max_y};
    for (const auto &cell : map.cells)
        input.cell_definition_ids.push_back(cell.definition_id);
    for (const auto &d : data.definitions)
        input.definitions.push_back({d.id, d.activity_category, d.economy.attributes[2].first});
    for (const auto id : game.state().instance_order)
        input.instances.push_back({id, game.state().facilities.at(id).definition_id, 1});
    for (const auto spawn : data.spawn_points) {
        const auto field = *world::search(map, spawn).field;
        const auto scene = *collect_activity_candidates(field, input).snapshot;
        DepartureOverrideInput priority;
        priority.self = {1}; // Original first UID0, mapped to nonzero conditional snapshot ID1.
        priority.flags = 2U | 8192U;
        for (const auto &cell : scene.cells)
            priority.reachable.push_back(cell.position);
        std::set<int> visited;
        for (int category = 0; category < 70; ++category)
            for (int ticket = 0; ticket < (category < 40 ? 9 : 5); ++ticket) {
                const auto decision = prepare_decision(field, scene, priority,
                                                       {0, {}, priority.flags, category, ticket});
                check(decision.candidate && decision.candidate->facility &&
                          decision.candidate->priority.kind == DepartureOverrideKind::ordinary,
                      "explicit no-override snapshot reaches researched ordinary selector");
                const auto &departure = *decision.candidate->facility;
                if (!visited.insert(departure.binding.definition_id).second)
                    continue;
                ActorControlState control;
                control.flags = priority.flags;
                control.queue = {{8, 0}, {20, 1}, {19, 6, 0, 1}};
                const auto prefix = prepare_local_control_prefix(control, {{}, true});
                check(prefix.candidate &&
                          prefix.candidate->flow == ActorControlFlow::departure_started &&
                          prefix.candidate->state.queue ==
                              std::vector<LegacyActorControl>{{20, 1}, {19, 6, 0, 1}} &&
                          control.queue.size() == 3,
                      "successful8 stops before marker/attribute, original queue immutable");
                Travel travel{spawn, departure.binding, departure.route.steps,
                              waypoint(spawn, 4, -1)};
                int entries{};
                for (int tick = 0; tick < 1000 && travel.phase == TravelPhase::travelling; ++tick) {
                    const auto next = advance_travel(map, travel, priority.flags);
                    entries += next.entered;
                    travel = next.travel;
                }
                check(travel.phase == TravelPhase::entered && entries == 1,
                      "priority/selection/control result can feed continuous travel once");
                const auto tail = prepare_local_control_prefix(prefix.candidate->state);
                check(tail.candidate && tail.candidate->flow == ActorControlFlow::delegated &&
                          tail.candidate->state.queue.front()[0] == 19,
                      "later attribute remains explicit external work, never silently applied");
            }
        check(visited == std::set<int>{28, 30, 33}, "weapons remain among first ordinary choices");
    }
    check(game.state().money == 5000 && game.state().expenses.empty() && !game.state().adventurer &&
              game.state().simulation_steps == 0,
          "pure combination leaves live aggregate untouched");
}
void priority_handoff() {
    DecisionFixture fixture;
    fixture.add({3, 2}, 1, 30);
    fixture.add({2, 3}, 2, 33);
    auto field = fixture.field();
    const auto scene = fixture.collect();
    DepartureOverrideInput input;
    input.self = {1};
    for (const auto &cell : scene.cells)
        input.reachable.push_back(cell.position);
    input.active_task = input.definition_task_flag = true;
    input.task_center = {3, 2};
    input.nearest_down = world::Cell{2, 3};
    input.nearest_object = world::Cell{3, 2};
    const auto expect = [&](DepartureOverrideKind kind, world::Cell cell) {
        // Invalid ordinary tickets must remain unconsumed for a successful special handoff.
        const auto result = prepare_decision(field, scene, input, {0, {}, input.flags, -1, -1});
        check(result.candidate && !result.candidate->facility &&
                  result.candidate->priority.kind == kind &&
                  result.candidate->priority.destination == cell,
              "special priority never falls through to ordinary random draw");
    };
    expect(DepartureOverrideKind::task, {3, 2});
    input.definition_task_flag = false;
    expect(DepartureOverrideKind::rescue, {2, 3});
    input.people = {{{2}, {2, 3}}};
    expect(DepartureOverrideKind::object, {3, 2});
    input.flags = 512;
    const auto skipped = prepare_decision(field, scene, input, {0, {}, 512, -1, -1});
    check(skipped.error == DecisionError::facility_refused &&
              skipped.facility_error == FacilityDepartureError::invalid_ticket &&
              !skipped.candidate,
          "512 bypasses override and surfaces ordinary refusal, not fake success");
    input.flags = 0;
    auto stale = scene;
    stale.cells[0].instance->instance_id = 99;
    check(prepare_decision(field, stale, input, {}).error == DecisionError::invalid_input,
          "stale binding rejected even for special handoff");
    auto mismatch = input;
    mismatch.activity = 6;
    check(prepare_decision(field, scene, mismatch, {}).error == DecisionError::invalid_input,
          "cannot mix activity0 snapshot request with activity6 ordinary request");
    std::reverse(mismatch.reachable.begin(), mismatch.reachable.end());
    mismatch.activity = 0;
    check(prepare_decision(field, scene, mismatch, {}).error == DecisionError::invalid_input,
          "ordered candidate view cannot be independently reordered");
    field.previous[1] = 1;
    check(prepare_decision(field, scene, input, {}).error == DecisionError::invalid_field,
          "invalid predecessor graph refused before priority selection");
}
void final_state_probe() {
    HumanIdleInput idle;
    idle.outside_counter = 100;
    idle.nearby_event = true;
    idle.reported_hp = 24;
    idle.capacity_hp = 100;
    const auto choice = prepare_human_idle(idle);
    check(choice.candidate && choice.candidate->activity == 0 && choice.candidate->run_spawn_probe,
          "lowHP activity0 overrides same-round nearby-event activity6");
    SpawnProbeInput probe{ActorKind::human, 0, false, false, 10, 10, 0, 4, 0};
    const auto result = prepare_spawn_probe(probe);
    check(result.candidate && result.candidate->consumes_ticket &&
              !result.candidate->request_event_probe,
          "probe after state change consumes draw but cannot use old state5 threshold");
}
void equipment_tail_and_failed_activity() {
    const auto tail = prepare_equipment_exit_tail({1, 1, 0, 1});
    check(tail && tail->front() == LegacyActorControl{8, 0},
          "equipment exit first requests another activity");
    ActorControlState state;
    state.queue = *tail;
    auto prefix = prepare_local_control_prefix(state, {{}, true});
    check(prefix.candidate && prefix.candidate->flow == ActorControlFlow::departure_started &&
              prefix.candidate->state.queue.front()[0] == 20,
          "successful departure preserves all equipment tail work");
    state = prefix.candidate->state;
    for (int tick = 0; tick < 5; ++tick) {
        prefix = prepare_local_control_prefix(state);
        check(prefix.candidate.has_value(), "equipment pre-display waiting prefix");
        state = prefix.candidate->state;
    }
    check(prefix.candidate->flow == ActorControlFlow::delegated &&
              state.queue.front() == LegacyActorControl{27, 0, 1} && (state.flags & 64U) &&
              !prepare_equipment_commit(state.queue.front()),
          "display27 delegates without equipping; movement blocked until tail resumes");
    // Stand-in for the external renderer accepting display27. No game equipment is changed.
    state.queue.erase(state.queue.begin());
    for (int tick = 0; tick < 42; ++tick) {
        prefix = prepare_local_control_prefix(state);
        check(prefix.candidate.has_value(), "weapon animation wait prefix");
        state = prefix.candidate->state;
    }
    const auto commit = prepare_equipment_commit(state.queue.front());
    check(prefix.candidate->flow == ActorControlFlow::delegated && !(state.flags & 64U) &&
              state.queue.front()[0] == 28 && commit && commit->equipment == 1 &&
              commit->reselect_counter == 6 && commit->update_actor_weapon,
          "only after42 waits is actual weapon commit request reached");
    const auto old = prepare_failed_activity(1024U | 32768U);
    const auto newly = prepare_failed_activity(512U | 32768U | 66U);
    check(old.delete_instance && !old.cleanup && old.expression18 && !newly.delete_instance &&
              newly.cleanup && (newly.flags & 1024U) && !(newly.flags & 66U),
          "new1024 after failure does not re-enter old1024 deletion branch");
}
} // namespace
int main() {
    try {
        real_scene();
        priority_handoff();
        final_state_probe();
        equipment_tail_and_failed_activity();
        std::cout << "PASS AI priority/control combination checks=" << ark::test::checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
