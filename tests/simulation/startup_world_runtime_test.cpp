#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
void main_character_profile() {
    StartupSession startup;
    StartupWorldRuntimeSession session(startup.state(),
                                       ref::WorldRandomStream::from_java_seed(12345));
    auto s = session.state();
    const auto base0 = startup_world_human_profile(s, 0);
    const auto base1 = startup_world_human_profile(s, 1);
    check(base0 && base1 && s.human_profiles.empty(),
          "default world resolves immutable profiles without override");
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto presence = s.human_presence;
    check(install_startup_world_main_character(s, {"研究姓名", 1, true}),
          "profile installs only actual fresh private world candidate");
    const auto value = startup_world_human_profile(s, 0);
    const auto other = startup_world_human_profile(s, 1);
    const auto details = startup_world_human_details(s, 0);
    const auto portrait = startup_world_portrait(s, 0);
    const int job = s.scene.world.world.ai.growth.at(0).definition.current_profession;
    check(value && value->name == "研究姓名" && value->sex == 1 && value->custom_name && details &&
              details->name == value->name && details->sex == 1 && portrait &&
              portrait->image == s.rules->jobs.at(job).sprites[1] &&
              startup_world_runtime_scripts(s).humans.at(0).name == "研究姓名",
          "definition zero name/sex agree in details portrait and dialogue projection without "
          "instance");
    check(other && other->name == base1->name && other->sex == base1->sex &&
              s.rules->humans.at(0).name == base0->name &&
              s.rules->humans.at(0).sex == base0->sex &&
              s.scene.world.world.ai.battle.actors.empty() && s.scene.random.draws() == 0 &&
              s.scene.world.world.ai.accounting.funds() == cash && s.human_presence == presence,
          "profile preserves definition1 frozen table real arrival ordering funds and random");
    auto directory = s; // 页61与开放条件夹具；没有到访、资金或属性注入。
    const auto male = std::find_if(s.rules->jobs.begin(), s.rules->jobs.end(),
                                   [](const auto &j) { return j.script_extra == 0; });
    const auto female = std::find_if(s.rules->jobs.begin(), s.rules->jobs.end(),
                                     [](const auto &j) { return j.script_extra == 1; });
    check(male != s.rules->jobs.end() && female != s.rules->jobs.end(),
          "fixed profession catalogue has both sex restrictions");
    const int male_id = static_cast<int>(male - s.rules->jobs.begin());
    const int female_id = static_cast<int>(female - s.rules->jobs.begin());
    directory.scripts.professions.at(male_id).status = 1;
    directory.scripts.professions.at(female_id).status = 1;
    ref::WorldScriptPage page;
    page.id = directory.scripts.next_page_id++;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 61;
    directory.scripts.pages.front().lifecycle = 3;
    directory.scripts.pages.push_back(page);
    directory.page_human_bindings[page.id] = 0;
    check(initialize_startup_world_human_pages(directory),
          "definition0 profile catalogue real initializer");
    const auto &list = directory.human_page_catalogs.at(page.id);
    check(std::find(list.begin(), list.end(), female_id) != list.end() &&
              std::find(list.begin(), list.end(), male_id) == list.end() &&
              directory.scene.world.world.ai.battle.actors.empty(),
          "female main profile uses actual profession filter without fabricating arrival");
    check(!install_startup_world_main_character(s, {"重复", 0, true}) &&
              startup_world_human_profile(s, 0)->name == "研究姓名",
          "duplicate profile installation refuses without overwriting private candidate");
    auto invalid = session.state();
    check(!install_startup_world_main_character(invalid, {"非法", 2, false}) &&
              invalid.human_profiles.empty() && invalid.scripts.humans.at(0).name == base0->name,
          "invalid sex refuses before any shared name mutation");
    invalid.simulation_steps = 1; // 运行中世界资格边界夹具，不推进/注入原游戏日期。
    check(!install_startup_world_main_character(invalid, {"运行中", 0, true}) &&
              invalid.human_profiles.empty(),
          "running world cannot receive title editing");
    for (const auto &name :
         {std::string("\xc0\xaf", 2), std::string("a\0b", 3), std::string("\xed\xa0\x80", 3),
          std::string("\n"), std::string(4097, 'x')})
        check(!valid_startup_world_human_profile({name, 0, true}),
              "malformed UTF8/control/budget profile rejects");
    check(valid_startup_world_human_profile({"", 0, true}) &&
              valid_startup_world_human_profile({"A中\xf0\x9f\x8c\x9f", 1, true}),
          "empty original-unclosed input and valid multibyte profile remain distinct from "
          "malformed text");
    invalid = s;
    invalid.human_profiles.emplace(1, StartupWorldHumanProfile{"越权", 0, true});
    check(!valid_startup_world_human_profiles(invalid) &&
              !startup_world_human_profile(invalid, 0) &&
              !startup_world_human_details(invalid, 0) && !startup_world_portrait(invalid, 0),
          "unsupported stable definition override explicitly rejects all profile consumers");
}
void initial_owner() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(),
                                       ref::WorldRandomStream::from_java_seed(12345));
    const auto &s = runtime.state();
    check(s.rules && s.scene.world.world.ai.battle.actors.empty() &&
              s.scene.world.facility_order.size() == 8,
          "actual empty startup owns one world without fabricated actor");
    check(s.scene.world.world.ai.accounting.funds() == 5000 && s.village_points == 10 &&
              s.popularity == 50 && s.scene.calendar.month == 3 && s.arrival_counter == 420,
          "actual cash, points, popularity, calendar and arrival fields");
    check(!s.scripts.finance && s.scripts.human_order.empty() &&
              s.scripts.popularity_queue.empty() && s.scripts.pending_completion == 0 &&
              s.scene.random.draws() == 0,
          "persistent script metadata is not a second cash, I, actor or random owner");
    check(s.human_calendar.size() == 25 && s.facility_definitions.size() == 85 &&
              s.scripts.professions.size() == 23 && s.shop_item_stock.size() == 36 &&
              s.task_progress.definitions.size() == 81 && s.yearly_statistics.size() == 30 &&
              s.base_variants.size() == 576,
          "full original definitions and zero-allocated calendar arrays present");
    // 独立原新局oracle：n.c/3355–3362对这五条flags1调用a.i.a，保留首次NEW。
    const std::set<int> known_recipes{14, 22, 24, 30, 31};
    check(s.magic_pot_recipes.size() == 40 && s.legacy_n[11] == 1 && s.magic_pot_comment.empty() &&
              s.magic_pot_output == std::array<std::int32_t, 4>{},
          "actual new game installs forty recipes and source pot level/display initial values");
    for (const auto &[id, progress] : s.magic_pot_recipes)
        check(progress.identity == id && progress.status == (known_recipes.count(id) ? 1 : 0) &&
                  progress.pending_notice == (known_recipes.count(id) != 0),
              "source new game recipe bit1 grants known progress without clearing its NEW flag");
    const auto script = startup_world_runtime_scripts(s);
    check(script.finance && script.finance->cash == s.scene.world.world.ai.accounting.funds() &&
              script.finance->legacy_flags14 == 0 &&
              script.finance->localized_gold_template == "G" && s.cash_peak == 0 &&
              s.cash_peak_village == "没有记录",
          "call-point finance reads current ledger and fresh system source defaults");
    const auto route = startup_world_runtime_routes(s);
    check(route.world.ai.growth.size() == 25 && route.world.facility_uses.size() == 85 &&
              route.world.map.cells.size() == 576 && route.random.draws() == 0,
          "routes are disposable canonical projections, not stored second worlds");
    const auto adapter = startup_world_runtime_adapter();
    check(adapter.scene && adapter.scripts && adapter.report && adapter.maintenance &&
              adapter.tasks && adapter.factory && adapter.entry && adapter.arrival &&
              adapter.arrival_private && adapter.calendar_other && adapter.calendar_request &&
              adapter.actors.owned_command,
          "actual typed world, calendar, arrival and current-FIFO providers registered");
}
void pause_and_private_failure() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(),
                                       ref::WorldRandomStream::from_java_seed(12345));
    runtime.set_paused(true);
    const auto paused = runtime.update();
    check(paused.error == StartupWorldRuntimeError::none &&
              runtime.state().arrival_counter == 420 && runtime.state().scene.calendar.units == 0 &&
              runtime.state().scene.random.draws() == 0 && runtime.state().simulation_steps == 0 &&
              runtime.checkpoints().empty(),
          "framework pause consumes neither world/date/random nor memory checkpoints");
    runtime.set_paused(false);
    auto impossible = runtime.state();
    impossible.scene.scene_state = 2; // 显式focus夹具走共同世界，避开未见自动脚本7的L159。
    impossible.scene.world.map_flags.pop_back();
    const auto rejected = prepare_startup_world_runtime(impossible);
    check(rejected.error != StartupWorldRuntimeError::none && !rejected.candidate &&
              rejected.checkpoints.empty() && impossible.arrival_counter == 420 &&
              impossible.scene.random.draws() == 0 && impossible.scene.calendar.units == 0,
          "late invalid canonical fact gives no partially committed actor/date/random/checkpoint");
}
void synchronous_event_seen() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(), ref::WorldRandomStream::from_java_seed(1));
    auto s = runtime.state();
    const auto executed = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                    startup_world_runtime_scripts(s), {90, {}, {}});
    check(executed.candidate && write_startup_world_runtime_scripts(s, executed.candidate->state) &&
              s.scripts.event_calls.at(90) == 1 && s.scene.world.world.ai.battle.events.count(90),
          "real event90 synchronizes aM-derived battle seen before same-call daily8 guard");
    auto projected = startup_world_runtime_finish(s);
    projected.event_calls[90] = 0;
    projected.event_calls[116] = 2;
    check(write_startup_world_runtime_finish(s, projected) &&
              !s.scene.world.world.ai.battle.events.count(90) &&
              s.scene.world.world.ai.battle.events.count(116) && s.scripts.event_calls.at(116) == 2,
          "finish writer regenerates seen from counts without a second persistent event owner");
}
void facing_snapshot_boundaries(const StartupWorldRuntimeState &baseline,
                                ref::CharacterId live_actor) {
    const auto adapter = startup_world_runtime_adapter().actors;
    const auto providers = [&](const StartupWorldRuntimeState &owner) {
        const auto routes = adapter.read_current_routes(owner);
        const auto full = adapter.decision(owner, live_actor);
        const auto reused = adapter.decision_from_routes(owner, routes, live_actor);
        const auto attack = adapter.owned_command(owner, routes, live_actor, {14});
        check(full && full->combat && full->combat->facing_for && reused && reused->combat &&
                  reused->combat->facing_for && attack && attack->attack &&
                  attack->attack->facing_for,
              "all three production input paths retain an owned facing callback");
        return std::array<ref::WorldCombatFacingConsumer, 3>{
            full->combat->facing_for, reused->combat->facing_for, attack->attack->facing_for};
    };
    std::array<ref::WorldCombatFacingConsumer, 3> retained;
    const ref::CharacterId historical_self{800}, target{1000}, added{900};
    {
        auto owner = baseline;
        owner.scene.world.world.ai.battle.actors.at(live_actor).control.state = 1;
        owner.actor_metadata[historical_self] = owner.actor_metadata.at(live_actor);
        owner.actor_metadata[target] = owner.actor_metadata.at(live_actor);
        owner.actor_metadata[historical_self].cached_view = {10, 20};
        check(!owner.scene.world.world.ai.battle.actors.count(historical_self) &&
                  !owner.scene.world.world.ai.battle.actors.count(target),
              "facing fixture uses historical metadata-only IDs without inventing live actors");
        struct Case {
            ref::Position target;
            int direction;
        };
        const std::array<Case, 9> cases{{{{11, 21}, 0},
                                         {{11, 19}, 1},
                                         {{9, 19}, 2},
                                         {{9, 21}, 3},
                                         {{10, 21}, 3},
                                         {{10, 19}, 2},
                                         {{11, 20}, 1},
                                         {{9, 20}, 2},
                                         {{10, 20}, 2}}};
        for (const auto &test : cases) {
            owner.actor_metadata.at(target).cached_view = test.target;
            const auto snapshot = providers(owner);
            for (const auto &facing : snapshot) {
                check(facing(historical_self, target) == test.direction,
                      "metadata-only facing preserves four quadrants and equality boundaries");
                check(!facing({0}, target) && !facing(historical_self, {2000}) &&
                          !facing(added, target) && !facing(historical_self, added),
                      "missing first/last and interior keys cannot alias lower_bound neighbours");
            }
        }
        owner.actor_metadata.at(target).cached_view = {11, 21};
        retained = providers(owner);
        owner.actor_metadata.at(historical_self).cached_view = {100, 100};
        owner.actor_metadata.at(target).cached_view = {90, 90};
        owner.actor_metadata[added] = owner.actor_metadata.at(target);
        const auto changed = providers(owner);
        for (std::size_t n = 0; n < retained.size(); ++n)
            check(
                retained[n](historical_self, target) == 0 && !retained[n](historical_self, added) &&
                    changed[n](historical_self, target) == 2 &&
                    changed[n](historical_self, added) == 2,
                "old facing keeps prior positions and absent keys while a new call reads changes");
        owner.actor_metadata.erase(historical_self);
        owner.actor_metadata.erase(target);
        const auto deleted = providers(owner);
        for (std::size_t n = 0; n < retained.size(); ++n)
            check(retained[n](historical_self, target) == 0 &&
                      !deleted[n](historical_self, added) && !deleted[n](added, target),
                  "deleting historical IDs affects only new callbacks and preserves old snapshots");
    }
    for (const auto &facing : retained)
        check(facing(historical_self, target) == 0 && !facing(historical_self, added),
              "facing callback owns its original key/xy snapshot beyond Owner lifetime");
}

