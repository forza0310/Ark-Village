#include "ark/simulation/rules/world_arrivals.hpp"

#include <filesystem>
#include <fstream>
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
std::string read(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error("missing source script catalog");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
WorldScriptCatalog catalog() {
    const auto root = std::filesystem::path(ARK_WORLD_TEST_DATA) / "scripts/original";
    const auto parsed =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(parsed.catalog.has_value(), "parse actual fixed source scripts");
    return *parsed.catalog;
}
WorldArrivalsState fixture(std::vector<int> raw = {}) {
    WorldArrivalsState s;
    s.random = WorldRandomStream::from_raw(std::move(raw));
    s.scripts.village_name = "FIXTURE";
    s.definitions = {{0, 1, 1, 0, {}}, {1, 1, 2, 0, {}}, {2, 1, 1, 0, {}}, {3, 1, 2, 0, {}}};
    s.spawn_cells = {{2, 3}, {4, 5}};
    return s;
}
void mark_seen(WorldArrivalsState &s, int id) {
    s.scripts.event_calls[id] = 1;
    s.finish.event_calls = s.scripts.event_calls;
}
void add_actor(WorldArrivalsState &s, int definition, int uid, CharacterId id, Position spawn) {
    BattleActorRecord a;
    a.id = id;
    a.definition = definition;
    a.legacy_id = uid;
    a.control.flags = 2;
    a.control.queue = {{8, 0}};
    a.hp = {0, 90, 90, 90, false, 0}; // 明确构造夹具；原HP/装备初值由生产消费者负责。
    a.position = {spawn.x * 100.0F + 50.0F, 0, spawn.y * 100.0F + 50.0F};
    auto &world = s.finish.dungeon.world;
    world.ai.battle.actors.emplace(id, a);
    world.ai.human_order.push_back(id);
    RewardActorContext cache;
    cache.cell = spawn;
    world.ai.contexts.emplace(id, cache);
    world.actors.emplace(id, RescueActorContext{});
    s.scripts.human_order.push_back(id.value);
    world.ai.next_actor_id = id.value + 1;
}
WorldArrivalCreationConsumer create_fixture() {
    return [](const auto &s, const auto &input) -> std::optional<WorldArrivalCreation> {
        auto n = s;
        const CharacterId id{n.finish.dungeon.world.ai.next_actor_id};
        add_actor(n, input.definition, input.legacy_uid, id, input.spawn);
        return WorldArrivalCreation{std::move(n), id};
    };
}
void gates(const WorldScriptCatalog &catalog) {
    for (int delay : {-1, 0, 1, 2})
        for (int follow : {0, 1, 2})
            for (int counter : {-1, 0, 1, 2}) {
                auto s = fixture({149, 0, 0});
                mark_seen(s, 89);
                s.camera_delay = delay;
                s.camera_follow = follow;
                s.arrival_counter = counter;
                const auto r = prepare_world_arrivals(s, catalog, create_fixture());
                const bool due = counter <= 1;
                const bool fast = delay > 1 || follow == 1;
                check(r.candidate && r.candidate->due == due &&
                          r.candidate->state.camera_delay == (delay > 0 ? delay - 1 : delay) &&
                          r.candidate->state.arrival_counter ==
                              (due ? (fast ? 19 : 249) : counter - 1) &&
                          r.candidate->state.random.draws() == (due ? 3u : 0u),
                      "delay decrements but admits arrivals, follow exactly1, B decrements first");
                check(s.random.draws() == 0 && s.arrival_counter == counter &&
                          s.camera_delay == delay,
                      "input counters and random unchanged");
            }
    for (int delay : {0, 1, 2}) {
        auto s = fixture({0, 0, 0});
        mark_seen(s, 89);
        add_actor(s, 0, 0, {1}, {0, 0});
        s.soft_limit = 1;
        s.hard_limit = 2;
        s.camera_delay = delay;
        s.arrival_counter = 1;
        const auto r = prepare_world_arrivals(s, catalog, create_fixture());
        check(r.candidate && r.candidate->due == (delay > 0) &&
                  r.candidate->state.random.draws() == (delay > 0 ? 3u : 0u),
              "old positive camera bypasses Y even if new delay0, but still checks aa");
        s.hard_limit = 1;
        const auto blocked = prepare_world_arrivals(s, catalog);
        check(blocked.candidate && !blocked.candidate->due &&
                  blocked.candidate->state.arrival_counter == 1 &&
                  blocked.candidate->state.camera_delay == (delay > 0 ? delay - 1 : delay),
              "aa blocks B, camera decrement happens before aa");
    }
    auto s = fixture({});
    s.arrival_counter = 420;
    for (int i = 0; i < 419; ++i) {
        auto r = prepare_world_arrivals(s, catalog);
        check(r.candidate && !r.candidate->due && r.candidate->state.random.draws() == 0,
              "real new-game420 gate does not consume before last update");
        s = r.candidate->state;
    }
    check(s.arrival_counter == 1, "420th admitted arrival update becomes due");
}
void candidates(const WorldScriptCatalog &catalog) {
    auto s = fixture({0, 0, 1});
    mark_seen(s, 89);
    auto r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->sorted_candidates == std::vector<int>({3, 1, 2, 0}) &&
              r.candidate->top_candidates == std::vector<int>({3, 1}) &&
              r.candidate->selected_definition == 3 &&
              r.candidate->random_bounds == std::vector<int>({150, 2, 2}),
          "reverse inner swap preserves source nonstable top order, consumes bound1 too");
    s.random = WorldRandomStream::from_raw({0, 1, 0});
    r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->selected_definition == 1, "tie ticket1 reads swapped order");
    s.definitions[3].presence = 0;
    add_actor(s, 1, 9, {1}, {0, 0});
    r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->sorted_candidates == std::vector<int>({0, 2}) &&
              r.candidate->selected_definition == 2 &&
              r.candidate->state.finish.dungeon.world.ai.battle.actors.at(*r.candidate->created)
                      .legacy_id == 0,
          "presence0 excluded, existing definition excluded, UID first hole independently0");
    s.definitions[0].presence = -3;
    s.definitions[2].presence = 0;
    r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->selected_definition == 0 &&
              r.candidate->random_bounds == std::vector<int>({150, 1, 2}),
          "negative nonzero presence admitted; single top candidate still draws bound1");
    s.definitions[0].presence = 0;
    s.random = WorldRandomStream::from_raw({149});
    r = prepare_world_arrivals(s, catalog);
    check(r.candidate && r.candidate->due && !r.candidate->created &&
              r.candidate->state.arrival_counter == 249 && r.candidate->state.random.draws() == 1,
          "empty pool resets B/consumes reset, no creation callback needed");
}
void first_and_debug(const WorldScriptCatalog &catalog) {
    for (int mode : {0, 1, 2, 3}) {
        auto s = fixture({0, 1, 0, 1});
        s.definitions[0].presence = 0;
        s.definitions[0].flags = 8;
        s.definitions[2].flags = 8;
        s.debug_mode = mode;
        s.debug_definitions = {{{2}, {3}}};
        const auto r = prepare_world_arrivals(s, catalog, create_fixture());
        check(r.candidate && r.candidate->created && r.candidate->selected_definition == 0 &&
                  r.candidate->executed_events == std::vector<int>({89}) &&
                  r.candidate->state.random.draws() == (mode == 1 || mode == 2 ? 4u : 3u),
              "first flags8 overrides even presence0, ordinary/debug draws still consumed");
        const auto &n = r.candidate->state;
        check(world_script_seen(n.scripts, 89) && n.finish.event_calls == n.scripts.event_calls &&
                  n.scripts.selected_actor == r.candidate->created->value &&
                  n.scripts.scene_mode == 6 && n.scripts.scene_updates == 0 &&
                  n.scripts.pages.size() == 1 && n.scripts.pages.front().source_record == 69 &&
                  (n.finish.dungeon.world.ai.battle.actors.at(*r.candidate->created).control.flags &
                   8192U),
              "actual89 executes talk69 and12 after actor append, scene6 camera selected");
    }
    auto s = fixture({0, 0, 0});
    add_actor(s, 0, 0, {1}, {0, 0});
    s.definitions[0].flags = 8;
    s.definitions[0].job_history = {false, true};
    auto r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->selected_definition == 0 &&
              r.candidate->state.finish.dungeon.world.ai.human_order.size() == 2 &&
              r.candidate->state.scripts.selected_actor == 1 &&
              r.candidate->executed_events == std::vector<int>({89, 218}) &&
              r.candidate->state.scripts.continuations.size() == 1 &&
              r.candidate->state.scripts.continuations[0].remaining_updates == 300 &&
              r.candidate->state.scripts.continuations[0].event == 218,
          "first override bypasses existing definition,12 uses actualbl0, Q schedules actual218");
    auto n = r.candidate->state;
    const auto continuation = prepare_world_script_continuations(catalog, n.scripts, true);
    check(continuation.candidate &&
              continuation.candidate->state.continuations[0].remaining_updates == 299,
          "218 actual waiting point, no newspaper immediately");
    mark_seen(s, 89);
    mark_seen(s, 218);
    s.debug_mode = 1;
    s.debug_definitions[0] = {0};
    s.random = WorldRandomStream::from_raw({0, 0, 0, 0});
    r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->selected_definition == 0 &&
              r.candidate->executed_events.empty(),
          "debug may create duplicate definition, seen218 prevents repeat");
    s = fixture({0, 1, 1}); // first but no flag8: retain ordinary chosen1。
    r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->selected_definition == 1 && r.candidate->first,
          "first scan exhausted keeps previously selected instead of -1");
    s = fixture({0, 0, 0});
    mark_seen(s, 89);
    s.debug_mode = 2;
    s.debug_definitions[1] = {2};
    s.random = WorldRandomStream::from_raw({0, 0, 0, 0});
    r = prepare_world_arrivals(s, catalog, create_fixture());
    check(r.candidate && r.candidate->selected_definition == 2, "debug2 uses ai not ah");
    s = fixture({0, 0, 0});
    const auto refreshed = [](const auto &owner,
                              const auto &input) -> std::optional<WorldArrivalCreation> {
        auto made = create_fixture()(owner, input);
        for (auto &d : made->state.definitions)
            if (d.identity == input.definition)
                d.job_history = {true};
        return made;
    };
    r = prepare_world_arrivals(s, catalog, refreshed);
    check(r.candidate && r.candidate->executed_events == std::vector<int>({89, 218}),
          "218 reads job history after actual equipment-derived refresh, not before creation");
}
void failures(const WorldScriptCatalog &catalog) {
    auto s = fixture({0, 0, 0});
    check(prepare_world_arrivals(s, catalog).error == WorldArrivalsError::missing_consumer,
          "no real creation consumer rejects success");
    for (std::vector<int> tape : {std::vector<int>{}, {0}, {0, 0}}) {
        s.random = WorldRandomStream::from_raw(tape);
        check(prepare_world_arrivals(s, catalog, create_fixture()).error ==
                      WorldArrivalsError::random_failed &&
                  s.random.draws() == 0,
              "each missing actual random boundary fails without committing partial B");
    }
    s = fixture({0, 0, 0});
    const auto failed_consumer =
        [](const auto &, const auto &) -> std::optional<WorldArrivalCreation> { return {}; };
    check(prepare_world_arrivals(s, catalog, failed_consumer).error ==
                  WorldArrivalsError::consumer_failed &&
              s.arrival_counter == 0 && s.random.draws() == 0,
          "late creation failure rolls back all candidate counters/random");
    auto missing = catalog;
    missing.events.erase(89);
    check(prepare_world_arrivals(s, missing, create_fixture()).error ==
                  WorldArrivalsError::script_failed &&
              s.scripts.pages.empty() && s.finish.dungeon.world.ai.human_order.empty(),
          "actual89 missing after creating actor rolls back actor/page/counters");
    s.definitions[3].job_history = {true};
    missing = catalog;
    missing.events.erase(218);
    check(prepare_world_arrivals(s, missing, create_fixture()).error ==
                  WorldArrivalsError::script_failed &&
              s.scripts.pages.empty(),
          "actual218 failure after actual89 is atomic");
    s.spawn_cells.clear();
    check(prepare_world_arrivals(s, catalog, create_fixture()).error ==
              WorldArrivalsError::random_failed,
          "empty spawn bound0 explicit error, no invented origin");
    s = fixture({0, 0, 0});
    s.debug_mode = 1;
    check(prepare_world_arrivals(s, catalog, create_fixture()).error ==
              WorldArrivalsError::random_failed,
          "empty debug pool bound0 consumes only candidate random then fails");
    auto forged = [](const auto &s, const auto &input) -> std::optional<WorldArrivalCreation> {
        auto r = create_fixture()(s, input);
        r->state.finish.dungeon.world.ai.battle.actors.at(r->actor).legacy_id =
            input.legacy_uid + 1;
        return r;
    };
    s = fixture({0, 0, 0});
    check(prepare_world_arrivals(s, catalog, forged).error == WorldArrivalsError::consumer_failed,
          "consumer must use real first freeUID");
    auto bad = s;
    bad.definitions[1].identity = 0;
    check(prepare_world_arrivals(bad, catalog).error == WorldArrivalsError::invalid_owner,
          "duplicate source identities rejected");
    bad = s;
    bad.finish.event_calls[89] = 1;
    check(prepare_world_arrivals(bad, catalog).error == WorldArrivalsError::invalid_owner,
          "split script seen ownership rejected");
}
} // namespace
int main() {
    try {
        const auto scripts = catalog();
        gates(scripts);
        candidates(scripts);
        first_and_debug(scripts);
        failures(scripts);
        std::cout << "world_arrivals: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_arrivals: " << e.what() << '\n';
        return 1;
    }
}
