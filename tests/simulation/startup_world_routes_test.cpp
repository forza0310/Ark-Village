#include "ark/simulation/actors/startup_world_routes.hpp"
#include "ark/simulation/village/rules/world_popularity.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
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
        const auto selected = prepare_startup_world_decision_input_for_state(routes, id, facts);
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
                  !prepare_startup_world_decision_input_for_state(routes, id, f),
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
    const auto selected = prepare_startup_world_decision_input_for_state(carrier, id, facts);
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
    for (const auto &command : {ref::LegacyActorControl{19, 6, 0, 10},
                               ref::LegacyActorControl{27, 0, 0}, ref::LegacyActorControl{28, 0},
                               ref::LegacyActorControl{29, 21, 0}, ref::LegacyActorControl{30, 1, 0}}) {
        auto routes = source;
        routes.world.ai.battle.actors.at(id).control.queue = {command};
        const auto full = prepare_startup_world_command_input(routes, id, command, facts);
        const auto selected = prepare_startup_world_command_input_for_command(routes, id, command, facts);
        check(full && selected && same_gear(full->equipment, selected->equipment) &&
                  selected->equipment.size() == 113 && routes.random.draws() == source.random.draws(),
              "all five shop growth/display/commit opcodes retain complete current equipment without drawing");
        ref::ShopWorldState shop{routes.world, routes.shop_humans, routes.shop_actors,
                                  routes.items, routes.popularity_queue};
        const auto expected = ref::prepare_world_shop_command(shop, id, full->equipment);
        const auto actual = ref::prepare_world_shop_command(shop, id, selected->equipment);
        check(expected.candidate && actual.candidate && expected.error == actual.error &&
                  expected.candidate->requests.size() == actual.candidate->requests.size() &&
                  expected.candidate->state.world.ai.growth.at(definition_id).definition.extra ==
                      actual.candidate->state.world.ai.growth.at(definition_id).definition.extra &&
                  expected.candidate->state.world.ai.growth.at(definition_id).definition.equipment ==
                      actual.candidate->state.world.ai.growth.at(definition_id).definition.equipment &&
                  expected.candidate->state.world.ai.battle.actors.at(id).control.queue ==
                      actual.candidate->state.world.ai.battle.actors.at(id).control.queue,
              "selected equipment reaches actual shop consumer with identical growth/equipment/FIFO effects");
    }
    for (const auto &command : {ref::LegacyActorControl{0, 350, 350},
                               ref::LegacyActorControl{1, 10, 0}, ref::LegacyActorControl{8, 0}}) {
        const auto full = prepare_startup_world_command_input(source, id, command, facts);
        const auto selected = prepare_startup_world_command_input_for_command(source, id, command, facts);
        check(full && selected && full->equipment.size() == 113 && selected->equipment.empty() &&
                  full->departure.has_value() == selected->departure.has_value(),
              "movement/wait/departure omit unused gear while retaining actual departure payload");
        auto missing = source;
        const auto &last = facts.rules->equipment.back().shop;
        missing.catalog.erase({last.kind, last.id});
        check(!prepare_startup_world_command_input(missing, id, command, facts) &&
                  !prepare_startup_world_command_input_for_command(missing, id, command, facts) &&
                  missing.random.draws() == source.random.draws(),
              "unused gear still validates the last equipment key before accepting a command");
    }
    auto exit_source = source;
    auto ordinary = std::find_if(exit_source.world.facilities.begin(), exit_source.world.facilities.end(),
                                 [](const auto &entry) { return entry.second.category == 1; });
    check(ordinary != exit_source.world.facilities.end(), "actual reset contains an ordinary shop exit fixture");
    const auto goal = ordinary->second.placement.anchor;
    exit_source.world.actors.at(id).binding = ref::ArrivalBinding{
        goal, ordinary->second.placement.instance_id, ordinary->second.placement.definition_id};
    exit_source.world.ai.contexts.at(id).cell = goal;
    const auto full_exit = prepare_startup_world_command_input(exit_source, id, {24}, facts);
    const auto selected_exit = prepare_startup_world_command_input_for_command(exit_source, id, {24}, facts);
    check(full_exit && selected_exit && full_exit->shop_exit && selected_exit->shop_exit &&
              selected_exit->equipment.empty() &&
              same_gear(full_exit->shop_exit->catalogue, selected_exit->shop_exit->catalogue) &&
              selected_exit->shop_exit->catalogue.size() == 113 &&
              full_exit->shop_exit->quality == selected_exit->shop_exit->quality &&
              full_exit->shop_exit->job_thresholds == selected_exit->shop_exit->job_thresholds,
          "real ordinary shop exit retains its full gear/quality/profession input independently of unused command gear");
    exit_source.world.ai.battle.actors.at(id).control.state = 0;
    const auto full_arrival = prepare_startup_world_decision_input(exit_source, id, facts);
    const auto selected_arrival = prepare_startup_world_decision_input_for_state(exit_source, id, facts);
    check(full_arrival && selected_arrival && full_arrival->shop_arrival && selected_arrival->shop_arrival &&
              same_gear(full_arrival->shop_arrival->catalogue, selected_arrival->shop_arrival->catalogue),
          "daily bound ordinary shop preserves complete gear at the actual arrival consumer");
    auto walking = source;
    walking.world.ai.battle.actors.at(id).control.state = 0;
    walking.world.actors.at(id).binding.reset();
    const auto full_walk = prepare_startup_world_decision_input(walking, id, facts);
    const auto selected_walk = prepare_startup_world_decision_input_for_state(walking, id, facts);
    check(full_walk && selected_walk && !full_walk->shop_arrival && !selected_walk->shop_arrival,
          "unbound daily walk does not prepare an equipment arrival consumer");
    const auto &last = facts.rules->equipment.back().shop;
    walking.catalog.erase({last.kind, last.id});
    check(!prepare_startup_world_decision_input(walking, id, facts) &&
              !prepare_startup_world_decision_input_for_state(walking, id, facts) &&
              walking.random.draws() == source.random.draws(),
          "unbound daily walk still refuses a missing equipment key although gear storage is unused");
}
void routing() {
    auto p = installed();
    auto f = facts(p);
    const ref::CharacterId id{1};
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
    const auto selected_idle =
        prepare_startup_world_decision_input_for_state(idle, id, duplicate_facts);
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
              !prepare_startup_world_decision_input_for_state(idle, id, missing_late) &&
              idle.random.draws() == before,
          "unused departure still validates late unique economic evidence without drawing or early "
          "success");
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
        routing();
        std::cout << "startup world route checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
