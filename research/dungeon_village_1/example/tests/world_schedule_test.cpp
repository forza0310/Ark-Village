#include "dungeon_village_reference/object_commit.hpp"
#include "dungeon_village_reference/world_control.hpp"
#include "dungeon_village_reference/world_departure.hpp"
#include "dungeon_village_reference/world_encounters.hpp"
#include "dungeon_village_reference/world_misc_control.hpp"
#include "dungeon_village_reference/world_schedule.hpp"

#include <algorithm>
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
WorldScheduleState fixture() {
    WorldScheduleState s;
    s.world.map = {6, 6, std::vector<LegacyMapCell>(36)};
    for (auto &cell : s.world.map.cells)
        cell.legacy_state = 4;
    s.surface = std::vector<int>(36, 1);
    s.map_flags = std::vector<std::uint32_t>(36);
    s.town = {0, 5, 0, 5};
    RescueFacility f;
    f.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {1, 1}};
    f.category = 2;
    f.upgrade_uses = {2, 10};
    s.world.facilities.emplace(3, f);
    s.facility_order = {3};
    s.world.map = *bind_facility_map(s.world.map, {{f.placement, 3}}).map;
    s.world.facility_uses.emplace(33, FacilityUseProgress{});
    RewardHumanDefinition g;
    g.definition.base = {100, 20, 20, 20, 20, 20};
    g.definition.profession_levels = {1};
    s.world.ai.professions.push_back({{9, 9, 9, 9, 9, 9}, {100, 100, 100, 100, 100, 100}, 5, true});
    g.derived = *derive_human_stats(g.definition, s.world.ai.professions).candidate;
    s.world.ai.growth.emplace(0, g);
    s.world.ai.battle.humans.emplace(0, HumanBattleRecord{});
    return s;
}
void add_actor(WorldScheduleState &s, std::uint64_t key, ActorKind kind = ActorKind::human,
               int state = 14) {
    BattleActorRecord a;
    a.id = {key};
    a.kind = kind;
    a.control.state = state;
    a.control.flags = 2;
    a.capacity = 100;
    a.hp = {0, 50, 50, 50, false, 0};
    a.position = {150, 0, 150};
    a.decision_start = a.position;
    a.physics_pause = 1;
    s.world.ai.battle.actors.emplace(a.id, a);
    (kind == ActorKind::human ? s.world.ai.human_order : s.world.ai.monster_order).push_back(a.id);
    s.world.ai.contexts.emplace(a.id, RewardActorContext{{1, 1}, true, {}, {}, true, {3, 3}});
    s.world.actors.emplace(a.id, RescueActorContext{});
    if (kind == ActorKind::human) {
        s.world.actors.at(a.id).binding = ArrivalBinding{{1, 1}, {3}, 33};
        s.world.actors.at(a.id).journey = FacilityDeparture{};
        s.world.actors.at(a.id).journey->route.steps = {{1, 1}, {2, 1}};
    }
}
std::optional<WorldScheduleStep> consumer(const WorldScheduleState &s,
                                          const WorldScheduleCall &call,
                                          const CombatInfluenceCandidate &) {
    WorldScheduleStep step{s};
    if (call.stage == WorldScheduleStage::decision) {
        const CharacterId id{*call.id};
        const auto &actor = s.world.ai.battle.actors.at(id);
        if (actor.kind == ActorKind::human && actor.control.state == 14) {
            const auto inn = prepare_world_inn_c(s.world, id);
            if (!inn.candidate)
                return {};
            step.state.world = inn.candidate->state;
        }
    } else if (call.stage == WorldScheduleStage::control) {
        WorldControlAdapter<WorldScheduleState> adapter;
        adapter.read = [](const WorldScheduleState &w,
                          CharacterId id) -> const ActorControlState * {
            const auto a = w.world.ai.battle.actors.find(id);
            return a == w.world.ai.battle.actors.end() ? nullptr : &a->second.control;
        };
        adapter.write = [](WorldScheduleState &w, CharacterId id, const ActorControlState &c) {
            w.world.ai.battle.actors.at(id).control = c;
            return true;
        };
        adapter.domain = [](const WorldScheduleState &w,
                            CharacterId id) -> std::optional<WorldControlStep<WorldScheduleState>> {
            const auto r = prepare_world_facility_control(w.world, {id, {}, {}});
            if (!r.candidate || r.candidate->flow == ActorControlFlow::delegated)
                return {};
            auto next = w;
            next.world = r.candidate->state;
            return WorldControlStep<WorldScheduleState>{
                next, r.candidate->flow == ActorControlFlow::waiting ||
                              r.candidate->flow == ActorControlFlow::moving ||
                              r.candidate->flow == ActorControlFlow::departure_started
                          ? WorldControlAction::hold_false
                          : WorldControlAction::continue_same_call};
        };
        const auto r = prepare_world_control(s, {*call.id}, adapter);
        if (!r.candidate)
            return {};
        step.state = r.candidate->state;
        if (r.candidate->flow == WorldControlFlow::delete_requested)
            step.disposition = WorldScheduleDisposition::remove_requested;
    } else if (call.stage == WorldScheduleStage::finalize) {
        const auto overlap = prepare_world_schedule_overlap(s, std::vector<int>(100, 0));
        if (!overlap)
            return {};
        step.state = *overlap;
    }
    return step;
}
void prelude_and_round() {
    auto s = fixture();
    add_actor(s, 1);
    add_actor(s, 2);
    s.world.ai.battle.actors.at({1}).control.queue = {{21}, {1, 3, 0}};
    s.world.ai.battle.actors.at({2}).control.queue = {{21}, {1, 3, 0}};
    s.world.ai.growth.at(0).pending = {9, -3};
    s.hints = {{99, 19, 101}, {100, 0}};
    s.floating_notes = {{7, 99}, {0, 101}, {7, 102}};
    s.popularity_queue = {{10, 12, 1}, {1, 5, 0}, {0, 7, 2}};
    s.updates = std::numeric_limits<int>::max() - 1;
    std::vector<std::array<int, 2>> credits;
    auto r = prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
        if (call.popularity)
            credits.push_back(*call.popularity); // 只供测试观察；运行副作用须存候选所有者。
        return consumer(w, call, field);
    });
    check(r.candidate.has_value(), "full common round succeeds on current one-owner fixture");
    const auto &w = r.candidate->state;
    check(w.updates == 0 && w.hints == std::vector<std::vector<int>>{{100, 0}} &&
              w.floating_notes == std::vector<std::vector<int>>{{1, 101}},
          "head hint20, reverse notes8, p modulo run once before actors");
    check(w.popularity_queue == std::vector<std::array<int, 3>>{{9, 12, 1}} &&
              credits == std::vector<std::array<int, 2>>{{7, 0}, {5, 0}} && w.rescue_available,
          "I decrements before <=0 credit in reverse, flag exactly1, S status exactly1");
    check(w.world.facilities.at(3).occupants == std::vector<CharacterId>{{2}, {1}} &&
              w.world.ai.battle.actors.at({1}).control.queue.front()[1] == 2 &&
              w.world.ai.battle.actors.at({2}).control.queue.front()[1] == 2 &&
              w.world.ai.battle.actors.at({1}).state_counter == 1 &&
              w.world.actors.at({1}).town_updates == 1 &&
              w.world.ai.growth.at(0).pending.counter == -1,
          "reverse occupation, one d-prefix and tail per instance, shared growth per instance");
    check(r.candidate->visits.size() == 6 &&
              r.candidate->visits[0].phase == AiSchedulePhase::human_decision &&
              r.candidate->visits[0].id == 2 && r.candidate->visits[1].id == 1 &&
              r.candidate->visits[2].phase == AiSchedulePhase::human_execution &&
              r.candidate->visits[2].id == 2 && r.candidate->visits[3].id == 1 &&
              r.candidate->visits.back().phase == AiSchedulePhase::finalize,
          "actual common round preserves separate c/d and one final L");
    check(s.updates == std::numeric_limits<int>::max() - 1 &&
              s.world.facilities.at(3).occupants.empty() && s.popularity_queue.size() == 3,
          "input remains unchanged across every prepared owner segment");
    for (int status : {0, 1, 2, 3}) {
        auto empty = fixture();
        empty.world.facilities.at(3).status = status;
        const auto result = prepare_world_schedule(empty, {}, consumer);
        check(result.candidate && result.candidate->state.rescue_available == (status == 1),
              "rescue does not treat unfinished/nonzero as available");
    }
}
void arrival_field_visibility() {
    const auto s = fixture();
    const auto r =
        prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
            auto next = consumer(w, call, field);
            if (next && call.stage == WorldScheduleStage::arrival_front)
                add_actor(next->state, 1, ActorKind::human, 1);
            return next;
        });
    check(r.candidate && r.candidate->state.world.ai.battle.actors.at({1}).state_counter == 1 &&
              r.candidate->visits[0].phase == AiSchedulePhase::human_decision &&
              r.candidate->visits[1].phase == AiSchedulePhase::human_execution,
          "arrival after influence gets both c and d in current live world round");
    check(std::all_of(r.candidate->start_field->monster_field.begin(),
                      r.candidate->start_field->monster_field.end(), [](int x) { return x == 0; }),
          "new arrival is absent from already captured round-start influence");
    const auto later = prepare_world_influence(r.candidate->state.world.ai,
                                               world_schedule_facts(r.candidate->state));
    check(later.candidate &&
              std::any_of(later.candidate->monster_field.begin(),
                          later.candidate->monster_field.end(), [](int x) { return x != 0; }),
          "same actor is eligible in next influence capture, no empty-field fixture shortcut");
}
void live_rosters_and_removal() {
    auto s = fixture();
    add_actor(s, 1);
    add_actor(s, 2, ActorKind::monster, 17);
    add_actor(s, 3, ActorKind::monster, 17);
    add_actor(s, 4, ActorKind::monster, 17);
    auto r = prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
        auto next = consumer(w, call, field);
        if (next && call.stage == WorldScheduleStage::decision && call.id == 1)
            add_actor(next->state, 5);
        return next;
    });
    check(r.candidate && r.candidate->state.world.ai.monster_order == std::vector<CharacterId>{{3}},
          "actual unbound-monster tail removes2/4, forward deletion skips3");
    check(r.candidate->state.world.ai.battle.actors.at({3}).state_counter == 0 &&
              r.candidate->state.world.ai.battle.actors.at({5}).state_counter == 1,
          "c append human5 receives current d but not current c; skipped monster d untouched");
    auto human = fixture();
    add_actor(human, 1);
    human.world.facilities.at(3).occupants = {{1}, {1}};
    human.spawn_cells = {{1, 1}};
    human.world.actors.at({1}).spawn_updates = 99;
    r = prepare_world_schedule(human, {}, consumer);
    check(r.candidate && r.candidate->state.world.ai.human_order.empty() &&
              r.candidate->state.world.facilities.at(3).occupants ==
                  std::vector<CharacterId>{{1}} &&
              r.candidate->state.world.ai.retired_actors.count({1}) &&
              r.candidate->state.world.ai.facility_actor_roots == std::vector<CharacterId>{{1}},
          "human d return true releases first q exactly once, duplicate occupation remains root");
    const auto retained = collect_ai_references(r.candidate->state.world.ai);
    check(retained.retired_actors.count({1}) && retained.contexts.count({1}),
          "subsequent narrow collector cannot reclaim current facility reference");
    r = prepare_world_schedule(human, {}, [&](const auto &w, const auto &call, const auto &field) {
        auto next = consumer(w, call, field);
        if (next && call.stage == WorldScheduleStage::decision)
            next->disposition = WorldScheduleDisposition::remove_requested;
        return next;
    });
    check(r.candidate && r.candidate->state.world.facilities.at(3).occupants.size() == 2,
          "human c return true retires without inventing d release");
}
void real_death_consumer() {
    auto s = fixture();
    add_actor(s, 2, ActorKind::monster, 3);
    auto &a = s.world.ai.battle.actors.at({2});
    a.definition = 7;
    a.state_counter = 12;
    a.state_parameter = 1; // 取消死亡，不制造奖励；仍执行原删除/退休路径。
    s.world.ai.monster_growth.emplace(7, RewardMonsterDefinition{});
    s.world.ai.battle.monsters.emplace(7, MonsterBattleRecord{});
    const auto r =
        prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
            if (call.stage == WorldScheduleStage::decision && call.id == 2) {
                const auto death = prepare_monster_death_commit(w.world.ai, {2});
                if (!death.candidate)
                    return std::optional<WorldScheduleStep>{};
                auto next = w;
                next.world.ai = death.candidate->state;
                return std::optional<WorldScheduleStep>{
                    {next, death.candidate->removed ? WorldScheduleDisposition::already_removed
                                                    : WorldScheduleDisposition::keep}};
            }
            return consumer(w, call, field);
        });
    check(r.candidate && r.candidate->state.world.ai.monster_order.empty() &&
              r.candidate->visits.size() == 3,
          "real death removes owner once, schedule mirrors once and no corpse d remains");
}
void actual_final_boundary() {
    auto s = fixture();
    add_actor(s, 1, ActorKind::human, 1);
    add_actor(s, 2, ActorKind::human, 1);
    s.world.ai.contexts.at({1}).inside_town = false;
    s.world.ai.contexts.at({2}).inside_town = false;
    s.world.ai.contexts.at({1}).cell = {1, s.town.top};
    s.world.ai.contexts.at({2}).cell = {1, s.town.top};
    auto r = prepare_world_schedule_overlap(s, {});
    check(r && r->world.ai.battle.actors.at({1}).position.x == 150,
          "L excludes exact transformed minimumY h.l[n.o][1][1] even with cached ax=false");
    s.world.ai.contexts.at({1}).cell.y = s.town.top + 1;
    s.world.ai.contexts.at({2}).cell.y = s.town.top + 1;
    check(!prepare_world_schedule_overlap(s, {}),
          "eligible outside pair requires RNG even if cached/world coordinates disagree");
    s.world.ai.battle.actors.at({2}).position = {160, 13, 160};
    r = prepare_world_schedule_overlap(s, {0});
    check(r && r->world.ai.battle.actors.at({1}).position.x == 130 &&
              r->world.ai.contexts.at({1}).cell == Position{1, s.town.top + 1} &&
              r->world.ai.contexts.at({1}).half_cell == Position{3, 3} &&
              r->world.ai.battle.actors.at({2}).position.height == 13,
          "L writes n.x/z only, leaves s/t and height untouched after source alias separation");
}
void deletion_differential() {
    for (const auto kind : {ActorKind::human, ActorKind::monster})
        for (const bool decision : {false, true})
            for (std::size_t count = 0; count <= 7; ++count)
                for (unsigned mask = 0; mask < (1U << count); ++mask) {
                    auto s = fixture();
                    for (std::size_t n = 0; n < count; ++n)
                        add_actor(s, n + 1, kind, kind == ActorKind::human ? 14 : 3);
                    std::vector<std::uint64_t> expected;
                    for (std::size_t n = 0; n < count; ++n)
                        expected.push_back(n + 1);
                    std::vector<std::uint64_t> visited;
                    std::int64_t cursor = !decision && kind == ActorKind::monster
                                              ? 0
                                              : static_cast<std::int64_t>(count) - 1;
                    while (cursor >= 0 && static_cast<std::size_t>(cursor) < expected.size()) {
                        const auto id = expected[static_cast<std::size_t>(cursor)];
                        visited.push_back(id);
                        if (mask & (1U << (id - 1)))
                            expected.erase(expected.begin() + cursor);
                        cursor += !decision && kind == ActorKind::monster ? 1 : -1;
                    }
                    const auto result = prepare_world_schedule(
                        s, {}, [&](const auto &w, const auto &call, const auto &field) {
                            auto next = consumer(w, call, field);
                            if (next && call.id &&
                                call.stage == (decision ? WorldScheduleStage::decision
                                                        : WorldScheduleStage::control) &&
                                (mask & (1U << (*call.id - 1))))
                                next->disposition = WorldScheduleDisposition::remove_requested;
                            return next;
                        });
                    check(result.candidate.has_value(),
                          "all real owner deletion-mask rounds prepare");
                    const auto &output = result.candidate->state.world.ai;
                    std::vector<std::uint64_t> actual;
                    for (const auto id :
                         kind == ActorKind::human ? output.human_order : output.monster_order)
                        actual.push_back(id.value);
                    check(actual == expected,
                          "real owner deletion matches independent index traversal");
                    actual.clear();
                    for (const auto &call : result.candidate->calls)
                        if (call.stage ==
                            (decision ? WorldScheduleStage::decision : WorldScheduleStage::control))
                            actual.push_back(*call.id);
                    check(actual == visited,
                          "phase visits match reverse and forward-with-skip source traversal");
                    for (const auto id : expected)
                        check(output.battle.actors.at({id}).state_counter ==
                                  (!decision && kind == ActorKind::monster &&
                                           std::find(visited.begin(), visited.end(), id) ==
                                               visited.end()
                                       ? 0
                                       : 1),
                              "skipped monster cannot advance B while all retained executed actors "
                              "do once");
                }
}
void actual_nonactor_consumers() {
    struct Owner {
        WorldScheduleState common;
        std::map<std::pair<int, int>, ObjectCatalogRecord> catalog;
        int item_rewards{};
        std::map<std::uint64_t, int> facility_updates;
        std::vector<ObjectCommitRequest> object_requests;
        std::vector<EncounterRequest> encounter_requests;
    };
    Owner source{fixture(), {}, 0, {{3, 0}}, {}, {}};
    add_actor(source.common, 1);
    add_actor(source.common, 2, ActorKind::monster, 3);
    source.common.world.ai.battle.actors.at({1}).control.queue = {{1, 10, 0}};
    source.common.world.ai.battle.actors.at({2}).hp = {0, 0, 0, 0, false, 0};
    source.common.world.ai.battle.actors.at({2}).capacity = 100;
    source.common.world.ai.projectiles.emplace(
        9, ProjectileState{ProjectileKind::delayed_damage, {1}, {2}, {}, {}, {}, 0, 6, 6, 0, 3});
    source.common.world.ai.projectile_order = {9};
    source.common.world.ai.battle.monsters.emplace(0, MonsterBattleRecord{});
    source.common.world.ai.monster_growth.emplace(0, RewardMonsterDefinition{});
    source.common.world.ai.battle.events.insert(89); // 无首访掉落副作用。
    GroundObjectState object;
    object.id = {8};
    object.state = 5;
    object.counter = 19;
    object.kind = 0;
    object.definition = 4;
    object.cached_cell = {1, 1};
    source.common.world.ai.battle.objects.emplace(8, object);
    source.common.world.object_order = {8};
    source.catalog.emplace(std::pair<int, int>{0, 4}, ObjectCatalogRecord{});
    RewardEncounter event;
    event.runtime.id = 7;
    event.runtime.state = 1;
    event.runtime.counter = 99;
    source.common.world.ai.encounters.emplace(7, event);
    source.common.world.ai.encounter_order = {7};
    OwnedWorldScheduleAdapter<Owner> adapter;
    adapter.read = [](const Owner &w) -> const WorldScheduleState & { return w.common; };
    adapter.write = [](Owner &w) -> WorldScheduleState & { return w.common; };
    adapter.consume = [&](const Owner &w, const auto &call,
                          const auto &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        auto next = w;
        auto &common = next.common;
        auto &ai = common.world.ai;
        WorldScheduleDisposition disposition{WorldScheduleDisposition::keep};
        if (call.stage == WorldScheduleStage::projectile) {
            WorldProjectileInput input;
            input.projectile = *call.id;
            input.drop_ticket = 99;                // 尸体再击仍消费100抽号，不能省略原已证分支。
            input.box = CollisionBox{-1, 0, 2, 2}; // 明确碰撞查询夹具，不认证原素材帧矩形。
            input.monster_boxes[0] = CollisionBox{-10, 10, 20, 20};
            const auto step = prepare_world_projectile(ai, input);
            if (!step.candidate)
                return {};
            ai = step.candidate->state;
            for (const auto id : step.candidate->spawned_objects)
                common.world.object_order.push_back(id);
            if (step.candidate->step.remove)
                disposition = WorldScheduleDisposition::already_removed;
        } else if (call.stage == WorldScheduleStage::object) {
            // 目录与物体只在这个调用点投影，不永久保存第二份objects/events。
            ObjectCommitState projection;
            projection.catalog = next.catalog;
            projection.objects = ai.battle.objects;
            projection.events = ai.battle.events;
            projection.item_rewards = next.item_rewards;
            const auto &o = ai.battle.objects.at(*call.id);
            const auto step = prepare_object_update(projection, {*call.id},
                                                    inside_town(o.cached_cell, common.town));
            if (!step.candidate)
                return {};
            next.catalog = step.candidate->state.catalog;
            next.item_rewards = step.candidate->state.item_rewards;
            ai.battle.objects = step.candidate->state.objects;
            ai.battle.events = step.candidate->state.events;
            next.object_requests.insert(next.object_requests.end(),
                                        step.candidate->requests.begin(),
                                        step.candidate->requests.end());
            if (step.candidate->remove) {
                auto &order = common.world.object_order;
                order.erase(std::find(order.begin(), order.end(), *call.id));
                disposition = WorldScheduleDisposition::already_removed;
            }
        } else if (call.stage == WorldScheduleStage::encounter) {
            EncounterCommitInput input;
            input.encounter = *call.id;
            const auto step =
                prepare_world_encounter_update(ai, world_schedule_facts(common), input, field);
            if (!step.candidate)
                return {};
            ai = step.candidate->state;
            common.world.map = step.candidate->facts.map;
            common.surface = step.candidate->facts.surface;
            common.map_flags = step.candidate->facts.flags;
            next.encounter_requests.insert(next.encounter_requests.end(),
                                           step.candidate->requests.begin(),
                                           step.candidate->requests.end());
            if (step.candidate->removed)
                disposition = WorldScheduleDisposition::already_removed;
        } else if (call.stage == WorldScheduleStage::facility) {
            // 本夹具明确status1/category2/空通知：Tenant.c只增f，随后d无记录。
            ++next.facility_updates.at(*call.id);
        } else {
            const auto step = consumer(common, call, field);
            if (!step)
                return {};
            common = step->state;
            disposition = step->disposition;
        }
        return OwnedWorldScheduleStep<Owner>{next, disposition};
    };
    const auto first = prepare_owned_world_schedule(source, {}, adapter);
    check(first.state && first.state->common.world.ai.projectile_order.empty() &&
              first.state->common.world.ai.encounter_order.empty() &&
              first.state->common.world.object_order == std::vector<std::uint64_t>{8},
          "real delayed projectile and event retirement erase storage/order once, object remains");
    check(first.state->catalog.at({0, 4}).inventory == 1 && first.state->item_rewards == 1 &&
              first.state->common.world.ai.battle.objects.at(8).counter == 20 &&
              first.state->facility_updates.at(3) == 1 &&
              first.state->common.world.ai.battle.actors.at({2}).hit_count == 1,
          "projectile re-hits corpse then same-round object20 grants once before facility update");
    check(source.catalog.at({0, 4}).inventory == 0 &&
              source.common.world.ai.projectiles.size() == 1,
          "original catalogue/projectile owner is not changed by successful preparation");
    auto spell_source = source;
    auto &spell = spell_source.common.world.ai.projectiles.at(9);
    spell.kind = ProjectileKind::spell;
    spell.position = {150, -1, 150};
    spell.counter = 0;
    spell.effect = 4;
    const auto spell_round = prepare_owned_world_schedule(spell_source, {}, adapter);
    check(spell_round.state &&
              spell_round.state->common.world.ai.projectile_order ==
                  std::vector<std::uint64_t>{1} &&
              spell_round.state->common.world.ai.projectiles.at(1).counter == 0 &&
              spell_round.state->common.world.ai.projectiles.at(1).kind ==
                  ProjectileKind::delayed_damage,
          "actual spell appends a delayed projectile unseen by current reverse bo pass");
    auto last = *first.state;
    last.common.world.ai.battle.objects.at(8).counter = 59;
    const auto final = prepare_owned_world_schedule(last, {}, adapter);
    check(final.state && final.state->common.world.object_order.empty() &&
              final.state->common.world.ai.battle.objects.empty() &&
              final.state->catalog.at({0, 4}).inventory == 1,
          "actual object60 removal synchronizes original bp without replaying inventory20");
    auto late = adapter;
    late.consume = [&](const Owner &w, const auto &call,
                       const auto &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        if (call.stage == WorldScheduleStage::finalize)
            return {};
        return adapter.consume(w, call, field);
    };
    const auto refused = prepare_owned_world_schedule(source, {}, late);
    check(
        !refused.state && source.item_rewards == 0 &&
            source.common.world.ai.encounters.size() == 1 &&
            source.common.world.ai.battle.objects.at(8).counter == 19,
        "late final-domain failure rolls back actual HP, inventory, projectile and event removal");
}
void errors_and_atomic_extension() {
    auto s = fixture();
    add_actor(s, 1);
    s.world.ai.battle.actors.at({1}).control.queue = {{21}, {1, 3, 0}};
    auto r = prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
        return call.stage == WorldScheduleStage::finalize ? std::optional<WorldScheduleStep>{}
                                                          : consumer(w, call, field);
    });
    check(!r.candidate && r.error == WorldScheduleError::consumer_failed &&
              s.world.facilities.at(3).occupants.empty() && s.updates == 0,
          "late missing L/random consumer rolls back counters, wait and occupation together");
    r = prepare_world_schedule(s, {false, 1000000}, {});
    check(r.candidate && !r.candidate->start_field && r.candidate->calls.empty() &&
              r.candidate->state.world.ai.battle.actors.at({1}).state_counter == 0,
          "nonadmitted world does not accumulate AI work or run any handler");
    check(prepare_world_schedule(s, {}, {}).error == WorldScheduleError::missing_consumer,
          "missing domain consumer is explicit, no demo fallback");
    check(prepare_world_schedule(s, {true, 1}, consumer).error ==
              WorldScheduleError::dispatch_limit,
          "maintenance budget failure does not postpone partially consumed world");
    r = prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
        auto next = consumer(w, call, field);
        if (next && call.stage == WorldScheduleStage::decision) {
            next->state.world.ai.human_order.clear();
            next->state.world.ai.battle.actors.clear();
        }
        return next;
    });
    check(!r.candidate && r.error == WorldScheduleError::invalid_mutation,
          "silent domain removal cannot desynchronize the source/live roster mirror");
    struct Owner {
        WorldScheduleState common;
        int committed{};
    };
    Owner outer{s, 0};
    OwnedWorldScheduleAdapter<Owner> adapter;
    adapter.read = [](const Owner &w) -> const WorldScheduleState & { return w.common; };
    adapter.write = [](Owner &w) -> WorldScheduleState & { return w.common; };
    adapter.consume = [&](const Owner &w, const auto &call,
                          const auto &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        if (call.stage == WorldScheduleStage::finalize)
            return {};
        const auto next = consumer(w.common, call, field);
        if (!next)
            return {};
        auto output = w;
        output.common = next->state;
        ++output.committed;
        return OwnedWorldScheduleStep<Owner>{output, next->disposition};
    };
    const auto owned = prepare_owned_world_schedule(outer, {}, adapter);
    check(!owned.state && outer.committed == 0 &&
              outer.common.world.facilities.at(3).occupants.empty(),
          "outer catalogue/task/random-cursor extension and common world fail atomically");
    adapter.consume = [&](const Owner &w, const auto &call,
                          const auto &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        const auto next = consumer(w.common, call, field);
        if (!next)
            return {};
        auto output = w;
        output.common = next->state;
        ++output.committed;
        return OwnedWorldScheduleStep<Owner>{output, next->disposition};
    };
    const auto success = prepare_owned_world_schedule(outer, {}, adapter);
    check(success.state && success.state->committed == 5 &&
              success.state->common.world.ai.battle.actors.at({1}).state_counter == 1 &&
              success.state->common.world.facilities.at(3).occupants.size() == 1,
          "all private outer and common fields commit once after final L");
    s.world.actors.clear();
    check(!valid_world_schedule_owner(s), "spawn without shared actor context rejects before c");
}
void prelude_mutation_guards() {
    for (const auto stage : {WorldScheduleStage::arrival_front, WorldScheduleStage::popularity,
                             WorldScheduleStage::carry_expression, WorldScheduleStage::finalize}) {
        auto s = fixture();
        add_actor(s, 1);
        s.popularity_queue = {{1, 1, 1}};
        s.world.ai.battle.actors.at({1}).object_slot = -2;
        s.world.ai.battle.actors.at({1}).rescue = CharacterId{1};
        const auto r =
            prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
                if (call.stage == stage) {
                    auto next = w;
                    next.world.ai.human_order.clear();
                    next.world.ai.battle.actors.clear();
                    return std::optional<WorldScheduleStep>{{next}};
                }
                return consumer(w, call, field);
            });
        check(!r.candidate && r.error == WorldScheduleError::invalid_mutation,
              "front/popularity/carry/L cannot silently erase before next owner consumer");
    }
    auto s = fixture();
    const auto r =
        prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
            auto next = consumer(w, call, field);
            if (next && call.stage == WorldScheduleStage::finalize)
                add_actor(next->state, 1);
            return next;
        });
    check(!r.candidate && r.error == WorldScheduleError::invalid_mutation,
          "L only changes positions, it cannot append an otherwise valid new character");
    for (const bool monster : {false, true}) {
        const auto denied =
            prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
                auto next = consumer(w, call, field);
                if (next && call.stage == WorldScheduleStage::arrival_front) {
                    if (monster)
                        add_actor(next->state, 1, ActorKind::monster);
                    else {
                        auto f = next->state.world.facilities.at(3);
                        f.placement.instance_id = {4};
                        next->state.world.facilities.emplace(4, f);
                        next->state.facility_order.push_back(4);
                    }
                }
                return next;
            });
        check(!denied.candidate && denied.error == WorldScheduleError::invalid_mutation,
              "arrival cannot append an otherwise valid monster or facility");
    }
    s.world.ai.projectiles.emplace(1, ProjectileState{});
    s.world.ai.projectiles.emplace(2, ProjectileState{});
    s.world.ai.projectile_order = {1, 2};
    s.world.ai.encounters.emplace(1, RewardEncounter{});
    s.world.ai.encounter_order = {1};
    for (int mode = 0; mode < 4; ++mode) {
        const auto result =
            prepare_world_schedule(s, {}, [&](const auto &w, const auto &call, const auto &field) {
                auto next = consumer(w, call, field);
                if (next && call.stage == WorldScheduleStage::arrival_front) {
                    auto &ai = next->state.world.ai;
                    if (mode == 0) {
                        ai.projectiles.clear();
                        ai.projectile_order.clear();
                        ai.encounters.clear();
                        ai.encounter_order.clear();
                    } else if (mode == 1) {
                        ai.projectiles.erase(1);
                        ai.projectile_order.erase(ai.projectile_order.begin());
                    } else if (mode == 2) {
                        ai.projectiles.emplace(3, ProjectileState{});
                        ai.projectile_order.push_back(3);
                    } else {
                        ai.encounters.clear();
                        ai.encounter_order = {3};
                        ai.encounters.emplace(3, RewardEncounter{});
                    }
                }
                return next;
            });
        check(mode == 0
                  ? result.candidate.has_value()
                  : (!result.candidate && result.error == WorldScheduleError::invalid_mutation),
              "arrival allows source clearAll but forbids partial removal, append or replacement");
    }
}
void actual_exit_journey_rounds() {
    struct Owner {
        WorldScheduleState common;
        std::map<int, int> definitions;
    };
    Owner source{fixture(), {{0, 0}}};
    add_actor(source.common, 1, ActorKind::human, 0);
    source.common.world.actors.at({1}).binding.reset();
    source.common.world.actors.at({1}).journey.reset();
    source.common.world.ai.battle.actors.at({1}).position = {250, 0, 150};
    source.common.world.ai.battle.actors.at({1}).control.queue = {{8, 5}};
    source.common.world.ai.contexts.at({1}).cell = {2, 1};
    source.common.world.ai.contexts.at({1}).half_cell = {5, 3};
    source.common.spawn_cells = {{3, 1}, {4, 1}}; // 有来源的接口，坐标本身仍是小地图夹具。
    OwnedWorldScheduleAdapter<Owner> adapter;
    adapter.read = [](const Owner &w) -> const WorldScheduleState & { return w.common; };
    adapter.write = [](Owner &w) -> WorldScheduleState & { return w.common; };
    adapter.consume = [&](const Owner &w, const auto &call,
                          const auto &field) -> std::optional<OwnedWorldScheduleStep<Owner>> {
        auto next = w;
        if (call.stage == WorldScheduleStage::decision) {
            WorldPathInput input;
            input.actor = {*call.id};
            input.facts = world_schedule_facts(w.common);
            input.exits = w.common.spawn_cells;
            const auto p = prepare_world_path_c(w.common.world, input);
            if (!p.candidate)
                return {};
            next.common.world = p.candidate->state;
            return OwnedWorldScheduleStep<Owner>{
                next, p.candidate->delete_instance ? WorldScheduleDisposition::remove_requested
                                                   : WorldScheduleDisposition::keep};
        }
        if (call.stage == WorldScheduleStage::control) {
            WorldControlAdapter<Owner> control;
            control.read = [](const Owner &o, CharacterId id) -> const ActorControlState * {
                const auto a = o.common.world.ai.battle.actors.find(id);
                return a == o.common.world.ai.battle.actors.end() ? nullptr : &a->second.control;
            };
            control.write = [](Owner &o, CharacterId id, const ActorControlState &c) {
                o.common.world.ai.battle.actors.at(id).control = c;
                return true;
            };
            control.domain = [](const Owner &o,
                                CharacterId id) -> std::optional<WorldControlStep<Owner>> {
                auto n = o;
                const int opcode = o.common.world.ai.battle.actors.at(id).control.queue.front()[0];
                if (opcode == 8) {
                    WorldDepartureControlInput input;
                    auto &d = input.departure;
                    d.actor = id;
                    d.catalogue.town = o.common.town;
                    d.catalogue.definitions = {{0, 0, 0}, {33, 2, 10}};
                    d.catalogue.cell_definition_ids.assign(o.common.world.map.cells.size(), 0);
                    for (std::size_t at = 0; at < o.common.world.map.cells.size(); ++at)
                        if (o.common.world.map.cells[at].facility)
                            d.catalogue.cell_definition_ids[at] =
                                o.common.world.map.cells[at].facility->definition_id;
                    d.home = WorldDepartureHome{{0, 0}, 0};
                    d.exits = o.common.spawn_cells;
                    d.tickets = {0};
                    const auto r = prepare_world_departure_control(o.common.world, input);
                    if (!r.candidate)
                        return {};
                    n.common.world = r.candidate->state;
                    return WorldControlStep<Owner>{n, r.candidate->delete_instance
                                                          ? WorldControlAction::delete_true
                                                      : r.candidate->continue_interpreter
                                                          ? WorldControlAction::continue_same_call
                                                          : WorldControlAction::hold_false};
                }
                if (opcode == 26) {
                    const auto r =
                        prepare_world_misc_control({o.common.world, o.definitions}, {id, {}, {}});
                    if (!r.candidate)
                        return {};
                    n.common.world = r.candidate->state.world;
                    n.definitions = r.candidate->state.human_definition_state;
                    return WorldControlStep<Owner>{n, r.candidate->action};
                }
                if (opcode != 0)
                    return {};
                const auto r = prepare_world_facility_control(o.common.world, {id, {}, {}});
                if (!r.candidate)
                    return {};
                n.common.world = r.candidate->state;
                return WorldControlStep<Owner>{n, r.candidate->flow == ActorControlFlow::moving
                                                      ? WorldControlAction::hold_false
                                                      : WorldControlAction::continue_same_call};
            };
            const auto r = prepare_world_control(w, {*call.id}, control);
            if (!r.candidate)
                return {};
            return OwnedWorldScheduleStep<Owner>{
                r.candidate->state, r.candidate->flow == WorldControlFlow::delete_requested
                                        ? WorldScheduleDisposition::remove_requested
                                        : WorldScheduleDisposition::keep};
        }
        const auto r = consumer(w.common, call, field);
        if (!r)
            return {};
        next.common = r->state;
        return OwnedWorldScheduleStep<Owner>{next, r->disposition};
    };
    auto current = source;
    int ticks{};
    while (!current.common.world.ai.human_order.empty() && ticks++ < 120) {
        const auto r = prepare_owned_world_schedule(current, {}, adapter);
        check(r.state.has_value(), "actual exit journey advances under one real common owner");
        current = *r.state;
    }
    check(ticks > 1 && ticks < 120 && current.common.world.ai.human_order.empty() &&
              current.definitions.at(0) == 1 && source.definitions.at(0) == 0,
          "real8 exit choice then P/c movement, d projection, exit0/26 and scheduler deletion "
          "complete");
    const auto consume = adapter.consume;
    adapter.consume = [consume](const Owner &w, const auto &call, const auto &field) {
        return call.stage == WorldScheduleStage::finalize
                   ? std::optional<OwnedWorldScheduleStep<Owner>>{}
                   : consume(w, call, field);
    };
    check(!prepare_owned_world_schedule(source, {}, adapter).state &&
              !source.common.world.actors.at({1}).unbound_route &&
              source.common.world.ai.battle.actors.at({1}).control.queue.front()[0] == 8 &&
              source.common.updates == 0,
          "late common L refusal rolls back actual departure and all common-prefix state");
}
void actual_facility_self_removal() {
    auto source = fixture();
    for (const auto id : {4ULL, 5ULL, 6ULL}) {
        auto facility = source.world.facilities.at(3);
        facility.placement.instance_id = {id};
        source.world.facilities.emplace(id, facility);
        source.facility_order.push_back(id);
    }
    const auto erase = [](auto &state, std::uint64_t id) {
        state.world.facilities.erase(id);
        auto &order = state.facility_order;
        order.erase(std::find(order.begin(), order.end(), id));
        for (auto &cell : state.world.map.cells)
            if (cell.facility && cell.facility->instance_id.value == id) {
                cell.facility.reset();
                cell.legacy_state = 4;
                cell.category = RouteCategory::terminal;
            }
    };
    const auto complete = [&](const auto &state, const auto &call, const auto &field) {
        auto step = consumer(state, call, field);
        if (step && call.stage == WorldScheduleStage::facility && call.id == 3) {
            erase(step->state, 3);
            auto appended = state.world.facilities.at(4);
            appended.placement.instance_id = {7};
            step->state.world.facilities.emplace(7, appended);
            step->state.facility_order.push_back(7);
            step->disposition = WorldScheduleDisposition::already_removed;
        }
        return step;
    };
    const auto result = prepare_world_schedule(source, {}, complete);
    check(result.candidate &&
              result.candidate->state.facility_order == std::vector<std::uint64_t>{4, 5, 6, 7} &&
              !result.candidate->state.world.facilities.count(3),
          "real facility c completion self-removal accepted only as already_removed with absent "
          "owner");
    std::vector<std::uint64_t> visited;
    for (const auto &call : result.candidate->calls)
        if (call.stage == WorldScheduleStage::facility)
            visited.push_back(*call.id);
    check(visited == std::vector<std::uint64_t>{3, 5, 6, 7},
          "forward self-removal mirrors once, skips shifted4 and visits new appended7 same round");
    const auto next = prepare_world_schedule(result.candidate->state, {}, consumer);
    visited.clear();
    for (const auto &call : next.candidate->calls)
        if (call.stage == WorldScheduleStage::facility)
            visited.push_back(*call.id);
    check(next.candidate && visited == std::vector<std::uint64_t>{4, 5, 6, 7},
          "shifted skipped facility remains active and updates on subsequent complete round");
    for (const auto disposition :
         {WorldScheduleDisposition::keep, WorldScheduleDisposition::remove_requested}) {
        const auto rejected = prepare_world_schedule(
            source, {}, [&](const auto &state, const auto &call, const auto &field) {
                auto step = complete(state, call, field);
                if (step && call.stage == WorldScheduleStage::facility && call.id == 3)
                    step->disposition = disposition;
                return step;
            });
        check(!rejected.candidate && rejected.error == WorldScheduleError::invalid_mutation,
              "actual self-removal cannot claim keep or request a second scheduler removal");
    }
    const auto false_claim = prepare_world_schedule(
        source, {}, [&](const auto &state, const auto &call, const auto &field) {
            auto step = consumer(state, call, field);
            if (step && call.stage == WorldScheduleStage::facility && call.id == 3)
                step->disposition = WorldScheduleDisposition::already_removed;
            return step;
        });
    check(!false_claim.candidate && false_claim.error == WorldScheduleError::invalid_mutation,
          "already_removed must observe actual absence, not a fake successful deletion token");
    for (const bool remove_other : {false, true}) {
        const auto invalid = prepare_world_schedule(
            source, {}, [&](const auto &state, const auto &call, const auto &field) {
                auto step = complete(state, call, field);
                if (step && call.stage == WorldScheduleStage::facility && call.id == 3) {
                    if (remove_other)
                        erase(step->state, 4);
                    else
                        std::swap(step->state.facility_order[0], step->state.facility_order[1]);
                }
                return step;
            });
        check(!invalid.candidate && invalid.error == WorldScheduleError::invalid_mutation,
              "facility self-removal does not permit deletion/reordering of other live facilities");
    }
    const auto failed = prepare_world_schedule(
        source, {}, [&](const auto &state, const auto &call, const auto &field) {
            return call.stage == WorldScheduleStage::finalize ? std::optional<WorldScheduleStep>{}
                                                              : complete(state, call, field);
        });
    check(!failed.candidate && failed.error == WorldScheduleError::consumer_failed &&
              source.facility_order == std::vector<std::uint64_t>{3, 4, 5, 6} &&
              source.world.facilities.count(3) && source.updates == 0,
          "late L refusal rolls back domain self-removal, new facility and common counters");
}
} // namespace
int main() {
    try {
        prelude_and_round();
        arrival_field_visibility();
        live_rosters_and_removal();
        real_death_consumer();
        actual_final_boundary();
        deletion_differential();
        actual_nonactor_consumers();
        errors_and_atomic_extension();
        prelude_mutation_guards();
        actual_exit_journey_rounds();
        actual_facility_self_removal();
        std::cout << "world_schedule " << checks << " checks\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
