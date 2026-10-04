#include "ark/simulation/rules/world_actor_routes.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool yes, const char *what) {
    ++checks;
    if (!yes)
        throw std::runtime_error(what);
}
WorldActorRoutesState fixture(int state = 5) {
    WorldActorRoutesState s;
    s.world.map = {8, 8, std::vector<LegacyMapCell>(64)};
    for (auto &cell : s.world.map.cells) {
        cell.legacy_state = 4;
        cell.category = RouteCategory::road;
    }
    s.facts = {s.world.map, std::vector<int>(64, 1), std::vector<std::uint32_t>(64), {0, 3, 0, 3}};
    BattleActorRecord a;
    a.id = {1};
    a.control.state = state;
    a.control.flags = 2;
    a.capacity = 100;
    a.hp = {0, 100, 100, 100, false, 0};
    a.position = {550, 0, 550};
    a.attack_position = a.position;
    a.baseline = 5;
    a.state_counter = 10;
    s.world.ai.battle.actors.emplace(a.id, a);
    s.world.ai.human_order = {a.id};
    s.world.ai.contexts.emplace(a.id, RewardActorContext{{5, 5}, false, {}, {}, true, {11, 11}});
    s.world.actors.emplace(a.id, RescueActorContext{});
    s.world.actors.at(a.id).destination = Position{};
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    g.definition.legacy_u = 50;
    s.world.ai.professions = {{{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 1, true}};
    g.derived = *derive_human_stats(g.definition, s.world.ai.professions).candidate;
    s.world.ai.growth.emplace(0, g);
    s.world.ai.battle.humans.emplace(0, HumanBattleRecord{});
    s.shop_humans.emplace(0, ShopHumanRecord{});
    s.shop_actors.emplace(a.id, ShopActorRecord{});
    s.dungeon_actors.emplace(a.id, DungeonActorProgress{});
    s.human_definition_state.emplace(0, 0);
    return s;
}
WorldActorDecisionInput decision() {
    WorldActorDecisionInput i;
    i.actor = {1};
    i.daily.expressions = {{999, 4, {}}, {999, 3, {}}};
    i.daily.spawn_ticket = 999;
    i.lifecycle.expressions = {{999, 2, {}}, {999, 1, {}}};
    i.actor_box = CollisionBox{-4, 4, 8, 8};
    i.rescue_box = i.actor_box;
    i.special_expression = WorldExpressionTicket{999, 1, {}};
    return i;
}
void all_state_routing() {
    for (int state = 0; state <= 20; ++state) {
        auto s = fixture(state);
        auto i = decision();
        auto &actor = s.world.ai.battle.actors.at({1});
        if (state == 0) {
            WorldPathInput path;
            path.actor = {1};
            path.facts = s.facts;
            i.daily.path = path;
        }
        if (state == 1) {
            WorldCombatPolicyInput combat;
            combat.actor = {1};
            combat.weapon = {0, 100, 1, 0, 0};
            combat.boost_ticket = 99;
            i.combat = combat;
        }
        if (state == 3 || state == 8 || state == 9 || state == 17) {
            actor.kind = ActorKind::monster;
            actor.baseline = 17;
            s.world.ai.human_order.clear();
            s.world.ai.monster_order = {{1}};
            s.world.ai.battle.monsters.emplace(0, MonsterBattleRecord{});
            s.world.ai.monster_growth.emplace(0, RewardMonsterDefinition{});
        }
        const auto r = prepare_world_actor_decision(s, i);
        if (!r.candidate)
            throw std::runtime_error("state " + std::to_string(state) + " error " +
                                     std::to_string(static_cast<int>(r.error)));
        check(!r.candidate->removed && !r.candidate->delete_requested,
              "each source state selects real domain branch without premature removal");
        check(s.world.ai.battle.actors.at({1}).control.state == state &&
                  s.world.ai.battle.actors.at({1}).state_counter == 10,
              "all-state routing leaves source and common counters unchanged");
        if (state == 3)
            check(r.candidate->state.world.ai.battle.actors.at({1}).attack_position.height > 0 &&
                      r.candidate->state.world.ai.battle.actors.at({1}).position.height == 0,
                  "monster death arc affects au, not n, before oldB12 deletion");
    }
    auto s = fixture(8);
    s.world.ai.battle.actors.at({1}).state_counter = 73;
    auto i = decision();
    i.event = [](const auto &owner, int id) -> std::optional<WorldActorRoutesState> {
        auto next = owner;
        next.world.ai.battle.events.insert(id);
        next.item_rewards = 19; // 外部私有投影副作用夹具，不称脚本实现。
        return next;
    };
    const auto r = prepare_world_actor_decision(s, i);
    check(r.candidate && r.candidate->consumed_events == std::vector<int>{90} &&
              r.candidate->state.item_rewards == 19 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 17,
          "synchronous90 retains outer-domain side effects before c17 without second world");
    i.event = {};
    check(!prepare_world_actor_decision(s, i).candidate && s.item_rewards == 0,
          "missing synchronous event rolls back full outer owner");
}
void fifo_and_failures() {
    auto s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{21},   {22, 30, 0}, {32},
                                                      {3, 0}, {26},        {25, 10}};
    std::vector<int> calls;
    const auto provider =
        [&](const auto &, CharacterId,
            const LegacyActorControl &c) -> std::optional<WorldActorCommandInput> {
        calls.push_back(c[0]);
        return WorldActorCommandInput{};
    };
    auto r = prepare_world_actor_control(s, {1}, provider);
    check(r.candidate && r.candidate->flow == WorldControlFlow::delete_requested &&
              calls == std::vector<int>{21, 22, 32, 26} && r.candidate->domain_segments == 4 &&
              r.candidate->local_commands == 1 &&
              r.candidate->state.human_definition_state.at(0) == 1 &&
              r.candidate->state.world.ai.battle.actors.at({1}).position.height == 30,
          "null-O21 only consumes current command; fresh22/32/local3/26 same-call delete skips25");
    check(s.human_definition_state.at(0) == 0 &&
              s.world.ai.battle.actors.at({1}).control.queue.size() == 6,
          "domain routing never mutates source before publication");
    s.world.ai.battle.actors.at({1}).control.queue = {{32}, {25, 10}};
    r = prepare_world_actor_control(s, {1}, provider);
    check(!r.candidate && s.world.ai.battle.actors.at({1}).vertical_velocity == 0,
          "late missing old-u rolls back earlier actual jump velocity");
    s.world.ai.battle.actors.at({1}).control.queue = {{1, 2, 0}, {32}};
    r = prepare_world_actor_control(s, {1}, {});
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.queue.front()[1] == 1,
          "positive wait needs no domain provider and stops entire interpreter");
    s.world.ai.battle.actors.at({1}).control.queue = {{32}};
    check(prepare_world_actor_control(s, {1}, {}).error == WorldActorRouteError::missing_consumer,
          "domain front cannot succeed with absent provider");
    check(!prepare_world_actor_control(s, {1}, provider, 0).candidate,
          "zero maintenance budget refuses without deferring original command");
    s.world.ai.battle.actors.at({1}).control.queue = {{2, 0}, {32}, {9}};
    calls.clear();
    r = prepare_world_actor_control(s, {1}, provider);
    check(r.candidate && r.candidate->state.world.ai.battle.actors.at({1}).control.queue.empty() &&
              calls == std::vector<int>{2},
          "real c0 replaces queue; old32/9 are not replayed from snapshot");
}
void shared_random_sequence() {
    auto s = fixture();
    s.random = WorldRandomStream::from_raw({-1, 6, 3, 2, 11, 19, 99, 2, 5, 7});
    s.world.ai.battle.actors.at({1}).control.queue = {{18, 3, 0}, {23}, {10, 0}, {1, 2, 0}};
    const auto provider = [](const auto &, CharacterId,
                             const LegacyActorControl &) -> std::optional<WorldActorCommandInput> {
        WorldActorCommandInput i;
        i.use_shared_random = true;
        i.primary_expression_table = true;
        return i;
    };
    const auto r = prepare_world_actor_control(s, {1}, provider);
    check(
        r.candidate && r.candidate->state.random.draws() == 10 && s.random.draws() == 0 &&
            r.candidate->flow == WorldControlFlow::held &&
            r.candidate->state.world.ai.contexts.at({1}).effects.display.front() ==
                ActorEffectRecord{12, 0, 30, 3, 2},
        "expression1000/variant4, launch4 and seven actual wander draws share one private cursor");
    auto failed = s;
    failed.random = WorldRandomStream::from_raw({-1, 6, 3, 2, 11, 19, 99, 2, 5});
    check(!prepare_world_actor_control(failed, {1}, provider).candidate &&
              failed.random.draws() == 0 &&
              failed.world.ai.contexts.at({1}).effects.display.empty(),
          "late wander exhaustion rolls back expression, launch and entire shared random stream");
    s = fixture();
    s.random = WorldRandomStream::from_raw({0});
    s.world.ai.contexts.at({1}).effects.display = {{12, 0, 30, 3, 0}};
    s.world.ai.battle.actors.at({1}).control.queue = {{18, 4, 0}};
    const auto suppressed = prepare_world_actor_control(s, {1}, provider);
    check(suppressed.candidate && suppressed.candidate->state.random.draws() == 1 &&
              suppressed.candidate->state.world.ai.contexts.at({1}).effects.display.size() == 1,
          "suppressed guaranteed expression still draws1000, never a variant");
}
void decision_shared_random() {
    auto s = fixture(5);
    s.random = WorldRandomStream::from_raw({999, -999});
    auto i = decision();
    i.use_shared_random = true;
    i.primary_expression_table = true;
    i.daily.expressions.clear();
    i.daily.spawn_ticket.reset();
    const auto r = prepare_world_actor_decision(s, i);
    check(r.candidate && r.candidate->state.random.draws() == 2 &&
              r.candidate->daily->consumed_spawn && s.random.draws() == 0,
          "state5 expression8 actual failure then L1000 share candidate cursor");
    s = fixture(2);
    s.world.ai.battle.actors.at({1}).state_counter = 900;
    s.random = WorldRandomStream::from_raw({0, 7, 0});
    i.lifecycle.expressions.clear();
    const auto down = prepare_world_actor_decision(s, i);
    check(down.candidate && down.candidate->state.random.draws() == 3 &&
              down.candidate->lifecycle->consumed_expressions == 2 &&
              down.candidate->lifecycle->consumed_variants == 1 &&
              down.candidate->state.world.ai.battle.actors.at({1}).state_counter == 900,
          "old900 c3/variant then suppressedc4 consumes1000 and b retains B");
    s.random = WorldRandomStream::from_raw({0, 7});
    check(!prepare_world_actor_decision(s, i).candidate && s.random.draws() == 0 &&
              s.world.ai.contexts.at({1}).effects.display.empty(),
          "second actual recovery expression exhaustion rolls back HP/control and first variant");
}
void every_control_route() {
    const std::vector<LegacyActorControl> commands{
        {0, 550, 550}, {1, 1, 0},   {2, 0},     {3, 0},  {4, 0},     {5, 16},       {6, 16},
        {7, 2},        {8, 0},      {9},        {10, 0}, {11},       {12},          {13},
        {14},          {15},        {16},       {17},    {18, 6, 0}, {19, 0, 0, 0}, {20},
        {21},          {22, 10, 0}, {23},       {24},    {25, 10},   {26},          {27, 0, 0},
        {28, 0},       {29, 21, 0}, {30, 1, 0}, {31},    {32},       {33}};
    for (int op = 0; op < 34; ++op) {
        auto s = fixture();
        auto &actor = s.world.ai.battle.actors.at({1});
        actor.control.queue = {commands[op]};
        actor.combo_count = 1;
        actor.control.action = op == 15 || op == 16 ? 4 : 1;
        actor.control.action_counter = 30;
        if (op == 24)
            actor.control.flags |= 1024;
        if (op == 17) {
            actor.kind = ActorKind::monster;
            actor.control.action = 6; // 原怪物17只接受动作3/6；30已结束6型扑击。
            s.world.ai.human_order.clear();
            s.world.ai.monster_order = {{1}};
            s.world.ai.battle.monsters.emplace(0, MonsterBattleRecord{});
            s.world.ai.monster_growth.emplace(0, RewardMonsterDefinition{});
        }
        const auto provider =
            [](const WorldActorRoutesState &owner, CharacterId id,
               const LegacyActorControl &) -> std::optional<WorldActorCommandInput> {
            WorldActorCommandInput i;
            i.facility.expressions = {{999, 3, {}}};
            i.facility.launch_tickets = {0};
            i.cached_view = Position{33, 44};
            i.sound_projection = [](Position p) -> std::optional<Position> {
                return p;
            }; // 显式表现投影夹具。
            i.wander_tickets = {0, 0, 0, 0, 0, 0, 0};
            i.attack = WorldAttackInput{};
            i.attack->actor = id;
            i.attack->weapon = {0, 100, 1, 0, 0};
            i.equipment = {{1, 0, 1, 0, true, 0, {0, 0, 0, 0}},
                           {2, 0, 1, 0, true, 0, {0, 0, 0, 0}}};
            i.departure = WorldDepartureControlInput{};
            auto &d = i.departure->departure;
            d.actor = id;
            d.catalogue.town = owner.facts.town;
            d.catalogue.definitions = {{0, 0, 0}};
            d.catalogue.cell_definition_ids = std::vector<int>(owner.world.map.cells.size());
            return i;
        };
        const auto r = prepare_world_actor_control(s, {1}, provider);
        if (!r.candidate)
            throw std::runtime_error("opcode " + std::to_string(op) + " error " +
                                     std::to_string(static_cast<int>(r.error)) + " control " +
                                     std::to_string(static_cast<int>(r.control_error)));
        check(
            s.world.ai.battle.actors.at({1}).control.queue ==
                std::vector<LegacyActorControl>{commands[op]},
            "all34 decoded opcodes route to actual local/domain consumers without source mutation");
    }
}
} // namespace
int main() {
    try {
        all_state_routing();
        fifo_and_failures();
        shared_random_sequence();
        decision_shared_random();
        every_control_route();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
