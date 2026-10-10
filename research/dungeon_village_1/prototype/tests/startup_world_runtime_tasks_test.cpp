#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_presentation.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_reference/world_notices.hpp"
#include "support/world_fixture.hpp"
#include "support/audio_requests.hpp"

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
    // 独立原表oracle：c/n.d在标题初始化时填目录，新局reset不将其清空。
    const std::vector<int> special{39, 47, 55, 63, 72, 80};
    const std::vector<int> replay{35, 36, 37, 38, 42, 43, 44, 45, 46, 50,
                                  51, 52, 53, 54, 58, 59, 60, 61, 62, 66,
                                  67, 68, 69, 70, 71, 75, 76, 77, 78, 79};
    check(s.task_special_selection_list == special && s.task_replay_order == replay &&
              projection.special_selection == special && projection.replay_order == replay &&
              projection.special_selection_mode == 0 && projection.special_selection_index == 0,
          "real reset owner projects ordered six special and thirty replay definitions with zero mode/index");
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
    check(t.pending_completion_value == 20 && t.newly_available,
          "first-stage original quest keeps twenty pending popularity and constructor NEW=true");
    const auto roundtrip = startup_world_runtime_factory(next);
    check(roundtrip.finish.tasks.at(task).site == t.site &&
              roundtrip.facility_order == next.scene.world.facility_order &&
              roundtrip.finish.dungeon.world.map.cells.size() == 576,
          "next factory read sees current map/task/metadata, no reset shadow");
    const auto failed = prepare_startup_world_runtime_dungeon_finish(next, 99999);
    check(!failed && next.tasks.count(task) && next.scene.world.world.facilities.count(site),
          "missing finish site leaves world/reference/metadata untouched");
    // 显式阶段2调用点夹具，仅认证真实生成site后的组合提交，不冒充自然完成计时。
    next.tasks.at(task).newly_available=false;
    next.scene.world.world.facilities.at(site).status = 2;
    next.dungeon_facilities.at(site).updates = 10;
    const auto cleaned = prepare_startup_world_runtime_dungeon_finish(next, site);
    check(cleaned && !cleaned->scene.world.world.facilities.count(site) &&
              cleaned->task_order.empty() && cleaned->tasks.at(task).facility == site &&
              !cleaned->tasks.at(task).newly_available &&
              cleaned->scene.world.facility_order == s.scene.world.facility_order,
          "real stage2 no-active-task restores map/removesorder but retains old task object "
          "reference");
    const auto later = ref::prepare_world_task_creation(startup_world_runtime_factory(*cleaned), 0);
    check(later.candidate && later.candidate->created_task &&
              *later.candidate->created_task != task &&
              later.candidate->state.finish.tasks.at(*later.candidate->created_task).facility !=
                  site &&
              later.candidate->state.finish.tasks.at(task).facility == site &&
              !later.candidate->state.finish.tasks.at(task).newly_available &&
              later.candidate->state.finish.tasks.at(*later.candidate->created_task).newly_available,
          "new factory preserves retired task read state and gives distinct new task its own NEW=true");
}
void task_new_notices() {
    auto s=test_support::world_fixture();
    check(startup_world_has_new_tasks(s)==false,"empty current task list has no NEW");
    const auto made=ref::prepare_world_task_creation(startup_world_runtime_factory(s),0);
    check(made.candidate && made.candidate->created_task &&
              write_startup_world_runtime_factory(s,made.candidate->state),
          "task NEW fixture starts from real factory instance");
    const auto first=*made.candidate->created_task;
    check(open_startup_world_runtime_task_menu(s)==StartupWorldRuntimeError::none,
          "raw22 opens for task NEW checks");
    const auto page=s.scripts.pages.back().id;
    check(s.scripts.pages.back().legacy_page==22 && s.tasks.at(first).newly_available &&
              startup_world_has_new_tasks(s)==true,
          "opening raw22 does not clear task NEW");
    // 明确调用点夹具：入页后多一个当前任务和一个仅历史对象，检验清理范围。
    const auto second=s.next_task_identity++;
    const auto historical=s.next_task_identity++;
    auto task=s.tasks.at(first);task.identity=second;s.tasks.emplace(second,task);
    task.identity=historical;s.tasks.emplace(historical,task);
    s.task_order={first,second,first};
    const auto before=startup_world_state_digest(s);
    check(startup_world_has_new_tasks(s)==true && startup_world_state_digest(s)==before,
          "NEW query preserves Owner outputs, random and duplicate current references");
    for(const auto action:{StartupWorldTaskAction::confirm,StartupWorldTaskAction::cancel}) {
        auto next=s;
        check(act_startup_world_runtime_task_page(next,page,action,0).error==StartupWorldRuntimeError::none &&
                  !next.tasks.at(first).newly_available && !next.tasks.at(second).newly_available &&
                  next.tasks.at(historical).newly_available && startup_world_has_new_tasks(next)==false &&
                  next.task_order==s.task_order && next.scene.random.draws()==s.scene.random.draws(),
              "raw22 exit clears complete current list, including later entry and duplicates, retaining history");
    }
    auto invalid=s;invalid.task_order.push_back(99999);
    check(startup_world_has_new_tasks(invalid)==true,
          "source NEW query stops at first true and does not inspect unread suffix");
    for(const auto action:{StartupWorldTaskAction::confirm,StartupWorldTaskAction::cancel}) {
        const auto unchanged=startup_world_state_digest(invalid);
        check(act_startup_world_runtime_task_page(invalid,page,action,0).error==StartupWorldRuntimeError::missing_source &&
                  startup_world_state_digest(invalid)==unchanged,
              "raw22 clearing checks full current list and rejects late missing reference atomically");
    }
    invalid.tasks.at(first).newly_available=false;
    invalid.tasks.at(second).newly_available=false;
    check(!startup_world_has_new_tasks(invalid),"NEW query rejects missing reference reached after false entries");
    invalid=s;invalid.tasks.at(first).identity=second;
    check(!startup_world_has_new_tasks(invalid),"NEW query rejects mismatched stable identity");
    invalid=s;invalid.task_order={historical};invalid.next_task_identity=historical;
    check(!startup_world_has_new_tasks(invalid),"NEW query rejects allocator-outside identity");
    invalid=s;invalid.task_order.clear();invalid.rules=nullptr;
    check(startup_world_has_new_tasks(invalid)==false,"empty query ignores unconsumed rules and retained NEW tasks");
    for(const bool locked:{false,true}) {
        auto late=s;
        late.scripts.page_mutations_locked=locked;
        const auto unchanged=startup_world_state_digest(late);
        const int row=locked?0:99;
        check(act_startup_world_runtime_task_page(late,page,StartupWorldTaskAction::confirm,row).error!=
                  StartupWorldRuntimeError::none && startup_world_state_digest(late)==unchanged,
              "failed row or later locked child insertion rolls back already prepared NEW clearing and page state");
    }
}
void selection_catalogue_consumers() {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream::from_java_seed(3));
    const auto &original = session.state();
    auto s = original;
    // 局部事件资格夹具；不用夹具重填目录，不宣称首次讨伐已自然完成。
    s.scripts.event_calls[60] = 1;
    const auto projection = startup_world_runtime_factory(s);
    check(projection.monsters.at(0).replay_available &&
              projection.monsters.at(1).replay_available &&
              projection.monsters.at(2).replay_available &&
              !projection.monsters.at(3).replay_available,
          "source initial flags1 y values reach replay qualification independently of task history");
    const auto replay = ref::prepare_world_task_creation(projection, 1);
    check(replay.candidate && replay.candidate->created_task &&
              replay.candidate->selected_definition &&
              (*replay.candidate->selected_definition == 35 ||
               *replay.candidate->selected_definition == 36 ||
               *replay.candidate->selected_definition == 37) &&
              replay.candidate->random_bounds.size() >= 2 &&
              replay.candidate->random_bounds[0] == 100 && replay.candidate->random_bounds[1] == 3,
          "seed3 replay ticket consumes actual reset catalogue and initial introduced monster pool");
    auto committed = s;
    check(write_startup_world_runtime_factory(committed, replay.candidate->state) &&
              committed.tasks.at(*replay.candidate->created_task).definition ==
                  *replay.candidate->selected_definition &&
              committed.task_replay_order == original.task_replay_order &&
              committed.task_special_selection_list == original.task_special_selection_list &&
              original.tasks.empty() && original.scene.random.draws() == 0,
          "replay writeback retains definition directories and publishes task/random only to candidate owner");
    const auto ordinary = ref::prepare_world_task_creation(startup_world_runtime_factory(original), 1);
    check(ordinary.candidate && ordinary.candidate->selected_definition &&
              (*ordinary.candidate->selected_definition == 33 ||
               *ordinary.candidate->selected_definition == 34),
          "same replay ticket without event60 uses ordinary stage0 catalogue");
    // 明确y边界夹具：开放p不作为本选择器的替代条件。
    for (int id = 0; id < 3; ++id)
        s.scene.world.world.ai.monster_growth.at(id).introduced = false;
    s.scene.world.world.ai.monster_growth.at(3).introduced = true;
    const auto discovered = ref::prepare_world_task_creation(startup_world_runtime_factory(s), 1);
    check(discovered.candidate && discovered.candidate->selected_definition == 38 &&
              s.scene.world.world.ai.monster_growth.at(3).status == 0,
          "current y projection selects rabbit definition38 without substituting monster open status");
    // 原v==1未找到自然启用器；此处仅认证已保留模式的真实目录接线。
    s = original;
    s.task_special_selection = 1;
    s.task_special_selection_index = 5;
    const auto special = ref::prepare_world_task_creation(startup_world_runtime_factory(s), 1);
    check(special.candidate && special.candidate->selected_definition == 80 &&
              !special.candidate->random_bounds.empty() && special.candidate->random_bounds.front() == 30,
          "explicit special mode resolves original sixth boss without ordinary selection draw");
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
StartupWorldRuntimeState task_start_fixture(int first_state) {
    StartupSession startup;
    for (int n = 0; n < 420; ++n)
        check(startup.update() == StartupError::none, "task audio fixture retains actual first visitor initialization");
    StartupWorldRuntimeSession session(startup.state(), ref::WorldRandomStream::from_java_seed(3));
    auto s = session.state();
    const auto task = ref::prepare_world_task_creation(startup_world_runtime_factory(s), 1);
    check(task.candidate && task.candidate->created_task &&
              write_startup_world_runtime_factory(s, task.candidate->state),
          "task audio fixture uses real combat task factory and current source quota");
    s.active_task = task.candidate->created_task;
    s.task = {true, 1, *s.tasks.at(*s.active_task).site, {}};
    auto &world = s.scene.world.world;
    auto &ai = world.ai;
    ai.task_active = true;
    const auto first = ai.human_order.front();
    // 第二个到场人物是明确调用点夹具；任务/遭遇仍由真实工厂建立，不冒充自然招募。
    const ref::CharacterId second{ai.next_actor_id++};
    auto other = ai.battle.actors.at(first);
    other.id = second;
    other.definition = 0;
    other.legacy_id = 1;
    ai.battle.actors.emplace(second, other);
    ai.human_order.push_back(second);
    ai.contexts.emplace(second, ai.contexts.at(first));
    world.actors.emplace(second, world.actors.at(first));
    s.actor_metadata.emplace(second, s.actor_metadata.at(first));
    s.shop_actors.emplace(second, s.shop_actors.at(first));
    s.dungeon_actors.emplace(second, s.dungeon_actors.at(first));
    s.human_presence.at(0) = 1;
    s.participants = {1, 0};
    for (const auto id : ai.human_order) {
        auto &actor = ai.battle.actors.at(id);
        // 原人物c按名单逆序；新追加实例先走本例F/P，原首访同轮随后只绑定。
        actor.control.state = id == second ? first_state : 11;
        actor.control.flags = 2;
        actor.control.queue.clear();
        actor.state_counter = 5;
        actor.position = {s.task.center.x * 100.0f + 50, 0, s.task.center.y * 100.0f + 50};
        actor.encounter.reset();
        auto &ctx = ai.contexts.at(id);
        ctx.cell = s.task.center;
        ctx.half_cell = {s.task.center.x * 2 + 1, s.task.center.y * 2 + 1};
        ctx.inside_town = false;
        ctx.move_area = true;
        ctx.effects.display.clear();
        world.actors.at(id).definition_task_flag = true;
        s.human_flags.at(actor.definition) |= 2U;
    }
    s.sound_requests = {{StartupAudioOperation::ordinary_play, 7}};
    s.scripts.notices.clear();
    return s;
}
void task_start_audio() {
    for (const int first_state : {11, 0}) {
        const auto s = task_start_fixture(first_state);
        const auto adapter = startup_world_runtime_adapter();
        auto actors = adapter.actors;
        actors.other = [&](const auto &owner, const auto &call, const auto &field) {
            return ref::prepare_owned_world_runtime_domain(owner, call, field, adapter);
        };
        const auto first = s.scene.world.world.ai.human_order.back();
        const auto second = s.scene.world.world.ai.human_order.front();
        // arrival_front可在人物c前追加访客；按真实调用的稳定ID定位审计，不假设front就是夹具人物。
        std::vector<ref::CharacterId> decision_actors;
        const auto decision = actors.decision;
        actors.decision = [&](const auto &owner, ref::CharacterId actor) {
            decision_actors.push_back(actor);
            return decision(owner, actor);
        };
        int starts{};
        const auto presentation = actors.presentation;
        actors.presentation = [&](const auto &owner, const auto &request)
            -> std::optional<StartupWorldRuntimeState> {
            if (request.task_encounter_start) {
                ++starts;
                const auto id = *request.task_encounter_start;
                check(request.actor == first && owner.task.encounter == id &&
                          owner.scene.world.world.ai.encounters.at(id).runtime.quota > 0 &&
                          owner.sound_requests == s.sound_requests && owner.scripts.notices.empty(),
                      "presentation bridge publishes real created quota/task before BGM2 and notice24");
            }
            return presentation(owner, request);
        };
        const auto run = ref::prepare_world_actor_schedule(s, {true}, actors);
        check(run.state && starts == 1 && run.decisions.size() >= 2,
              "direct F or path P new task creates one presentation request before later same-round actor");
        const auto &out = *run.state;
        const auto created_at = std::find(decision_actors.begin(), decision_actors.end(), first);
        const auto bound_at = std::find(decision_actors.begin(), decision_actors.end(), second);
        check(decision_actors.size() == run.decisions.size() &&
                  created_at != decision_actors.end() && bound_at != decision_actors.end() &&
                  created_at < bound_at,
              "original reverse human decision order creates encounter before later task participant");
        const auto &daily = run.decisions.at(created_at - decision_actors.begin()).daily;
        const auto &later = run.decisions.at(bound_at - decision_actors.begin()).daily;
        check(daily && ((first_state == 11 && daily->task_entry &&
                        daily->task_entry->created == out.task.encounter) ||
                       (first_state == 0 && daily->path &&
                        daily->path->created_task_encounter == out.task.encounter)) &&
                  out.task.encounter &&
                  out.scene.world.world.ai.battle.actors.at(second).encounter == out.task.encounter,
              "both actual daily carriers retain created identity and later actor binds existing encounter without second start");
        check(later && !later->task_entry && !later->path &&
                  later->state.world.ai.battle.actors.at(second).encounter == out.task.encounter,
              "later actor direct F audit binds the committed encounter without another creation carrier");
        check(out.sound_requests == std::vector<StartupAudioRequest>{
                  {StartupAudioOperation::ordinary_play, 7}, {StartupAudioOperation::replace_bgm, 2}} &&
                  out.scripts.notices.size() == 1 && out.scripts.notices.front().message == 24 &&
                  out.scripts.notices.front().counter == -1 &&
                  out.scripts.notices.front().duration == 80 &&
                  out.scripts.notices.front().text == "战斗任务开始",
              "new encounter appends ordered BGM2 and exact queued notice24 without eager confirmation11");
        const auto zero = ref::prepare_world_notices(out.scripts.notices);
        const auto one = zero ? ref::prepare_world_notices(zero->notices) : std::nullopt;
        check(zero && zero->sounds.empty() && zero->notices.front().counter == 0 &&
                  one && one->sounds == std::vector<int>{11} && one->notices.front().counter == 1,
              "newly connected notice uses original common q counter1 sound rather than creation-time sound");
        ref::WorldActorPresentationRequest invalid;
        invalid.actor = first;
        invalid.task_encounter_start = *out.task.encounter + 1;
        check(!presentation(out, invalid) && out.sound_requests.size() == 2 && out.scripts.notices.size() == 1,
              "stale task start identity explicitly rejects without residual sound or notice");
        // 整轮晚期拒绝：证明已经产生的声音/通知与新遭遇一并留在私有候选。
        starts = 0;
        actors.tail_cache = [](const auto &, auto, const auto &)
            -> std::optional<StartupWorldRuntimeState> { return {}; };
        const auto failed = ref::prepare_world_actor_schedule(s, {true}, actors);
        check(!failed.state && starts == 1 && !s.task.encounter &&
                  s.scene.world.world.ai.encounters.empty() &&
                  s.sound_requests == std::vector<StartupAudioRequest>{{StartupAudioOperation::ordinary_play, 7}} &&
                  s.scripts.notices.empty(),
              "late round failure exposes no partial task encounter, random-owner outputs, BGM2 or notice24");
    }
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
              state.sound_requests.back().id == 1 &&
              state.sound_requests.back().operation == StartupAudioOperation::replace_bgm &&
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
            s.sound_requests = {{StartupAudioOperation::jingle, 5}}; // 既有jingle输出条件夹具。
            auto request = presentation_request(s,Mode::top_only,true);
            request.application_preview = guard == 1;
            request.sound_paused = guard == 2;
            const auto first = prepare_startup_world_presentation(s,request);
            const std::size_t count = counter == 45 && guard == 0 ? 1 : 0;
            check(first.candidate && first.plan && first.plan->sound_requests == count &&
                      test_support::audio_ids(first.candidate->sound_requests) ==
                          (count ? std::vector<int>({5,8}) : std::vector<int>({5})) &&
                      first.candidate->page_counters.at(page) == counter &&
                      first.plan->random_before == first.plan->random_after,
                  "66 literal44/45/46 and app-preview/sound-paused guards emit only counter45 sound8");
            check(first.candidate->sound_requests == (count
                      ? std::vector<StartupAudioRequest>{{StartupAudioOperation::jingle,5},
                                                        {StartupAudioOperation::ordinary_play,8}}
                      : std::vector<StartupAudioRequest>{{StartupAudioOperation::jingle,5}}),
                  "原D5保留在前，66实际C8不能由SE资源通道统一成jingle");
            const auto repeated = prepare_startup_world_presentation(*first.candidate,request);
            check(repeated.candidate && repeated.plan->sound_requests == count &&
                      repeated.candidate->sound_requests.size() == 1 + count * 2 &&
                      repeated.candidate->page_counters.at(page) == counter,
                  "new same-state66 invocation repeats its original outlet, preserving counter and old outputs");
            check(repeated.candidate->sound_requests == (count
                      ? std::vector<StartupAudioRequest>{{StartupAudioOperation::jingle,5},
                                                        {StartupAudioOperation::ordinary_play,8},
                                                        {StartupAudioOperation::ordinary_play,8}}
                      : std::vector<StartupAudioRequest>{{StartupAudioOperation::jingle,5}}),
                  "重复准入C8必须保原序和次数，不能按ID或操作去重");
        }
    }
    auto s = presentation_task();
    const auto page = presentation_gift(s);
    auto request = presentation_request(s,Mode::full_redraw,true);
    const auto valid = prepare_startup_world_presentation(s,request);
    check(valid.candidate && valid.plan && valid.plan->random_after == 2 &&
              valid.plan->sound_requests == 1 && valid.candidate->sound_requests.back().id == 8 &&
              valid.candidate->sound_requests.back().operation == StartupAudioOperation::ordinary_play,
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
        task_new_notices();
        selection_catalogue_consumers();
        crew_item_reward_writeback();
        encounter();
        task_start_audio();
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
