#include "ark/simulation/facilities/rules/facility_service.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
ResolvedArrivalInput arrival_fixture() {
    ResolvedArrivalInput i;
    i.arrival = {{1}, {2}, 30, 3, 1, 1, 0, 2U, -1, 3, 300};
    i.equipment_id = 0;
    i.equipment_price = 400;
    return i;
}
FacilityServiceExitInput exit_fixture() {
    FacilityServiceExitInput i;
    i.actor = {1};
    i.control.flags = 1U | 2U | 16U | 32U | 8192U;
    i.control.state = 14;
    i.control.action = 9;
    i.control.queue = {{24}, {3, 11}};
    i.binding_valid = true;
    i.map = {3, 3, std::vector<LegacyMapCell>(9)};
    i.facility = {{2}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    const auto map = bind_facility_map(i.map, {{i.facility, 3}});
    check(map.map.has_value(), "exit fixture complete binding");
    i.map = *map.map;
    i.position = {155, 151};
    i.category = 1;
    i.upgrade_uses = {2, 10};
    i.occupants = {{3}, {1}, {1}, {4}};
    i.satisfaction = {{1}, {2}, 33, 0, {10, 100}, 5, 0};
    i.effects = {{2, 3}, {4, 2}};
    i.effect_ticket = 1;
    return i;
}
void arrivals() {
    for (int detail : {1, 4, 5})
        for (int flags : {0, 256, 512, 768})
            for (int actor_kind : {0, 1}) {
                auto i = arrival_fixture();
                i.arrival.legacy_detail = detail;
                i.arrival.legacy_flags = static_cast<unsigned>(flags);
                i.arrival.legacy_actor_kind = actor_kind;
                auto c = prepare_resolved_arrival(i);
                const int income = flags == 0 && actor_kind == 0 ? 400 : 0;
                check(c.candidate && c.candidate->arrival.cash_income == income &&
                          c.candidate->selected_equipment == 0 &&
                          c.candidate->selected_detail == detail &&
                          c.candidate->arrival.state.legacy_visit_counts[0] == 1 &&
                          c.candidate->arrival.state.current_month_facility_sales == income,
                      "equipment selected before flags/kind payment guard, visits still count");
            }
    for (int category : {1, 2, 7})
        for (int price : {0, 200}) {
            auto i = arrival_fixture();
            i.arrival.legacy_detail = 0;
            i.equipment_id.reset();
            i.equipment_price.reset();
            i.arrival.legacy_selection = 3;
            i.arrival.legacy_category = category;
            i.arrival.resolved_instance_price = price;
            i.carried_object_exists = true;
            auto c = prepare_resolved_arrival(i);
            const bool delivers = category != 2;
            check(c.candidate && c.candidate->delivered_object.has_value() == delivers &&
                      c.candidate->object_slot == (delivers ? -1 : 3) &&
                      c.candidate->arrival.cash_income ==
                          (price == 0 ? 0 : price + (delivers ? 5000 : 0)),
                  "delivery category1/7 before visit; 5000 not payable by itself or category2");
        }
    auto i = arrival_fixture();
    i.arrival.legacy_selection = 0;
    i.carried_object_exists = true;
    i.equipment_price = std::numeric_limits<int>::max();
    i.arrival.legacy_flags = 512;
    auto c = prepare_resolved_arrival(i);
    check(c.candidate && c.candidate->arrival.cash_income == 0 &&
              c.candidate->delivered_object == 0,
          "suppressed payment never adds bonus, avoids fabricated overflow");
    i.arrival.legacy_flags = 0;
    check(prepare_resolved_arrival(i).error == FacilityServiceError::numeric_overflow,
          "payable price plus bonus overflow rejects without delivering object");
    i = arrival_fixture();
    i.equipment_id.reset();
    check(prepare_resolved_arrival(i).error == FacilityServiceError::unresolved_selection,
          "special store cannot become free ordinary use without equipment selection");
    i = arrival_fixture();
    i.arrival.legacy_selection = -2;
    check(prepare_resolved_arrival(i).error == FacilityServiceError::unresolved_selection,
          "rescued actor requires double-owner preparation");
    i = arrival_fixture();
    i.arrival.legacy_selection = 0;
    check(!prepare_resolved_arrival(i).candidate,
          "missing carried object rejects all arrival state");
}
void use_plans() {
    FacilityUsePlanInput i;
    i.control.flags = 16U | 512U | 1024U | 32768U | 2U;
    i.control.queue = {{3, 9}};
    i.definition_wait = 60;
    i.category_six_counter = 9;
    for (int category = 0; category <= 10; ++category)
        for (int detail = 0; detail <= 5; ++detail)
            for (int activity = 0; activity <= 8; ++activity) {
                i.category = category;
                i.detail = detail;
                i.activity = activity;
                i.world_target = Position{125, 140};
                i.direction_ticket = 3;
                const auto p = prepare_facility_use_plan(i);
                check(p.candidate.has_value(), "all use helper categories/details/modes resolved");
                const auto &c = *p.candidate;
                for (const auto &command : c.control.queue)
                    check(valid_actor_control(command), "service queue uses valid34 opcode shapes");
                if (category == 1 || category == 7 || category == 9)
                    check(c.control.queue ==
                                  std::vector<LegacyActorControl>{{6, 1}, {21}, {1, 60, 0}, {24}} &&
                              c.control.state == 14 && (c.control.flags & 16U) == 0,
                          "ordinary stores/services/home use definition wait and clear16");
                else if (category == 2)
                    check(c.control.queue ==
                                  (activity <= 1
                                       ? std::vector<LegacyActorControl>{{6, 1},
                                                                         {21},
                                                                         {6, 32},
                                                                         {1, 200, 0},
                                                                         {24}}
                                       : std::vector<LegacyActorControl>{{6, 1}, {21}, {24}}) &&
                              (c.control.flags & (512U | 1024U | 32768U)) == 0,
                          "inn mode2+ has no wait32, but still occupy then exit");
                else if (category == 4)
                    check(c.category_six_counter == 0 &&
                              c.control.queue == std::vector<LegacyActorControl>{{8, 6}},
                          "category4 early branch activity6; duplicate later branch unreachable");
                else if (category == 5)
                    check(c.control.queue == std::vector<LegacyActorControl>{{6, 1}, {21}},
                          "residence occupies without invented wait/exit");
                else if (category == 6 && detail == 3)
                    check(c.ground_effect20 &&
                              c.control.queue ==
                                  std::vector<LegacyActorControl>{
                                      {6, 1}, {0, 125, 140}, {1, 20, 0}, {7, 1}, {23}, {2, 15}},
                          "external exit uses movement/effect and flight state15");
                else if (category == 8 && detail == 2)
                    check(c.control.state == 14 && c.control.queue.size() == 19 &&
                              c.control.queue.front() == LegacyActorControl{0, 125, 140} &&
                              c.control.queue[4] == LegacyActorControl{4, 3} &&
                              c.control.queue.back() == LegacyActorControl{24},
                          "category8 complete movement/pose/effect/wait/exit chain");
                else
                    check(c.control.queue.empty() && c.cleanup == (category == 10),
                          "no-op category clears old queue,10 requests cleanup");
            }
    i.category = 8;
    i.detail = 2;
    i.direction_ticket.reset();
    check(prepare_facility_use_plan(i).error == FacilityServiceError::missing_ticket,
          "category8 missing direction explicit");
    i.direction_ticket = 4;
    check(prepare_facility_use_plan(i).error == FacilityServiceError::invalid_ticket,
          "direction bound strict4");
    i.category = 1;
    i.control.queue = {{33}, {99}};
    check(!prepare_facility_use_plan(i).candidate,
          "bad old queue cannot be silently reset by safety preflight");
}
void exits() {
    auto i = exit_fixture();
    auto c = prepare_facility_service_exit(i);
    check(c.candidate && c.candidate->shared_use->progress.completed_uses == 1 &&
              c.candidate->occupants == std::vector<CharacterId>{{3}, {1}, {4}} &&
              c.candidate->control.queue ==
                  std::vector<LegacyActorControl>{{8, 0}, {20, 1}, {19, 6, 4, 2}} &&
              c.candidate->satisfaction->satisfaction_delta == 1 &&
              c.candidate->position->position.x == 155 &&
              c.candidate->control.flags == (2U | 8192U),
          "full exit stages shared use,first-match release,reset and immediate satisfaction; tail "
          "deferred");
    check(c.candidate->order ==
              std::vector<FacilityExitCommit>{
                  FacilityExitCommit::position, FacilityExitCommit::shared_use,
                  FacilityExitCommit::clear_flags33, FacilityExitCommit::release_occupation,
                  FacilityExitCommit::reset_control, FacilityExitCommit::enqueue_activity,
                  FacilityExitCommit::satisfaction},
          "exit commit source order explicit");
    auto control = prepare_local_control_prefix(c.candidate->control, {std::nullopt, true});
    check(control.candidate && control.candidate->flow == ActorControlFlow::departure_started &&
              control.candidate->state.queue.size() == 2,
          "successful next activity stops interpreter before attribute effect");
    for (int detail : {1, 4, 5}) {
        i.detail = detail;
        i.equipment = {1, detail, 0, 1, 2, 2, 3};
        c = prepare_facility_service_exit(i);
        check(c.candidate && !c.candidate->satisfaction &&
                  c.candidate->control.queue.size() == 11 &&
                  c.candidate->control.queue.front() == LegacyActorControl{8, 0},
              "equipment full exit no satisfaction; display and commit queued after activity");
        const auto &commit = c.candidate->control.queue[9];
        check(prepare_equipment_commit(commit).has_value(), "equipment commit28/30 after delay");
    }
    i.category = 9;
    i.detail = 0;
    c = prepare_facility_service_exit(i);
    check(c.candidate && c.candidate->home_hp_and_visits && c.candidate->control.queue.size() == 1,
          "home resets HP/visits immediately, not through delayed attribute tail");
    i.category = 2;
    c = prepare_facility_service_exit(i);
    check(c.candidate &&
              c.candidate->control.queue == std::vector<LegacyActorControl>{{8, 0}, {18, 9, 0}},
          "inn queues expression after next activity");
    i.binding_valid = false;
    c = prepare_facility_service_exit(i);
    check(c.candidate && c.candidate->cleanup && !c.candidate->shared_use &&
              !c.candidate->position && c.candidate->occupants == i.occupants &&
              c.candidate->control.queue.empty(),
          "stale q cleanup has no partial shared use or arbitrary instance release");
    i = exit_fixture();
    i.effect_ticket = 2;
    check(prepare_facility_service_exit(i).error == FacilityServiceError::invalid_ticket &&
              i.progress.completed_uses == 0 && i.occupants.size() == 4,
          "bad late tail ticket rejects entire exit candidate");
    i = exit_fixture();
    i.map.cells[4].facility.reset();
    check(!prepare_facility_service_exit(i).candidate,
          "partial current binding rejected even when caller says live");
    i = exit_fixture();
    i.progress.completed_uses = std::numeric_limits<int>::max();
    check(prepare_facility_service_exit(i).error == FacilityServiceError::numeric_overflow,
          "overflow shared use cannot release actor first");
}
} // namespace
int main() {
    try {
        arrivals();
        use_plans();
        exits();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
