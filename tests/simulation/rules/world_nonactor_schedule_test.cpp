#include "ark/simulation/ai/rules/world_nonactor_schedule.hpp"
#include "ark/simulation/village/rules/world_scripts.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
struct Owner {
    WorldScheduleState common;
    WorldRandomStream random;
    std::map<std::pair<int, int>, ObjectCatalogRecord> catalog;
    int item_rewards{};
    WorldScriptState scripts;
    WorldScriptCatalog programs;
    std::vector<int> visual;
};
Owner fixture(std::vector<std::int32_t> raw = {}) {
    Owner owner;
    owner.random = WorldRandomStream::from_raw(std::move(raw));
    auto &common = owner.common;
    common.world.map = {6, 6, std::vector<LegacyMapCell>(36)};
    for (auto &cell : common.world.map.cells)
        cell.legacy_state = 4;
    common.surface.assign(36, 1);
    common.map_flags.assign(36, 0);
    common.town = {0, 1, 0, 1};
    return owner;
}
void actor(Owner &owner, std::uint64_t id, ActorKind kind, CombatPoint point) {
    auto &ai = owner.common.world.ai;
    BattleActorRecord a;
    a.id = {id};
    a.definition = kind == ActorKind::human ? 1 : 7;
    a.kind = kind;
    a.control.state = 1;
    a.control.flags = 2;
    a.position = a.decision_start = point;
    a.hp = {0, 500, 500, 500, false, 0};
    a.capacity = 500;
    ai.battle.actors.emplace(a.id, a);
    (kind == ActorKind::human ? ai.human_order : ai.monster_order).push_back(a.id);
    ai.contexts.emplace(a.id, RewardActorContext{{3, 3}, false, {}, {}, true, {7, 7}});
    owner.common.world.actors.emplace(a.id, RescueActorContext{});
    if (kind == ActorKind::human) {
        ai.battle.humans.emplace(1, HumanBattleRecord{});
        RewardHumanDefinition growth;
        growth.derived.combat = {500, 100, 100, 100};
        ai.growth.emplace(1, growth);
    } else {
        ai.battle.monsters.emplace(7, MonsterBattleRecord{});
        RewardMonsterDefinition growth;
        growth.base_hp = 500;
        growth.base_attack = 100;
        growth.base_defense = 50;
        ai.monster_growth.emplace(7, growth);
    }
}
CombatInfluenceCandidate field() {
    CombatInfluenceCandidate result;
    result.width = result.height = 12;
    result.human_field.assign(144, 17);
    result.monster_field.assign(144, 23);
    return result;
}
WorldNonactorScheduleAdapter<Owner> adapter() {
    WorldNonactorScheduleAdapter<Owner> result;
    result.read_common = [](const Owner &o) -> const WorldScheduleState & { return o.common; };
    result.write_common = [](Owner &o) -> WorldScheduleState & { return o.common; };
    result.read_routes = [](const Owner &o) {
        WorldNonactorScheduleState projected{o.common, o.random, {}};
        projected.objects.catalog = o.catalog;
        projected.objects.item_rewards = o.item_rewards;
        return projected;
    };
    result.write_routes = [](Owner &o, const WorldNonactorScheduleState &projected) {
        o.random = projected.random;
        o.catalog = projected.objects.catalog;
        o.item_rewards = projected.objects.item_rewards;
        return true;
    };
    result.projectile = [](const Owner &, std::uint64_t id) -> std::optional<WorldProjectileInput> {
        WorldProjectileInput input;
        input.projectile = id;
        input.caster_visible = true;
        input.box = CollisionBox{-1, 0, 2, 2};
        input.monster_boxes[0] = CollisionBox{-10, 10, 20, 20};
        return input; // 碰撞矩形是隔离夹具，非素材帧认证。
    };
    result.encounter = [](const Owner &, std::uint64_t id) -> std::optional<EncounterCommitInput> {
        EncounterCommitInput input;
        input.encounter = id;
        return input;
    };
    result.primary_expression_table = true;
    result.request =
        [](Owner &o, const WorldNonactorRequest &request) -> std::optional<WorldNonactorWriteback> {
        auto globals = encounter_external_writeback(o.common.world.ai);
        if (request.kind == WorldNonactorRequestKind::object && request.object) {
            if (request.object->kind == ObjectCommitRequestKind::ground_effect)
                o.visual.push_back(-2); // 隔离夹具标记，不是原版效果号。
            else if (request.object->kind == ObjectCommitRequestKind::notice) {
                const auto &r = *request.object;
                o.scripts.notices.push_back({r.parameter, -1, 80, "fixture", "fixture"});
            } else {
                return {}; // 未提供实际脚本消费者时不吞事件。
            }
        } else if (request.kind == WorldNonactorRequestKind::projectile_hit && request.hit) {
            if (request.hit->kind != HitRequestKind::attack_sound)
                return {};
            o.visual.push_back(request.hit->parameter);
        } else if (request.kind == WorldNonactorRequestKind::projectile_contact) {
            o.visual.push_back(-1); // cd10接触请求的夹具标记。
            if (!request.target || !request.source_position)
                return {};
            auto effects = o.common.world.ai.contexts.at(*request.target).effects;
            effects.display.push_back({10, 0, 0, 0, 0}); // 仅typed写回隔离夹具，不代替原位置投影。
            return WorldNonactorWriteback{globals, {}, {}, effects};
        } else if (request.kind == WorldNonactorRequestKind::projectile_visual) {
            o.visual.push_back(request.visual);
        } else if (request.kind == WorldNonactorRequestKind::encounter && request.encounter) {
            if (request.encounter->kind != EncounterRequestKind::event)
                return {};
            o.scripts.pending_completion = globals.pending_completion;
            const auto script =
                prepare_world_script(o.programs, o.scripts, {request.encounter->parameter, {}, {}});
            if (!script.candidate)
                return {};
            o.scripts = script.candidate->state;
            globals.pending_completion = o.scripts.pending_completion;
            return WorldNonactorWriteback{globals, {}, o.scripts.popularity_queue};
        } else {
            return {};
        }
        return WorldNonactorWriteback{globals, {}};
    };
    return result;
}
void projectile_and_objects() {
    auto owner = fixture({0});
    actor(owner, 1, ActorKind::human, {});
    actor(owner, 2, ActorKind::monster, {20, 0, 0});
    owner.common.world.ai.projectiles.emplace(
        9, *prepare_projectile(ProjectileKind::arrow, {1}, {2}, {}, {1000, 0, 0}, 0).candidate);
    owner.common.world.ai.projectile_order = {9};
    const auto result = prepare_owned_world_nonactor_stage(
        owner, {WorldScheduleStage::projectile, 9, {}}, field(), adapter());
    check(result && result->disposition == WorldScheduleDisposition::already_removed &&
              result->state.common.world.ai.projectiles.empty() &&
              result->state.random.draws() == 1 &&
              result->state.common.world.ai.battle.actors.at({2}).hp.target == 392 &&
              result->state.visual == std::vector<int>{14, -1},
          "shared physical24 actual collision consumes once; real sound/contact consumers execute");
    check(owner.random.draws() == 0 && owner.common.world.ai.battle.actors.at({2}).hp.target == 500,
          "nonactor template leaves original HP and random private until whole owner success");
    auto missing = adapter();
    missing.request = {};
    check(!prepare_owned_world_nonactor_stage(owner, {WorldScheduleStage::projectile, 9, {}},
                                              field(), missing),
          "hit presentation cannot become unconsumed typed notification");
    auto lethal = owner;
    lethal.random = WorldRandomStream::from_raw({0, 99});
    lethal.common.world.ai.pending_completion = 37;
    lethal.common.world.ai.battle.actors.at({2}).hp.target = 1;
    lethal.common.world.ai.battle.monsters.at(7).flags = 4;
    lethal.common.world.ai.battle.monsters.at(7).rank = 5;
    auto event_adapter = adapter();
    auto visuals = event_adapter.request;
    int scripts{};
    event_adapter.request =
        [&](Owner &current,
            const WorldNonactorRequest &request) -> std::optional<WorldNonactorWriteback> {
        if (!request.hit || request.hit->kind != HitRequestKind::event217)
            return visuals(current, request);
        ++scripts;
        const auto &ai = current.common.world.ai;
        check(ai.battle.actors.at({2}).control.state == 3 &&
                  ai.battle.defeated_definitions.empty() && current.random.draws() == 2,
              "nonactor217 callback has source death/stats before N record and actual shared24/100 "
              "cursor");
        WorldScriptCatalog catalog;
        catalog.events.emplace(217, WorldScriptDefinition{217, "fixture", 0, {}, {{22}}});
        current.scripts.pending_completion = ai.pending_completion;
        const auto program = prepare_world_script(catalog, current.scripts, {217, {}, {}});
        if (!program.candidate)
            return {};
        current.scripts = program.candidate->state;
        auto globals = encounter_external_writeback(ai);
        globals.pending_completion = current.scripts.pending_completion;
        return WorldNonactorWriteback{globals, {}, current.scripts.popularity_queue};
    };
    const auto boss = prepare_owned_world_nonactor_stage(
        lethal, {WorldScheduleStage::projectile, 9, {}}, field(), event_adapter);
    check(boss && scripts == 1 && boss->state.common.world.ai.pending_completion == 0 &&
              boss->state.common.popularity_queue == std::vector<std::array<int, 3>>{{10, 37, 1}} &&
              boss->state.common.world.ai.battle.defeated_definitions == std::vector<int>{7},
          "projectile217 actual script executes once at source slot and its typed I survives later "
          "contacts/core commit");
    auto object_owner = fixture();
    GroundObjectState object;
    object.id = {8};
    object.state = 5;
    object.counter = 19;
    object.kind = 0;
    object.definition = 4;
    object.cached_cell = {0, 0};
    object_owner.common.world.ai.battle.objects.emplace(8, object);
    object_owner.common.world.object_order = {8};
    object_owner.catalog.emplace(std::pair<int, int>{0, 4}, ObjectCatalogRecord{});
    const auto updated = prepare_owned_world_nonactor_stage(
        object_owner, {WorldScheduleStage::object, 8, {}}, field(), adapter());
    check(updated && updated->state.item_rewards == 1 &&
              updated->state.catalog.at({0, 4}).inventory == 1 &&
              updated->state.common.world.ai.battle.objects.at(8).counter == 20 &&
              updated->state.visual == std::vector<int>{-2} &&
              updated->state.scripts.notices.size() == 1,
          "actual j20 object grant/catalog plus effect/notice consumers, no repeated inventory "
          "ownership");
    object_owner = updated->state;
    object_owner.common.world.ai.battle.objects.at(8).counter = 59;
    const auto removed = prepare_owned_world_nonactor_stage(
        object_owner, {WorldScheduleStage::object, 8, {}}, field(), adapter());
    check(removed && removed->disposition == WorldScheduleDisposition::already_removed &&
              removed->state.common.world.object_order.empty() &&
              removed->state.catalog.at({0, 4}).inventory == 1,
          "j60 removes exact bp identity once without re-grant");
}
void synchronous_event() {
    auto owner = fixture({999, 0, 0, 0});
    actor(owner, 1, ActorKind::human, {350, 0, 350});
    owner.common.world.ai.battle.actors.at({1}).attack_count = 2;
    owner.common.world.ai.pending_completion = 37;
    RewardEncounter event;
    event.runtime = {7, {3, 3}, 0, 0, 0, 1, 1, 100};
    event.group_exists = false;
    owner.common.world.ai.encounters.emplace(7, event);
    owner.common.world.ai.encounter_order = {7};
    WorldScriptPage scene;
    scene.id = 1;
    owner.scripts.pages = {scene};
    owner.scripts.next_page_id = 2;
    owner.programs.talks = {{"fixture", 0, -1, -1, {"fixture"}}};
    // 明确控制夹具：实际脚本解释器/页栈/续体消费者，不冒充APK的91原脚本内容。
    owner.programs.events.emplace(
        91, WorldScriptDefinition{91, "fixture", 0, {}, {{2, 0}, {6, 2}, {22}}});
    auto routes = adapter();
    auto actual = routes.request;
    routes.request =
        [&](Owner &current,
            const WorldNonactorRequest &request) -> std::optional<WorldNonactorWriteback> {
        if (request.encounter)
            check(current.common.world.ai.encounters.at(7).runtime.state == 1 &&
                      current.common.world.ai.encounters.at(7).influence->human_field ==
                          field().human_field &&
                      current.common.world.ai.contexts.at({1}).effects.display.size() == 2 &&
                      current.random.draws() == 4,
                  "source event91 observes retired k1, current snapshot/rewards, exact shared "
                  "expression cursor");
        return actual(current, request);
    };
    const auto result = prepare_owned_world_nonactor_stage(
        owner, {WorldScheduleStage::encounter, 7, {}}, field(), routes);
    check(result && result->state.random.draws() == 4 && result->state.scripts.pages.size() == 2 &&
              result->state.scripts.event_calls.at(91) == 1 &&
              result->state.scripts.continuations.size() == 1 &&
              result->state.common.world.ai.pending_completion == 37 &&
              result->state.common.world.ai.battle.actors.at({1}).control.state == 10,
          "actual script pushes dialogue and saveswait while typed writeback preserves core "
          "actor/field");
    auto continuation =
        prepare_world_script_continuations(result->state.programs, result->state.scripts, true);
    continuation = prepare_world_script_continuations(result->state.programs,
                                                      continuation.candidate->state, true);
    check(continuation.candidate && continuation.candidate->state.pending_completion == 0 &&
              continuation.candidate->state.popularity_queue ==
                  std::vector<std::array<int, 3>>{{10, 37, 1}},
          "actual subsequent script22 changes completion/popularity through sole outer projection");
    for (const bool missing_consumer : {false, true}) {
        auto failed = routes;
        if (missing_consumer)
            failed.request = {};
        else
            failed.request =
                [](Owner &, const WorldNonactorRequest &) -> std::optional<WorldNonactorWriteback> {
                return {};
            };
        check(!prepare_owned_world_nonactor_stage(owner, {WorldScheduleStage::encounter, 7, {}},
                                                  field(), failed) &&
                  owner.random.draws() == 0 && owner.scripts.pages.size() == 1 &&
                  owner.common.world.ai.battle.actors.at({1}).control.state == 1,
              "late event consumer rejection discards state10/reward/field/script and raw cursor "
              "together");
    }
    auto immediate = owner;
    immediate.programs.events.at(91).commands = {{22}, {6, 2}};
    const auto committed = prepare_owned_world_nonactor_stage(
        immediate, {WorldScheduleStage::encounter, 7, {}}, field(), adapter());
    check(committed && committed->state.common.world.ai.pending_completion == 0 &&
              committed->state.common.popularity_queue ==
                  std::vector<std::array<int, 3>>{{10, 37, 1}},
          "immediate real script22 pending/I writeback survives later common projection commit");
    auto undeclared = adapter();
    auto actual_request = undeclared.request;
    undeclared.request =
        [&](Owner &current,
            const WorldNonactorRequest &request) -> std::optional<WorldNonactorWriteback> {
        auto fields = actual_request(current, request);
        if (!fields)
            return {};
        current.common.popularity_queue.push_back({10, 99, 1});
        fields->popularity_queue.reset();
        return fields;
    };
    check(!prepare_owned_world_nonactor_stage(immediate, {WorldScheduleStage::encounter, 7, {}},
                                              field(), undeclared),
          "undeclared external common I mutation refuses rather than silently discard actual "
          "script effects");
}
void final_and_missing_domains() {
    auto owner = fixture({0});
    actor(owner, 1, ActorKind::human, {350, 0, 350});
    actor(owner, 2, ActorKind::monster, {355, 0, 355});
    const auto result = prepare_owned_world_nonactor_stage(
        owner, {WorldScheduleStage::finalize, {}, {}}, field(), adapter());
    check(result && result->state.random.draws() == 1 &&
              result->state.common.world.ai.contexts.at({1}).cell == Position{3, 3} &&
              result->state.common.world.ai.battle.actors.at({1}).position.x != 350,
          "actual finalL shares draw2 and changes n only, never cached s/t/ax");
    auto exhausted = owner;
    exhausted.random = WorldRandomStream::from_raw({});
    check(!prepare_owned_world_nonactor_stage(exhausted, {WorldScheduleStage::finalize, {}, {}},
                                              field(), adapter()) &&
              exhausted.common.world.ai.battle.actors.at({1}).position.x == 350,
          "late final shared random failure leaves source world untouched");
    for (const auto stage :
         {WorldScheduleStage::arrival_front, WorldScheduleStage::popularity,
          WorldScheduleStage::facility, WorldScheduleStage::decision, WorldScheduleStage::control})
        check(!prepare_owned_world_nonactor_stage(owner, {stage, {}, {}}, field(), adapter()),
              "missing arrival/popularity/facility/person consumers never return default success");
}
void spawned_context_and_stale_projection() {
    auto owner = fixture({0, 0, 0, 0});
    actor(owner, 1, ActorKind::human, {350, 0, 350});
    // 怪物定义是源接口夹具，当前名单仍为空。
    owner.common.world.ai.battle.monsters.emplace(7, MonsterBattleRecord{});
    RewardMonsterDefinition definition;
    definition.base_hp = 500;
    owner.common.world.ai.monster_growth.emplace(7, definition);
    RewardEncounter event;
    event.runtime = {7, {3, 3}, 3, 0, 0, 0, 2, 0};
    event.group_exists = false;
    owner.common.world.ai.encounters.emplace(7, event);
    owner.common.world.ai.encounter_order = {7};
    owner.common.world.ai.task_active = true;
    auto routes = adapter();
    routes.encounter = [](const Owner &, std::uint64_t id) -> std::optional<EncounterCommitInput> {
        EncounterCommitInput input;
        input.encounter = id;
        input.quest.normal_definitions = {7};
        return input;
    };
    // 故意提供旧投影；模板必须用当前common覆盖，而不恢复旧人物/影响/根。
    auto read = routes.read_routes;
    routes.read_routes = [&](const Owner &current) {
        auto stale = read(current);
        stale.common.world.ai.battle.actors.clear();
        stale.common.world.ai.external_actor_roots.clear();
        return stale;
    };
    owner.common.world.ai.external_actor_roots.insert({1});
    const auto result = prepare_owned_world_nonactor_stage(
        owner, {WorldScheduleStage::encounter, 7, {}}, field(), routes);
    check(result && result->state.random.draws() == 4 &&
              result->state.common.world.ai.monster_order == std::vector<CharacterId>{{3}} &&
              result->state.common.world.actors.at({3}).destination == Position{} &&
              result->state.common.world.ai.encounters.at(7).runtime.spawned == 1 &&
              result->state.common.world.ai.encounters.at(7).influence->monster_field ==
                  field().monster_field &&
              result->state.common.world.ai.external_actor_roots.count({1}),
          "real task spawn shares cell/definition/offset draws, installs one empty constructor "
          "context and preserves current roots");
    owner.random = WorldRandomStream::from_raw({0, 0});
    check(!prepare_owned_world_nonactor_stage(owner, {WorldScheduleStage::encounter, 7, {}},
                                              field(), routes) &&
              owner.random.draws() == 0 && owner.common.world.ai.monster_order.empty() &&
              owner.common.world.actors.size() == 1 && owner.common.world.ai.next_actor_id == 3,
          "late actual spawn offset exhaustion rolls back whole outer "
          "common/catalog/random/context allocation");
}
void typed_task_monster_unlock() {
    auto owner = fixture({999, 0, 999, 0, 0, 0, 0, 0});
    actor(owner, 1, ActorKind::human, {350, 0, 350});
    owner.common.world.ai.task_active = true;
    // 原任务参与者由共同战斗 Owner 持有，奖励提交不能采用临时输入的旧名单。
    owner.common.world.ai.battle.participants = {1};
    owner.common.world.ai.feature16 = true;
    owner.common.world.ai.battle.events.insert(128);
    owner.common.world.ai.battle.events.insert(205);
    RewardMonsterDefinition definition;
    definition.growth = 9;
    owner.common.world.ai.monster_growth.emplace(7, definition);
    RewardEncounter encounter;
    encounter.runtime = {7, {3, 3}, 3, 0, 0, 1, 1, 0};
    encounter.group_exists = false;
    owner.common.world.ai.encounters.emplace(7, encounter);
    owner.common.world.ai.encounter_order = {7};
    auto routes = adapter();
    routes.encounter = [](const Owner &, std::uint64_t id) -> std::optional<EncounterCommitInput> {
        EncounterCommitInput input;
        input.encounter = id;
        return input;
    };
    bool observed_success{};
    routes.request = [&](Owner &current, const WorldNonactorRequest &request)
        -> std::optional<WorldNonactorWriteback> {
        if (!request.encounter)
            return {};
        auto globals = encounter_external_writeback(current.common.world.ai);
        if (request.encounter->kind == EncounterRequestKind::mark_task_complete) {
            globals.monster_availability = std::map<int, std::array<int, 2>>{{7, {1, 1}}};
        } else if (request.encounter->kind == EncounterRequestKind::clear_task) {
            const auto &d = current.common.world.ai.monster_growth.at(7);
            observed_success = d.status == 1 && d.newly_unlocked && d.growth == 9;
        }
        return WorldNonactorWriteback{globals, {}}; // 明确夹具：只验证typed source时序和边界。
    };
    const auto result = prepare_owned_world_nonactor_stage(
        owner, {WorldScheduleStage::encounter, 7, {}}, field(), routes);
    check(result && observed_success && result->state.common.world.ai.monster_growth.at(7).status == 1 &&
        result->state.common.world.ai.monster_growth.at(7).newly_unlocked &&
        result->state.common.world.ai.monster_growth.at(7).growth == 9,
        "typed task p/r survives nested core commit and is visible to next source clear request");
    check(owner.common.world.ai.monster_growth.at(7).status == 0 && owner.random.draws() == 0,
        "successful private task candidate does not mutate original catalogue/random");
    auto invalid = routes;
    invalid.request = [](Owner &current, const WorldNonactorRequest &request)
        -> std::optional<WorldNonactorWriteback> {
        auto globals = encounter_external_writeback(current.common.world.ai);
        if (request.encounter && request.encounter->kind == EncounterRequestKind::event)
            globals.monster_availability = std::map<int, std::array<int, 2>>{{7, {1, 1}}};
        return WorldNonactorWriteback{globals, {}};
    };
    check(!prepare_owned_world_nonactor_stage(owner, {WorldScheduleStage::encounter, 7, {}},
                                              field(), invalid) &&
        owner.common.world.ai.monster_growth.at(7).status == 0 && owner.random.draws() == 0,
        "event cannot forge task catalogue writeback; whole nested random/reward candidate rolls back");
}
} // namespace
int main() {
    try {
        projectile_and_objects();
        synchronous_event();
        final_and_missing_domains();
        spawned_context_and_stale_projection();
        typed_task_monster_unlock();
        std::cout << "world_nonactor_schedule: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "world_nonactor_schedule: " << error.what() << '\n';
        return 1;
    }
}
