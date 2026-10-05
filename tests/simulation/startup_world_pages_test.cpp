#include "ark/simulation/startup_world_runtime.hpp"

#include <algorithm>
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
void task_focus_and_introduction() {
    for (int delay : {0, 1, 3, 90}) {
        auto waiting = fixture(16);
        const auto waiting_id = waiting.scripts.pages.back().id;
        waiting.scripts.pages.back().legacy_l = delay;
        for (int count = 1; count <= std::max(1, delay); ++count) {
            const auto tick = prepare_startup_world_runtime(waiting);
            check(tick.candidate && tick.candidate->page_counters.at(waiting_id) == count &&
                      (tick.candidate->scripts.pages.back().lifecycle == 4) ==
                          (count == std::max(1, delay)) &&
                      tick.candidate->scene.world.updates == 0 &&
                      tick.candidate->scene.calendar.units == 0 &&
                      tick.candidate->scene.random.draws() == 0,
                  "opcode7 raw16 honors exact page delay without world/date/random advance");
            waiting = *tick.candidate;
        }
    }
    auto s = fixture(57);
    const auto id = s.scripts.pages.back().id;
    auto result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->scripts.pages.back().lifecycle == 4,
          "raw57 empty task list automatically closes");
    s.task_order = {1};
    s.tasks.emplace(1, ref::DungeonFinishTask{1, 0, 0, 0, {}, ref::Position{1, 1}});
    result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->scripts.pages.back().lifecycle == 4 &&
              result.candidate->camera == s.camera,
          "raw57 unbound first task closes rather than aiming at task site");
    const auto facility = s.scene.world.facility_order.front();
    s.tasks.at(1).facility = facility;
    const auto target = startup_world_runtime_facility_target(s, facility);
    check(target.has_value(), "actual startup bound facility has source focus center");
    s.camera = {(*target)[0] - 5, (*target)[1]};
    s.previous_camera = s.camera;
    const auto units = s.scene.calendar.units;
    const auto draws = s.scene.random.draws();
    result = prepare_startup_world_runtime(s);
    check(result.candidate && result.candidate->camera == *target &&
              result.candidate->scripts.pages.back().lifecycle != 4,
          "raw57 distance equals step moves once before close");
    result = prepare_startup_world_runtime(*result.candidate);
    check(result.candidate && result.candidate->scripts.pages.back().lifecycle == 4 &&
              result.candidate->scene.calendar.units == units &&
              result.candidate->scene.random.draws() == draws &&
              result.candidate->scene.world.updates == 0,
          "raw57 reached focus closes automatically with world/date/random frozen");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::missing_source,
          "automatic focus cannot be deleted by manual confirm");
    s.tasks.at(1).facility = 99999;
    check(!prepare_startup_world_runtime(s).candidate && s.camera[0] == (*target)[0] - 5,
          "stale bound focus rejects without camera or page partial write");
    s = fixture(89);
    const auto intro = s.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(s, intro) == StartupWorldRuntimeError::none &&
              s.page_counters.at(intro) == 40 && s.scripts.pages.back().lifecycle != 4,
          "raw89 early confirm only fastforwards40");
    check(acknowledge_startup_world_runtime_page(s, intro) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 && s.scripts.event_calls.empty(),
          "raw89 next confirm closes without repeating monster introduction program");
}
void rank_conditions() {
    for (int rank = 0; rank < 5; ++rank) {
        auto s = fixture(49);
        const auto id = s.scripts.pages.back().id;
        s.rank = rank;
        s.rank_met.fill(true); // 旧显示缓存不是本次查询输入。
        s.rank_values.fill(-1);
        const auto cash = s.scene.world.world.ai.accounting.funds();
        const auto tick = prepare_startup_world_runtime(s);
        check(tick.candidate && tick.candidate->rank == rank &&
                  tick.candidate->page_counters.at(id) == 1 &&
                  tick.candidate->rank_values != s.rank_values,
              "raw49 initializes actual shared rank criteria without a monthly prompt");
        check(tick.candidate->scripts.event_calls == s.scripts.event_calls &&
                  tick.candidate->scene.calendar.units == s.scene.calendar.units &&
                  tick.candidate->scene.random.draws() == s.scene.random.draws() &&
                  tick.candidate->scene.world.updates == s.scene.world.updates,
              "rank display initialization neither triggers36 nor advances world/random/date");
        s = *tick.candidate;
        s.rank_met.fill(true); // 即使所有条件已满足，页49也不是页48的晋级入口。
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.rank == rank && (s.scripts.user_flags & 8) != 0 &&
                  s.scripts.pages.back().lifecycle == 4 &&
                  s.scene.world.world.ai.accounting.funds() == cash,
              "raw49 confirm marksu8 and closes only, no rank advance or cash mutation");
        check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::invalid_page,
              "rank page duplicate confirmation rejects");
        const auto resumed = prepare_startup_world_runtime(s);
        check(resumed.candidate &&
                  std::none_of(resumed.candidate->scripts.pages.begin(),
                               resumed.candidate->scripts.pages.end(),
                               [&](const auto &p) { return p.id == id; }) &&
                  resumed.candidate->simulation_steps == s.simulation_steps + 1,
              "rank page framework removal admits actual main script/world routing");
        auto running = *resumed.candidate;
        for (int frame = 0; frame < 10 && running.scene.world.updates == s.scene.world.updates;
             ++frame) {
            // 初局页夹具尚未执行事件7，先保留真实介绍/等待，不强令同次世界更新。
            const auto &top = running.scripts.pages.back();
            if (top.kind != ref::WorldScriptPageKind::scene && top.lifecycle != 4)
                check(acknowledge_startup_world_runtime_page(running, top.id) ==
                          StartupWorldRuntimeError::none,
                      "resumed actual opening dialogue is acknowledged normally");
            const auto next = prepare_startup_world_runtime(running);
            check(next.candidate.has_value(), "post-rank main continuation stays executable");
            running = *next.candidate;
        }
        check(running.scene.world.updates > s.scene.world.updates,
              "post-rank real script continuation eventually resumes world/date");
    }
    auto early = fixture(49);
    const auto id = early.scripts.pages.back().id;
    check(acknowledge_startup_world_runtime_page(early, id) == StartupWorldRuntimeError::none &&
              early.page_counters.at(id) == 0 && early.scripts.pages.back().lifecycle == 4,
          "early raw49 confirm cannot skip source initialization");
    auto max = fixture(49);
    const auto max_id = max.scripts.pages.back().id;
    max.rank = 5;
    max.rank_values = {1, 2, 3, 4};
    const auto result = prepare_startup_world_runtime(max);
    check(result.candidate && result.candidate->scripts.event_calls.at(48) == 1 &&
              result.candidate->rank == 5 && result.candidate->rank_values == max.rank_values &&
              (result.candidate->scripts.user_flags & 8) == 0,
          "rank5 initialization runs48 then closes without resetting old criteria or confirmu8");
    check(result.candidate->scripts.pages.front().lifecycle == 3 &&
              std::any_of(result.candidate->scripts.pages.begin(),
                          result.candidate->scripts.pages.end(),
                          [&](const auto &p) { return p.id == max_id && p.lifecycle == 4; }),
          "rank5 replacement talk preserves page anchor and marks only old rank page closed");
    auto malformed = fixture(49);
    const auto bad_id = malformed.scripts.pages.back().id;
    malformed.rank = -1;
    check(!prepare_startup_world_runtime(malformed).candidate &&
              acknowledge_startup_world_runtime_page(malformed, bad_id) ==
                  StartupWorldRuntimeError::missing_source &&
              malformed.page_counters.empty() && (malformed.scripts.user_flags & 8) == 0 &&
              malformed.scripts.pages.back().lifecycle != 4,
          "invalid rank rejects without page, criteria or flag partial writes");
}
void annual_termination() {
    auto s = fixture(87);
    const auto id = s.scripts.pages.back().id;
    s.human_presence.at(1) = 1;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->medal_count == 1 &&
              tick.candidate->sound_requests == std::vector<int>{3} &&
              tick.candidate->award_rankings.count(id) && tick.candidate->award_announced.at(id),
          "raw87 actual page initialization increments j once and first update sounds3 only");
    s = *tick.candidate;
    const auto repeated = prepare_startup_world_runtime(s);
    check(repeated.candidate && repeated.candidate->medal_count == 1 &&
              repeated.candidate->sound_requests == std::vector<int>{3},
          "raw87 later updates neither award another medal nor replay first sound");
    check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::missing_source &&
              act_startup_world_runtime_award_page(s, id,
                                                   ref::WorldAwardAction::confirm_termination) ==
                  StartupWorldRuntimeError::missing_source &&
              s.medal_count == 1,
          "ordinary confirm and absent termination question cannot silently end ceremony");
    check(act_startup_world_runtime_award_page(s, id, ref::WorldAwardAction::request_termination) ==
                  StartupWorldRuntimeError::none &&
              s.award_termination_pending.at(id),
          "explicit test player requests the source termination question");
    check(act_startup_world_runtime_award_page(s, id, ref::WorldAwardAction::reject_termination) ==
                  StartupWorldRuntimeError::none &&
              !s.award_termination_pending.at(id) && s.scripts.pages.back().lifecycle != 4,
          "termination no answer retains actual page and medals");
    check(act_startup_world_runtime_award_page(s, id, ref::WorldAwardAction::request_termination) ==
                  StartupWorldRuntimeError::none &&
              act_startup_world_runtime_award_page(s, id,
                                                   ref::WorldAwardAction::confirm_termination) ==
                  StartupWorldRuntimeError::none &&
              s.medal_count == 1 && s.scripts.event_calls.at(22) == 1 &&
              s.sound_requests.back() == 1 && s.scene.world.world.ai.accounting.funds() == cash &&
              s.scene.calendar.units == 0 && s.scene.random.draws() == 0,
          "termination preserves unused medal and cash/date/RNG, runs22 then source BGM refresh");
    check(std::any_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                      [&](const auto &p) { return p.id == id && p.lifecycle == 4; }) &&
              !s.scripts.executing_page,
          "termination closes only parent award page and releases callback root");
    auto malformed = fixture(87);
    const auto bad = malformed.scripts.pages.back().id;
    malformed.medal_count = std::numeric_limits<int>::max();
    check(!prepare_startup_world_runtime(malformed).candidate &&
              act_startup_world_runtime_award_page(malformed, bad,
                                                   ref::WorldAwardAction::request_termination) ==
                  StartupWorldRuntimeError::missing_source &&
              malformed.award_rankings.empty() && malformed.page_counters.empty(),
          "late award initialization error cannot leave partial medal/list/page state");
}
} // namespace
int main() {
    try {
        summary();
        gift();
        focus_and_pause();
        crew_initialization();
        rank_conditions();
        task_focus_and_introduction();
        annual_termination();
        std::cout << "startup world pages checks: " << checks << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
