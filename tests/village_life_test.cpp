// Normal startup integration: live construction, life AI and cash share the Game aggregate.
#include "ark/app/game.hpp"
#include "ark/people/activity_candidates.hpp"
#include "ark/people/activity_choice.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace ark;
namespace {
int checks{};
void check(bool pass, const char *message) {
    ++checks;
    if (!pass)
        throw std::runtime_error(message);
}
void steps(app::Game &game, int count, int speed = 1) {
    for (int i = 0; i < count; ++i)
        check(game.update(speed) == app::Error::none, "eligible Game update");
}
void close_tutorial(app::Game &game) {
    while (game.state().mode == app::Mode::tutorial)
        check(game.acknowledge_talk() == app::Error::none, "close actual introduction lines");
    check(game.finish_camera() == app::Error::none, "close actual camera introduction");
}
app::Game arrived(std::uint32_t seed = 20261003U) {
    app::Game game(seed);
    steps(game, 420);
    check(!game.ai_preview_enabled() && !game.ai_state() && game.life_state() &&
              game.state().mode == app::Mode::tutorial,
          "normal startup installs life AI without preview");
    return game;
}
void reconcile(const app::Game &game) {
    std::int64_t expected = app::startup_data().money;
    for (const auto &[id, entry] : game.state().accounting.entries()) {
        check(id == entry.event_id && entry.amount >= 0, "stable positive cash event");
        expected +=
            entry.direction == economy::CashDirection::income ? entry.amount : -entry.amount;
    }
    check(expected == game.state().money && expected == game.state().accounting.funds(),
          "HUD, construction and service cash reconcile with one ledger");
}
std::string fingerprint(const app::Game &game) {
    const auto &s = game.state();
    std::ostringstream out;
    out << std::setprecision(17) << s.simulation_steps << ',' << s.money << ',' << s.next_cash_id
        << ',' << s.layout_revision << ',' << s.arrival_counter << ',' << s.event89_count << ','
        << static_cast<int>(s.mode) << ',' << s.selection.value_or(-1) << ',' << s.paused;
    for (const auto n : s.calendar)
        out << ',' << n;
    for (const auto &[id, f] : s.facilities)
        out << ";f" << id << ',' << f.definition_id << ',' << f.anchor.x << ',' << f.anchor.y << ','
            << f.remaining_ticks << ',' << f.orientation;
    for (const auto &[id, p] : s.definition_progress)
        out << ";p" << id << ',' << p.level << ',' << p.completed_uses << ',' << p.upgrade_pending;
    for (const auto &[id, f] : s.facility_life) {
        out << ";s" << id << ',' << f.sales.current_month_facility_sales;
        for (const auto actor : f.occupants)
            out << ',' << actor.value;
    }
    for (const auto &[id, cash] : s.accounting.entries())
        out << ";c" << id << ',' << cash.sequence << ',' << cash.amount << ','
            << static_cast<int>(cash.direction);
    if (const auto *a = game.life_state()) {
        out << ";a" << a->rounds << ',' << a->position.x << ',' << a->position.z << ','
            << a->control.state << ',' << a->control.flags << ',' << a->counters.state << ','
            << a->waypoint << ',' << a->route_revision << ',' << a->arrivals << ',' << a->departures
            << ',' << a->completions << ',' << a->satisfaction << ',' << static_cast<int>(a->error)
            << ',' << a->pending_category.value_or(-1);
        for (const auto &command : a->control.queue)
            for (const auto n : command)
                out << ',' << n;
        if (a->journey) {
            out << ";j" << a->journey->binding.instance;
            for (const auto cell : a->journey->route.steps)
                out << ',' << cell.x << ',' << cell.y;
        }
        if (a->active_facility)
            out << ";o" << a->active_facility->instance;
    }
    return out.str();
}
facilities::InstanceId build(app::Game &game, int definition, world::Cell cell) {
    check(game.open_catalog() == app::Error::none && game.select(definition) == app::Error::none,
          "real construction UI commands accepted");
    check(game.confirm(cell) == app::Error::none, "real current-map placement accepted");
    const auto id = game.facility_at(cell);
    check(id.has_value(), "new instance occupies current map");
    game.cancel();
    reconcile(game);
    return *id;
}

// Observe the complete first service in normal startup, including the actual once-only payment.
void autonomous_lifecycle() {
    auto game = arrived();
    check(game.life_state()->departures == 1 && game.life_state()->journey &&
              game.life_state()->rounds == 1 && !game.state().adventurer->pending_activity,
          "first visitor selects a facility in its admitted arrival round");
    const auto modal = fingerprint(game);
    steps(game, 80, 2);
    check(fingerprint(game) == modal, "tutorial freezes actor, cash, dates and construction");
    close_tutorial(game);
    const auto starting_position = game.life_state()->position;
    const auto start_date = game.state().calendar;
    steps(game, 1);
    check(game.life_state()->position.x != starting_position.x ||
              game.life_state()->position.z != starting_position.z,
          "normal update moves the autonomous actor");
    check(game.state().calendar != start_date, "life does not suspend calendar advancement");
    for (int n = 0; n < 700 && game.life_state()->arrivals == 0; ++n)
        steps(game, 1);
    const auto &a = *game.life_state();
    check(a.arrivals == 1 && a.active_facility && a.control.state == 14 && a.occupations == 1,
          "walking reaches arrival, payment and facility occupation");
    const auto active = *a.active_facility;
    const auto paid = game.state().money;
    const auto entries = game.state().accounting.entries().size();
    check(paid > 5000 && entries == 1 &&
              game.state().facility_life.at(active.instance).occupants ==
                  std::vector<people::ActorId>{a.actor},
          "normal service creates real income and one occupation");
    check(game.state().facility_life.at(active.instance).sales.current_month_facility_sales ==
              paid - 5000,
          "actual instance owns its sales");
    reconcile(game);
    steps(game, 10);
    check(game.state().money == paid && game.state().accounting.entries().size() == entries &&
              game.life_state()->completions == 0,
          "waiting neither repeats arrival nor charges a second time");
    for (int n = 0; n < 250 && game.life_state()->completions == 0; ++n)
        steps(game, 1);
    check(game.life_state()->completions == 1 && !game.life_state()->active_facility &&
              game.state().facility_life.at(active.instance).occupants.empty() &&
              game.state().definition_progress.at(active.definition_id).completed_uses == 1,
          "exit releases occupancy and commits definition-shared use progress");
    const auto context = app::InitialAiSession::from_village(game, *game.life_state());
    check(context.state().uses.at(active.definition_id).completed_uses == 1 &&
              context.state().accounting.entries() == game.state().accounting.entries(),
          "next AI context retains actual shared progress and cash history");
    reconcile(game);
}

// Menus, pause and placement suspend all owners together; admitted double steps match single steps.
void qualification_and_speed() {
    auto game = arrived();
    close_tutorial(game);
    const auto building = build(game, 28, {7, 3});
    game.set_paused(true);
    auto frozen = fingerprint(game);
    steps(game, 100, 2);
    check(fingerprint(game) == frozen, "pause freezes construction, actor and calendar");
    game.set_paused(false);
    game.open_catalog();
    frozen = fingerprint(game);
    steps(game, 100, 2);
    check(fingerprint(game) == frozen, "catalog freezes all simulation owners");
    game.select(66);
    frozen = fingerprint(game);
    steps(game, 100, 2);
    check(fingerprint(game) == frozen, "placement freezes all simulation owners");
    game.cancel();
    steps(game, 1);
    check(game.state().facilities.at(building).remaining_ticks == 279,
          "resume advances one construction tick without accumulated debt");
    auto slow = game;
    auto fast = game;
    steps(slow, 120);
    steps(fast, 60, 2);
    check(fingerprint(slow) == fingerprint(fast),
          "identical eligible rounds produce identical life, calendar, construction and income");
}

// Building at different actor phases must refresh bindings without resetting life or cash.
void live_construction() {
    app::Game before;
    const auto new_inn = build(before, 28, {7, 3});
    steps(before, 420);
    check(before.life_state() && before.life_state()->error == app::InitialAiError::none &&
              before.state().facilities.at(new_inn).remaining_ticks == 0 &&
              before.state().money == 4000,
          "pre-visit construction no longer disqualifies normal AI startup");
    close_tutorial(before);
    for (int n = 0; n < 800 && before.life_state()->arrivals == 0; ++n)
        steps(before, 1);
    check(before.life_state()->arrivals == 1 && before.state().money > 4000,
          "built map and spent cash remain live through first autonomous service");
    reconcile(before);

    auto walking = arrived();
    close_tutorial(walking);
    steps(walking, 8);
    const auto old_target = walking.life_state()->journey->binding.instance;
    const auto old_rounds = walking.life_state()->rounds;
    build(walking, 66, {7, 3});
    steps(walking, 1);
    check(walking.life_state()->error == app::InitialAiError::none &&
              walking.life_state()->rounds == old_rounds + 1 &&
              walking.life_state()->journey->binding.instance == old_target &&
              walking.life_state()->route_revision == walking.state().layout_revision,
          "construction while walking refreshes current route without reselecting target");
    for (int n = 0; n < 700 && !walking.life_state()->active_facility; ++n)
        steps(walking, 1);
    check(walking.life_state()->active_facility.has_value(),
          "live route reaches its selected target");
    const auto active = *walking.life_state()->active_facility;
    const auto rounds = walking.life_state()->rounds;
    const auto paid = walking.state().money;
    const auto entries = walking.state().accounting.entries().size();
    const auto inn = build(walking, 28, {8, 3});
    check(walking.life_state()->active_facility->instance == active.instance &&
              walking.state().facility_life.at(active.instance).occupants.size() == 1 &&
              walking.state().money == paid - 1000,
          "construction while using retains actual occupation and charges construction once");
    steps(walking, 1);
    check(walking.life_state()->rounds == rounds + 1 &&
              walking.state().facilities.at(inn).remaining_ticks == 279 &&
              walking.state().accounting.entries().size() == entries + 1,
          "construction, service wait and shared ledger advance together");
    for (int n = 0; n < 250 && walking.life_state()->completions == 0; ++n)
        steps(walking, 1);
    check(walking.life_state()->completions == 1 &&
              walking.state().facility_life.at(active.instance).occupants.empty(),
          "live-layout service can still exit and release occupancy");
    reconcile(walking);
}

people::ActivityCandidateSnapshot candidates(const app::Game &game,
                                             const app::LifeActorState &actor) {
    const auto map = game.route_map();
    const auto cell = people::world_cell(actor.position);
    check(cell.has_value(), "candidate actor position");
    const auto field = world::search(map, *cell);
    check(field.field.has_value(), "current-map reachable field");
    people::ActivityCandidateInput input;
    const auto bounds = app::startup_data().build_bounds;
    input.town = {bounds.min_x - 1, bounds.max_x + 1, bounds.min_y - 1, bounds.max_y + 1};
    input.last_visited_instance = actor.visits.last_visited_instance;
    for (const auto &tile : map.cells)
        input.cell_definition_ids.push_back(tile.definition_id);
    for (const auto &d : app::startup_data().definitions)
        input.definitions.push_back({d.id, d.activity_category, d.economy.attributes[2].first});
    for (const auto id : game.state().instance_order) {
        const auto &f = game.state().facilities.at(id);
        input.instances.push_back({id, f.definition_id, f.remaining_ticks > 0 ? 0 : 1});
    }
    const auto result = people::collect_activity_candidates(*field.field, input);
    check(result.snapshot.has_value(), "valid candidates on actual current map");
    return *result.snapshot;
}
app::LifeActorState ready_actor(const app::Game &game) {
    auto actor = *game.life_state();
    actor.control.state = 0;
    actor.control.queue = {{8, 0}};
    actor.journey.reset();
    actor.active_facility.reset();
    actor.visits = {};
    actor.error = app::InitialAiError::none;
    actor.position = {1150.0F, 50.0F};
    actor.cached_cell = {11, 0};
    return actor;
}
app::InitialAiTickets ticket_for(const people::ActivityCandidateSnapshot &snapshot,
                                 const app::LifeActorState &actor, facilities::InstanceId target) {
    const auto found =
        std::find_if(snapshot.cells.begin(), snapshot.cells.end(), [&](const auto &c) {
            return c.instance && c.instance->instance_id == target;
        });
    check(found != snapshot.cells.end(), "requested completed instance is selectable");
    std::array<std::int64_t, 6> visits{};
    std::copy(actor.visits.legacy_visit_counts.begin(), actor.visits.legacy_visit_counts.end(),
              visits.begin());
    const auto plan = people::plan_activity_categories(
        {0, snapshot.category_counts, visits, actor.control.flags});
    check(plan.plan && !plan.plan->forced_category, "ordinary category ticket has a weighted plan");
    int category_ticket = 0;
    bool category_found{};
    for (const auto &option : plan.plan->options) {
        if (option.category == found->definition.legacy_category && option.weight > 0) {
            category_found = true;
            break;
        }
        category_ticket += static_cast<int>(option.weight);
    }
    check(category_found, "target category is enabled by real original weights");
    int facility_ticket = 0;
    for (const auto &c : snapshot.cells) {
        if (c.instance && c.instance->instance_id == target)
            break;
        if (c.instance && c.instance->legacy_phase == 1 &&
            c.definition.legacy_category == found->definition.legacy_category)
            facility_ticket += static_cast<int>(c.definition.definition_charm);
    }
    return {category_ticket, facility_ticket, 0, 0, std::nullopt};
}

// A phase0 instance must enter the selection set only after actual Game construction completes.
void construction_qualification() {
    auto game = arrived();
    close_tutorial(game);
    const auto inn = build(game, 28, {7, 3});
    auto actor = ready_actor(game);
    const auto unavailable = candidates(game, actor);
    check(std::none_of(unavailable.cells.begin(), unavailable.cells.end(),
                       [&](const auto &c) { return c.instance && c.instance->instance_id == inn; }),
          "construction0 is excluded from current activity candidates");
    steps(game, 279);
    check(game.state().facilities.at(inn).remaining_ticks == 1,
          "construction completion is not early");
    const auto revision = game.state().layout_revision;
    steps(game, 1);
    check(game.state().facilities.at(inn).remaining_ticks == 0 &&
              game.state().layout_revision == revision + 1,
          "completion publishes a new usable layout revision");
    actor = ready_actor(game);
    const auto available = candidates(game, actor);
    auto session = app::InitialAiSession::from_village(game, actor);
    check(session.round(ticket_for(available, actor, inn)) == app::InitialAiError::none &&
              session.state().journey && session.state().journey->binding.instance == inn,
          "fresh AI context can actually select the player-built completed inn");
}

// Local ticket input proves arrival derives the current instance price, including new neighbours.
void live_neighbour_fee() {
    auto game = arrived();
    close_tutorial(game);
    const auto food =
        std::find_if(game.state().facilities.begin(), game.state().facilities.end(),
                     [](const auto &entry) { return entry.second.definition_id == 33; });
    check(food != game.state().facilities.end(), "source initial food shop exists");
    const auto food_id = food->first;
    bool added{};
    for (const auto cell :
         facilities::surroundings(game.definition(33).shape, food->second.orientation,
                                  food->second.anchor, app::startup_data().map)) {
        game.open_catalog();
        game.select(66);
        if (game.preview(cell) == app::Error::none) {
            check(game.confirm(cell) == app::Error::none, "actual neighbouring plant placement");
            added = true;
        }
        game.cancel();
        if (added)
            break;
    }
    check(added, "construct a legal neighbour using current map bounds");
    auto actor = ready_actor(game);
    auto session = app::InitialAiSession::from_village(game, actor);
    const auto opening = session.state().accounting.funds();
    check(session.round(ticket_for(candidates(game, actor), actor, food_id)) ==
              app::InitialAiError::none,
          "select actual food instance on modified village");
    for (int n = 0; n < 700 && session.state().arrivals == actor.arrivals; ++n)
        check(session.round({0, 0, 0, 0, {}}) == app::InitialAiError::none,
              "current-neighbour journey");
    check(
        session.state().active_facility && session.state().active_facility->instance == food_id &&
            session.state().accounting.funds() - opening ==
                game.facility_values(33, food_id).instance[0],
        "charged income equals current level, improvements, jobs and instance neighbourhood quote");
    check(session.state().facilities.at(food_id).sales.current_month_facility_sales ==
              game.state().facility_life.at(food_id).sales.current_month_facility_sales +
                  game.facility_values(33, food_id).instance[0],
          "instance sales and actual payment agree after neighbouring construction");
}

// An unconsumed researched handoff retains its selection; only that actor stops, not the world.
void actor_handoff_and_month_boundary() {
    auto game = arrived();
    close_tutorial(game);
    for (int n = 0; n < 850 && game.life_state()->error == app::InitialAiError::none; ++n)
        steps(game, 1);
    const auto &a = *game.life_state();
    check(a.error == app::InitialAiError::unsupported_branch &&
              (a.pending_category || a.pending_definition || a.pending_activity),
          "unknown branch records its selected category, definition or activity without reroll");
    // WORLD_DEPARTURE now removes8 before o. A category4 choice proceeds through its gate
    // and activity6; source gaps preserve the resulting FIFO, rather than resurrecting old8.
    check(a.pending_category != 4 && a.handoff != app::LifeHandoff::none,
          "category4 no longer stops at selection; handoff names an actual missing consumer");
    const auto retained_queue = a.control.queue;
    const auto actor_rounds = a.rounds;
    const auto pending = a.pending_category;
    const auto before_date = game.state().calendar;
    const auto work = build(game, 28, {7, 3});
    steps(game, 20);
    check(game.state().mode == app::Mode::normal && game.life_state()->rounds == actor_rounds &&
              game.life_state()->pending_category == pending &&
              game.life_state()->control.queue == retained_queue &&
              game.state().calendar != before_date &&
              game.state().facilities.at(work).remaining_ticks == 260,
          "actor handoff preserves selection while calendar and new construction continue");
    while (game.state().mode == app::Mode::normal)
        steps(game, 1);
    check(game.state().mode == app::Mode::research_boundary &&
              game.state().simulation_steps == 1456,
          "normal autonomous world still guards unresolved monthly effects at original boundary");
    const auto boundary = fingerprint(game);
    steps(game, 50, 2);
    check(fingerprint(game) == boundary, "monthly handoff does not invent effects or accrue drift");
    reconcile(game);
}
} // namespace
int main() {
    try {
        autonomous_lifecycle();
        qualification_and_speed();
        live_construction();
        construction_qualification();
        live_neighbour_fee();
        actor_handoff_and_month_boundary();
        std::cout << checks
                  << " checks passed: normal village life, shared construction and cash\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
