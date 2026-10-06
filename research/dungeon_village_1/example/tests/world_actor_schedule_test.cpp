#include "dungeon_village_reference/world_actor_schedule.hpp"
#include "dungeon_village_reference/world_nonactor_schedule.hpp"
#include <iostream>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool yes, const char *what) {
    ++checks;
    if (!yes)
        throw std::runtime_error(what);
}
struct Owner {
    WorldScheduleState common;
    WorldRandomStream random;
    std::map<int, int> definition_state;
    std::vector<int> script_entries; // 外层脚本/页面保留观测夹具，不作为真实脚本目录。
};
// 长等待隔离共同计数与旅店恢复；不是原新局或完整生活/月份夹具。
Owner fixture() {
    Owner o;
    auto &s = o.common;
    s.world.map = {6, 6, std::vector<LegacyMapCell>(36)};
    for (auto &cell : s.world.map.cells) {
        cell.legacy_state = 4;
        cell.category = RouteCategory::road;
    }
    s.surface = std::vector<int>(36, 1);
    s.map_flags = std::vector<std::uint32_t>(36);
    s.town = {0, 5, 0, 5};
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.category = 2;
    f.occupants = {{1}};
    s.world.facilities.emplace(3, f);
    s.facility_order = {3};
    s.world.map = *bind_facility_map(s.world.map, {{f.placement, 3}}).map;
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    s.world.ai.professions = {{{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 5, true}};
    g.derived = *derive_human_stats(g.definition, s.world.ai.professions).candidate;
    s.world.ai.growth.emplace(0, g);
    s.world.ai.battle.humans.emplace(0, HumanBattleRecord{});
    BattleActorRecord a;
    a.id = {1};
    a.control.state = 14;
    a.control.flags = 2;
    a.capacity = 100;
    a.control.queue = {{1, 1200, 0}};
    a.hp = {0, 50, 50, 50, false, 0};
    a.position = {150, 0, 150};
    a.physics_pause = 1;
    s.world.ai.battle.actors.emplace(a.id, a);
    s.world.ai.human_order = {a.id};
    s.world.ai.contexts.emplace(a.id, RewardActorContext{{1, 1}, true, {}, {}, true, {3, 3}});
    s.world.actors.emplace(a.id, RescueActorContext{});
    s.world.actors.at(a.id).binding = ArrivalBinding{{1, 1}, {3}, 33};
    s.world.actors.at(a.id).destination = Position{1, 1};
    o.definition_state.emplace(0, 0);
    return o;
}
WorldActorScheduleAdapter<Owner> adapter() {
    WorldActorScheduleAdapter<Owner> a;
    a.read_common = [](const Owner &s) -> const WorldScheduleState & { return s.common; };
    a.write_common = [](Owner &s) -> WorldScheduleState & { return s.common; };
    a.read_routes = [](const Owner &s) {
        WorldActorRoutesState r;
        r.world = s.common.world;
        r.facts = world_schedule_facts(s.common);
        r.random = s.random;
        r.human_definition_state = s.definition_state;
        return r;
    };
    a.write_routes = [](Owner &s, const WorldActorRoutesState &r) {
        s.random = r.random;
        s.definition_state = r.human_definition_state;
        return true;
    };
    a.decision = [](const Owner &, CharacterId id) -> std::optional<WorldActorDecisionInput> {
        WorldActorDecisionInput i;
        i.actor = id;
        i.use_shared_random = true;
        i.primary_expression_table = true;
        return i;
    };
    a.command = [](const auto &, CharacterId,
                   const auto &) -> std::optional<WorldActorCommandInput> {
        return {}; // 本夹具只有本地等待，不给其他领域默认成功输入。
    };
    a.primary_expression_table = true;
    a.other = [](const Owner &o, const WorldScheduleCall &call,
                 const CombatInfluenceCandidate &) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        auto next = o;
        if (call.stage == WorldScheduleStage::finalize) {
            const auto actual = prepare_world_schedule_overlap(o.common, {});
            if (!actual)
                return {};
            next.common = *actual;
        } else if (call.stage != WorldScheduleStage::arrival_front &&
                   call.stage != WorldScheduleStage::facility) {
            return {}; // 明确隔离到访/单旅店夹具；不冒充真实设施/到访前段。
        }
        return OwnedWorldScheduleStep<Owner>{std::move(next)};
    };
    return a;
}
void drop_created_by_control_runs_same_round() {
    auto owner = fixture();
    auto &common = owner.common;
    common.world.facilities.clear();
    common.facility_order.clear();
    common.town = {0, 1, 0, 1}; // 明确外镇战斗夹具，遭遇区域不进入城镇取消分支。
    common.world.map = {6, 6, std::vector<LegacyMapCell>(36)};
    for (auto &cell : common.world.map.cells) {
        cell.legacy_state = 4;
        cell.category = RouteCategory::road;
    }
    common.world.actors.at({1}).binding.reset();
    auto &human = common.world.ai.battle.actors.at({1});
    human.control.state = 1;
    human.control.action = 1;
    human.control.action_counter = 4; // 本轮d前缀+1后落在真实近战5..8窗口。
    human.control.flags = 2U | 4U;
    human.control.queue = {{14}};
    human.hp = {0, 100, 100, 100, false, 0};
    human.position = human.attack_position = {250, 0, 250};
    human.combo_count = 1;
    human.encounter = 3; // 原e()无db即为空，必须建立真实遭遇引用才能攻击。
    common.world.ai.contexts.at({1}) = RewardActorContext{{2, 2}, false, {}, {}, true, {5, 5}};
    BattleActorRecord monster;
    monster.id = {2};
    monster.kind = ActorKind::monster;
    monster.definition = 7;
    monster.control.state = 1;
    monster.control.flags = 2U | 4U;
    monster.capacity = 100;
    monster.hp = {0, 1, 1, 1, false, 0};
    monster.position = monster.attack_position = {290, 0, 250};
    monster.physics_pause = 1;
    monster.encounter = 3;
    common.world.ai.battle.actors.emplace(monster.id, monster);
    common.world.ai.monster_order = {monster.id};
    common.world.ai.contexts.emplace(monster.id,
        RewardActorContext{{2, 2}, false, {}, {}, true, {5, 5}});
    common.world.actors.emplace(monster.id, RescueActorContext{});
    common.world.ai.battle.monsters.emplace(7, MonsterBattleRecord{});
    RewardMonsterDefinition definition;
    definition.base_hp = 100;
    definition.base_attack = definition.base_defense = 1;
    common.world.ai.monster_growth.emplace(7, definition);
    RewardEncounter encounter;
    encounter.runtime = {3, {3, 3}, 0, 0, 0, 1, 5, 0}; // upper row2不与town0..1相交。
    encounter.members = {{2}};
    encounter.group_exists = false;
    common.world.ai.encounters.emplace(3, encounter);
    common.world.ai.encounter_order = {3};
    // 近战/drop五票，随后本轮正常bn空生成分支仍抽1000；不能删除实际第六抽。
    owner.random = WorldRandomStream::from_raw({0, 0, 99, 0, 0, 999});
    auto actors = adapter();
    actors.decision = [](const Owner &, CharacterId id) -> std::optional<WorldActorDecisionInput> {
        WorldActorDecisionInput input;
        input.actor = id;
        input.use_shared_random = true;
        input.primary_expression_table = true;
        WorldCombatPolicyInput combat;
        combat.actor = id;
        combat.weapon = {0, 100, 1, 0, 0};
        combat.monster_range = 100;
        input.combat = combat;
        return input;
    };
    actors.command = [](const WorldActorRoutesState &, CharacterId id,
        const LegacyActorControl &op) -> std::optional<WorldActorCommandInput> {
        if (op[0] != 14)
            return {};
        WorldActorCommandInput input;
        input.use_shared_random = true;
        input.primary_expression_table = true;
        WorldAttackInput attack;
        attack.actor = id;
        attack.weapon = {0, 100, 1, 0, 0};
        DropSelectionInput drop;
        drop.definitions = {{4, 0, 1, 2U, 0}}; // 明确单项掉落目录夹具，不冒充原初始catalog。
        attack.drop_selection = drop;
        input.attack = attack;
        return input;
    };
    WorldNonactorScheduleAdapter<Owner> nonactors;
    nonactors.read_common = actors.read_common;
    nonactors.write_common = actors.write_common;
    nonactors.read_routes = [](const Owner &s) {
        return WorldNonactorScheduleState{s.common, s.random, {}};
    };
    nonactors.write_routes = [](Owner &s, const WorldNonactorScheduleState &r) {
        s.random = r.random;
        return true;
    };
    std::size_t encounter_draws{};
    nonactors.encounter = [&](const Owner &s, std::uint64_t id)
        -> std::optional<EncounterCommitInput> {
        encounter_draws = s.random.draws();
        EncounterCommitInput input;
        input.encounter = id;
        return input;
    };
    actors.other = [nonactors](const Owner &s, const WorldScheduleCall &call,
        const CombatInfluenceCandidate &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        if (call.stage == WorldScheduleStage::arrival_front)
            return OwnedWorldScheduleStep<Owner>{s}; // 明确隔离到访，实际测试人物/物体/最终L。
        return prepare_owned_world_nonactor_stage(s, call, field, nonactors);
    };
    const auto result = prepare_world_actor_schedule(owner, {}, actors);
    if (!result.state)
        throw std::runtime_error("drop fixture schedule error " +
            std::to_string(static_cast<int>(result.error)));
    if (result.state->random.draws() != 6)
        throw std::runtime_error("drop fixture random draws " +
            std::to_string(result.state->random.draws()) + "; bn entry=" +
            std::to_string(encounter_draws) + "; objects=" +
            std::to_string(result.state->common.world.ai.battle.objects.size()) + "; victimstate=" +
            std::to_string(result.state->common.world.ai.battle.actors.at({2}).control.state) +
            "; expected hit/drop5 plus normal bn1");
    check(result.state && result.state->common.world.object_order == std::vector<std::uint64_t>{1} &&
        result.state->common.world.ai.battle.objects.at(1).state == 6 &&
        result.state->common.world.ai.battle.objects.at(1).counter == 1 &&
        result.state->common.world.ai.battle.objects.at(1).definition == 4 &&
        result.state->common.world.ai.encounters.at(3).runtime.state == 0 &&
        result.state->common.world.ai.battle.actors.at({2}).state_parameter == 0 &&
        result.state->random.draws() == 6,
        "lethal front14 appends actual drop bp identity and later object stage advances it same round");
    check(owner.common.world.object_order.empty() && owner.common.world.ai.battle.objects.empty() &&
        owner.common.world.ai.battle.actors.at({2}).hp.target == 1 && owner.random.draws() == 0,
        "created drop and all source hit/drop/normal-bn draws stay private until complete world success");
    auto border = owner;
    border.common.world.ai.encounters.at(3).runtime.center = {2, 2}; // upper row1含town边界(1,1)。
    const auto cancelled = prepare_world_actor_schedule(border, {}, actors);
    check(cancelled.state && cancelled.state->random.draws() == 5 &&
        cancelled.state->common.world.ai.encounters.at(3).runtime.state == 1 &&
        cancelled.state->common.world.ai.battle.actors.at({2}).state_parameter == 1 &&
        cancelled.state->common.world.object_order == std::vector<std::uint64_t>{1} &&
        cancelled.state->common.world.ai.battle.objects.at(1).counter == 1,
        "source upper-row town cancellation keeps same-round drop but does not draw normal spawn1000");
    auto fail = actors;
    const auto actual = actors.other;
    fail.other = [actual](const Owner &s, const WorldScheduleCall &call,
        const CombatInfluenceCandidate &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        return call.stage == WorldScheduleStage::finalize ? std::nullopt : actual(s, call, field);
    };
    check(!prepare_world_actor_schedule(owner, {}, fail).state &&
        owner.common.world.ai.battle.next_object_id == 1 && owner.common.world.object_order.empty() &&
        owner.common.world.ai.battle.objects.empty() && owner.random.draws() == 0,
        "lateL failure rolls back lethal HP, new object allocation/order and same-round object tick");
}
} // namespace
int main() {
    try {
        auto s = fixture();
        const auto a = adapter();
        std::optional<WorldActorScheduleResult<Owner>> first_round;
        for (int n = 0; n < 1000; ++n) {
            auto r = prepare_world_actor_schedule(s, {}, a);
            if (!r.state)
                throw std::runtime_error("round " + std::to_string(n) + " error " +
                                         std::to_string(static_cast<int>(r.error)));
            const auto &actor = r.state->common.world.ai.battle.actors.at({1});
            check(actor.state_counter == n + 1 && actor.control.queue.front()[1] == 1199 - n &&
                      r.state->common.updates == n + 1,
                  "actual common c/d and full routing advance each counter and wait exactly once");
            check(actor.hp.target == (n >= 170 ? 100 : 50) && s.random.draws() == 0 &&
                      r.decisions.size() == 1 && r.controls.size() == 1,
                  "oldB170 inn recovery, zero unused random and one decision/control segment");
            s = *r.state;
            if (n == 0)
                first_round = std::move(r);
        }
        // 保留首轮公开审计到千轮之后：c、v和最终共同世界必须仍各自保持原时点。
        const auto &decision = first_round->decisions.front().state;
        const auto &control = first_round->controls.front().state;
        check(decision.world.ai.battle.actors.at({1}).state_counter == 0 &&
                  decision.world.ai.battle.actors.at({1}).control.queue.front()[1] == 1200 &&
                  decision.world.map.cells.size() == 36 && decision.world.facilities.count(3) &&
                  decision.human_definition_state.at(0) == 0 && decision.random.draws() == 0,
              "retained decision audit contains full world and definition state before d/v");
        check(control.world.ai.battle.actors.at({1}).state_counter == 1 &&
                  control.world.ai.battle.actors.at({1}).control.queue.front()[1] == 1199 &&
                  control.world.map.cells.size() == 36 && control.world.facilities.count(3) &&
                  control.human_definition_state.at(0) == 0 && control.random.draws() == 0,
              "retained control audit contains full world after d/v independent of later rounds");
        check(first_round->audit && first_round->audit->state.updates == 1 &&
                  first_round->audit->state.world.ai.battle.actors.at({1}).state_counter == 1 &&
                  first_round->audit->state.facility_order == std::vector<std::uint64_t>{3} &&
                  first_round->state->common.updates == 1 && s.common.updates == 1000,
              "first returned owner and final schedule audit remain complete after owner advances");
        auto missing = a;
        missing.other = {};
        check(
            !prepare_world_actor_schedule(s, {}, missing).state,
            "non-actor consumers mandatory, cannot turn full actor route into fake complete world");
        auto failed = a;
        failed.other =
            [](const Owner &o, const WorldScheduleCall &call,
               const CombatInfluenceCandidate &) -> std::optional<OwnedWorldScheduleStep<Owner>> {
            if (call.stage == WorldScheduleStage::finalize)
                return {};
            return OwnedWorldScheduleStep<Owner>{o};
        };
        check(!prepare_world_actor_schedule(s, {}, failed).state &&
                  s.common.world.ai.battle.actors.at({1}).state_counter == 1000,
              "late finalize error rolls back common counters, domains and outer extra together");
        missing = a;
        missing.decision = {};
        missing.other = {};
        check(prepare_world_actor_schedule(s, {false}, missing).state.has_value(),
              "not-admitted round does not require domain consumers or advance anything");
        auto appearance = fixture();
        auto &world = appearance.common.world;
        auto &monster = world.ai.battle.actors.at({1});
        monster.kind = ActorKind::monster;
        monster.control.state = 8;
        monster.state_counter = 73;
        monster.baseline = 17;
        world.ai.human_order.clear();
        world.ai.monster_order = {{1}};
        world.ai.battle.monsters.emplace(0, MonsterBattleRecord{});
        RewardMonsterDefinition definition;
        definition.base_hp = 100;
        world.ai.monster_growth.emplace(0, definition);
        world.actors.at({1}).monster_mode = 2; // 明确存取状态夹具，不造原实时T2生成入口。
        auto event_adapter = adapter();
        event_adapter.event = [](const Owner &current, int code) -> std::optional<Owner> {
            auto next = current;
            next.script_entries.push_back(code);
            next.common.world.ai.battle.events.insert(code);
            return next;
        };
        const auto appeared = prepare_world_actor_schedule(appearance, {}, event_adapter);
        check(appeared.state && appeared.state->script_entries == std::vector<int>{90} &&
                  appeared.state->common.world.ai.retired_actors.at({1}).control.state == 17 &&
                  appeared.state->common.world.ai.monster_order.empty() &&
                  appeared.state->common.world.ai.battle.events.count(90),
              "source90 retains outer script with c17; unbound monster removed at real d tail");
        event_adapter.other = failed.other;
        check(!prepare_world_actor_schedule(appearance, {}, event_adapter).state &&
                  appearance.script_entries.empty() &&
                  appearance.common.world.ai.battle.actors.at({1}).control.state == 8,
              "lateL failure discards outer event side effects as well as current actors");
        drop_created_by_control_runs_same_round();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
