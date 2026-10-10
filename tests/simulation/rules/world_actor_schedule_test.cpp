#include "ark/simulation/ai/rules/world_actor_schedule.hpp"
#include "ark/simulation/ai/rules/world_nonactor_schedule.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace ark::simulation::rules;
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
    nonactors.encounter = [&](const Owner &s,
                              std::uint64_t id) -> std::optional<EncounterCommitInput> {
        encounter_draws = s.random.draws();
        EncounterCommitInput input;
        input.encounter = id;
        return input;
    };
    actors.other =
        [nonactors](
            const Owner &s, const WorldScheduleCall &call,
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
        throw std::runtime_error(
            "drop fixture random draws " + std::to_string(result.state->random.draws()) +
            "; bn entry=" + std::to_string(encounter_draws) + "; objects=" +
            std::to_string(result.state->common.world.ai.battle.objects.size()) + "; victimstate=" +
            std::to_string(result.state->common.world.ai.battle.actors.at({2}).control.state) +
            "; expected hit/drop5 plus normal bn1");
    check(result.state &&
              result.state->common.world.object_order == std::vector<std::uint64_t>{1} &&
              result.state->common.world.ai.battle.objects.at(1).state == 6 &&
              result.state->common.world.ai.battle.objects.at(1).counter == 1 &&
              result.state->common.world.ai.battle.objects.at(1).definition == 4 &&
              result.state->common.world.ai.encounters.at(3).runtime.state == 0 &&
              result.state->common.world.ai.battle.actors.at({2}).state_parameter == 0 &&
              result.state->random.draws() == 6,
          "lethal front14 appends actual drop bp identity and later object stage advances it same "
          "round");
    check(owner.common.world.object_order.empty() && owner.common.world.ai.battle.objects.empty() &&
              owner.common.world.ai.battle.actors.at({2}).hp.target == 1 &&
              owner.random.draws() == 0,
          "created drop and all source hit/drop/normal-bn draws stay private until complete world "
          "success");
    auto border = owner;
    border.common.world.ai.encounters.at(3).runtime.center = {2, 2}; // upper row1含town边界(1,1)。
    const auto cancelled = prepare_world_actor_schedule(border, {}, actors);
    check(cancelled.state && cancelled.state->random.draws() == 5 &&
              cancelled.state->common.world.ai.encounters.at(3).runtime.state == 1 &&
              cancelled.state->common.world.ai.battle.actors.at({2}).state_parameter == 1 &&
              cancelled.state->common.world.object_order == std::vector<std::uint64_t>{1} &&
              cancelled.state->common.world.ai.battle.objects.at(1).counter == 1,
          "source upper-row town cancellation keeps same-round drop but does not draw normal "
          "spawn1000");
    auto fail = actors;
    const auto actual = actors.other;
    fail.other =
        [actual](
            const Owner &s, const WorldScheduleCall &call,
            const CombatInfluenceCandidate &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        return call.stage == WorldScheduleStage::finalize ? std::nullopt : actual(s, call, field);
    };
    check(!prepare_world_actor_schedule(owner, {}, fail).state &&
              owner.common.world.ai.battle.next_object_id == 1 &&
              owner.common.world.object_order.empty() &&
              owner.common.world.ai.battle.objects.empty() && owner.random.draws() == 0,
          "lateL failure rolls back lethal HP, new object allocation/order and same-round object "
          "tick");
}
// 受击轨迹使用真实共同调度；到访/设施明确隔离，外部表现只保存typed诊断输出。
struct HitTraceRow {
    std::string scenario;
    int round{};
    std::string phase;
    CharacterId actor;
    BattleActorRecord value;
    WorldPosition velocity;
    std::size_t actors{}, projectiles{}, retired{}, outputs{}, draws{};
};
using HitTrace = std::vector<HitTraceRow>;
void hit_row(HitTrace &rows, const std::string &name, int round, const char *phase,
             const Owner &owner, CharacterId id) {
    const auto &ai = owner.common.world.ai;
    rows.push_back({name, round, phase, id, ai.battle.actors.at(id),
                    owner.common.world.actors.at(id).horizontal_velocity, ai.battle.actors.size(),
                    ai.projectiles.size(), ai.retired_actors.size(), owner.script_entries.size(),
                    owner.random.draws()});
}
Owner hit_owner(CharacterId target) {
    auto o = fixture();
    o.random = WorldRandomStream::from_java_seed(17);
    auto &w = o.common.world;
    w.facilities.clear();
    o.common.facility_order.clear();
    o.common.town = {0, 1, 0, 1};
    w.map = {6, 6, std::vector<LegacyMapCell>(36)};
    for (auto &cell : w.map.cells) {
        cell.legacy_state = 4;
        cell.category = RouteCategory::road;
    }
    auto &h = w.ai.battle.actors.at({1});
    h.control.state = 1;
    h.control.action = 0;
    h.control.action_counter = 0;
    h.control.flags = 2U | 4U;
    h.control.queue = {{1, 1000, 0}};
    h.hp = {0, 100, 100, 100, false, 0};
    h.position = h.attack_position = {250, 0, 250};
    h.encounter = 3;
    h.physics_pause = 0;
    w.ai.contexts.at(h.id) = RewardActorContext{{2, 2}, false, {}, {}, true, {5, 5}};
    w.actors.at(h.id) = RescueActorContext{};
    BattleActorRecord m = h;
    m.id = {2};
    m.kind = ActorKind::monster;
    m.definition = 7;
    m.baseline = 17;
    m.position = m.attack_position = {290, 0, 250};
    w.ai.battle.actors.emplace(m.id, m);
    w.ai.monster_order = {m.id};
    w.ai.contexts.emplace(m.id, RewardActorContext{{2, 2}, false, {}, {}, true, {5, 5}});
    w.actors.emplace(m.id, RescueActorContext{});
    w.ai.battle.monsters.emplace(7, MonsterBattleRecord{});
    RewardMonsterDefinition growth;
    growth.base_hp = 100;
    growth.base_attack = growth.base_defense = 1;
    w.ai.monster_growth.emplace(7, growth);
    RewardEncounter e;
    e.runtime = {3, {3, 3}, 0, 0, 0, 1, 5, 0};
    e.members = {m.id};
    e.group_exists = false;
    w.ai.encounters.emplace(3, e);
    w.ai.encounter_order = {3};
    w.actors.at(target).horizontal_velocity = {3.25F, -2.5F};
    return o;
}
// 保存事件与外部表现载荷是本套件的诊断夹具，绝不修改受击HP/state/速度来制造结果。
WorldActorScheduleAdapter<Owner> hit_adapter(HitTrace &trace, const std::string &name, int &round,
                                             CharacterId target) {
    auto a = adapter();
    // prefix_effects仅在有实际成长/声音输出时派发；不能拿它冒充每次d观测点。
    // 复用真实尾部发布接口，分别记逻辑n与供表现缓存读取的投影；回调不改Owner。
    a.tail_cache = [&trace, &round, name, target](const Owner &o, CharacterId id,
                        const BattleActorRecord &projected) -> std::optional<Owner> {
        if (id == target) {
            hit_row(trace, name, round, "after-target-d-tail", o, target);
            hit_row(trace, name, round, "target-render-projection", o, target);
            trace.back().value = projected;
        }
        return o;
    };
    a.decision = [](const Owner &, CharacterId id) -> std::optional<WorldActorDecisionInput> {
        WorldActorDecisionInput i;
        i.actor = id;
        i.use_shared_random = true;
        i.primary_expression_table = true;
        WorldCombatPolicyInput combat;
        combat.actor = id;
        combat.weapon = {0, 100, 1, 0, 0};
        combat.monster_range = 100;
        i.combat = combat;
        return i;
    };
    a.owned_command = [&trace, &round, name, target](const Owner &o, const WorldActorRoutesState &, CharacterId id,
                          const LegacyActorControl &op) -> std::optional<WorldActorCommandInput> {
        if (op[0] == 0)
            return WorldActorCommandInput{}; // 真正opcode0消费者由既有world_facilities执行。
        if (op[0] != 14 && op[0] != 17)
            return {};
        hit_row(trace, name, round, "before-attack-control", o, target);
        WorldActorCommandInput i;
        i.use_shared_random = true;
        i.primary_expression_table = true;
        WorldAttackInput attack;
        attack.actor = id;
        attack.weapon = {0, 100, 1, 0, 0};
        attack.drop_ticket = 99; // 独立拒绝掉物夹具；不认证原版随机掉物轨迹。
        i.attack = attack;
        return i;
    };
    a.presentation = [&trace, &round, name, target](const Owner &o,
                         const WorldActorPresentationRequest &r) -> std::optional<Owner> {
        auto next = o;
        if (r.hit && r.hit->kind == HitRequestKind::face_attacker) {
            if (!r.target)
                return {};
            const auto &from = o.common.world.ai.battle.actors.at(r.actor).position;
            const auto &to = o.common.world.ai.battle.actors.at(*r.target).position;
            next.common.world.ai.battle.actors.at(*r.target).control.facing =
                from.z > to.z ? (from.x > to.x ? 0 : 3) : (from.x > to.x ? 1 : 2);
            next.script_entries.push_back(100); // 明确诊断输出，非原程序脚本编号。
        } else if (r.hit && r.hit->kind == HitRequestKind::attack_sound) {
            next.script_entries.push_back(r.hit->parameter);
        } else if (r.attack && r.attack->kind == WorldAttackVisual::contact) {
            next.script_entries.push_back(101); // 接触请求观察，不伪造全原皮肤。
        } else {
            return {};
        }
        hit_row(trace, name, round, "after-hit-presentation", next, target);
        return next;
    };
    WorldNonactorScheduleAdapter<Owner> n;
    n.read_common = a.read_common;
    n.write_common = a.write_common;
    n.read_routes = [](const Owner &o) {
        return WorldNonactorScheduleState{o.common, o.random, {}};
    };
    n.write_routes = [](Owner &o, const WorldNonactorScheduleState &r) {
        o.random = r.random;
        return true;
    };
    n.primary_expression_table = true;
    n.projectile = [](const Owner &, std::uint64_t id) -> std::optional<WorldProjectileInput> {
        WorldProjectileInput i;
        i.projectile = id;
        i.drop_ticket = 99;
        i.box = CollisionBox{-1, 0, 2, 2};
        i.monster_boxes[0] = CollisionBox{-10, 10, 20, 20};
        return i; // 合法隔离碰撞框，不宣称原素材尺寸。
    };
    n.encounter = [](const Owner &, std::uint64_t id) -> std::optional<EncounterCommitInput> {
        EncounterCommitInput i;
        i.encounter = id;
        return i;
    };
    n.request = [](Owner &o,
                   const WorldNonactorRequest &r) -> std::optional<WorldNonactorWriteback> {
        WorldNonactorWriteback fields;
        fields.globals = encounter_external_writeback(o.common.world.ai);
        if (r.kind == WorldNonactorRequestKind::projectile_contact && r.target &&
            r.source_position) {
            auto effects = o.common.world.ai.contexts.at(*r.target).effects;
            // 同现有非人物套件：typed cd10隔离夹具，不代替像素锚点合同。
            effects.display.push_back({10, 0, 0, 0, 0});
            fields.target_effects = std::move(effects);
            o.script_entries.push_back(101);
        } else if (r.kind == WorldNonactorRequestKind::projectile_hit && r.hit && r.target) {
            if (r.hit->kind == HitRequestKind::face_attacker && r.caster) {
                const auto &from = o.common.world.ai.battle.actors.at(*r.caster).position;
                const auto &to = o.common.world.ai.battle.actors.at(*r.target).position;
                fields.target_facing =
                    from.z > to.z ? (from.x > to.x ? 0 : 3) : (from.x > to.x ? 1 : 2);
                o.script_entries.push_back(100);
            } else if (r.hit->kind == HitRequestKind::attack_sound) {
                o.script_entries.push_back(r.hit->parameter);
            } else {
                return {};
            }
        } else {
            return {}; // 不给未知对象/事件/视觉域默认成功消费者。
        }
        return fields;
    };
    a.other =
        [&trace, &round, name, target, n](const Owner &o, const WorldScheduleCall &call,
            const CombatInfluenceCandidate &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        if (call.stage == WorldScheduleStage::arrival_front)
            return OwnedWorldScheduleStep<Owner>{o}; // 本批不测试到访，显式隔离原前段。
        if (call.stage == WorldScheduleStage::prefix_effects) {
            if (!call.id || !call.effects || !call.effects->sounds.empty() ||
                !call.effects->growth.empty())
                return {};
            if (*call.id == target.value)
                hit_row(trace, name, round, "after-target-d-prefix", o, target);
            return OwnedWorldScheduleStep<Owner>{o}; // 夹具无待成长/到期声音；拒绝有效未消费输出。
        }
        if (call.stage == WorldScheduleStage::projectile)
            hit_row(trace, name, round, "before-projectile", o, target);
        auto next = prepare_owned_world_nonactor_stage(o, call, field, n);
        if (next && call.stage == WorldScheduleStage::projectile)
            hit_row(trace, name, round, "after-projectile", next->state, target);
        return next;
    };
    return a;
}
void hit_reaction_owner_trace(HitTrace &trace) {
    struct Scenario {
        const char *name;
        CharacterId target;
        int kind; // 0人物近战、1怪物近战、2箭、3迟到法术、4独立状态4、5两怪近战。
        bool miss;
        int damage, delay, second_delay, rounds;
    };
    const std::vector<Scenario> scenarios{
        {"human-melee-monster", {2}, 0, false, 0, 0, -1, 8},
        {"monster-melee-human", {1}, 1, false, 0, 0, -1, 8},
        {"melee-miss-monster", {2}, 0, true, 0, 0, -1, 8},
        {"melee-miss-human", {1}, 1, true, 0, 0, -1, 8},
        {"arrow-monster", {2}, 2, false, 0, 0, -1, 8},
        {"spell-delayed-monster", {2}, 3, false, 3, 2, -1, 10},
        {"spell-zero-monster", {2}, 3, false, 0, 0, -1, 8},
        {"spell-death-monster", {2}, 3, false, 100, 0, -1, 8},
        {"melee-death-human", {1}, 1, false, 100, 0, -1, 8},
        {"spell-second-hit-monster", {2}, 3, false, 3, 0, 3, 11},
        {"melee-second-hit-human", {1}, 5, false, 0, 0, 3, 11},
        {"independent-state4-monster", {2}, 4, false, 0, 0, -1, 7},
        {"independent-state4-human", {1}, 4, false, 0, 0, -1, 7}};
    for (const auto &s : scenarios) {
        auto o = hit_owner(s.target);
        auto &ai = o.common.world.ai;
        const CharacterId caster{s.target.value == 1 ? 2U : 1U};
        auto &source = ai.battle.actors.at(caster);
        source.miss = s.miss;
        if (s.kind < 2 || s.kind == 5) {
            source.control.action = s.kind == 0 ? 1 : 6;
            source.control.action_counter = s.kind == 0 ? 4 : 11;
            source.control.queue = {{s.kind == 0 ? 14 : 17}, {1, 1000, 0}};
            source.attack_destination = ai.battle.actors.at(s.target).position;
            if (s.damage == 100)
                ai.battle.actors.at(s.target).hp = {0, 1, 1, 1, false, 0}; // 合法低HP条件夹具。
            if (s.kind == 5) {
                auto second = source;
                second.id = {3};
                second.position = second.attack_position = {330, 0, 250};
                second.control.action_counter = 8; // 各自真实d推进，第4轮到近战12窗口。
                ai.battle.actors.emplace(second.id, second);
                ai.monster_order.push_back(second.id);
                ai.contexts.emplace(second.id, ai.contexts.at(source.id));
                o.common.world.actors.emplace(second.id, RescueActorContext{});
                ai.encounters.at(3).members.push_back(second.id);
            }
        } else if (s.kind == 2) {
            auto p = prepare_projectile(ProjectileKind::arrow, caster, s.target, source.position,
                                        ai.battle.actors.at(s.target).position, 0);
            check(p.candidate.has_value(), "hit trace valid arrow preparation");
            p.candidate->position.x = 273; // 下次17水平单位后恰抵290；碰撞框是明确夹具。
            ai.projectiles.emplace(9, *p.candidate);
            ai.projectile_order = {9};
        } else if (s.kind == 3) {
            auto p = prepare_projectile(ProjectileKind::delayed_damage, caster, s.target, {}, {}, 0,
                                        0, s.damage, s.delay);
            check(p.candidate.has_value(), "hit trace valid delayed payload preparation");
            ai.projectiles.emplace(9, *p.candidate);
            ai.projectile_order = {9};
            if (s.second_delay >= 0) {
                p.candidate->delay = s.second_delay;
                ai.projectiles.emplace(10, *p.candidate);
                ai.projectile_order.push_back(10);
            }
        } else {
            auto &v = ai.battle.actors.at(s.target);
            v.control.state = 4;
            v.control.queue.clear();
            v.hit_flash = 7; // 独立合法状态快照，不能称自然命中进入4。
            o.common.world.actors.at(s.target).horizontal_velocity = {0.012F, 2.5F};
        }
        int round = 0;
        auto a = hit_adapter(trace, s.name, round, s.target);
        hit_row(trace, s.name, round, "initial", o, s.target);
        const int hit_round = (s.kind < 2 || s.kind == 5) ? 1 : s.kind == 2 ? 1 : s.delay + 1;
        for (round = 1; round <= s.rounds; ++round) {
            const auto first_row = trace.size();
            const auto before = o.common.world.ai.battle.actors.at(s.target);
            auto r = prepare_world_actor_schedule(o, {}, a);
            if (!r.state)
                throw std::runtime_error(std::string(s.name) + " round " + std::to_string(round) +
                                         " owner schedule error " +
                                         std::to_string(static_cast<int>(r.error)));
            o = std::move(*r.state);
            const auto &v = o.common.world.ai.battle.actors.at(s.target);
            hit_row(trace, s.name, round, "committed-round-end", o, s.target);
            // d先后决定首轮末6/7；不把OS绘制帧或统一命中帧7当作oracle。
            const int latest_hit =
                s.second_delay >= 0 && round >= s.second_delay + 1 ? s.second_delay + 1 : hit_round;
            const int first_flash = s.kind == 0 ? 6 : 7;
            const int expected_aw = s.kind == 4 ? 7 - round
                                    : round < hit_round
                                        ? 0
                                        : std::max(0, first_flash - (round - latest_hit));
            check(v.hit_flash == expected_aw, "Owner aw follows human/monster/bo stage order");
            if (s.kind != 4) {
                const int expected_state = s.damage == 100 && !s.miss && round >= hit_round
                                               ? (s.target.value == 1 ? 2 : 3)
                                               : 1;
                check(v.control.state == expected_state && !(v.control.flags & 64U) &&
                          v.physics_pause == 0,
                      "Owner hit never adds state4/flag64/P; lethal2/3 remains separate");
                const int expected_hits = s.miss || round < hit_round                          ? 0
                                          : s.second_delay >= 0 && round >= s.second_delay + 1 ? 2
                                                                                               : 1;
                check(v.hit_count == expected_hits,
                      "Owner hit count includes0 and second hit, excludesmiss");
                if (s.miss || (s.kind == 3 && s.damage == 0))
                    check(v.hp.target == 100, "miss and zero targetHP remain100");
                for (std::size_t n = first_row; n < trace.size(); ++n) {
                    const auto &row = trace[n];
                    if (row.phase == "after-hit-presentation" || row.phase == "after-projectile") {
                        check(row.velocity.x == 3.25F && row.velocity.z == -2.5F,
                              "published hit changes no horizontal velocity");
                        // 同轮bo逆序会先更新尚在等待的第二颗投射；只有这次真正提交命中
                        // 才要求新aw/aq，不能用整轮存在另一次命中替代本调用资格。
                        const bool submitted_hit = row.phase != "after-projectile" ||
                            (n > first_row && trace[n - 1].phase == "before-projectile" &&
                             row.value.hit_count > trace[n - 1].value.hit_count);
                        if (submitted_hit && (round == hit_round ||
                            (s.second_delay >= 0 && round == s.second_delay + 1))) {
                            if (row.value.hit_flash != 7 || row.value.label_timer != 16 ||
                                row.value.miss_label != s.miss)
                                throw std::runtime_error(std::string(s.name) + " round " +
                                    std::to_string(round) + " phase=" + row.phase +
                                    " expected aw7/aq16/miss=" + std::to_string(s.miss) +
                                    " actual=" + std::to_string(row.value.hit_flash) + "/" +
                                    std::to_string(row.value.label_timer) + "/" +
                                    std::to_string(row.value.miss_label));
                            ++checks;
                        }
                    }
                }
            } else {
                const auto velocity = o.common.world.actors.at(s.target).horizontal_velocity;
                check(velocity.x == 0 && v.position.z > before.position.z,
                      "independent4 clamps sub0.01 axis and moves logical n while aw positive");
                check(v.control.state == (round < 7 ? 4 : 1),
                      "independent4 restores state1 only when oldB reaches6");
            }
            check(o.common.world.ai.battle.actors.size() == (s.kind == 5 ? 3U : 2U) &&
                      o.common.world.ai.retired_actors.empty() &&
                      o.common.world.ai.projectiles.size() <= 2,
                  "short trace has bounded actors/retired/projectile resources");
            o.script_entries.clear(); // 本轮typed诊断输出已记录消费，不积累未消费通知。
        }
        check((s.kind != 2 && s.kind != 3) ||
                  (o.common.world.ai.projectiles.empty() && o.common.world.ai.projectile_order.empty()),
              "consumed projectile references/order retire by trace end");
    }
}
void hit_invalid_human_projectile_rejected() {
    for (const int damage : {0, 3, 100}) {
        auto o = hit_owner({1});
        auto p = prepare_projectile(ProjectileKind::delayed_damage, {2}, {1}, {}, {}, 0, 0, damage, 0);
        check(p.candidate.has_value(), "invalid caster fixture has structurally valid projectile");
        o.common.world.ai.projectiles.emplace(9, *p.candidate);
        o.common.world.ai.projectile_order = {9};
        HitTrace rejected;
        int round = 1;
        auto a = hit_adapter(rejected, "rejected-monster-spell-human", round, {1});
        check(valid_world_schedule_owner(o.common), "invalid caster fixture starts from valid Owner");
        const auto r = prepare_world_actor_schedule(o, {}, a);
        check(!r.state && r.error != WorldScheduleError::none && o.random.draws() == 0 &&
                  o.common.updates == 0 && o.common.world.ai.projectile_order == std::vector<std::uint64_t>{9} &&
                  o.common.world.ai.projectiles.at(9).counter == 0 &&
                  o.common.world.ai.battle.actors.at({1}).hp.target == 100 &&
                  o.common.world.ai.battle.actors.at({1}).hit_flash == 0 &&
                  o.common.world.ai.battle.actors.at({1}).control.queue ==
                      std::vector<LegacyActorControl>{{1, 1000, 0}} && o.script_entries.empty(),
              "unsupported monster spell to human rejects complete Owner without partial mutation");
    }
}
// 同一合法opcode0目标从同一Owner快照分出受击/无投射两条轨迹，
// 比较真实c/d/FIFO/尾部结果；不以自己重算的位移公式作上层oracle。
void hit_moving_twins(HitTrace &trace) {
    for (const CharacterId target : {CharacterId{1}, CharacterId{2}}) {
        auto hit = hit_owner(target);
        auto &actor = hit.common.world.ai.battle.actors.at(target);
        actor.control.queue = {{0, static_cast<int>(actor.position.x), 450}, {1, 1000, 0}};
        auto control = hit;
        const CharacterId caster{target.value == 1 ? 2U : 1U};
        if (target.value == 1) {
            auto &attacker = hit.common.world.ai.battle.actors.at(caster);
            attacker.control.action = 6;
            attacker.control.action_counter = 11;
            attacker.control.queue = {{17}, {1, 1000, 0}};
            attacker.attack_destination = actor.position;
        } else {
            auto p = prepare_projectile(ProjectileKind::delayed_damage, caster, target,
                                        {}, {}, 0, 0, 3, 0);
            check(p.candidate.has_value(), "moving twin valid delayed-hit condition fixture");
            hit.common.world.ai.projectiles.emplace(9, *p.candidate);
            hit.common.world.ai.projectile_order = {9};
        }
        const std::string hit_name = target.value == 1 ? "melee-moving-human" : "spell-moving-monster";
        const std::string control_name = target.value == 1 ? "control-moving-human" : "control-moving-monster";
        int round{};
        auto hit_route = hit_adapter(trace, hit_name, round, target);
        auto control_route = hit_adapter(trace, control_name, round, target);
        hit_row(trace, hit_name, round, "initial", hit, target);
        hit_row(trace, control_name, round, "initial", control, target);
        for (round = 1; round <= 8; ++round) {
            const auto previous = hit.common.world.ai.battle.actors.at(target).position;
            auto h = prepare_world_actor_schedule(hit, {}, hit_route);
            auto c = prepare_world_actor_schedule(control, {}, control_route);
            if (!h.state || !c.state)
                throw std::runtime_error(hit_name + " moving twin Owner failed at " +
                    std::to_string(round) + " hit=" + std::to_string(static_cast<int>(h.error)) +
                    " control=" + std::to_string(static_cast<int>(c.error)));
            hit = std::move(*h.state);
            control = std::move(*c.state);
            const auto &hv = hit.common.world.ai.battle.actors.at(target);
            const auto &cv = control.common.world.ai.battle.actors.at(target);
            const auto &hr = hit.common.world.actors.at(target).horizontal_velocity;
            const auto &cr = control.common.world.actors.at(target).horizontal_velocity;
            check(hv.position.x == cv.position.x && hv.position.height == cv.position.height &&
                      hv.position.z == cv.position.z && hv.position.z > previous.z &&
                      hr.x == cr.x && hr.z == cr.z && hv.control.queue == cv.control.queue &&
                      hv.control.state == 1 && cv.control.state == 1 &&
                      hv.control.flags == cv.control.flags && !(hv.control.flags & 64U) &&
                      hv.physics_pause == 0 && cv.physics_pause == 0,
                  "Owner ordinary movement and FIFO remain identical while hit aw is positive");
            check(hv.hit_flash == 8 - round && cv.hit_flash == 0 &&
                      hv.hit_count == 1 && cv.hit_count == 0 && hv.hp.target < 100 &&
                      cv.hp.target == 100 && hv.damage_total == 100 - hv.hp.target,
                  "moving twin has real nonzero hit and distinct aw/HP without frozen movement");
            hit_row(trace, hit_name, round, "committed-round-end", hit, target);
            hit_row(trace, control_name, round, "committed-round-end", control, target);
            for (auto *o : {&hit, &control}) {
                check(o->common.world.ai.projectiles.empty() &&
                          o->common.world.ai.projectile_order.empty() &&
                          o->common.world.ai.battle.actors.size() == 2 &&
                          o->common.world.ai.retired_actors.empty(),
                      "moving twins consume projectile references and keep two live actors");
                o->script_entries.clear();
            }
        }
    }
}
void write_hit_trace(const HitTrace &rows, const char *file) {
    std::ofstream out(file);
    if (!out)
        throw std::runtime_error("cannot open requested hit trace output");
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "scenario,round,phase,actor,state,B,action,l,aw,aq,miss,ao,ap,hp,flags,P,x,y,z,vx,vz,"
           "au_x,au_y,au_z,queue_front,actors,projectiles,retired,outputs,draws\n";
    for (const auto &r : rows) {
        const auto &a = r.value;
        out << r.scenario << ',' << r.round << ',' << r.phase << ',' << r.actor.value << ','
            << a.control.state << ',' << a.state_counter << ',' << a.control.action << ','
            << a.control.action_counter << ',' << a.hit_flash << ',' << a.label_timer << ','
            << a.miss_label << ',' << a.damage_total << ',' << a.hit_count << ',' << a.hp.target
            << ',' << a.control.flags << ',' << a.physics_pause << ',' << a.position.x << ','
            << a.position.height << ',' << a.position.z << ',' << r.velocity.x << ','
            << r.velocity.z << ',' << a.attack_position.x << ',' << a.attack_position.height << ','
            << a.attack_position.z << ','
            << (a.control.queue.empty() ? -1 : a.control.queue.front()[0]) << ',' << r.actors << ','
            << r.projectiles << ',' << r.retired << ',' << r.outputs << ',' << r.draws << '\n';
    }
    if (!out)
        throw std::runtime_error("hit trace write failed");
}

} // namespace
int main(int argc, char **argv) {
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
        HitTrace hit_trace;
        hit_reaction_owner_trace(hit_trace);
        hit_invalid_human_projectile_rejected();
        hit_moving_twins(hit_trace);
        if (argc == 3 && std::string(argv[1]) == "--hit-reaction-trace")
            write_hit_trace(hit_trace, argv[2]);
        else if (argc != 1)
            throw std::runtime_error("expected --hit-reaction-trace OUTPUT.csv");
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
