#include "ark/simulation/rules/world_departure.hpp"
#include "ark/simulation/rules/world_misc_control.hpp"

#include <algorithm>
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
RescueWorldState fixture() {
    RescueWorldState s;
    s.map = {8, 8, std::vector<LegacyMapCell>(64)};
    for (auto &cell : s.map.cells)
        cell.category = RouteCategory::road;
    std::vector<BoundFacility> facilities;
    for (const auto definition : {22, 33, 44, 55}) {
        RescueFacility f;
        f.placement = {{static_cast<std::uint64_t>(definition)},
                       definition,
                       FacilityShape::single,
                       FacilityOrientation::first,
                       definition == 22   ? Position{2, 2}
                       : definition == 33 ? Position{4, 2}
                       : definition == 44 ? Position{6, 2}
                                          : Position{6, 3}};
        f.category = definition == 22 ? 2 : definition == 33 ? 1 : definition == 44 ? 4 : 5;
        facilities.push_back({f.placement, 3});
        s.facilities.emplace(definition, f);
    }
    const auto bound = bind_facility_map(s.map, facilities);
    if (!bound.map)
        throw std::runtime_error("fixture bindings invalid");
    s.map = *bound.map;
    BattleActorRecord human;
    human.id = {1};
    human.definition = 1;
    human.control.flags = 2;
    human.position = {750, 0, 750}; // Intentionally different from cached s used by o.
    s.ai.battle.actors.emplace(human.id, human);
    s.ai.human_order = {{1}};
    s.ai.contexts.emplace(human.id, RewardActorContext{{1, 1}, true, {}, {}});
    s.actors.emplace(human.id, RescueActorContext{});
    s.ai.growth[1].definition.legacy_u = 100;
    return s;
}
WorldDepartureInput input(const RescueWorldState &s, int activity = 0) {
    WorldDepartureInput i;
    i.actor = {1};
    i.activity = activity;
    i.catalogue.town = {0, 7, 0, 7};
    i.catalogue.definitions = {{0, 0, 0}};
    i.catalogue.cell_definition_ids.assign(s.map.cells.size(), 0);
    for (const auto &[key, f] : s.facilities) {
        (void)key;
        i.catalogue.definitions.push_back({f.placement.definition_id, f.category, 10});
        i.definition_details[f.placement.definition_id] = f.detail;
    }
    for (std::size_t n = 0; n < s.map.cells.size(); ++n)
        if (s.map.cells[n].facility)
            i.catalogue.cell_definition_ids[n] = s.map.cells[n].facility->definition_id;
    i.home = WorldDepartureHome{{0, 0}, 0};
    i.exits = {{0, 4}, {7, 4}};
    return i;
}
void ordinary_and_clearing() {
    auto s = fixture();
    auto i = input(s);
    i.tickets = {0, 0};
    const auto r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded && r.candidate->goal == Position{4, 2} &&
              r.candidate->consumed_tickets == 2,
          "ordinary weighted category then live instance selects shop with two draws");
    const auto &next = r.candidate->state.actors.at({1});
    check(
        next.binding && next.binding->instance_id == BuildingId{33} &&
            next.destination == r.candidate->goal && next.journey && next.path_pending &&
            r.candidate->route->cost == 22 &&
            r.candidate->state.ai.battle.actors.at({1}).position.x == 750 &&
            r.candidate->state.ai.battle.actors.at({1}).control.facing == 1 &&
            !s.actors.at({1}).binding,
        "old cached s route and O/facing committed without motion, reservation or source changes");
    s = r.candidate->state;
    for (auto &[key, f] : s.facilities) {
        (void)key;
        f.status = 0;
    }
    auto failed = prepare_world_departure(s, input(s));
    check(
        failed.candidate && !failed.candidate->succeeded &&
            failed.candidate->denial == WorldDepartureDenial::no_candidates &&
            !failed.candidate->state.actors.at({1}).journey &&
            !failed.candidate->state.actors.at({1}).path_pending &&
            failed.candidate->state.actors.at({1}).binding->instance_id == BuildingId{33} &&
            failed.candidate->state.actors.at({1}).destination == Position{4, 2},
        "normal no-candidate failure clears G/H while preserving previous O identity/coordinates");
    i = input(fixture());
    i.tickets = {0};
    failed = prepare_world_departure(fixture(), i);
    check(!failed.candidate && failed.error == WorldDepartureError::missing_ticket,
          "missing second-level random ticket is atomic error, not gameplay false");
    i.tickets = {0, 10};
    failed = prepare_world_departure(fixture(), i);
    check(!failed.candidate && failed.error == WorldDepartureError::invalid_ticket,
          "instance ticket at exact upper bound rejected");
}
void dynamic_random_provider() {
    const auto s = fixture();
    auto i = input(s);
    std::vector<int> bounds;
    i.draw = [&](int bound) -> std::optional<std::int64_t> {
        bounds.push_back(bound);
        return 0;
    };
    auto r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded && r.candidate->consumed_tickets == 2 &&
              bounds.size() == 2 && bounds[0] > 0 && bounds[1] == 10,
          "dynamic provider observes actual category then instance bound, never pre-draws");
    const auto category_bound = bounds[0];
    bounds.clear();
    i.tickets = {0};
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->consumed_tickets == 2 && bounds == std::vector<int>{10},
          "explicit ticket prefix precedes dynamic provider without consuming duplicate category");
    i.tickets.clear();
    i.draw = [category_bound](int) -> std::optional<std::int64_t> { return category_bound; };
    r = prepare_world_departure(s, i);
    check(!r.candidate && r.error == WorldDepartureError::invalid_ticket &&
              !s.actors.at({1}).journey,
          "provider upper-bound ticket fails without publishing route");
    i.draw = [](int) -> std::optional<std::int64_t> { return {}; };
    check(prepare_world_departure(s, i).error == WorldDepartureError::missing_ticket,
          "provider exhaustion is explicit missing ticket");
    i.draw = [](int) -> std::optional<std::int64_t> { throw std::runtime_error("draw"); };
    check(prepare_world_departure(s, i).error == WorldDepartureError::preparation_failed,
          "provider exception is contained at transaction boundary");
    auto empty = s;
    for (auto &[id, f] : empty.facilities) {
        (void)id;
        f.status = 0;
    }
    int calls{};
    i = input(empty);
    i.draw = [&](int) -> std::optional<std::int64_t> {
        ++calls;
        return 0;
    };
    r = prepare_world_departure(empty, i);
    check(r.candidate && !r.candidate->succeeded && calls == 0,
          "no candidates never request a speculative draw");
}
void task_and_priority() {
    auto s = fixture();
    s.ai.task_active = true;
    s.actors.at({1}).definition_task_flag = true;
    auto i = input(s);
    i.task_center = {2, 5};
    i.catalogue.events = {{{2, 5}, 1}};
    i.tickets = {19};
    auto r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded &&
              r.candidate->priority == DepartureOverrideKind::task &&
              r.candidate->consumed_tickets == 1 && r.candidate->event116 &&
              (r.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U),
          "task first priority uses20-percent boost, not state18's12-percent multiplier");
    check(r.candidate->unbound_motion && r.candidate->state.actors.at({1}).unbound_route &&
              !r.candidate->state.actors.at({1}).binding &&
              r.candidate->state.actors.at({1}).destination == Position{2, 5} &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 0,
          "ground task retains real route/O and explicit motion handoff without c18 transition");
    i.tickets = {20};
    r = prepare_world_departure(s, i);
    check(r.candidate && !(r.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U) &&
              !r.candidate->event116,
          "task boost upper boundary remains unboosted");
    auto blocked = s;
    blocked.map.cells[42] = {0, RouteCategory::blocked, {}}; // Task(2,5) appended despite no route.
    i.tickets = {0};
    r = prepare_world_departure(blocked, i);
    check(r.candidate && !r.candidate->succeeded &&
              r.candidate->denial == WorldDepartureDenial::no_route &&
              r.candidate->consumed_tickets == 1 && r.candidate->event116 &&
              (r.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U) &&
              !r.candidate->state.actors.at({1}).destination &&
              !(blocked.ai.battle.actors.at({1}).control.flags & 2048U),
          "task boost occurs before ordinary route false, old O preserved, source still unchanged");
    s.ai.battle.actors.at({1}).control.flags |= 2048U;
    i.tickets.clear();
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->consumed_tickets == 0,
          "already boosted task departure consumes no draw or duplicate event116");
    s = fixture();
    auto down = s.ai.battle.actors.at({1});
    down.id = {2};
    down.legacy_id = 1;
    down.control.state = 2;
    down.position = {210, 0, 510};
    s.ai.battle.actors.emplace(down.id, down);
    s.ai.human_order.push_back(down.id);
    s.ai.contexts.emplace(down.id, RewardActorContext{{2, 5}, true, {}, {}});
    s.actors.emplace(down.id, RescueActorContext{});
    GroundObjectState object;
    object.id = {9};
    object.state = 3;
    object.position = {310, 0, 510};
    object.cached_cell = {3, 5};
    s.ai.battle.objects.emplace(9, object);
    s.object_order = {9};
    i = input(s);
    i.catalogue.events = {{{2, 5}, 1}, {{3, 5}, 1}};
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->priority == DepartureOverrideKind::rescue &&
              r.candidate->goal == Position{2, 5} && r.candidate->consumed_tickets == 0,
          "fresh nearest I goal outranks H without random draws");
    auto unrelated = s;
    unrelated.object_order = {123};
    unrelated.ai.encounter_order = {999};
    auto exterior = i;
    exterior.activity = 6;
    exterior.catalogue.town.bottom = 3;
    r = prepare_world_departure(unrelated, exterior);
    check(r.candidate && r.candidate->priority == DepartureOverrideKind::rescue &&
              r.candidate->goal == Position{2, 5},
          "successful rescue priority never reads unrelated invalid object/event rosters");
    s.actors.at({2}).destination = Position{2, 5};
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->priority == DepartureOverrideKind::object &&
              r.candidate->goal == Position{3, 5},
          "other actor's unbound O reservation skips rescue to nearest H");
    unrelated = s;
    unrelated.ai.encounter_order = {999};
    r = prepare_world_departure(unrelated, exterior);
    check(r.candidate && r.candidate->priority == DepartureOverrideKind::object &&
              r.candidate->goal == Position{3, 5},
          "successful H priority never reads unrelated invalid event roster");
    s.ai.battle.actors.at({1}).control.flags |= 1024U;
    i.tickets = {0, 0};
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->priority == DepartureOverrideKind::ordinary &&
              r.candidate->goal == Position{4, 2},
          "1024 disables every o priority but ordinary draw");
}
void activity_one_and_inn() {
    for (int rested : {0, 1})
        for (int count = 0; count < 5; ++count) {
            auto s = fixture();
            auto &ctx = s.actors.at({1});
            ctx.visits.legacy_category_one_count = count;
            ctx.visits.legacy_visit_counts[2] = rested;
            constexpr int before[]{30, 40, 50};
            constexpr int after[]{60, 70, 80, 90};
            const int threshold = rested ? after[std::min(count, 3)] : before[std::min(count, 2)];
            auto i = input(s, 1);
            i.tickets = {threshold - 1, 0};
            auto r = prepare_world_departure(s, i);
            check(r.candidate && r.candidate->succeeded && r.candidate->goal == Position{6, 2} &&
                      r.candidate->consumed_tickets == 2,
                  "activity1 probability selects category4 using category5 count without renaming");
            i.tickets = {threshold};
            r = prepare_world_departure(s, i);
            check(r.candidate && !r.candidate->succeeded && r.candidate->consumed_tickets == 1,
                  "activity1 exact probability threshold produces normal false");
        }
    auto s = fixture();
    auto i = input(s, 1);
    s.ai.battle.actors.at({1}).object_slot = 0;
    auto r = prepare_world_departure(s, i);
    check(r.candidate && !r.candidate->succeeded && r.candidate->consumed_tickets == 0,
          "carrying guard precedes activity1 probability consumption");
    s = fixture();
    s.facilities.at(22).status = 2;
    r = prepare_world_departure(s, input(s, 4));
    check(r.candidate && r.candidate->succeeded && r.candidate->goal == Position{2, 2} &&
              r.candidate->consumed_tickets == 0,
          "activity4 cost-min inn includes unfinished nonzero instance rather than active count");
}
void home_exit_and_routes() {
    auto s = fixture();
    auto i = input(s, 5);
    i.home = WorldDepartureHome{{2, 2}, 1};
    i.tickets = {89};
    auto r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded && r.candidate->goal == Position{2, 2} &&
              r.candidate->consumed_tickets == 1,
          "category3 home preference checks nonzero instance with one draw");
    auto home_only = i;
    home_only.exits.clear();
    r = prepare_world_departure(s, home_only);
    check(r.candidate && r.candidate->succeeded && r.candidate->goal == Position{2, 2},
          "successful home preference never requires unused exit projection");
    s.ai.contexts.at({1}).cell = {2, 1};
    i.tickets = {0, 1};
    r = prepare_world_departure(s, i);
    check(
        r.candidate && r.candidate->goal == Position{7, 4} && r.candidate->consumed_tickets == 2 &&
            r.candidate->unbound_motion,
        "equal home x alone defeats source AND guard after90-percent draw, then actual exit draw");
    s.ai.contexts.at({1}).cell = i.exits[0];
    i.home->state = 0;
    i.tickets.clear();
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->goal == i.exits[1] && r.candidate->consumed_tickets == 0,
          "at first exit choose other first-two exit without random draw");
    s = fixture();
    i = input(s, 5);
    i.exits[1] = {7, 7};
    s.map.cells[63] = {0, RouteCategory::blocked, {}};
    i.tickets = {1};
    r = prepare_world_departure(s, i);
    check(
        r.candidate && !r.candidate->succeeded &&
            r.candidate->denial == WorldDepartureDenial::no_route &&
            r.candidate->consumed_tickets == 1 && !r.candidate->state.actors.at({1}).destination,
        "selected real but unreachable exit returns false, never alternate gate/teleport fallback");
    i = input(fixture(), 5);
    i.exits.clear();
    check(!prepare_world_departure(fixture(), i).candidate,
          "missing actual exit contract cannot be replaced with display bounds");
}
void exterior_and_monster() {
    auto s = fixture();
    auto i = input(s, 6);
    i.catalogue.town = {0, 7, 0, 3};
    i.tickets = {0, 0, 0, 0, 0, 0, 0};
    auto r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded && r.candidate->unbound_motion &&
              r.candidate->consumed_tickets == 7 &&
              r.candidate->priority == DepartureOverrideKind::ordinary,
          "activity6 wildcard consumes category draw and five failed preferred cells plus sixth");
    auto wide_bounds = i;
    wide_bounds.catalogue.town.bottom = std::numeric_limits<int>::max();
    r = prepare_world_departure(s, wide_bounds);
    check(r.candidate && r.candidate->succeeded && r.candidate->consumed_tickets == 7,
          "wildcard preference cutoff uses widened arithmetic even at INT_MAX town bottom");
    RewardEncounter event;
    event.runtime.id = 4;
    event.runtime.center = {3, 5};
    s.ai.encounters.emplace(4, event);
    s.ai.encounter_order = {4};
    i.tickets.clear();
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->priority == DepartureOverrideKind::encounter &&
              r.candidate->goal->x >= 2 && r.candidate->goal->x <= 4 && r.candidate->goal->y >= 4 &&
              r.candidate->goal->y <= 6 && r.candidate->consumed_tickets == 0,
          "activity6 nearest encounter chooses first cost-ordered candidate in its square");
    s.ai.encounters.at(4).runtime.center = {std::numeric_limits<int>::min(),
                                            std::numeric_limits<int>::min()};
    r = prepare_world_departure(s, i);
    check(!r.candidate && r.error == WorldDepartureError::invalid_input,
          "extreme encounter center is rejected before squaring distance arithmetic");
    s.ai.encounters.clear();
    s.ai.encounter_order.clear();
    auto &actor = s.ai.battle.actors.at({1});
    actor.kind = ActorKind::monster;
    s.ai.human_order.clear();
    s.ai.monster_order = {{1}};
    i = input(s, 7);
    i.tickets = {0};
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded && r.candidate->consumed_tickets == 1 &&
              r.candidate->priority == DepartureOverrideKind::ordinary,
          "monster activity7 uses entire interior snapshot and no human priority");
    i = input(s, 8);
    i.catalogue.town = {0, 7, 0, 3};
    i.tickets = {0};
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded && r.candidate->unbound_motion &&
              r.candidate->consumed_tickets == 1,
          "activity8 no special detail3 falls back to whole exterior snapshot with actual draw");
    i.catalogue.definitions.push_back({66, 6, 0});
    i.definition_details[66] = 3;
    i.catalogue.cell_definition_ids[40] = 66; // (0,5), ground point with detail3.
    i.catalogue.cell_definition_ids[41] = 66; // (1,5), same definition, independently drawn cell.
    i.tickets = {1};
    r = prepare_world_departure(s, i);
    check(r.candidate && r.candidate->succeeded && r.candidate->consumed_tickets == 1 &&
              (r.candidate->goal == Position{0, 5} || r.candidate->goal == Position{1, 5}),
          "activity8 independently counts detail3 cells without charm/phase/instance weighting");
    i.tickets = {2};
    r = prepare_world_departure(s, i);
    check(!r.candidate && r.error == WorldDepartureError::invalid_ticket,
          "activity8 special-cell count rejects ticket at its real bound");
    i.tickets = {0};
    i.definition_details.erase(66);
    r = prepare_world_departure(s, i);
    check(!r.candidate && r.error == WorldDepartureError::invalid_catalogue,
          "missing original detail projection does not silently become fallback ground");
}
void fifo_departure_control() {
    auto s = fixture();
    auto &actor = s.ai.battle.actors.at({1});
    actor.control.state = 19;
    actor.control.action = 6;
    actor.control.action_counter = 42;
    actor.control.alternate_counter = 7;
    actor.baseline = 5;
    actor.state_counter = 99;
    actor.state_parameter = 8;
    actor.control.queue = {{8, 0}, {19, 1, 2, 3}};
    WorldDepartureControlInput i;
    i.departure = input(s);
    i.departure.tickets = {0, 0};
    auto r = prepare_world_departure_control(s, i);
    check(r.candidate && r.candidate->departure_succeeded && !r.candidate->continue_interpreter &&
              !r.candidate->delete_instance &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{19, 1, 2, 3}},
          "successful actual front8 is removed before o, remaining attribute tail holds this d");
    const auto &started = r.candidate->state.ai.battle.actors.at({1});
    check(started.control.state == 0 && started.state_counter == 99 &&
              started.state_parameter == 8 && started.baseline == 5 &&
              started.control.action == 6 && started.control.action_counter == 42 &&
              started.control.alternate_counter == 7,
          "8 success writes A0 only, never c0, counters/growth/action or tail eager execution");
    check(s.ai.battle.actors.at({1}).control.queue.front() == LegacyActorControl{8, 0},
          "successful FIFO candidate never changes its source queue");
    for (const unsigned flags :
         {0U, 512U, 1024U, 32768U, 512U | 32768U, 1024U | 32768U, 512U | 1024U | 32768U}) {
        auto failed = s;
        auto &a = failed.ai.battle.actors.at({1});
        a.control.flags = flags;
        a.control.queue = {{8, 2}, {19, 1, 2, 3}};
        a.object_slot = 7;
        a.rescue = CharacterId{3};
        a.encounter = 4;
        a.group = 4;
        a.position.height = 9;
        auto request = i;
        request.departure.tickets.clear();
        request.failure_expression = WorldExpressionTicket{999, 3, 1};
        auto result = prepare_world_departure_control(failed, request);
        const bool immediate_delete = (flags & 1024U) && (flags & 32768U);
        check(result.candidate && !result.candidate->departure_succeeded &&
                  result.candidate->delete_instance == immediate_delete &&
                  result.candidate->continue_interpreter == !immediate_delete &&
                  result.candidate->cleaned_up == !immediate_delete &&
                  result.candidate->consumed_expression == static_cast<bool>(flags & 1024U) &&
                  result.candidate->consumed_variant == static_cast<bool>(flags & 1024U),
              "old1024/32768 ordering controls real expression, delete versus r/same-d resume");
        const auto &next = result.candidate->state.ai.battle.actors.at({1});
        if (immediate_delete) {
            check(next.control.flags == flags && next.control.state == 19 && next.rescue &&
                      next.encounter && next.group && next.position.height == 9 &&
                      next.control.queue == std::vector<LegacyActorControl>{{19, 1, 2, 3}} &&
                      result.candidate->state.ai.human_order == std::vector<CharacterId>{{1}},
                  "true deletes only by request, before512/r, preserving live owner and old tail");
        } else {
            const bool waiting = (flags & (512U | 1024U)) != 0;
            check(next.control.state == 19 && next.control.action == 6 &&
                      next.control.action_counter == 42 && next.control.alternate_counter == 0 &&
                      next.state_counter == 0 && next.state_parameter == 0 &&
                      next.object_slot == 7 && !next.rescue && !next.encounter && !next.group &&
                      next.position.height == 0 && (next.control.flags & 514U) == 514U &&
                      next.control.queue ==
                          (waiting ? std::vector<LegacyActorControl>{{1, 120, 0}, {8, 5}}
                                   : std::vector<LegacyActorControl>{{8, 5}}),
                  "human r replaces tail, preserves k/l/N and sets c19, optional120 then8(5)");
            if (waiting) {
                const auto continued = prepare_local_control_prefix(next.control);
                check(continued.candidate &&
                          continued.candidate->flow == ActorControlFlow::waiting &&
                          continued.candidate->state.queue.front() == LegacyActorControl{1, 119, 0},
                      "same-d continuation actually consumes first wait tick120->119");
            } else {
                auto again = request;
                again.departure.tickets = {0};
                const auto continued =
                    prepare_world_departure_control(result.candidate->state, again);
                check(continued.candidate && continued.candidate->departure_succeeded &&
                          continued.candidate->state.ai.battle.actors.at({1}).control.state == 0 &&
                          continued.candidate->state.actors.at({1}).destination == Position{0, 4},
                      "same-d second8 from r actually routes to real exit and stops after success");
            }
        }
        if (flags & 1024U)
            check(
                result.candidate->state.ai.contexts.at({1}).effects.display.front() ==
                    ActorEffectRecord{12, 0, 30, 18, 1},
                "failed8 real c18 consumes platform variant and inserts untouched display record");
    }
    auto failed = s;
    failed.ai.battle.actors.at({1}).control.flags = 1024U | 32768U;
    failed.ai.battle.actors.at({1}).control.queue = {{8, 2}};
    i.departure = input(failed);
    i.failure_expression = WorldExpressionTicket{0, 2, {}};
    r = prepare_world_departure_control(failed, i);
    check(!r.candidate && r.error == WorldDepartureError::missing_ticket &&
              failed.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 2}},
          "missing unsuppressed expression variant rolls back removed8 and o's path clearing");
    failed.ai.contexts.at({1}).effects.display = {{24, 3, 0, 1, 0, 0}};
    r = prepare_world_departure_control(failed, i);
    check(
        r.candidate && r.candidate->delete_instance && r.candidate->consumed_expression &&
            !r.candidate->consumed_variant &&
            r.candidate->state.ai.contexts.at({1}).effects.display ==
                failed.ai.contexts.at({1}).effects.display,
        "existing24 suppresses failed8 expression after probability without variant/timeline tick");
    failed.ai.battle.actors.at({1}).control.queue = {LegacyActorControl{}};
    check(!prepare_world_departure_control(failed, i).candidate,
          "empty malformed front command rejected without accessing opcode/index");
    auto monster = s;
    auto &m = monster.ai.battle.actors.at({1});
    m.kind = ActorKind::monster;
    m.control.queue = {{8, 2}, {19, 1, 2, 3}};
    m.control.flags = 2;
    m.rescue = CharacterId{3};
    m.encounter = 4;
    m.group = 4;
    m.object_slot = 7;
    m.position.height = 9;
    monster.ai.human_order.clear();
    monster.ai.monster_order = {{1}};
    monster.ai.contexts.at({1}).cell = {2, 2};
    monster.actors.at({1}).binding = ArrivalBinding{{2, 2}, {22}, 22};
    monster.actors.at({1}).destination = Position{2, 2};
    monster.facilities.at(22).occupants = {{1}, {1}};
    i.departure = input(monster);
    i.failure_expression.reset();
    r = prepare_world_departure_control(monster, i);
    check(r.candidate && r.candidate->cleaned_up && r.candidate->continue_interpreter &&
              r.candidate->state.facilities.at(22).occupants == std::vector<CharacterId>{{1}} &&
              r.candidate->state.actors.at({1}).binding &&
              r.candidate->state.actors.at({1}).destination == Position{2, 2},
          "monster r releases only first matching q occupant and retains O coordinates/identity");
    const auto &cleaned = r.candidate->state.ai.battle.actors.at({1});
    check(cleaned.control.state == 0 && cleaned.control.action == 0 &&
              cleaned.control.action_counter == 0 &&
              cleaned.control.queue == std::vector<LegacyActorControl>{{8, 5}} &&
              cleaned.object_slot == 7 && !cleaned.rescue && !cleaned.encounter && !cleaned.group &&
              cleaned.position.height == 0,
          "monster uses real c0 reset only, no human wait nor extra n0/N/path rewrite");
}
WorldPathInput path_input(const RescueWorldState &s) {
    WorldPathInput i;
    i.actor = {1};
    i.facts = {s.map,
               std::vector<int>(s.map.cells.size(), 0),
               std::vector<std::uint32_t>(s.map.cells.size(), 0),
               {0, 7, 0, 7}};
    i.exits = std::vector<Position>{{0, 4}, {7, 4}};
    return i;
}
RescueWorldState inn_journey() {
    auto s = fixture();
    s.ai.battle.actors.at({1}).position = {150, 9, 150};
    s.ai.contexts.at({1}).half_cell = {3, 3};
    s.human_spending[1] = 0;
    s.facilities.at(22).price = 17;
    const auto d = prepare_world_departure(s, input(s, 4));
    if (!d.candidate || !d.candidate->succeeded)
        throw std::runtime_error("real inn departure fixture failed");
    return d.candidate->state;
}
void path_real_journey_and_entry() {
    auto s = inn_journey();
    const auto old = s;
    auto r = prepare_world_path_c(s, path_input(s));
    check(r.candidate && r.candidate->moved && !r.candidate->arrived &&
              std::abs(r.candidate->state.ai.battle.actors.at({1}).position.x - 156.7F) < .001F &&
              r.candidate->state.ai.contexts.at({1}).cell == Position{1, 1} &&
              r.candidate->state.ai.contexts.at({1}).half_cell == Position{3, 3} &&
              r.candidate->state.ai.battle.actors.at({1}).position.height == 9 &&
              r.candidate->state.ai.battle.actors.at({1}).state_counter == 0,
          "real o route P moves6.7 n only, preserving old s/t/height and no shared d prefix");
    check(std::abs(r.candidate->state.actors.at({1}).horizontal_velocity.x - 6.7F) < .001F &&
              old.ai.battle.actors.at({1}).position.x == 150,
          "P preserves source snapshot and writes actual r.x/z rather than visual interpolation");
    auto blocked = s;
    blocked.ai.battle.actors.at({1}).control.flags |= 64U;
    r = prepare_world_path_c(blocked, path_input(blocked));
    check(r.candidate && !r.candidate->moved && !r.candidate->advanced_waypoint,
          "bit64 prevents route movement without consuming a waypoint");
    // 调度的d尾部才刷新s；此夹具只组合该投影，不冒充完整d计数/物理验收。
    bool reached = false;
    for (int tick = 0; tick < 80 && !reached; ++tick) {
        const auto before = s;
        r = prepare_world_path_c(s, path_input(s));
        check(r.candidate.has_value(), "each original route c step prepares successfully");
        reached = r.candidate->arrived;
        s = r.candidate->state;
        check(s.ai.contexts.at({1}).cell == before.ai.contexts.at({1}).cell,
              "every P step leaves cached s to the original d projection point");
        if (!reached) {
            const auto p = prepare_world_actor_projection(s.ai, {1});
            check(p.candidate.has_value(), "real d-tail projection is available after P motion");
            s.ai = p.candidate->state;
        }
    }
    check(reached && s.ai.battle.actors.at({1}).control.state == 14 && !s.actors.at({1}).journey &&
              !s.actors.at({1}).unbound_route && !s.actors.at({1}).path_pending &&
              s.actors.at({1}).waypoint == 0 && s.actors.at({1}).destination == Position{2, 2} &&
              s.actors.at({1}).binding && s.actors.at({1}).visits.legacy_visit_counts[2] == 1 &&
              s.facilities.at(22).sales == 17 && s.human_spending.at(1) == 17,
          "old s admission commits real arrival/accounting/use and O clears G/H only");
    check(s.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{6, 1}, {21}, {6, 32}, {1, 200, 0}, {24}} &&
              s.facilities.at(22).occupants.empty(),
          "P only arranges inn use; occupancy starts later at actual control21");
    check(!prepare_world_path_c(s, path_input(s)).candidate,
          "state14 cannot replay P to charge a second arrival");
    auto helper = inn_journey();
    helper.ai.contexts.at({1}).cell = {2, 2};
    helper.ai.battle.actors.at({1}).control.flags |= 256U;
    r = prepare_world_path_c(helper, path_input(helper));
    check(
        r.candidate && !(r.candidate->state.ai.battle.actors.at({1}).control.flags & 256U) &&
            r.candidate->state.facilities.at(22).sales == 0 &&
            r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                std::vector<LegacyActorControl>{{6, 1}, {21}, {24}},
        "old256 suppresses arrival fee then is cleared before exact helper use2 without inn wait");
    auto empty = inn_journey();
    empty.actors.at({1}).journey->route.steps.clear();
    empty.ai.contexts.at({1}).cell = {2, 2};
    r = prepare_world_path_c(empty, path_input(empty));
    check(r.candidate && !r.candidate->arrived && r.candidate->state.facilities.at(22).sales == 0,
          "G empty never enters even when old s already equals O");
}
void path_waypoint_identity_and_ground() {
    auto s = inn_journey();
    auto &ctx = s.actors.at({1});
    auto &a = s.ai.battle.actors.at({1});
    const auto first = ctx.journey->route.steps.front();
    a.position = {first.x * 100.0F + 50, 0, first.y * 100.0F + 50};
    a.control.flags |= 64U;
    auto r = prepare_world_path_c(s, path_input(s));
    check(r.candidate && r.candidate->advanced_waypoint && !r.candidate->moved &&
              r.candidate->state.actors.at({1}).waypoint == 1,
          "blocked actor can consume overlap of a nonfinal G point exactly as b(world) returns");
    ctx.waypoint = ctx.journey->route.steps.size() - 1;
    a.position = {250, 0, 250};
    r = prepare_world_path_c(s, path_input(s));
    check(r.candidate && !r.candidate->advanced_waypoint && !r.candidate->arrived &&
              r.candidate->state.actors.at({1}).waypoint == ctx.waypoint,
          "last G point remains until old s reaches O, never increments H out of bounds");
    a.position = {250, 0, 290};
    s.map.cells[18].legacy_state = 6;
    auto i = path_input(s);
    r = prepare_world_path_c(s, i);
    check(!r.candidate && r.error == WorldPathError::missing_fact,
          "entry6 waypoint rejects missing current definition direction rather than assume q");
    i.definition_directions[22] = 0;
    r = prepare_world_path_c(s, i);
    check(r.candidate && !r.candidate->moved,
          "definition direction0 offsets entry6 point to z290 while bit64 retains n");
    i.definition_directions[22] = 4;
    check(!prepare_world_path_c(s, i).candidate, "direction outside0..3 is rejected");
    for (const auto state : {6, 7})
        for (int direction = 0; direction < 4; ++direction) {
            s.map.cells[18].legacy_state = state;
            a.position = {direction < 2    ? 250.0F
                          : direction == 2 ? 290.0F
                                           : 210.0F,
                          7,
                          direction >= 2   ? 250.0F
                          : direction == 0 ? 290.0F
                                           : 210.0F};
            a.control.flags |= 64U;
            ctx.horizontal_velocity = {2, 3};
            i = path_input(s);
            i.definition_directions[22] = direction;
            r = prepare_world_path_c(s, i);
            check(r.candidate && !r.candidate->moved && !r.candidate->advanced_waypoint &&
                      r.candidate->state.actors.at({1}).horizontal_velocity.x == 2 &&
                      r.candidate->state.actors.at({1}).horizontal_velocity.z == 3,
                  "state6/7 all four definition entrances retain last H and old r under64");
            a.control.flags &= ~64U;
            r = prepare_world_path_c(s, i);
            check(r.candidate && !r.candidate->moved &&
                      r.candidate->state.actors.at({1}).horizontal_velocity.x == 2 &&
                      r.candidate->state.actors.at({1}).horizontal_velocity.z == 3,
                  "exact entry point distance0 preserves source r instead of zeroing velocity");
        }
    auto stale = inn_journey();
    stale.ai.contexts.at({1}).cell = {2, 2};
    stale.map.cells[18].facility->instance_id = {999};
    r = prepare_world_path_c(stale, path_input(stale));
    check(r.candidate && r.candidate->cleaned_up && !r.candidate->arrived &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 19 &&
              r.candidate->state.actors.at({1}).journey &&
              r.candidate->state.actors.at({1}).destination == Position{2, 2} &&
              r.candidate->state.facilities.at(22).sales == 0,
          "j instance mismatch performs real r, retains old G/O and does not charge replacement");
    auto ground = fixture();
    ground.ai.task_active = true;
    ground.actors.at({1}).definition_task_flag = true;
    auto departure = input(ground);
    departure.task_center = {2, 5};
    departure.catalogue.events = {{{2, 5}, 1}};
    departure.tickets = {99};
    const auto d = prepare_world_departure(ground, departure);
    check(d.candidate && d.candidate->unbound_motion, "real task departure constructs unbound G");
    ground = d.candidate->state;
    ground.ai.contexts.at({1}).cell = {2, 5};
    ground.map.cells[42].facility = ground.map.cells[18].facility;
    auto g = path_input(ground);
    g.exits.reset();
    r = prepare_world_path_c(ground, g);
    check(!r.candidate && r.error == WorldPathError::missing_fact &&
              ground.actors.at({1}).unbound_route,
          "missing real Map.f rolls back O route clearing rather than infer a nonexit");
    g.exits = std::vector<Position>{};
    r = prepare_world_path_c(ground, g);
    check(r.candidate && r.candidate->arrived && !r.candidate->scheduled_exit &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 5 &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{10, 0}} &&
              !r.candidate->state.actors.at({1}).unbound_route &&
              r.candidate->state.actors.at({1}).destination == Position{2, 5},
          "unbound O2=-1 still enters ground c5/10,0 after a building appears at that tile");
    auto outside = inn_journey();
    outside.ai.contexts.at({1}).cell = {-1, 2};
    r = prepare_world_path_c(outside, path_input(outside));
    check(r.candidate && !r.candidate->moved && r.candidate->state.actors.at({1}).journey,
          "old s outside map returns false before route movement and preserves G");
}
void path_exit_and_monster() {
    auto s = fixture();
    auto d = prepare_world_departure(s, input(s, 5));
    auto departure = input(s, 5);
    departure.tickets = {0};
    d = prepare_world_departure(s, departure);
    check(d.candidate && d.candidate->succeeded, "real activity5 builds first actual exit route");
    s = d.candidate->state;
    s.ai.contexts.at({1}).cell = *d.candidate->goal;
    s.ai.battle.actors.at({1}).position = {50, 0, 450};
    s.ai.battle.actors.at({1}).control.queue = {{9}};
    auto r = prepare_world_path_c(s, path_input(s));
    check(r.candidate && r.candidate->scheduled_exit && !r.candidate->delete_instance &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{9}, {0, 50, 400}, {26}} &&
              r.candidate->state.ai.human_order == std::vector<CharacterId>{{1}},
          "actual exit appends center z-50 movement then26 behind old m tail, c stays false");
    auto exit = r.candidate->state;
    for (int tick = 0; tick < 12 && exit.ai.battle.actors.at({1}).control.queue.front()[0] != 26;
         ++tick) {
        const auto motion = prepare_world_facility_control(exit, {{1}, {}, {}});
        check(motion.candidate.has_value(), "actual opcode0 consumes exit motion separate from G");
        exit = motion.candidate->state;
    }
    WorldMiscControlState owner{exit, {{1, 0}}};
    const auto retire = prepare_world_misc_control(owner, {{1}, {}, {}});
    check(retire.candidate && retire.candidate->action == WorldControlAction::delete_true &&
              retire.candidate->state.human_definition_state.at(1) == 1 &&
              retire.candidate->state.world.ai.human_order == std::vector<CharacterId>{{1}},
          "later true26 changes shared m only and delegates physical roster deletion to scheduler");
    auto monster = s;
    monster.ai.battle.actors.at({1}).kind = ActorKind::monster;
    monster.ai.human_order.clear();
    monster.ai.monster_order = {{1}};
    r = prepare_world_path_c(monster, path_input(monster));
    check(r.candidate && r.candidate->scheduled_exit && !r.candidate->delete_instance &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue.back() ==
                  LegacyActorControl{26},
          "monster ground exit also queues26 and never pretends category3 c=true happened");
    monster = inn_journey();
    monster.ai.human_order.clear();
    monster.ai.monster_order = {{1}};
    monster.ai.battle.actors.at({1}).kind = ActorKind::monster;
    monster.ai.contexts.at({1}).cell = {2, 2};
    monster.facilities.at(22).category = 3;
    monster.ai.battle.actors.at({1}).control.queue = {{19, 1, 2, 3}};
    r = prepare_world_path_c(monster, path_input(monster));
    check(r.candidate && r.candidate->delete_instance && r.candidate->arrived &&
              r.candidate->path_returned_true &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue.empty() &&
              r.candidate->state.ai.battle.actors.at({1}).state_counter == 0 &&
              r.candidate->state.ai.monster_order == std::vector<CharacterId>{{1}} &&
              r.candidate->state.facilities.at(22).sales == 0,
          "monster category3 commits actual arrival then w and returns c=true without running d");
    monster.ai.battle.actors.at({1}).control.state = 17;
    for (const int mode : {1, 4}) {
        monster.actors.at({1}).monster_mode = mode;
        r = prepare_world_path_c(monster, path_input(monster));
        check(r.candidate && r.candidate->path_returned_true && !r.candidate->delete_instance &&
                  r.candidate->state.ai.battle.actors.at({1}).control.state == 17 &&
                  r.candidate->state.ai.monster_order == std::vector<CharacterId>{{1}},
              "monster17/T1/T4 consume P=true without propagating state0 c-deletion semantics");
        auto travelling = monster;
        travelling.ai.contexts.at({1}).cell = {1, 1};
        travelling.ai.battle.actors.at({1}).position = {150, 0, 150};
        r = prepare_world_path_c(travelling, path_input(travelling));
        check(r.candidate && r.candidate->moved && !r.candidate->path_returned_true &&
                  !r.candidate->delete_instance &&
                  r.candidate->state.ai.battle.actors.at({1}).control.state == 17 &&
                  r.candidate->state.actors.at({1}).journey,
              "monster17 actual P advances retained route while mode qualification stays in c");
    }
    monster.ai.battle.actors.at({1}).control.state = 0;
    monster.facilities.at(22).category = 2;
    r = prepare_world_path_c(monster, path_input(monster));
    check(r.candidate && !r.candidate->delete_instance &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 17 &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 7}},
          "other monster facilities route to c17/activity7, not human use14");
    monster.facilities.at(22).category = 6;
    monster.facilities.at(22).detail = 3;
    auto i = path_input(monster);
    r = prepare_world_path_c(monster, i);
    check(!r.candidate && monster.actors.at({1}).journey,
          "missing special h.e target rejects all arrival/route/counter changes atomically");
    i.use_world_target = {420, 260};
    r = prepare_world_path_c(monster, i);
    check(r.candidate && r.candidate->ground_effect20 &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{
                      {6, 1}, {0, 420, 260}, {1, 20, 0}, {7, 1}, {23}, {2, 15}},
          "monster6/3 arranges real special entry plan with effect20 and later launch23/state15");
}
void path_real_f_preemption() {
    auto s = inn_journey();
    s.ai.battle.actors.at({1}).state_counter = 5;
    s.ai.battle.actors.at({1}).legacy_id = 1;
    s.ai.contexts.at({1}).move_area = true;
    RewardEncounter event;
    event.runtime.id = 9;
    event.runtime.center = {1, 1};
    event.legacy_id = 9;
    s.ai.encounters.emplace(9, event);
    s.ai.encounter_order = {9};
    auto i = path_input(s);
    i.facts.flags[9] = 2;
    auto r = prepare_world_path_c(s, i);
    check(!r.candidate && r.error == WorldPathError::missing_ticket &&
              !s.ai.battle.actors.at({1}).encounter && s.ai.battle.events.empty(),
          "real F matching event needs c18 boost draw, rolls back db binding when missing");
    i.boost_ticket = 0;
    r = prepare_world_path_c(s, i);
    check(r.candidate && r.candidate->event_preempted && r.candidate->consumed_boost_ticket &&
              r.candidate->event116 && !r.candidate->moved &&
              r.candidate->state.ai.battle.actors.at({1}).encounter == 9 &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 18 &&
              r.candidate->state.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{10, 1}} &&
              r.candidate->state.actors.at({1}).journey &&
              r.candidate->state.ai.battle.actors.at({1}).state_counter == 0,
          "F precedes travel and commits db,c18,12-percent boost,event116 while preserving G");
    i.boost_ticket = 12;
    r = prepare_world_path_c(s, i);
    check(r.candidate && r.candidate->consumed_boost_ticket && !r.candidate->event116 &&
              !(r.candidate->state.ai.battle.actors.at({1}).control.flags & 2048U),
          "F c18 strict12 boundary does not reuse o task's20-percent threshold");
    i.boost_ticket = 100;
    check(prepare_world_path_c(s, i).error == WorldPathError::invalid_ticket,
          "invalid boost ticket does not commit matching-event db or replace m");
    auto arriving = s;
    arriving.ai.contexts.at({1}).cell = {2, 2};
    arriving.ai.encounters.at(9).runtime.center = {2, 2};
    i = path_input(arriving);
    i.facts.flags[18] = 2;
    i.boost_ticket = 99;
    r = prepare_world_path_c(arriving, i);
    check(r.candidate && r.candidate->event_preempted && !r.candidate->arrived &&
              r.candidate->state.facilities.at(22).sales == 0 &&
              r.candidate->state.actors.at({1}).journey,
          "actual F preempts even old s==O so P never clears G or charges facility in that c");
    auto suppressed = s;
    suppressed.ai.battle.actors.at({1}).control.flags |= 2048U;
    i = path_input(suppressed);
    i.facts.flags[9] = 2;
    r = prepare_world_path_c(suppressed, i);
    check(r.candidate && !r.candidate->consumed_boost_ticket,
          "already2048 F c18 does not require or consume a new boost ticket");
    auto other = s.ai.battle.actors.at({1});
    other.id = {2};
    other.legacy_id = 2;
    s.ai.battle.actors.emplace(other.id, other);
    s.ai.human_order.push_back(other.id);
    s.ai.contexts.emplace(other.id, RewardActorContext{{2, 1}, true, {}, {}});
    s.actors.emplace(other.id, RescueActorContext{});
    i = path_input(s);
    i.facts.flags[9] = 2;
    i.boost_ticket = 99;
    r = prepare_world_path_c(s, i);
    check(!r.candidate && r.error == WorldPathError::missing_ticket,
          "adjacent different legacyID must consume actual expression7 before c18");
    i.nearby_expression = WorldExpressionTicket{999, 1, 0};
    r = prepare_world_path_c(s, i);
    check(r.candidate && r.candidate->consumed_expression && r.candidate->consumed_variant &&
              r.candidate->state.ai.contexts.at({1}).effects.display.front()[3] == 7,
          "F nearby expression7 probability1000 always requires actual platform variant");
    s.ai.contexts.at({1}).effects.display = {{24, 3, 0, 1, 0, 0}};
    i.nearby_expression = WorldExpressionTicket{999, 1, {}};
    r = prepare_world_path_c(s, i);
    check(r.candidate && r.candidate->consumed_expression && !r.candidate->consumed_variant,
          "existing display24 suppresses F nearby variant only after real probability draw");
    auto task = inn_journey();
    task.ai.task_active = true;
    task.ai.battle.actors.at({1}).state_counter = 5;
    task.ai.contexts.at({1}).move_area = true;
    i = path_input(task);
    i.task = {true, 1, {1, 1}, {}};
    i.boost_ticket = 99;
    r = prepare_world_path_c(task, i);
    check(!r.candidate && r.error == WorldPathError::missing_domain,
          "task F cannot silently skip its actual creation attempt");
    int attempts = 0;
    i.task_attempt = [&](const AiRewardState &ai, const WorldMapFacts &facts, CharacterId id,
                         const WorldEventTask &view) -> std::optional<WorldEventEntryCandidate> {
        ++attempts;
        check(id == CharacterId{1} && view.kind == 1,
              "task creation adapter receives current source task and stable actor identity");
        // 实际上方带碰到城界的拒绝，仍由原事件消费者完成创建尝试。
        return prepare_world_event_entry(ai, facts, {id, view, 3, 2, 4}).candidate;
    };
    r = prepare_world_path_c(task, i);
    check(r.candidate && attempts == 1 && r.candidate->attempted_task_creation &&
              r.candidate->event_preempted &&
              !r.candidate->state.ai.battle.actors.at({1}).encounter &&
              r.candidate->task_creation_denial == EncounterCreationDenial::town &&
              !r.candidate->created_task_encounter && !r.candidate->music2 &&
              !r.candidate->notice24,
          "creation attempt failure still yields real F=true,c18 without fabricated task event");
    i.task_attempt = [](const AiRewardState &ai, const WorldMapFacts &facts, CharacterId id,
                        const WorldEventTask &view) -> std::optional<WorldEventEntryCandidate> {
        auto next = prepare_world_event_entry(ai, facts, {id, view, 3, 2, 4}).candidate;
        if (next)
            next->state.human_order.clear();
        return next;
    };
    check(prepare_world_path_c(task, i).error == WorldPathError::invalid_callback,
          "task callback cannot delete or reorder actors within state0 P");
    auto outside_task = task;
    outside_task.ai.contexts.at({1}).cell = {1, 4};
    outside_task.ai.contexts.at({1}).inside_town = false;
    i = path_input(outside_task);
    i.facts.town = {4, 7, 0, 7};
    i.task = {true, 1, {1, 4}, {}};
    i.boost_ticket = 99;
    i.task_attempt = [](const AiRewardState &ai, const WorldMapFacts &facts, CharacterId id,
                        const WorldEventTask &view) -> std::optional<WorldEventEntryCandidate> {
        return prepare_world_event_entry(ai, facts, {id, view, 3, 2, 4}).candidate;
    };
    r = prepare_world_path_c(outside_task, i);
    check(r.candidate && r.candidate->created_task_encounter == 1 &&
              r.candidate->task.encounter == 1 && r.candidate->music2 && r.candidate->notice24 &&
              (r.candidate->facts.flags[33] & 2U) &&
              r.candidate->state.ai.encounters.at(1).runtime.quota == 7 &&
              !r.candidate->state.ai.battle.actors.at({1}).encounter &&
              r.candidate->state.actors.at({1}).journey,
          "actual F task creation preserves facts bit2,k.g,quota,music/notice and no same-call db "
          "bind");
    if (r.candidate) {
        auto later_state = r.candidate->state.ai;
        later_state.battle.actors.at({1}).state_counter = 5;
        const auto later =
            prepare_world_event_gate(later_state, {1}, r.candidate->facts, r.candidate->task);
        check(later.candidate && later.candidate->gate.bind_encounter == 1,
              "later qualified B5 real F consumes returned task/map projections and binds event");
    }
    auto monster = task;
    monster.ai.task_active = false;
    monster.ai.human_order = {{2}};
    monster.ai.monster_order = {{1}};
    monster.ai.battle.actors.at({1}).kind = ActorKind::monster;
    monster.ai.battle.actors.at({1}).perceived_enemy = CharacterId{2};
    other.kind = ActorKind::human;
    monster.ai.battle.actors.emplace(other.id, other);
    monster.ai.contexts.emplace(other.id, RewardActorContext{{1, 1}, true, {}, {}});
    monster.actors.emplace(other.id, RescueActorContext{});
    i = path_input(monster);
    r = prepare_world_path_c(monster, i);
    check(r.candidate && r.candidate->event_preempted &&
              r.candidate->state.ai.battle.actors.at({1}).control.state == 1 &&
              !r.candidate->consumed_boost_ticket && r.candidate->state.actors.at({1}).journey,
          "monster real F uses current opponent side then c1, never human c18 or route motion");
}
void path_domain_callback_and_strict_failure() {
    auto s = inn_journey();
    s.ai.contexts.at({1}).cell = {2, 2};
    auto carried = s.ai.battle.actors.at({1});
    carried.id = {2};
    carried.definition = 2;
    carried.object_slot = -1;
    carried.control.state = 16;
    carried.rescue = CharacterId{1};
    s.ai.battle.actors.emplace(carried.id, carried);
    s.ai.human_order.push_back(carried.id);
    s.ai.contexts.emplace(carried.id, RewardActorContext{{1, 1}, true, {}, {}});
    s.actors.emplace(carried.id, RescueActorContext{});
    s.human_spending[2] = 0;
    s.ai.battle.actors.at({1}).object_slot = -2;
    s.ai.battle.actors.at({1}).rescue = carried.id;
    auto i = path_input(s);
    auto r = prepare_world_path_c(s, i);
    check(!r.candidate && r.error == WorldPathError::missing_domain && s.actors.at({1}).journey &&
              s.ai.battle.actors.at({1}).rescue == carried.id,
          "recursive rescue arrival requires explicit real consumer, not ordinary no-payment stub");
    int calls = 0;
    i.facility_consumer =
        [&](const RescueWorldState &state,
            const WorldPathFacilityRequest &request) -> std::optional<WorldPathFacilityCandidate> {
        ++calls;
        check(request.human_arrival_and_use && request.binding.instance_id == BuildingId{22} &&
                  !state.actors.at({1}).journey && state.actors.at({1}).waypoint == 0,
              "real domain consumer runs only after P has cleared G/H and confirmed O identity");
        const auto rescue = prepare_world_rescue_delivery(state, request.actor);
        if (!rescue.candidate)
            return {};
        return WorldPathFacilityCandidate{rescue.candidate->state, false};
    };
    r = prepare_world_path_c(s, i);
    check(
        r.candidate && calls == 1 && r.candidate->arrived &&
            !r.candidate->state.ai.battle.actors.at({1}).rescue &&
            !r.candidate->state.ai.battle.actors.at({2}).rescue &&
            r.candidate->state.ai.battle.actors.at({1}).object_slot == -1 &&
            r.candidate->state.ai.battle.actors.at({2}).control.state == 14 &&
            r.candidate->state.ai.contexts.at({2}).cell == Position{2, 2} &&
            r.candidate->state.facilities.at(22).sales == 17,
        "actual rescue delivery callback atomically charges rescued only and selects helper use2");
    i.facility_consumer =
        [](const RescueWorldState &state,
           const WorldPathFacilityRequest &) -> std::optional<WorldPathFacilityCandidate> {
        return WorldPathFacilityCandidate{state, false};
    };
    check(prepare_world_path_c(s, i).error == WorldPathError::invalid_callback,
          "noop complex callback cannot falsely report original arrival J was committed");
    i.facility_consumer =
        [](const RescueWorldState &,
           const WorldPathFacilityRequest &) -> std::optional<WorldPathFacilityCandidate> {
        throw std::runtime_error("fixture failure");
    };
    check(prepare_world_path_c(s, i).error == WorldPathError::invalid_callback &&
              s.actors.at({1}).journey && s.facilities.at(22).sales == 0,
          "domain callback exception rolls back route clearing and all financial/rescue state");
    for (const int mutation : {0, 1, 2}) {
        i.facility_consumer = [mutation](const RescueWorldState &state,
                                         const WorldPathFacilityRequest &request)
            -> std::optional<WorldPathFacilityCandidate> {
            const auto rescue = prepare_world_rescue_delivery(state, request.actor);
            if (!rescue.candidate)
                return {};
            auto next = rescue.candidate->state;
            if (mutation == 0)
                next.ai.human_order.clear();
            else if (mutation == 1)
                next.ai.battle.actors.at(request.actor).position.x += 1;
            else
                next.actors.at(request.actor).destination = Position{3, 3};
            return WorldPathFacilityCandidate{std::move(next), false};
        };
        check(prepare_world_path_c(s, i).error == WorldPathError::invalid_callback &&
                  s.actors.at({1}).journey && s.facilities.at(22).sales == 0,
              "even real rescue candidate cannot alter P roster,self n or O identity");
    }
    auto invalid = inn_journey();
    auto facts = path_input(invalid);
    facts.facts.map.cells[0].legacy_state = 0;
    check(prepare_world_path_c(invalid, facts).error == WorldPathError::invalid_input,
          "F facts must describe same current map as owner, not a stale snapshot");
    invalid.actors.at({1}).unbound_route = invalid.actors.at({1}).journey->route;
    check(!prepare_world_path_c(invalid, path_input(invalid)).candidate,
          "simultaneous bound and unbound G authorities rejected");
    invalid = inn_journey();
    invalid.actors.at({1}).waypoint = invalid.actors.at({1}).journey->route.steps.size();
    check(!prepare_world_path_c(invalid, path_input(invalid)).candidate,
          "invalid H rejected instead of out-of-bounds route access");
    invalid = inn_journey();
    invalid.ai.contexts.at({1}).cell = {2, 2};
    invalid.human_spending.clear();
    check(prepare_world_path_c(invalid, path_input(invalid)).error ==
                  WorldPathError::missing_fact &&
              invalid.actors.at({1}).journey && invalid.facilities.at(22).sales == 0,
          "missing shared B2 cannot be filled with zero to manufacture ordinary arrival");
}
} // namespace
int main() {
    try {
        ordinary_and_clearing();
        dynamic_random_provider();
        task_and_priority();
        activity_one_and_inn();
        home_exit_and_routes();
        exterior_and_monster();
        fifo_departure_control();
        path_real_journey_and_entry();
        path_waypoint_identity_and_ground();
        path_exit_and_monster();
        path_real_f_preemption();
        path_domain_callback_and_strict_failure();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
