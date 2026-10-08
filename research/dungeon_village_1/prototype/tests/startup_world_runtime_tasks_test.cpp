#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_presentation.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
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
void factory() {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream{});
    const auto &s = session.state();
    const auto projection = startup_world_runtime_factory(s);
    check(projection.definitions.size() == 81 && projection.monsters.size() == 36 &&
              projection.humans.size() == 25 && projection.rewards.size() == 149 &&
              projection.facility_definitions.size() == 85 &&
              projection.base_variants.size() == 576,
          "real reset owner projects every original factory catalogue");
    check(projection.first_reward_weapon == 26 &&
              projection.generation_bounds == std::array<ref::Position, 2>{{{4, 21}, {19, 15}}} &&
              projection.task_sequence == 0 && projection.rank == 0,
          "source p.J flag16 and region/currentrank/sequence retain actual reset values");
    const auto created = ref::prepare_world_task_creation(projection, 0);
    check(created.candidate && created.candidate->created_task,
          "actual stage0 kind0 factory allocates task/site on real new map");
    auto next = s;
    check(write_startup_world_runtime_factory(next, created.candidate->state),
          "factory map/task/random writes to one current owner");
    const auto task = *created.candidate->created_task;
    const auto &t = next.tasks.at(task);
    check(t.facility && t.site && next.task_order == std::vector<std::uint64_t>{task} &&
              next.task_original_ids.at(task) == 1 && next.task_sequence == 1 &&
              next.scene.random.draws() == created.candidate->state.random.draws() &&
              s.tasks.empty() && s.scene.random.draws() == 0,
          "rawID/orderedtask/newsite/candidate RNG commit atomically, original remains untouched");
    const auto site = *t.facility;
    check(next.scene.world.world.facilities.at(site).category == 5 &&
              next.facility_residents.at(site) == -1 &&
              next.facility_details.at(site).condition == t.difficulty &&
              next.dungeon_facilities.at(site).challenges.back()[5] == 26,
          "new exploration source tenant is usable/no resident/difficulty plus real first weapon");
    check(next.scene.world.world.ai.accounting.funds() == s.scene.world.world.ai.accounting.funds(),
          "task creation is not a build transaction and never charges source funds");
    check(t.pending_completion_value == 20,
          "first-stage original quest column14 keeps twenty pending popularity, not zero");
    const auto roundtrip = startup_world_runtime_factory(next);
    check(roundtrip.finish.tasks.at(task).site == t.site &&
              roundtrip.facility_order == next.scene.world.facility_order &&
              roundtrip.finish.dungeon.world.map.cells.size() == 576,
          "next factory read sees current map/task/metadata, no reset shadow");
    const auto failed = prepare_startup_world_runtime_dungeon_finish(next, 99999);
    check(!failed && next.tasks.count(task) && next.scene.world.world.facilities.count(site),
          "missing finish site leaves world/reference/metadata untouched");
    // 显式阶段2调用点夹具，仅认证真实生成site后的组合提交，不冒充自然完成计时。
    next.scene.world.world.facilities.at(site).status = 2;
    next.dungeon_facilities.at(site).updates = 10;
    const auto cleaned = prepare_startup_world_runtime_dungeon_finish(next, site);
    check(cleaned && !cleaned->scene.world.world.facilities.count(site) &&
              cleaned->task_order.empty() && cleaned->tasks.at(task).facility == site &&
              cleaned->scene.world.facility_order == s.scene.world.facility_order,
          "real stage2 no-active-task restores map/removesorder but retains old task object "
          "reference");
    const auto later = ref::prepare_world_task_creation(startup_world_runtime_factory(*cleaned), 0);
    check(later.candidate && later.candidate->created_task &&
              *later.candidate->created_task != task &&
              later.candidate->state.finish.tasks.at(*later.candidate->created_task).facility !=
                  site &&
              later.candidate->state.finish.tasks.at(task).facility == site,
          "new factory after restored original site cannot alias retired summary/task identity");
}
void crew_item_reward_writeback() {
    auto s = test_support::world_fixture();
    const auto created = ref::prepare_world_task_creation(startup_world_runtime_factory(s), 0);
    check(created.candidate && created.candidate->created_task &&
              write_startup_world_runtime_factory(s, created.candidate->state),
          "crew reward fixture uses actual factory site and full original item catalogue");
    const auto task = *created.candidate->created_task;
    const auto facility = *s.tasks.at(task).facility;
    const ref::CharacterId actor_id{900};
    ref::BattleActorRecord actor;
    actor.id = actor_id;
    actor.definition = 1;
    actor.control.state = 14;
    actor.capacity = 100;
    actor.hp = {0, 100, 100, 100, false, 0};
    const auto cell = s.scene.world.world.facilities.at(facility).placement.anchor;
    actor.position = {static_cast<float>(cell.x * 100 + 50), 0,
                      static_cast<float>(cell.y * 100 + 50)};
    s.scene.world.world.ai.battle.actors.emplace(actor_id, actor);
    s.scene.world.world.ai.human_order.push_back(actor_id);
    s.scene.world.world.ai.contexts.emplace(actor_id, ref::RewardActorContext{cell, false, {}, {}});
    s.scene.world.world.actors.emplace(actor_id, ref::RescueActorContext{});
    s.scene.world.world.actors.at(actor_id).binding = ref::ArrivalBinding{
        cell, {facility}, s.scene.world.world.facilities.at(facility).placement.definition_id};
    s.dungeon_actors.emplace(actor_id, ref::DungeonActorProgress{});
    s.scene.world.world.facilities.at(facility).occupants = {actor_id};
    s.scene.world.world.facilities.at(facility).status = 1;
    // 最小已到达挑战位置的调用点夹具；仍由真实crew计算与grant回调发普通道具。
    s.dungeon_facilities.at(facility).challenges = {{0, 0, 0, 0, 0, 0}};
    const auto before = s.items.at(0).inventory;
    const auto draws = s.scene.random.draws();
    const auto result = prepare_startup_world_runtime_dungeon_crew(s, facility);
    check(result && result->items.at(0).inventory == before + 1 &&
              result->catalog.at({0, 0}).inventory == before + 1 &&
              result->item_rewards == s.item_rewards + 1 && !result->scripts.notices.empty() &&
              result->scripts.notices.back().message == 2 &&
              result->scene.random.draws() == draws && s.items.at(0).inventory == before,
          "actual crew callback publishes one item to both owner projections and emits original "
          "notice2");
    auto missing = s;
    missing.items.erase(0);
    check(!prepare_startup_world_runtime_dungeon_crew(missing, facility) &&
              missing.catalog.at({0, 0}).inventory == before &&
              missing.item_rewards == s.item_rewards && missing.scene.random.draws() == draws,
          "crew callback missing item mirror rolls back reward, notices and random");
}
void encounter() {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream{});
    ref::EncounterCreationInput input;
    input.kind = 0;
    input.center = {10, 18};
    input.year_index = 0;
    input.month_index = 3;
    const auto result = prepare_startup_world_runtime_encounter(session.state(), input);
    check(result && result->scene.world.world.ai.encounters.size() == 1 &&
              result->scene.world.world.ai.monster_order.size() == 1,
          "actual first-year typed creation consumes shared RNG then spawns real monster");
    const auto actor = result->scene.world.world.ai.monster_order.front();
    check(result->scene.world.world.actors.count(actor) && result->actor_metadata.count(actor),
          "new monster receives canonical movement and old-u metadata, not AI-only orphan");
    // 局部P生成后的投影边界夹具：真实怪物已存在，但同轮尚未建立原型表现缓存。
    auto local_spawn = *result;
    local_spawn.actor_metadata.erase(actor);
    const auto routes = startup_world_runtime_routes(*result);
    const auto draws = local_spawn.scene.random.draws();
    check(write_startup_world_runtime_routes(local_spawn, routes) &&
              local_spawn.actor_metadata.count(actor) &&
              startup_world_runtime_adapter()
                  .actors
                  .projected_facing(local_spawn, actor, routes.world.ai.battle.actors.at(actor))
                  .has_value() &&
              local_spawn.scene.random.draws() == draws,
          "local actor route publication supplies real spawned monster metadata before same d");
    local_spawn.actor_metadata.at(actor).cached_view = {99, 88};
    check(write_startup_world_runtime_routes(local_spawn, routes) &&
              local_spawn.actor_metadata.at(actor).cached_view == ref::Position{99, 88},
          "monster synchronization never recomputes existing old-u cache on route publication");
    check(result->scene.world.map_flags.at(18 * 24 + 10) & 2U,
          "source event map refresh happens in typed request before returned owner");
    check(session.state().scene.world.world.ai.monster_order.empty() &&
              session.state().scene.random.draws() == 0,
          "typed creation never mutates caller or exposes partial owner");
    auto residential = session.state();
    ref::WorldScriptContinuation residence;
    residence.event = 2701;
    residence.remaining_updates = 100;
    residential.scripts.continuations.push_back(residence);
    const auto alongside = prepare_startup_world_runtime_encounter(residential, input);
    check(alongside && alongside->scripts.continuations.size() == 1 &&
              alongside->scripts.continuations.front().event == 2701 &&
              alongside->scripts.continuations.front().remaining_updates == 100,
          "encounter validates full catalog while retaining unrelated authentic residence "
          "continuation");
    const auto adapter = startup_world_runtime_adapter();
    check(static_cast<bool>(adapter.create_encounter),
          "original month7 world-entry has an actual ordinary creation consumer");
    input.month_index = 6;
    const auto global = adapter.create_encounter(session.state(), input);
    check(global && global->created && global->denial == ref::EncounterCreationDenial::none &&
              global->state.scene.world.world.ai.encounters.count(*global->created) &&
              !global->state.scene.world.world.ai.monster_order.empty(),
          "global callback publishes canonical created identity and actual spawn result");
    for (const auto id : global->state.scene.world.world.ai.monster_order)
        check(global->state.scene.world.world.actors.count(id) &&
                  global->state.actor_metadata.count(id) &&
                  global->state.scene.world.world.ai.contexts.count(id),
              "global spawn installs every new monster context before same-frame consumers");
    auto blocked = input;
    blocked.upper_band_town = {true, true, true};
    const auto denied = adapter.create_encounter(session.state(), blocked);
    check(denied && !denied->created && denied->denial == ref::EncounterCreationDenial::town &&
              denied->state.scene.world.world.ai.encounters.empty() &&
              denied->state.scene.random.draws() == session.state().scene.random.draws(),
          "ordinary town rejection preserves typed denial, not missing-consumer failure");
    // 模拟共同调度局部L的refresh提交，不经过全局生成wrapper末尾。
    auto local = session.state();
    local.scene.world.world.ai = result->scene.world.world.ai;
    const auto refreshed = consume_startup_world_runtime_encounter_request(
        local, {ref::EncounterCreationRequestKind::refresh_map, 0});
    check(refreshed && refreshed->scene.world.world.actors.count(actor) &&
              refreshed->actor_metadata.count(actor) && !local.actor_metadata.count(actor),
          "local L refresh atomically registers canonical monster runtime and cached-u metadata");
    const auto &n = refreshed->scene.world.world.ai.battle.actors.at(actor).position;
    check(refreshed->actor_metadata.at(actor).cached_view ==
              ref::Position{static_cast<int>(n.x * 30.0F / 100.0F + n.z * 30.0F / 100.0F),
                            static_cast<int>(n.x * -15.0F / 100.0F + n.z * 15.0F / 100.0F)},
          "local initial cached-u preserves actual floating spawn jitter");
}
void task_victory_requests() {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream{});
    const auto created =
        ref::prepare_world_task_creation(startup_world_runtime_factory(session.state()), 1);
    check(created.candidate && created.candidate->created_task,
          "original stage0 battle task factory provides an actual task definition and site");
    auto state = session.state();
    check(write_startup_world_runtime_factory(state, created.candidate->state),
          "battle task source factory commits current owner");
    const auto id = *created.candidate->created_task;
    state.active_task = id;
    state.participants = {1, 3, 2};
    state.task.encounter = 7;
    state.scene.world.world.ai.task_active = true;
    ref::RewardEncounter event;
    event.runtime.id = 7;
    event.runtime.state = 3;
    state.scene.world.world.ai.encounters.emplace(7, event);
    state.scene.world.world.ai.encounter_order.push_back(7);
    const auto definition = state.tasks.at(id).definition;
    const auto &rules = *state.rules;
    const auto input = startup_world_runtime_encounter_input(state, 7);
    check(input && input->quest.participants == state.participants &&
              input->quest.normal_definitions == rules.tasks.at(definition).encounter_monsters &&
              input->quest.completion_delta ==
                  rules.tasks.at(definition).factory.pending_completion_value &&
              input->quest.flags == state.task_progress.definitions.at(definition).flags,
          "quest encounter reads current task participants/flags and authentic normal "
          "monster/delta columns");
    const auto original_cash = state.scene.world.world.ai.accounting.funds();
    check(consume_startup_world_runtime_task_encounter_request(
              state, {ref::EncounterRequestKind::page30, {}, 30, 70}),
          "task victory inserts actual source page30 without granting summary cash");
    check(consume_startup_world_runtime_task_encounter_request(
              state, {ref::EncounterRequestKind::page31, {}, 1, 2}),
          "task victory inserts actual source page31 before mark/clear task");
    const auto result_page = std::find_if(state.scripts.pages.begin(), state.scripts.pages.end(),
                                          [](const auto &p) { return p.legacy_page == 31; });
    check(result_page != state.scripts.pages.end() && result_page->task_identity == id &&
              result_page->task_definition == definition && result_page->legacy_f == 1 &&
              result_page->legacy_g == 2 && state.crew_summaries.empty(),
          "result page holds retired-safe real task and sampled speakers, initialization is not "
          "premature");
    const auto page_id = result_page->id;
    // 明确特殊旗标夹具，认证真实页初始化的H限缩；不声称首期普通任务拥有flag2。
    state.task_progress.definitions.at(definition).flags |= 2U;
    auto &humans = state.scene.world.world.ai.battle.humans;
    humans.at(1).task_kills = 4;
    humans.at(3).task_kills = 2;
    humans.at(2).task_kills = 0;
    humans.at(1).participant_downs = 5;
    check(initialize_startup_world_runtime_task_result_page(state, page_id) &&
              state.crew_summaries.at(page_id) == state.participants &&
              state.scene.world.world.ai.battle.humans.at(1).task_kills == 1 &&
              state.scene.world.world.ai.battle.humans.at(3).task_kills == 0 &&
              state.scene.world.world.ai.battle.humans.at(1).participant_downs == 5,
          "raw31 lifecycle initialization copies exact X and clamps source first H without "
          "touching I");
    state.scene.world.world.ai.battle.humans.at(1).task_kills = 3;
    check(initialize_startup_world_runtime_task_result_page(state, page_id) &&
              state.scene.world.world.ai.battle.humans.at(1).task_kills == 3,
          "already initialized page cannot repeat H side effect on acknowledgement");
    state.task_progress.definitions.at(definition).flags &= ~2U;
    state.scene.world.world.ai.task_completed = true; // 原encounter核心先提交scalar。
    check(consume_startup_world_runtime_task_encounter_request(
              state, {ref::EncounterRequestKind::mark_task_complete, {}, 0, 0, 0}) &&
              state.task_progress.definitions.at(definition).completed == 1 &&
              state.task_progress.successes == 1,
          "quest success calls current original task statistics once before clear");
    for (auto &human : state.human_flags)
        human.second |= 2U;
    check(consume_startup_world_runtime_task_encounter_request(
              state, {ref::EncounterRequestKind::clear_task, {}, 0, 0, 0}) &&
              !state.active_task && state.task_order.empty() && state.tasks.count(id) &&
              state.participants == std::vector<int>{1, 3, 2},
          "clear removes ordered active reference yet preserves old task object and participant "
          "display");
    check(std::all_of(state.human_flags.begin(), state.human_flags.end(),
                      [](const auto &h) { return (h.second & 2U) == 0; }),
          "clear applies original bit2 cleanup to every human definition");
    check(consume_startup_world_runtime_task_encounter_request(
              state, {ref::EncounterRequestKind::refresh_global, {}, 0, 0, 0}) &&
              state.sound_requests.back() == 1 &&
              state.scene.world.world.ai.accounting.funds() == original_cash,
          "post-clear original refresh restores music1 and never repays encounter reward");
    const auto pending = state.scene.world.world.ai.pending_completion;
    check(
        consume_startup_world_runtime_task_encounter_request(
            state, {ref::EncounterRequestKind::completion_delta, {}, 20}) &&
            state.scene.world.world.ai.pending_completion == pending,
        "external typed completion does not duplicate scalar already committed by encounter core");
    check(consume_startup_world_runtime_task_encounter_request(
              state, {ref::EncounterRequestKind::refresh_task_catalog, {}, 0, 0, 0}),
          "first task victory performs original H current catalogue restock immediately");
    check(state.crew_summaries.at(page_id) == std::vector<int>{1, 3, 2} && state.tasks.count(id),
          "subsequent catalogue update cannot invalidate closed-task result payload");
}
void active_management() {
    auto s = test_support::world_fixture(ref::WorldRandomStream::from_java_seed(0));
    const auto created = ref::prepare_world_task_creation(startup_world_runtime_factory(s), 0);
    check(created.candidate && created.candidate->created_task &&
              write_startup_world_runtime_factory(s, created.candidate->state),
          "active management conditional fixture uses real source task factory");
    const auto task = *created.candidate->created_task;
    const auto site = *s.tasks.at(task).facility;
    s.active_task = task;
    s.scene.world.world.ai.task_active = true;
    s.task.kind = 0;
    s.task.center = *s.tasks.at(task).site;
    s.participants = {1};
    s.human_presence.at(3) = 1;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    check(open_startup_world_runtime_task_menu(s) == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 26,
          "active task opens real participant page26 rather than rejecting all task management");
    const auto team = s.scripts.pages.back().id;
    check(act_startup_world_runtime_task_page(s, team, StartupWorldTaskAction::depart).error ==
                  StartupWorldRuntimeError::invalid_page &&
              s.active_task == task,
          "active team cannot redepart or replace active task");
    check(act_startup_world_runtime_task_page(s, team, StartupWorldTaskAction::add_member).error ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 27 &&
              std::find(s.task_extra_pages.at(s.scripts.pages.back().id).begin(),
                        s.task_extra_pages.at(s.scripts.pages.back().id).end(),
                        3) != s.task_extra_pages.at(s.scripts.pages.back().id).end(),
          "active team reuses source unpaid extra-candidate snapshot with actual available "
          "definition");
    const auto extra = s.scripts.pages.back().id;
    check(act_startup_world_runtime_task_page(s, extra, StartupWorldTaskAction::hire, 3).error ==
                  StartupWorldRuntimeError::none &&
              s.participants == std::vector<int>({1, 3}) &&
              s.scene.world.world.ai.accounting.funds() == cash - 200 && s.active_task == task &&
              (s.human_flags.at(3) & 2U),
          "active hiring pays current200 once, marks same definition and preserves active task");
    // 关闭实际64提示与旧追加页，不推进真实世界；仍是显式页面输入的条件调用点。
    for (auto &p : s.scripts.pages)
        if (p.id != team && p.kind != ref::WorldScriptPageKind::scene)
            p.lifecycle = 4;
    check(act_startup_world_runtime_task_page(s, team, StartupWorldTaskAction::inspect, 0).error ==
                  StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 60 &&
              s.page_human_bindings.at(s.scripts.pages.back().id) == 1,
          "participant detail binds source definition, not synthetic actor or another roster");
    check(act_startup_world_runtime_task_page(s, s.scripts.pages.back().id,
                                              StartupWorldTaskAction::cancel)
                      .error == StartupWorldRuntimeError::none &&
              act_startup_world_runtime_task_page(s, team, StartupWorldTaskAction::cancel).error ==
                  StartupWorldRuntimeError::none,
          "detail and team return independently, no fee or active task cancellation");
    check(open_startup_world_runtime_task_control_menu(s) == StartupWorldRuntimeError::none,
          "source adventure4 task-control entry opens without cancelling task");
    const auto parent =
        std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(), [](const auto &p) {
            return p.lifecycle != 4;
        })->id;
    check(act_startup_world_runtime_task_page(s, parent, StartupWorldTaskAction::request_abort)
                      .error == StartupWorldRuntimeError::none &&
              s.scripts.pages.back().legacy_page == 1,
          "source task abort creates real raw1 yes/no question");
    const auto question = s.scripts.pages.back().id;
    const auto before = s.scene.world.world.ai.pending_completion;
    const auto draws = s.scene.random.draws();
    check(act_startup_world_runtime_task_page(s, question, StartupWorldTaskAction::confirm, 0)
                      .error == StartupWorldRuntimeError::none &&
              s.active_task == task && s.scene.world.world.facilities.count(site) &&
              !ref::world_script_seen(s.scripts, 80),
          "question close records yes only; n.o and effects wait for actual parent callback");
    auto broken = s;
    broken.sites.at(site).occupied_cells.clear();
    check(!update_startup_world_runtime_task_control_page(broken, parent) &&
              broken.active_task == task && broken.task_abort_answers.at(parent) == 0 &&
              broken.scene.world.world.facilities.count(site),
          "late restoration rejection preserves pending answer and entire active world");
    const auto settled = update_startup_world_runtime_task_control_page(s, parent);
    check(
        settled && !settled->active_task && !settled->scene.world.world.facilities.count(site) &&
            settled->scene.world.world.ai.pending_completion == before - 10 &&
            settled->scripts.event_calls.at(80) == 1 && settled->scripts.event_calls.at(162) == 1 &&
            settled->scripts.notices.back().message == 26 &&
            settled->scene.world.world.ai.accounting.funds() == cash - 200 &&
            settled->scene.random.draws() == draws && !settled->deadline_page &&
            settled->participants == std::vector<int>({1, 3}),
        "parent restores whole site then80/-10/first162/notice26; no deadlinefee or random shadow");
}

