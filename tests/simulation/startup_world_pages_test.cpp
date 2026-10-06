#include "ark/simulation/startup_world_human.hpp"
#include "ark/simulation/startup_world_runtime.hpp"
#include "ark/simulation/startup_world_tax.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
const auto fixture = test_support::page_fixture;
StartupWorldRuntimeState human_fixture(int raw) {
    auto s = fixture(raw);
    s.page_human_bindings[s.scripts.pages.back().id] = 1;
    // 管理页合同夹具：绕开首次说明的模态，仅避免它遮住被测栈顶。
    for (int event : {99, 111, 112, 119})
        s.scripts.event_calls[event] = 1;
    return s;
}
void page_tick(StartupWorldRuntimeState &s) {
    const auto r = prepare_startup_world_runtime(s);
    check(r.candidate.has_value(), "managed page actual framework update succeeds");
    s = *r.candidate;
}
void human_details_and_gifts() {
    using A = StartupHumanPageAction;
    using E = StartupWorldRuntimeError;
    auto s = human_fixture(60);
    const auto id = s.scripts.pages.back().id;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    const auto date = s.scene.calendar.units;
    const auto old = startup_world_human_details(s, 1);
    check(!startup_world_human_page_ready(s, id),
          "new human page read-only readiness does not construct a catalogue or advance state");
    check(old && old->equipment == s.shop_humans.at(1).equipment,
          "human query reads current sole definition and equipment, not spawn metadata");
    page_tick(s);
    check(startup_world_human_page_ready(s, id),
          "only successful framework initialization makes detail payload ready");
    for (int tab = 0; tab < 4; ++tab)
        check(act_startup_world_human_page(s, id, A::view_tab, tab) == E::none &&
                  s.page_phases.at(id) == tab,
              "all four detail tabs have explicit current page input");
    check(act_startup_world_human_page(s, id, A::view_tab, 4) == E::invalid_page &&
              s.scene.random.draws() == draws && s.scene.calendar.units == date &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "detail tabs and rejected tab do not advance world/date/random/cash");
    check(act_startup_world_human_page(s, id, A::gifts) == E::none,
          "actual human detail opens equipment gift catalogue");
    const auto gift = s.scripts.pages.back().id;
    page_tick(s);
    const auto &list = s.equipment_page_catalogs.at(gift)[0];
    check(!list.empty(), "real initial weapon catalogue is nonempty");
    const int weapon = list.front();
    s.catalog.at({1, weapon}).free_purchases = 1;
    s.catalog.at({1, weapon}).inventory = 7; // 两种库存刻意不同，防止接错ObjectCatalog字段。
    check(act_startup_world_human_page(s, gift, A::confirm) == E::none,
          "gift catalogue opens exact selected weapon confirmation");
    const auto question = s.scripts.pages.back().id;
    const auto satisfaction = s.shop_humans.at(1).satisfaction;
    for (int fault = 0; fault < 4; ++fault) {
        auto invalid = s;
        auto &parent_page =
            *std::find_if(invalid.scripts.pages.begin(), invalid.scripts.pages.end(),
                          [&](const auto &p) { return p.id == gift; });
        if (fault == 0)
            parent_page.lifecycle = 4;
        if (fault == 1)
            parent_page.legacy_page = 60;
        if (fault == 2)
            invalid.page_human_bindings.at(gift) = 2;
        if (fault == 3)
            invalid.human_equipment_choices.at(gift)[1] = -1;
        check(acknowledge_startup_world_runtime_page(invalid, question) == E::missing_source &&
                  invalid.human_page_answers.empty() &&
                  invalid.scene.world.world.ai.accounting.funds() == cash &&
                  invalid.catalog.at({1, weapon}).free_purchases == 1,
              "65 rejects retired, wrong-kind, different-human or mismatched-choice parent "
              "atomically");
    }
    check(acknowledge_startup_world_runtime_page(s, question) == E::none &&
              s.catalog.at({1, weapon}).free_purchases == 1 &&
              s.shop_humans.at(1).satisfaction == satisfaction &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "raw65 confirm only returns K0; no eager charge, inventory or reward");
    check(act_startup_world_human_page(s, gift, A::confirm) == E::invalid_page,
          "resumed parent answer cannot be bypassed by another input");
    auto broken = s;
    broken.scene.random =
        ref::WorldRandomStream::from_raw({}); // 晚期文案抽签无票，整个消费必须回滚。
    const auto result = prepare_startup_world_runtime(broken);
    check(!result.candidate && broken.catalog.at({1, weapon}).free_purchases == 1 &&
              broken.shop_humans.at(1).satisfaction == satisfaction &&
              broken.human_page_answers.at(gift) == 0,
          "late gift dialogue failure rolls back stock, reward and parent answer");
    page_tick(s);
    check(s.shop_humans.at(1).equipment[0] == weapon &&
              s.catalog.at({1, weapon}).free_purchases == 0 &&
              s.catalog.at({1, weapon}).inventory == 7 &&
              s.scene.world.world.ai.accounting.funds() == cash &&
              s.shop_humans.at(1).reselect[0] == 6 && s.scene.random.draws() == draws + 1,
          "resumed64 commits stock-only gift, final setter, cooldown6 and one draw2");
    check(acknowledge_startup_world_runtime_page(s, question) == E::invalid_page,
          "retired65 cannot grant twice");
    // 取消问答复用同一目录，不发礼物或随机；不另建一次性bug套件。
    s = human_fixture(64);
    const auto parent = s.scripts.pages.back().id;
    page_tick(s);
    check(act_startup_world_human_page(s, parent, A::confirm) == E::none,
          "cash-backed weapon quote opens65");
    const auto cancelled = s.scripts.pages.back().id;
    const auto before = s.scene.world.world.ai.accounting.funds();
    check(cancel_startup_world_runtime_page(s, cancelled) == E::none,
          "65 cancel returns K1 instead of deleting parent");
    page_tick(s);
    check(s.scene.world.world.ai.accounting.funds() == before && s.scene.random.draws() == 0 &&
              s.scripts.pages.back().id == parent,
          "resumed64 cancellation keeps cash/random/catalogue unchanged");
    const int selected_weapon = s.equipment_page_catalogs.at(parent)[0].front();
    s.catalog.at({1, selected_weapon}).newly_unlocked = true;
    check(act_startup_world_human_page(s, parent, A::inspect_equipment) == E::none &&
              !startup_world_human_page_ready(s, s.scripts.pages.back().id) &&
              s.catalog.at({1, selected_weapon}).newly_unlocked,
          "selected equipment has read-only raw73 details");
    page_tick(s);
    check(s.scripts.pages.back().legacy_page == 73 &&
              acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) == E::none &&
              s.scene.world.world.ai.accounting.funds() == before && s.scene.random.draws() == 0,
          "equipment details confirm only closes without gift side effects");
}
void human_profession_and_mastery() {
    using A = StartupHumanPageAction;
    using E = StartupWorldRuntimeError;
    auto s = human_fixture(61);
    const auto id = s.scripts.pages.back().id;
    const int old_job = s.scene.world.world.ai.growth.at(1).definition.current_profession;
    const auto original_job_counts = s.scripts.job_counts;
    // 最小缓存夹具：同定义的活跃/保留实例上限随共享重算，当前HP保持原值。
    ref::BattleActorRecord active;
    active.id = {501};
    active.definition = 1;
    active.capacity = s.scene.world.world.ai.growth.at(1).derived.combat[0];
    active.hp.target = active.hp.displayed = 9;
    s.scene.world.world.ai.battle.actors.emplace(active.id, active);
    auto retained = active;
    retained.id = {502};
    s.scene.world.world.ai.retired_actors.emplace(retained.id, retained);
    s.village_points = 999;
    s.scene.world.world.ai.growth.at(1).definition.legacy_u = 7; // 3点奖励跨十位，强制实际67接线。
    s.human_calendar.at(1).celebrations = 99;
    for (auto &job : s.scripts.professions)
        job.second.status = 1;
    page_tick(s);
    const auto &list = s.human_page_catalogs.at(id);
    const auto found =
        std::find_if(list.begin(), list.end(), [&](int job) { return job != old_job; });
    check(found != list.end(), "full actual profession directory has an alternative for sex");
    const int job = *found;
    check(act_startup_world_human_page(s, id, A::select, static_cast<int>(found - list.begin())) ==
                  E::none &&
              act_startup_world_human_page(s, id, A::confirm) == E::none,
          "actual61 accepts distinct affordable profession and opens62");
    const auto question = s.scripts.pages.back().id;
    const int cost = s.rules->jobs.at(job).change_points;
    const int effort = s.scene.world.world.ai.growth.at(1).definition.legacy_u;
    auto wrong_parent = s;
    wrong_parent.page_human_bindings.at(id) = 2;
    check(acknowledge_startup_world_runtime_page(wrong_parent, question) == E::missing_source &&
              wrong_parent.village_points == 999 &&
              wrong_parent.human_profession_changes.at(1).at(job) == 0 &&
              wrong_parent.human_page_answers.empty(),
          "62 cannot charge or reward through another human's profession parent");
    check(acknowledge_startup_world_runtime_page(s, question) == E::none &&
              s.village_points == 999 - cost && s.human_profession_changes.at(1).at(job) == 1 &&
              s.scene.world.world.ai.growth.at(1).definition.current_profession == old_job &&
              s.scene.world.world.ai.growth.at(1).definition.legacy_u == effort + 3,
          "62 pays points, rewards old profession, increments R but does not switch t yet");
    const auto animation =
        std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                     [](const auto &p) { return p.lifecycle != 4 && p.legacy_page == 63; });
    check(animation != s.scripts.pages.end(), "confirmation queues real63 animation");
    const auto aid = animation->id;
    page_tick(s);
    check(s.human_pages_initialized.count(aid) && s.page_counters.at(aid) == 0 &&
              s.scene.random.draws() == 1 && s.scripts.pages.back().legacy_page == 67,
          "framework initializes hidden63 before updating result67; draw5 does not wait for focus");
    // 领域67已单独覆盖；这里只按合法确认消掉它，保留63的真实计数时点。
    for (int n = 0; n < 120 && s.scripts.pages.back().id != aid; ++n) {
        page_tick(s);
        if (s.scripts.pages.back().legacy_page == 67)
            check(acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) == E::none,
                  "effort result drains before profession animation");
    }
    check(s.scripts.pages.back().id == aid, "63 becomes actual top after reward result");
    for (int n = s.page_counters[aid]; n < 54; ++n)
        page_tick(s);
    check(s.scene.world.world.ai.growth.at(1).definition.current_profession == old_job,
          "frame54 retains original profession");
    const auto cache = s.scene.world.world.ai.growth.at(1).derived.attributes;
    page_tick(s);
    check(s.page_counters.at(aid) == 55 &&
              s.scene.world.world.ai.growth.at(1).definition.current_profession == job &&
              s.scene.world.world.ai.growth.at(1).derived.attributes == cache &&
              s.scene.random.draws() == 1,
          "exact55 switches shared t only; no eager recalculation or repeated dialogue draw");
    s.scene.world.world.ai.growth.at(1).experience = 9;
    s.scene.world.world.ai.growth.at(1).pending = {4, 8};
    check(acknowledge_startup_world_runtime_page(s, aid) == E::none &&
              s.scene.world.world.ai.growth.at(1).experience == 9,
          "early63 confirm does not clear current experience");
    while (s.page_counters.at(aid) < 197)
        page_tick(s);
    check(acknowledge_startup_world_runtime_page(s, aid) == E::none &&
              s.scene.world.world.ai.growth.at(1).experience == 0 &&
              s.scene.world.world.ai.growth.at(1).pending.amount == 0 &&
              s.scene.world.world.ai.growth.at(1).pending.counter == 8 &&
              s.scene.random.draws() == 1,
          "197 confirm recomputes and closes, clears L/N only, preserves O and random");
    const int changed_capacity = s.scene.world.world.ai.growth.at(1).derived.combat[0];
    check(s.scene.world.world.ai.battle.actors.at(active.id).capacity == changed_capacity &&
              s.scene.world.world.ai.retired_actors.at(retained.id).capacity == changed_capacity &&
              s.scene.world.world.ai.battle.actors.at(active.id).hp.target == 9 &&
              s.scene.world.world.ai.retired_actors.at(retained.id).hp.displayed == 9,
          "197 refresh propagates shared maximum HP to live and retained actors without healing");
    const auto old_type = s.rules->jobs.at(old_job).type;
    const auto new_type = s.rules->jobs.at(job).type;
    // a/h.d()统计p!=0定义；新局另一名已开放人物仍贡献旧职业人数。
    check(s.scripts.job_counts.at(new_type) ==
                  original_job_counts.at(new_type) + (old_type == new_type ? 0 : 1) &&
              s.scripts.job_counts.at(old_type) ==
                  original_job_counts.at(old_type) - (old_type == new_type ? 0 : 1),
          "final profession refresh synchronizes shared h.C read by later route and construction");
    page_tick(s);
    check(s.scripts.pages.back().lifecycle == 4 && s.scripts.pages.back().id == id &&
              !s.human_page_answers.count(id),
          "61 closes only when its update consumes returned K0");
    const auto usage = startup_world_resource_usage(s);
    check(usage.pages <= 2 && usage.page_payloads < 12,
          "closed child payloads retire instead of accumulating for detail chain");
    s = human_fixture(70);
    const auto mastery = s.scripts.pages.back().id;
    const auto extra = s.scene.world.world.ai.growth.at(1).definition.extra;
    for (int phase = 0; phase < 2; ++phase) {
        check(acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
                  s.page_counters.at(mastery) == 40 &&
                  s.scene.world.world.ai.growth.at(1).definition.extra == extra,
              "mastery phase early confirmation fastforwards only");
        check(acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
                  s.page_phases.at(mastery) == phase + 1,
              "mastery independently enters each following phase");
    }
    check(act_startup_world_human_page(s, mastery, A::select, 1) == E::none &&
              acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
              s.scene.world.world.ai.growth.at(1).definition.extra == extra,
          "final choice still waits its own40 before granting");
    check(acknowledge_startup_world_runtime_page(s, mastery) == E::none &&
              s.scripts.pages.back().lifecycle == 4 &&
              acknowledge_startup_world_runtime_page(s, mastery) == E::invalid_page,
          "mastery final return grants once and stale duplicate rejects");
}
void tax_pages() {
    using E = StartupWorldRuntimeError;
    auto s = fixture(90);
    const auto id = s.scripts.pages.back().id;
    for (int n = 0; n < 6; ++n) {
        s.human_presence.at(n) = 1;
        s.human_homes.at(n)[2] = 1;
        s.human_calendar.at(n).legacy_G = (n + 1) * 10;
        s.scene.world.world.ai.battle.humans.at(n).battle_reward_stat = 100 + n;
    }
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto draws = s.scene.random.draws();
    page_tick(s);
    check(s.tax_page_residents.at(id) == std::vector<int>({0, 1, 2, 3, 4, 5}) &&
              act_startup_world_tax_page(s, id, StartupWorldTaxAction::select, 5) == E::none &&
              s.tax_page_scroll.at(id) == 1,
          "90 freezes source residents and keeps selected row in five-row viewport");
    s.human_calendar.at(5).legacy_G = 90;
    s.human_homes.at(5)[2] = 0;
    const auto view = inspect_startup_world_tax_page(s, id);
    check(view && view->rows.size() == 6 && view->rows.back().amount == 90 && view->total == 240,
          "90 frozen identities still read current G without refreshing eligibility");
    check(cancel_startup_world_runtime_page(s, id) == E::invalid_page &&
              acknowledge_startup_world_runtime_page(s, id) == E::none &&
              s.scene.world.world.ai.accounting.funds() == cash,
          "tax list has no cancellation and confirmation never pays");
    s = fixture(98);
    const auto payment = s.scripts.pages.back().id;
    s.human_presence.at(1) = 1;
    s.human_homes.at(1)[2] = 1;
    s.human_calendar.at(1).legacy_G = 140;
    s.human_calendar.at(2).legacy_G = 90;
    s.scene.world.world.ai.battle.humans.at(2).battle_reward_stat = 700;
    check(acknowledge_startup_world_runtime_page(s, payment) == E::invalid_page,
          "automatic98 cannot be executed by manual confirm");
    page_tick(s);
    check(s.scene.world.world.ai.accounting.funds() == cash + 140 &&
              s.monthly_cash.at(s.scene.calendar.month)[4][0] == 140 &&
              s.human_calendar.at(2).legacy_G == 0 &&
              s.scene.world.world.ai.battle.humans.at(2).battle_reward_stat == 0 &&
              s.scene.random.draws() == draws && s.scripts.pages.back().lifecycle == 4,
          "98 pays current eligible residents once to other income, clears every F/G and no draw");
    check(s.scripts.notices.back().message == 28, "automatic tax receipt submits source notice28");
    auto broken = fixture(98);
    broken.human_presence.at(1) = 1;
    broken.human_homes.at(1)[2] = 1;
    broken.human_calendar.at(1).legacy_G = 10;
    broken.monthly_cash.at(broken.scene.calendar.month)[4][0] = std::numeric_limits<int>::max();
    check(!prepare_startup_world_runtime(broken).candidate &&
              broken.human_calendar.at(1).legacy_G == 10 &&
              broken.scene.world.world.ai.accounting.funds() == cash &&
              broken.page_counters.empty(),
          "tax ledger overflow rolls back cash, F/G, counter and close");
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
    tick = prepare_startup_world_runtime(s);
    check(tick.candidate && !tick.candidate->exploration_summaries.count(id) &&
              !s.exploration_summaries.empty(),
          "framework retires closed result payload only on successful next Owner commit");
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
void rank_promotion() {
    auto s = fixture(48);
    const auto id = s.scripts.pages.back().id;
    s.scripts.pages.back().legacy_f = 1;
    auto initialized = prepare_startup_world_runtime(s);
    check(initialized.candidate && initialized.candidate->rank == 0,
          "promotion page initialization only refreshes source criteria");
    s = *initialized.candidate;
    const auto draws = s.scene.random.draws();
    const auto cash = s.scene.world.world.ai.accounting.funds();
    check(act_startup_world_runtime_rank_page(s, id, 1) == StartupWorldRuntimeError::none &&
              s.rank == 0 && (s.scripts.user_flags & 8) == 0 && s.scripts.event_calls.at(159) == 1,
          "condition explanation does not promote or marku8");
    // 条件缓存/页面调用点夹具，不伪造自然新局已满足晋级条件。
    auto ready = fixture(48);
    const auto ready_id = ready.scripts.pages.back().id;
    ready.scripts.pages.back().legacy_f = 1;
    ready.page_counters[ready_id] = 1;
    ready.rank_met.fill(true);
    ready.scene.calendar.year = 2;
    ready.scene.calendar.month = 7;
    check(act_startup_world_runtime_rank_page(ready, ready_id) == StartupWorldRuntimeError::none &&
              ready.rank == 1 && ready.rank_history[0] == std::array<int, 2>{2, 7} &&
              (ready.scripts.user_flags & 8) != 0 && ready.scripts.event_calls.at(37) == 1 &&
              ready.scripts.event_calls.at(53) == 1 && ready.scripts.event_calls.at(42) == 1 &&
              ready.scripts.event_calls.at(41) == 1 && ready.scripts.event_calls.at(206) == 1,
          "actual promotion commits rank/date and manual/facility/title/tutorial script chain");
    const auto closed = std::find_if(ready.scripts.pages.begin(), ready.scripts.pages.end(),
                                     [&](const auto &p) { return p.id == ready_id; });
    check(closed != ready.scripts.pages.end() && closed->lifecycle == 4 &&
              ready.scene.world.world.ai.accounting.funds() == cash &&
              ready.scene.random.draws() == draws && ready.scene.world.updates == 0,
          "promotion closes only source page, no invented cost/world update/shuffle before raw50 "
          "initialization");
    check(std::any_of(ready.scripts.pages.begin(), ready.scripts.pages.end(),
                      [](const auto &p) { return p.legacy_page == 50; }) &&
              std::any_of(ready.scripts.notices.begin(), ready.scripts.notices.end(),
                          [](const auto &n) { return n.message == 18; }) &&
              std::any_of(ready.scripts.notices.begin(), ready.scripts.notices.end(),
                          [](const auto &n) { return n.message == 6; }),
          "rank1 creates actual celebration and definition/activity notices");
    for (const auto &activity : ready.rules->activities)
        if (activity.parameters[6] == 1)
            check(ready.scripts.activities.at(activity.identity).status == 1 &&
                      ready.scripts.activities.at(activity.identity).pending_notice,
                  "matched original activity j opens shared definition and retains new notice");
    check(act_startup_world_runtime_rank_page(ready, ready_id) ==
                  StartupWorldRuntimeError::invalid_page &&
              ready.rank == 1,
          "closed promotion cannot advance rank twice");
    auto denied = fixture(48);
    const auto denied_id = denied.scripts.pages.back().id;
    denied.page_counters[denied_id] = 1;
    denied.rank_met = {true, true, true, false};
    check(act_startup_world_runtime_rank_page(denied, denied_id) ==
                  StartupWorldRuntimeError::none &&
              denied.rank == 0 && denied.scripts.event_calls.at(39) == 1 &&
              denied.scripts.pages[1].lifecycle != 4,
          "three-condition refusal preserves rank and source page, invokes39");
    auto broken = fixture(48);
    const auto bad_id = broken.scripts.pages.back().id;
    broken.page_counters[bad_id] = 1;
    broken.rank_met.fill(true);
    broken.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
    check(
        act_startup_world_runtime_rank_page(broken, bad_id) ==
                StartupWorldRuntimeError::script_failed &&
            broken.rank == 0 && broken.rank_history[0] == std::array<int, 2>{0, 0} &&
            broken.scripts.notices.empty() && broken.scripts.event_calls.empty() &&
            (broken.scripts.user_flags & 8) == 0 && broken.scene.random.draws() == 0,
        "late page-ID exhaustion rolls back rank/history/unlocks/notices/userflag/scripts/random");
    auto celebration = fixture(50);
    const auto celebration_id = celebration.scripts.pages.back().id;
    celebration.human_presence.at(1) = 1;
    celebration.human_presence.at(2) = 0; // 原表2同为p1；本条件夹具只保留一名入场者。
    celebration.human_presence.at(3) = 2; // p2不入庆典，不按p!=0扩宽。
    celebration.scene.random = ref::WorldRandomStream::from_raw({0});
    auto tick = prepare_startup_world_runtime(celebration);
    check(tick.candidate &&
              tick.candidate->rank_celebration_participants.at(celebration_id) ==
                  std::vector<std::array<int, 5>>{{1, 287, 143, 3, 0}} &&
              tick.candidate->scene.random.draws() == 1 &&
              tick.candidate->sound_requests == std::vector<int>{3},
          "celebration initializes current p1 definitions, one shuffle draw and first music3");
    celebration = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(celebration, celebration_id) ==
                  StartupWorldRuntimeError::none &&
              celebration.page_counters.at(celebration_id) == 1 &&
              celebration.page_phases.at(celebration_id) == 0,
          "early celebration confirm does not fastforward or close");
    for (const auto threshold : {135, 20}) {
        celebration.page_counters[celebration_id] = threshold - 1;
        tick = prepare_startup_world_runtime(celebration);
        check(tick.candidate && tick.candidate->page_counters.at(celebration_id) == 0,
              "source phase threshold automatically transitions and resets local counter");
        celebration = *tick.candidate;
    }
    celebration.page_counters[celebration_id] = 139;
    check(acknowledge_startup_world_runtime_page(celebration, celebration_id) ==
                  StartupWorldRuntimeError::none &&
              celebration.scripts.pages.back().lifecycle != 4,
          "last celebration phase139 cannot close before140");
    tick = prepare_startup_world_runtime(celebration);
    check(tick.candidate.has_value(), "last celebration threshold update valid");
    celebration = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(celebration, celebration_id) ==
                  StartupWorldRuntimeError::none &&
              celebration.scripts.pages.back().lifecycle == 4 &&
              celebration.sound_requests.back() == 1 && celebration.scene.world.updates == 0 &&
              celebration.scene.random.draws() == 1,
          "final confirm refreshes original music and closes, no repeat shuffle/world update");
    auto exhausted = fixture(50);
    exhausted.human_presence.at(1) = 1;
    exhausted.scene.random = ref::WorldRandomStream::from_raw({});
    check(!prepare_startup_world_runtime(exhausted).candidate &&
              exhausted.rank_celebration_participants.empty() && exhausted.page_counters.empty(),
          "celebration random exhaustion fails whole Owner without synthetic participants");
}
void annual_termination() {
    auto s = fixture(87);
    const auto id = s.scripts.pages.back().id;
    s.human_presence.at(1) = 1;
    const auto issued = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                  startup_world_runtime_scripts(s), {108, {}, {}});
    check(issued.candidate && write_startup_world_runtime_scripts(s, issued.candidate->state) &&
              s.medal_count == 1 && s.scripts.medal_count == 0,
          "real opcode29 commits medal to sole Owner, not persistent script duplicate");
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->medal_count == 2 &&
              tick.candidate->sound_requests == std::vector<int>{3} &&
              tick.candidate->award_rankings.count(id) && tick.candidate->award_announced.at(id),
          "raw87 actual page initialization increments j once and first update sounds3 only");
    s = *tick.candidate;
    const auto repeated = prepare_startup_world_runtime(s);
    check(repeated.candidate && repeated.candidate->medal_count == 2 &&
              repeated.candidate->sound_requests == std::vector<int>{3},
          "raw87 later updates neither award another medal nor replay first sound");
    check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::missing_source &&
              act_startup_world_runtime_award_page(s, id,
                                                   ref::WorldAwardAction::confirm_termination) ==
                  StartupWorldRuntimeError::missing_source &&
              s.medal_count == 2,
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
              s.medal_count == 2 && s.scripts.event_calls.at(22) == 1 &&
              s.sound_requests.back() == 1 && s.scene.world.world.ai.accounting.funds() == cash &&
              s.scene.calendar.units == 0 && s.scene.random.draws() == 0,
          "termination preserves unused medal and cash/date/RNG, runs22 then source BGM refresh");
    check(std::any_of(s.scripts.pages.begin(), s.scripts.pages.end(),
                      [&](const auto &p) { return p.id == id && p.lifecycle == 4; }) &&
              !s.scripts.executing_page,
          "termination closes only parent award page and releases callback root");
    const auto reissued = ref::prepare_world_script(
        startup_world_runtime_catalog(), startup_world_runtime_scripts(s), {108, {}, {}});
    check(reissued.candidate && reissued.candidate->state.medal_count == 3 &&
              write_startup_world_runtime_scripts(s, reissued.candidate->state) &&
              s.medal_count == 3 && s.scripts.medal_count == 0,
          "script after ceremony reads actual retained medals and increments same field");
    auto malformed = fixture(87);
    const auto bad = malformed.scripts.pages.back().id;
    malformed.medal_count = std::numeric_limits<int>::max();
    const auto overflow = ref::prepare_world_script(
        startup_world_runtime_catalog(), startup_world_runtime_scripts(malformed), {108, {}, {}});
    check(!overflow.candidate && malformed.medal_count == std::numeric_limits<int>::max() &&
              malformed.scripts.medal_count == 0 && malformed.scripts.notices.empty(),
          "real medal overflow returns no partial shared field or notification");
    check(!prepare_startup_world_runtime(malformed).candidate &&
              act_startup_world_runtime_award_page(malformed, bad,
                                                   ref::WorldAwardAction::request_termination) ==
                  StartupWorldRuntimeError::missing_source &&
              malformed.award_rankings.empty() && malformed.page_counters.empty(),
          "late award initialization error cannot leave partial medal/list/page state");
}
void annual_award() {
    auto s = fixture(87);
    s.human_presence.at(1) = 1;
    const auto parent = s.scripts.pages.back().id;
    const auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate.has_value(), "actual annual page initialized");
    s = *tick.candidate;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto old_u = s.scene.world.world.ai.growth.at(1).definition.legacy_u;
    const auto old_C = s.shop_humans.at(1).satisfaction;
    const auto completion = s.scene.world.world.ai.pending_completion;
    check(act_startup_world_runtime_award_page(s, parent, ref::WorldAwardAction::request_award) ==
                  StartupWorldRuntimeError::none &&
              s.award_pending_humans.at(parent) == 1 && s.medal_count == 1,
          "request binds definition and preserves unique medal");
    auto overflow = s;
    overflow.human_calendar.at(1).celebrations = std::numeric_limits<int>::max();
    const auto counter = overflow.page_counters.at(parent);
    check(act_startup_world_runtime_award_page(overflow, parent,
                                               ref::WorldAwardAction::confirm_award) ==
                  StartupWorldRuntimeError::missing_source &&
              overflow.medal_count == 1 && overflow.page_counters.at(parent) == counter &&
              overflow.scripts.pages.size() == s.scripts.pages.size() &&
              overflow.award_pending_humans.at(parent) == 1,
          "late reward overflow rolls back medal, pending question, counter and page stack");
    check(act_startup_world_runtime_award_page(s, parent, ref::WorldAwardAction::confirm_award) ==
                  StartupWorldRuntimeError::none &&
              s.medal_count == 0 && s.human_calendar.at(1).celebrations == 1 &&
              s.scripts.event_calls.at(58) == 1 &&
              s.scene.world.world.ai.pending_completion == completion + 10 &&
              s.shop_humans.at(1).satisfaction == std::min(old_C + 10, 100) &&
              s.scene.world.world.ai.growth.at(1).definition.legacy_u ==
                  std::min(old_u + 10, 100) &&
              s.page_counters.at(parent) == 0 && s.award_announced.at(parent),
          "yes commits reward and event58 once, preserves announced flag at parent counter0");
    check(s.scripts.pages.back().legacy_page == 88 &&
              s.page_human_bindings.at(s.scripts.pages.back().id) == 1,
          "framework inserts each child after same callback anchor: raw88 above later67");
    const auto display = s.scripts.pages.back().id;
    s.page_phases[display] = 1;
    s.page_counters[display] = 212;
    s.scene.random = ref::WorldRandomStream::from_raw({1});
    auto exhausted = s;
    exhausted.scene.random = ref::WorldRandomStream::from_raw({});
    check(acknowledge_startup_world_runtime_page(exhausted, display) ==
                  StartupWorldRuntimeError::missing_source &&
              exhausted.scripts.pages.back().id == display &&
              exhausted.scripts.pages.back().lifecycle != 4 && exhausted.scene.random.draws() == 0,
          "late display random failure keeps source page and global random unchanged");
    check(
        acknowledge_startup_world_runtime_page(s, display) == StartupWorldRuntimeError::none &&
            s.scene.random.draws() == 1 && s.scripts.event_calls.at(25) == 1 &&
            s.scripts.pages.back().speaker_kind == 1 &&
            s.scripts.pages.back().speaker_definition == 1 &&
            s.scene.world.world.ai.accounting.funds() == cash && s.scene.calendar.units == 0 &&
            s.scene.world.world.ai.battle.actors.empty(),
        "raw88 selects actual event25 on sole RNG and binds last page without world/cash advance");
    for (int n = 0; n < 100 && !ref::world_script_seen(s.scripts, 22); ++n) {
        const auto next = prepare_startup_world_runtime(s);
        check(next.candidate.has_value(), "award return chain keeps valid Owner");
        s = *next.candidate;
        if (s.scripts.pages.back().kind == ref::WorldScriptPageKind::dialogue)
            check(acknowledge_startup_world_runtime_page(s, s.scripts.pages.back().id) ==
                      StartupWorldRuntimeError::none,
                  "actual award dialogue returns");
        else if (s.scripts.pages.back().legacy_page == 67) {
            const auto effort = s.scripts.pages.back().id;
            s.page_counters[effort] = 73;
            check(acknowledge_startup_world_runtime_page(s, effort) ==
                      StartupWorldRuntimeError::none,
                  "effort page returns after original award dialogue at threshold73");
        }
    }
    check(ref::world_script_seen(s.scripts, 22) && s.medal_count == 0 &&
              s.human_calendar.at(1).celebrations == 1,
          "zero medals close ceremony after child return, never repeat reward");
}
void task_display() {
    for (int raw : {99, 100}) {
        auto s = fixture(raw);
        const auto id = s.scripts.pages.back().id;
        auto missing = prepare_startup_world_runtime(s);
        check(
            !missing.candidate && s.scene.random.draws() == 0,
            "task display requires actual bound monster definition; page number cannot invent it");
        s.scripts.pages.back().monster_definition = 0;
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.page_counters.at(id) == 0 && s.scripts.pages.back().lifecycle != 4 &&
                  s.scene.random.draws() == (raw == 100 ? 19U : 0U),
              "early confirm initializes true display once but never skips forty counter");
        const auto initial_draws = s.scene.random.draws();
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.scene.random.draws() == initial_draws,
              "repeated early confirmation does not reinitialize or run same-frame F");
        for (int count = 1; count <= 40; ++count) {
            const auto tick = prepare_startup_world_runtime(s);
            check(tick.candidate && tick.candidate->page_counters.at(id) == count &&
                      tick.candidate->scene.calendar.units == 0 &&
                      tick.candidate->scene.world.updates == 0,
                  "presentation page advances separately while shared world and calendar stay "
                  "frozen");
            s = *tick.candidate;
        }
        const auto draws = s.scene.random.draws();
        check((raw == 99 ? draws == 0 : draws > initial_draws) &&
                  acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.scripts.pages.back().lifecycle == 4 && s.scene.random.draws() == draws,
              "ready confirm closes true display without extra presentation random");
    }
}
void event_message_and_shop_return() {
    for (int command : {0, 1, 2}) {
        auto s = fixture(11);
        const auto id = s.scripts.pages.back().id;
        s.scripts.pages.back().message_commands = {command};
        s.scripts.pages.back().paragraphs = {"调用点夹具"};
        const auto funds = s.scene.world.world.ai.accounting.funds();
        const auto tick = prepare_startup_world_runtime(s);
        check(
            tick.candidate && tick.candidate->page_counters.at(id) == 1 &&
                tick.candidate->sound_requests ==
                    (command == 2 ? std::vector<int>{} : std::vector<int>{command == 0 ? 4 : 6}) &&
                tick.candidate->scene.world.updates == 0 &&
                tick.candidate->scene.random.draws() == 0,
            "raw11 source sound only on first page update, no world/random tick");
        s = *tick.candidate;
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.page_counters.at(id) == 40 && s.scripts.pages.back().lifecycle != 4,
              "raw11 early confirm only fast-forwards to40");
        auto locked = s;
        locked.scripts.page_mutations_locked = true;
        // kairo/android/a/b.c(a)无l守卫；l限制新增页，不限制关闭已有页。
        check(acknowledge_startup_world_runtime_page(locked, id) ==
                      StartupWorldRuntimeError::none &&
                  locked.scripts.pages.back().lifecycle == 4 &&
                  locked.scene.world.world.ai.accounting.funds() == funds,
              "source page creation lock does not prevent raw11 existing-page close");
        check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                  s.scripts.pages.back().lifecycle == 4 &&
                  s.scene.world.world.ai.accounting.funds() == funds,
              "raw11 second confirm only closes, no extra reward/payment");
    }
    auto invalid = fixture(11);
    const auto invalid_id = invalid.scripts.pages.back().id;
    check(!prepare_startup_world_runtime(invalid).candidate &&
              acknowledge_startup_world_runtime_page(invalid, invalid_id) ==
                  StartupWorldRuntimeError::missing_source &&
              invalid.page_counters.empty(),
          "raw11 missing authenticated message payload rejects without partial update");
    auto s = fixture(83);
    const auto id = s.scripts.pages.back().id;
    const auto funds = s.scene.world.world.ai.accounting.funds();
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::missing_source,
          "shop confirm does not silently mean cancel or fabricate purchase");
    auto locked = s;
    locked.scripts.page_mutations_locked = true;
    check(cancel_startup_world_runtime_page(locked, id) == StartupWorldRuntimeError::none &&
              locked.scripts.pages.back().lifecycle == 4,
          "source page creation lock does not prevent raw83 existing-page cancel");
    check(cancel_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().lifecycle == 4 && s.scripts.pages.front().lifecycle != 4 &&
              s.scene.random.draws() == 0 && s.scene.world.world.ai.accounting.funds() == funds,
          "explicit shop cancel preserves lower scene, money and random");
    check(cancel_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "retired shop cannot be cancelled twice");
    auto unknown = fixture(1234);
    check(cancel_startup_world_runtime_page(unknown, unknown.scripts.pages.back().id) ==
                  StartupWorldRuntimeError::invalid_page &&
              unknown.scripts.pages.back().lifecycle != 4,
          "cancel only supports proven raw83 source route");
}
void popularity_return() {
    auto s = fixture(97);
    const auto id = s.scripts.pages.back().id;
    s.scripts.pages.back().lifecycle = 1;
    s.popularity_display = true;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto rewards = s.popularity_rewards;
    check(acknowledge_startup_world_runtime_page(s, id) ==
                  StartupWorldRuntimeError::missing_source &&
              s.popularity_display && s.scripts.pages.back().lifecycle == 1,
          "raw97 early player input cannot bypass actual framework update");
    check(!update_startup_world_runtime_page(s),
          "raw97 direct consumer preserves lifecycle2 requirement");
    s.scene.framework_paused = true;
    auto tick = prepare_startup_world_runtime(s);
    check(tick.candidate && tick.candidate->popularity_display &&
              tick.candidate->scripts.pages.back().lifecycle != 4,
          "raw97 framework pause does not clear reward display");
    s.scene.framework_paused = false;
    tick = prepare_startup_world_runtime(s);
    check(tick.candidate && !tick.candidate->popularity_display &&
              tick.candidate->scripts.pages.back().lifecycle == 4,
          "raw97 actual first update automatically clears R and closes");
    check(tick.candidate->scene.calendar.units == s.scene.calendar.units &&
              tick.candidate->scene.world.updates == s.scene.world.updates &&
              tick.candidate->scene.random.draws() == s.scene.random.draws() &&
              tick.candidate->scene.world.world.ai.accounting.funds() == cash &&
              tick.candidate->sound_requests.empty() && tick.checkpoints.empty(),
          "raw97 does not advance world or replay cash/random/reward effects");
    check(tick.candidate->popularity_rewards.size() == rewards.size(),
          "raw97 preserves reward catalog size");
    for (std::size_t index = 0; index < rewards.size(); ++index)
        check(tick.candidate->popularity_rewards[index].status == rewards[index].status &&
                  tick.candidate->popularity_rewards[index].pending_notice ==
                      rewards[index].pending_notice,
              "raw97 leaves each claimed reward and pending notice untouched");
    s = *tick.candidate;
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "retired raw97 cannot repeat its effect through player confirmation");
}
void unlocked_visitor() {
    auto s = fixture(59);
    s.scripts.pages.back().legacy_f = 2;
    const auto id = s.scripts.pages.back().id;
    const auto actors = s.scene.world.world.ai.human_order;
    const auto priority = s.human_calendar.at(2).absent_months;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.page_counters.at(id) == 0 && s.scripts.pages.back().lifecycle != 4 &&
              s.human_calendar.at(2).absent_months == priority,
          "raw59 early confirm does not fastforward or change arrival priority");
    for (int count = 1; count <= 70; ++count) {
        const auto tick = prepare_startup_world_runtime(s);
        check(tick.candidate && tick.candidate->page_counters.at(id) == count &&
                  tick.candidate->scripts.pages.back().lifecycle != 4 &&
                  tick.candidate->scene.world.world.ai.human_order == actors &&
                  tick.candidate->scene.random.draws() == s.scene.random.draws() &&
                  tick.candidate->scene.calendar.units == s.scene.calendar.units &&
                  tick.candidate->scene.world.world.ai.accounting.funds() == cash &&
                  tick.candidate->human_calendar.at(2).absent_months == priority,
              "raw59 page clock advances without auto-close, spawn, reward or world tick");
        s = *tick.candidate;
        check(s.sound_requests == std::vector<int>{5},
              "raw59 sound5 occurs only on first actual update");
        if (count < 70)
            check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
                      s.page_counters.at(id) == count && s.scripts.pages.back().lifecycle != 4,
                  "raw59 confirms below70 preserve page and exact counter");
    }
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::none &&
              s.human_calendar.at(2).absent_months == 10 && s.scripts.pages.back().lifecycle == 4 &&
              s.scene.world.world.ai.human_order == actors,
          "raw59 ready confirm writes aq10 and closes without direct arrival");
    check(acknowledge_startup_world_runtime_page(s, id) == StartupWorldRuntimeError::invalid_page,
          "raw59 closed page rejects duplicate confirm");
    auto bad = fixture(59);
    bad.scripts.pages.back().legacy_f = -1;
    check(!prepare_startup_world_runtime(bad).candidate && bad.page_counters.empty() &&
              acknowledge_startup_world_runtime_page(bad, bad.scripts.pages.back().id) ==
                  StartupWorldRuntimeError::missing_source &&
              bad.scripts.pages.back().lifecycle != 4,
          "raw59 invalid definition rolls back update and confirmation");
}
} // namespace
int main() {
    try {
        summary();
        gift();
        focus_and_pause();
        crew_initialization();
        rank_conditions();
        rank_promotion();
        task_focus_and_introduction();
        annual_termination();
        annual_award();
        task_display();
        event_message_and_shop_return();
        popularity_return();
        unlocked_visitor();
        human_details_and_gifts();
        human_profession_and_mastery();
        tax_pages();
        std::cout << "startup world pages checks: " << checks << '\n';
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
