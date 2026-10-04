#include "dungeon_village_prototype/startup_world_routes.hpp"
#include "dungeon_village_reference/world_popularity.hpp"

#include <iostream>
#include <stdexcept>

using namespace dungeon_village_prototype;
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
    check(rules.facility_initial.size() == 85 && rules.facility_initial.at(24).shared_n == 0 &&
              rules.facility_initial.at(24).construction_limit == 1 &&
              rules.facility_initial.at(28).construction_limit == 280 &&
              rules.facility_initial.at(0).construction_limit == 0,
          "original sharedN and construction guard not generic defaults");
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
    const auto started = ref::prepare_world_actor_control(
        p.routes, id, [&](const auto &r, auto actor, const auto &op) {
            return prepare_startup_world_command_input(r, actor, op, f);
        });
    check(started.candidate.has_value() && started.candidate->state.world.actors.at(id).journey &&
              started.candidate->state.world.ai.battle.actors.at(id).control.state == 0 &&
              started.candidate->state.random.draws() > before,
          "actual first8 selects and routes autonomously from real initial map");
    const auto &r = started.candidate->state;
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
