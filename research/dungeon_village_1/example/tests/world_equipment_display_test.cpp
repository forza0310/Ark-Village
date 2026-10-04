#include "dungeon_village_reference/world_equipment_display.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
std::vector<ShopEquipmentDefinition> catalogue() {
    return {{1, 0, 1, 0, true, 100, {0, 1, 0, 0}},
            {1, 2, 2, 0, true, 200, {30, 10, 0, 0}},
            {2, 0, 1, 2, true, 300, {0, 0, 3, 0}},
            {3, 0, 1, 0, true, 500, {0, 0, 0, 5}}};
}
ShopWorldState fixture(int opcode = 27, int display = 21) {
    ShopWorldState s;
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    g.definition.equipment[0] = catalogue()[0].combat;
    s.world.ai.professions.push_back({{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 1, true});
    g.derived = *derive_human_stats(g.definition, s.world.ai.professions).candidate;
    s.world.ai.growth.emplace(0, g);
    s.humans.emplace(0, ShopHumanRecord{});
    s.humans.at(0).equipment = {0, 0, 0, 0};
    for (const CharacterId id : {CharacterId{1}, CharacterId{2}}) {
        BattleActorRecord a;
        a.id = id;
        a.control.state = 0;
        a.control.action = 9;
        a.control.action_counter = 10;
        a.control.alternate_counter = 11;
        a.state_counter = 13;
        a.capacity = 100;
        a.hp = {0, 50, 50, 50, false, 0};
        a.position = {150, 30, 170};
        s.world.ai.battle.actors.emplace(id, a);
        s.world.ai.human_order.push_back(id);
        s.world.ai.contexts.emplace(id, RewardActorContext{{1, 1}, true, {}, {}});
        s.world.actors.emplace(id, RescueActorContext{});
        s.actors.emplace(id, ShopActorRecord{});
    }
    auto &queue = s.world.ai.battle.actors.at({1}).control.queue;
    queue = opcode == 27 ? std::vector<LegacyActorControl>{{27, 0, 2}, {1, 42, 0}, {28, 2}}
                         : std::vector<LegacyActorControl>{
                               {29, display, 0}, {1, 32, 0}, {30, display == 21 ? 2 : 3, 0}};
    return s;
}
ShopWorldRequest request(int opcode = 27, int display = 21) {
    return {ShopWorldRequestKind::equipment_display, opcode, 0,
            opcode == 27 ? LegacyActorControl{27, 0, 2} : LegacyActorControl{29, display, 0}};
}
void exact_payloads() {
    for (int opcode : {27, 29})
        for (int display : {21, 22}) {
            auto s = fixture(opcode, display);
            const auto emitted = prepare_world_shop_command(s, {1}, catalogue());
            check(emitted.candidate && emitted.candidate->requests.size() == 1,
                  "actual shop command emits display request after verified catalogue lookup");
            const auto &already = emitted.candidate->state;
            WorldEquipmentDisplayInput i{
                {1}, emitted.candidate->requests.front(), Position{-15, 303}};
            if (opcode == 29)
                i.cached_view.reset();
            const auto r = prepare_world_equipment_display(already.world, i);
            check(r.candidate.has_value(), "verified emitted request installs canonical display");
            const std::vector<ActorEffectRecord> expected =
                opcode == 27 ? std::vector<ActorEffectRecord>{{15, 0, -15, 303, 0, 2, 571, -71},
                                                              {7, -8, 0, 0}}
                             : std::vector<ActorEffectRecord>{{display, 0, 218, -18, 0}};
            const auto &c = *r.candidate;
            check(c.appended == expected &&
                      c.state.ai.contexts.at({1}).effects.display == expected &&
                      c.state.ai.contexts.at({1}).effects.delayed.empty(),
                  "27/29 exact ages, old u and truncated motion payload; smoke7 belongs to cd not "
                  "ce");
            const auto &actor = c.state.ai.battle.actors.at({1});
            check(actor.control.queue == already.world.ai.battle.actors.at({1}).control.queue &&
                      actor.control.queue.front()[0] == 1 && actor.state_counter == 13 &&
                      actor.control.action_counter == 10 && actor.control.alternate_counter == 11 &&
                      actor.position.x == 150 && actor.position.height == 30 &&
                      actor.position.z == 170 && actor.hp.displayed == 50 && actor.capacity == 100,
                  "display application does not erase NEXT wait or advance B/action/HP/physics");
            check(c.state.ai.contexts.at({2}).effects.display.empty() &&
                      already.world.ai.contexts.at({1}).effects.display.empty() &&
                      c.state.ai.growth.at(0).derived.combat ==
                          already.world.ai.growth.at(0).derived.combat,
                  "only initiating actor receives cd; definition capacity/equipment unchanged");
            auto installed = already;
            installed.world = c.state;
            installed.world.ai.battle.actors.at({1}).control.queue.erase(
                installed.world.ai.battle.actors.at({1})
                    .control.queue.begin()); // Separate wait fixture.
            const auto equip = prepare_world_shop_command(installed, {1}, catalogue());
            check(equip.candidate &&
                      equip.candidate->state.humans.at(0).reselect[opcode == 27    ? 0
                                                                   : display == 21 ? 2
                                                                                   : 3] == 6 &&
                      equip.candidate->state.world.ai.contexts.at({1}).effects.display ==
                          expected &&
                      equip.candidate->state.world.ai.battle.actors.at({1}).hp.displayed == 50,
                  "later28/30 alone commit equipment; do not replay display, heal or tick its age");
        }
}
void timeline_and_order() {
    auto s = fixture();
    auto &effects = s.world.ai.contexts.at({1}).effects;
    effects.display = {{0, 31}};
    effects.delayed = {{4, 3, 77, 88}};
    const auto r = prepare_world_equipment_display(s.world, {{1}, request(), Position{8, 9}});
    check(r.candidate &&
              r.candidate->state.ai.contexts.at({1}).effects.display ==
                  std::vector<ActorEffectRecord>{
                      {0, 31}, {15, 0, 8, 9, 0, 2, 571, -71}, {7, -8, 0, 0}} &&
              r.candidate->state.ai.contexts.at({1}).effects.delayed == effects.delayed,
          "append order retains old display and untouched ce, not front insertion or c7 expansion");
    const auto next = advance_actor_effects(r.candidate->state.ai.contexts.at({1}).effects);
    check(next.candidate &&
              next.candidate->state.display ==
                  std::vector<ActorEffectRecord>{{15, 0, 8, 9, 0, 2, 571, -71}, {7, -7, 0, 0}} &&
              next.candidate->state.delayed == std::vector<ActorEffectRecord>{{4, 2, 77, 88}},
          "existing source forward deletion skips shifted weapon display while smoke advances next "
          "tick");
    effects = {};
    const auto fresh = prepare_world_equipment_display(s.world, {{1}, request(), Position{8, 9}});
    check(fresh.candidate &&
              fresh.candidate->state.ai.contexts.at({1}).effects.display[0][1] == 0 &&
              fresh.candidate->state.ai.contexts.at({1}).effects.display[1][1] == -8,
          "inserted AFTER common display pass stays age0/negative8 for the rest of this d");
    auto timeline = fresh.candidate->state.ai.contexts.at({1}).effects;
    for (int tick = 1; tick <= 40; ++tick) {
        const auto step = advance_actor_effects(timeline);
        check(step.candidate && step.candidate->sounds.empty(),
              "no deferred spell or sound invented");
        timeline = step.candidate->state;
        if (tick == 8)
            check(timeline.display.size() == 2 && timeline.display[1][1] == 0,
                  "negative smoke age reaches zero on eighth SUBSEQUENT common update");
        if (tick == 32)
            check(timeline.display.size() == 1 && timeline.display[0][0] == 15,
                  "smoke cd7 negative8 lasts until subsequent tick32, duration24 unchanged");
    }
    check(timeline.display.empty() && timeline.delayed.empty(),
          "weapon display duration40 ends on its own later common update");
}
void refusal() {
    auto s = fixture();
    WorldEquipmentDisplayInput i{{1}, request(), {}};
    const auto missing = prepare_world_equipment_display(s.world, i);
    check(missing.error == WorldEquipmentDisplayError::missing_cached_view && !missing.candidate &&
              s.world.ai.contexts.at({1}).effects.display.empty(),
          "27 requires real old u, not a default0 or replacement world/logical projection");
    i.cached_view = Position{0, 0};
    for (const ShopWorldRequest &bad :
         {ShopWorldRequest{ShopWorldRequestKind::cash_display, 27, 0, {27, 0, 2}},
          ShopWorldRequest{ShopWorldRequestKind::equipment_display, 29, 0, {27, 0, 2}},
          ShopWorldRequest{ShopWorldRequestKind::equipment_display, 27, 1, {27, 0, 2}},
          ShopWorldRequest{ShopWorldRequestKind::equipment_display, 27, 0, {27, 0}},
          ShopWorldRequest{ShopWorldRequestKind::equipment_display, 27, 0, {27, -1, 2}},
          ShopWorldRequest{ShopWorldRequestKind::equipment_display, 29, 0, {29, 23, 0}},
          ShopWorldRequest{ShopWorldRequestKind::equipment_display, 28, 0, {28, 2}}}) {
        i.request = bad;
        const auto r = prepare_world_equipment_display(s.world, i);
        check(
            r.error == WorldEquipmentDisplayError::invalid_input && !r.candidate,
            "invalid kind/header/shape/ID or real equipment commit cannot be mistaken for display");
    }
    i.request = request();
    for (const ActorEffectRecord &bad :
         {ActorEffectRecord{27, 0}, ActorEffectRecord{7, std::numeric_limits<int>::max()},
          ActorEffectRecord{12, 0}, ActorEffectRecord{16, 0, 7}}) {
        s.world.ai.contexts.at({1}).effects.display = {bad};
        check(!valid_actor_effect_state(s.world.ai.contexts.at({1}).effects) &&
                  !prepare_world_equipment_display(s.world, i).candidate,
              "shared validator rejects invalid existing display without ticking or repairing it");
    }
    s.world.ai.contexts.at({1}).effects = {{}, {{7, 0, 2, 3}}};
    check(!prepare_world_equipment_display(s.world, i).candidate,
          "invalid existing ce effect7 cannot be silently discarded by cd adapter");
    s.world.ai.contexts.at({1}).effects = {{{7, -8, 0, 0}}, {}};
    check(
        valid_actor_effect_state(s.world.ai.contexts.at({1}).effects),
        "valid source negative age remains accepted, strict validation does not weaken smoke rule");
    s.world.ai.contexts.erase({1});
    check(prepare_world_equipment_display(s.world, i).error ==
              WorldEquipmentDisplayError::stale_actor,
          "missing canonical effect owner is explicit failure");
    s = fixture();
    s.world.ai.human_order.push_back({1});
    check(!prepare_world_equipment_display(s.world, i).candidate,
          "duplicate current human roster membership rejected");
}
} // namespace
int main() {
    try {
        exact_payloads();
        timeline_and_order();
        refusal();
        std::cout << checks << " world equipment display checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
