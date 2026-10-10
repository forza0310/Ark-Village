#include "../../src/simulation/actors/startup_world_route_facts_private.hpp"
#include "ark/simulation/actors/startup_world_routes.hpp"
#include "ark/simulation/village/rules/world_popularity.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
bool borrowed{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(std::string(borrowed ? "borrowed facts: " : "owning facts: ") +
                                 message);
}
StartupWorldRouteFactsView view(const StartupWorldRouteFacts &f) {
    StartupWorldRouteFactsView v{f.rules,
                                 f.surface,
                                 f.exits,
                                 f.human_homes,
                                 f.neighbourhood,
                                 f.actor_metadata,
                                 [&f](int id) -> const std::array<int, 4> * {
                                     const auto found = f.facility_improvements.find(id);
                                     return found == f.facility_improvements.end() ? nullptr
                                                                                   : &found->second;
                                 }};
    v.tasks = f.tasks;
    v.job_counts = f.job_counts;
    v.facing = f.facing;
    v.actor_visible = f.actor_visible;
    v.task_entry = f.task_entry;
    v.task_attempt = f.task_attempt;
    v.actor_box = f.actor_box;
    v.rescue_box = f.rescue_box;
    v.object_box = f.object_box;
    v.calendar = f.calendar;
    v.primary_expression_table = f.primary_expression_table;
    v.sound_projection = f.sound_projection;
    return v;
}
auto selected_decision(const ref::WorldActorRoutesState &r, ref::CharacterId id,
                       const StartupWorldRouteFacts &f) {
    return borrowed ? prepare_startup_world_decision_input_borrowed(r, id, view(f))
                    : prepare_startup_world_decision_input_for_state(r, id, f);
}
auto selected_command(const ref::WorldActorRoutesState &r, ref::CharacterId id,
                      const ref::LegacyActorControl &c, const StartupWorldRouteFacts &f) {
    return borrowed ? prepare_startup_world_command_input_borrowed(r, id, c, view(f))
                    : prepare_startup_world_command_input_for_command(r, id, c, f);
}
StartupWorldProjection installed() {
    StartupSession session;
    for (int n = 0; n < 420; ++n)
        check(session.update() == StartupError::none, "source startup advances to first visitor");
    const auto result = prepare_startup_world_projection(session.state(), ref::WorldRandomStream{});
    check(result.candidate.has_value(), "true source startup projection");
    return *result.candidate;
}
StartupWorldRouteFacts facts(const StartupWorldProjection &p) {
    StartupWorldRouteFacts f;
    f.rules = p.rules;
    f.surface = p.surface;
    f.human_homes = p.human_homes;
    f.neighbourhood = p.neighbourhood;
    f.actor_metadata = p.actor_metadata;
    f.calendar = p.calendar;
    f.job_counts = {};
    for (const auto &human : p.rules->humans)
        if (p.human_presence.at(human.identity) != 0)
            ++f.job_counts.at(p.rules->jobs.at(human.definition.current_profession).type);
    for (const auto &d : p.rules->facilities)
        f.facility_improvements.emplace(d.id, std::array<int, 4>{}); // 原o.I新数组。
    // 原h.f180f同时用于出生和活动5出口，发布时保留原数组序。
    for (const auto &exit : startup_evidence().spawn_points)
        f.exits.push_back(exit);
    return f;
}
void catalogue() {
    const auto &rules = startup_world_rules();
    const auto &s = rules.script_sources;
    const auto catalog =
        ref::parse_world_script_catalog(s.events, s.talks, s.news, s.event_messages);
    const auto rewards = ref::parse_world_popularity_rewards(s.popularity_rewards);
    check(catalog.catalog.has_value() && rewards.rewards.has_value(),
          "five fixed SHA source scripts compile into fully parsed catalog");
    check(catalog.catalog->events.size() == 200 && catalog.catalog->talks.size() == 182 &&
              catalog.catalog->news.size() == 28 && rewards.rewards->size() == 100,
          "full original script row counts preserved");
    check(ref::world_popularity_script_catalog(*catalog.catalog, *rewards.rewards).has_value(),
          "raw sources register real continuation programs");
    // 固定DEX c/n.c pc336调o.g；后者pc21写20、pc28仅对kind2写30。
    check(rules.facility_initial.size() == 85 && rules.facility_initial.at(24).shared_n == 20 &&
              rules.facility_initial.at(28).shared_n == 20 &&
              rules.facility_initial.at(66).shared_n == 30 &&
              rules.facility_initial.at(24).construction_limit == 1 &&
              rules.facility_initial.at(28).construction_limit == 280 &&
              rules.facility_initial.at(0).construction_limit == 0,
          "actual reset sharedN20/30 and construction guards follow source initialization, not "
          "allocation zeros");
}
void state_inputs(const ref::WorldActorRoutesState &source, ref::CharacterId id,
                  const StartupWorldRouteFacts &facts) {
    for (int state = 0; state <= 20; ++state) {
        auto routes = source;
        routes.world.ai.battle.actors.at(id).control.state = state;
        const auto full = prepare_startup_world_decision_input(routes, id, facts);
        const auto selected = selected_decision(routes, id, facts);
        check(full && selected, "both builders accept every state with complete original evidence");
        const bool daily = state == 0 || state == 5 || state == 8 || state == 9 || state == 11;
        check(
            selected->daily.path.has_value() == daily &&
                selected->monster_path.has_value() == (state == 17) &&
                selected->landing_departure.has_value() == (state == 20) &&
                selected->combat.has_value() == (state == 1) && full->daily.path &&
                full->monster_path && full->landing_departure && full->combat,
            "selected branch stores its real inputs while full builder preserves complete oracle");
        if (daily)
            check(selected->daily.path->facts.map.cells.size() == source.world.map.cells.size() &&
                      selected->daily.path->exits == full->daily.path->exits &&
                      selected->daily.path->definition_directions ==
                          full->daily.path->definition_directions &&
                      selected->daily.spawn_creation->year_index ==
                          full->daily.spawn_creation->year_index,
                  "daily path retains complete fresh map, source order and calendar");
        if (state == 20)
            check(selected->landing_departure->catalogue.definitions.size() == 85 &&
                      selected->landing_departure->catalogue.cell_definition_ids ==
                          full->landing_departure->catalogue.cell_definition_ids &&
                      selected->landing_departure->exits == full->landing_departure->exits,
                  "landing keeps complete departure rather than validation-only storage");
        const auto expected = ref::prepare_world_actor_decision(routes, *full);
        const auto actual = ref::prepare_world_actor_decision(routes, *selected);
        check(expected.error == actual.error &&
                  expected.candidate.has_value() == actual.candidate.has_value(),
              "selected input preserves actual state-route acceptance and rejection");
        if (actual.candidate) {
            const auto &a = *actual.candidate;
            const auto &e = *expected.candidate;
            const auto &actor = a.state.world.ai.battle.actors.at(id);
            const auto &oracle = e.state.world.ai.battle.actors.at(id);
            check(a.removed == e.removed && a.delete_requested == e.delete_requested &&
                      a.consumed_events == e.consumed_events &&
                      a.lifecycle_requests.size() == e.lifecycle_requests.size() &&
                      a.attack_requests.size() == e.attack_requests.size() &&
                      a.shop_requests.size() == e.shop_requests.size() &&
                      a.daily.has_value() == e.daily.has_value() &&
                      a.monster.has_value() == e.monster.has_value() &&
                      a.lifecycle.has_value() == e.lifecycle.has_value() &&
                      ref::world_control_detail::same_control(actor.control, oracle.control) &&
                      actor.position.x == oracle.position.x &&
                      actor.position.z == oracle.position.z &&
                      actor.position.height == oracle.position.height &&
                      a.state.world.map.cells.size() == e.state.world.map.cells.size(),
                  "selected builder preserves decision outputs and full audit branches");
            auto random = a.state.random;
            auto oracle_random = e.state.random;
            check(random.draws() == oracle_random.draws(),
                  "input selection preserves random cursor");
            for (int n = 0; n < 8; ++n) {
                const auto x = random.draw(1000), y = oracle_random.draw(1000);
                check(x.error == y.error && x.raw == y.raw && x.ticket == y.ticket,
                      "selected input preserves future random values");
            }
        }
    }
    // State2 does not use these payloads, but old unconditional validation is
    // still part of the input contract. Omitting storage cannot accept them.
    const auto rejected = [&](ref::WorldActorRoutesState routes, StartupWorldRouteFacts f) {
        routes.world.ai.battle.actors.at(id).control.state = 2;
        check(!prepare_startup_world_decision_input(routes, id, f) &&
                  !selected_decision(routes, id, f),
              "unused branch evidence retains the full builder rejection contract");
    };
    auto bad_facts = facts;
    bad_facts.surface.pop_back();
    rejected(source, bad_facts);
    bad_facts = facts;
    bad_facts.facility_improvements.erase(facts.rules->facilities.front().id);
    rejected(source, bad_facts);
    bad_facts = facts;
    bad_facts.actor_metadata.erase(id);
    rejected(source, bad_facts);
    auto bad_routes = source;
    const auto &equipment = facts.rules->equipment.front().shop;
    bad_routes.catalog.erase({equipment.kind, equipment.id});
    rejected(bad_routes, facts);
    bad_routes = source;
    bad_routes.world.facility_uses.at(facts.rules->facilities.front().id).level = 0;
    rejected(bad_routes, facts);
    bad_routes = source;
    bad_routes.shop_actors.erase(id);
    rejected(bad_routes, facts);
    bad_routes = source;
    bad_routes.world.ai.battle.actors.at(id).kind = ref::ActorKind::monster;
    bad_routes.world.ai.battle.actors.at(id).body = 4;
    rejected(bad_routes, facts);
    bad_routes = source;
    const auto binding = bad_routes.world.actors.at(id).binding;
    check(binding.has_value(), "state-input validation fixture retains its real first journey");
    bad_routes.world.ai.contexts.at(id).cell = binding->goal;
    bad_routes.world.facilities.erase(binding->instance_id.value);
    rejected(bad_routes, facts);
    auto no_ground = *facts.rules;
    for (auto &facility : no_ground.facilities)
        if (facility.kind == 7)
            facility.kind = 0;
    bad_facts = facts;
    bad_facts.rules = &no_ground;
    rejected(source, bad_facts);

    auto carrier = source;
    carrier.world.ai.battle.actors.at(id).control.state = 0;
    carrier.world.ai.battle.actors.at(id).object_slot = -2;
    const auto full = prepare_startup_world_decision_input(carrier, id, facts);
    const auto selected = selected_decision(carrier, id, facts);
    check(full && selected && full->rescue_direction_target && selected->rescue_direction_target,
          "daily rescue retains the direction provider used at actual recursive delivery");
    for (int direction = 0; direction < 4; ++direction)
        check(full->rescue_direction_target(id, direction) ==
                  selected->rescue_direction_target(id, direction),
              "selected rescue direction retains the same old-cell snapshot for every direction");
    check(!selected->rescue_direction_target({999}, 0),
          "selected rescue snapshot still rejects an absent actor identity");
}
void command_inputs(const ref::WorldActorRoutesState &source, ref::CharacterId id,
                    const StartupWorldRouteFacts &facts) {
    const auto definition_id = source.world.ai.battle.actors.at(id).definition;
    const auto same_gear = [](const auto &a, const auto &b) {
        if (a.size() != b.size())
            return false;
        for (std::size_t n = 0; n < a.size(); ++n)
            if (a[n].kind != b[n].kind || a[n].id != b[n].id || a[n].rank != b[n].rank ||
                a[n].type != b[n].type || a[n].unlocked != b[n].unlocked ||
                a[n].price != b[n].price || a[n].combat != b[n].combat)
                return false;
        return true;
    };
    for (const auto &command :
         {ref::LegacyActorControl{19, 6, 0, 10}, ref::LegacyActorControl{27, 0, 0},
          ref::LegacyActorControl{28, 0}, ref::LegacyActorControl{29, 21, 0},
          ref::LegacyActorControl{30, 1, 0}}) {
        auto routes = source;
        routes.world.ai.battle.actors.at(id).control.queue = {command};
        const auto full = prepare_startup_world_command_input(routes, id, command, facts);
        const auto selected = selected_command(routes, id, command, facts);
        check(full && selected && same_gear(full->equipment, selected->equipment) &&
                  selected->equipment.size() == 113 &&
                  routes.random.draws() == source.random.draws(),
              "all five shop growth/display/commit opcodes retain complete current equipment "
              "without drawing");
        ref::ShopWorldState shop{routes.world, routes.shop_humans, routes.shop_actors, routes.items,
                                 routes.popularity_queue};
        const auto expected = ref::prepare_world_shop_command(shop, id, full->equipment);
        const auto actual = ref::prepare_world_shop_command(shop, id, selected->equipment);
        check(
            expected.candidate && actual.candidate && expected.error == actual.error &&
                expected.candidate->requests.size() == actual.candidate->requests.size() &&
                expected.candidate->state.world.ai.growth.at(definition_id).definition.extra ==
                    actual.candidate->state.world.ai.growth.at(definition_id).definition.extra &&
                expected.candidate->state.world.ai.growth.at(definition_id).definition.equipment ==
                    actual.candidate->state.world.ai.growth.at(definition_id)
                        .definition.equipment &&
                expected.candidate->state.world.ai.battle.actors.at(id).control.queue ==
                    actual.candidate->state.world.ai.battle.actors.at(id).control.queue,
            "selected equipment reaches actual shop consumer with identical growth/equipment/FIFO "
            "effects");
    }
    for (const auto &command : {ref::LegacyActorControl{0, 350, 350},
                                ref::LegacyActorControl{1, 10, 0}, ref::LegacyActorControl{8, 0}}) {
        const auto full = prepare_startup_world_command_input(source, id, command, facts);
        const auto selected = selected_command(source, id, command, facts);
        check(full && selected && full->equipment.size() == 113 && selected->equipment.empty() &&
                  full->departure.has_value() == selected->departure.has_value(),
              "movement/wait/departure omit unused gear while retaining actual departure payload");
        auto missing = source;
        const auto &last = facts.rules->equipment.back().shop;
        missing.catalog.erase({last.kind, last.id});
        check(!prepare_startup_world_command_input(missing, id, command, facts) &&
                  !selected_command(missing, id, command, facts) &&
                  missing.random.draws() == source.random.draws(),
              "unused gear still validates the last equipment key before accepting a command");
    }
    auto exit_source = source;
    auto ordinary =
        std::find_if(exit_source.world.facilities.begin(), exit_source.world.facilities.end(),
                     [](const auto &entry) { return entry.second.category == 1; });
    check(ordinary != exit_source.world.facilities.end(),
          "actual reset contains an ordinary shop exit fixture");
    const auto goal = ordinary->second.placement.anchor;
    exit_source.world.actors.at(id).binding = ref::ArrivalBinding{
        goal, ordinary->second.placement.instance_id, ordinary->second.placement.definition_id};
    exit_source.world.ai.contexts.at(id).cell = goal;
    const auto full_exit = prepare_startup_world_command_input(exit_source, id, {24}, facts);
    const auto selected_exit = selected_command(exit_source, id, {24}, facts);
    check(full_exit && selected_exit && full_exit->shop_exit && selected_exit->shop_exit &&
              selected_exit->equipment.empty() &&
              same_gear(full_exit->shop_exit->catalogue, selected_exit->shop_exit->catalogue) &&
              selected_exit->shop_exit->catalogue.size() == 113 &&
              full_exit->shop_exit->quality == selected_exit->shop_exit->quality &&
              full_exit->shop_exit->job_thresholds == selected_exit->shop_exit->job_thresholds,
          "real ordinary shop exit retains its full gear/quality/profession input independently of "
          "unused command gear");
    exit_source.world.ai.battle.actors.at(id).control.state = 0;
    const auto full_arrival = prepare_startup_world_decision_input(exit_source, id, facts);
    const auto selected_arrival = selected_decision(exit_source, id, facts);
    check(full_arrival && selected_arrival && full_arrival->shop_arrival &&
              selected_arrival->shop_arrival &&
              same_gear(full_arrival->shop_arrival->catalogue,
                        selected_arrival->shop_arrival->catalogue),
          "daily bound ordinary shop preserves complete gear at the actual arrival consumer");
    auto walking = source;
    walking.world.ai.battle.actors.at(id).control.state = 0;
    walking.world.actors.at(id).binding.reset();
    const auto full_walk = prepare_startup_world_decision_input(walking, id, facts);
    const auto selected_walk = selected_decision(walking, id, facts);
    check(full_walk && selected_walk && !full_walk->shop_arrival && !selected_walk->shop_arrival,
          "unbound daily walk does not prepare an equipment arrival consumer");
    const auto &last = facts.rules->equipment.back().shop;
    walking.catalog.erase({last.kind, last.id});
    check(
        !prepare_startup_world_decision_input(walking, id, facts) &&
            !selected_decision(walking, id, facts) &&
            walking.random.draws() == source.random.draws(),
        "unbound daily walk still refuses a missing equipment key although gear storage is unused");
}
struct CallbackCopyProbe {
    bool *armed;
    int *copies;
    CallbackCopyProbe(bool &trap, int &count) : armed(&trap), copies(&count) {}
    CallbackCopyProbe(const CallbackCopyProbe &other) : armed(other.armed), copies(other.copies) {
        ++*copies;
        if (*armed)
            throw std::runtime_error("callback copy trap");
    }
    std::optional<ref::Position> operator()(ref::Position p) const { return p; }
    std::optional<ref::WorldEventEntryCandidate> operator()(const ref::AiRewardState &,
                                                            const ref::WorldMapFacts &,
                                                            ref::CharacterId,
                                                            const ref::WorldEventTask &) const {
        return {};
    }
};
void owning_callback_boundaries(const StartupWorldProjection &projection, ref::CharacterId id) {
    // 116867d rejects invalid actor/rules/empty command before callbacks are
    // copied. Its decision path never reads sound_projection; commands never
    // read task_attempt. A throwing copy makes those public boundaries observable.
    for (bool selective : {false, true}) {
        bool armed{};
        int copies{};
        auto f = facts(projection);
        f.sound_projection = CallbackCopyProbe(armed, copies);
        f.task_attempt = CallbackCopyProbe(armed, copies);
        const auto decision = [&](ref::CharacterId actor) {
            return selective
                       ? prepare_startup_world_decision_input_for_state(projection.routes, actor, f)
                       : prepare_startup_world_decision_input(projection.routes, actor, f);
        };
        const auto command = [&](ref::CharacterId actor, const ref::LegacyActorControl &op) {
            return selective ? prepare_startup_world_command_input_for_command(projection.routes,
                                                                               actor, op, f)
                             : prepare_startup_world_command_input(projection.routes, actor, op, f);
        };
        armed = true;
        const int before = copies;
        check(!decision({999}) && !command({999}, {8, 0}) && !command(id, {}),
              "owning public builders reject stale actor/empty command before copying callbacks");
        const auto *rules = f.rules;
        f.rules = nullptr;
        check(!decision(id) && !command(id, {8, 0}) && copies == before,
              "missing rules is ordinary early refusal even when callback copying would throw");
        f.rules = rules;
        f.task_attempt = {};
        check(decision(id).has_value() && copies == before,
              "valid decision does not observe an unused throwing sound callback");
        f.sound_projection = {};
        armed = false;
        f.task_attempt = CallbackCopyProbe(armed, copies);
        armed = true;
        const int command_before = copies;
        check(command(id, {8, 0}).has_value() && copies == command_before,
              "valid departure command does not observe an unused throwing task callback");
        // Used callback ownership is still real: preserve the existing exception
        // rather than masking it or returning a callable that borrows its source.
        f.task_attempt = {};
        armed = false;
        f.sound_projection = CallbackCopyProbe(armed, copies);
        armed = true;
        bool threw{};
        try {
            (void)command(id, {33});
        } catch (const std::runtime_error &) {
            threw = true;
        }
        check(threw, "valid sound command retains the observable callback-copy failure");
    }
}

