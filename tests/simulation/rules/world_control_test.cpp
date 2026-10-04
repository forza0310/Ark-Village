#include "ark/simulation/rules/combat_commit.hpp"
#include "ark/simulation/rules/world_control.hpp"
#include "ark/simulation/rules/world_facilities.hpp"
#include "ark/simulation/rules/world_shop.hpp"

#include <iostream>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
struct Owner {
    ShopWorldState shop;
    std::vector<int> consumed;
    std::vector<ShopWorldRequest> requests;
    bool departure_succeeds{true}; // 显式路径结果夹具，不冒充完整 o()。
    bool fail_departure_consumer{};
    std::vector<bool> departure_results;
    std::size_t departure_cursor{};
    std::vector<ShopEquipmentDefinition> catalogue;
};
Owner fixture(int category = 1) {
    Owner s;
    auto &w = s.shop.world;
    w.map = {4, 4, std::vector<LegacyMapCell>(16)};
    for (auto &cell : w.map.cells)
        cell.legacy_state = 4;
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.category = category;
    f.upgrade_uses = {2, 10};
    w.facilities.emplace(3, f);
    w.facility_uses.emplace(33, FacilityUseProgress{});
    w.map = *bind_facility_map(w.map, {{f.placement, 3}}).map;
    BattleActorRecord a;
    a.id = {1};
    a.capacity = 100;
    a.position = {150, 0, 150};
    a.hp = {0, 50, 50, 50, false, 0};
    a.control.state = 14;
    a.control.action = 1;
    a.control.action_counter = 12; // 已完成的真实攻击帧，不在命中窗口。
    a.control.alternate_counter = 9;
    a.state_counter = 40;
    w.ai.battle.actors.emplace(a.id, a);
    w.ai.human_order = {a.id};
    w.ai.contexts.emplace(a.id, RewardActorContext{{1, 1}, true, {}, {}});
    w.actors.emplace(a.id, RescueActorContext{});
    w.actors.at(a.id).binding = ArrivalBinding{{1, 1}, {3}, 33};
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    w.ai.professions.push_back({{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 1, true});
    g.derived = *derive_human_stats(g.definition, w.ai.professions).candidate;
    w.ai.growth.emplace(0, g);
    s.shop.humans.emplace(0, ShopHumanRecord{});
    s.shop.humans.at(0).equipment[0] = 0;
    s.shop.actors.emplace(a.id, ShopActorRecord{});
    s.catalogue = {{1, 0, 1, 0, true, 100, {0, 1, 0, 0}}, {1, 2, 2, 0, true, 200, {30, 10, 0, 0}}};
    return s;
}
WorldControlAdapter<Owner> adapter() {
    WorldControlAdapter<Owner> a;
    a.read = [](const Owner &s, CharacterId id) -> const ActorControlState * {
        const auto found = s.shop.world.ai.battle.actors.find(id);
        return found == s.shop.world.ai.battle.actors.end() ? nullptr : &found->second.control;
    };
    a.write = [](Owner &s, CharacterId id, const ActorControlState &control) {
        const auto found = s.shop.world.ai.battle.actors.find(id);
        if (found == s.shop.world.ai.battle.actors.end())
            return false;
        found->second.control = control;
        return true;
    };
    a.domain = [](const Owner &s, CharacterId id) -> std::optional<WorldControlStep<Owner>> {
        Owner next = s;
        auto &control = next.shop.world.ai.battle.actors.at(id).control;
        const int code = control.queue.front()[0];
        next.consumed.push_back(code);
        if (code == 8) {
            control.queue.erase(control.queue.begin());
            if (next.fail_departure_consumer)
                return std::nullopt;
            const bool succeeds = next.departure_cursor < next.departure_results.size()
                                      ? next.departure_results[next.departure_cursor++]
                                      : next.departure_succeeds;
            if (succeeds) {
                control.state = 0; // 真实成功8是直接赋值，不是 c0。
                return WorldControlStep<Owner>{std::move(next), WorldControlAction::hold_false};
            }
            const auto failure = prepare_failed_activity(control.flags);
            if (failure.expression18) {
                auto &effects = next.shop.world.ai.contexts.at(id).effects;
                const auto face = prepare_actor_expression({effects, 18, 0, 0, 3, 0});
                if (!face.candidate)
                    return std::nullopt;
                effects = face.candidate->state;
            }
            control.flags = failure.flags;
            if (failure.delete_instance)
                return WorldControlStep<Owner>{std::move(next), WorldControlAction::delete_true};
            const auto cleanup = prepare_world_rescue_cleanup(next.shop.world, id);
            if (!cleanup.candidate)
                return std::nullopt;
            next.shop.world = cleanup.candidate->state;
            return WorldControlStep<Owner>{std::move(next)};
        }
        if (code >= 14 && code <= 17) {
            WorldAttackInput input;
            input.actor = id;
            input.weapon = {0, 100, 1, 0, 0};
            const auto r = prepare_world_attack_control(next.shop.world.ai, input);
            if (!r.candidate) {
                std::cerr << "attack opcode " << code << " error " << static_cast<int>(r.error)
                          << '\n';
                return std::nullopt;
            }
            next.shop.world.ai = r.candidate->state;
            return WorldControlStep<Owner>{
                std::move(next), r.candidate->completed ? WorldControlAction::continue_same_call
                                                        : WorldControlAction::hold_false};
        }
        if (code == 19 || code == 27 || code == 28 || code == 29 || code == 30 ||
            (code == 24 && next.shop.world.facilities.at(3).category == 1)) {
            const auto r = code == 24
                               ? prepare_world_shop_exit(
                                     next.shop, {id, next.catalogue, {30, 50}, 30, 0, {{1, 5}}, 0})
                               : prepare_world_shop_command(next.shop, id, next.catalogue);
            if (!r.candidate) {
                std::cerr << "shop opcode " << code << " error " << static_cast<int>(r.error)
                          << '\n';
                return std::nullopt;
            }
            next.shop = r.candidate->state;
            next.requests.insert(next.requests.end(), r.candidate->requests.begin(),
                                 r.candidate->requests.end());
            return WorldControlStep<Owner>{std::move(next)};
        }
        const auto r = prepare_world_facility_control(next.shop.world, {id, {}, {}});
        if (!r.candidate)
            return std::nullopt;
        next.shop.world = r.candidate->state;
        const bool holds = r.candidate->flow == ActorControlFlow::waiting ||
                           r.candidate->flow == ActorControlFlow::moving;
        return WorldControlStep<Owner>{std::move(next),
                                       holds ? WorldControlAction::hold_false
                                             : WorldControlAction::continue_same_call};
    };
    return a;
}
void mixed_domains() {
    auto s = fixture();
    auto &c = s.shop.world.ai.battle.actors.at({1}).control;
    c.queue = {{21}, {19, 6, 0, 7}, {14}, {22, 30, 0}, {24}};
    const auto r = prepare_world_control(s, {1}, adapter());
    if (!r.candidate)
        throw std::runtime_error("mixed-domain preparation error " +
                                 std::to_string(static_cast<int>(r.error)));
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.consumed == std::vector<int>{21, 19, 14, 22, 24, 8},
          "same v delegates occupation, attribute, attack, height, shop exit and activity");
    const auto &next = r.candidate->state.shop;
    check(next.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{20, 1}, {19, 6, 1, 5}} &&
              next.world.ai.growth.at(0).definition.extra[0] == 7 &&
              next.world.ai.growth.at(0).definition.extra[1] == 0,
          "successful8 defers fresh shop tail; old discarded FIFO not restored");
    check(next.world.facilities.at(3).occupants.empty() &&
              next.world.facility_uses.at(33).completed_uses == 1 &&
              next.popularity_queue == std::vector<std::array<int, 3>>{{10, 1, 0}},
          "real shop exit releases occupation and commits shared use/popularity once");
    check(next.world.ai.contexts.at({1}).effects.display.size() == 1 &&
              next.world.ai.contexts.at({1}).effects.display.front()[1] == -6 &&
              next.world.ai.battle.actors.at({1}).hp.displayed == 50,
          "same-call attribute display is not advanced by a second common d prefix");
    check(s.shop.world.facilities.at(3).occupants.empty() &&
              s.shop.world.facility_uses.at(33).completed_uses == 0 &&
              s.shop.world.ai.growth.at(0).definition.extra[0] == 0,
          "successful candidate never mutates original owner");
    const auto tail = prepare_world_control(r.candidate->state, {1}, adapter());
    check(tail.candidate && tail.candidate->flow == WorldControlFlow::finished &&
              tail.candidate->state.shop.world.ai.growth.at(0).definition.extra[1] == 5,
          "deferred attribute consumes on next v without implicit growth/effect ticking");
}
void waits_and_exit() {
    auto s = fixture(2);
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{21}, {1, 2, 0}, {24}};
    auto r = prepare_world_control(s, {1}, adapter());
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.consumed == std::vector<int>{21} &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue.front()[1] == 1,
          "facility delegated segment waiting holds; outer loop never decrements wait twice");
    r = prepare_world_control(r.candidate->state, {1}, adapter());
    check(r.candidate && r.candidate->state.consumed == std::vector<int>{21, 24, 8} &&
              r.candidate->state.shop.world.facility_uses.at(33).completed_uses == 1 &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{18, 9, 0}},
          "wait reaches0 then exits and starts departure same v; exited is not hold");
    s = fixture();
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{14}, {19, 6, 0, 7}};
    s.shop.world.ai.battle.actors.at({1}).control.action_counter = 1;
    r = prepare_world_control(s, {1}, adapter());
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 0 &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue.size() == 2,
          "ongoing real attack holds its command and does not execute attribute tail");
}
void errors_and_true() {
    auto s = fixture();
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{6, 4}, {19, 6, 0, 7}, {32}};
    const auto r = prepare_world_control(s, {1}, adapter());
    check(r.error == WorldControlError::no_progress && !r.candidate &&
              s.shop.world.ai.battle.actors.at({1}).control.flags == 0 &&
              s.shop.world.ai.growth.at(0).definition.extra[0] == 0,
          "unhandled32 rolls back earlier local flag and real attribute segment");
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{6, 4}, {8, 0}};
    s.fail_departure_consumer = true;
    check(prepare_world_control(s, {1}, adapter()).error == WorldControlError::consumer_failed &&
              s.shop.world.ai.battle.actors.at({1}).control.flags == 0,
          "late domain failure exposes no private partially executed candidate");
    auto a = adapter();
    a.domain = {};
    check(prepare_world_control(s, {1}, a).error == WorldControlError::missing_consumer,
          "missing required domain is an error, not successful next-frame delegation");
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{6, 4}, {26}, {19, 6, 0, 7}};
    a = adapter();
    a.domain = [](const Owner &old, CharacterId id) -> std::optional<WorldControlStep<Owner>> {
        auto next = old;
        next.shop.world.ai.battle.actors.at(id).control.queue.erase(
            next.shop.world.ai.battle.actors.at(id).control.queue.begin());
        return WorldControlStep<Owner>{std::move(next), WorldControlAction::delete_true};
    };
    const auto removed = prepare_world_control(s, {1}, a);
    check(
        removed.candidate && removed.candidate->flow == WorldControlFlow::delete_requested &&
            removed.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue.front()[0] ==
                19 &&
            removed.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 0,
        "true stops before all remaining FIFO/tail; schedule owns actual release/removal");
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{19, 6, 0, 1}, {19, 6, 0, 2}};
    check(prepare_world_control(s, {1}, adapter(), 1).error ==
                  WorldControlError::budget_exhausted &&
              s.shop.world.ai.growth.at(0).definition.extra[0] == 0,
          "maintenance budget rolls back, never postpones a partially applied v");
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{6, 4}, {99}};
    check(prepare_world_control(s, {1}, adapter()).error == WorldControlError::malformed_control &&
              s.shop.world.ai.battle.actors.at({1}).control.flags == 0,
          "full FIFO preflight precedes local side effects");
    check(prepare_world_control(s, {9}, adapter()).error == WorldControlError::stale_actor,
          "stale actor rejects explicit adapter read");
    check(prepare_world_control(s, {1}, WorldControlAdapter<Owner>{}).error ==
              WorldControlError::invalid_adapter,
          "missing ownership accessors cannot create second control authority");
}
void failed_departures() {
    auto s = fixture(2);
    s.shop.world.facilities.at(3).occupants = {{1}, {1}};
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{8, 0}, {19, 6, 0, 7}};
    s.departure_results = {false, true};
    const auto r = prepare_world_control(s, {1}, adapter());
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.consumed == std::vector<int>{8, 8} &&
              r.candidate->state.departure_cursor == 2 &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.state == 0 &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue.empty(),
          "real failed8 cleanup replaces queue and new8 succeeds during same v");
    check(r.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 0 &&
              r.candidate->state.shop.world.facilities.at(3).occupants ==
                  std::vector<CharacterId>{{1}} &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.action_counter == 12,
          "real r releases first reference, drops old attributes, preserves k/l");
    s = fixture(2);
    auto &control = s.shop.world.ai.battle.actors.at({1}).control;
    control.flags = 512U | 32768U;
    control.queue = {{8, 0}, {19, 6, 0, 7}};
    s.departure_succeeds = false;
    const auto waiting = prepare_world_control(s, {1}, adapter());
    check(waiting.candidate && waiting.candidate->flow == WorldControlFlow::held &&
              waiting.candidate->state.consumed == std::vector<int>{8} &&
              waiting.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{1, 119, 0}, {8, 5}} &&
              (waiting.candidate->state.shop.world.ai.battle.actors.at({1}).control.flags & 1024U),
          "new1024 does not recheck32768; real cleanup wait120 first ticks once to119");
    control.flags = 1024U | 32768U;
    const auto removed = prepare_world_control(s, {1}, adapter());
    check(removed.candidate && removed.candidate->flow == WorldControlFlow::delete_requested &&
              removed.candidate->state.consumed == std::vector<int>{8} &&
              removed.candidate->state.shop.world.facilities.at(3).occupants.empty() &&
              removed.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{19, 6, 0, 7}},
          "old1024 and32768 requests true after face18, before cleanup or attributes");
}
void fresh_waits_and_motion() {
    auto s = fixture();
    s.shop.world.ai.battle.actors.at({1}).control.queue = {
        {1, 1, 0}, {19, 6, 0, 7}, {1, 2, 0}, {19, 6, 1, 5}};
    const auto r = prepare_world_control(s, {1}, adapter());
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 7 &&
              r.candidate->state.shop.world.ai.growth.at(0).definition.extra[1] == 0 &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{1, 1, 0}, {19, 6, 1, 5}},
          "first wait expires, attribute continues, distinct next wait ticks only once");
    s = fixture();
    s.shop.world.ai.battle.actors.at({1}).control.queue = {
        {0, 150, 150}, {19, 6, 0, 7}, {0, 200, 150}, {19, 6, 1, 5}};
    const auto motion = prepare_world_control(s, {1}, adapter());
    check(motion.candidate && motion.candidate->flow == WorldControlFlow::held &&
              motion.candidate->state.consumed == std::vector<int>{0, 19, 0} &&
              motion.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 7 &&
              motion.candidate->state.shop.world.ai.growth.at(0).definition.extra[1] == 0 &&
              motion.candidate->state.shop.world.ai.battle.actors.at({1}).position.x > 156.6F &&
              motion.candidate->state.shop.world.ai.battle.actors.at({1}).position.x < 156.8F &&
              motion.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue.front() ==
                  LegacyActorControl{0, 200, 150},
          "second motion gets fresh target query, cannot reuse first arrived result");
    s.shop.world.ai.battle.actors.at({1}).control.flags = 64U;
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{0, 200, 150}, {19, 6, 0, 7}};
    const auto blocked = prepare_world_control(s, {1}, adapter());
    check(blocked.candidate && blocked.candidate->flow == WorldControlFlow::held &&
              blocked.candidate->state.shop.world.ai.battle.actors.at({1}).position.x == 150 &&
              blocked.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 0,
          "64 motion hold leaves target and all remaining commands untouched");
}
void setters_and_equipment() {
    auto s = fixture();
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{2, 15}, {19, 6, 0, 7}};
    const auto changed = prepare_world_control(s, {1}, adapter());
    check(changed.candidate && changed.candidate->flow == WorldControlFlow::finished &&
              changed.candidate->state.shop.world.ai.battle.actors.at({1}).control.state == 15 &&
              changed.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue.empty() &&
              changed.candidate->state.shop.world.ai.battle.actors.at({1}).state_counter == 0 &&
              changed.candidate->state.shop.world.ai.battle.actors.at({1}).control.action_counter ==
                  12 &&
              changed.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 0,
          "real c15 replaces entire FIFO and resets B/i but preserves k/l, no saved tail replay");
    s = fixture();
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{27, 0, 2}, {1, 2, 0}, {28, 2}};
    const auto display = prepare_world_control(s, {1}, adapter());
    check(display.candidate && display.candidate->flow == WorldControlFlow::held &&
              display.candidate->state.requests.size() == 1 &&
              display.candidate->state.requests.front().kind ==
                  ShopWorldRequestKind::equipment_display &&
              display.candidate->state.shop.humans.at(0).equipment[0] == 0 &&
              display.candidate->state.shop.actors.at({1}).weapon == 0,
          "real27 emits display request then waits, no eager weapon change or cooldown");
    const auto equip = prepare_world_control(display.candidate->state, {1}, adapter());
    check(equip.candidate && equip.candidate->flow == WorldControlFlow::finished &&
              equip.candidate->state.requests.size() == 1 &&
              equip.candidate->state.shop.humans.at(0).equipment[0] == 2 &&
              equip.candidate->state.shop.humans.at(0).reselect[0] == 6 &&
              equip.candidate->state.shop.actors.at({1}).weapon == 2 &&
              equip.candidate->state.shop.world.ai.battle.actors.at({1}).hp.displayed == 50,
          "real28 commits after wait, no duplicate display or common HP progression");
    s.shop.world.ai.battle.actors.at({1}).control.queue = {{28, 2}, {19, 6, 0, 7}};
    const auto budget = prepare_world_control(s, {1}, adapter(), 1);
    check(budget.error == WorldControlError::budget_exhausted && !budget.candidate &&
              s.shop.humans.at(0).equipment[0] == 0 && s.shop.humans.at(0).reselect[0] == 0 &&
              s.shop.world.ai.growth.at(0).definition.extra[0] == 0 &&
              s.shop.world.ai.battle.actors.at({1}).control.queue.size() == 2,
          "late budget failure rolls back actual equipment, shared derived stats and FIFO");
}
void no_target_monster() {
    auto s = fixture();
    auto &world = s.shop.world.ai;
    world.human_order.clear();
    world.monster_order = {{1}};
    auto &actor = world.battle.actors.at({1});
    actor.kind = ActorKind::monster;
    actor.control.action = 3;
    actor.control.action_counter = 12;
    actor.control.queue = {{17}, {19, 6, 0, 7}};
    const auto r = prepare_world_control(s, {1}, adapter());
    check(r.candidate && r.candidate->flow == WorldControlFlow::held &&
              r.candidate->state.consumed == std::vector<int>{17} &&
              r.candidate->state.shop.world.ai.battle.actors.at({1}).control.queue.front()[0] ==
                  17 &&
              r.candidate->state.shop.world.ai.growth.at(0).definition.extra[0] == 0,
          "real monster17 with no target still holds while animation remains unfinished");
}
} // namespace
int main() {
    try {
        mixed_domains();
        waits_and_exit();
        errors_and_true();
        failed_departures();
        fresh_waits_and_motion();
        setters_and_equipment();
        no_target_monster();
        std::cout << "world_control checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
