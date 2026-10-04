#include "dungeon_village_reference/world_dungeon_finish.hpp"

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
// 显式夹具：不是固定APK的新局、页面栈或真实地图刷新器。
DungeonFinishState fixture() {
    DungeonFinishState s;
    s.dungeon.world.map = {6, 6, std::vector<LegacyMapCell>(36)};
    s.surface.resize(36);
    s.ground_definition = 7;
    RescueFacility site;
    site.placement = {{3}, 33, FacilityShape::square, FacilityOrientation::first, {2, 2}};
    site.category = 5;
    site.status = 2;
    site.occupants = {{1}, {1}, {2}};
    auto global = site;
    global.placement = {{4}, 44, FacilityShape::single, FacilityOrientation::first, {5, 5}};
    s.dungeon.world.facilities = {{3, site}, {4, global}};
    s.dungeon.world.map =
        *bind_facility_map(s.dungeon.world.map, {{site.placement, 3}, {global.placement, 3}}).map;
    for (const auto &f : s.dungeon.world.facilities) {
        const auto footprint =
            facility_footprint(f.second.placement.shape, f.second.placement.orientation,
                               f.second.placement.anchor, 6, 6);
        for (const auto &cell : footprint.cells) {
            s.sites[f.first].occupied_cells.push_back(cell.position);
            auto &surface = s.surface[cell.position.y * 6 + cell.position.x];
            surface = {f.second.placement.definition_id,
                       91,
                       static_cast<int>(f.first),
                       cell.fragment_index,
                       90,
                       89,
                       88};
        }
    }
    s.dungeon.facilities[3].updates = 10;
    s.dungeon.facilities[3].challenges = {{0, 0, 0, 0, 0, 99}};
    s.dungeon.facilities[4].challenges = {
        {1, 0, 0, 0, 1, 11}, {2, 0, 3, 80, 2, 22}, {3, 1, 2, 0, 500, 23}};
    s.task_progress.definitions = {{0, {0, 0, 0, 0}}, {1, {1, 0, 0, 0}}, {2, {0, 2, 0, 0}}};
    s.task_progress.task_pool_progress = 275;
    s.tasks = {{10, {10, 0, 3, 9, 4, Position{5, 5}}},
               {11, {11, 0, 1, 0, 3, Position{2, 2}}},
               {12, {12, 0, 1, 0, {}, {}}},
               {13, {13, 1, 1, 0, {}, {}}},
               {14, {14, 0, 1, 0, 99, Position{1, 3}}}};
    s.task_order = {10, 11, 12, 13, 10, 14};
    s.active_task = 10;
    s.participants = {0, 0, 2}; // 重复参与者与当前不存在的人物定义。
    s.human_definition_flags = {{0, 2U | 64U}, {1, 2U}, {2, 2U | 8U}};
    s.dungeon.world.ai.task_active = true;
    s.dungeon.world.ai.pending_completion = 100;
    for (int id = 1; id <= 2; ++id) {
        BattleActorRecord actor;
        actor.id = {static_cast<std::uint64_t>(id)};
        actor.definition = 0;
        actor.control.flags = 2048U | 64U;
        actor.control.queue = {{8, 0}};
        s.dungeon.world.ai.battle.actors.emplace(actor.id, actor);
        s.dungeon.world.ai.human_order.push_back(actor.id);
        s.dungeon.world.actors[actor.id].binding = ArrivalBinding{{2, 2}, {3}, 33};
    }
    s.dungeon.world.ai.growth[0] = RewardHumanDefinition{};
    s.raw_year = 2;
    s.raw_month = 11;
    return s;
}
// 夹具消费器只记录可检查的变化；每种真实消费者依旧由上层显式接入。
std::optional<DungeonFinishState> fixture_consumer(const DungeonFinishState &s,
                                                   const DungeonFinishEffect &effect) {
    auto next = s;
    if (effect.kind == DungeonFinishEffectKind::event)
        ++next.event_calls[effect.first];
    return next;
}
void normal_success() {
    auto s = fixture();
    std::vector<DungeonFinishEffectKind> order;
    int reward_displays{};
    const auto r = prepare_world_dungeon_finish(
        s, {3, {2, 2}}, [&](const DungeonFinishState &candidate, const DungeonFinishEffect &e) {
            order.push_back(e.kind);
            if (e.kind == DungeonFinishEffectKind::actor_reward_display) {
                check(e.actor == CharacterId{1} && e.first == -8 && e.second == 15 &&
                          candidate.dungeon.world.ai.growth.at(0).pending.amount ==
                              reward_displays * 15 &&
                          candidate.dungeon.world.ai.battle.actors.at({1}).control.flags ==
                              (reward_displays ? 64U : (2048U | 64U)),
                      "display precedes its definition reward and boost clear, repeated source "
                      "order");
                ++reward_displays;
            }
            if (e.kind == DungeonFinishEffectKind::event && e.first == 126)
                check(candidate.dungeon.world.ai.pending_completion == 100 &&
                          candidate.active_task == 10 && candidate.sites.count(3),
                      "event126 observes old completion and occupied site before recording");
            if (e.kind == DungeonFinishEffectKind::site_success_display)
                check(
                    candidate.dungeon.world.ai.pending_completion == 109 &&
                        candidate.dungeon.world.map.cells[e.cell->y * 6 + e.cell->x].legacy_state ==
                            4,
                    "each success display follows its own cell restoration and completion");
            if (e.kind == DungeonFinishEffectKind::rebuild_display)
                check(!candidate.dungeon.world.facilities.count(3) && !candidate.sites.count(3) &&
                          !candidate.dungeon.facilities.count(3) && candidate.active_task == 10,
                      "remove instance and occupancy before full map rebuild, task still exists");
            if (e.kind == DungeonFinishEffectKind::threshold_notice)
                check(candidate.task_progress.successes == 1 && candidate.active_task == 10 &&
                          candidate.human_definition_flags.at(1) == 2,
                      "task statistics and threshold precede clearing all definition flags");
            if (e.kind == DungeonFinishEffectKind::event && e.first != 126)
                check(!candidate.active_task && !candidate.dungeon.world.ai.task_active &&
                          candidate.task_progress.successes == 1,
                      "late event follows task clear and successful statistics");
            return fixture_consumer(candidate, e);
        });
    check(r.candidate.has_value(), "normal stage2 accepts explicit fixture consumers");
    const auto &c = *r.candidate;
    check(c.site_restored && c.task_cleared && c.consumed_summary_tickets == 2 &&
              c.state.dungeon.world.ai.pending_completion == 109,
          "success restores whole site and clears task, consumes only two summary draws");
    check(c.state.dungeon.world.ai.growth.at(0).pending.amount == 30 &&
              c.state.dungeon.world.ai.growth.at(0).pending.counter == -20 &&
              c.state.dungeon.world.ai.growth.at(0).experience == 0 &&
              c.state.dungeon.world.ai.battle.actors.at({1}).control.flags == 64U &&
              c.state.dungeon.world.ai.battle.actors.at({2}).control.flags == (2048U | 64U),
          "duplicate participants reward FIRST matching actor twice, delayed XP not immediate");
    check(c.state.participants == s.participants && c.state.tasks.size() == s.tasks.size() &&
              c.state.task_order == std::vector<std::uint64_t>{11, 12, 13, 10, 14} &&
              c.state.human_definition_flags.at(0) == 64U &&
              c.state.human_definition_flags.at(1) == 0 &&
              c.state.human_definition_flags.at(2) == 8U,
          "clear removes first task reference, preserves participants/object refs, clears ALL bv");
    check(c.state.dungeon.world.actors.at({1}).binding->instance_id.value == 3 &&
              c.state.dungeon.world.ai.battle.actors.at({1}).control.queue ==
                  std::vector<LegacyActorControl>{{8, 0}},
          "removed site does not clear stale O, queued activity or unrelated actor state");
    const auto summary = std::find_if(c.effects.begin(), c.effects.end(), [](const auto &e) {
        return e.kind == DungeonFinishEffectKind::summary32;
    });
    check(summary != c.effects.end() && summary->first == 2 && summary->second == 2 &&
              summary->summary_rewards == std::vector<std::array<int, 2>>{{1, 11}, {2, 22}},
          "summary uses global task facility all type0 states, absent same actor allowed");
    for (const auto p : s.sites.at(3).occupied_cells) {
        const auto n = p.y * 6 + p.x;
        const auto &cell = c.state.dungeon.world.map.cells[n];
        const auto &surface = c.state.surface[n];
        check(!cell.facility && cell.legacy_state == 4 && cell.category == RouteCategory::ground &&
                  surface.definition == 7 && surface.updates == 0 && surface.instance == -1 &&
                  surface.fragment == -1,
              "all occupied cells restore exact ground definition/state/counter/binding");
    }
    check(c.state.dungeon.world.facilities.count(4) &&
              c.state.dungeon.world.map.cells[35].facility->instance_id.value == 4,
          "caller site distinct from global task site; never restore summary provider");
    const std::vector<DungeonFinishEffectKind> expected{
        DungeonFinishEffectKind::actor_reward_display,
        DungeonFinishEffectKind::actor_reward_display,
        DungeonFinishEffectKind::summary30,
        DungeonFinishEffectKind::summary32,
        DungeonFinishEffectKind::event,
        DungeonFinishEffectKind::site_success_display,
        DungeonFinishEffectKind::site_success_display,
        DungeonFinishEffectKind::site_success_display,
        DungeonFinishEffectKind::site_success_display,
        DungeonFinishEffectKind::rebuild_display,
        DungeonFinishEffectKind::rebuild_roads_fences,
        DungeonFinishEffectKind::refresh_scene,
        DungeonFinishEffectKind::rebuild_neighbours,
        DungeonFinishEffectKind::threshold_notice,
        DungeonFinishEffectKind::event,
        DungeonFinishEffectKind::event};
    check(order == expected,
          "all effects preserve rewards/pages/script/map/stats/clear/event order");
    check(s.active_task == 10 && s.dungeon.world.facilities.count(3) &&
              s.dungeon.world.ai.growth.at(0).pending.amount == 0 && s.event_calls.empty(),
          "successful preparation never mutates input");
}
void task_branches() {
    auto s = fixture();
    s.active_task.reset();
    auto r = prepare_world_dungeon_finish(s, {3, {}}, fixture_consumer);
    check(r.candidate && r.candidate->site_restored && !r.candidate->task_cleared &&
              r.candidate->state.task_order == std::vector<std::uint64_t>{10, 11, 13, 10} &&
              r.candidate->effects.size() == 4 && r.candidate->consumed_summary_tickets == 0 &&
              r.candidate->state.human_definition_flags == s.human_definition_flags,
          "no active task restores failure ground, reverse removes no-site/same-site tasks only");
    s = fixture();
    s.active_task = 13;
    r = prepare_world_dungeon_finish(s, {3, {}}, {});
    check(r.candidate && r.candidate->task_cleared && !r.candidate->site_restored &&
              r.candidate->effects.empty() && r.candidate->state.sites.count(3) &&
              r.candidate->state.task_progress.successes == 0 &&
              r.candidate->state.dungeon.world.ai.pending_completion == 100,
          "kind1 clears all flags/task only; no callbacks/rewards/site/stats/draws");
    s = fixture();
    s.tasks.at(13).site = Position{1, 1}; // c/k.a(definition,cell)真实kind1仍写e[0/1]。
    s.active_task = 13;
    r = prepare_world_dungeon_finish(s, {3, {}}, {});
    check(r.candidate && r.candidate->task_cleared && !r.candidate->site_restored &&
              r.candidate->effects.empty(),
          "kind1 real map coordinate does not imply exploration facility ownership");
    for (int count = 0; count < 10; ++count) {
        s = fixture();
        s.dungeon.facilities.at(3).updates = count;
        s.active_task = 999;
        r = prepare_world_dungeon_finish(s, {3, {}}, {});
        check(r.candidate && r.candidate->effects.empty() && r.candidate->state.active_task == 999,
              "pre-threshold no-op neither resolves active task nor requires consumers/tickets");
    }
    s = fixture();
    s.active_task.reset();
    // 原a/o按当前格查实例；无绑定直接返回，但仍执行无任务分支的任务移除。
    for (const auto p : s.sites.at(3).occupied_cells)
        s.dungeon.world.map.cells[p.y * 6 + p.x].facility.reset();
    r = prepare_world_dungeon_finish(s, {3, {}}, {});
    check(r.candidate && !r.candidate->site_restored && r.candidate->effects.empty() &&
              r.candidate->state.task_order == std::vector<std::uint64_t>{10, 11, 13, 10} &&
              r.candidate->state.sites.count(3),
          "source cell no current binding means restore no-op, still remove matching tasks");
}
void live_events_and_special_tasks() {
    for (int seen = 0; seen < 4; ++seen) {
        auto s = fixture();
        if (seen & 1)
            s.event_calls[201] = 1;
        if (seen & 2)
            s.event_calls[92] = 2;
        const auto r = prepare_world_dungeon_finish(s, {3, {0, 1}}, fixture_consumer);
        check(r.candidate && r.candidate->state.event_calls.at(201) == 1 &&
                  r.candidate->state.event_calls.at(92) == ((seen & 2) ? 2 : 1),
              "seen201/92 skip only positive live event counts");
    }
    auto s = fixture();
    auto r =
        prepare_world_dungeon_finish(s, {3, {0, 0}}, [](const auto &candidate, const auto &effect) {
            auto next = fixture_consumer(candidate, effect);
            if (effect.kind == DungeonFinishEffectKind::event && effect.first == 126)
                next->event_calls[201] = 1;
            if (effect.kind == DungeonFinishEffectKind::event && effect.first == 92)
                check(next->event_calls.at(201) == 1,
                      "201 sees mutations made inside earlier event126 consumer");
            return next;
        });
    check(r.candidate && r.candidate->effects.back().first == 92 &&
              std::count_if(r.candidate->effects.begin(), r.candidate->effects.end(),
                            [](const auto &e) {
                                return e.kind == DungeonFinishEffectKind::event && e.first == 201;
                            }) == 0,
          "late event guards do not use frozen pre126 seen snapshot");
    s = fixture();
    s.tasks.at(10).definition = 2;
    s.task_progress.exploration_stage = 5;
    r = prepare_world_dungeon_finish(s, {3, {0, 0}}, fixture_consumer);
    check(r.candidate && r.candidate->state.task_progress.task_pool_progress == 275 &&
              r.candidate->state.task_progress.exploration_dates[5] == std::array<int, 2>{2, 11} &&
              r.candidate->state.task_progress.exploration_stage == 5,
          "active flags2 task still in bq blocks G growth; stage5 writes source raw date");
    for (int old_status : {0, 1})
        for (bool old_notice : {false, true}) {
            s = fixture();
            s.task_progress.definitions.at(0).flags = 4;
            s.task_progress.definitions.at(0).monster_definition = 9;
            s.dungeon.world.ai.monster_growth[9].status = old_status;
            s.dungeon.world.ai.monster_growth[9].newly_unlocked = old_notice;
            r = prepare_world_dungeon_finish(s, {3, {0, 0}}, fixture_consumer);
            check(
                r.candidate &&
                    r.candidate->state.dungeon.world.ai.monster_growth.at(9).status == 1 &&
                    r.candidate->state.dungeon.world.ai.monster_growth.at(9).newly_unlocked ==
                        (old_status == 0 || old_notice) &&
                    r.candidate->state.task_progress.monsters.empty() &&
                    r.candidate->state.task_progress.remaining_task_definitions.empty(),
                "flag4 availability copies back canonical monster owner, no duplicate projections");
        }
    s = fixture();
    r = prepare_world_dungeon_finish(s, {3, {0, 0}}, [](const auto &candidate, const auto &effect) {
        auto next = fixture_consumer(candidate, effect);
        if (effect.kind == DungeonFinishEffectKind::event && effect.first == 201)
            next->event_calls[92] = 1;
        return next;
    });
    check(r.candidate && r.candidate->effects.back().first == 201 &&
              r.candidate->state.event_calls.at(92) == 1,
          "event92 guard sees earlier event201 live effects too");
}
void rollback_and_validation() {
    const auto original = fixture();
    const auto good = prepare_world_dungeon_finish(original, {3, {0, 0}}, fixture_consumer);
    check(good.candidate.has_value(), "rollback matrix baseline is valid");
    for (std::size_t fail_at = 0; fail_at < good.candidate->effects.size(); ++fail_at) {
        std::size_t calls{};
        const auto r = prepare_world_dungeon_finish(
            original, {3, {0, 0}}, [&](const auto &candidate, const auto &effect) {
                if (calls++ == fail_at)
                    return std::optional<DungeonFinishState>{};
                return fixture_consumer(candidate, effect);
            });
        check(r.error == DungeonFinishError::consumer_failed && !r.candidate &&
                  original.active_task == 10 && original.sites.count(3) &&
                  original.dungeon.world.ai.growth.at(0).pending.amount == 0 &&
                  original.dungeon.world.ai.pending_completion == 100,
              "every early/late effect failure discards rewards/map/tasks/stats/events candidate");
    }
    check(prepare_world_dungeon_finish(original, {3, {0, 0}}).error ==
              DungeonFinishError::missing_consumer,
          "no default successful display/page/script/map consumer");
    check(prepare_world_dungeon_finish(
              original, {3, {0, 0}},
              [](const auto &s, const auto &) {
                  return std::optional<DungeonFinishState>{s};
              }).error == DungeonFinishError::consumer_failed,
          "event consumer must register aM increment, no-op cannot fake actual event execution");
    check(prepare_world_dungeon_finish(
              original, {3, {0, 0}},
              [](const auto &s, const auto &e) {
                  auto next = fixture_consumer(s, e);
                  next->surface.clear();
                  return next;
              }).error == DungeonFinishError::consumer_failed,
          "invalid consumer map/surface snapshot rejected before following cell access");
    for (int mutation = 0; mutation < 13; ++mutation) {
        auto s = original;
        switch (mutation) {
        case 0:
            s.surface.pop_back();
            break;
        case 1:
            s.sites.at(3).occupied_cells.push_back({2, 2});
            break;
        case 2:
            s.sites.at(3).occupied_cells.pop_back();
            break;
        case 3:
            s.sites.at(3).occupied_cells.push_back({6, 6});
            break;
        case 4:
            s.dungeon.world.map.cells[14].facility->definition_id = 999;
            break;
        case 5:
            s.tasks.at(10).identity = 99;
            break;
        case 6:
            s.tasks.at(10).site.reset();
            break;
        case 7:
            s.task_progress.definitions.erase(0);
            break;
        case 8:
            s.human_definition_flags.erase(2);
            break;
        case 9:
            s.dungeon.world.ai.growth.erase(0);
            break;
        case 10:
            s.dungeon.world.ai.pending_completion = std::numeric_limits<int>::max();
            break;
        case 11:
            s.task_progress.successes = std::numeric_limits<int>::max();
            break;
        case 12:
            s.event_calls[126] = std::numeric_limits<int>::max();
            break;
        }
        check(!prepare_world_dungeon_finish(s, {3, {0, 0}}, fixture_consumer).candidate &&
                  s.active_task == 10 && s.dungeon.world.facilities.count(3),
              "malformed boundary/bindings/reference/catalog/overflow reject whole candidate");
    }
    for (const auto &tickets :
         {std::vector<int>{}, std::vector<int>{0}, std::vector<int>{-1, 0}, std::vector<int>{0, 3}})
        check(!prepare_world_dungeon_finish(original, {3, tickets}, fixture_consumer).candidate,
              "summary requires exactly valid two independent participant-count choices");
}
} // namespace
int main() {
    try {
        normal_success();
        task_branches();
        live_events_and_special_tasks();
        rollback_and_validation();
        std::cout << "world dungeon finish checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