void route_context_map_boundaries(const StartupWorldRuntimeState &baseline,
                                  ref::CharacterId live_actor) {
    const auto adapter = startup_world_runtime_adapter().actors;
    auto source = baseline;
    // Historical route contexts remain legitimate projection data even when no
    // live actor references them. They must not be filtered to the active roster.
    for (std::uint64_t key : {10, 20, 30, 40, 50}) {
        ref::RescueActorContext context;
        context.binding = ref::ArrivalBinding{{2, 3}, {77}, 28};
        context.destination = ref::Position{2, 3};
        context.journey = ref::FacilityDeparture{};
        context.journey->binding = *context.binding;
        context.journey->route.steps = {{1, 2}, {2, 3}};
        context.journey->legacy_direction = 2;
        context.path_pending = true;
        context.waypoint = 1;
        context.town_updates = static_cast<int>(key);
        context.outside_updates = 7;
        source.scene.world.world.actors.emplace(ref::CharacterId{key}, context);
        source.actor_metadata.emplace(ref::CharacterId{key}, source.actor_metadata.at(live_actor));
    }
    const auto original = startup_world_state_digest(source);
    auto routes = adapter.read_current_routes(source);
    auto projection_oracle = source;
    projection_oracle.scene.world.world = routes.world;
    check(startup_world_state_digest(projection_oracle) == original,
          "fresh route projection preserves the complete world and all historical nested contexts");
    for (std::uint64_t key : {10, 30, 50})
        routes.world.actors.erase({key});
    for (std::uint64_t key : {5, 25, 60}) {
        auto context = routes.world.actors.at({20});
        context.town_updates = static_cast<int>(key);
        context.journey->route.steps.push_back({4, 5});
        routes.world.actors.emplace(ref::CharacterId{key}, std::move(context));
    }
    auto &changed = routes.world.actors.at({20});
    changed.binding.reset();
    changed.journey.reset();
    changed.destination = ref::Position{6, 7};
    changed.unbound_route = ref::LegacyPathResult{};
    changed.unbound_route->steps = {{4, 5}, {6, 7}};
    changed.waypoint = 0;
    changed.on_event_cell = true;
    changed.definition_task_flag = true;
    changed.horizontal_velocity = {3.5F, -2.5F};
    changed.blocked_updates = 3;
    changed.spawn_updates = 4;
    changed.no_path_updates = 5;
    changed.short_exit_updates = 6;
    changed.bad_area_updates = 7;
    changed.monster_mode = 4;
    routes.world.actors.at({40}).journey->route.steps = {{7, 8}};
    routes.world.actors.at({40}).journey->legacy_direction.reset();
    auto published = source;
    check(adapter.write_current_routes(published, routes),
          "route publication accepts inserted/deleted historical keys and complete changed values");
    auto expected = published;
    expected.scene.world.world =
        routes.world; // Complete member-assignment oracle, no merge helper.
    check(startup_world_state_digest(published) == startup_world_state_digest(expected) &&
              startup_world_state_digest(source) == original &&
              published.scene.world.world.actors.size() == source.scene.world.world.actors.size() &&
              !published.scene.world.world.actors.count({10}) &&
              !published.scene.world.world.actors.count({30}) &&
              !published.scene.world.world.actors.count({50}) &&
              published.scene.world.world.actors.count({5}) &&
              published.scene.world.world.actors.count({25}) &&
              published.scene.world.world.actors.count({60}),
          "publication matches full-world assignment with exact leading/interior/trailing key "
          "changes");
    const auto committed = startup_world_state_digest(published);
    routes.world.actors.at({20}).unbound_route->steps.clear();
    routes.world.actors.at({40}).journey->route.steps.clear();
    source.scene.world.world.actors.at({20}).journey->route.steps.clear();
    source.scene.world.world.actors.erase({40});
    check(startup_world_state_digest(published) == committed,
          "published complete contexts do not alias later route or source nested changes");
    auto retained = adapter.read_current_routes(published);
    published.scene.world.world.actors.at({20}).unbound_route->steps.clear();
    published.scene.world.world.actors.erase({60});
    auto retained_oracle = expected;
    retained_oracle.scene.world.world = retained.world;
    check(startup_world_state_digest(retained_oracle) == committed &&
              retained.random.draws() == baseline.scene.random.draws() &&
              retained.world.ai.accounting.funds() ==
                  baseline.scene.world.world.ai.accounting.funds(),
          "returned projection retains full independent nested contexts, cash and random after "
          "Owner edits");
}