// 本批仍用真实工厂分配身份；挑战门槛和显式绘制次数是调用点夹具，不冒充自然玩家路线。
StartupWorldRuntimeState presentation_task(bool retired = false) {
    auto s = test_support::world_fixture();
    const auto made = ref::prepare_world_task_creation(startup_world_runtime_factory(s), 0);
    check(made.candidate && made.candidate->created_task &&
              write_startup_world_runtime_factory(s, made.candidate->state),
          "presentation fixture allocates actual original exploration task and map site");
    const auto task = *made.candidate->created_task;
    const auto facility = *s.tasks.at(task).facility;
    if (retired) {
        s.scene.world.world.facilities.at(facility).status = 2;
        s.dungeon_facilities.at(facility).updates = 10;
        const auto ended = prepare_startup_world_runtime_dungeon_finish(s, facility);
        check(ended && ended->tasks.count(task) &&
                  !ended->scene.world.world.facilities.count(facility),
              "missing-binding fixture retires real factory site through actual phase2 consumer");
        s = *ended;
        s.task_order = {task};
    } else {
        s.dungeon_facilities.at(facility).challenges = {
            {0,1,1,10,0,0}, {20,1,1,9,0,0}, {40,0,1,10,0,0},
            {60,1,2,10,0,0}, {80,1,1,10,0,0}};
    }
    s.active_task = task;
    s.scene.world.world.ai.task_active = true;
    s.task.kind = 0;
    s.task.center = *s.tasks.at(task).site;
    s.task.encounter.reset();
    s.participants = {1,3,2};
    s.scripts.executing_page.reset();
    s.scripts.pages.front().lifecycle = 3;
    s.scene.random = ref::WorldRandomStream::from_raw({2,0,1,2});
    return s;
}
StartupPresentationRequest presentation_request(const StartupWorldRuntimeState &s,
                                               StartupPresentationMode mode,
                                               bool gift = false) {
    const auto pages = startup_world_presentation_pages(s, mode);
    check(pages.has_value(), "presentation fixture has valid physical page stack");
    StartupPresentationRequest request;
    request.ordinal = 7;
    request.mode = mode;
    request.expected_pages = *pages;
    request.gift_wrapper_ready = gift;
    return request;
}
std::uint64_t presentation_gift(StartupWorldRuntimeState &s, int counter = 45) {
    ref::WorldScriptPage p;
    p.id = s.scripts.next_page_id++;
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = 66;
    p.lifecycle = 1;
    s.scripts.pages.push_back(p);
    s.page_human_bindings[p.id] = 1;
    s.human_equipment_choices[p.id] = {4,29};
    s.human_gift_scores[p.id] = 50;
    s.human_gift_messages[p.id] = "谢谢";
    check(initialize_startup_world_human_pages(s),
          "raw66 uses actual human-page initialization and already-consumed recovery item binding");
    s.scripts.executing_page.reset();
    s.page_counters[p.id] = counter;
    return p.id;
}
bool same_random(const ref::WorldRandomStream &a, const ref::WorldRandomStream &b) {
    const auto x = a.snapshot(), y = b.snapshot();
    return x.engine_state == y.engine_state && x.tape == y.tape && x.cursor == y.cursor &&
           x.tape_mode == y.tape_mode;
}
void presentation_admission_and_random() {
    using Mode = StartupPresentationMode;
    auto s = presentation_task();
    const auto task = *s.active_task;
    const auto facility = *s.tasks.at(task).facility;
    const auto scene = s.scripts.pages.front().id;
    const auto original = s.scene.random;
    check(s.scene.random.draws() == 0 && s.sound_requests.empty(),
          "zero admitted presentation calls consume no implicit decorative random or sound");
    auto request = presentation_request(s, Mode::full_redraw);
    const auto first = prepare_startup_world_presentation(s, request);
    check(first.candidate && first.plan && first.plan->dungeon_jitters.size() == 2 &&
              first.plan->dungeon_jitters[0].page == scene &&
              first.plan->dungeon_jitters[0].facility == facility &&
              first.plan->dungeon_jitters[0].challenge == 4 &&
              first.plan->dungeon_jitters[0].offset == 1 &&
              first.plan->dungeon_jitters[1].challenge == 0 &&
              first.plan->dungeon_jitters[1].offset == -1 &&
              first.plan->random_before == 0 && first.plan->random_after == 2 &&
              same_random(s.scene.random, original),
          "one scene invocation draws reverse4 then0; monster age9/treasure/state2 are excluded");
    s = *first.candidate;
    const auto second = prepare_startup_world_presentation(s, request);
    check(second.candidate && second.plan && second.plan->ordinal == 7 &&
              second.plan->dungeon_jitters.size() == 2 &&
              second.plan->dungeon_jitters[0].challenge == 4 &&
              second.plan->dungeon_jitters[0].offset == 0 &&
              second.plan->dungeon_jitters[1].challenge == 0 &&
              second.plan->dungeon_jitters[1].offset == 1 &&
              second.plan->random_before == 2 && second.plan->random_after == 4,
          "same ordinal identifies another explicitly admitted invocation, never automatic deduplication");
    s = *second.candidate;
    const auto frozen = *first.plan;
    std::vector<int> displayed;
    for (int repeat = 0; repeat < 2; ++repeat)
        for (const auto &jitter : frozen.dungeon_jitters) displayed.push_back(jitter.offset);
    check(displayed == std::vector<int>({1,-1,1,-1}) && s.scene.random.draws() == 4 &&
              frozen.random_after == 2 && s.tasks.at(task).facility == facility,
          "re-presenting frozen jitter facts is read-only and does not submit another invocation");
    auto exhausted = presentation_task();
    exhausted.scene.random = ref::WorldRandomStream::from_raw({2});
    const auto tape = exhausted.scene.random;
    const auto rejected = prepare_startup_world_presentation(exhausted,
        presentation_request(exhausted, Mode::full_redraw));
    check(!rejected.candidate && !rejected.plan && same_random(exhausted.scene.random,tape) &&
              exhausted.active_task == task && exhausted.sound_requests.empty(),
          "second challenge tape exhaustion discards earlier ticket and every partial drawing plan");
    auto paused = presentation_task();
    paused.scene.framework_paused = true;
    const auto allowed = prepare_startup_world_presentation(paused,
        presentation_request(paused, Mode::full_redraw));
    check(allowed.candidate && allowed.plan && allowed.plan->random_after == 2,
          "world pause is independent of an explicitly admitted actual presentation call");

    s = presentation_task();
    const auto gift = presentation_gift(s);
    const auto full = startup_world_presentation_pages(s,Mode::full_redraw);
    check(full && *full == std::vector<std::uint64_t>({scene,gift}),
          "full repaint visits live lower scene before top raw66");
    auto top = presentation_request(s,Mode::top_only);
    check(top.expected_pages == std::vector<std::uint64_t>{gift},
          "top-only selection is physical last page rather than last live scene");
    const auto top_call = prepare_startup_world_presentation(s,top);
    check(top_call.candidate && top_call.plan->dungeon_jitters.empty() &&
              top_call.plan->sound_requests == 0 && top_call.plan->random_after == 0,
          "raw66 wrapper readiness false omits its outlet and top-only does not draw lower task footer");
    for (const int lifecycle : {0,4}) {
        auto hidden = s;
        hidden.scripts.pages.back().lifecycle = lifecycle;
        const auto top_pages = startup_world_presentation_pages(hidden,Mode::top_only);
        const auto all_pages = startup_world_presentation_pages(hidden,Mode::full_redraw);
        check(top_pages && top_pages->empty() && all_pages &&
                  *all_pages == std::vector<std::uint64_t>{scene},
              "physical top life0/4 suppresses top-only without falling back; full filters only that page");
        auto no_top = presentation_request(hidden,Mode::top_only);
        const auto empty = prepare_startup_world_presentation(hidden,no_top);
        check(empty.candidate && empty.plan->pages.empty() && empty.plan->random_after == 0,
              "zero-page admitted request publishes an empty plan without random");
        no_top.gift_wrapper_ready = true;
        check(!prepare_startup_world_presentation(hidden,no_top).candidate,
              "wrapper readiness cannot authorize lifecycle-filtered raw66");
    }
    for (int fault = 0; fault < 5; ++fault) {
        auto bad = presentation_request(s,Mode::full_redraw);
        if (fault == 0) std::reverse(bad.expected_pages.begin(),bad.expected_pages.end());
        if (fault == 1) bad.expected_pages.pop_back();
        if (fault == 2) bad.expected_pages.push_back(gift);
        if (fault == 3) bad.ordinal = 0;
        if (fault == 4) bad.mode = static_cast<Mode>(99);
        const auto rejection = prepare_startup_world_presentation(s,bad);
        check(!rejection.candidate && !rejection.plan && s.scene.random.draws() == 0,
              "incorrect page order/missing/duplicate/zero ordinal/unknown mode rejects before any draw");
    }
    auto scene_only = presentation_task();
    auto inappropriate = presentation_request(scene_only,Mode::full_redraw,true);
    check(!prepare_startup_world_presentation(scene_only,inappropriate).candidate,
          "explicit gift readiness is only valid for actual physical top raw66");
    for (int fault = 0; fault < 4; ++fault) {
        auto broken = s;
        if (fault == 0) broken.scripts.executing_page = scene;
        if (fault == 1) broken.scripts.page_mutations_locked = true;
        if (fault == 2) broken.scripts.pages.back().id = scene;
        if (fault == 3) broken.scripts.pages.back().lifecycle = 5;
        check(!startup_world_presentation_pages(broken,Mode::full_redraw),
              "live executing root/locked mutations/duplicate page identity/bad lifecycle rejects admission");
    }
    for (int fault = 0; fault < 4; ++fault) {
        auto broken = presentation_task();
        auto &challenge = broken.dungeon_facilities.at(facility).challenges[0];
        // index4先抽，index0最后失败，仍须退回整批。
        if (fault == 0) challenge[0] = -1;
        if (fault == 1) challenge[1] = 2;
        if (fault == 2) challenge[2] = 4;
        if (fault == 3) challenge[3] = -1;
        const auto random = broken.scene.random;
        const auto bad = prepare_startup_world_presentation(broken,
            presentation_request(broken,Mode::full_redraw));
        check(!bad.candidate && !bad.plan && same_random(broken.scene.random,random),
              "late challenge-domain failure rolls back preceding reverse-order ticket");
    }
    auto battle = test_support::world_fixture();
    const auto made = ref::prepare_world_task_creation(startup_world_runtime_factory(battle),1);
    check(made.candidate && made.candidate->created_task &&
              write_startup_world_runtime_factory(battle,made.candidate->state),
          "non-exploration gate uses actual original kind1 task rather than rewriting kind0 source");
    battle.active_task = *made.candidate->created_task;
    battle.scene.world.world.ai.task_active = true;
    battle.scripts.pages.front().lifecycle = 3;
    battle.scene.random = ref::WorldRandomStream::from_raw({});
    const auto skipped = prepare_startup_world_presentation(battle,
        presentation_request(battle,Mode::full_redraw));
    check(skipped.candidate && skipped.plan && skipped.plan->dungeon_jitters.empty() &&
              skipped.plan->random_after == 0 && !skipped.plan->cleared_task &&
              skipped.candidate->active_task == battle.active_task,
          "non-exploration active task does not draw dungeon jitter or trigger missing-site cleanup");
}
void presentation_gift_sound_and_rollback() {
    using Mode = StartupPresentationMode;
    for (const int counter : {44,45,46}) {
        for (int guard = 0; guard < 3; ++guard) {
            auto s = test_support::world_fixture();
            s.scripts.pages.front().lifecycle = 3;
            const auto page = presentation_gift(s,counter);
            s.sound_requests = {5};
            auto request = presentation_request(s,Mode::top_only,true);
            request.application_preview = guard == 1;
            request.sound_paused = guard == 2;
            const auto first = prepare_startup_world_presentation(s,request);
            const std::size_t count = counter == 45 && guard == 0 ? 1 : 0;
            check(first.candidate && first.plan && first.plan->sound_requests == count &&
                      first.candidate->sound_requests ==
                          (count ? std::vector<int>({5,8}) : std::vector<int>({5})) &&
                      first.candidate->page_counters.at(page) == counter &&
                      first.plan->random_before == first.plan->random_after,
                  "66 literal44/45/46 and app-preview/sound-paused guards emit only counter45 sound8");
            const auto repeated = prepare_startup_world_presentation(*first.candidate,request);
            check(repeated.candidate && repeated.plan->sound_requests == count &&
                      repeated.candidate->sound_requests.size() == 1 + count * 2 &&
                      repeated.candidate->page_counters.at(page) == counter,
                  "new same-state66 invocation repeats its original outlet, preserving counter and old outputs");
        }
    }
    auto s = presentation_task();
    const auto page = presentation_gift(s);
    auto request = presentation_request(s,Mode::full_redraw,true);
    const auto valid = prepare_startup_world_presentation(s,request);
    check(valid.candidate && valid.plan && valid.plan->random_after == 2 &&
              valid.plan->sound_requests == 1 && valid.candidate->sound_requests.back() == 8,
          "one actual full repaint commits lower scene random followed by top66 sound8");
    s.human_gift_messages.erase(page);
    const auto random = s.scene.random;
    const auto task = s.active_task;
    const auto failed = prepare_startup_world_presentation(s,request);
    check(!failed.candidate && !failed.plan && same_random(s.scene.random,random) &&
              s.active_task == task && s.sound_requests.empty() && s.page_counters.at(page) == 45,
          "late invalid66 payload rolls back already-drawn scene tickets and does not publish sound or partial plan");
}
void presentation_missing_binding() {
    using Mode = StartupPresentationMode;
    auto s = presentation_task(true);
    const auto id = *s.active_task;
    const auto task = s.tasks.at(id);
    const auto position = *task.site;
    const auto index = static_cast<std::size_t>(position.y*s.scene.world.world.map.width+position.x);
    s.task_order = {id,id,id};
    for (auto &human : s.human_flags) human.second |= 2U;
    // 原定义旗位与活跃人物路线别名不同；最小人物辅助夹具只核此同步，非自然到访。
    const ref::CharacterId actor{900};
    ref::BattleActorRecord human;
    human.id = actor;
    human.kind = ref::ActorKind::human;
    human.definition = 1;
    s.scene.world.world.ai.battle.actors.emplace(actor,human);
    s.scene.world.world.actors.emplace(actor,ref::RescueActorContext{});
    s.scene.world.world.actors.at(actor).definition_task_flag = true;
    auto &surface = s.surface.at(index);
    surface.updates = 7; // 合法历史格计数夹具；检出tile.a(false)归零且未全图重建。
    const auto before_surface = surface;
    const auto original_random = s.scene.random;
    const auto cash = s.scene.world.world.ai.accounting.funds();
    const auto success = s.task_progress.successes;
    const auto pending = s.scene.world.world.ai.pending_completion;
    const auto items = s.item_rewards;
    const auto displays = s.exploration_displays.size();
    const auto participants = s.participants;
    const auto request = presentation_request(s,Mode::full_redraw);
    const auto cleared = prepare_startup_world_presentation(s,request);
    check(cleared.candidate && cleared.plan && cleared.plan->cleared_task == id &&
              cleared.plan->dungeon_jitters.empty() && cleared.plan->sound_requests == 0 &&
              cleared.candidate->task_order == std::vector<std::uint64_t>{id} &&
              !cleared.candidate->active_task && !cleared.candidate->scene.world.world.ai.task_active &&
              !cleared.candidate->scene.world.world.actors.at(actor).definition_task_flag &&
              cleared.candidate->task.kind == 0 && cleared.candidate->task.center == ref::Position{} &&
              !cleared.candidate->task.encounter &&
              cleared.candidate->tasks.at(id).facility == task.facility &&
              cleared.candidate->tasks.at(id).site == task.site &&
              cleared.candidate->participants == participants,
          "missing footer restores real retired site, removes exactly two first references, preserves task and participants");
    const auto &out = *cleared.candidate;
    check(std::all_of(out.human_flags.begin(),out.human_flags.end(),
                      [](const auto &h){return (h.second & 2U)==0;}) &&
              out.human_flags.size() == s.rules->humans.size() &&
              !out.scene.world.world.map.cells.at(index).facility &&
              out.scene.world.world.map.cells.at(index).legacy_state == 4 &&
              out.scene.world.world.map.cells.at(index).category == ref::RouteCategory::ground &&
              out.surface.at(index).definition == s.ground_definition &&
              out.surface.at(index).updates == 0 && out.surface.at(index).instance == -1 &&
              out.surface.at(index).fragment == -1 &&
              out.surface.at(index).display_definition == before_surface.display_definition &&
              out.surface.at(index).variant == before_surface.variant &&
              out.surface.at(index).road_mask == before_surface.road_mask &&
              out.scene.world.surface.at(index) == static_cast<int>(ref::RouteCategory::ground),
          "tile(false) synchronizes only original ground fields while preserving h/i/k and clearing all25 definition flags");
    check(same_random(out.scene.random,original_random) &&
              out.scene.world.world.ai.accounting.funds() == cash &&
              out.task_progress.successes == success &&
              out.scene.world.world.ai.pending_completion == pending && out.item_rewards == items &&
              out.exploration_displays.size() == displays && out.sound_requests.empty(),
          "missing-binding cleanup grants no reward/success/popularity/random/smoke or sound");
    const auto again = prepare_startup_world_presentation(out,request);
    check(again.candidate && again.plan && !again.plan->cleared_task &&
              again.candidate->task_order == std::vector<std::uint64_t>{id} &&
              same_random(again.candidate->scene.random,original_random),
          "second missing-binding invocation sees no active task and does not consume remaining historical duplicate");
    auto late = s;
    const auto page = presentation_gift(late);
    late.human_equipment_choices.erase(page);
    const auto failure = prepare_startup_world_presentation(late,
        presentation_request(late,Mode::full_redraw,true));
    check(!failure.candidate && !failure.plan && late.active_task == id &&
              late.task_order == std::vector<std::uint64_t>({id,id,id}) &&
              late.surface.at(index).updates == 7 &&
              std::all_of(late.human_flags.begin(),late.human_flags.end(),
                          [](const auto &h){return (h.second & 2U)!=0;}) &&
              same_random(late.scene.random,original_random) && late.sound_requests.empty(),
          "late invalid66 rejects full batch and restores prior missing-site fields, two references and all task flags");
    auto abort = s;
    check(abort_startup_world_runtime_task_entities(abort) && abort.active_task == id &&
              abort.task_order == s.task_order && abort.surface.at(index).updates == 7,
          "ordinary abort with missing binding retains original h and must not share presentation cleanup semantics");
}
} // namespace
int main() {
    try {
        factory();
        crew_item_reward_writeback();
        encounter();
        task_victory_requests();
        active_management();
        presentation_admission_and_random();
        presentation_gift_sound_and_rollback();
        presentation_missing_binding();
        std::cout << "startup runtime task checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
