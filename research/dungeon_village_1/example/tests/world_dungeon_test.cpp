#include "dungeon_village_reference/world_dungeon.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
DungeonWorldState fixture(int count = 1) {
    DungeonWorldState s;
    s.world.map = {5, 5, std::vector<LegacyMapCell>(25)};
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {2, 2}};
    f.category = 5;
    s.world.facilities.emplace(3, f);
    s.world.map = *bind_facility_map(s.world.map, {{f.placement, 3}}).map;
    DungeonFacilityProgress p;
    p.extent = 10000;
    s.facilities.emplace(3, p);
    RewardHumanDefinition g;
    g.definition.profession_levels = {1, 4, 10};
    s.world.ai.growth.emplace(0, g);
    for (int n = 1; n <= count; ++n) {
        const CharacterId id{static_cast<std::uint64_t>(n)};
        BattleActorRecord a;
        a.id = id;
        a.control.state = 14;
        a.control.queue = {{21}, {3, 0}, {1, 900, 0}};
        a.capacity = 100;
        a.position = {70, 30, 90};
        a.hp = {7, 80, 80, 90, true, 18};
        s.world.ai.battle.actors.emplace(id, a);
        s.world.ai.human_order.push_back(id);
        s.world.ai.contexts.emplace(id, RewardActorContext{{0, 0}, true, {}, {}});
        s.world.actors.emplace(id, RescueActorContext{});
        s.world.actors.at(id).binding = ArrivalBinding{{2, 2}, {3}, 33};
        s.actors.emplace(id, DungeonActorProgress{});
    }
    return s;
}
DungeonWorldCrewInput input() { return {3, {0, 1, 0, 1}, 10000, {}}; }
void occupation() {
    auto s = fixture();
    s.world.facilities.at(3).status = 0;
    s.world.actors.at({1}).binding->instance_id = {9}; // O cell still owns actual instance3.
    s.actors.at({1}) = {99, 98, 97, 96, 1, 95, true};
    auto r = prepare_world_dungeon_entry(s, {1});
    check(r.candidate && r.candidate->state.world.facilities.at(3).occupants.size() == 1 &&
              r.candidate->state.actors.at({1}).endurance == 8 &&
              r.candidate->state.actors.at({1}).constrained &&
              r.candidate->state.world.ai.contexts.at({1}).cell == Position{0, 0},
          "opcode21 reads current O instance despite old identity/s/status, preserves bw");
    s = r.candidate->state;
    s.world.ai.battle.actors.at({1}).control.queue.insert(
        s.world.ai.battle.actors.at({1}).control.queue.begin(), {21});
    r = prepare_world_dungeon_entry(s, {1});
    check(r.candidate && r.candidate->state.world.facilities.at(3).occupants ==
                             std::vector<CharacterId>{{1}, {1}},
          "occupation appends repeated reference, does not invent idempotence");
    for (int type : {0, 1})
        for (int count = 0; count <= 8; ++count) {
            s = fixture();
            s.facilities.at(3).challenges.assign(count, {30, type, 0, 0, 0, 1});
            if (count)
                s.facilities.at(3).challenges.back()[1] = 1 - type;
            r = prepare_world_dungeon_entry(s, {1}, 1);
            check(r.candidate &&
                      r.candidate->entry_event ==
                          (count >= 6 ? std::optional<int>{169 + type * 2} : std::nullopt),
                  "entry notice inspects all BUT final record and requires five-prefix count");
            if (count >= 6)
                check(prepare_world_dungeon_entry(s, {1}).error ==
                          DungeonWorldError::missing_ticket,
                      "only actual notice requires draw2");
        }
    s = fixture();
    s.world.ai.battle.actors.at({1}).control.queue = {{}};
    check(!prepare_world_dungeon_entry(s, {1}).candidate,
          "malformed front21 rejected before indexing or occupation");
}
void launches_and_binding() {
    for (int cell = 0; cell < 8; ++cell)
        for (int x : {0, 1, 79})
            for (int z : {0, 1, 79}) {
                auto s = fixture();
                s.world.facilities.at(3).occupants = {{1}, {1}};
                auto &a = s.world.ai.battle.actors.at({1});
                a.control.flags = 97U | 16U | 2048U;
                a.state_counter = 99;
                a.encounter = 7;
                const auto r = prepare_world_dungeon_retreat(s, {1}, {2, 2}, {0, 1, 0, 1},
                                                             DungeonLaunchTickets{cell, x, z});
                static constexpr Position offsets[]{{-1, 1}, {0, 1},   {1, 1},  {-1, 0},
                                                    {1, 0},  {-1, -1}, {0, -1}, {1, -1}};
                check(r.candidate && r.candidate->removed_occupation &&
                          r.candidate->consumed_launches == 1,
                      "eight-neighbor retreat consumes exact selection/offset tickets");
                const auto &next = r.candidate->state;
                const auto &actor = next.world.ai.battle.actors.at({1});
                const auto velocity = next.world.actors.at({1}).horizontal_velocity;
                const float tx = (2 + offsets[cell].x) * 100.0F + x + 10;
                const float tz = (2 + offsets[cell].y) * 100.0F + z + 10;
                check(actor.position.x == 250 && actor.position.z == 250 &&
                          actor.position.height == 30 && velocity.x == (tx - 250) / 36.0F &&
                          velocity.z == (tz - 250) / 36.0F &&
                          actor.vertical_velocity == 80.0F / 17.0F,
                      "teleport then 36-step horizontal velocity; source height remains unchanged");
                check(actor.control.state == 20 && actor.control.action == 7 &&
                          actor.control.flags == 2048U && actor.control.queue.empty() &&
                          actor.state_counter == 0 && !actor.encounter && actor.hp.target == 0 &&
                          actor.hp.displayed == 0 && actor.hp.animating &&
                          actor.hp.legacy_tick == 18 &&
                          next.world.facilities.at(3).occupants == std::vector<CharacterId>{{1}} &&
                          next.world.ai.contexts.at({1}).cell == Position{2, 2} &&
                          next.world.ai.contexts.at({1}).inside_town,
                      "q removes FIRST only; c20/d0/n7 preserve old HP animation/ax");
            }
    auto s = fixture();
    s.world.facilities.at(3).occupants = {{1}};
    auto r = prepare_world_dungeon_retreat(s, {1}, {2, 2}, {0, 4, 0, 4});
    check(r.candidate && r.candidate->consumed_launches == 0 &&
              r.candidate->state.world.ai.battle.actors.at({1}).position.x == 250 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 14 &&
              r.candidate->state.world.ai.battle.actors.at({1}).hp.target == 90 &&
              r.candidate->state.world.facilities.at(3).occupants.size() == 1,
          "no legal outside neighbor still commits center teleport, no HP/control/q/RNG effects");
    s.world.actors.at({1}).binding->goal = {1, 2};
    r = prepare_world_dungeon_retreat(s, {1}, {2, 2}, {0, 1, 0, 1}, DungeonLaunchTickets{0, 0, 0});
    check(r.candidate && !r.candidate->removed_occupation &&
              r.candidate->state.world.facilities.at(3).occupants.size() == 1,
          "q requires exact O cell, not merely same instance ID or teleport origin");
    s = fixture();
    check(prepare_world_dungeon_retreat(s, {1}, {2, 2}, {0, 1, 0, 1}).error ==
                  DungeonWorldError::missing_ticket &&
              s.world.ai.contexts.at({1}).cell == Position{0, 0},
          "missing required draw rolls back prelaunch teleport too");
}
void actual_crew_rewards() {
    auto s = fixture(2);
    s.world.facilities.at(3).occupants = {{1}, {2}};
    s.facilities.at(3).challenges = {{0, 0, 0, 0, 0, 9}, {0, 0, 0, 0, 1, 46}};
    s.catalog.emplace(std::pair<int, int>{0, 9}, ObjectCatalogRecord{});
    s.catalog.emplace(std::pair<int, int>{1, 46}, ObjectCatalogRecord{});
    s.shops.emplace(8, ObjectShopRecord{1, {}});
    s.shop_order = {8};
    auto r = prepare_world_dungeon_crew(s, input());
    check(r.candidate && r.candidate->requests.size() == 2 &&
              r.candidate->state.catalog.at({0, 9}).inventory == 1 &&
              r.candidate->state.catalog.at({1, 46}).free_purchases == 1 &&
              r.candidate->state.world.ai.battle.events.count(110) &&
              r.candidate->state.shops.at(8).notices.size() == 1 &&
              r.candidate->state.item_rewards == 1 &&
              r.candidate->state.world.ai.battle.objects.empty(),
          "treasure direct grant commits catalog/shop/events; does NOT create ground object");
    int consumed{};
    int item_grants{};
    const auto synchronized = prepare_world_dungeon_crew(
        s, input(),
        [&](const DungeonWorldState &current,
            const DungeonWorldRequest &request) -> std::optional<DungeonWorldState> {
            item_grants += request.source.first == 0 ? 1 : 0;
            check(request.source.kind == DungeonCrewRequestKind::grant_catalog_reward &&
                      current.item_rewards == consumed + item_grants,
                  "each crew grant synchronously sees prior typed consumer and current grant");
            auto next = current;
            ++next.item_rewards; // 明确外部回写夹具；不把该额外统计称原版奖励。
            ++consumed;
            return next;
        });
    check(synchronized.candidate && consumed == 2 &&
              synchronized.candidate->state.item_rewards == 3 && s.item_rewards == 0,
          "crew typed consumer writeback is visible to next grant without touching original owner");
    consumed = 0;
    const auto rejected = prepare_world_dungeon_crew(
        s, input(),
        [&](const DungeonWorldState &current,
            const DungeonWorldRequest &) -> std::optional<DungeonWorldState> {
            if (++consumed == 2)
                return {};
            return current;
        });
    check(!rejected.candidate && consumed == 2 && s.catalog.at({1, 46}).status == 0 &&
              s.shops.at(8).notices.empty(),
          "late real crew request rejection rolls back all earlier grants and shop events");
    s.catalog.erase({0, 9});
    check(!prepare_world_dungeon_crew(s, input()).candidate && s.catalog.at({1, 46}).status == 0 &&
              s.shops.at(8).notices.empty(),
          "late missing reward definition rolls back earlier weapon/shop/event unlock");
    s = fixture(2);
    s.world.facilities.at(3).occupants = {{1}, {2}};
    s.facilities.at(3).extent = 10;
    r = prepare_world_dungeon_crew(s, input());
    check(r.candidate && r.candidate->completed &&
              r.candidate->state.world.facilities.at(3).status == 2 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{1, 0, 0}, {8, 0}} &&
              r.candidate->state.world.ai.battle.actors.at({2}).control.queue ==
                  std::vector<LegacyActorControl>{{1, 5, 0}, {8, 0}} &&
              r.candidate->state.world.facilities.at(3).occupants.size() == 2,
          "completed expedition queues staggered direct-use exits without premature occupation "
          "clear");
    s = fixture();
    s.world.facilities.at(3).occupants = {{1}, {1}};
    s.actors.at({1}).retreat_state = 1;
    s.actors.at({1}).retreat_updates = 19;
    auto i = input();
    i.launches = {{0, 0, 0}, {1, 79, 79}};
    r = prepare_world_dungeon_crew(s, i);
    check(r.candidate && r.candidate->consumed_launches == 2 &&
              r.candidate->state.world.facilities.at(3).occupants.empty() &&
              r.candidate->state.actors.at({1}).retreat_updates == 21 &&
              r.candidate->state.world.ai.battle.actors.at({1}).control.state == 20,
          "duplicate crew retreat resolves two real q removals and two actual launches");
    s = fixture();
    s.world.facilities.at(3).occupants = {{1}};
    s.facilities.at(3).challenges = {{0, 0, 1, 9, 0, 9}};
    i = input();
    i.launches = {{7, 79, 0}};
    r = prepare_world_dungeon_crew(s, i);
    check(r.candidate && r.candidate->consumed_launches == 1 &&
              r.candidate->state.world.object_order == std::vector<std::uint64_t>{1} &&
              r.candidate->state.world.ai.battle.objects.at(1).state == 2 &&
              r.candidate->state.world.ai.battle.objects.at(1).cached_cell == Position{2, 2} &&
              r.candidate->state.world.ai.battle.objects.at(1).duration == 26 &&
              r.candidate->state.facilities.at(3).challenges.empty() &&
              r.candidate->state.catalog.empty(),
          "old status1 age9 ->10 creates26-step thrown object; grant awaits later pickup20");
    i.town = {0, 4, 0, 4};
    i.launches.clear();
    r = prepare_world_dungeon_crew(s, i);
    check(r.candidate && r.candidate->state.facilities.at(3).challenges.empty() &&
              r.candidate->state.world.ai.battle.objects.empty() &&
              r.candidate->state.world.ai.battle.next_object_id == 1,
          "no thrown destination still removes reward record with no RNG/ID/inventory grant");
}
void direct_landing() {
    for (int count = 0; count <= 40; ++count) {
        auto s = fixture();
        auto &a = s.world.ai.battle.actors.at({1});
        a.control.state = 20;
        a.state_counter = count;
        s.world.actors.at({1}).horizontal_velocity = {3, 4};
        int calls{};
        const auto r = prepare_world_dungeon_landing(
            s, {1}, Position{60, 9}, [&](const DungeonWorldState &candidate, CharacterId id) {
                ++calls;
                const auto &actor = candidate.world.ai.battle.actors.at(id);
                check(actor.control.state == 0 && actor.state_counter == 0 &&
                          actor.position.x == 73 && actor.position.z == 94 &&
                          actor.position.height == 0 && actor.control.queue.empty(),
                      "landing horizontal move/display/c0 precede DIRECT o0 callback");
                auto next = candidate;
                next.world.actors.at(id).path_pending =
                    true; // Test-only source no-route/success marker.
                next.world.ai.battle.actors.at(id).control.queue = {{3, 6}};
                return std::optional<DungeonWorldState>{next};
            });
        check(r.candidate && r.candidate->landed == (count >= 36) &&
                  calls == (count >= 36 ? 1 : 0) &&
                  r.candidate->state.world.ai.battle.actors.at({1}).position.x == 73,
              "oldB36 threshold follows horizontal move, no duplicate count increment");
        if (count >= 36)
            check(r.candidate->state.world.ai.battle.actors.at({1}).control.state == 2 &&
                      r.candidate->state.world.ai.battle.actors.at({1}).control.queue.empty() &&
                      r.candidate->state.world.actors.at({1}).path_pending &&
                      r.candidate->state.world.ai.contexts.at({1}).effects.display.back() ==
                          ActorEffectRecord{18, 0, 60, 9},
                  "c2 follows direct o0, clears its controls but retains route and old-u display");
    }
    auto s = fixture();
    s.world.ai.battle.actors.at({1}).control.state = 20;
    s.world.ai.battle.actors.at({1}).state_counter = 36;
    const auto r = prepare_world_dungeon_landing(
        s, {1}, Position{60, 9},
        [](const DungeonWorldState &, CharacterId) { return std::optional<DungeonWorldState>{}; });
    check(!r.candidate && s.world.ai.battle.actors.at({1}).position.height == 30 &&
              s.world.ai.contexts.at({1}).effects.display.empty(),
          "late direct-o0 preparation failure rolls back movement/display/c0 atomically");
}
void task_success_owner() {
    DungeonTaskSuccessState s;
    s.definitions.emplace(0, DungeonTaskDefinitionProgress{0, 0, 2, 0});
    for (int kind : {0, 1})
        for (int old = 240; old <= 520; ++old) {
            s.definitions.at(0).kind = kind;
            s.task_pool_progress = old;
            const auto r = prepare_dungeon_task_success(s, 0, 1, 4);
            std::vector<int> notices;
            const int amount = kind == 0 ? 50 : 100;
            for (int n = 0; n < 3; ++n)
                if (old < 300 + 100 * n && old + amount >= 300 + 100 * n)
                    notices.push_back(29 + n);
            check(r && r->state.successes == 1 &&
                      r->state.ordinary_explorations == (kind == 0 ? 1 : 0) &&
                      r->state.definitions.at(0).completed == 3 &&
                      r->state.task_pool_progress == old + amount &&
                      r->threshold_notice_ids == notices,
                  "task success exact G crossing, kind-specific counters and message IDs");
        }
    s.definitions.at(0) = {0, 2U | 4U | 8U, 2, 7};
    s.monsters.emplace(7, DungeonMonsterAvailability{0, false});
    s.remaining_task_definitions = {0};
    s.task_pool_progress = 290;
    for (int stage = 0; stage <= 5; ++stage) {
        s.exploration_stage = stage;
        const auto r = prepare_dungeon_task_success(s, 0, 2, 11);
        check(r && r->state.exploration_dates[stage] == std::array<int, 2>{2, 11} &&
                  r->state.exploration_stage == std::min(stage + 1, 5) &&
                  r->state.ordinary_explorations == 0 && r->state.monsters.at(7).status == 1 &&
                  r->state.monsters.at(7).pending_notice && r->state.task_pool_progress == 290 &&
                  r->threshold_notice_ids.empty(),
              "current special task blocks G before clear, stage5 still rewrites date");
    }
    s.remaining_task_definitions.clear();
    s.monsters.at(7) = {2, false};
    auto r = prepare_dungeon_task_success(s, 0, 1, 0);
    check(r && r->state.monsters.at(7).status == 1 && !r->state.monsters.at(7).pending_notice &&
              r->state.task_pool_progress == 340 && r->threshold_notice_ids == std::vector<int>{29},
          "nonzero monster status overwritten without new notice, no implicit special guard");
    r = prepare_dungeon_task_success(r->state, 0, 1, 0);
    check(r && r->state.successes == 2 && r->state.definitions.at(0).completed == 4,
          "task-success helper preserves repeated calls instead of inventing deduplication");
    s.remaining_task_definitions = {99};
    check(!prepare_dungeon_task_success(s, 0, 1, 0) && s.successes == 0 &&
              s.monsters.at(7).status == 2,
          "late missing task definition rolls back counters/stage/monster changes");
    s.remaining_task_definitions.clear();
    s.monsters.clear();
    check(!prepare_dungeon_task_success(s, 0, 1, 0) && s.exploration_dates[5][0] == 0,
          "flag4 missing actual monster is rejected without fabricated definition");
    s.definitions.at(0).flags = 0;
    s.task_pool_progress = std::numeric_limits<int>::max();
    check(!prepare_dungeon_task_success(s, 0, 1, 0) && s.successes == 0,
          "G overflow does not leave partial success counters");
    s.remaining_task_definitions = {0};
    s.definitions.at(0).flags = 2;
    check(prepare_dungeon_task_success(s, 0, 1, 0).has_value(),
          "suppressed G does not reject unrelated pool overflow");
    check(!prepare_dungeon_task_success(s, 99, 1, 0) && !prepare_dungeon_task_success(s, 0, 1, 12),
          "unknown task and non-source month rejected");
    s.successes = std::numeric_limits<int>::max();
    check(!prepare_dungeon_task_success(s, 0, 1, 0), "success counter overflow rejected");
    s.successes = 0;
    s.definitions.at(0).completed = std::numeric_limits<int>::max();
    check(!prepare_dungeon_task_success(s, 0, 1, 0), "definition completion overflow rejected");
}
} // namespace
int main() {
    try {
        occupation();
        launches_and_binding();
        actual_crew_rewards();
        direct_landing();
        task_success_owner();
        std::cout << checks << " world dungeon checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
