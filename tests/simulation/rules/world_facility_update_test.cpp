#include "ark/simulation/facilities/rules/world_facility_update.hpp"

#include <algorithm>
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
    const auto result =
        parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                   read(root / "news.txt"), read(root / "evtmsgs.txt"));
    check(result.catalog.has_value(), "fixed source script tables parse");
    return *result.catalog;
}
// 不代表新局设施或原版动态consumer；只验证本地规则和已发布固定脚本。
WorldFacilityUpdateState fixture() {
    WorldFacilityUpdateState state;
    state.cycle_length = 80;
    WorldScriptPage scene;
    scene.id = 1;
    state.scripts.pages.push_back(scene);
    state.scripts.next_page_id = 2;
    state.scripts.executing_page = 1;
    state.scripts.village_name = "FIXTURE";
    state.definitions[35] = {0, 20};
    RescueFacility facility;
    facility.placement = {{1}, 35, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    facility.kind = 3;
    facility.category = 5;
    facility.detail = 0;
    facility.status = 0;
    state.finish.dungeon.world.facilities[1] = facility;
    state.finish.dungeon.facilities[1] = {};
    state.details[1] = {10, 1, 0, 0, -1, {}};
    for (int id : {7, 13, 21, 22})
        state.scripts.activities[id] = {};
    return state;
}
void prefix(const WorldScriptCatalog &scripts) {
    for (int status : {0, 1, 2, 3})
        for (int notice = 0; notice < 8; ++notice) {
            const int duration = notice == 0 ? 44 : notice == 7 ? 60 : 43;
            auto state = fixture();
            state.finish.dungeon.world.facilities.at(1).status = status;
            state.details.at(1).notices = {{notice, duration - 2}, {7, 17}};
            const auto before = prepare_world_facility_update(state, 1, scripts);
            check(before.candidate &&
                      before.candidate->state.finish.dungeon.facilities.at(1).updates == 1 &&
                      before.candidate->state.details.at(1).notices ==
                          std::vector<std::array<int, 2>>{{notice, duration - 1}, {7, 17}},
                  "all statuses run f prefix and only the head display timer");
            const auto after = prepare_world_facility_update(before.candidate->state, 1, scripts);
            check(after.candidate && after.candidate->state.details.at(1).notices ==
                                         std::vector<std::array<int, 2>>{{7, 17}},
                  "expiry deletes head without ageing newly exposed second timer this call");
        }
    auto state = fixture();
    state.details.at(1).notices = {{8, 0}};
    const auto invalid = prepare_world_facility_update(state, 1, scripts);
    check(!invalid.candidate && state.finish.dungeon.facilities.at(1).updates == 0,
          "invalid source F index rolls back increment");
}
void construction(const WorldScriptCatalog &scripts) {
    for (int condition = -1; condition <= 11; ++condition)
        for (int kind : {3, 12, 13}) {
            auto state = fixture();
            state.finish.dungeon.world.facilities.at(1).kind = kind;
            state.finish.dungeon.facilities.at(1).updates = 9;
            state.details.at(1).condition = condition;
            const auto result = prepare_world_facility_update(state, 1, scripts);
            check(result.candidate && result.candidate->construction_completed &&
                      result.candidate->state.finish.dungeon.world.facilities.at(1).status == 1 &&
                      result.candidate->state.finish.dungeon.facilities.at(1).updates == 0 &&
                      result.candidate->state.finish.dungeon.facilities.at(1).extent ==
                          9600 + (std::clamp(condition, 1, 9) - 1) * (1920 - 9600) / 8,
                  "b1 resets f/g and interpolates clamped original Q*120 to Q*24, no guessed "
                  "seconds");
            const auto &next = result.candidate->state;
            check(next.details.at(1).notices.size() == (kind == 13 ? 0u : 1u) &&
                      next.details.at(1).completion_popularity == (kind == 3 ? 20 : 0) &&
                      next.definitions.at(35).popularity_reward == (kind == 3 ? 10 : 20) &&
                      next.scripts.popularity_queue.size() == (kind == 3 ? 1u : 0u),
                  "ordinary construction snapshotsN/queuesI25/halves sharedN; housing and advert "
                  "zero m");
            check(world_script_seen(next.scripts, 88) == (kind != 13) &&
                      world_script_seen(next.scripts, 117) == (kind == 13) &&
                      world_script_seen(next.scripts, 118) == (kind == 12),
                  "actual first-script counters and waits distinguish kind13 advert from kind12 "
                  "housing");
            const auto repeated = prepare_world_facility_update(next, 1, scripts);
            check(
                repeated.candidate && !repeated.candidate->construction_completed &&
                    repeated.candidate->state.scripts.continuations.size() ==
                        next.scripts.continuations.size(),
                "completed status1 does not rerun construction or halve definition a second time");
        }
    auto state = fixture();
    state.finish.dungeon.facilities.at(1).updates = 9;
    state.details.at(1).notices = {{0, 43}, {2, 7}};
    const auto recycled = prepare_world_facility_update(state, 1, scripts);
    check(recycled.candidate && recycled.candidate->state.details.at(1).notices ==
                                    std::vector<std::array<int, 2>>{{0, 0}, {2, 7}},
          "old completion notice expires before source scan so fresh identity0 inserted at front");
    state.details.at(1).notices = {{2, 0}, {0, 23}};
    const auto duplicate = prepare_world_facility_update(state, 1, scripts);
    check(duplicate.candidate && duplicate.candidate->state.details.at(1).notices ==
                                     std::vector<std::array<int, 2>>{{2, 1}, {0, 23}},
          "any existing identity0 prevents duplicate even not front, no counter reset");
    state.definitions.at(35).flags = 256 | 512 | 1024 | 2048 | 4096 | 16384 | 32768;
    const auto flags = prepare_world_facility_update(state, 1, scripts);
    check(flags.candidate && flags.candidate->state.scripts.activities.at(7).status == 1 &&
              flags.candidate->state.scripts.activities.at(13).status == 1 &&
              flags.candidate->state.scripts.activities.at(21).status == 1 &&
              flags.candidate->state.scripts.activities.at(22).status == 1,
          "construction unlock script17 modifies actual activity definitions");
    for (int event : {88, 73, 74, 75, 76, 211, 212, 213, 214, 215})
        check(flags.candidate && world_script_seen(flags.candidate->state.scripts, event),
              "all source flag scripts invoked to actual return/wait, not ID queue");
    state.scripts.activities.erase(22);
    const auto late = prepare_world_facility_update(state, 1, scripts);
    check(late.error == WorldFacilityUpdateError::script_failed && !late.candidate &&
              state.finish.dungeon.world.facilities.at(1).status == 0 &&
              state.definitions.at(35).popularity_reward == 20 && state.scripts.event_calls.empty(),
          "late missing activity rolls back construction/popularity/previous real scripts");
}
void consumers(const WorldScriptCatalog &scripts) {
    for (int status : {0, 1, 2}) {
        auto state = fixture();
        state.finish.dungeon.world.facilities.at(1).status = status;
        state.finish.dungeon.world.facilities.at(1).occupants = {{1}};
        state.finish.dungeon.world.facilities.at(1).kind = 12;
        state.finish.dungeon.facilities.at(1).updates = 9;
        state.details.at(1).residence_mode = 1;
        state.details.at(1).resident_definition = 3;
        const auto missing = prepare_world_facility_update(state, 1, scripts);
        check(missing.error == WorldFacilityUpdateError::missing_consumer && !missing.candidate &&
                  state.finish.dungeon.facilities.at(1).updates == 9,
              "missing actual residence/crew/finish rejects private prefix rather than empty "
              "success");
        bool called{};
        const WorldFacilityUpdateConsumer external =
            [&](const auto &owner, const auto &request) -> std::optional<WorldFacilityUpdateState> {
            called = true; // 明确夹具，不冒充住宅创建或原探索提交。
            check(request.kind == (status == 0   ? WorldFacilityUpdateConsumerKind::residence_join
                                   : status == 1 ? WorldFacilityUpdateConsumerKind::dungeon_crew
                                                 : WorldFacilityUpdateConsumerKind::dungeon_finish),
                  "source status routes only its actual postprefix consumer");
            check(owner.finish.dungeon.facilities.at(1).updates == (status == 0 ? 0 : 10),
                  "consumer observes reset b1 or incremented prefix exactly once");
            return owner;
        };
        const auto observed = prepare_world_facility_update(state, 1, scripts, external);
        check(observed.candidate && called && observed.candidate->consumer_calls.size() == 1,
              "fixture confirms synchronous ordered consumer transaction");
    }
    auto state = fixture();
    state.finish.dungeon.facilities.at(1).updates = std::numeric_limits<int>::max();
    check(prepare_world_facility_update(state, 1, scripts).error ==
              WorldFacilityUpdateError::overflow,
          "signed f overflow is rejected, never wraps C++ UB");
}
} // namespace
int main() {
    try {
        const auto scripts = catalog();
        prefix(scripts);
        construction(scripts);
        consumers(scripts);
        std::cout << "facility update checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
