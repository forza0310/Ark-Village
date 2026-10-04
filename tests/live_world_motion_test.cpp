// Live owner integration of WORLD_DEPARTURE/CONTROL old-s routing and retained FIFO ordering.
// Explicit actor/home/route fixtures exercise published branches; they are NOT new-game facts.
#include "ark/app/game.hpp"
#include "ark/people/activity_candidates.hpp"
#include "ark/people/activity_choice.hpp"
#include "ark/people/motion.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark;
namespace {
int checks{};
void check(bool ok, const char *why) {
    ++checks;
    if (!ok)
        throw std::runtime_error(why);
}
app::Game arrived(std::uint32_t seed = 20261003U) {
    app::Game game(seed);
    for (int n = 0; n < 420; ++n)
        check(game.update() == app::Error::none, "real startup countdown");
    check(game.life_state() != nullptr, "first normal actor exists");
    while (game.state().mode == app::Mode::tutorial)
        check(game.acknowledge_talk() == app::Error::none, "close actual tutorial");
    check(game.finish_camera() == app::Error::none, "finish actual introductory camera");
    return game;
}
app::LifeActorState actor_fixture(const app::Game &game, world::Cell cell) {
    auto actor = *game.life_state();
    actor.control = {};
    actor.counters = {};
    actor.retention = {};
    actor.position = {cell.x * 100.0F + 50, cell.y * 100.0F + 50};
    actor.cached_cell = cell;
    actor.destination = cell;
    actor.journey.reset();
    actor.active_facility.reset();
    actor.unbound_route.reset();
    actor.waypoint = 0;
    actor.error = app::InitialAiError::none;
    actor.handoff = app::LifeHandoff::none;
    actor.pending_activity.reset();
    actor.pending_category.reset();
    actor.pending_definition.reset();
    actor.route_revision = game.state().layout_revision;
    return actor;
}
void round(app::InitialAiSession &engine, app::InitialAiTickets tickets = {}) {
    check(engine.round(tickets) == app::InitialAiError::none, "live candidate round succeeds");
    check(engine.state().error == app::InitialAiError::none, "no hidden actor handoff");
}
int category_ticket(const app::Game &game, const app::LifeActorState &actor, int category) {
    const auto map = game.route_map();
    const auto field = world::search(map, actor.cached_cell);
    check(field.field.has_value(), "real current-map search for explicit category fixture");
    people::ActivityCandidateInput input;
    input.legacy_activity = 0;
    const auto b = app::startup_data().build_bounds;
    input.town = {b.min_x - 1, b.max_x + 1, b.min_y - 1, b.max_y + 1};
    input.last_visited_instance = actor.visits.last_visited_instance;
    for (const auto &cell : map.cells)
        input.cell_definition_ids.push_back(cell.definition_id);
    for (const auto &d : app::startup_data().definitions)
        input.definitions.push_back({d.id, d.activity_category, d.economy.attributes[2].first});
    for (const auto id : game.state().instance_order) {
        const auto &f = game.state().facilities.at(id);
        input.instances.push_back({id, f.definition_id, f.remaining_ticks ? 0 : 1});
    }
    const auto candidates = people::collect_activity_candidates(*field.field, input);
    check(candidates.snapshot.has_value(), "actual map category candidates");
    people::ActivityChoiceInput choice;
    choice.available_category_counts = candidates.snapshot->category_counts;
    choice.legacy_flags = actor.control.flags;
    std::copy(actor.visits.legacy_visit_counts.begin(), actor.visits.legacy_visit_counts.end(),
              choice.legacy_visit_counts.begin());
    const auto plan = people::plan_activity_categories(choice);
    check(plan.plan && !plan.plan->forced_category, "fixture has a real weighted category choice");
    std::int64_t ticket{};
    for (const auto &option : plan.plan->options) {
        if (option.category == category && option.weight > 0) {
            check(ticket <= std::numeric_limits<int>::max(), "category fixture ticket fits");
            return static_cast<int>(ticket);
        }
        ticket += option.weight;
    }
    throw std::runtime_error("requested category has no actual candidate weight");
}
void exit_route(const app::Game &game) {
    auto actor = actor_fixture(game, {11, 6});
    // D/state0 is an explicit branch fixture, not an assertion about the first visitor's home.
    actor.home = app::LifeHome{{11, 6}, 0};
    actor.control.queue = {{8, 5}};
    auto engine = app::InitialAiSession::from_village(game, actor);
    round(engine);
    check(engine.state().unbound_route && !engine.state().journey &&
              !engine.state().unbound_route->steps.empty() && engine.state().control.queue.empty(),
          "actual activity5 creates an unbound route and consumes only its departure command");
    const auto goal = engine.state().destination;
    check(std::find(app::startup_data().spawn_points.begin(),
                    app::startup_data().spawn_points.end(),
                    goal) != app::startup_data().spawn_points.end(),
          "exit destination is a published spawn point");
    bool exit_fifo{}, moved{};
    for (int n = 0; n < 700 && !engine.state().removed; ++n) {
        const auto old = engine.state().position;
        round(engine);
        moved |= engine.state().position.x != old.x || engine.state().position.z != old.z;
        const auto &queue = engine.state().control.queue;
        if (queue.size() >= 2 && queue[0][0] == 0 && queue[1][0] == 26) {
            exit_fifo = true;
            check(queue[0][1] == goal.x * 100 + 50 && queue[0][2] == goal.y * 100,
                  "exit FIFO moves to the original edge anchor before definition departure");
        }
    }
    check(moved && exit_fifo && engine.state().removed && engine.state().definition_departed,
          "route, exit0 and control26 complete real instance removal and shared departure flag");
}
void cached_arrival_and_flags(const app::Game &game) {
    auto actor = actor_fixture(game, {11, 6});
    actor.destination = {11, 7};
    actor.unbound_route = world::Route{world::RouteError::none, {{11, 7}}, 7};
    // n has entered O, while old s still names the preceding tile. c must not enter yet.
    actor.position = {1150, 701};
    auto engine = app::InitialAiSession::from_village(game, actor);
    round(engine);
    check(engine.state().control.state == 0 && engine.state().unbound_route &&
              engine.state().cached_cell == actor.destination && engine.state().waypoint == 0,
          "c uses old cached cell; d projects new cell and retains the final route point");
    round(engine);
    check(engine.state().control.state == 5 && !engine.state().unbound_route &&
              !engine.state().active_facility,
          "only the following c enters unbound ground and clears route");

    actor.position = {1150, 650};
    actor.control.flags = 64;
    auto blocked = app::InitialAiSession::from_village(game, actor);
    round(blocked);
    check(blocked.state().position.x == actor.position.x &&
              blocked.state().position.z == actor.position.z && blocked.state().waypoint == 0 &&
              blocked.state().retention.blocked_updates == 1,
          "flags64 blocks actual motion while common retention still counts exactly once");
}
void building_on_unbound_goal(app::Game game) {
    check(game.open_catalog() == app::Error::none && game.select(33) == app::Error::none,
          "actual building commands open placement");
    std::optional<world::Cell> goal;
    const auto b = app::startup_data().build_bounds;
    for (int y = b.min_y; y <= b.max_y && !goal; ++y)
        for (int x = b.min_x; x <= b.max_x && !goal; ++x)
            if (game.preview({x, y}) == app::Error::none)
                goal = world::Cell{x, y};
    check(goal.has_value(), "current map has legal construction space");
    auto actor = actor_fixture(game, *goal);
    actor.unbound_route = world::Route{world::RouteError::none, {*goal}, 0};
    const auto funds = game.state().money;
    check(game.confirm(*goal) == app::Error::none && game.facility_at(*goal).has_value() &&
              game.state().money < funds,
          "real construction changes map and funds after the unbound route was selected");
    game.cancel();
    auto engine = app::InitialAiSession::from_village(game, actor);
    round(engine);
    check(engine.state().control.state == 5 && !engine.state().journey &&
              !engine.state().active_facility && engine.state().arrivals == actor.arrivals &&
              engine.state().accounting.funds() == game.state().money,
          "unbound arrival identity survives later building and does not invent a shop visit");
}
void gate_to_ground(const app::Game &game) {
    auto actor = actor_fixture(game, {11, 6});
    actor.control.queue = {{8, 0}};
    app::InitialAiTickets tickets;
    tickets.category = category_ticket(game, actor, 4);
    auto engine = app::InitialAiSession::from_village(game, actor);
    round(engine, tickets);
    check(engine.state().journey && engine.state().journey->category == 4,
          "category4 makes a real bound route to the outside entrance");
    const auto departures = engine.state().departures;
    for (int n = 0; n < 700 && engine.state().departures == departures; ++n)
        round(engine);
    check(engine.state().departures == departures + 1 && engine.state().unbound_route &&
              !engine.state().journey && !engine.state().active_facility,
          "outside-entrance arrival executes actual8,6 and creates an unbound ground route");
}
void wander_and_rollback(const app::Game &game) {
    auto actor = actor_fixture(game, {11, 6});
    // control10 is independently callable; normal state5's preceding L remains an explicit gap.
    actor.control.queue = {{10, 0}, {1, 9, 0}, {4, 2}};
    actor.counters.state = 17;
    actor.control.action_counter = 6;
    actor.hp = {1, 5, 5, 6, true, 11};
    auto engine = app::InitialAiSession::from_village(game, actor);
    round(engine);
    const auto &s = engine.state();
    check(s.control.queue.size() > 2 && s.control.queue[0] == people::LegacyActorControl{1, 8, 0} &&
              s.control.queue[1] == people::LegacyActorControl{4, 2},
          "wander removes itself and appends behind the preserved old FIFO tail");
    check(s.counters.state == 18 && s.control.action_counter == 7 && s.hp.legacy_tick == 12 &&
              s.rounds == actor.rounds + 1,
          "wander FIFO continuation does not repeat d counters or HP advancement");

    actor.control.queue = {{8, 5}};
    actor.home.reset();
    auto missing = app::InitialAiSession::from_village(game, actor);
    std::mt19937 random(871);
    const auto before = random;
    check(missing.round_random(random) == app::InitialAiError::none &&
              missing.state().error == app::InitialAiError::unsupported_branch &&
              missing.state().handoff == app::LifeHandoff::home_projection &&
              missing.state().pending_category == 3 && !missing.state().unbound_route,
          "missing definition home is a recorded handoff, never assumed to mean no home");
    check(random == before &&
              missing.round_random(random) == app::InitialAiError::unsupported_branch &&
              random == before,
          "direct activity5 missing-home handoff draws nothing and repeated handoff cannot reroll");

    actor.home = app::LifeHome{{11, 6}, 0};
    actor.retention.bad_area_updates = std::numeric_limits<int>::max();
    auto invalid = app::InitialAiSession::from_village(game, actor);
    check(invalid.round_random(random) == app::InitialAiError::preparation_failed &&
              random == before,
          "late retention failure rolls back earlier random exit selection");
    check(!invalid.state().unbound_route && invalid.state().control.queue == actor.control.queue &&
              invalid.state().hp.legacy_tick == actor.hp.legacy_tick &&
              invalid.state().rounds == actor.rounds,
          "late failure keeps route, FIFO, HP and admitted count unchanged");
}
void invalid_numeric_input(const app::Game &game) {
    for (const float x : {std::numeric_limits<float>::quiet_NaN(),
                          std::numeric_limits<float>::infinity(), 2000000.0F}) {
        auto actor = actor_fixture(game, {11, 4});
        actor.position.x = x;
        auto engine = app::InitialAiSession::from_village(game, actor);
        std::mt19937 random(42), original = random;
        check(engine.round_random(random) == app::InitialAiError::invalid_input &&
                  random == original,
              "invalid float is rejected before integer map projection, with RNG unchanged");
    }
    auto actor = actor_fixture(game, {11, 4});
    actor.control.state = 21;
    auto engine = app::InitialAiSession::from_village(game, actor);
    check(engine.round({}) == app::InitialAiError::invalid_input,
          "invalid actor state does not dereference an absent area candidate");
}
void normal_exterior() {
    auto game = arrived(20261004U);
    bool gate{}, ground_route{}, ground_state{};
    for (int n = 0; n < 1000; ++n) {
        const auto *actor = game.life_state();
        check(actor != nullptr, "normal exterior slice retains actor before research handoff");
        gate |= actor->journey && actor->journey->category == 4;
        ground_route |= actor->unbound_route.has_value();
        ground_state |= actor->control.state == 5;
        if (actor->error != app::InitialAiError::none) {
            check(actor->handoff == app::LifeHandoff::encounter_creation &&
                      actor->pending_activity == 6,
                  "normal actor reaches explicit L/encounter aggregation gap, not category4 stop");
            break;
        }
        check(game.update() == app::Error::none, "normal shared scheduler advances exterior route");
    }
    check(gate && ground_route && ground_state &&
              game.life_state()->handoff == app::LifeHandoff::encounter_creation,
          "unmodified normal seed traverses outside entrance and ground before honest L handoff");
}
} // namespace
int main() {
    try {
        const auto game = arrived();
        exit_route(game);
        cached_arrival_and_flags(game);
        building_on_unbound_goal(game);
        gate_to_ground(game);
        wander_and_rollback(game);
        invalid_numeric_input(game);
        normal_exterior();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
