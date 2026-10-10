#include "dungeon_village_prototype/startup_world_projection.hpp"
#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
StartupSession installed() {
    StartupSession session;
    for (int i = 0; i < 420; ++i)
        check(session.update() == StartupError::none, "actual first arrival steps");
    return session;
}
void town_information() {
    auto state=test_support::world_fixture();
    // 只构造统计输入，不宣称从真实经营取得。固定原表无kind9定义，
    // 私有规则变体覆盖原34的kind9分支；不修改原表或创建可运行的演示世界。
    StartupWorldRules rules=*state.rules;
    state.rules=&rules;
    auto type9=std::find_if(rules.facilities.begin(),rules.facilities.end(),[](const auto &d){return d.id==18;});
    check(type9!=rules.facilities.end(),"town statistics fixture uses an existing private definition");
    type9->kind=9;
    for(auto &entry:state.human_presence)entry.second=0;
    for(auto &entry:state.human_homes)entry.second[2]=0;
    state.human_presence.at(1)=1;state.human_presence.at(2)=2;state.human_presence.at(3)=1;
    state.human_homes.at(0)[2]=1;state.human_homes.at(1)[2]=1;state.human_homes.at(2)[2]=2;
    state.rank=3;state.task_progress.successes=12;state.events_held=7;
    auto &world=state.scene.world.world;
    world.facilities.clear();
    const auto add_facility=[&](std::uint64_t id,int definition) {
        const auto d=std::find_if(rules.facilities.begin(),rules.facilities.end(),
                                 [=](const auto &value){return value.id==definition;});
        check(d!=rules.facilities.end(),"statistics fixture retains original facility definitions");
        ref::RescueFacility f;
        f.placement.instance_id={id};f.placement.definition_id=definition;
        f.kind=d->kind;f.category=d->category;f.detail=d->detail;
        world.facilities.emplace(id,f);
    };
    add_facility(1,35);add_facility(2,18);add_facility(3,25);add_facility(4,28);
    state.scene.world.facility_order={3,1,2,4,1}; // 原g按出现次数计，不能偷偷唯一化。
    for(auto &entry:state.catalog)if(entry.first.first>0)entry.second.status=0;
    for(const auto key:{std::pair<int,int>{1,0},{2,45},{2,0},{3,26},{3,29}}) {
        state.catalog.at(key).status=1;state.catalog.at(key).flags=0;
    }
    state.catalog.at({1,1}).status=2;
    state.catalog.at({2,0}).inventory=999;
    const auto before=startup_world_state_digest(state);
    const auto result=startup_town_information(state);
    check(result && result->rank==3 && result->adventurers==2 && result->residents==2 &&
              result->facilities==4 && result->completed_tasks==12 && result->activities_held==7 &&
              result->known_equipment==std::array<int,4>{1,1,1,2},
          "raw34 counts p1, D2==1, ordered kind3/9 occurrences and known equipment without38 flag filter");
    check(startup_world_state_digest(state)==before,
          "town projection keeps NEW, records, random, medals, pages and outputs unchanged");
    const std::array<const char*,11> refusals{{
        "town: missing rules", "town: missing human presence", "town: missing home definition",
        "town: extra human identity", "town: retired facility reference", "town: missing facility definition",
        "town: inconsistent facility kind", "town: missing equipment progress", "town: negative rank",
        "town: negative task total", "town: negative activity total"}};
    for(int fault=0;fault<static_cast<int>(refusals.size());++fault) {
        auto bad=state;
        if(fault==0)bad.rules=nullptr;
        if(fault==1)bad.human_presence.erase(1);
        if(fault==2)bad.human_homes.erase(0);
        if(fault==3)bad.human_presence.emplace(99,1);
        if(fault==4)bad.scene.world.facility_order.push_back(999);
        if(fault==5)bad.scene.world.world.facilities.at(1).placement.definition_id=999;
        if(fault==6)bad.scene.world.world.facilities.at(1).kind=12;
        if(fault==7)bad.catalog.erase({2,45});
        if(fault==8)bad.rank=-1;
        if(fault==9)bad.task_progress.successes=-1;
        if(fault==10)bad.events_held=-1;
        const auto unchanged=startup_world_state_digest(bad);
        check(!startup_town_information(bad) && startup_world_state_digest(bad)==unchanged,
              refusals[static_cast<std::size_t>(fault)]);
    }
    auto duplicated=rules;
    duplicated.humans.push_back(duplicated.humans.front());
    state.rules=&duplicated;
    check(!startup_town_information(state),"duplicate human definitions cannot inflate source counts");
}
void menu_information() {
    auto state=test_support::world_fixture();
    state.village_points=1234;
    const auto idle=startup_menu_information(state);
    check(idle && idle->village_points==1234 && !idle->monster_remaining && !idle->task,
          "menu without selected task projects only town points");
    auto unrelated=state;
    unrelated.rules=nullptr;
    unrelated.scene.world.world.ai.human_order.push_back({999});
    unrelated.task_subperiods=-1;
    check(startup_menu_information(unrelated).has_value(),
          "no selected task does not inspect unrelated roster, definitions or period counter");
    // 定向条件夹具：不声明为自然生成任务或世界长跑。
    state.active_task=1;
    state.next_task_identity=2;
    state.task_order={1};
    state.tasks.emplace(1,ref::DungeonFinishTask{1,35,0,0,{},ref::Position{1,1}});
    state.task_subperiods=4;
    state.task_progress.definitions.at(35).completed=3;
    state.human_profiles.emplace(0,StartupWorldHumanProfile{"测试主角",1,true});
    auto &ai=state.scene.world.world.ai;
    ai.growth.at(0).definition.current_profession=1;
    const auto fallback=startup_menu_information(state);
    check(fallback && fallback->task && fallback->task->type==1 && fallback->task->humans==0 &&
              fallback->task->monsters==0 && fallback->task->residences==0 &&
              fallback->task->remaining_subperiods==8 && fallback->monster_remaining==16 &&
              fallback->task->portrait_definition==0 && fallback->task->portrait_profession==1 &&
              fallback->task->portrait_sex==1 && fallback->task->portrait_body==21 && ai.human_order.empty(),
          "empty roster uses current definition0 female farmer body21; quest35 quota10 plus clear3 bonus6");
    ai.human_order={{10},{11}};
    ai.monster_order={{12}};
    ai.next_actor_id=13;
    for(const auto value:{10,11,12}) {
        ref::BattleActorRecord actor;
        actor.id={static_cast<std::uint64_t>(value)};
        actor.kind=value==12?ref::ActorKind::monster:ref::ActorKind::human;
        actor.definition=value==10?1:0;
        actor.hp.target=0; // 完整名单含倒下人物和待清理怪物，不按HP过滤。
        ai.battle.actors.emplace(actor.id,actor);
    }
    ai.growth.at(1).definition.current_profession=0;
    ref::RescueFacility residence;
    residence.placement.instance_id={100};residence.placement.definition_id=25;residence.kind=12;
    state.scene.world.world.facilities.emplace(100,residence);
    state.scene.world.facility_order.push_back(100);
    state.next_facility_identity=101;
    ref::RewardEncounter event;
    event.runtime.id=7;event.runtime.spawned=7;event.linked_monsters=2;
    ai.encounters.emplace(7,event);ai.encounter_order.push_back(7);
    state.task.encounter=7;
    const auto before=startup_world_state_digest(state);
    const auto draws=state.scene.random.draws();
    const auto projected=startup_menu_information(state);
    check(projected && projected->task && projected->monster_remaining==11 &&
              projected->task->humans==2 && projected->task->monsters==1 && projected->task->residences==1 &&
              projected->task->portrait_definition==1 && projected->task->portrait_profession==0 &&
              projected->task->portrait_sex==0 && projected->task->portrait_body==14,
          "menu uses full rosters, current first actor job and encounter cache2, not actual monster count1");
    check(startup_world_state_digest(state)==before && state.scene.random.draws()==draws,
          "menu projection does not mutate Owner, outputs, references or random");
    auto bound=state;
    bound.tasks.at(1).facility=100;
    bound.task.encounter=999; // 隐藏怪物栏时不读取无关遭遇；四摘要仍有真实来源。
    const auto hidden=startup_menu_information(bound);
    check(hidden && !hidden->monster_remaining && hidden->task && hidden->task->humans==2,
          "actual nonempty facility reference hides monster HUD regardless of task kind1");
    auto type0=state;
    type0.tasks.at(1).definition=0;
    type0.task.encounter.reset();
    type0.task_subperiods=13;
    const auto exploration=startup_menu_information(type0);
    check(exploration && exploration->task && exploration->task->type==0 &&
              exploration->monster_remaining==7 && exploration->task->remaining_subperiods==-1,
          "kind0 with null facility also shows monster HUD; remaining period is not clamped");
    const std::array<const char*,23> refusals{{
        "menu missing rules", "menu missing selected task", "menu duplicate selected task order",
        "menu missing selected task order", "menu mismatched task identity", "menu missing first actor",
        "menu duplicate human roster", "menu wrong actor kind", "menu missing monster actor",
        "menu duplicate monster roster", "menu missing growth", "menu invalid current profession",
        "menu invalid profile sex", "menu retired facility", "menu duplicate facility order",
        "menu missing facility definition", "menu facility kind mismatch", "menu missing task progress",
        "menu missing encounter", "menu retired encounter", "menu invalid encounter cache",
        "menu invalid period", "menu invalid village points"}};
    for(int fault=0;fault<static_cast<int>(refusals.size());++fault) {
        auto bad=state;
        auto &broken=bad.scene.world.world.ai;
        if(fault==0)bad.rules=nullptr;
        if(fault==1)bad.tasks.erase(1);
        if(fault==2)bad.task_order.push_back(1);
        if(fault==3)bad.task_order.clear();
        if(fault==4)bad.tasks.at(1).identity=2;
        if(fault==5) {
            broken.retired_actors.emplace(ref::CharacterId{10},broken.battle.actors.at({10}));
            broken.battle.actors.erase({10});
        }
        if(fault==6)broken.human_order.push_back({10});
        if(fault==7)broken.battle.actors.at({10}).kind=ref::ActorKind::monster;
        if(fault==8)broken.battle.actors.erase({12});
        if(fault==9)broken.monster_order.push_back({12});
        if(fault==10)broken.growth.erase(1);
        if(fault==11)broken.growth.at(1).definition.current_profession=-1;
        if(fault==12)bad.human_profiles.at(0).sex=2;
        if(fault==13)bad.scene.world.world.facilities.erase(100);
        if(fault==14)bad.scene.world.facility_order.push_back(100);
        if(fault==15)bad.scene.world.world.facilities.at(100).placement.definition_id=999;
        if(fault==16)bad.scene.world.world.facilities.at(100).kind=3;
        if(fault==17)bad.task_progress.definitions.erase(35);
        if(fault==18)broken.encounters.erase(7);
        if(fault==19) {
            broken.retired_encounters.emplace(7,broken.encounters.at(7));
            broken.encounters.erase(7);
        }
        if(fault==20)broken.encounters.at(7).linked_monsters=-1;
        if(fault==21)bad.task_subperiods=-1;
        if(fault==22)bad.village_points=-1;
        const auto unchanged=startup_world_state_digest(bad);
        check(!startup_menu_information(bad) && startup_world_state_digest(bad)==unchanged,
              refusals[static_cast<std::size_t>(fault)]);
    }
    auto overflow=state;
    overflow.scene.world.world.ai.encounters.at(7).linked_monsters=std::numeric_limits<int>::max();
    check(!startup_menu_information(overflow),"menu quota plus encounter cache overflow rejects without wrap");
    auto stale=bound;
    stale.tasks.at(1).facility=999;
    check(!startup_menu_information(stale),"bound retired facility cannot silently hide monster HUD");
    auto missing_definition=*state.rules;
    missing_definition.tasks.erase(missing_definition.tasks.begin()+35);
    stale=state;stale.rules=&missing_definition;
    check(!startup_menu_information(stale),"selected task definition missing rejects");
    auto invalid_body=*state.rules;
    invalid_body.jobs[0].sprites[0]=32;
    stale=state;stale.rules=&invalid_body;
    check(!startup_menu_information(stale),"unpublished body32 rejects before emitting an unusable portrait");
    // 真实工厂生成的探险设施/任务仍可投影，选中标志是本查询的局部调用点夹具。
    auto generated=test_support::world_fixture();
    const auto created=ref::prepare_world_task_creation(startup_world_runtime_factory(generated),0);
    check(created.candidate && created.candidate->created_task &&
              write_startup_world_runtime_factory(generated,created.candidate->state),
          "menu fixture uses actual task factory and Owner writeback for dungeon references");
    generated.active_task=created.candidate->created_task;
    const auto generated_before=startup_world_state_digest(generated);
    const auto dungeon=startup_menu_information(generated);
    check(dungeon && dungeon->task && dungeon->task->type==0 && !dungeon->monster_remaining &&
              generated.tasks.at(*generated.active_task).facility &&
              generated.dungeon_facilities.count(*generated.tasks.at(*generated.active_task).facility) &&
              startup_world_state_digest(generated)==generated_before,
          "real generated dungeon facility/site yields four summaries without monster HUD or writes");
}
void catalogue() {
    const auto &rules = startup_world_rules();
    check(rules.humans.size() == 25 && rules.jobs.size() == 23 && rules.equipment.size() == 113 &&
              rules.monsters.size() == 36 && rules.items.size() == 36 && rules.tasks.size() == 81 &&
              rules.facilities.size() == 85,
          "full original catalogue counts compiled, not initial subset");
    check(rules.equipment[0].battle.kind == 0 && rules.equipment[0].battle.range == 130 &&
              rules.equipment[0].battle.combo == 5 && rules.equipment[0].battle.miss_low == 3 &&
              rules.equipment[0].battle.miss_high == 15,
          "actual shortsword complete battle rule");
    check(rules.tasks[0].recruitment_fee == 1300 && rules.tasks[0].crew_rating_penalty == 1500,
          "questData constructor columns6/15 publish fee and display-only rating penalty");
    check(rules.facilities[54].exit_effects.size() == 1 &&
              rules.facilities[54].exit_effects[0].attribute_index == 5 &&
              rules.unconsumed_exit_deltas.at(54) == std::vector<int>{3} &&
              rules.unconsumed_exit_deltas.size() == 4,
          "fixed source longA shortz tail preserved; consumed effect not fabricated");
    check(rules.fences[0][0] == ref::Position{6, 10} && rules.fences[0][1] == ref::Position{17, 2},
          "published raw regions independently flip bothY endpoints");
    check(rules.monsters[0].initial.base_hp == 16 &&
              rules.monsters[0].initial.base_cash_reward == 50 &&
              rules.monsters[0].initial.base_death_reward == 4 &&
              rules.monsters[0].initial.newly_unlocked && rules.monsters[0].initial.introduced &&
              !rules.monsters[3].initial.introduced,
          "monster.b source flags reset distinguishes pending notice and introduced");
}
void reset() {
    StartupSession session;
    const auto random = ref::WorldRandomStream::from_raw({7, 8, 9});
    const auto result = prepare_startup_world_projection(session.state(), random);
    check(result.candidate.has_value(), "actual reset projects without visitor");
    const auto &c = *result.candidate;
    const auto &world = c.routes.world;
    check(world.ai.human_order.empty() && world.ai.monster_order.empty() &&
              world.ai.battle.actors.empty() && world.ai.battle.objects.empty() &&
              world.ai.encounters.empty(),
          "source reset has genuinely empty live roster/object/event, not invented actor");
    check(world.ai.growth.size() == 25 && world.ai.monster_growth.size() == 36 &&
              world.ai.professions.size() == 23 && c.routes.catalog.size() == 149 &&
              c.routes.items.size() == 36 && world.facility_uses.size() == 85,
          "all shared definitions initialized even without current actor or facility");
    check(world.ai.accounting.funds() == 5000 && world.ai.accounting.village_points() == 10 &&
              c.popularity == 50 && c.calendar == std::array<int, 4>{0, 3, 0, 0} &&
              c.arrival_counter == 420 && c.ground_definition == 17 &&
              c.special_ground_definition == 19,
          "published resources/date and real first-kind S/T, not lastkind terrain");
    check(c.human_presence.at(1) == 1 && c.human_presence.at(2) == 1 &&
              c.human_presence.at(3) == 1 && c.human_presence.at(4) == 0 &&
              world.ai.growth.at(1).notice_attributes == std::array<std::array<int, 2>, 4>{},
          "source p1 unlocks and newly allocated ap zeros preserved");
    for (const auto &entry : c.human_homes)
        check(entry.second == std::array<int, 4>{} &&
                  c.routes.human_definition_state.at(entry.first) == 0,
              "all definitions D and m source reset zero, not guessed home");
    check(c.routes.random.draws() == 0 && random.draws() == 0,
          "projection consumes no random and leaves caller stream unchanged");
    for (std::size_t n = 0; n < session.state().loaded_map.instances.size(); ++n) {
        const auto &old = session.state().loaded_map.instances[n];
        const auto identity = static_cast<std::uint64_t>(old.legacy_id) + 1;
        check(c.facility_order[n] == identity &&
                  c.facility_original_ids.at(identity) == old.legacy_id &&
                  c.facility_residents.at(identity) == -1 &&
                  world.facilities.at(identity).placement.anchor == old.anchor &&
                  world.facilities.at(identity).occupants.empty(),
              "original facility order/raw0 identities/t-1 and empty occupations preserved");
    }
    for (std::size_t n = 0; n < session.state().loaded_map.cells.size(); ++n) {
        const auto &old = session.state().loaded_map.cells[n];
        check(c.routes.facts.surface[n] == static_cast<int>(old.category) &&
                  c.routes.facts.flags[n] == 0 &&
                  world.map.cells[n].legacy_state == old.legacy_state &&
                  c.surface[n].definition == old.definition_id &&
                  c.surface[n].instance == old.external_direction &&
                  c.surface[n].variant == old.variant &&
                  c.road_patches[n] == std::array<bool, 2>{old.road_quad, old.edge_road_pair},
              "all576 cells preserve c/i.m external direction separately from facility identity "
              "and visual variants");
    }
}
void first() {
    const auto session = installed();
    const auto result = prepare_startup_world_projection(session.state(), ref::WorldRandomStream{});
    check(result.candidate.has_value(), "source first visitor projects without running another AI");
    const auto &c = *result.candidate;
    const auto &world = c.routes.world;
    const auto &old = *session.state().character;
    const auto &actor = world.ai.battle.actors.at({1});
    const auto &dungeon = c.routes.dungeon_actors.at({1});
    check(dungeon.progress == 0 && dungeon.previous == 0 && dungeon.percent == 0 &&
              dungeon.endurance == 0 && dungeon.retreat_state == 0 &&
              dungeon.retreat_updates == 0 && !dungeon.constrained,
          "new Character dungeon fields preserve Java zero; endurance belongs to actual entry");
    check(actor.definition == 1 && actor.legacy_id == 0 && actor.control.state == 0 &&
              actor.control.flags == (2U | 8192U) &&
              actor.control.queue == std::vector<ref::LegacyActorControl>{{8, 0}} &&
              actor.hp.target == 22 && actor.hp.displayed == 22 && actor.capacity == 22,
          "actual first uid0 flags/queued activity/hp22 on stable1");
    check(actor.position.x == old.cell.x * 100.0F + 50.0F &&
              actor.position.z == old.cell.y * 100.0F + 50.0F &&
              world.ai.contexts.at({1}).cell == old.cell &&
              world.ai.contexts.at({1}).half_cell == ref::Position{old.cell.x * 2 + 1, 1},
          "source n/a initializes world n and cacheds/t, no alternate birth random");
    check(actor.attack_position.x == 0 && actor.attack_position.z == 0 &&
              actor.attack_position.height == 0,
          "unrun first-visitor d preserves constructor au0; n/a only initializes n/s/t/u/v");
    check(!actor.attack_armed && actor.combo_count == 0 && actor.perceived_distance == 0 &&
              !world.ai.contexts.at({1}).inside_town && !world.ai.contexts.at({1}).move_area &&
              world.actors.at({1}).destination == std::optional<ref::Position>({0, 0}) &&
              !world.actors.at({1}).binding && !actor.encounter,
          "unrun c retains Java0 ai/y/aA/ax/aB and allocated O0; no eager perception or binding");
    check(c.routes.shop_humans.at(1).reselect[0] == 6 && c.routes.shop_actors.at({1}).weapon == 0 &&
              world.ai.growth.at(1).derived.combat == old.combat,
          "source initial equip and shared derived stats not inventory substitution");
    check(c.actor_metadata.at({1}).profession == old.job_id && old.job_id == 1 &&
              c.actor_metadata.at({1}).sex == old.sex && c.event89_count == 1 &&
              session.state().mode == StartupMode::tutorial && world.ai.accounting.funds() == 5000,
          "projection preserves installed/tutorial chronology, never advances script/date/world");
    auto corrupt = session.state();
    corrupt.character->combat[0]++;
    check(prepare_startup_world_projection(corrupt, {}).error ==
              StartupWorldProjectionError::invalid_snapshot,
          "corrupt first stats rejects rather than correcting evidence");
    corrupt = session.state();
    corrupt.terrain_edits[0] = 27;
    check(prepare_startup_world_projection(corrupt, {}).error ==
              StartupWorldProjectionError::invalid_snapshot,
          "modified old startup map cannot become a fake canonical full world");
    corrupt = session.state();
    corrupt.loaded_map.cells[0].variant++;
    check(prepare_startup_world_projection(corrupt, {}).error ==
              StartupWorldProjectionError::source_mismatch,
          "modified reset snapshot rejects even with valid dimensions and bindings");
}
} // namespace
int main() {
    try {
        catalogue();
        reset();
        first();
        town_information();
        menu_information();
        std::cout << "startup world projection checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