// 只比较本批完整路线适配边界；源初访、模板事务与规则计算各有主责套件。
void current_route_adapter_boundaries() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(),
                                       ref::WorldRandomStream::from_raw({149, 0, 1}));
    const auto production = startup_world_runtime_adapter();
    auto source = runtime.state();
    for (int n = 0; n < 420; ++n) {
        auto private_arrival = source;
        auto arrived = production.arrival(source);
        check(arrived.has_value(),
              "current-route boundary fixture uses actual first arrival calls");
        check(
            production.arrival_private(private_arrival) &&
                startup_world_state_digest(private_arrival) == startup_world_state_digest(*arrived),
            "private arrival preserves every full Owner field and random at the real first visit");
        source = std::move(*arrived);
    }
    const auto id = source.scene.world.world.ai.human_order.front();
    facing_snapshot_boundaries(source, id);
    route_context_map_boundaries(source, id);
    const int definition = source.scene.world.world.ai.battle.actors.at(id).definition;
    const auto &a = production.actors;
    check(a.read_current_routes && a.write_current_routes && a.decision_from_routes,
          "production supplies explicit complete-current route and reused-decision hooks");
    const auto legacy_read = [&](const StartupWorldRuntimeState &s) {
        auto routes = a.read_routes(s);
        routes.world = s.scene.world.world;
        routes.facts = ref::world_schedule_facts(s.scene.world);
        routes.popularity_queue = s.scene.world.popularity_queue;
        return routes;
    };
    const auto legacy_publish = [&](StartupWorldRuntimeState &s,
                                    const ref::WorldActorRoutesState &r) {
        if (!a.write_routes(s, r))
            return false;
        auto &common = a.write_common(s);
        common.world = r.world;
        common.surface = r.facts.surface;
        common.map_flags = r.facts.flags;
        common.town = r.facts.town;
        common.popularity_queue = r.popularity_queue;
        return true;
    };
    for (const bool flag : {false, true}) {
        auto stale = source;
        stale.human_flags.at(definition) = flag ? 2U : 0U;
        stale.scene.world.world.actors.at(id).definition_task_flag = !flag;
        const auto before = startup_world_state_digest(stale);
        const auto old_routes = legacy_read(stale);
        const auto current_routes = a.read_current_routes(stale);
        const auto projected_routes = a.read_routes(stale);
        check(old_routes.world.actors.at(id).definition_task_flag == !flag &&
                  current_routes.world.actors.at(id).definition_task_flag == !flag &&
                  projected_routes.world.actors.at(id).definition_task_flag == flag,
              "initial/event retain raw task flag while presentation/encounter retain fresh "
              "projected flag");
        auto old_owner = stale;
        auto current_owner = stale;
        check(legacy_publish(old_owner, old_routes) &&
                  a.write_current_routes(current_owner, current_routes) &&
                  startup_world_state_digest(old_owner) ==
                      startup_world_state_digest(current_owner) &&
                  startup_world_state_digest(stale) == before,
              "full current read/write matches old common overwrite with stale flags and "
              "independent input");
        const auto old_input = a.decision(stale, id);
        const auto current_input = a.decision_from_routes(stale, current_routes, id);
        check(old_input.has_value() == current_input.has_value() && old_input &&
                  old_input->actor == current_input->actor &&
                  old_input->use_shared_random == current_input->use_shared_random &&
                  startup_world_state_digest(stale) == before,
              "decision reuses current route or fresh task-flag fallback without changing original "
              "owner");
        const auto old_decision = ref::prepare_world_actor_decision(old_routes, *old_input);
        const auto current_decision =
            ref::prepare_world_actor_decision(current_routes, *current_input);
        check(old_decision.error == current_decision.error &&
                  old_decision.candidate.has_value() == current_decision.candidate.has_value(),
              "stale task flag yields identical full decision acceptance at production boundary");
        if (old_decision.candidate) {
            old_owner = stale;
            current_owner = stale;
            check(legacy_publish(old_owner, old_decision.candidate->state) &&
                      a.write_current_routes(current_owner, current_decision.candidate->state) &&
                      startup_world_state_digest(old_owner) ==
                          startup_world_state_digest(current_owner),
                  "complete decision output preserves all owner fields and future random with "
                  "stale flags");
        }
    }
    auto missing_flag = source;
    missing_flag.human_flags.erase(definition);
    const auto missing_before = startup_world_state_digest(missing_flag);
    int rejected_reads{};
    for (const bool current : {false, true}) {
        try {
            (void)(current ? a.read_current_routes(missing_flag) : legacy_read(missing_flag));
        } catch (const std::invalid_argument &) {
            ++rejected_reads;
        }
    }
    check(rejected_reads == 2 && startup_world_state_digest(missing_flag) == missing_before,
          "old and current reads reject missing authoritative human flag key without partial owner "
          "mutation");
    int rejected_decisions{};
    const auto valid_routes = legacy_read(source);
    for (const bool current : {false, true}) {
        try {
            (void)(current ? a.decision_from_routes(missing_flag, valid_routes, id)
                           : a.decision(missing_flag, id));
        } catch (const std::invalid_argument &) {
            ++rejected_decisions;
        }
    }
    check(rejected_decisions == 2 && startup_world_state_digest(missing_flag) == missing_before,
          "reused decision retains old missing-key rejection even with an otherwise complete "
          "cached projection");
    auto missing_monster = source;
    const ref::CharacterId monster{900}; // 显式不完整引用夹具，不冒充真实生成入口。
    auto actor = missing_monster.scene.world.world.ai.battle.actors.at(id);
    actor.id = monster;
    actor.kind = ref::ActorKind::monster;
    actor.definition = 0;
    missing_monster.scene.world.world.ai.battle.actors.emplace(monster, actor);
    missing_monster.scene.world.world.ai.monster_order.push_back(monster);
    missing_monster.scene.world.world.ai.contexts.emplace(
        monster, missing_monster.scene.world.world.ai.contexts.at(id));
    const auto broken_before = startup_world_state_digest(missing_monster);
    const auto routes = legacy_read(missing_monster);
    auto old_owner = missing_monster;
    auto current_owner = missing_monster;
    check(legacy_publish(old_owner, routes) && a.write_current_routes(current_owner, routes) &&
              !old_owner.scene.world.world.actors.count(monster) &&
              !current_owner.scene.world.world.actors.count(monster) &&
              old_owner.actor_metadata.count(monster) &&
              current_owner.actor_metadata.count(monster) &&
              startup_world_state_digest(old_owner) == startup_world_state_digest(current_owner),
          "current writer preserves old monster metadata synchronization while discarding "
          "synthesized route context");
    const auto old_rejected = ref::prepare_world_schedule(old_owner.scene.world, {}, {});
    const auto current_rejected = ref::prepare_world_schedule(current_owner.scene.world, {}, {});
    check(old_rejected.error == ref::WorldScheduleError::invalid_owner &&
              current_rejected.error == old_rejected.error && !old_rejected.candidate &&
              !current_rejected.candidate &&
              startup_world_state_digest(missing_monster) == broken_before,
          "missing monster context remains invalid after either writer and original input stays "
          "independent");
}
void successful_result_retains_independent_owner() {
    StartupSession reset;
    StartupWorldRuntimeSession runtime(reset.state(), ref::WorldRandomStream::from_java_seed(1));
    auto result = runtime.update();
    check(result.error == StartupWorldRuntimeError::none && result.candidate,
          "successful session update returns its complete candidate after commit");
    auto &candidate = *result.candidate;
    const auto &committed = runtime.state();
    check(candidate.catalog.size() == 149 && candidate.catalog.size() == committed.catalog.size() &&
              candidate.scripts.pages.size() == committed.scripts.pages.size() &&
              candidate.scene.world.world.map.cells.size() == 576 &&
              candidate.scene.calendar.units == committed.scene.calendar.units &&
              candidate.simulation_steps == committed.simulation_steps,
          "moving private intermediates retains returned catalogs, world, pages and date");
    auto returned_random = candidate.scene.random;
    auto committed_random = committed.scene.random;
    for (int i = 0; i < 8; ++i)
        check(returned_random.draw(97).raw == committed_random.draw(97).raw,
              "returned and committed random engines retain the same future stream");
    const auto pages = committed.scripts.pages.size();
    candidate.catalog.clear();
    candidate.scripts.pages.clear();
    candidate.scene.world.world.map.cells.clear();
    check(committed.catalog.size() == 149 && committed.scripts.pages.size() == pages &&
              committed.scene.world.world.map.cells.size() == 576,
          "returned candidate remains independently mutable without changing committed owner");
}
// 真实高星脚本到奖励页的桥接，不重复通用95的拒绝／回滚矩阵。
void delayed_rank_rewards() {
    for (const auto scenario : {std::array<int, 4>{44, 3, 3, 63}, {45, 30, 11, 26}}) {
        StartupSession startup;
        StartupWorldRuntimeSession runtime(startup.state(),
                                           ref::WorldRandomStream::from_java_seed(1));
        auto s = runtime.state();
        const auto count = s.scene.world.facility_order.size();
        const auto funds = s.scene.world.world.ai.accounting.funds();
        const auto &catalog = startup_world_runtime_catalog();
        const auto locked = [&] {
            return scenario[2] == 3 ? s.facility_presence.at(scenario[3]) == 0
                                    : s.scripts.activities.at(scenario[3]).status == 0;
        };
        check(locked(), "real school or second expansion reward begins locked");
        const auto started = ref::prepare_world_script(catalog, startup_world_runtime_scripts(s),
                                                       {scenario[0], {}, {}});
        check(started.candidate &&
                  write_startup_world_runtime_scripts(s, started.candidate->state) &&
                  s.scripts.continuations.size() == 1 &&
                  s.scripts.continuations.front().remaining_updates == scenario[1],
              "actual rank event retains its distinct3 or30 logical-update delay");
        for (int tick = 1; tick <= scenario[1]; ++tick) {
            const auto continued = ref::prepare_world_script_continuations(
                catalog, startup_world_runtime_scripts(s), true);
            check(continued.candidate &&
                      write_startup_world_runtime_scripts(s, continued.candidate->state) &&
                      locked(),
                  "school and expansion stay locked throughout script continuation, including "
                  "reward creation");
            if (tick < scenario[1])
                check(std::none_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [](const auto &p) { return p.legacy_page == 95; }),
                      "actual delayed script does not insert95 before its original threshold");
        }
        const auto reward =
            std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(), [&](const auto &p) {
                return p.legacy_page == 95 && p.legacy_r == scenario[2] &&
                       p.legacy_s == scenario[3];
            });
        check(reward != s.scripts.pages.end() && s.scripts.continuations.empty(),
              "actual44 or45 creates correct95 kind and original definition identity");
        const auto id = reward->id;
        const auto initialized = prepare_startup_world_runtime(s);
        check(initialized.candidate.has_value(), "actual rank reward enters common page framework");
        s = *initialized.candidate;
        s.sound_requests.clear(); // 消费本轮奖励音；不是业务前置或额外逻辑更新。
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.page_counters.at(id) == 40 && locked(),
              "real delayed reward early confirmation reaches40 while definition remains locked");
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  !locked(),
              "actual95 full40 confirmation commits original school or expansion entitlement");
        if (scenario[2] == 3)
            check(s.facility_presence.at(63) == 2 && s.facility_free_builds.at(63) == 1 &&
                      s.facility_unlock_notices.at(63),
                  "school reward opens definition with one H entitlement, never constructs it");
        else
            check(s.scripts.activities.at(26).pending_notice && s.activity_counts.at(26) == 0 &&
                      s.events_held == 0 && s.fence_level == 0,
                  "second expansion reward only opens activity, never holds it or expands map");
        check(s.scene.world.facility_order.size() == count &&
                  s.scene.world.world.ai.accounting.funds() == funds && s.scene.random.draws() == 0,
              "upstream rank rewards create no world entity, payment or random draw");
    }
}
// 事件调用点夹具：五星资格由晋级套件负责，这里只验真实46续体与Owner投影。
void final_rank_profession_unlocks() {
    StartupSession startup;
    StartupWorldRuntimeSession runtime(startup.state(), ref::WorldRandomStream::from_java_seed(1));
    auto s = runtime.state();
    const auto before = s;
    const auto &catalog = startup_world_runtime_catalog();
    check(s.scripts.professions.at(21).status == 0 && s.scripts.professions.at(22).status == 0,
          "original new-world final professions start locked");
    const auto started =
        ref::prepare_world_script(catalog, startup_world_runtime_scripts(s), {46, {}, {}});
    check(started.candidate && write_startup_world_runtime_scripts(s, started.candidate->state) &&
              s.scripts.continuations.size() == 1 &&
              s.scripts.continuations.front().remaining_updates == 30,
          "actual event46 starts its original30 logical update continuation");
    for (int tick = 1; tick <= 30; ++tick) {
        const auto continued = ref::prepare_world_script_continuations(
            catalog, startup_world_runtime_scripts(s), true);
        check(continued.candidate &&
                  write_startup_world_runtime_scripts(s, continued.candidate->state),
              "actual event46 continuation synchronizes through script Owner writer");
        for (int job : {21, 22})
            check(s.scripts.professions.at(job).status == (tick == 30 ? 1 : 0) &&
                      s.scene.world.world.ai.professions.at(job).unlocked == (tick == 30),
                  "both final professions unlock at30, with no stale AI projection or early state");
    }
    check(s.scripts.continuations.empty() && s.scripts.professions.at(21).pending_notice &&
              s.scripts.professions.at(22).pending_notice &&
              std::any_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                          [](const auto &p) {
                              return p.legacy_page == 95 && p.legacy_r == 4 && p.legacy_s == 22;
                          }),
          "opcode31 already unlocks shared professions while actual95 announcement remains "
          "unconfirmed");
    check(s.human_presence == before.human_presence &&
              s.scene.world.world.ai.battle.actors.size() ==
                  before.scene.world.world.ai.battle.actors.size() &&
              s.scene.world.world.ai.growth.size() == before.scene.world.world.ai.growth.size() &&
              s.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds() &&
              s.scene.random.draws() == before.scene.random.draws(),
          "profession unlock does not fabricate arrival, cash or random draws");
    for (const auto &[id, growth] : s.scene.world.world.ai.growth)
        check(growth.definition.current_profession ==
                  before.scene.world.world.ai.growth.at(id).definition.current_profession,
              "opening final professions does not automatically change any human profession");
}
} // namespace
int main() {
    try {
        initial_owner();
        main_character_profile();
        pause_and_private_failure();
        synchronous_event_seen();
        current_route_adapter_boundaries();
        successful_result_retains_independent_owner();
        delayed_rank_rewards();
        final_rank_profession_unlocks();
        std::cout << "startup_world_runtime: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "startup_world_runtime: " << error.what() << '\n';
        return 1;
    }
}
