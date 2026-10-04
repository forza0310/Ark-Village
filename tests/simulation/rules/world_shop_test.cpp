#include "ark/simulation/rules/world_random.hpp"
#include "ark/simulation/rules/world_shop.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool v, const char *message) {
    ++checks;
    if (!v)
        throw std::runtime_error(message);
}
std::vector<ShopEquipmentDefinition> catalogue() {
    return {{1, 0, 1, 0, true, 100, {0, 1, 0, 0}},
            {1, 2, 2, 0, true, 200, {30, 10, 0, 0}},
            {2, 0, 1, 2, true, 300, {0, 0, 3, 0}},
            {2, 1, 1, 1, true, 400, {0, 0, 4, 0}},
            {3, 0, 1, 0, true, 500, {0, 0, 0, 5}}};
}
ShopWorldState fixture(int detail = 0) {
    ShopWorldState s;
    s.world.map = {4, 4, std::vector<LegacyMapCell>(16)};
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.category = 1;
    f.detail = detail;
    f.price = 10;
    f.definition_wait = 2;
    f.upgrade_uses = {2, 10};
    s.world.facilities.emplace(3, f);
    s.world.facility_uses.emplace(33, FacilityUseProgress{});
    s.world.map = *bind_facility_map(s.world.map, {{f.placement, 3}}).map;
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    g.definition.equipment[0] = catalogue()[0].combat;
    s.world.ai.professions.push_back({{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 1, true});
    g.derived = *derive_human_stats(g.definition, s.world.ai.professions).candidate;
    s.world.ai.growth.emplace(0, g);
    s.world.human_spending.emplace(0, 0);
    ShopHumanRecord h;
    h.equipment = {0, 0, 1, 0};
    s.humans.emplace(0, h);
    for (const CharacterId id : {CharacterId{1}, CharacterId{2}}) {
        BattleActorRecord a;
        a.id = id;
        a.capacity = 100;
        a.position = {150, 0, 150};
        a.hp = {0, 50, 50, 50, false, 0};
        s.world.ai.battle.actors.emplace(id, a);
        s.world.ai.human_order.push_back(id);
        s.world.ai.contexts.emplace(id, RewardActorContext{{1, 1}, true, {}, {}});
        s.world.actors.emplace(id, RescueActorContext{});
        s.world.actors.at(id).binding = ArrivalBinding{{1, 1}, {3}, 33};
        s.actors.emplace(id, ShopActorRecord{});
    }
    return s;
}
ShopExitInput exit_input(int quality = 30, int ticket = 0) {
    return {{1}, catalogue(), {30, 50}, quality, ticket, {{0, 10}, {1, 5}}, 0};
}
void arrivals() {
    for (int detail : {0, 1, 4, 5})
        for (unsigned flags : {0U, 512U, 256U, 512U | 256U})
            for (int slot : {0, 1}) {
                auto s = fixture(detail);
                s.world.ai.battle.actors.at({1}).control.flags = flags;
                ShopArrivalInput i{{1}, catalogue(), slot, detail == 1 ? 1 : 0};
                const auto r = prepare_world_shop_arrival(s, i);
                const int price = detail == 0   ? 10
                                  : detail == 1 ? 200
                                  : detail == 4 ? (slot ? 400 : 300)
                                                : 500;
                check(r.candidate && r.candidate->consumed_armor_slot == (detail == 4) &&
                          r.candidate->consumed_selection == (detail != 0) &&
                          r.candidate->state.world.ai.accounting.funds() == (flags ? 0 : price),
                      "equipment selection/slot2 draw precedes free/512 payment guards");
                check(r.candidate->state.humans.at(0).equipment[0] == 0 &&
                          r.candidate->state.world.actors.at({1}).visits.legacy_visit_counts[0] ==
                              1 &&
                          r.candidate->state.world.human_spending.at(0) == (flags ? 0 : price) &&
                          r.candidate->state.world.facilities.at(3).sales == (flags ? 0 : price),
                      "arrival books shared spending/sales, no eager equipment commit");
                check(!prepare_world_shop_arrival(r.candidate->state, i).candidate,
                      "already-using14 cannot replay arrival transaction");
            }
    auto s = fixture(4);
    s.humans.at(0).reselect[1] = 6;
    const auto r = prepare_world_shop_arrival(s, {{1}, catalogue(), 0, {}});
    check(r.candidate && r.candidate->consumed_armor_slot && !r.candidate->consumed_selection,
          "armor cooldown keeps mandatory slot draw but no selection draw");
    check(prepare_world_shop_arrival(s, {{1}, catalogue(), {}, {}}).error ==
              ShopWorldError::missing_ticket,
          "cooldown cannot erase mandatory arrival draw2");
    for (unsigned flags : {0U, 512U, 256U})
        for (int price : {0, 10}) {
            s = fixture();
            s.world.facilities.at(3).price = price;
            s.world.ai.battle.actors.at({1}).control.flags = flags;
            s.world.ai.battle.actors.at({1}).object_slot = 9;
            s.items.emplace(9, ObjectCatalogRecord{});
            const auto delivery = prepare_world_shop_arrival(s, {{1}, {}, {}, {}});
            check(delivery.candidate && delivery.candidate->state.items.at(9).inventory == 1 &&
                      delivery.candidate->state.items.at(9).status == 1 &&
                      delivery.candidate->state.world.ai.battle.actors.at({1}).object_slot == -1 &&
                      delivery.candidate->requests.front().kind ==
                          ShopWorldRequestKind::delivered_item_notice &&
                      delivery.candidate->state.world.ai.accounting.funds() ==
                          (flags || !price ? 0 : price + 5000) &&
                      !delivery.candidate->state.world.ai.battle.events.count(151),
                  "carried item delivery precedes payment; bonus only payable, no rewardE/151");
        }
    s = fixture(1);
    s.world.ai.battle.actors.at({1}).object_slot = 9;
    s.items.emplace(9, ObjectCatalogRecord{});
    check(!prepare_world_shop_arrival(s, {{1}, catalogue(), {}, {}}).candidate &&
              s.items.at(9).inventory == 0 && s.world.ai.battle.actors.at({1}).object_slot == 9,
          "late missing equipment ticket rolls back preceding item delivery");
    s = fixture();
    s.world.human_spending.at(0) = std::numeric_limits<int>::max();
    check(!prepare_world_shop_arrival(s, {{1}, {}, {}, {}}).candidate &&
              s.world.facilities.at(3).sales == 0 && s.world.ai.accounting.funds() == 0,
          "late spending overflow rolls back cash/use/counters");
}
void ordinary_exits() {
    for (int satisfaction : {0, 50, 100})
        for (int quality = 0; quality <= 70; ++quality)
            for (int ticket = 0; ticket < 10; ++ticket) {
                auto s = fixture();
                s.humans.at(0).satisfaction = satisfaction;
                s.world.ai.battle.actors.at({1}).control.queue = {{24}};
                s.world.facilities.at(3).occupants = {{1}, {1}};
                s.popularity_queue = {{8, 3, 1}};
                const auto r = prepare_world_shop_exit(s, exit_input(quality, ticket));
                const bool gain = 30 + satisfaction * 20 / 100 + ticket - 5 <= quality;
                check(r.candidate &&
                          r.candidate->state.humans.at(0).satisfaction ==
                              std::min(100, satisfaction + (gain ? 1 : 0)) &&
                          r.candidate->state.popularity_queue.front() ==
                              std::array<int, 3>{10, gain ? 1 : 0, 0} &&
                          r.candidate->state.popularity_queue.back() == std::array<int, 3>{8, 3, 1},
                      "satisfaction now; popularity front insertion even zero or satisfaction100");
                check(r.candidate->state.world.facilities.at(3).occupants ==
                              std::vector<CharacterId>{{1}} &&
                          r.candidate->state.world.facility_uses.at(33).completed_uses == 1 &&
                          r.candidate->state.world.ai.growth.at(0).definition.extra[0] == 0 &&
                          r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                              std::vector<LegacyActorControl>{{8, 0}, {20, 1}, {19, 6, 0, 10}},
                      "first release/shared use commit; attributes remain AFTER activity8");
            }
    auto s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{19, 6, 0, 10}};
    const auto r = prepare_world_shop_command(s, {1}, {});
    check(r.candidate && r.candidate->state.world.ai.growth.at(0).definition.extra[0] == 10 &&
              r.candidate->state.world.ai.growth.at(0).derived.combat ==
                  s.world.ai.growth.at(0).derived.combat &&
              r.candidate->state.world.ai.contexts.at({1}).effects.display.front() ==
                  ActorEffectRecord{13, -6, 0, 10},
          "19 writes z/cd13, no eager derived stats/HP recalculation");
    s.world.ai.growth.at(0).definition.extra[0] = std::numeric_limits<int>::max();
    check(!prepare_world_shop_command(s, {1}, {}).candidate &&
              s.world.ai.contexts.at({1}).effects.display.empty(),
          "late attribute overflow rolls back command/display");
}
void delayed_equipment() {
    for (int detail : {1, 4, 5}) {
        auto s = fixture(detail);
        auto r = prepare_world_shop_arrival(s, {{1}, catalogue(), 0, detail == 1 ? 1 : 0});
        check(r.candidate.has_value(), "equipment fixture arrives");
        s = r.candidate->state;
        s.world.ai.battle.actors.at({1}).control.queue = {{24}};
        s.world.facilities.at(3).occupants = {{1}};
        r = prepare_world_shop_exit(s, exit_input());
        check(r.candidate && r.candidate->state.popularity_queue.empty() &&
                  r.candidate->state.humans.at(0).reselect == std::array<int, 4>{0, 0, 0, 0},
              "equipment exit neither ordinary satisfaction nor eager commit");
        s = r.candidate->state;
        const auto leave =
            prepare_local_control_prefix(s.world.ai.battle.actors.at({1}).control, {{}, true});
        check(leave.candidate && leave.candidate->flow == ActorControlFlow::departure_started &&
                  leave.candidate->state.queue.front() == LegacyActorControl{20, 1},
              "successful8 stops BEFORE remaining equipment animation controls");
        auto &queue = s.world.ai.battle.actors.at({1}).control.queue;
        auto find = [&](int x, int y) {
            return std::find_if(queue.begin(), queue.end(),
                                [&](const auto &c) { return c[0] == x || c[0] == y; });
        };
        queue.erase(queue.begin(), find(27, 29)); // Isolate the post-wait display consumer.
        r = prepare_world_shop_command(s, {1}, catalogue());
        check(r.candidate && r.candidate->requests.size() == 1 &&
                  r.candidate->state.humans.at(0).reselect == std::array<int, 4>{0, 0, 0, 0},
              "27/29 display request cannot equip or set cooldown");
        s = r.candidate->state;
        auto &tail = s.world.ai.battle.actors.at({1}).control.queue;
        tail.erase(tail.begin(), std::find_if(tail.begin(), tail.end(), [](const auto &c) {
                       return c[0] == 28 || c[0] == 30;
                   }));
        r = prepare_world_shop_command(s, {1}, catalogue());
        const int slot = detail == 1 ? 0 : detail == 4 ? 1 : 3;
        check(r.candidate && r.candidate->state.humans.at(0).reselect[slot] == 6 &&
                  r.candidate->state.actors.at({2}).weapon == 0 &&
                  r.candidate->state.world.ai.battle.actors.at({1}).hp.target == 50 &&
                  r.candidate->state.world.ai.battle.actors.at({2}).hp.target == 50,
              "actual commit6 counter/shared gear, no healing or other-instance ae propagation");
        if (detail == 1)
            check(r.candidate->state.actors.at({1}).weapon == 2 &&
                      r.candidate->state.world.ai.battle.actors.at({1}).capacity == 130 &&
                      r.candidate->state.world.ai.battle.actors.at({2}).capacity == 130,
                  "shared h() changes all capacities but ae only initiator");
    }
    auto s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{28, 2}};
    s.world.ai.growth.at(0).definition.profession_levels = {0};
    check(!prepare_world_shop_command(s, {1}, catalogue()).candidate &&
              s.humans.at(0).equipment[0] == 0 && s.actors.at({1}).weapon == 0,
          "late growth validation rejects without partial gear/cooldown/ae");
    s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{27, 0, 999}};
    check(!prepare_world_shop_command(s, {1}, catalogue()).candidate,
          "display validates actual equipment references, not only minimum command size");
    s = fixture(4);
    s.actors.at({1}).selected_armor = 1;
    s.world.ai.battle.actors.at({1}).control.queue = {{24}};
    const auto exit = prepare_world_shop_exit(s, exit_input());
    check(exit.candidate &&
              std::find(exit.candidate->state.world.ai.battle.actors.at({1}).control.queue.begin(),
                        exit.candidate->state.world.ai.battle.actors.at({1}).control.queue.end(),
                        LegacyActorControl{30, 2, 1}) !=
                  exit.candidate->state.world.ai.battle.actors.at({1}).control.queue.end(),
          "armor type1 commits slot2 independently of arrival draw slot");
    s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{24}};
    s.world.actors.at({1}).binding->definition_id = 99;
    const auto stale = prepare_world_shop_exit(s, exit_input());
    check(stale.candidate && stale.candidate->cleaned_up &&
              stale.candidate->state.world.facility_uses.at(33).completed_uses == 0 &&
              stale.candidate->state.popularity_queue.empty() &&
              stale.candidate->state.world.ai.battle.actors.at({1}).control.state == 19,
          "stale q invokes actual r, never credits shared use or satisfaction");
    s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{24}};
    s.world.facilities.at(3).status = 0;
    check(prepare_world_shop_exit(s, exit_input()).candidate.has_value(),
          "complete q binding may exit construction0; no extra status guard");
    s = fixture();
    s.humans.at(0).equipment[0] = 2;
    s.world.ai.growth.at(0).definition.equipment[0] = catalogue()[1].combat;
    s.world.ai.growth.at(0).derived =
        *derive_human_stats(s.world.ai.growth.at(0).definition, s.world.ai.professions).candidate;
    s.world.ai.battle.actors.at({1}).capacity = 130;
    s.world.ai.battle.actors.at({1}).hp = {0, 130, 130, 130, false, 0};
    s.world.ai.battle.actors.at({1}).control.queue = {{28, 0}};
    const auto downgrade = prepare_world_shop_command(s, {1}, catalogue());
    check(downgrade.candidate &&
              downgrade.candidate->state.world.ai.battle.actors.at({1}).capacity == 100 &&
              downgrade.candidate->state.world.ai.battle.actors.at({1}).hp.displayed == 130,
          "actual equip can decrease h() without clamping or healing current HP");
    TimedLifecycleInput life;
    life.state = 2;
    life.hp_capacity = downgrade.candidate->state.world.ai.battle.actors.at({1}).capacity;
    life.hp_slot1 = downgrade.candidate->state.world.ai.battle.actors.at({1}).hp.displayed;
    const auto recovery = prepare_timed_lifecycle(life);
    check(recovery.candidate && recovery.candidate->write_hp_slot1_and3 == 100,
          "subsequent down consumer accepts preserved HP then performs its own source cap");
}
void lazy_random() {
    for (const int detail : {0, 1, 4, 5}) {
        auto s = fixture(detail);
        auto random = WorldRandomStream::from_raw({-1, -7});
        std::vector<int> bounds;
        ShopArrivalInput input{{1}, catalogue(), {}, {}};
        input.draw = [&](int bound) -> std::optional<int> {
            bounds.push_back(bound);
            const auto r = random.draw(bound);
            return r.error == WorldRandomError::none ? std::optional<int>(r.ticket) : std::nullopt;
        };
        const auto r = prepare_world_shop_arrival(s, input);
        const std::vector<int> expected = detail == 0   ? std::vector<int>{}
                                          : detail == 1 ? std::vector<int>{2}
                                          : detail == 4 ? std::vector<int>{2, 1}
                                                        : std::vector<int>{1};
        check(r.candidate && bounds == expected && random.draws() == expected.size(),
              "arrival draws slot first and only actual eligible equipment count");
        if (detail == 4) {
            s.humans.at(0).reselect[2] = 5;
            bounds.clear();
            random = WorldRandomStream::from_raw({1});
            check(prepare_world_shop_arrival(s, input).candidate && bounds == std::vector<int>{2},
                  "armor cooldown consumes mandatory slot but no selection");
            s.humans.at(0).equipment[2].reset();
            bounds.clear();
            random = WorldRandomStream::from_raw({1, 0});
            check(prepare_world_shop_arrival(s, input).candidate &&
                      bounds == std::vector<int>({2, 1}),
                  "empty armor slot with positive cooldown still draws selection");
        }
        s.world.ai.battle.actors.at({1}).control.queue = {{24}};
        if (r.candidate)
            s.actors = r.candidate->state.actors;
        auto exit = exit_input();
        exit.effect_ticket.reset();
        bounds.clear();
        random = WorldRandomStream::from_raw({3, 1});
        exit.draw = input.draw;
        check(prepare_world_shop_exit(s, exit).candidate &&
                  bounds == (detail == 0 ? std::vector<int>({10, 2}) : std::vector<int>{}),
              "ordinary exit consumes satisfaction then effect; equipment exit draws neither");
    }
}
} // namespace
int main() {
    try {
        arrivals();
        ordinary_exits();
        delayed_equipment();
        lazy_random();
        std::cout << checks << " world shop checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