void borrowed_lifetime(const StartupWorldProjection &projection, ref::CharacterId id) {
    std::optional<ref::WorldActorCommandInput> retained_departure, retained_sound;
    std::optional<ref::WorldActorDecisionInput> retained_rescue;
    ref::Position old_view{}, old_rescue{}, old_home{};
    int old_surface{};
    std::vector<ref::Position> old_exits;
    {
        auto f = facts(projection);
        auto routes = projection.routes;
        routes.world.ai.battle.actors.at(id).control.state = 0;
        routes.world.ai.battle.actors.at(id).object_slot = -2;
        const auto human = routes.world.ai.battle.actors.at(id).definition;
        f.human_homes.at(human) = {8, 4, 1, 0};
        f.actor_metadata.at(id).cached_view = {41, 53};
        f.sound_projection = [offset = ref::Position{3, 7}](ref::Position p) {
            return std::optional<ref::Position>{{p.x + offset.x, p.y + offset.y}};
        };
        retained_departure =
            prepare_startup_world_command_input_borrowed(routes, id, {8, 0}, view(f));
        retained_sound = prepare_startup_world_command_input_borrowed(routes, id, {33}, view(f));
        retained_rescue = prepare_startup_world_decision_input_borrowed(routes, id, view(f));
        check(retained_departure && retained_departure->departure && retained_sound &&
                  retained_sound->cached_view && retained_sound->sound_projection &&
                  retained_rescue && retained_rescue->rescue_direction_target,
              "borrowed preparation produces owned departure, sound and rescue observations");
        old_view = *retained_sound->cached_view;
        old_exits = f.exits;
        old_surface = f.surface.front().definition;
        old_home = retained_departure->departure->departure.home->cell;
        const auto rescue = retained_rescue->rescue_direction_target(id, 0);
        check(rescue.has_value(), "source rescue direction0 has a definite old-cell target");
        old_rescue = *rescue;
        f.exits = {{7, 3}};
        f.surface.front().definition = old_surface == 0 ? 1 : 0;
        f.human_homes.at(human)[0] = 9;
        f.actor_metadata.at(id).cached_view = {91, 103};
        f.sound_projection = [](ref::Position p) { return std::optional<ref::Position>{p}; };
        routes.world.ai.contexts.at(id).cell = {9, 4};
        const auto fresh =
            prepare_startup_world_command_input_borrowed(routes, id, {8, 0}, view(f));
        const auto fresh_sound =
            prepare_startup_world_command_input_borrowed(routes, id, {33}, view(f));
        const auto fresh_rescue =
            prepare_startup_world_decision_input_borrowed(routes, id, view(f));
        check(fresh && fresh->departure && fresh->departure->departure.exits == f.exits &&
                  fresh->departure->departure.catalogue.cell_definition_ids.front() ==
                      f.surface.front().definition &&
                  fresh->departure->departure.home->cell.x == 9 && fresh_sound &&
                  fresh_sound->cached_view == ref::Position{91, 103} &&
                  fresh_sound->sound_projection({1, 2}) == ref::Position{1, 2} && fresh_rescue &&
                  !(fresh_rescue->rescue_direction_target(id, 0) == old_rescue),
              "subsequent borrowed calls read changed facts, metadata, sound and actor cell "
              "without cache");
        // Destroy both backing facts and routes before exercising retained results.
    }
    check(
        retained_departure->departure->departure.exits == old_exits &&
            retained_departure->departure->departure.catalogue.cell_definition_ids.front() ==
                old_surface &&
            retained_departure->departure->departure.home->cell == old_home &&
            retained_sound->cached_view == old_view &&
            retained_sound->sound_projection({1, 2}) == ref::Position{4, 9} &&
            retained_rescue->rescue_direction_target(id, 0) == old_rescue &&
            !retained_rescue->rescue_direction_target({999}, 0),
        "returned inputs and callbacks outlive borrowed source storage with original observations");
}

