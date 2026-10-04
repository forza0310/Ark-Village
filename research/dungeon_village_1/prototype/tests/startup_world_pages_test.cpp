#include "dungeon_village_prototype/startup_world_runtime.hpp"

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
// 明确页面夹具：初始世界与目录均真实，只有待测试的页面由此装入。
StartupWorldRuntimeState fixture(int raw) {
    StartupSession initial;
    StartupWorldRuntimeSession runtime(initial.state(), ref::WorldRandomStream::from_java_seed(1));
    auto state = runtime.state();
    state.scripts.pages.front().lifecycle = 3;
    ref::WorldScriptPage page;
    page.id = state.scripts.next_page_id++;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    state.scripts.pages.push_back(page);
    state.scripts.executing_page = page.id;
    return state;
}
void summary() {
    auto s = fixture(30);
    const auto id = s.scripts.pages.back().id;
    s.exploration_summaries[id] = {30, 1, 0, {}};
    const auto funds = s.scene.world.world.ai.accounting.funds();
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->page_counters.at(id) == 1 &&
              tick.candidate->sound_requests == std::vector<int>{4},
          "page30 first phase sound4");
    check(tick.candidate->scene.calendar.units == s.scene.calendar.units &&
              tick.candidate->scene.random.draws() == s.scene.random.draws() &&
              tick.candidate->scene.world.updates == s.scene.world.updates &&
              tick.checkpoints.empty(),
          "nonmain page freezes world/date/random/checkpoints");
    s = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 40 && s.page_phases[id] == 0,
          "early confirm skips40 only");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 0 && s.page_phases.at(id) == 1,
          "ready confirm enters phase1");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 40 && s.scripts.pages.back().lifecycle != 4,
          "second phase also requires independent fastforward");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.accounting.funds() == funds,
          "fourth immediate confirm closes without paying again");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "closed summary cannot be acknowledged twice");
    s = fixture(32);
    const auto missing = s.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(s, missing) ==
                  StartupWorldRuntimeError::missing_source &&
              s.scripts.pages.back().lifecycle == 2 && s.page_counters.empty(),
          "missing summary is rejected without partial mutation");
    s.exploration_summaries[missing] = {32, 1, 0, {{0, 1}}};
    check(acknowledge_startup_world_runtime_page(s, missing) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4,
          "page32 closes authenticated summary");
}
void gift() {
    auto s = fixture(94);
    auto &page = s.scripts.pages.back();
    const auto id = page.id;
    page.legacy_r = 3;
    page.legacy_s = 0;
    s.facility_presence.at(0) = 0;
    s.facility_free_builds[0] = 0;
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->sound_requests == std::vector<int>{5},
          "gift first update sound5 and no eager claim");
    s = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 40 && s.facility_free_builds.at(0) == 0,
          "gift first confirm only skips animation");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.facility_free_builds.at(0) == 1 && s.facility_presence.at(0) == 2 &&
              s.facility_unlock_notices.at(0) && s.scripts.pages.back().lifecycle == 4,
          "gift authentic claim writes unique catalog and closes");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page &&
              s.facility_free_builds.at(0) == 1,
          "gift duplicate confirm cannot grant twice");
}
void focus_and_pause() {
    auto s = fixture(56);
    s.scene.framework_paused = true;
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->page_counters.empty(),
          "framework pause freezes page too");
    s.scene.framework_paused = false;
    tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->scripts.pages.back().lifecycle == 4 &&
              tick.candidate->scene.random.draws() == 0 && tick.candidate->scene.world.updates == 0,
          "camera page without monster closes without fake world round");
    s = fixture(1234);
    const auto id = s.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::missing_source &&
              s.scripts.pages.back().lifecycle == 2 && s.page_counters.empty(),
          "unimplemented rawpage action explicitly rejects without mutation");
}
void crew_initialization() {
    auto s = fixture(31);
    const auto id = s.scripts.pages.back().id;
    s.scripts.pages.back().lifecycle = 0;
    s.scripts.pages.back().task_identity = 1;
    s.scripts.pages.back().task_definition = 0;
    s.tasks.emplace(1, ref::DungeonFinishTask{1, 0, 0, 0, {}, {}});
    s.participants = {1, 3}; // 明确成果页夹具，不宣称正常新局已接受该任务。
    s.task_progress.definitions.at(0).flags |= 2U;
    s.scene.world.world.ai.battle.humans.at(1).task_kills = 4;
    s.scene.world.world.ai.battle.humans.at(3).task_kills = 2;
    const auto result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->crew_summaries.at(id) == s.participants &&
              result.candidate->scene.world.world.ai.battle.humans.at(1).task_kills == 1 &&
              result.candidate->scene.world.world.ai.battle.humans.at(3).task_kills == 0 &&
              result.candidate->scripts.pages.back().lifecycle == 2,
          "frame page initialization performs original X/H once before update");
    const auto funds = s.scene.world.world.ai.accounting.funds();
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.crew_summaries.at(id) == s.participants && s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.accounting.funds() == funds,
          "early input cannot bypass initialization and never pays task reward again");
    auto malformed = fixture(31);
    const auto missing = malformed.scripts.pages.back().id;
    check(!prepare_startup_world_runtime(malformed).candidate &&
              acknowledge_startup_world_runtime_page(malformed, missing) ==
                  StartupWorldRuntimeError::missing_source &&
              malformed.crew_summaries.empty() && malformed.scripts.pages.back().lifecycle == 2,
          "missing task-bound raw31 payload cannot become fake close success");
}
} // namespace
int main() {
    try {
        summary();
        gift();
        focus_and_pause();
        crew_initialization();
        std::cout << "startup world pages checks: " << checks << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
