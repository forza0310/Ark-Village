#include "dungeon_village_reference/world_residence.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
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
        throw std::runtime_error("missing real scripts");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
WorldScriptCatalog catalog() {
    const auto root = std::filesystem::path(__FILE__).parent_path().parent_path().parent_path() /
                      "data/scripts/original";
    const auto result =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(result.catalog.has_value(), "published original tables parse");
    return *result.catalog;
}
// 人物/建筑/职业/本地化名字为明确实例夹具；原e.i程序列另逐25原记录验证。
WorldResidenceState fixture(int effort = 0, int satisfaction = 0) {
    WorldResidenceState state;
    state.facility.cycle_length = 80;
    RescueFacility house;
    house.placement = {{1}, 79, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    house.kind = 12;
    house.category = 3;
    house.status = 1;
    state.facility.finish.dungeon.world.facilities[1] = house;
    state.facility.finish.dungeon.facilities[1] = {};
    state.facility.details[1] = {10, 1, 0, 1, 0, {}};
    state.facility.definitions[79] = {0, 20};
    state.facility.scripts.humans[0] = {1, false, "fixture", 0, satisfaction};
    WorldScriptPage scene;
    scene.id = 1;
    state.facility.scripts.pages = {scene};
    state.facility.scripts.next_page_id = 2;
    state.facility.scripts.executing_page = 1;
    state.facility.scripts.village_name = "FIXTURE";
    state.facility.random = WorldRandomStream::from_raw({});
    HumanProfessionRule profession;
    profession.attribute_percent.fill(100);
    state.professions = {profession};
    WorldResidenceHuman human;
    human.definition.current_profession = 0;
    human.definition.profession_levels = {1};
    human.definition.legacy_u = effort;
    human.definition.base = {40, 5, 5, 5, 5, 5};
    human.derived = *derive_human_stats(human.definition, state.professions).candidate;
    human.arrival_program = *parse_world_script_program("2,29&2,34&27,1"); // 原人物0第9列。
    human.localized_name = "FIXTURE HUMAN";
    human.celebrations = 7;
    state.humans[0] = human;
    state.effort_display = {std::array<int, 4>{1, 2, 3, 4}, std::array<int, 4>{5, 6, 7, 8},
                            std::array<int, 4>{4, 4, 4, 4}};
    return state;
}
void awards(const WorldScriptCatalog &scripts) {
    for (int old_effort = 0; old_effort <= 100; ++old_effort)
        for (int old_satisfaction : {0, 90, 94, 95, 96, 99, 100}) {
            const auto input = fixture(old_effort, old_satisfaction);
            const auto result = prepare_world_residence(input, 1, scripts);
            const int new_effort = std::min(old_effort + 15, 100);
            const int new_satisfaction = std::min(old_satisfaction + 5, 100);
            const bool crossed = new_effort / 10 > old_effort / 10;
            check(result.candidate && result.candidate->effort_threshold_crossed == crossed &&
                      result.candidate->state.humans.at(0).definition.legacy_u == new_effort &&
                      result.candidate->state.facility.scripts.humans.at(0).satisfaction ==
                          new_satisfaction,
                  "source C+5/u+15 uppercaps100 and effort tenboundary gate for every old value");
            const auto &next = result.candidate->state;
            check(next.reward_display ==
                          std::array<std::array<int, 2>, 3>{{{old_satisfaction, old_effort},
                                                             {new_satisfaction, new_effort},
                                                             {5, 15}}} &&
                      next.facility.scripts.pending_completion == 5 &&
                      next.facility.finish.dungeon.world.ai.pending_completion == 5 &&
                      next.facility.scripts.event_calls.at(58) == 1 &&
                      next.facility.scripts.continuations.front().remaining_updates == 3,
                  "even saturated satisfaction contributes request5 before real58 wait3, aH keeps "
                  "requested increments");
            check(next.facility.random.draws() == 0 && next.humans.at(0).celebrations == 7 &&
                      next.facility.finish.dungeon.world.ai.battle.actors.empty(),
                  "residence reward is not actor creation, zfalse does not incrementE or consume "
                  "random");
            if (crossed) {
                check(result.candidate->pages.at(1).legacy_page == 67 &&
                          result.candidate->pages.at(1).legacy_f == old_effort &&
                          result.candidate->pages.at(1).legacy_g == new_effort &&
                          next.effort_display[0] == input.humans.at(0).derived.combat &&
                          next.effort_display[1] == next.humans.at(0).derived.combat,
                      "crossing recomputes old/new sharedw and actual67 payload, no HP refresh "
                      "invented");
                for (std::size_t slot = 0; slot < 4; ++slot)
                    check(next.effort_display[2][slot] ==
                              next.effort_display[1][slot] - next.effort_display[0][slot],
                          "as effort delta uses actual combat slot difference");
            } else
                check(next.effort_display == input.effort_display &&
                          next.humans.at(0).derived.combat == input.humans.at(0).derived.combat,
                      "without crossing old derived stats/as remain untouched evenu rises within "
                      "same decade");
            check(
                result.candidate->pages.front().legacy_page == 96 &&
                    next.page_bindings.at(result.candidate->pages.front().id).facility_definition ==
                        std::optional<int>{79} &&
                    next.facility.scripts.event_calls.size() == 2 &&
                    next.facility.scripts.event_calls.at(202) == 1 &&
                    next.facility.scripts.continuations.back().replacement ==
                        "FIXTURE\tFIXTURE HUMAN",
                "actual96 m/o references precede optional67 then unregistered human program and "
                "first202 wait300");
            check(result.candidate->pages.back().legacy_page == 94 &&
                      result.candidate->pages.back().legacy_r == 7 &&
                      result.candidate->pages.back().legacy_s == 1 &&
                      next.facility.finish.dungeon.catalog.empty(),
                  "source gift27 prepares page94r7/s1, no eager accessory unlock before page "
                  "completion");
        }
}
void original_programs(const WorldScriptCatalog &scripts) {
    // 固定character.txt SHA 8d9bb046...，第9列原25值；不是自定入住奖励程序。
    const std::array<const char *, 25> programs{
        {"2,29&2,34&27,1",  "2,28&2,34&25,13", "2,30&2,34&24,59", "2,31&2,34&28,5",
         "2,28&2,34&25,3",  "2,29&2,34&28,2",  "2,29&2,34&27,18", "2,30&2,34&26,21",
         "2,31&2,34&28,24", "2,28&2,34&28,26", "2,29&2,34&24,62", "2,30&2,34&28,8",
         "2,30&2,34&25,11", "2,31&2,34&24,55", "2,28&2,34&24,61", "2,29&2,34&25,31",
         "2,30&2,34&26,36", "2,31&2,34&26,40", "2,31&2,34&28,15", "2,28&2,34&24,43",
         "2,29&2,34&27,25", "2,30&2,34&25,6",  "2,31&2,34&28,14", "2,28&2,34&25,25",
         "2,28&2,34&24,65"}};
    for (std::size_t id = 0; id < programs.size(); ++id) {
        auto input = fixture(100, 100);
        auto human = input.humans.at(0);
        human.arrival_program = *parse_world_script_program(programs[id]);
        input.humans.clear();
        input.humans[static_cast<int>(id)] = human;
        input.facility.scripts.humans.clear();
        input.facility.scripts.humans[static_cast<int>(id)] = {1, false, "fixture", 0, 100};
        input.facility.details.at(1).resident_definition = static_cast<int>(id);
        const auto result = prepare_world_residence(input, 1, scripts);
        check(result.candidate && result.candidate->pages.size() == 4 &&
                  result.candidate->state.facility.scripts.event_calls.size() == 2,
              "all25 original human programs run exactly three pages without fictitious aM "
              "registration");
        const auto &gift = result.candidate->pages.back();
        const int opcode = human.arrival_program.back()[0];
        check(gift.legacy_r == (opcode == 24 ? 3 : opcode - 20) &&
                  gift.legacy_s == human.arrival_program.back()[1] && gift.speaker_kind == 1 &&
                  gift.speaker_definition == static_cast<int>(id) &&
                  result.candidate->state.page_bindings.at(gift.id).human == static_cast<int>(id),
              "original24..28 map r3/5/6/7/8 with definition and correctm/j/k");
    }
    auto changed = scripts; // 显式夹具检验解释器的原override守卫，不覆盖原表文件。
    changed.talks.at(29).speaker_kind = 2;
    changed.talks.at(29).speaker_definition = 5;
    const auto source = fixture(100, 100);
    const auto preserved = prepare_world_residence(source, 1, changed);
    check(preserved.candidate && preserved.candidate->pages.at(1).speaker_kind == 2 &&
              preserved.candidate->pages.at(1).speaker_definition == 5 &&
              preserved.candidate->state.page_bindings.at(preserved.candidate->pages.at(1).id)
                      .human == 0,
          "source override preserves explicit talkk while m still binds the resident");
}
void failures_and_repeats(const WorldScriptCatalog &scripts) {
    auto input = fixture(0, 0);
    auto first = prepare_world_residence(input, 1, scripts);
    check(first.candidate.has_value(), "first valid construction award candidate");
    auto repeat = prepare_world_residence(first.candidate->state, 1, scripts);
    check(repeat.candidate && repeat.candidate->state.facility.scripts.event_calls.at(58) == 2 &&
              repeat.candidate->state.facility.scripts.event_calls.at(202) == 1 &&
              repeat.candidate->state.facility.scripts.pending_completion == 10,
          "source reward function has no once dedup, only202 guard; outer constructionstatus "
          "avoids repeat normally");
    input.facility.scripts.page_mutations_locked = true;
    const auto locked = prepare_world_residence(input, 1, scripts);
    check(locked.candidate && locked.candidate->pages.empty() &&
              locked.candidate->state.page_bindings.empty() &&
              locked.candidate->state.humans.at(0).definition.legacy_u == 15 &&
              locked.candidate->state.facility.scripts.continuations.size() == 2,
          "l lock suppresses page stack only, not sharedstats/reward/scripts");
    input = fixture();
    input.humans.at(0).arrival_program.push_back({6, 10});
    const auto unknown = prepare_world_residence(input, 1, scripts);
    check(unknown.error == WorldResidenceError::unsupported_program && !unknown.candidate &&
              input.facility.scripts.event_calls.empty() &&
              input.humans.at(0).definition.legacy_u == 0,
          "unsupported unregistered wait cannot fabricate continuationID, late whole reward/pages "
          "roll back");
    input = fixture();
    input.professions.clear();
    check(prepare_world_residence(input, 1, scripts).error == WorldResidenceError::stats_failed,
          "missing real stat rules cannot be filled with demo attributes");
    input = fixture(100, 100);
    input.professions.clear();
    check(prepare_world_residence(input, 1, scripts).candidate.has_value(),
          "already100 does not read unused profession rules or recompute cache");
    input = fixture();
    input.facility.scripts.pending_completion = std::numeric_limits<int>::max();
    input.facility.finish.dungeon.world.ai.pending_completion =
        input.facility.scripts.pending_completion;
    check(prepare_world_residence(input, 1, scripts).error == WorldResidenceError::overflow,
          "rewardcompletion overflow fails before scripts/pages commit");
    input = fixture();
    input.facility.details.at(1).residence_mode = 0;
    check(prepare_world_residence(input, 1, scripts).error ==
              WorldResidenceError::invalid_construction_route,
          "only original u1 construction route admitted");
}
} // namespace
int main() {
    try {
        const auto scripts = catalog();
        awards(scripts);
        original_programs(scripts);
        failures_and_repeats(scripts);
        std::cout << "residence checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