void routing() {
    auto p = installed();
    auto f = facts(p);
    const ref::CharacterId id{1};
    if (borrowed)
        borrowed_lifetime(p, id);
    else
        owning_callback_boundaries(p, id);
    const auto before = p.routes.random.draws();
    const auto i = prepare_startup_world_command_input(p.routes, id, {8, 0}, f);
    check(i && i->departure && i->equipment.size() == 113 &&
              i->departure->departure.catalogue.definitions.size() == 85 &&
              i->departure->departure.catalogue.cell_definition_ids.size() == 576 &&
              i->departure->departure.exits == f.exits,
          "actual first8 projects full source catalog/map/exit order");
    check(p.routes.random.draws() == before && i->use_shared_random &&
              i->departure->departure.tickets.empty(),
          "provider borrows runtime stream lazily and never draws during read");
    // 显式重复ID夹具：源表本身唯一；优化仍必须保留旧首项经济／明细的查找语义。
    auto duplicate_rules = *f.rules;
    auto duplicate = duplicate_rules.facilities.front();
    duplicate.economy.attributes[2] = {987654, 987654};
    duplicate.detail = 987;
    duplicate.category = 876;
    duplicate_rules.facilities.push_back(duplicate);
    auto duplicate_facts = f;
    duplicate_facts.rules = &duplicate_rules;
    const auto repeated =
        prepare_startup_world_command_input(p.routes, id, {8, 0}, duplicate_facts);
    check(repeated && repeated->departure &&
              repeated->departure->departure.catalogue.definitions.size() == 86 &&
              repeated->departure->departure.catalogue.definitions.back().definition_charm ==
                  i->departure->departure.catalogue.definitions.front().definition_charm &&
              repeated->departure->departure.catalogue.definitions.back().legacy_category == 876 &&
              repeated->departure->departure.definition_details.at(duplicate.id) ==
                  duplicate_rules.facilities.front().detail &&
              p.routes.random.draws() == before,
          "duplicate ID keeps first economic definition/detail and current category without draw");
    auto idle = p.routes;
    idle.world.ai.battle.actors.at(id).control.state = 2;
    duplicate_rules.facilities.back().economy.construction_cost = -1;
    const auto full_idle = prepare_startup_world_decision_input(idle, id, duplicate_facts);
    const auto selected_idle = selected_decision(idle, id, duplicate_facts);
    check(full_idle && selected_idle && !selected_idle->landing_departure &&
              full_idle->landing_departure &&
              full_idle->landing_departure->definition_details.at(duplicate.id) ==
                  duplicate_rules.facilities.front().detail &&
              idle.random.draws() == before,
          "unused departure skips storage but still resolves invalid later duplicate through valid "
          "first definition");
    auto missing_late = duplicate_facts;
    missing_late.facility_improvements.erase(
        duplicate_rules.facilities[duplicate_rules.facilities.size() - 2].id);
    check(!prepare_startup_world_decision_input(idle, id, missing_late) &&
              !selected_decision(idle, id, missing_late) && idle.random.draws() == before,
          "unused departure still validates late unique economic evidence without drawing or early "
          "success");
    // IDs outside the bounded first-definition index retain the old fallback;
    // these catalogue-only fixtures are not inserted into the game world/map.
    for (const int extra_id : {-7, 10000}) {
        auto extra_rules = *f.rules;
        auto first = extra_rules.facilities.front();
        first.id = extra_id;
        first.detail = 37;
        first.economy.attributes[2] = {17, 17};
        auto second = first;
        second.detail = 93;
        second.category = 876;
        second.economy.construction_cost = -1;
        second.economy.attributes[2] = {31, 31};
        extra_rules.facilities.push_back(first);
        extra_rules.facilities.push_back(second);
        auto extra_facts = f;
        extra_facts.rules = &extra_rules;
        extra_facts.facility_improvements.emplace(extra_id, std::array<int, 4>{});
        auto extra_routes = p.routes;
        extra_routes.world.facility_uses.emplace(extra_id,
                                                 p.routes.world.facility_uses.begin()->second);
        const auto full =
            prepare_startup_world_command_input(extra_routes, id, {8, 0}, extra_facts);
        const auto selected = selected_command(extra_routes, id, {8, 0}, extra_facts);
        check(full && selected && full->departure && selected->departure,
              "negative/large duplicate definition IDs preserve successful fallback preparation");
        for (const auto *input : {&*full, &*selected}) {
            const auto &d = input->departure->departure;
            const auto &list = d.catalogue.definitions;
            check(list.size() == 87 && list[85].definition_charm == 17 &&
                      list[86].definition_charm == 17 && list[86].legacy_category == 876 &&
                      d.definition_details.at(extra_id) == 37,
                  "out-of-range duplicate uses first valid economy/detail and later row category");
        }
        extra_facts.facility_improvements.erase(extra_id);
        check(!prepare_startup_world_command_input(extra_routes, id, {8, 0}, extra_facts) &&
                  !selected_command(extra_routes, id, {8, 0}, extra_facts) &&
                  extra_routes.random.draws() == before,
              "out-of-range fallback still rejects absent first economic mapping without drawing");
    }
    const auto started = ref::prepare_world_actor_control(
        p.routes, id, [&](const auto &r, auto actor, const auto &op) {
            return prepare_startup_world_command_input(r, actor, op, f);
        });
    check(started.candidate.has_value() && started.candidate->state.world.actors.at(id).journey &&
              started.candidate->state.world.ai.battle.actors.at(id).control.state == 0 &&
              started.candidate->state.random.draws() > before,
          "actual first8 selects and routes autonomously from real initial map");
    const auto &r = started.candidate->state;
    state_inputs(r, id, f);
    command_inputs(r, id, f);
    const auto decision = prepare_startup_world_decision_input(r, id, f);
    check(decision && decision->daily.path && decision->daily.spawn_creation &&
              decision->daily.spawn_creation->year_index == 0 &&
              decision->daily.spawn_creation->month_index == 3 && decision->combat &&
              decision->combat->profession_role == 3 && decision->combat->weapon.range == 130,
          "daily/path/currentjob combat inputs derive from actual first character");
    const auto binding = r.world.actors.at(id).binding;
    check(binding.has_value(), "real first journey has source facility binding");
    const auto &facility = r.world.facilities.at(binding->instance_id.value);
    auto current = r;
    current.world.ai.contexts.at(id).cell = binding->goal;
    current.world.ai.battle.actors.at(id).control.queue = {{24}};
    const auto exit = prepare_startup_world_command_input(current, id, {24}, f);
    check(exit.has_value() &&
              (facility.category != 1 ||
               (exit->shop_exit && exit->shop_exit->job_thresholds == std::array<int, 2>{10, 100})),
          "source exit resolves fresh quality/currentad threshold without drawing");
    const auto dynamic = current.catalog.find({1, 2});
    check(dynamic != current.catalog.end(), "complete weapon namespace available");
    current.catalog.at({1, 2}).status = 1;
    const auto updated = prepare_startup_world_command_input(current, id, {19, 2}, f);
    check(updated && updated->equipment.at(2).unlocked,
          "unlock follows current owner status, not immutable source reset");
    auto bad = f;
    bad.facility_improvements.erase(0);
    check(!prepare_startup_world_command_input(current, id, {8, 0}, bad),
          "missing shared improvement evidence rejects even before selection");
    bad = f;
    bad.surface.pop_back();
    check(!prepare_startup_world_decision_input(current, id, bad),
          "incomplete map rejects instead of introducing fallback cells");
    check(!prepare_startup_world_command_input(current, {999}, {8, 0}, f),
          "stale actor cannot receive a valid provider input");
    // 明确退休目标夹具：原版O保留旧目标，但恢复后地图格不再绑定该实例。
    auto restored = current;
    restored.world.facilities.erase(binding->instance_id.value);
    for (auto &cell : restored.world.map.cells)
        if (cell.facility && cell.facility->instance_id == binding->instance_id) {
            cell.facility.reset();
            cell.category = ref::RouteCategory::ground;
        }
    check(prepare_startup_world_decision_input(restored, id, f).has_value() &&
              prepare_startup_world_command_input(restored, id, {24}, f).has_value(),
          "restored map invalidates old O identity normally; providers do not reject legal "
          "retirement");
}
} // namespace
int main() {
    try {
        catalogue();
        for (bool use_borrowed : {false, true}) {
            borrowed = use_borrowed;
            routing();
        }
        std::cout << "startup world route checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
