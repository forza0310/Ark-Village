#include "ark/simulation/startup_world_runtime_tasks.hpp"
#include "support/world_fixture.hpp"

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
// 专项期限调用点夹具：地图/目录/任务工厂真实，但活动任务、期限和页33由本测试明确安装。
// 不把该快照冒充自然任务到期，也不修改主线程自然长跑的输入。
StartupWorldRuntimeState fixture(int kind = 0) {
    auto s = test_support::world_fixture(ref::WorldRandomStream::from_java_seed(0));
    const auto created = ref::prepare_world_task_creation(startup_world_runtime_factory(s), kind);
    check(created.candidate && created.candidate->created_task &&
              write_startup_world_runtime_factory(s, created.candidate->state),
          "deadline fixture uses actual source task factory and map");
    const auto id = *created.candidate->created_task;
    s.active_task = id;
    s.scene.world.world.ai.task_active = true;
    s.participants = {0, 3, 0};
    s.task_subperiods = 12;
    s.human_calendar.at(0).continuation_cost = 100;
    s.human_calendar.at(3).continuation_cost = 300;
    s.human_flags.at(0) |= 2U;
    s.human_flags.at(3) |= 2U;
    s.task.kind = kind;
    s.task.center = *s.tasks.at(id).site;
    if (kind == 0) {
        const auto site = *s.tasks.at(id).facility;
        s.scene.world.world.facilities.at(site).status = 1;
        s.dungeon_facilities.at(site).extent = 100;
        s.dungeon_facilities.at(site).progress = 55;
    } else {
        ref::RewardEncounter event;
        event.runtime.id = 7;
        event.runtime.state = 3;
        event.runtime.center = s.task.center;
        event.runtime.spawned = 3;
        event.runtime.quota = 4;
        event.linked_monsters = 1;
        event.legacy_id = 3;
        s.scene.world.world.ai.encounters.emplace(7, event);
        s.scene.world.world.ai.encounter_order.push_back(7);
        s.task.encounter = 7;
    }
    for (auto &p : s.scripts.pages)
        if (p.kind == ref::WorldScriptPageKind::scene)
            p.lifecycle = 3;
    ref::WorldScriptPage page;
    page.id = s.scripts.next_page_id++;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 33;
    page.task_identity = id;
    page.task_definition = s.tasks.at(id).definition;
    page.legacy_f = 500;
    page.lifecycle = 2;
    s.scripts.pages.push_back(page);
    s.deadline_page = page.id;
    return s;
}
void page_and_return() {
    auto s = fixture();
    const auto id = *s.deadline_page;
    const auto funds = s.scene.world.world.ai.accounting.funds();
    const auto random = s.scene.random.draws();
    auto updated = update_startup_world_runtime_page(s);
    check(updated && updated->deadline_grades.at(id) == 2 && updated->page_counters.at(id) == 1 &&
              updated->scene.calendar.units == s.scene.calendar.units &&
              updated->scene.random.draws() == random,
          "page33 actual update initializes source progress grade without world or random ticks");
    s = *updated;
    const auto counter = s.page_counters.at(id);
    check(act_startup_world_runtime_task_page(s, id, StartupWorldTaskAction::cancel).error ==
                  StartupWorldRuntimeError::invalid_page &&
              s.page_counters.at(id) == counter,
          "raw33 has no Back cancellation");
    check(act_startup_world_runtime_task_page(s, id, StartupWorldTaskAction::confirm, 0).error ==
                  StartupWorldRuntimeError::none &&
              s.page_phases.at(id) == 1 && s.deadline_returns.at(id) == 0 &&
              s.page_counters.at(id) == 0 && s.scene.world.world.ai.accounting.funds() == funds &&
              s.task_subperiods == 12,
          "phase0 renew starts animation, preserving actual funds and deadline");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 50 && s.scripts.pages.back().lifecycle != 4,
          "phase1 generic confirmation fast forwards50 without instant close or settlement");
    updated = update_startup_world_runtime_page(s);
    check(updated && updated->scripts.pages.back().lifecycle == 4 && updated->deadline_page == id &&
              updated->scene.world.world.ai.accounting.funds() == funds,
          "actual page close preserves aI and cash until main entry");
    s = *updated;
    // 返回时读取当前an，而非旧页费用500或根据M重新计算；重复参与定义再次贡献费用。
    s.human_calendar.at(0).continuation_cost = 200;
    const auto settled = prepare_startup_world_runtime_deadline_result(s);
    check(settled.has_value(), "actual deadline return has a complete current owner candidate");
    check(settled && !settled->deadline_page && settled->task_subperiods == 0 &&
              settled->scene.world.world.ai.accounting.funds() == funds - 700 &&
              settled->active_task == s.active_task && settled->scene.random.draws() == random &&
              settled->scripts.notices.back().message == 25,
          "main return re-sums current fees with source repeats and charges once with notice25");
    const auto repeated = prepare_startup_world_runtime_deadline_result(*settled);
    check(repeated && repeated->scene.world.world.ai.accounting.funds() == funds - 700,
          "cleared aI prevents duplicate charge on later main entry");
    // 不绕过真正框架入口：该入口先删除closed页，aI只能读取栈外原对象payload。
    const auto main = prepare_startup_world_runtime(s);
    check(
        main.candidate && !main.candidate->deadline_page && !main.candidate->deadline_closed_page &&
            main.candidate->scene.world.world.ai.accounting.funds() == funds - 700 &&
            !main.candidate->deadline_returns.count(id) &&
            std::none_of(main.candidate->scripts.pages.begin(), main.candidate->scripts.pages.end(),
                         [&](const auto &p) { return p.id == id; }),
        "real framework removesclosed raw33 before main entry, settles saved aI once then "
        "retirespayload");
}
void denial_and_rollback() {
    auto s = fixture();
    const auto id = *s.deadline_page;
    s.scene.world.world.ai.accounting = ref::PeriodAccounting(499);
    const auto result =
        act_startup_world_runtime_task_page(s, id, StartupWorldTaskAction::confirm, 0);
    check(result.error == StartupWorldRuntimeError::none &&
              result.denial == ref::TaskCommandDenial::insufficient_funds &&
              s.page_phases.at(id) == 0 && ref::world_script_seen(s.scripts, 11) &&
              s.deadline_returns.at(id) == -1 && s.scene.world.world.ai.accounting.funds() == 499,
          "page funds denial consumes actual11 and keeps raw33 pending behind dialogue");
    auto bad = fixture();
    const auto bad_id = *bad.deadline_page;
    bad.scripts.page_mutations_locked = true;
    const auto before = bad.scene.random.draws();
    // 正常锁页初始化是源框架行为，不把缺初始化/返回身份放宽成main成功。
    bad.deadline_returns[bad_id] = 0;
    check(!prepare_startup_world_runtime_deadline_result(bad) && bad.deadline_page == bad_id &&
              bad.scene.random.draws() == before,
          "main result rejects still-open page without altering owner");
}
void abort_and_late_denial() {
    for (int kind : {0, 1})
        for (int choice : {0, 1}) {
            auto s = fixture(kind);
            const auto id = *s.deadline_page;
            const auto task = *s.active_task;
            const auto site = s.tasks.at(task).facility;
            const auto rng = s.scene.random.draws();
            const auto original_order = s.scene.world.facility_order;
            // choice0先在有钱时确认，页演出后资金不足；返回入口必须改走完整中止。
            check(
                act_startup_world_runtime_task_page(s, id, StartupWorldTaskAction::confirm, choice)
                        .error == StartupWorldRuntimeError::none,
                "both choices enter source deadline animation");
            check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none,
                  "abort fixture fast forwards through real page action");
            const auto close = update_startup_world_runtime_page(s);
            check(close.has_value(), "deadline animation closes normally");
            s = *close;
            s.scene.world.world.ai.accounting = ref::PeriodAccounting(499);
            s.scene.world.world.ai.pending_completion = 20;
            s.scripts.pending_completion = 20;
            const auto result = prepare_startup_world_runtime_deadline_result(s);
            check(result && !result->deadline_page && !result->active_task &&
                      result->task_order.empty() && result->tasks.count(task) &&
                      result->participants == std::vector<int>({0, 3, 0}) &&
                      result->scene.world.world.ai.accounting.funds() == 499 &&
                      result->scene.world.world.ai.pending_completion == 10 &&
                      result->popularity == s.popularity && result->scene.random.draws() == rng &&
                      ref::world_script_seen(result->scripts, 80) &&
                      ref::world_script_seen(result->scripts, 162) &&
                      result->scripts.notices.back().message == 26 &&
                      (choice != 0 || ref::world_script_seen(result->scripts, 11)),
                  "main abort performs80 delayed22 before pending-10, first162 and notice26; no "
                  "immediate popularity");
            check((result->human_flags.at(0) & 2U) == 0 && (result->human_flags.at(3) & 2U) == 0,
                  "n.o clears definition task flags but preserves participant list and retired "
                  "task reference");
            if (kind == 0) {
                check(site && !result->scene.world.world.facilities.count(*site) &&
                          result->scene.world.facility_order.size() + 1 == original_order.size() &&
                          !result->exploration_displays.empty() &&
                          result->scene.first_normal_refresh,
                      "exploration abort restores actual full site plus displays, roads and "
                      "neighbourhood");
                for (const auto cell : s.sites.at(*site).occupied_cells) {
                    const auto index = static_cast<std::size_t>(
                        cell.y * result->scene.world.world.map.width + cell.x);
                    check(result->scene.world.world.map.cells.at(index).legacy_state == 4 &&
                              !result->scene.world.world.map.cells.at(index).facility &&
                              result->surface.at(index).definition == result->ground_definition,
                          "every source occupied cell is restored to real ground definition");
                }
            } else {
                const auto &event = result->scene.world.world.ai.encounters.at(7);
                check(event.runtime.state == 1 && event.runtime.counter == 0 &&
                          result->sound_requests.back().id == 1 &&
                          result->sound_requests.back().operation == StartupAudioOperation::replace_bgm,
                      "battle abort retires actual encounter, rebuilds event map and restores "
                      "music1");
            }
        }
}
void actual_monster_abort() {
    auto s = fixture(1);
    ref::EncounterCreationInput input;
    input.kind = 0;
    input.center = {10, 18};
    input.year_index = 0;
    input.month_index = 3;
    const auto spawned = prepare_startup_world_runtime_encounter(s, input);
    check(spawned && spawned->scene.world.world.ai.monster_order.size() == 1,
          "deadline cancellation fixture creates a real source monster with canonical metadata");
    s = *spawned;
    const auto actor = s.scene.world.world.ai.monster_order.front();
    auto &ai = s.scene.world.world.ai;
    const auto encounter = *ai.battle.actors.at(actor).encounter;
    // 调用点夹具：将已真实生成的事件原ID设为本任务ID，验证n.o按f164b而非稳定db身份匹配。
    ai.encounters.at(encounter).legacy_id = ai.encounters.at(7).legacy_id;
    const auto original_position = ai.battle.actors.at(actor).position;
    const auto original_order = ai.monster_order;
    const auto draws = s.scene.random.draws();
    const auto page = *s.deadline_page;
    check(act_startup_world_runtime_task_page(s, page, StartupWorldTaskAction::confirm, 1).error ==
                  StartupWorldRuntimeError::none &&
              acknowledge_startup_world_runtime_page(s, page) == StartupWorldRuntimeError::none,
          "real monster cancellation enters and fast forwards genuine raw33");
    const auto closed = update_startup_world_runtime_page(s);
    check(closed.has_value(), "real monster deadline closes through actual update");
    s = *closed;
    const auto cancelled = prepare_startup_world_runtime_deadline_result(s);
    check(cancelled.has_value(), "actual monster abort returns complete source owner");
    const auto &m = cancelled->scene.world.world.ai.battle.actors.at(actor);
    check(m.control.state == 3 && m.control.action == 9 && m.state_parameter == 1 &&
              m.attack_position.x == original_position.x &&
              m.attack_position.height == original_position.height &&
              m.attack_position.z == original_position.z && m.state_counter == 0 &&
              cancelled->scene.world.world.ai.monster_order == original_order &&
              cancelled->scene.world.world.actors.count(actor) &&
              cancelled->actor_metadata.count(actor) && cancelled->scene.random.draws() == draws,
          "n.o source originalID alias cancels real monster without removing corpse or random "
          "draw; death lifecycle owns later removal");
}
void late_abort_rollback() {
    auto s = fixture();
    const auto page = *s.deadline_page;
    check(act_startup_world_runtime_task_page(s, page, StartupWorldTaskAction::confirm, 1).error ==
                  StartupWorldRuntimeError::none &&
              acknowledge_startup_world_runtime_page(s, page) == StartupWorldRuntimeError::none,
          "late rollback enters source abort animation");
    const auto closed = update_startup_world_runtime_page(s);
    check(closed.has_value(), "late rollback has authentic saved closed aI root");
    s = *closed;
    s.ground_definition = -1; // 显式坏投影，必须在已清占地之后的地图重建仍原子回滚。
    const auto task = *s.active_task;
    const auto facility = *s.tasks.at(task).facility;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    check(!prepare_startup_world_runtime_deadline_result(s) && s.active_task == task &&
              s.deadline_page == page && s.deadline_closed_page &&
              s.deadline_closed_page->id == page && s.deadline_returns.at(page) == 1 &&
              s.scene.world.world.facilities.count(facility) &&
              s.scene.world.world.ai.accounting.funds() == cash &&
              s.scene.random.draws() == draws && !ref::world_script_seen(s.scripts, 80),
          "late ground restoration failure preserves task, whole facility, aI root, cash, random "
          "and pending source script");
}
} // namespace
int main() {
    try {
        page_and_return();
        denial_and_rollback();
        abort_and_late_denial();
        actual_monster_abort();
        late_abort_rollback();
        std::cout << "startup_world_deadline: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "startup_world_deadline: " << e.what() << '\n';
        return 1;
    }
}
