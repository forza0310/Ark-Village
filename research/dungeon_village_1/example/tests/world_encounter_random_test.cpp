#include "dungeon_village_reference/encounter_creation.hpp"
#include "dungeon_village_reference/world_dungeon_finish.hpp"
#include "dungeon_village_reference/world_random.hpp"

#include <iostream>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
struct Provider {
    WorldRandomStream stream;
    std::vector<int> bounds{};
    WorldRandomError error{WorldRandomError::none};
    std::optional<int> draw(int bound) {
        bounds.push_back(bound);
        const auto result = stream.draw(bound);
        if (result.error != WorldRandomError::none) {
            error = result.error;
            return {};
        }
        return result.ticket;
    }
};
AiRewardState creation_fixture() {
    AiRewardState s;
    s.next_actor_id = 10;
    for (int id : {0, 1, 2}) {
        RewardMonsterDefinition d;
        d.growth = id == 0 ? 8 : 0;
        d.base_hp = 20;
        d.status = id == 0 ? 1 : 0;
        d.introduced = true;
        s.monster_growth[id] = d;
        s.battle.monsters[id] = MonsterBattleRecord{};
        s.monster_definition_order.push_back(id);
    }
    BattleActorRecord actor;
    actor.id = {1};
    actor.control.state = 5;
    s.battle.actors[actor.id] = actor;
    s.contexts[actor.id].cell = {10, 10};
    s.human_order = {actor.id};
    return s;
}
DungeonWorldState dungeon_fixture() {
    DungeonWorldState s;
    s.world.map = {5, 5, std::vector<LegacyMapCell>(25)};
    RescueFacility facility;
    facility.placement = {{3}, 33, FacilityShape::single, FacilityOrientation::first, {2, 2}};
    facility.category = 5;
    s.world.facilities[3] = facility;
    s.world.map = *bind_facility_map(s.world.map, {{facility.placement, 3}}).map;
    s.facilities[3].extent = 10000;
    s.world.ai.growth[0] = RewardHumanDefinition{};
    s.world.ai.growth[0].definition.profession_levels = {1};
    BattleActorRecord actor;
    actor.id = {1};
    actor.control.state = 14;
    actor.control.queue = {{21}};
    actor.capacity = 100;
    actor.hp = {0, 10, 10, 10, false, 0};
    s.world.ai.battle.actors[actor.id] = actor;
    s.world.ai.human_order = {actor.id};
    s.world.ai.contexts[actor.id] = RewardActorContext{};
    s.world.actors[actor.id].binding = ArrivalBinding{{2, 2}, {3}, 33};
    s.actors[actor.id] = DungeonActorProgress{};
    return s;
}
void creation_gates_and_dynamic_catalogue() {
    const auto original = creation_fixture();
    Provider p{WorldRandomStream::from_raw({99, 0, 0, 10, 20})};
    EncounterCreationInput i;
    i.center = {10, 10};
    i.month_index = 3;
    i.draw = [&](int bound) { return p.draw(bound); };
    auto r = prepare_encounter_creation(original, i);
    check(r.candidate && r.candidate->created && r.candidate->state.monster_order.size() == 1 &&
              p.bounds == std::vector<int>{100, 1, 2, 100, 100} && p.stream.draws() == 5,
          "count100/actualnearby1/unlocked catalogue2/spawn100/100 in source order");
    check(r.candidate->state.battle.actors.at({10}).definition == 1 &&
              r.candidate->state.monster_growth.at(1).status == 1 &&
              original.monster_growth.at(1).status == 0 && original.monster_order.empty(),
          "candidate unlock affects actual drawbound and no source mutation");
    i.upper_band_town[1] = true;
    p = {WorldRandomStream::from_raw({})};
    r = prepare_encounter_creation(original, i);
    check(r.candidate && r.candidate->denial == EncounterCreationDenial::town && p.bounds.empty(),
          "town denial before ordinary count, no prefetch/exhaustion error");
    i.upper_band_town[1] = false;
    i.probe = EncounterCreationProbe{{1}, true, 0, {}, 4, {}};
    r = prepare_encounter_creation(original, i);
    check(r.candidate && r.candidate->denial == EncounterCreationDenial::probe && p.bounds.empty(),
          "destination-town L guard avoids probe1000 entirely");
    i.probe->destination_inside_town = false;
    p = {WorldRandomStream::from_raw({999})};
    r = prepare_encounter_creation(original, i);
    check(r.candidate && r.candidate->denial == EncounterCreationDenial::probe &&
              p.bounds == std::vector<int>{1000},
          "eligible L consumes1000 even denied by probability");
    i.probe.reset();
    i.count_ticket = 99;
    i.nearby_ticket = 0;
    i.monsters = {{0, {10, 20}}};
    p = {WorldRandomStream::from_raw({})};
    r = prepare_encounter_creation(original, i);
    check(r.candidate && p.bounds.empty(), "complete explicit creation prefix needs no provider");
    i.monsters.clear();
    p = {WorldRandomStream::from_raw({0, 10})};
    r = prepare_encounter_creation(original, i);
    check(!r.candidate && p.error == WorldRandomError::exhausted &&
              p.bounds == std::vector<int>{2, 100, 100} && original.encounters.empty(),
          "late missing spawn draw fails, never silently substitutes zero or partial world");
}
void group_and_event_interleaving() {
    BattleGroupState group;
    group.tick = 79;
    group.humans = {{{1}, 128U}};
    group.monsters = {{{2}, 128U}, {{3}, 128U | 16384U}, {{4}, 0}};
    Provider p{WorldRandomStream::from_raw({99})};
    auto r = prepare_battle_group_step(group, {49}, [&](int bound) { return p.draw(bound); });
    check(r.candidate && r.candidate->consumed_tickets == 2 && p.bounds == std::vector<int>{100} &&
              r.candidate->assignments[0].monster_posture == 0 &&
              r.candidate->assignments[1].monster_posture == 2,
          "prune then explicit posture prefix and real lazy100 for surviving second monster");
    group.monsters = {{{2}, 128U}};
    p = {WorldRandomStream::from_raw({})};
    r = prepare_battle_group_step(group, {}, [&](int bound) { return p.draw(bound); });
    check(r.candidate && p.bounds.empty(), "only one surviving monster does not draw posture");
    EncounterStepInput i;
    i.state = {1, {5, 5}, 0, 0, 0, 1, 0, 100};
    i.humans = {{{1}, 0, {5, 5}, false, 1, 2, 128U, 10, 1},
                {{2}, 1, {5, 5}, false, 1, 2, 128U, 10, 1}};
    p = {WorldRandomStream::from_raw({0, 0, 0, 0, 0, 0, 0, 0})};
    i.draw = [&](int bound) { return p.draw(bound); };
    i.random_request = [&](const EncounterRequest &request) {
        if (request.kind == EncounterRequestKind::update_group)
            return p.draw(100).has_value(); // 显式顺序夹具，非真实group消费者。
        if (request.kind == EncounterRequestKind::expression5)
            return p.draw(1000).has_value() && p.draw(3).has_value();
        return true;
    };
    auto event = prepare_encounter_step(i);
    check(event.candidate && p.bounds == std::vector<int>{100, 1000, 100, 1000, 3, 100, 1000, 3},
          "group first, semantic1000, eachhuman100 then expression1000/variant before nexthuman");
    p = {WorldRandomStream::from_raw({0, 0, 0, 0, 0, 0, 0})};
    event = prepare_encounter_step(i);
    // 正常两人需8值；缺最后变体必须失败，不能继续下一消费器。
    check(!event.candidate && p.error == WorldRandomError::exhausted,
          "request consumer failure cannot produce partial event candidate");
}
void quest_branch_bounds() {
    EncounterStepInput i;
    i.state = {1, {5, 5}, 3, 0, 0, 0, 2, 0};
    i.group_exists = false;
    i.humans = {{{1}, 0, {5, 5}, false, 1, 0, 0, 0, 0}};
    i.quest.normal_definitions = {1, 2, 3};
    i.quest.boss_definition = 9;
    i.quest_cells = {{{4, 5}, 4, true}, {{6, 5}, 4, false}};
    Provider p{WorldRandomStream::from_raw({0})};
    i.draw = [&](int bound) { return p.draw(bound); };
    auto r = prepare_encounter_step(i);
    check(r.candidate && !r.candidate->spawn && p.bounds == std::vector<int>{2},
          "town chosen quest cell consumes cell draw only, no definition or offsets");
    p = {WorldRandomStream::from_raw({1, 2})};
    r = prepare_encounter_step(i);
    check(r.candidate && r.candidate->spawn->definition == 3 && p.bounds == std::vector<int>{2, 3},
          "non-last quest spawn draws actual normal list3");
    i.state.spawned = 1;
    p = {WorldRandomStream::from_raw({1})};
    r = prepare_encounter_step(i);
    check(r.candidate && r.candidate->spawn->definition == 9 && p.bounds == std::vector<int>{2},
          "last quota selects fixed boss, never draws normal catalogue");
}
void dungeon_launch_order() {
    auto s = dungeon_fixture();
    s.facilities[3].challenges.assign(6, {30, 0, 0, 0, 0, 9});
    Provider p{WorldRandomStream::from_raw({1})};
    auto entry = prepare_world_dungeon_entry(s, {1}, {}, [&](int bound) { return p.draw(bound); });
    check(entry.candidate && entry.candidate->entry_event == 169 && p.bounds == std::vector<int>{2},
          "first uniform expedition prefix only emits actualnotice draw2");
    s.world.facilities.at(3).occupants = {{1}};
    p = {WorldRandomStream::from_raw({0, 79, 1})};
    auto retreat = prepare_world_dungeon_retreat(s, {1}, {2, 2}, {0, 1, 0, 1}, {},
                                                 [&](int bound) { return p.draw(bound); });
    check(retreat.candidate && p.bounds == std::vector<int>{8, 80, 80} &&
              retreat.candidate->state.world.ai.battle.actors.at({1}).control.state == 20,
          "actual legaldestination8 then horizontal offsets80/80");
    p = {WorldRandomStream::from_raw({})};
    retreat = prepare_world_dungeon_retreat(s, {1}, {2, 2}, {0, 4, 0, 4}, {},
                                            [&](int bound) { return p.draw(bound); });
    check(retreat.candidate && p.bounds.empty() && retreat.candidate->consumed_launches == 0,
          "no legal destination still teleports but draws no launch tickets");
    s.actors[{1}].retreat_state = 1;
    s.actors[{1}].retreat_updates = 19;
    s.facilities[3].challenges = {{0, 0, 1, 9, 0, 9}};
    p = {WorldRandomStream::from_raw({0, 1, 2, 3, 4, 5})};
    DungeonWorldCrewInput input{3, {0, 1, 0, 1}, 10000, {}};
    input.draw = [&](int bound) { return p.draw(bound); };
    const auto crew = prepare_world_dungeon_crew(s, input);
    check(crew.candidate && crew.candidate->consumed_launches == 2 &&
              p.bounds == std::vector<int>{8, 80, 80, 8, 80, 80} &&
              crew.candidate->state.world.ai.battle.objects.size() == 1,
          "live retreat launch precedes challenge-age treasure throw launch in same crew update");
}
void finish_summary_draws() {
    DungeonFinishState s;
    s.dungeon = dungeon_fixture();
    s.dungeon.world.facilities.at(3).status = 2;
    s.dungeon.facilities.at(3).updates = 10;
    s.sites[3].occupied_cells = {{2, 2}};
    s.surface.resize(25);
    s.ground_definition = 7;
    s.task_progress.definitions[0] = DungeonTaskDefinitionProgress{};
    s.tasks[10] = {10, 0, 1, 9, 3, Position{2, 2}};
    s.active_task = 10;
    s.task_order = {10};
    s.participants = {0, 0, 2};
    s.human_definition_flags = {{0, 2U}, {2, 2U}};
    const auto consumer = [](const DungeonFinishState &before, const DungeonFinishEffect &e) {
        auto after = before;
        if (e.kind == DungeonFinishEffectKind::event)
            ++after.event_calls[e.first];
        return std::optional<DungeonFinishState>{after}; // 显式请求顺序夹具，不冒称真实map/UI。
    };
    const auto committed = WorldRandomStream::from_raw({2, 2});
    Provider p{committed}; // 必须是外层私有流，失败时不能推进committed。
    DungeonFinishInput input{3, {}};
    input.draw = [&](int bound) { return p.draw(bound); };
    auto r = prepare_world_dungeon_finish(s, input, consumer);
    check(
        r.candidate && p.bounds == std::vector<int>{3, 3} &&
            r.candidate->consumed_summary_tickets == 2 && committed.draws() == 0,
        "summary two independent actualparticipant-count3 draws, outer committed stream untouched");
    bool found{};
    for (const auto &e : r.candidate->effects)
        if (e.kind == DungeonFinishEffectKind::summary32)
            found = e.first == 2 && e.second == 2;
    check(found, "same absent-live participant may be selected twice independently");
    input.summary_tickets = {2};
    p = {WorldRandomStream::from_raw({1})};
    r = prepare_world_dungeon_finish(s, input, consumer);
    check(r.candidate && p.bounds == std::vector<int>{3},
          "one summary explicit prefix leaves only second independent lazy draw");
    input.summary_tickets = {3};
    p = {WorldRandomStream::from_raw({0})};
    check(!prepare_world_dungeon_finish(s, input, consumer).candidate && p.bounds.empty(),
          "invalid first summary prefix rejected without consuming missing second draw");
    input.summary_tickets.clear();
    s.dungeon.facilities.at(3).updates = 9;
    p = {WorldRandomStream::from_raw({})};
    check(prepare_world_dungeon_finish(s, input, consumer).candidate && p.bounds.empty(),
          "pre-threshold finish does not eagerly request summary choices");
    s.dungeon.facilities.at(3).updates = 10;
    s.task_progress.definitions.at(0).kind = 1;
    check(prepare_world_dungeon_finish(s, input, consumer).candidate && p.bounds.empty(),
          "kind1 clear path needs no summary draws");
    s.task_progress.definitions.at(0).kind = 0;
    p = {WorldRandomStream::from_raw({2})};
    check(!prepare_world_dungeon_finish(s, input, consumer).candidate &&
              p.error == WorldRandomError::exhausted && s.active_task == 10 &&
              s.dungeon.world.facilities.count(3) && committed.draws() == 0,
          "exhausted second draw rejects all candidate changes, caller can discard private cursor");
}
} // namespace
int main() {
    try {
        creation_gates_and_dynamic_catalogue();
        group_and_event_interleaving();
        quest_branch_bounds();
        dungeon_launch_order();
        finish_summary_draws();
        std::cout << "world encounter random checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
