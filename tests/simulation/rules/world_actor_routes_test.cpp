#include "ark/simulation/actors/rules/world_actor_routes.hpp"

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
enum class ControlEntry { copied, consuming };
WorldActorDecisionResult decide(ControlEntry entry, const WorldActorRoutesState &source,
                                const WorldActorDecisionInput &input) {
    if (entry == ControlEntry::copied)
        return prepare_world_actor_decision(source, input);
    auto disposable = source;
    return prepare_world_actor_decision_consuming(std::move(disposable), input);
}
WorldActorControlResult control(ControlEntry entry, const WorldActorRoutesState &source,
                                CharacterId actor, const WorldActorCommandProvider &provider,
                                std::size_t budget = 4096) {
    if (entry == ControlEntry::copied)
        return prepare_world_actor_control(source, actor, provider, budget);
    // Match the scheduler: transfer a disposable projection, keep the original
    // owner available to all existing rollback/source-immutability assertions.
    auto disposable = source;
    return prepare_world_actor_control_consuming(std::move(disposable), actor, provider, budget);
}
void independent_nested_world(RescueWorldState &published, const RescueWorldState &audit,
                              const RescueWorldState &source) {
    check(audit.map.width == 8 && audit.map.height == 8 && audit.map.cells.size() == 64 &&
              audit.ai.battle.actors.count({1}) == 1 && audit.ai.contexts.count({1}) == 1 &&
              audit.actors.count({1}) == 1 && audit.ai.growth.count(0) == 1,
          "decision nested audit retains its complete map and actor domains");
    const auto audit_counter = audit.ai.battle.actors.at({1}).state_counter;
    const auto audit_cell = audit.map.cells.front().legacy_state;
    published.ai.battle.actors.at({1}).state_counter = -1;
    published.map.cells.front().legacy_state = -1;
    check(audit.ai.battle.actors.at({1}).state_counter == audit_counter &&
              audit.map.cells.front().legacy_state == audit_cell &&
              source.ai.battle.actors.at({1}).state_counter == 10 &&
              source.map.cells.front().legacy_state == 4,
          "published decision, original input and nested audit own independent mutable storage");
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
void all_state_routing(ControlEntry entry) {
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
        auto r = decide(entry, s, i);
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
        if (r.candidate->daily) {
            independent_nested_world(r.candidate->state.world, r.candidate->daily->state.world,
                                     s.world);
            if (r.candidate->daily->path)
                independent_nested_world(r.candidate->state.world, r.candidate->daily->path->state,
                                         s.world);
        }
        if (r.candidate->monster)
            independent_nested_world(r.candidate->state.world, r.candidate->monster->state,
                                     s.world);
        if (r.candidate->lifecycle)
            independent_nested_world(r.candidate->state.world, r.candidate->lifecycle->state,
                                     s.world);
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
    const auto r = decide(entry, s, i);
    check(r.candidate && r.candidate->consumed_events == std::vector<int>{90} &&
              r.candidate->state.item_rewards == 19 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 17,
          "synchronous90 retains outer-domain side effects before c17 without second world");
    i.event = {};
    check(!decide(entry, s, i).candidate && s.item_rewards == 0,
          "missing synchronous event rolls back full outer owner");
}
void fifo_and_failures(ControlEntry entry) {
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
    auto r = control(entry, s, {1}, provider);
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
    r = control(entry, s, {1}, provider);
    check(!r.candidate && s.world.ai.battle.actors.at({1}).vertical_velocity == 0,
          "late missing old-u rolls back earlier actual jump velocity");
    s.world.ai.battle.actors.at({1}).control.queue = {{1, 2, 0}, {32}};
    r = control(entry, s, {1}, {});
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.queue.front()[1] == 1,
          "positive wait needs no domain provider and stops entire interpreter");
    s.world.ai.battle.actors.at({1}).control.queue = {{32}};
    check(control(entry, s, {1}, {}).error == WorldActorRouteError::missing_consumer,
          "domain front cannot succeed with absent provider");
    check(!control(entry, s, {1}, provider, 0).candidate,
          "zero maintenance budget refuses without deferring original command");
    const auto stale = control(entry, s, {2}, provider);
    check(!stale.candidate && stale.error == WorldActorRouteError::stale_actor &&
              stale.control_error == WorldControlError::stale_actor,
          "missing actor preserves both route and control refusal codes");
    auto malformed = s;
    malformed.facts.map.cells.front().legacy_state = 3;
    const auto mismatch = control(entry, malformed, {1}, provider);
    check(!mismatch.candidate && mismatch.error == WorldActorRouteError::invalid_input &&
              mismatch.control_error == WorldControlError::invalid_adapter &&
              malformed.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{32}},
          "mismatched world/facts maps refuse without consuming the command");
    const auto missing =
        control(entry, s, {1},
                [](const auto &, auto, const auto &) -> std::optional<WorldActorCommandInput> {
                    return {};
                });
    check(!missing.candidate && missing.error == WorldActorRouteError::missing_fact,
          "present provider with missing current-command input refuses both entry forms");
    const auto throwing =
        control(entry, s, {1},
                [](const auto &, auto, const auto &) -> std::optional<WorldActorCommandInput> {
                    throw std::runtime_error("fixture");
                });
    check(!throwing.candidate && throwing.error == WorldActorRouteError::consumer_failed &&
              s.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{32}},
          "provider exception preserves failure code and original control storage");
    s.world.ai.battle.actors.at({1}).control.queue = {{2, 0}, {32}, {9}};
    calls.clear();
    r = control(entry, s, {1}, provider);
    check(r.candidate && r.candidate->state.world.ai.battle.actors.at({1}).control.queue.empty() &&
              calls == std::vector<int>{2},
          "real c0 replaces queue; old32/9 are not replayed from snapshot");
}
// 显式两人救援夹具：验证完整c0→P路由消费已证递归交付，而非绕过P直接调用。
void recursive_rescue_arrival(ControlEntry entry) {
    auto s = fixture(0);
    auto &carrier = s.world.ai.battle.actors.at({1});
    carrier.object_slot = -2;
    carrier.rescue = CharacterId{2};
    auto rescued = carrier;
    rescued.id = {2};
    rescued.control.state = 16;
    rescued.control.flags |= 512U;
    rescued.object_slot = -1;
    rescued.rescue = CharacterId{1};
    s.world.ai.battle.actors.emplace(rescued.id, rescued);
    s.world.ai.human_order.push_back(rescued.id);
    s.world.ai.contexts.emplace(rescued.id, RewardActorContext{{4, 5}, false, {}, {}});
    s.world.actors.emplace(rescued.id, RescueActorContext{});
    s.world.actors.at(rescued.id).destination = Position{1, 1}; // 倒下前的旧O坐标。
    s.world.human_spending[0] = 0;
    RescueFacility inn;
    inn.placement = {{22}, 22, FacilityShape::single, FacilityOrientation::first, {5, 5}};
    inn.category = 2;
    inn.price = 17;
    s.world.facilities.emplace(22, inn);
    auto old_dungeon = inn;
    old_dungeon.placement = {{33}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    old_dungeon.category = 5;
    s.world.facilities.emplace(33, old_dungeon);
    const auto map =
        bind_facility_map(s.world.map, {{inn.placement, 3}, {old_dungeon.placement, 3}});
    check(map.map.has_value(), "rescue fixture binds exact inn tile");
    s.world.map = *map.map;
    s.facts.map = s.world.map;
    auto &path = s.world.actors.at({1});
    path.binding = ArrivalBinding{{5, 5}, {22}, 22};
    path.destination = Position{5, 5};
    path.path_pending = true;
    FacilityDeparture journey;
    journey.category = 2;
    journey.binding = *path.binding;
    journey.route.steps = {{5, 5}};
    path.journey = journey;
    auto i = decision();
    i.daily.path = WorldPathInput{};
    i.daily.path->actor = {1};
    i.daily.path->facts = s.facts;
    const auto result = decide(entry, s, i);
    check(result.candidate && result.candidate->daily && result.candidate->daily->path &&
              result.candidate->daily->path->arrived,
          "full actor router invokes recursive delivery after actual P arrival");
    const auto &next = result.candidate->state.world;
    check(next.ai.battle.actors.at({1}).object_slot == -1 &&
              !next.ai.battle.actors.at({1}).rescue && !next.ai.battle.actors.at({2}).rescue &&
              next.ai.battle.actors.at({1}).control.state == 14 &&
              next.ai.battle.actors.at({2}).control.state == 14 &&
              next.ai.contexts.at({2}).cell == Position{5, 5} &&
              next.actors.at({2}).destination == Position{5, 5} &&
              next.facilities.at(22).sales == 0 && !next.actors.at({1}).journey,
          "rescued use1 and helper use2 retain old512/256 fee guards and release both R");
    check(s.world.ai.battle.actors.at({1}).object_slot == -2 &&
              s.world.ai.battle.actors.at({2}).control.state == 16,
          "recursive arrival publishes only complete candidate");
    const auto entered = prepare_world_actor_control(
        result.candidate->state, {2},
        [](const auto &, auto, const auto &) -> std::optional<WorldActorCommandInput> {
            return WorldActorCommandInput{};
        });
    check(entered.candidate &&
              entered.candidate->state.world.facilities.at(22).occupants ==
                  std::vector<CharacterId>{{2}} &&
              entered.candidate->state.world.facilities.at(33).occupants.empty(),
          "rescued21 occupies new inn, never routes to pre-rescue dungeon O coordinates");
    auto rest = s;
    rest.world.facilities.at(22).category = 8;
    rest.world.facilities.at(22).detail = 2;
    rest.random = WorldRandomStream::from_raw({2, 3});
    i.use_shared_random = true;
    i.primary_expression_table = true;
    std::vector<std::pair<CharacterId, int>> projected;
    i.rescue_direction_target = [&](CharacterId actor, int direction) -> std::optional<Position> {
        projected.emplace_back(actor, direction);
        const auto cell = rest.world.ai.contexts.at(actor).cell;
        return Position{cell.x * 100 + direction, cell.y * 100};
    };
    const auto seated = decide(entry, rest, i);
    check(seated.candidate && seated.candidate->state.random.draws() == 2 &&
              projected == std::vector<std::pair<CharacterId, int>>{{{2}, 2}, {{1}, 3}} &&
              seated.candidate->state.world.ai.battle.actors.at({2}).control.queue.front() ==
                  LegacyActorControl{0, 402, 500} &&
              seated.candidate->state.world.ai.battle.actors.at({1}).control.queue.front() ==
                  LegacyActorControl{0, 503, 500},
          "full category8 router draws rescued then carrier, each target uses separate old cell");
    rest.random = WorldRandomStream::from_raw({2});
    check(!decide(entry, rest, i).candidate && rest.random.draws() == 0 &&
              rest.world.ai.battle.actors.at({1}).object_slot == -2,
          "second rescue direction exhaustion rolls back both people and shared stream");
}
void shared_random_sequence(ControlEntry entry) {
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
    const auto r = control(entry, s, {1}, provider);
    check(
        r.candidate && r.candidate->state.random.draws() == 10 && s.random.draws() == 0 &&
            r.candidate->flow == WorldControlFlow::held &&
            r.candidate->state.world.ai.contexts.at({1}).effects.display.front() ==
                ActorEffectRecord{12, 0, 30, 3, 2},
        "expression1000/variant4, launch4 and seven actual wander draws share one private cursor");
    auto failed = s;
    failed.random = WorldRandomStream::from_raw({-1, 6, 3, 2, 11, 19, 99, 2, 5});
    check(!control(entry, failed, {1}, provider).candidate && failed.random.draws() == 0 &&
              failed.world.ai.contexts.at({1}).effects.display.empty(),
          "late wander exhaustion rolls back expression, launch and entire shared random stream");
    s = fixture();
    s.random = WorldRandomStream::from_raw({0});
    s.world.ai.contexts.at({1}).effects.display = {{12, 0, 30, 3, 0}};
    s.world.ai.battle.actors.at({1}).control.queue = {{18, 4, 0}};
    const auto suppressed = control(entry, s, {1}, provider);
    check(suppressed.candidate && suppressed.candidate->state.random.draws() == 1 &&
              suppressed.candidate->state.world.ai.contexts.at({1}).effects.display.size() == 1,
          "suppressed guaranteed expression still draws1000, never a variant");
}
void decision_shared_random(ControlEntry entry) {
    auto s = fixture(5);
    s.random = WorldRandomStream::from_raw({999, -999});
    auto i = decision();
    i.use_shared_random = true;
    i.primary_expression_table = true;
    i.daily.expressions.clear();
    i.daily.spawn_ticket.reset();
    const auto r = decide(entry, s, i);
    check(r.candidate && r.candidate->state.random.draws() == 2 &&
              r.candidate->daily->consumed_spawn && s.random.draws() == 0,
          "state5 expression8 actual failure then L1000 share candidate cursor");
    s = fixture(2);
    s.world.ai.battle.actors.at({1}).state_counter = 900;
    s.random = WorldRandomStream::from_raw({0, 7, 0});
    i.lifecycle.expressions.clear();
    const auto down = decide(entry, s, i);
    check(down.candidate && down.candidate->state.random.draws() == 3 &&
              down.candidate->lifecycle->consumed_expressions == 2 &&
              down.candidate->lifecycle->consumed_variants == 1 &&
              down.candidate->state.world.ai.battle.actors.at({1}).state_counter == 900,
          "old900 c3/variant then suppressedc4 consumes1000 and b retains B");
    s.random = WorldRandomStream::from_raw({0, 7});
    check(!decide(entry, s, i).candidate && s.random.draws() == 0 &&
              s.world.ai.contexts.at({1}).effects.display.empty(),
          "second actual recovery expression exhaustion rolls back HP/control and first variant");
}
void delivered_item_catalogue(ControlEntry entry) {
    auto s = fixture(0);
    s.world.ai.battle.actors.at({1}).object_slot = 9;
    s.items.emplace(9, ObjectCatalogRecord{});
    s.catalog.emplace(std::pair<int, int>{0, 9}, ObjectCatalogRecord{});
    s.world.human_spending[0] = 0;
    RescueFacility shop;
    shop.placement = {{33}, 33, FacilityShape::single, FacilityOrientation::first, {5, 5}};
    shop.category = 1;
    shop.price = 10;
    shop.definition_wait = 2;
    shop.upgrade_uses = {2, 10};
    s.world.facilities.emplace(33, shop);
    s.world.facility_uses.emplace(33, FacilityUseProgress{});
    const auto bound = bind_facility_map(s.world.map, {{shop.placement, 3}});
    check(bound.map.has_value(), "carried-item fixture binds exact shop arrival cell");
    s.world.map = *bound.map;
    s.facts.map = s.world.map;
    auto &path = s.world.actors.at({1});
    path.binding = ArrivalBinding{{5, 5}, {33}, 33};
    path.destination = Position{5, 5};
    path.path_pending = true;
    FacilityDeparture journey;
    journey.category = 1;
    journey.binding = *path.binding;
    journey.route.steps = {{5, 5}};
    path.journey = journey;
    auto input = decision();
    input.daily.path = WorldPathInput{};
    input.daily.path->actor = {1};
    input.daily.path->facts = s.facts;
    input.shop_arrival = ShopArrivalInput{};
    input.shop_arrival->actor = {1};
    const auto result = decide(entry, s, input);
    check(result.candidate && result.candidate->daily && result.candidate->daily->path &&
              result.candidate->daily->path->arrived,
          "real P arrival invokes shop delivery without a direct inventory fixture increment");
    const auto &next = result.candidate->state;
    check(next.items.at(9).inventory == 1 && next.catalog.at({0, 9}).inventory == 1 &&
              next.items.at(9).status == 1 && next.catalog.at({0, 9}).status == 1 &&
              next.items.at(9).newly_unlocked && next.catalog.at({0, 9}).newly_unlocked &&
              s.items.at(9).inventory == 0 && s.catalog.at({0, 9}).inventory == 0,
          "shop-delivered ordinary item reaches both route projections exactly once");
    s.catalog.erase({0, 9});
    check(!decide(entry, s, input).candidate && s.items.at(9).inventory == 0 &&
              s.world.ai.accounting.funds() == 0 &&
              s.world.ai.battle.actors.at({1}).object_slot == 9,
          "missing ordinary-item catalogue mirror rejects arrival without partial delivery or "
          "income");
}
void departure_event_boundary(ControlEntry entry) {
    for (bool seen : {false, true}) {
        auto s = fixture();
        s.world.ai.task_active = true;
        s.world.actors.at({1}).definition_task_flag = true;
        s.world.ai.growth.at(0).definition.legacy_u = 100;
        s.world.ai.battle.actors.at({1}).control.queue = {{8, 0}};
        s.random = WorldRandomStream::from_raw({0});
        if (seen)
            s.world.ai.battle.events.insert(116);
        int providers{}, events{};
        bool reject{};
        const WorldActorCommandProvider provider =
            [&](const WorldActorRoutesState &owner, CharacterId id,
                const LegacyActorControl &command) -> std::optional<WorldActorCommandInput> {
            ++providers;
            check(command == LegacyActorControl{8, 0},
                  "departure provider observes its original front exactly once");
            WorldActorCommandInput i;
            i.use_shared_random = true;
            i.departure = WorldDepartureControlInput{};
            auto &d = i.departure->departure;
            d.actor = id;
            d.task_center = {4, 5};
            d.catalogue.town = owner.facts.town;
            d.catalogue.definitions = {{0, 0, 0}};
            d.catalogue.cell_definition_ids.assign(owner.world.map.cells.size(), 0);
            d.catalogue.events = {{{4, 5}, 1}};
            d.exits = {{0, 0}};
            i.event = [&](const WorldActorRoutesState &current,
                          int event) -> std::optional<WorldActorRoutesState> {
                ++events;
                check(event == 116 && current.world.ai.battle.events.count(116) &&
                          (current.world.ai.battle.actors.at({1}).control.flags & 2048U),
                      "op8 dispatches newly published116 after the real boost state change");
                auto replacement = current;
                replacement.human_definition_state.at(0) = 7;
                if (reject)
                    return {};
                return replacement;
            };
            return i;
        };
        const auto result = control(entry, s, {1}, provider);
        check(result.candidate && providers == 1 && events == (seen ? 0 : 1) &&
                  result.candidate->consumed_events ==
                      (seen ? std::vector<int>{} : std::vector<int>{116}) &&
                  result.candidate->state.human_definition_state.at(0) == (seen ? 0 : 7) &&
                  result.candidate->state.random.draws() == 1 && s.random.draws() == 0 &&
                  !(s.world.ai.battle.actors.at({1}).control.flags & 2048U) &&
                  s.world.ai.battle.actors.at({1}).control.queue ==
                      std::vector<LegacyActorControl>{{8, 0}},
              "private/value op8 preserve once-only116, one provider call and full original input");
        if (!seen) {
            providers = events = 0;
            reject = true;
            const auto failed = control(entry, s, {1}, provider);
            check(!failed.candidate && providers == 1 && events == 1 && s.random.draws() == 0 &&
                      !s.world.ai.battle.events.count(116) && s.human_definition_state.at(0) == 0 &&
                      s.world.ai.battle.actors.at({1}).control.queue ==
                          std::vector<LegacyActorControl>{{8, 0}},
                  "late116 consumer rejection rolls back boost, callback replacement, random and "
                  "command");
        }
    }
}

void attack_owner_replacement(ControlEntry entry) {
    auto s = fixture(1);
    auto &human = s.world.ai.battle.actors.at({1});
    human.control.queue = {{14}};
    human.control.action = 1;
    human.control.action_counter = 5;
    human.attack_armed = true;
    human.combo_count = 1;
    human.encounter = 0;
    auto monster = human;
    monster.id = {2};
    monster.kind = ActorKind::monster;
    monster.definition = 7;
    monster.control.queue.clear();
    monster.control.action = 6;
    monster.hp = {0, 1, 1, 1, false, 0};
    monster.position.x += 40;
    monster.attack_position = monster.position;
    s.world.ai.battle.actors.emplace(monster.id, monster);
    s.world.ai.monster_order = {monster.id};
    s.world.ai.contexts.emplace(monster.id, s.world.ai.contexts.at({1}));
    s.world.actors.emplace(monster.id, RescueActorContext{});
    auto &definition = s.world.ai.battle.monsters[7];
    definition.flags = 4;
    definition.rank = 5;
    auto &growth = s.world.ai.monster_growth[7];
    growth.base_hp = 100;
    growth.base_attack = growth.base_defense = 1;
    s.world.ai.encounters.emplace(0,
                                  RewardEncounter{{0, {5, 5}, 0, 0, 0, 1, 1, 0}, {{2}}, true, {}});
    int providers{}, events{};
    bool reject{};
    const WorldActorCommandProvider provider =
        [&](const WorldActorRoutesState &, CharacterId id,
            const LegacyActorControl &) -> std::optional<WorldActorCommandInput> {
        ++providers;
        WorldActorCommandInput input;
        input.attack = WorldAttackInput{};
        input.attack->actor = id;
        input.attack->weapon = {0, 100, 1, 0, 0};
        input.attack->physical_jitter = 0;
        input.attack->drop_ticket = 99;
        input.event = [&](const WorldActorRoutesState &owner,
                          int event) -> std::optional<WorldActorRoutesState> {
            ++events;
            check(event == 217 && owner.world.ai.battle.actors.at({2}).control.state == 3,
                  "attack event sees the live post-hit candidate before replacing the outer owner");
            auto replacement = owner;
            replacement.world.ai.battle.events.insert(300);
            replacement.human_definition_state.at(0) = 9;
            if (reject)
                return {};
            return replacement;
        };
        return input;
    };
    const auto result = control(entry, s, {1}, provider);
    check(result.candidate && providers == 1 && events == 1 &&
              result.candidate->consumed_events == std::vector<int>{217} &&
              result.candidate->state.world.ai.battle.events.count(300) &&
              result.candidate->state.human_definition_state.at(0) == 9 &&
              result.candidate->state.world.ai.battle.defeated_definitions == std::vector<int>{7} &&
              s.world.ai.battle.actors.at({2}).hp.target == 1 &&
              !s.world.ai.battle.events.count(217) && s.human_definition_state.at(0) == 0,
          "attack keeps value-source semantics across event Owner replacement in both control "
          "entries");
    providers = events = 0;
    reject = true;
    const auto failed = control(entry, s, {1}, provider);
    check(!failed.candidate && providers == 1 && events == 1 &&
              s.world.ai.battle.actors.at({2}).hp.target == 1 &&
              s.world.ai.battle.defeated_definitions.empty() && s.random.draws() == 0,
          "attack event refusal publishes no partial HP, records, callback replacement or random");
}

void every_control_route(ControlEntry entry) {
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
        auto r = control(entry, s, {1}, provider);
        if (!r.candidate)
            throw std::runtime_error("opcode " + std::to_string(op) + " error " +
                                     std::to_string(static_cast<int>(r.error)) + " control " +
                                     std::to_string(static_cast<int>(r.control_error)));
        check(
            s.world.ai.battle.actors.at({1}).control.queue ==
                std::vector<LegacyActorControl>{commands[op]},
            "all34 decoded opcodes route to actual local/domain consumers without source mutation");
        check(r.candidate->state.world.map.cells.size() == 64 &&
                  r.candidate->state.facts.map.cells.size() == 64 &&
                  r.candidate->state.world.ai.battle.actors.count({1}) == 1 &&
                  r.candidate->state.shop_humans.count(0) == 1 &&
                  r.candidate->state.dungeon_actors.count({1}) == 1,
              "all34 control results retain complete world and unrelated route domains");
        r.candidate->state.world.map.cells.front().legacy_state = -1;
        r.candidate->state.world.ai.battle.actors.at({1}).state_counter = -1;
        check(s.world.map.cells.front().legacy_state == 4 &&
                  s.world.ai.battle.actors.at({1}).state_counter == 10,
              "control candidate map and actor storage remain independent of source owner");
    }
}
} // namespace
int main() {
    try {
        for (const auto entry : {ControlEntry::copied, ControlEntry::consuming}) {
            try {
                all_state_routing(entry);
                recursive_rescue_arrival(entry);
                decision_shared_random(entry);
                delivered_item_catalogue(entry);
                fifo_and_failures(entry);
                shared_random_sequence(entry);
                departure_event_boundary(entry);
                attack_owner_replacement(entry);
                every_control_route(entry);
            } catch (const std::exception &e) {
                throw std::runtime_error(std::string(entry == ControlEntry::copied
                                                         ? "const control: "
                                                         : "consuming control: ") +
                                         e.what());
            }
        }
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
