#include "ark/simulation/combat/rules/world_encounters.hpp"
#include "ark/simulation/world/rules/world_world_entry.hpp"

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
        throw std::runtime_error("missing real script inputs");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
WorldScriptCatalog catalog() {
    const auto root = std::filesystem::path(ARK_WORLD_TEST_DATA) / "scripts/original";
    const auto parsed =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(parsed.catalog.has_value(), "fixed scripts parse");
    return *parsed.catalog;
}
WorldWorldEntryState fixture(std::vector<int> raw = {}) {
    WorldWorldEntryState state;
    state.finish.dungeon.world.map = {24, 24, std::vector<LegacyMapCell>(24 * 24)};
    state.map_surface.assign(24 * 24, 2); // 明确地表夹具，不冒充原新局i.g。
    state.map_flags.assign(24 * 24, 8);
    state.generation_bounds = {Position{0, 24}, Position{24, 0}};
    state.town = {0, 4, 18, 22};
    state.scripts.village_name = "FIXTURE";
    state.random = WorldRandomStream::from_raw(std::move(raw));
    return state;
}
// 这里只用原版f.a已维护的limit门槛做真实消费者；不是伪造空created/none。
WorldWorldEntryCreationConsumer limited() {
    return [](const auto &owner, const auto &input) -> std::optional<WorldWorldEntryCreation> {
        auto next = owner;
        auto actual = input;
        actual.draw = [&](int bound) -> std::optional<int> {
            const auto result = next.random.draw(bound);
            return result.error == WorldRandomError::none ? std::optional<int>{result.ticket}
                                                          : std::nullopt;
        };
        const auto creation = prepare_encounter_creation(next.finish.dungeon.world.ai, actual);
        if (!creation.candidate)
            return {};
        next.finish.dungeon.world.ai = creation.candidate->state;
        if (!creation.candidate->requests.empty())
            return {}; // 未消费任何实际UI/地图请求不能装作成功。
        return WorldWorldEntryCreation{std::move(next), creation.candidate->created,
                                       creation.candidate->denial};
    };
}
void counters_and_probability(const WorldScriptCatalog &scripts) {
    for (int month = 0; month < 12; ++month) {
        auto state = fixture({10});
        const auto result = prepare_world_world_entry(state, {0, month, 0, 0, 0, 0}, scripts);
        check(result.candidate && result.candidate->state.updates == 1 &&
                  result.candidate->state.global_updates == 1 &&
                  result.candidate->state.random.draws() == (month < 6 ? 0u : 1u),
              "first six months skips chance draw, all admitted e calls advance distinctV/aK");
    }
    for (int ticket : {9, 10, 999, -9, -10}) {
        auto state = fixture({ticket, 10, 10});
        state.finish.dungeon.world.ai.monster_limit = 0;
        const auto result =
            prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts, limited());
        const bool pass = std::abs(ticket) < 10;
        check(result.candidate && result.candidate->attempted_creation == pass &&
                  result.candidate->state.random.draws() == (pass ? 3u : 1u) &&
                  result.candidate->denial ==
                      (pass ? EncounterCreationDenial::limit : EncounterCreationDenial::none),
              "chance strict10 uses source signed remainder abs, selected-site actualf.a limit "
              "still consumes chance/site");
    }
    auto state = fixture({10});
    state.updates = std::numeric_limits<int>::max() - 1;
    state.global_updates = 17;
    const auto wrap = prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts);
    check(wrap.candidate && wrap.candidate->state.updates == 0 &&
              wrap.candidate->state.global_updates == 18,
          "onlyV wraps moduloMAX, aK is independent increment");
    state.global_updates = std::numeric_limits<int>::max();
    check(prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts).error ==
              WorldWorldEntryError::overflow,
          "aK overflow rejected before chance, not aliased to moduloV");
}
void sites(const WorldScriptCatalog &scripts) {
    for (int kind = 0; kind < 4; ++kind) {
        std::vector<int> raw{0};
        for (int n = 0; n < 20; ++n) {
            raw.push_back(10);
            raw.push_back(10);
        }
        auto state = fixture(raw);
        if (kind == 0)
            state.finish.dungeon.world.map.cells[10 * 24 + 10].legacy_state = 3;
        if (kind == 1)
            state.town = {9, 11, 9, 11};
        if (kind == 2) {
            state.finish.tasks[1] = {1, 30, 1, 0, {}, Position{13, 13}};
            state.finish.task_order = {1};
        }
        if (kind == 3) {
            RewardEncounter event;
            event.runtime.id = 1;
            event.runtime.center = {7, 7};
            state.finish.dungeon.world.ai.encounters[1] = event;
            state.finish.dungeon.world.ai.encounter_order = {1};
        }
        const auto result = prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts);
        check(result.candidate && !result.candidate->selected_site &&
                  !result.candidate->attempted_creation &&
                  result.candidate->state.random.draws() == 41 &&
                  result.candidate->random_bounds.size() == 41,
              "actualk.c twenty x/y pairs reject wrongstate/interior/closed3 task/closed3 "
              "encounter, normal null commits consumption");
    }
    auto state = fixture({0, 10, 10, 14, 14});
    state.finish.tasks[1] = {1, 30, 1, 0, {}, Position{10, 10}};
    state.finish.task_order = {1};
    state.finish.dungeon.world.ai.monster_limit = 0;
    const auto later = prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts, limited());
    check(later.candidate && later.candidate->selected_site == std::optional<Position>{{14, 14}} &&
              later.candidate->state.random.draws() == 5,
          "four-away second site accepted after first closed3 overlap, no extra twenty retries");
    state = fixture({0, 0, 10});
    state.town = {0, 4, 8, 12};
    state.finish.dungeon.world.ai.monster_limit = 0;
    const auto boundary = prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts, limited());
    check(boundary.candidate &&
              boundary.candidate->selected_site == std::optional<Position>{{0, 10}} &&
              boundary.candidate->denial == EncounterCreationDenial::town,
          "k.c allows strict townboundary, actualf.a upperband inclusive check then denies");
    state = fixture({0, 10, 10});
    state.generation_bounds = {Position{10, 11}, Position{11, 10}};
    state.finish.dungeon.world.ai.monster_limit = 0;
    const auto halfopen = prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts, limited());
    check(halfopen.candidate &&
              halfopen.candidate->selected_site == std::optional<Position>{{10, 10}} &&
              halfopen.candidate->random_bounds == std::vector<int>{1000, 1, 1},
          "bounds halfopen exclude right/lower endpoint and consume bound1 draws");
}
void failures(const WorldScriptCatalog &scripts) {
    auto state = fixture({0, 10, 10});
    const auto missing = prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts);
    check(missing.error == WorldWorldEntryError::missing_consumer && !missing.candidate &&
              state.updates == 0 && state.random.draws() == 0,
          "selected real site cannot succeed without actualf.a consumer, whole counters/random "
          "rollback");
    const WorldWorldEntryCreationConsumer empty =
        [](const auto &owner, const auto &) -> std::optional<WorldWorldEntryCreation> {
        return WorldWorldEntryCreation{owner, {}, EncounterCreationDenial::none};
    };
    check(prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts, empty).error ==
              WorldWorldEntryError::consumer_failed,
          "empty result without actualdenial explicitly rejected, no fake event success");
    state = fixture({0});
    state.generation_bounds = {Position{0, 24}, Position{0, 0}};
    check(prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts).error ==
              WorldWorldEntryError::random_failed,
          "zero width source draw0 fails on copied stream without unsigned bounds rewrite");
    state = fixture({0, 10, 10});
    state.finish.tasks[1] = {1, 30, 1, 0, {}, {}};
    state.finish.task_order = {1};
    check(prepare_world_world_entry(state, {1, 0, 0, 0, 0, 0}, scripts).error ==
              WorldWorldEntryError::invalid_owner,
          "all currentbq centers needed, cannot use only active task");
}
void actual_creation(const WorldScriptCatalog &scripts) {
    auto input = fixture({0, 10, 10, 0, 0, 0, 0});
    auto &world = input.finish.dungeon.world.ai;
    RewardMonsterDefinition monster;
    monster.base_hp = 20;
    monster.base_attack = 5;
    monster.base_defense = 5;
    monster.status = 1;
    monster.introduced = true; // 明确已介绍定义夹具，无未消费的inline脚本/页89。
    world.monster_growth[0] = monster;
    world.monster_definition_order = {0};
    world.battle.monsters[0] = {0, 0, 20, 5, 5, 0};
    const WorldWorldEntryCreationConsumer create =
        [](const auto &owner, const auto &request) -> std::optional<WorldWorldEntryCreation> {
        auto next = owner;
        auto actual = request;
        actual.draw = [&](int bound) -> std::optional<int> {
            const auto draw = next.random.draw(bound);
            return draw.error == WorldRandomError::none ? std::optional<int>{draw.ticket}
                                                        : std::nullopt;
        };
        const auto created = prepare_encounter_creation(next.finish.dungeon.world.ai, actual);
        if (!created.candidate)
            return {};
        next.finish.dungeon.world.ai = created.candidate->state;
        for (const auto &effect : created.candidate->requests) {
            if (effect.kind != EncounterCreationRequestKind::refresh_map)
                return {}; // 未实际消费首次程序/页89必须失败，不能排队装作全链成功。
            const auto map = prepare_world_event_map(
                next.finish.dungeon.world.ai,
                {next.finish.dungeon.world.map, next.map_surface, next.map_flags, next.town});
            if (!map.facts)
                return {};
            next.map_flags = map.facts->flags;
        }
        return WorldWorldEntryCreation{std::move(next), created.candidate->created,
                                       created.candidate->denial};
    };
    const auto created = prepare_world_world_entry(input, {0, 7, 0, 0, 0, 0}, scripts, create);
    check(created.candidate && created.candidate->created &&
              created.candidate->state.finish.dungeon.world.ai.monster_order.size() == 1 &&
              created.candidate->state.random.draws() == 7 &&
              created.candidate->state.map_flags[10 * 24 + 10] == 10,
          "actualf.a allocates one monster/event and sourcebit2 map, all seven RNG draws share "
          "owner");
    check(input.finish.dungeon.world.ai.monster_order.empty() && input.random.draws() == 0 &&
              input.map_flags[10 * 24 + 10] == 8,
          "successful private normalcreation never mutates original actors/stream/map flags");
    const WorldWorldEntryCreationConsumer empty_event =
        [](const auto &owner, const auto &) -> std::optional<WorldWorldEntryCreation> {
        auto next = owner;
        RewardEncounter event;
        event.runtime.id = 1;
        next.finish.dungeon.world.ai.encounters[1] = event;
        next.finish.dungeon.world.ai.encounter_order.push_back(1);
        return WorldWorldEntryCreation{std::move(next), 1, EncounterCreationDenial::none};
    };
    check(prepare_world_world_entry(input, {0, 7, 0, 0, 0, 0}, scripts, empty_event).error ==
              WorldWorldEntryError::consumer_failed,
          "empty event receipt without actual spawned members cannot fake completef.a");
}
} // namespace
int main() {
    try {
        const auto scripts = catalog();
        counters_and_probability(scripts);
        sites(scripts);
        failures(scripts);
        actual_creation(scripts);
        std::cout << "world entry checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
