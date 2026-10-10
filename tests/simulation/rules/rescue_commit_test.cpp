#include "ark/simulation/combat/rules/rescue_commit.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
RescueWorldState fixture(bool shared = false) {
    RescueWorldState s;
    s.map = {4, 4, std::vector<LegacyMapCell>(16)};
    for (auto &cell : s.map.cells)
        cell.legacy_state = 4;
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.price = 300;
    f.upgrade_uses = {2, 10};
    s.facilities.emplace(3, f);
    const auto map = bind_facility_map(s.map, {{f.placement, 3}});
    check(map.map.has_value(), "fixture binds complete inn footprint");
    s.map = *map.map;
    s.facility_uses.emplace(33, FacilityUseProgress{});
    for (CharacterId id : {CharacterId{1}, CharacterId{2}}) {
        BattleActorRecord a;
        a.id = id;
        a.definition = shared ? 0 : static_cast<int>(id.value) - 1;
        a.control.state = id.value == 1 ? 13 : 2;
        a.capacity = 100;
        a.position = id.value == 1 ? CombatPoint{250, 2, 250} : CombatPoint{260, 0, 250};
        s.ai.battle.actors.emplace(id, a);
        s.ai.human_order.push_back(id);
        s.ai.contexts.emplace(id, RewardActorContext{{2, 2}, false, {}, {}});
        s.actors.emplace(id, RescueActorContext{});
        s.actors.at(id).path_pending = true;
        s.human_spending.emplace(a.definition, 10);
    }
    return s;
}
RescueWorldState carried(bool shared = false) {
    const auto r = prepare_world_rescue_bind(fixture(shared), {1}, CharacterId{2}, true);
    check(r.candidate && r.candidate->binding_action == RescueBindingAction::bind,
          "bind prepares one two-actor owner");
    auto s = r.candidate->state;
    s.ai.battle.actors.at({1}).position = {150, 0, 150};
    s.ai.contexts.at({1}).cell = {1, 1};
    s.actors.at({1}).binding = ArrivalBinding{{1, 1}, {3}, 33};
    return s;
}
void binding_and_follow() {
    auto s = fixture();
    auto r = prepare_world_rescue_bind(s, {1}, CharacterId{2}, false);
    check(r.candidate && r.candidate->binding_action == RescueBindingAction::chase &&
              !r.candidate->state.ai.battle.actors.at({1}).rescue,
          "non-touching only requests approach, no half binding");
    s.actors.at({1}).unbound_route = LegacyPathResult{};
    s.actors.at({1}).unbound_route->steps = {{3, 2}};
    s.actors.at({1}).waypoint = 1;
    s.actors.at({1}).destination = Position{3, 2};
    r = prepare_world_rescue_bind(s, {1}, CharacterId{2}, true);
    check(r.candidate && r.candidate->state.ai.battle.actors.at({1}).object_slot == -2 &&
              r.candidate->state.ai.battle.actors.at({1}).rescue == CharacterId{2} &&
              r.candidate->state.ai.battle.actors.at({2}).rescue == CharacterId{1} &&
              r.candidate->state.ai.battle.actors.at({2}).control.state == 16 &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 4}} &&
              !r.candidate->state.actors.at({1}).path_pending &&
              s.ai.battle.actors.at({2}).control.state == 2,
          "bind commits both R, state16 and activity4 atomically without changing input");
    check(!r.candidate->state.actors.at({1}).unbound_route &&
              r.candidate->state.actors.at({1}).waypoint == 0 &&
              r.candidate->state.actors.at({1}).destination == Position{3, 2},
          "rescue binding clears real G/H in both route forms but preserves old O");
    s = r.candidate->state;
    r = prepare_world_rescue_follow(s, {2});
    check(r.candidate && r.candidate->state.ai.battle.actors.at({2}).position.height == 18 &&
              r.candidate->state.ai.battle.actors.at({2}).position.x == 250 &&
              r.candidate->state.ai.contexts.at({2}).cell == Position{2, 2},
          "follow copies n and adds16, not recompute cached s");
    s.ai.human_order.erase(s.ai.human_order.begin());
    r = prepare_world_rescue_follow(s, {2});
    check(r.candidate && r.candidate->cleaned_up &&
              r.candidate->state.ai.battle.actors.at({2}).control.state == 19 &&
              !r.candidate->state.ai.battle.actors.at({2}).rescue,
          "still-resolvable carrier outside bl triggers r, not chase");
    s = fixture();
    s.facilities.at(3).status = 2;
    r = prepare_world_rescue_bind(s, {1}, CharacterId{2}, true);
    check(r.candidate && r.candidate->binding_action == RescueBindingAction::baseline,
          "only status1 inn enables rescue; status2 not equivalent to nonzero");
    s = carried();
    s.ai.battle.actors.at({1}).rescue.reset();
    r = prepare_world_rescue_follow(s, {2});
    check(r.candidate && r.candidate->cleaned_up &&
              r.candidate->state.ai.battle.actors.at({2}).hp.target == 100 &&
              r.candidate->state.ai.battle.actors.at({2}).hp.displayed == 100,
          "one-sided R repair resets all HP before follow invokes cleanup");
}
void delivery_and_timing() {
    for (bool shared : {false, true})
        for (unsigned flags : {0U, 256U, 512U, 1024U, 32768U, 768U}) {
            auto s = carried(shared);
            s.ai.battle.actors.at({2}).control.flags = flags;
            const auto r = prepare_world_rescue_delivery(s, {1});
            const int income = flags & (256U | 512U) ? 0 : 300;
            check(r.candidate && r.candidate->arrived &&
                      r.candidate->state.ai.accounting.funds() == income &&
                      r.candidate->state.facilities.at(3).sales == income &&
                      r.candidate->state.human_spending.at(shared ? 0 : 1) == 10 + income &&
                      r.candidate->state.human_spending.at(0) == 10 + (shared ? income : 0),
                  "only rescued arrival pays with OLD flags, same-definition B2 shares increment");
            auto next = r.candidate->state;
            check(next.actors.at({1}).visits.legacy_visit_counts[2] == 1 &&
                      next.actors.at({2}).visits.legacy_visit_counts[2] == 1 &&
                      next.actors.at({1}).visits.legacy_actor_total == 0 &&
                      next.actors.at({2}).visits.current_month_facility_sales == 0 &&
                      !next.ai.battle.actors.at({1}).rescue &&
                      !next.ai.battle.actors.at({2}).rescue &&
                      next.ai.battle.actors.at({1}).object_slot == -1 &&
                      next.ai.contexts.at({2}).cell == Position{1, 1} &&
                      next.ai.battle.actors.at({2}).position.x == 150,
                  "both instance counters but no second statistics owners; recursive copy follows "
                  "use");
            check(!prepare_world_rescue_delivery(next, {1}).candidate,
                  "repeat delivery rejected before charging");
            const auto carrier = prepare_world_inn_d(next, {1});
            check(carrier.candidate && carrier.candidate->occupied && carrier.candidate->exited &&
                      carrier.candidate->state.facilities.at(3).occupants.empty() &&
                      carrier.candidate->state.facility_uses.at(33).completed_uses == 1 &&
                      carrier.candidate->state.ai.battle.actors.at({1}).control.queue ==
                          std::vector<LegacyActorControl>{{8, 0}, {18, 9, 0}},
                  "carrier mode2 occupies and exits same interpreter segment, keeps activity/tail");
            next = carrier.candidate->state;
            for (int round = 0; round < 200; ++round) {
                const auto first = prepare_world_inn_c(next, {2});
                check(first.candidate && first.candidate->recovered == (round == 170),
                      "inn c reads old B170, not 170th d nor later repeated heal");
                const auto second = prepare_world_inn_d(first.candidate->state, {2});
                check(
                    second.candidate && second.candidate->occupied == (round == 0) &&
                        second.candidate->exited == (round == 199),
                    "rescued occupies once then exactly200 eligible d waits before same-call exit");
                next = second.candidate->state;
                if (round == 170)
                    check(next.ai.battle.actors.at({2}).hp.target == 100 &&
                              next.ai.battle.actors.at({2}).hp.animating,
                          "recovery uses positive capacity delta HP protocol, not immediate "
                          "display assignment");
            }
            check(next.facilities.at(3).occupants.empty() &&
                      next.facility_uses.at(33).completed_uses == 2 &&
                      next.facility_uses.at(33).upgrade_pending &&
                      next.ai.accounting.funds() == income &&
                      next.ai.battle.actors.at({2}).control.state == 0,
                  "two exits share definition use count, latch notice without recharging/level "
                  "change");
        }
}
void rollback_and_cleanup() {
    auto s = carried(true);
    s.actors.at({1}).visits.legacy_visit_counts[2] = std::numeric_limits<int>::max();
    check(!prepare_world_rescue_delivery(s, {1}).candidate && s.facilities.at(3).sales == 0 &&
              s.human_spending.at(0) == 10 && s.ai.battle.actors.at({2}).rescue == CharacterId{1},
          "late carrier visit overflow rolls back rescued income/shared B2/R/use");
    s = carried();
    s.ai.next_cash_id = 0;
    check(!prepare_world_rescue_delivery(s, {1}).candidate &&
              s.ai.battle.actors.at({2}).control.state == 16 && s.facilities.at(3).sales == 0,
          "late ledger identity failure rolls back every arrival owner");
    s = carried();
    s.actors.at({1}).binding->definition_id = 34;
    check(prepare_world_rescue_delivery(s, {1}).error == RescueWorldError::stale_binding,
          "O definition mismatch cannot deliver to rebuilt instance");
    s = carried();
    s.facilities.at(3).occupants = {{1}, {1}, {2}};
    s.ai.battle.actors.at({1}).control.flags |= 1024U;
    s.ai.battle.actors.at({1}).control.action = 11;
    s.ai.battle.actors.at({1}).control.action_counter = 17;
    s.ai.battle.actors.at({1}).control.alternate_counter = 29;
    const auto r = prepare_world_rescue_cleanup(s, {1});
    check(r.candidate &&
              r.candidate->state.facilities.at(3).occupants == std::vector<CharacterId>{{1}, {2}} &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{1, 120, 0}, {8, 5}} &&
              r.candidate->state.ai.battle.actors.at({1}).object_slot == -2 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action == 11 &&
              r.candidate->state.ai.battle.actors.at({1}).control.action_counter == 17 &&
              r.candidate->state.ai.battle.actors.at({1}).control.alternate_counter == 0,
          "cleanup removes first occupant only, optional wait120, retains N and staged departure");
    s = prepare_world_rescue_delivery(carried(), {1}).candidate->state;
    s.facility_uses.at(33).completed_uses = std::numeric_limits<int>::max();
    check(!prepare_world_inn_d(s, {1}).candidate && s.facilities.at(3).occupants.empty() &&
              s.ai.battle.actors.at({1}).state_counter == 0,
          "late shared use overflow rolls back same-call occupation and counters");
}
void rest_delivery() {
    for (int first = 0; first < 4; ++first)
        for (int second = 0; second < 4; ++second) {
            auto s = carried();
            s.facilities.at(3).category = 8;
            s.facilities.at(3).detail = 2;
            // Explicit projections represent different old cells; not source initial values.
            RescueDeliveryProjection projection{Position{250, 250}, first, Position{150, 150},
                                                second};
            const auto r = prepare_world_rescue_delivery(s, {1}, projection);
            check(r.candidate && r.candidate->arrived &&
                      r.candidate->state.ai.battle.actors.at({2}).position.x == 150 &&
                      r.candidate->state.ai.battle.actors.at({2}).control.queue.front() ==
                          LegacyActorControl{0, 250, 250} &&
                      r.candidate->state.ai.battle.actors.at({1}).control.queue.front() ==
                          LegacyActorControl{0, 150, 150} &&
                      r.candidate->state.ai.battle.actors.at({2}).control.queue[4] ==
                          LegacyActorControl{4, first} &&
                      r.candidate->state.ai.battle.actors.at({1}).control.queue[4] ==
                          LegacyActorControl{4, second} &&
                      r.candidate->state.facilities.at(3).occupants.empty(),
                  "recursive category8 uses rescued old-s target before copy, separate ordered "
                  "draws");
            projection.carrier_direction.reset();
            check(!prepare_world_rescue_delivery(s, {1}, projection).candidate &&
                      s.ai.battle.actors.at({2}).control.state == 16 &&
                      s.ai.battle.actors.at({2}).rescue == CharacterId{1} &&
                      s.facilities.at(3).sales == 0 && s.ai.accounting.funds() == 0,
                  "late second rest draw missing rolls back first use/payment/reference release");
        }
}
ActivityCandidateInput view(const RescueWorldState &s) {
    ActivityCandidateInput input;
    input.town = {0, 4, 0, 4};
    input.definitions = {{0, 0, 0}, {33, 2, 100}};
    for (const auto &cell : s.map.cells)
        input.cell_definition_ids.push_back(cell.facility ? cell.facility->definition_id : 0);
    return input;
}
void seek_and_return() {
    auto s = fixture();
    const CollisionBox box{-4, 4, 4, 4};
    auto r = prepare_world_rescue_seek(s, {1}, box, box);
    check(r.candidate && r.candidate->binding_action == RescueBindingAction::chase &&
              std::abs(r.candidate->state.ai.battle.actors.at({1}).position.x - 256.7F) < 0.001F,
          "fresh I target chases by6.7 before actual rectangle touch on next c");
    r = prepare_world_rescue_seek(r.candidate->state, {1}, box, box);
    check(r.candidate && r.candidate->binding_action == RescueBindingAction::bind,
          "fresh collision binds both actors after chase");
    s = r.candidate->state;
    s.actors.at({1}).unbound_route = LegacyPathResult{};
    const auto map_view = view(s);
    r = prepare_world_rescue_return(s, {1}, map_view);
    check(r.candidate && r.candidate->state.actors.at({1}).journey &&
              r.candidate->state.actors.at({1}).journey->binding.instance_id == BuildingId{3} &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue.empty(),
          "activity4 selects cost-minimum inn without category/facility random and consumes8");
    check(!r.candidate->state.actors.at({1}).unbound_route,
          "new inn G replaces old ground G rather than owning two routes");
    s = r.candidate->state;
    int steps{};
    while (s.ai.battle.actors.at({1}).control.state == 0 && steps < 200) {
        r = prepare_world_rescue_path_c(s, {1});
        check(r.candidate.has_value(), "return P segment prepares current map and binding");
        s = r.candidate->state;
        const auto followed = prepare_world_rescue_follow(s, {2});
        check(followed.candidate.has_value(),
              "carried actor follows moving carrier before arrival");
        s = followed.candidate->state;
        ++steps;
    }
    check(
        steps < 200 && r.candidate->arrived && s.ai.battle.actors.at({1}).control.state == 14 &&
            s.ai.battle.actors.at({2}).control.state == 14 && !s.actors.at({1}).path_pending &&
            s.ai.accounting.funds() == 300,
        "seek->bind->actual weighted return path->recursive inn delivery closes without teleport");
    s = fixture();
    s.actors.at({2}).on_event_cell = true;
    r = prepare_world_rescue_seek(s, {1}, box, box);
    check(r.candidate && r.candidate->binding_action == RescueBindingAction::baseline,
          "fresh I excludes event-flag target, not stale previously scanned down actor");
    s = fixture();
    s.ai.battle.actors.at({1}).control.flags = 64U;
    r = prepare_world_rescue_seek(s, {1}, box, box);
    check(r.candidate && r.candidate->binding_action == RescueBindingAction::chase &&
              r.candidate->state.ai.battle.actors.at({1}).position.x == 250,
          "64 prevents straight chase but does not invent baseline/cancel");
    s = carried();
    s.ai.battle.actors.at({1}).position = {350, 0, 350};
    s.ai.contexts.at({1}).cell = {3, 3};
    s.actors.at({1}).binding.reset();
    r = prepare_world_rescue_return(s, {1}, view(s));
    check(r.candidate.has_value(), "longer return route can be prepared");
    s = r.candidate->state;
    s.facilities.erase(3); // q tests actual instance identity, not construction/operating status.
    // The already selected goal remains O, but final delivery must revalidate current liveness.
    s.ai.battle.actors.at({1}).position = {150, 0, 150};
    s.ai.contexts.at({1}).cell = {1, 1};
    check(!prepare_world_rescue_path_c(s, {1}).candidate && s.ai.accounting.funds() == 0 &&
              s.ai.battle.actors.at({1}).object_slot == -2,
          "mid-route stale facility rejects without losing cargo or paying");
    s = carried();
    s.ai.battle.actors.at({1}).position = {250, 0, 250};
    s.ai.contexts.at({1}).cell = {2, 2};
    s.actors.at({1}).binding.reset();
    s.facilities.at(3).status = 2;
    r = prepare_world_rescue_return(s, {1}, view(s));
    check(r.candidate && r.candidate->state.actors.at({1}).journey &&
              r.candidate->state.actors.at({1}).journey->binding.instance_id == BuildingId{3},
          "activity4 scans full nonzero candidate snapshot, not only phase1 weighted dl");
    auto object = *prepare_ground_drop({9}, {150, 0, 150}, 0, 0);
    object.state = 3;
    object.cached_cell = {1, 1};
    s.ai.battle.objects.emplace(9, object);
    s.object_order = {9};
    r = prepare_world_rescue_return(s, {1}, view(s));
    check(r.candidate && r.candidate->departure_override &&
              r.candidate->departure_override->kind == DepartureOverrideKind::object &&
              !r.candidate->state.actors.at({1}).journey &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue.front() ==
                  LegacyActorControl{8, 4},
          "N=-2 does not suppress higher-priority object target; handoff keeps8 unconsumed");
}
} // namespace
int main() {
    binding_and_follow();
    delivery_and_timing();
    rollback_and_cleanup();
    seek_and_return();
    rest_delivery();
    std::cout << "rescue commit checks: " << checks << '\n';
}
