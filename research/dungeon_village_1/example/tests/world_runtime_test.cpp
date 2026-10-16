#include "dungeon_village_reference/world_runtime.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
std::string table(const char *name) {
    const std::string source = __FILE__;
    const auto root =
        source.substr(0, source.find_last_of("/\\")) + "/../../data/scripts/original/";
    std::ifstream stream(root + name, std::ios::binary);
    if (!stream)
        throw std::runtime_error("missing published script table");
    return {std::istreambuf_iterator<char>(stream), {}};
}
// 显式空村组合夹具，不是原新局、有人物的完整世界或窗口轨迹认证。
// projection字段的共享子对象始终清空；真正world/random/script事实仅各保存一次。
struct Owner {
    WorldSceneState scene;
    WorldRandomStream random;
    WorldScriptState scripts;
    WorldMonthReportState report;
    WorldCalendarTasksState tasks;
    WorldCalendarMaintenanceState maintenance;
    WorldPopularityState popularity;
    WorldFacilityUpdateState facilities;
    std::vector<WorldSceneStage> calls;
    std::vector<WorldCalendarStage> calendars;
    int entries{};
    int arrivals{};
};
WorldScriptState scripts(const Owner &owner) {
    auto s = owner.scripts;
    s.scene_mode = owner.scene.scene_state;
    s.scene_updates = owner.scene.scene_counter;
    s.pending_completion = owner.scene.world.world.ai.pending_completion;
    s.popularity_queue = owner.scene.world.popularity_queue;
    s.human_order.clear();
    for (const auto id : owner.scene.world.world.ai.human_order)
        s.human_order.push_back(id.value);
    s.finance->month = owner.scene.calendar.month;
    return s;
}
bool write_scripts(Owner &owner, const WorldScriptState &s) {
    owner.scripts = s;
    owner.scene.scene_state = s.scene_mode;
    owner.scene.scene_counter = s.scene_updates;
    owner.scene.world.world.ai.pending_completion = s.pending_completion;
    owner.scene.world.popularity_queue = s.popularity_queue;
    owner.scripts.scene_mode = 0;
    owner.scripts.scene_updates = 0;
    owner.scripts.pending_completion = 0;
    owner.scripts.popularity_queue.clear();
    owner.scripts.human_order.clear();
    return true;
}
DungeonFinishState finish(const Owner &o) {
    auto f = o.tasks.finish;
    f.dungeon.world = o.scene.world.world;
    f.event_calls = o.scripts.event_calls;
    f.raw_year = o.scene.calendar.year;
    f.raw_month = o.scene.calendar.month;
    return f;
}
Owner fixture(const WorldScriptCatalog &catalog) {
    Owner o;
    o.scene.world.world.map = {6, 6, std::vector<LegacyMapCell>(36)};
    o.scene.world.surface.assign(36, 1);
    o.scene.world.map_flags.assign(36, 0);
    o.scene.world.town = {0, 5, 0, 5};
    o.scene.calendar = {0, 3, 0, 0, 0, 0};
    o.random = WorldRandomStream::from_java_seed(4);
    o.scripts.finance = WorldScriptFinance{};
    o.scripts.finance->cash = 5000;
    o.scripts.finance->cash_peak = 5000;
    o.scripts.finance->localized_gold_template = "G";
    WorldScriptPage scene_page;
    scene_page.id = 1;
    o.scripts.pages = {scene_page};
    o.scripts.next_page_id = 2;
    // 只隔离自动页面门槛，不把这些已见标志冒充真实新局。
    for (const auto &event : catalog.events)
        o.scripts.event_calls[event.first] = 1;
    o.tasks.rank_terms = fixed_calendar_task_rank_terms();
    o.tasks.rank = 5; // 隔离本fixture等级页，不使用该数值作新局默认。
    return o;
}
WorldRuntimeAdapter<Owner> adapter(const WorldScriptCatalog &catalog) {
    WorldRuntimeAdapter<Owner> a;
    a.catalog = catalog;
    a.scene.read = [](const Owner &o) { return o.scene; };
    a.scene.write = [](Owner &o, const WorldSceneState &s) {
        o.scene = s;
        return true;
    };
    a.scripts = {scripts, write_scripts};
    a.read_random = [](const Owner &o) -> const WorldRandomStream & { return o.random; };
    a.write_random = [](Owner &o) -> WorldRandomStream & { return o.random; };
    a.normal_conditions = [](const Owner &o) {
        return std::optional<OwnedWorldSceneStep<Owner>>{{o}};
    };
    a.entry.read = [](const Owner &o) {
        WorldWorldEntryState e;
        e.finish = finish(o);
        e.scripts = scripts(o);
        e.random = o.random;
        e.updates = e.global_updates = o.entries;
        e.town = o.scene.world.town;
        e.generation_bounds = {{{0, 6}, {6, 0}}};
        return e;
    };
    a.entry.write = [](Owner &o, const WorldWorldEntryState &e) {
        o.entries = e.updates;
        o.scene.world.world = e.finish.dungeon.world;
        return write_scripts(o, e.scripts);
    };
    a.before_common = [](const Owner &o) { return std::optional<Owner>(o); };
    a.arrival = [](const Owner &o) {
        auto n = o;
        ++n.arrivals;
        return std::optional<Owner>(n);
    };
    a.report.read = [](const Owner &o) {
        auto r = o.report;
        r.presentation = scripts(o);
        return r;
    };
    a.report.write = [](Owner &o, const WorldMonthReportState &r) {
        o.report = r;
        o.report.presentation = {};
        return write_scripts(o, r.presentation);
    };
    a.report_input = [](const Owner &o) {
        return std::optional<WorldMonthReportInput>{
            {o.scene.calendar.month_ticks, 80, o.scene.calendar.month, true, false}};
    };
    a.actors.read_common = [](const Owner &o) -> const WorldScheduleState & {
        return o.scene.world;
    };
    a.actors.write_common = [](Owner &o) -> WorldScheduleState & { return o.scene.world; };
    a.actors.read_routes = [](const Owner &o) {
        WorldActorRoutesState r;
        r.world = o.scene.world.world;
        r.facts = world_schedule_facts(o.scene.world);
        r.random = o.random;
        return r;
    };
    a.actors.write_routes = [](Owner &, const WorldActorRoutesState &) { return true; };
    a.actors.decision = [](const Owner &, CharacterId) -> std::optional<WorldActorDecisionInput> {
        return {};
    };
    a.nonactors.read_common = a.actors.read_common;
    a.nonactors.write_common = a.actors.write_common;
    a.nonactors.read_routes = [](const Owner &o) {
        WorldNonactorScheduleState n;
        n.common = o.scene.world;
        n.random = o.random;
        return n;
    };
    a.nonactors.write_routes = [](Owner &o, const WorldNonactorScheduleState &n) {
        o.random = n.random;
        return true;
    };
    a.maintenance.read = [](const Owner &o) {
        auto m = o.maintenance;
        m.random = o.random;
        m.monthly_cash = o.scripts.finance->monthly_totals;
        m.rank = o.tasks.rank;
        m.popularity = o.popularity.popularity;
        m.event203_seen = world_script_seen(o.scripts, 203);
        return m;
    };
    a.maintenance.write = [](Owner &o, const WorldCalendarMaintenanceState &m) {
        o.maintenance = m;
        o.maintenance.random = {};
        o.scripts.finance->monthly_totals = m.monthly_cash;
        return true;
    };
    a.tasks.read = [](const Owner &o) {
        auto t = o.tasks;
        t.finish = finish(o);
        t.scripts = scripts(o);
        t.random = o.random;
        t.facility_order = o.scene.world.facility_order;
        t.popularity = o.popularity.popularity;
        return t;
    };
    a.tasks.write = [](Owner &o, const WorldCalendarTasksState &t) {
        o.tasks = t;
        RescueWorldState empty_world;
        o.tasks.finish.dungeon.world = std::move(empty_world);
        o.tasks.finish.event_calls.clear();
        o.tasks.scripts = {};
        o.tasks.random = {};
        o.scene.world.world = t.finish.dungeon.world;
        return write_scripts(o, t.scripts);
    };
    a.calendar_other = [](const Owner &o, WorldCalendarStage stage) {
        auto n = o;
        n.calendars.push_back(stage);
        return std::optional<Owner>(n); // 保存/刷新观测夹具，不认证实际持久与镜头。
    };
    a.scene_other = [](const Owner &o, const WorldSceneCall &call) {
        auto n = o;
        n.calls.push_back(call.stage);
        return std::optional<OwnedWorldSceneStep<Owner>>{{n}}; // 明确无玩家输入的表现夹具。
    };
    return a;
}
void source_scan(const WorldScriptCatalog &catalog) {
    auto o = fixture(catalog);
    o.scripts.event_calls.erase(7);
    auto a = adapter(catalog);
    const auto original = o;
    auto r = prepare_owned_world_runtime(o, {27}, a);
    check(r.state && r.state->scripts.event_calls.at(7) == 1 &&
              r.state->scripts.continuations[0].remaining_updates == 3 &&
              r.state->scene.calendar.month_ticks == 0 && r.state->entries == 0,
          "automatic7 L159 skips freshly appended wait/world/calendar in same round");
    o = *r.state;
    for (int i = 0; i < 2; ++i) {
        r = prepare_owned_world_runtime(o, {27}, a);
        check(r.state.has_value(), "waiting round reaches actual common world");
        o = *r.state;
    }
    check(o.scripts.continuations[0].remaining_updates == 1 && o.scene.world.updates == 2,
          "two following main updates decrement before world");
    r = prepare_owned_world_runtime(o, {27}, a);
    check(r.state && r.state->scripts.continuations.empty() && r.state->scene.world.updates == 2 &&
              r.state->scene.calendar.month_ticks == 2 && r.state->scripts.pages.size() > 1,
          "first expiry resumes original talk9 then skips world/date irrespective unchanged mode");
    check(original.scripts.event_calls.count(7) == 0 && original.scene.world.updates == 0,
          "real original input remains unchanged");
    o = fixture(catalog);
    o.scene.speed_setting = 1;
    o.scripts.event_calls.erase(7);
    r = prepare_owned_world_runtime(o, {27}, a);
    check(r.state && r.scene->scheduled_rounds == 2 && r.state->scene.world.updates == 1 &&
              r.state->scripts.continuations[0].remaining_updates == 2,
          "L159 saves speed second round which sees liveaM and decrements newly appended wait");
}
void loop_and_rollback(const WorldScriptCatalog &catalog) {
    auto o = fixture(catalog);
    auto a = adapter(catalog);
    for (int i = 0; i < 1600; ++i) {
        const auto r = prepare_owned_world_runtime(o, {27}, a);
        check(r.state.has_value(), "empty fixture continuous original27 calendars/common updates");
        o = *r.state;
    }
    check(o.scene.calendar.month == 4 && o.scene.calendar.subperiod == 0 &&
              o.scene.calendar.month_ticks == 0 && o.scene.world.updates == 1600 &&
              o.entries == 1600 && o.arrivals == 1600 && o.report.display_state == 0 &&
              o.scripts.notices.size() == 1,
          "month report1457 and70+70 overlays settle before actual1600 month boundary");
    check(o.random.draws() == 0 && o.report.presentation.pages.empty() &&
              o.tasks.finish.dungeon.world.map.cells.empty(),
          "no lazy draws with no consumers; no persisted second script/world projection");
    auto boundary = fixture(catalog);
    boundary.scene.calendar = {0, 3, 3, 10790, 0, 1599};
    a.tasks = {};
    const auto failed = prepare_owned_world_runtime(boundary, {27}, a);
    check(!failed.state && failed.worlds.empty() && boundary.scene.world.updates == 0 &&
              boundary.entries == 0 && boundary.random.draws() == 0 &&
              boundary.scene.calendar.units == 10790,
          "missing late month task consumer rolls back completed earlier world and date");
    a = adapter(catalog);
    a.nonactors.read_routes = {};
    const auto missing = prepare_owned_world_runtime(fixture(catalog), {27}, a);
    check(!missing.state && missing.world_error == WorldScheduleError::consumer_failed,
          "missing actualL consumer cannot be empty successful finalize");
    a = adapter(catalog);
    auto paused = fixture(catalog);
    paused.scene.top_is_main = false;
    a.entry = {};
    const auto idle = prepare_owned_world_runtime(paused, {27}, a);
    check(idle.state && idle.state->scene.frame_counter == 0 && idle.state->entries == 0,
          "framework non-main admission is exact freeze not missing unused callback failure");
}
void combat_observation_rollback(const WorldScriptCatalog &catalog) {
    auto source=fixture(catalog);
    source.scene.speed_setting=1; // 两个已获准轮，不重放自然经营前缀。
    auto &ai=source.scene.world.world.ai;
    for(const std::uint64_t id:{1ULL,2ULL}) {
        BattleActorRecord actor;actor.id={id};actor.definition=int(id);
        actor.kind=id==1?ActorKind::human:ActorKind::monster;
        actor.control.state=1;actor.control.flags=2;
        actor.position=actor.decision_start={20,0,0};
        actor.hp={0,500,500,500,false,0};actor.capacity=500;
        ai.retired_actors.emplace(actor.id,actor);
        ai.contexts.emplace(actor.id,RewardActorContext{{1,1},true,{}, {},true,{2,2}});
    }
    ai.battle.humans.emplace(1,HumanBattleRecord{});
    ai.battle.monsters.emplace(2,MonsterBattleRecord{});
    RewardHumanDefinition human;human.derived.combat={500,100,100,100};ai.growth.emplace(1,human);
    RewardMonsterDefinition monster;monster.base_hp=500;monster.base_attack=100;monster.base_defense=50;
    ai.monster_growth.emplace(2,monster);
    ProjectileState delayed;delayed.kind=ProjectileKind::delayed_damage;
    delayed.caster={1};delayed.original_target={2};delayed.damage=7;delayed.delay=delayed.counter=6;
    ai.projectiles.emplace(9,delayed);ai.projectile_order={9};ai.next_projectile_id=10;
    auto a=adapter(catalog);a.observe_spell_consumption=true;
    // 仅已到期投射与保留引用的条件夹具；不认证取得法术或自然发射来源。
    a.nonactors.projectile=[](const Owner &,std::uint64_t id)->std::optional<WorldProjectileInput> {
        WorldProjectileInput input;input.projectile=id;return input;
    };
    a.nonactors.primary_expression_table=true;
    a.nonactors.request=[](Owner &current,const WorldNonactorRequest &request)
        ->std::optional<WorldNonactorWriteback> {
        if(request.kind!=WorldNonactorRequestKind::projectile_hit||!request.hit)return {};
        WorldNonactorWriteback output{encounter_external_writeback(current.scene.world.world.ai),{}};
        if(request.hit->kind==HitRequestKind::face_attacker)output.target_facing=0;
        return output;
    };
    const auto observed=prepare_owned_world_runtime(source,{27},a);
    check(observed.state&&observed.worlds.size()==2&&
          observed.worlds[0].spell_consumption.size()==1&&observed.worlds[1].spell_consumption.empty()&&
          observed.worlds[0].spell_consumption[0].kind==SpellConsumptionKind::delayed_hit&&
          observed.worlds[0].spell_consumption[0].projectile==9,
          "retained type2 fixture emits one first-round receipt; second round does not replay it");
    auto disabled=a;disabled.observe_spell_consumption=false;disabled.nonactors.observe_spell_consumption=true;
    const auto silent=prepare_owned_world_runtime(source,{27},disabled);
    check(silent.state&&silent.worlds.size()==2&&silent.worlds[0].spell_consumption.empty()&&
          silent.worlds[1].spell_consumption.empty()&&silent.state->random.draws()==observed.state->random.draws(),
          "runtime switch is authoritative over nested adapter and preserves successful random sequence");
    bool reached_second{};
    auto reject=a;reject.before_common=[&](const Owner &current)->std::optional<Owner> {
        if(current.scene.world.updates==1) {reached_second=true;return {};}
        return current;
    };
    const auto second_failed=prepare_owned_world_runtime(source,{27},reject);
    check(reached_second&&!second_failed.state&&second_failed.worlds.empty()&&
          source.scene.world.world.ai.projectiles.size()==1&&source.scene.world.updates==0&&
          source.scene.world.world.ai.retired_actors.at({2}).hp.target==500&&source.random.draws()==0,
          "second-round failure clears prior spell receipts together with private HP,projectile,date and random");
    auto boundary=source;boundary.scene.speed_setting=0;
    boundary.scene.calendar={0,3,3,10790,0,1599};reject=a;reject.tasks={};
    const auto calendar_failed=prepare_owned_world_runtime(boundary,{27},reject);
    check(!calendar_failed.state&&calendar_failed.worlds.empty()&&
          boundary.scene.world.world.ai.projectiles.size()==1&&boundary.scene.calendar.units==10790&&
          boundary.scene.world.world.ai.retired_actors.at({2}).hp.target==500,
          "late calendar refusal clears first-world spell receipt instead of publishing a partial frame");

    // 箭矢须扫描真实活跃怪物；状态7、空命令及保留遭遇引用隔离AI动作和遭遇调度。
    auto arrow=source;
    auto &arrow_ai=arrow.scene.world.world.ai;
    auto target=arrow_ai.retired_actors.at({2});target.control.state=7;target.encounter=99;
    RewardEncounter retained_encounter;retained_encounter.runtime.id=99;
    retained_encounter.members={target.id};arrow_ai.retired_encounters.emplace(99,retained_encounter);
    arrow_ai.retired_actors.erase({2});arrow_ai.battle.actors.emplace(target.id,target);
    arrow_ai.monster_order={target.id};arrow.scene.world.world.actors.emplace(target.id,RescueActorContext{});
    const auto projectile=prepare_projectile(ProjectileKind::arrow,{1},{2},{},{1000,0,0},0);
    check(projectile.candidate.has_value(),"physical runtime fixture creates an actual arrow");
    arrow_ai.projectiles.at(9)=*projectile.candidate;
    auto physical=adapter(catalog);
    physical.actors.decision=[](const Owner &,CharacterId id)->std::optional<WorldActorDecisionInput> {
        WorldActorDecisionInput input;input.actor=id;return input;
    };
    physical.nonactors.projectile=[](const Owner &,std::uint64_t id)->std::optional<WorldProjectileInput> {
        WorldProjectileInput input;input.projectile=id;input.box=CollisionBox{-1,0,2,2};
        input.monster_boxes[0]=CollisionBox{-10,10,20,20};
        input.resolved_weapon_definition=12;return input;
    };
    physical.nonactors.primary_expression_table=true;
    physical.nonactors.request=[](Owner &current,const WorldNonactorRequest &request)
        ->std::optional<WorldNonactorWriteback> {
        if(request.kind!=WorldNonactorRequestKind::projectile_hit&&
           request.kind!=WorldNonactorRequestKind::projectile_contact)return {};
        WorldNonactorWriteback output{encounter_external_writeback(current.scene.world.world.ai),{}};
        if(request.hit&&request.hit->kind==HitRequestKind::face_attacker)output.target_facing=0;
        if(request.kind==WorldNonactorRequestKind::projectile_contact) {
            if(!request.target||!request.source_position)return {};
            auto effects=current.scene.world.world.ai.contexts.at(*request.target).effects;
            effects.display.push_back({10,0,0,0,0}); // typed接触显示消费者，不代原素材位置。
            output.target_effects=std::move(effects);
        }
        return output;
    };
    const auto default_physical=prepare_owned_world_runtime(arrow,{27},physical);
    physical.observe_physical_consumption=true;
    const auto hit=prepare_owned_world_runtime(arrow,{27},physical);
    check(default_physical.state&&hit.state&&hit.worlds.size()==2&&
          default_physical.worlds[0].physical_consumption.empty()&&
          hit.worlds[0].physical_consumption.size()==1&&hit.worlds[1].physical_consumption.empty(),
          "physical observations default off and forward the actual first-round arrow hit only once");
    const auto &receipt=hit.worlds[0].physical_consumption.front();
    check(receipt.kind==PhysicalConsumptionKind::arrow_hit&&receipt.attacker==CharacterId{1}&&
          receipt.actual_target==CharacterId{2}&&receipt.projectile==9&&receipt.weapon_definition==12&&
          receipt.prior_launch_unavailable&&receipt.landed==true&&receipt.amount&&*receipt.amount>0&&
          receipt.target_delta&&receipt.target_delta->hp_before==500&&
          receipt.target_delta->hp_after==500-*receipt.amount&&
          hit.state->scene.world.world.ai.battle.actors.at({2}).hp.target==receipt.target_delta->hp_after&&
          hit.state->scene.world.world.ai.projectiles.empty(),
          "outer physical vector contains the real collision consumer and its committed local HP delta");
    auto physical_disabled=physical;physical_disabled.observe_physical_consumption=false;
    physical_disabled.nonactors.observe_physical_consumption=true;
    const auto physical_silent=prepare_owned_world_runtime(arrow,{27},physical_disabled);
    check(physical_silent.state&&physical_silent.worlds[0].physical_consumption.empty()&&
          physical_silent.worlds[1].physical_consumption.empty()&&
          physical_silent.state->scene.world.world.ai.battle.actors.at({2}).hp.target==
              hit.state->scene.world.world.ai.battle.actors.at({2}).hp.target&&
          physical_silent.state->scene.world.world.ai.projectiles.empty()&&
          physical_silent.state->scene.world.updates==hit.state->scene.world.updates&&
          physical_silent.state->scene.calendar.units==hit.state->scene.calendar.units&&
          physical_silent.state->random.snapshot().engine_state==hit.state->random.snapshot().engine_state&&
          physical_silent.state->random.draws()==hit.state->random.draws()&&hit.state->random.draws()>0,
          "outer physical switch overrides nested observation without changing real HP,calendar or shared random");
    reached_second=false;
    auto physical_reject=physical;
    physical_reject.before_common=[&](const Owner &current)->std::optional<Owner> {
        if(current.scene.world.updates==1) {reached_second=true;return {};}
        return current;
    };
    const auto physical_second=prepare_owned_world_runtime(arrow,{27},physical_reject);
    auto arrow_boundary=arrow;arrow_boundary.scene.speed_setting=0;
    arrow_boundary.scene.calendar={0,3,3,10790,0,1599};physical_reject=physical;physical_reject.tasks={};
    const auto physical_calendar=prepare_owned_world_runtime(arrow_boundary,{27},physical_reject);
    check(reached_second&&!physical_second.state&&physical_second.worlds.empty()&&
          !physical_calendar.state&&physical_calendar.worlds.empty()&&
          arrow.scene.world.world.ai.battle.actors.at({2}).hp.target==500&&
          arrow.scene.world.world.ai.projectiles.size()==1&&arrow.random.draws()==0&&
          arrow_boundary.scene.calendar.units==10790,
          "second-round and late-calendar failures discard physical receipts with the private owner and random");
}
void scoped_calendar_consumers(const WorldScriptCatalog &catalog) {
    auto source = fixture(catalog);
    source.scene.calendar = {0, 3, 0, 10790, 0, 399};
    const auto shared = adapter(catalog);
    std::vector<Owner> first_saved, second_saved;
    const WorldRuntimeCalendarConsumer<Owner> second = [&](const Owner &current,
                                                           WorldCalendarStage stage) {
        if (stage == WorldCalendarStage::checkpoint_before_normalize)
            second_saved.push_back(current);
        return shared.calendar_other(current, stage);
    };
    const WorldRuntimeCalendarConsumer<Owner> first = [&](const Owner &current,
                                                          WorldCalendarStage stage) {
        if (stage == WorldCalendarStage::checkpoint_before_normalize) {
            first_saved.push_back(current);
            // 嵌套借用同一只读adapter，不能把外层调用的检查点收集器换掉。
            const auto nested = prepare_owned_world_runtime_calendar_with_consumer(
                source, source.scene.calendar, stage, shared, second);
            check(nested && nested->scene.world.updates == 0,
                  "nested calendar consumer owns a separate private input");
        }
        return shared.calendar_other(current, stage);
    };
    const auto outer = prepare_owned_world_runtime_with_calendar(source, {27}, shared, first);
    const auto following = prepare_owned_world_runtime_with_calendar(source, {27}, shared, second);
    const auto original = prepare_owned_world_runtime(source, {27}, shared);
    check(outer.state && following.state && original.state && first_saved.size() == 1 &&
              second_saved.size() == 2 && first_saved[0].scene.calendar.units == 10790 &&
              first_saved[0].scene.world.updates == 1 && second_saved[0].scene.world.updates == 0 &&
              second_saved[1].scene.world.updates == 1 && outer.state->scene.calendar.units == 17 &&
              outer.state->calendars == original.state->calendars &&
              following.state->calendars == original.state->calendars,
          "shared adapter preserves checkpoint timing, nested isolation and default behavior");
    const WorldRuntimeCalendarConsumer<Owner> reject =
        [](const Owner &, WorldCalendarStage) -> std::optional<Owner> { return {}; };
    const auto failed = prepare_owned_world_runtime_with_calendar(source, {27}, shared, reject);
    check(!failed.state && failed.worlds.empty() && source.scene.world.updates == 0 &&
              source.scene.calendar.units == 10790 && source.random.draws() == 0 &&
              source.calendars.empty(),
          "call-local calendar rejection rolls back earlier world updates without shared mutation");
}
void actual_facility_completion(const WorldScriptCatalog &catalog) {
    auto source = fixture(catalog);
    for (const auto id : {10ULL, 11ULL, 12ULL}) {
        RescueFacility facility;
        facility.placement = {{id}, 3, FacilityShape::single, FacilityOrientation::first, {1, 1}};
        facility.kind = 1;
        facility.category = 5;
        facility.status = id == 10 ? 2 : 1;
        source.scene.world.world.facilities.emplace(id, facility);
        source.scene.world.facility_order.push_back(id);
        source.tasks.finish.dungeon.facilities.emplace(
            id, DungeonFacilityProgress{id == 10 ? 9 : 0, 9600, 9600, 100, 100, {}});
        source.facilities.details.emplace(id, WorldFacilityUpdateDetails{});
    }
    source.facilities.definitions.emplace(3, WorldFacilityUpdateDefinition{});
    source.facilities.cycle_length = 80;
    auto a = adapter(catalog);
    a.facilities.read = [](const Owner &o) {
        auto value = o.facilities;
        value.finish = finish(o);
        value.scripts = scripts(o);
        value.random = o.random;
        return value;
    };
    a.facilities.write = [](Owner &o, const WorldFacilityUpdateState &value) {
        o.facilities = value;
        o.facilities.finish = DungeonFinishState();
        o.facilities.scripts = {};
        o.facilities.random = {};
        o.tasks.finish = value.finish;
        o.tasks.finish.dungeon.world = RescueWorldState();
        o.tasks.finish.event_calls.clear();
        o.scene.world.world = value.finish.dungeon.world;
        auto &order = o.scene.world.facility_order;
        order.erase(
            std::remove_if(order.begin(), order.end(),
                           [&](auto id) { return !o.scene.world.world.facilities.count(id); }),
            order.end());
        return write_scripts(o, value.scripts);
    };
    a.facility = [](const Owner &current,
                    const WorldFacilityUpdateRequest &request) -> std::optional<Owner> {
        if (request.kind != WorldFacilityUpdateConsumerKind::dungeon_finish ||
            request.facility != 10)
            return {};
        auto next = current;
        next.scene.world.world.facilities.erase(10);
        next.tasks.finish.dungeon.facilities.erase(10);
        auto &order = next.scene.world.facility_order;
        order.erase(std::find(order.begin(), order.end(), 10));
        return next; // 明确阶段2域删除夹具，实际地图恢复另由world_exploration专项覆盖。
    };
    const auto completed = prepare_owned_world_runtime(source, {27}, a);
    check(completed.state && completed.worlds.size() == 1 &&
              completed.state->scene.world.facility_order == std::vector<std::uint64_t>{11, 12} &&
              !completed.state->scene.world.world.facilities.count(10) &&
              completed.state->tasks.finish.dungeon.facilities.at(11).updates == 0 &&
              completed.state->tasks.finish.dungeon.facilities.at(12).updates == 1,
          "runtime actual phase2 facility consumer self-removal reports already_removed; forward "
          "skip11 then12");
    const auto resumed = prepare_owned_world_runtime(*completed.state, {27}, a);
    check(resumed.state && resumed.state->tasks.finish.dungeon.facilities.at(11).updates == 1 &&
              resumed.state->tasks.finish.dungeon.facilities.at(12).updates == 2,
          "retained shifted facility updates next frame, removed current never invoked twice");
    // 下一帧推进Owner之后，公开world/scene审计仍保存自移除成功帧的完整状态。
    const auto &world_audit = completed.worlds.front().state;
    check(world_audit.updates == 1 && world_audit.world.map.cells.size() == 36 &&
              world_audit.facility_order == std::vector<std::uint64_t>{11, 12} &&
              world_audit.world.facilities.size() == 2 && world_audit.world.facilities.count(11) &&
              world_audit.world.facilities.count(12),
          "world audit retains full map and surviving facilities at completed first round");
    check(completed.scene && completed.scene->state.frame_counter == 1 &&
              completed.scene->state.world.updates == 1 &&
              completed.scene->state.world.world.map.cells.size() == 36 &&
              completed.scene->state.world.facility_order == std::vector<std::uint64_t>{11, 12} &&
              completed.scene->state.world.world.facilities.size() == 2 &&
              completed.state->scene.world.updates == 1 && resumed.state->scene.world.updates == 2,
          "scene audit and returned owner remain independent of subsequent successful frame");
    // 原adapter的L通过默认真实重叠路径，不借未注册request占位；清空read_routes触发晚期拒绝。
    a.nonactors.read_routes = {};
    const auto rejected = prepare_owned_world_runtime(source, {27}, a);
    check(!rejected.state && rejected.worlds.empty() &&
              source.scene.world.facility_order == std::vector<std::uint64_t>{10, 11, 12} &&
              source.scene.world.world.facilities.count(10) &&
              source.tasks.finish.dungeon.facilities.at(10).updates == 9 &&
              source.random.draws() == 0,
          "late missing L consumer rejects whole runtime after successful facility completion and "
          "skips");
}
} // namespace
int main() {
    try {
        const auto parsed = parse_world_script_catalog(table("events.txt"), table("talk.txt"),
                                                       table("news.txt"), table("evtmsgs.txt"));
        check(parsed.catalog.has_value(), "published scripts parse");
        source_scan(*parsed.catalog);
        loop_and_rollback(*parsed.catalog);
        combat_observation_rollback(*parsed.catalog);
        scoped_calendar_consumers(*parsed.catalog);
        actual_facility_completion(*parsed.catalog);
        std::cout << "world runtime: " << checks << " checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
