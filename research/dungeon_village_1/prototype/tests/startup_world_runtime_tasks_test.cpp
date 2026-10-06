#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
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
} // namespace
int main() {
    try {
        factory();
        crew_item_reward_writeback();
        encounter();
        task_victory_requests();
        active_management();
        std::cout << "startup runtime task checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
