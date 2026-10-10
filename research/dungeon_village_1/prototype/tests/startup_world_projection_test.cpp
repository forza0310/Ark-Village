#include "dungeon_village_prototype/startup_world_projection.hpp"
#include "dungeon_village_prototype/startup_information.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "support/world_fixture.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
StartupSession installed() {
    StartupSession session;
    for (int i = 0; i < 420; ++i)
        check(session.update() == StartupError::none, "actual first arrival steps");
    return session;
}
void town_information() {
    auto state=test_support::world_fixture();
    // 只构造统计输入，不宣称从真实经营取得。固定原表无kind9定义，
    // 私有规则变体覆盖原34的kind9分支；不修改原表或创建可运行的演示世界。
    StartupWorldRules rules=*state.rules;
    state.rules=&rules;
    auto type9=std::find_if(rules.facilities.begin(),rules.facilities.end(),[](const auto &d){return d.id==18;});
    check(type9!=rules.facilities.end(),"town statistics fixture uses an existing private definition");
    type9->kind=9;
    for(auto &entry:state.human_presence)entry.second=0;
    for(auto &entry:state.human_homes)entry.second[2]=0;
    state.human_presence.at(1)=1;state.human_presence.at(2)=2;state.human_presence.at(3)=1;
    state.human_homes.at(0)[2]=1;state.human_homes.at(1)[2]=1;state.human_homes.at(2)[2]=2;
    state.rank=3;state.task_progress.successes=12;state.events_held=7;
    auto &world=state.scene.world.world;
    world.facilities.clear();
    const auto add_facility=[&](std::uint64_t id,int definition) {
        const auto d=std::find_if(rules.facilities.begin(),rules.facilities.end(),
                                 [=](const auto &value){return value.id==definition;});
        check(d!=rules.facilities.end(),"statistics fixture retains original facility definitions");
        ref::RescueFacility f;
        f.placement.instance_id={id};f.placement.definition_id=definition;
        f.kind=d->kind;f.category=d->category;f.detail=d->detail;
        world.facilities.emplace(id,f);
    };
    add_facility(1,35);add_facility(2,18);add_facility(3,25);add_facility(4,28);
    state.scene.world.facility_order={3,1,2,4,1}; // 原g按出现次数计，不能偷偷唯一化。
    for(auto &entry:state.catalog)if(entry.first.first>0)entry.second.status=0;
    for(const auto key:{std::pair<int,int>{1,0},{2,45},{2,0},{3,26},{3,29}}) {
        state.catalog.at(key).status=1;state.catalog.at(key).flags=0;
    }
    state.catalog.at({1,1}).status=2;
    state.catalog.at({2,0}).inventory=999;
    const auto before=startup_world_state_digest(state);
    const auto result=startup_town_information(state);
    check(result && result->rank==3 && result->adventurers==2 && result->residents==2 &&
              result->facilities==4 && result->completed_tasks==12 && result->activities_held==7 &&
              result->known_equipment==std::array<int,4>{1,1,1,2},
          "raw34 counts p1, D2==1, ordered kind3/9 occurrences and known equipment without38 flag filter");
    check(startup_world_state_digest(state)==before,
          "town projection keeps NEW, records, random, medals, pages and outputs unchanged");
    const std::array<const char*,11> refusals{{
        "town: missing rules", "town: missing human presence", "town: missing home definition",
        "town: extra human identity", "town: retired facility reference", "town: missing facility definition",
        "town: inconsistent facility kind", "town: missing equipment progress", "town: negative rank",
        "town: negative task total", "town: negative activity total"}};
    for(int fault=0;fault<static_cast<int>(refusals.size());++fault) {
        auto bad=state;
        if(fault==0)bad.rules=nullptr;
        if(fault==1)bad.human_presence.erase(1);
        if(fault==2)bad.human_homes.erase(0);
        if(fault==3)bad.human_presence.emplace(99,1);
        if(fault==4)bad.scene.world.facility_order.push_back(999);
        if(fault==5)bad.scene.world.world.facilities.at(1).placement.definition_id=999;
        if(fault==6)bad.scene.world.world.facilities.at(1).kind=12;
        if(fault==7)bad.catalog.erase({2,45});
        if(fault==8)bad.rank=-1;
        if(fault==9)bad.task_progress.successes=-1;
        if(fault==10)bad.events_held=-1;
        const auto unchanged=startup_world_state_digest(bad);
        check(!startup_town_information(bad) && startup_world_state_digest(bad)==unchanged,
              refusals[static_cast<std::size_t>(fault)]);
    }
    auto duplicated=rules;
    duplicated.humans.push_back(duplicated.humans.front());
    state.rules=&duplicated;
    check(!startup_town_information(state),"duplicate human definitions cannot inflate source counts");
}
void catalogue() {
    const auto &rules = startup_world_rules();
    check(rules.humans.size() == 25 && rules.jobs.size() == 23 && rules.equipment.size() == 113 &&
              rules.monsters.size() == 36 && rules.items.size() == 36 && rules.tasks.size() == 81 &&
              rules.facilities.size() == 85,
          "full original catalogue counts compiled, not initial subset");
    check(rules.equipment[0].battle.kind == 0 && rules.equipment[0].battle.range == 130 &&
              rules.equipment[0].battle.combo == 5 && rules.equipment[0].battle.miss_low == 3 &&
              rules.equipment[0].battle.miss_high == 15,
          "actual shortsword complete battle rule");
    check(rules.tasks[0].recruitment_fee == 1300 && rules.tasks[0].crew_rating_penalty == 1500,
          "questData constructor columns6/15 publish fee and display-only rating penalty");
    check(rules.facilities[54].exit_effects.size() == 1 &&
              rules.facilities[54].exit_effects[0].attribute_index == 5 &&
              rules.unconsumed_exit_deltas.at(54) == std::vector<int>{3} &&
              rules.unconsumed_exit_deltas.size() == 4,
          "fixed source longA shortz tail preserved; consumed effect not fabricated");
    check(rules.fences[0][0] == ref::Position{6, 10} && rules.fences[0][1] == ref::Position{17, 2},
          "published raw regions independently flip bothY endpoints");
    check(rules.monsters[0].initial.base_hp == 16 &&
              rules.monsters[0].initial.base_cash_reward == 50 &&
              rules.monsters[0].initial.base_death_reward == 4 &&
              rules.monsters[0].initial.newly_unlocked && rules.monsters[0].initial.introduced &&
              !rules.monsters[3].initial.introduced,
          "monster.b source flags reset distinguishes pending notice and introduced");
}
void reset() {
    StartupSession session;
    const auto random = ref::WorldRandomStream::from_raw({7, 8, 9});
    const auto result = prepare_startup_world_projection(session.state(), random);
    check(result.candidate.has_value(), "actual reset projects without visitor");
    const auto &c = *result.candidate;
    const auto &world = c.routes.world;
    check(world.ai.human_order.empty() && world.ai.monster_order.empty() &&
              world.ai.battle.actors.empty() && world.ai.battle.objects.empty() &&
              world.ai.encounters.empty(),
          "source reset has genuinely empty live roster/object/event, not invented actor");
    check(world.ai.growth.size() == 25 && world.ai.monster_growth.size() == 36 &&
              world.ai.professions.size() == 23 && c.routes.catalog.size() == 149 &&
              c.routes.items.size() == 36 && world.facility_uses.size() == 85,
          "all shared definitions initialized even without current actor or facility");
    check(world.ai.accounting.funds() == 5000 && world.ai.accounting.village_points() == 10 &&
              c.popularity == 50 && c.calendar == std::array<int, 4>{0, 3, 0, 0} &&
              c.arrival_counter == 420 && c.ground_definition == 17 &&
              c.special_ground_definition == 19,
          "published resources/date and real first-kind S/T, not lastkind terrain");
    check(c.human_presence.at(1) == 1 && c.human_presence.at(2) == 1 &&
              c.human_presence.at(3) == 1 && c.human_presence.at(4) == 0 &&
              world.ai.growth.at(1).notice_attributes == std::array<std::array<int, 2>, 4>{},
          "source p1 unlocks and newly allocated ap zeros preserved");
    for (const auto &entry : c.human_homes)
        check(entry.second == std::array<int, 4>{} &&
                  c.routes.human_definition_state.at(entry.first) == 0,
              "all definitions D and m source reset zero, not guessed home");
    check(c.routes.random.draws() == 0 && random.draws() == 0,
          "projection consumes no random and leaves caller stream unchanged");
    for (std::size_t n = 0; n < session.state().loaded_map.instances.size(); ++n) {
        const auto &old = session.state().loaded_map.instances[n];
        const auto identity = static_cast<std::uint64_t>(old.legacy_id) + 1;
        check(c.facility_order[n] == identity &&
                  c.facility_original_ids.at(identity) == old.legacy_id &&
                  c.facility_residents.at(identity) == -1 &&
                  world.facilities.at(identity).placement.anchor == old.anchor &&
                  world.facilities.at(identity).occupants.empty(),
              "original facility order/raw0 identities/t-1 and empty occupations preserved");
    }
    for (std::size_t n = 0; n < session.state().loaded_map.cells.size(); ++n) {
        const auto &old = session.state().loaded_map.cells[n];
        check(c.routes.facts.surface[n] == static_cast<int>(old.category) &&
                  c.routes.facts.flags[n] == 0 &&
                  world.map.cells[n].legacy_state == old.legacy_state &&
                  c.surface[n].definition == old.definition_id &&
                  c.surface[n].instance == old.external_direction &&
                  c.surface[n].variant == old.variant &&
                  c.road_patches[n] == std::array<bool, 2>{old.road_quad, old.edge_road_pair},
              "all576 cells preserve c/i.m external direction separately from facility identity "
              "and visual variants");
    }
}
void first() {
    const auto session = installed();
    const auto result = prepare_startup_world_projection(session.state(), ref::WorldRandomStream{});
    check(result.candidate.has_value(), "source first visitor projects without running another AI");
    const auto &c = *result.candidate;
    const auto &world = c.routes.world;
    const auto &old = *session.state().character;
    const auto &actor = world.ai.battle.actors.at({1});
    const auto &dungeon = c.routes.dungeon_actors.at({1});
    check(dungeon.progress == 0 && dungeon.previous == 0 && dungeon.percent == 0 &&
              dungeon.endurance == 0 && dungeon.retreat_state == 0 &&
              dungeon.retreat_updates == 0 && !dungeon.constrained,
          "new Character dungeon fields preserve Java zero; endurance belongs to actual entry");
    check(actor.definition == 1 && actor.legacy_id == 0 && actor.control.state == 0 &&
              actor.control.flags == (2U | 8192U) &&
              actor.control.queue == std::vector<ref::LegacyActorControl>{{8, 0}} &&
              actor.hp.target == 22 && actor.hp.displayed == 22 && actor.capacity == 22,
          "actual first uid0 flags/queued activity/hp22 on stable1");
    check(actor.position.x == old.cell.x * 100.0F + 50.0F &&
              actor.position.z == old.cell.y * 100.0F + 50.0F &&
              world.ai.contexts.at({1}).cell == old.cell &&
              world.ai.contexts.at({1}).half_cell == ref::Position{old.cell.x * 2 + 1, 1},
          "source n/a initializes world n and cacheds/t, no alternate birth random");
    check(actor.attack_position.x == 0 && actor.attack_position.z == 0 &&
              actor.attack_position.height == 0,
          "unrun first-visitor d preserves constructor au0; n/a only initializes n/s/t/u/v");
    check(!actor.attack_armed && actor.combo_count == 0 && actor.perceived_distance == 0 &&
              !world.ai.contexts.at({1}).inside_town && !world.ai.contexts.at({1}).move_area &&
              world.actors.at({1}).destination == std::optional<ref::Position>({0, 0}) &&
              !world.actors.at({1}).binding && !actor.encounter,
          "unrun c retains Java0 ai/y/aA/ax/aB and allocated O0; no eager perception or binding");
    check(c.routes.shop_humans.at(1).reselect[0] == 6 && c.routes.shop_actors.at({1}).weapon == 0 &&
              world.ai.growth.at(1).derived.combat == old.combat,
          "source initial equip and shared derived stats not inventory substitution");
    check(c.actor_metadata.at({1}).profession == old.job_id && old.job_id == 1 &&
              c.actor_metadata.at({1}).sex == old.sex && c.event89_count == 1 &&
              session.state().mode == StartupMode::tutorial && world.ai.accounting.funds() == 5000,
          "projection preserves installed/tutorial chronology, never advances script/date/world");
    auto corrupt = session.state();
    corrupt.character->combat[0]++;
    check(prepare_startup_world_projection(corrupt, {}).error ==
              StartupWorldProjectionError::invalid_snapshot,
          "corrupt first stats rejects rather than correcting evidence");
    corrupt = session.state();
    corrupt.terrain_edits[0] = 27;
    check(prepare_startup_world_projection(corrupt, {}).error ==
              StartupWorldProjectionError::invalid_snapshot,
          "modified old startup map cannot become a fake canonical full world");
    corrupt = session.state();
    corrupt.loaded_map.cells[0].variant++;
    check(prepare_startup_world_projection(corrupt, {}).error ==
              StartupWorldProjectionError::source_mismatch,
          "modified reset snapshot rejects even with valid dimensions and bindings");
}
} // namespace
int main() {
    try {
        catalogue();
        reset();
        first();
        town_information();
        std::cout << "startup world projection checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
