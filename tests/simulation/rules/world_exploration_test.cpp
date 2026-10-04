#include "ark/simulation/rules/world_exploration.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
std::string read(const std::filesystem::path &file) {
    std::ifstream stream(file, std::ios::binary);
    if (!stream)
        throw std::runtime_error("missing published original script catalog");
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}
WorldScriptCatalog catalog() {
    const auto root = std::filesystem::path(ARK_WORLD_TEST_DATA) / "scripts/original";
    const auto r = parse_world_script_catalog(read(root / "events.txt"), read(root / "talk.txt"),
                                              read(root / "news.txt"));
    check(r.catalog.has_value(), "use real published original126/201/92 programs and text");
    return *r.catalog;
}
std::size_t index(Position p) { return static_cast<std::size_t>(p.y * 7 + p.x); }
// 本测试夹具不代表原新局；但事件来自已发布固定APK原表。
WorldExplorationState fixture() {
    WorldExplorationState s;
    auto &f = s.finish;
    f.dungeon.world.map = {7, 7, std::vector<LegacyMapCell>(49)};
    f.surface.assign(49, {7, 91, -1, 7, 99, 98, 15});
    f.ground_definition = 7;
    s.map.special_ground_definition = 8;
    s.map.definitions = {
        {7, {70, 0, FacilityShape::single, {}}},   {8, {80, 10, FacilityShape::single, {}}},
        {33, {330, 5, FacilityShape::square, {}}}, {44, {440, 5, FacilityShape::single, {}}},
        {55, {550, 3, FacilityShape::single, {}}}, {40, {400, 6, FacilityShape::single, {}}}};
    s.map.base_variants.assign(49, 6);
    s.map.road_quad.assign(49, true);
    s.map.edge_road_pair.assign(49, true);
    s.map.fence_level = 0;
    s.map.fence_levels = {{{{1, 5}, {5, 1}}}};
    std::vector<BoundFacility> bound;
    for (const auto placement :
         {FacilityPlacement{{3}, 33, FacilityShape::square, FacilityOrientation::first, {3, 3}},
          FacilityPlacement{{4}, 44, FacilityShape::single, FacilityOrientation::first, {5, 5}},
          FacilityPlacement{{5}, 55, FacilityShape::single, FacilityOrientation::first, {4, 3}}}) {
        RescueFacility facility;
        facility.placement = placement;
        facility.category = s.map.definitions.at(placement.definition_id).category;
        facility.status = 2;
        f.dungeon.world.facilities[placement.instance_id.value] = facility;
        bound.push_back({placement, 1});
        s.map.facility_order.push_back(placement.instance_id.value);
        s.map.neighbours[placement.instance_id.value].current = {1, 2, 3};
        for (const auto &cell :
             facility_footprint(placement.shape, placement.orientation, placement.anchor, 7, 7)
                 .cells) {
            f.sites[placement.instance_id.value].occupied_cells.push_back(cell.position);
            f.surface[index(cell.position)].definition = placement.definition_id;
            f.surface[index(cell.position)].instance =
                static_cast<int>(placement.instance_id.value);
            f.surface[index(cell.position)].fragment = cell.fragment_index;
        }
    }
    f.dungeon.world.map = *bind_facility_map(f.dungeon.world.map, bound).map;
    f.surface[index({4, 2})].definition = 40;
    f.dungeon.world.map.cells[index({4, 2})] = {3, RouteCategory::road, {}};
    f.dungeon.facilities[3].updates = 10;
    f.dungeon.facilities[4].challenges = {{0, 0, 3, 0, 1, 7}, {0, 1, 0, 0, 500, 9}};
    f.task_progress.definitions = {{0, {0, 0, 0, 0}}, {1, {1, 0, 0, 0}}};
    f.task_progress.task_pool_progress = 275;
    f.tasks = {{10, {10, 0, 3, 9, 4, Position{5, 5}}}, {11, {11, 1, 1, 0, {}, {}}}};
    f.task_order = {10, 11};
    f.active_task = 10;
    f.participants = {0, 0, 2};
    f.human_definition_flags = {{0, 2U | 64U}, {1, 2U}, {2, 2U}};
    f.dungeon.world.ai.task_active = true;
    f.dungeon.world.ai.pending_completion = 100;
    f.dungeon.world.ai.growth[0] = RewardHumanDefinition{};
    BattleActorRecord actor;
    actor.id = {1};
    actor.control.flags = 2048U | 64U;
    f.dungeon.world.ai.battle.actors[{1}] = actor;
    f.dungeon.world.ai.human_order = {{1}};
    f.dungeon.world.ai.contexts[{1}] = RewardActorContext{};
    s.scripts.village_name = "TEST_VILLAGE";
    s.scripts.context = 7;
    WorldScriptPage scene;
    scene.id = 1;
    s.ui.pages = {scene};
    s.scripts.executing_page = 1;
    s.scripts.next_page_id = 2;
    f.raw_year = 2;
    f.raw_month = 11;
    return s;
}
void real_finish_and_continuation(const WorldScriptCatalog &catalog) {
    const auto original = fixture();
    const auto r = prepare_world_exploration_finish(catalog, original, {3, {2, 2}});
    check(r.candidate.has_value(), "real finish joins source scripts, map and pages without stub");
    auto s = r.candidate->state;
    check(r.candidate->site_restored && r.candidate->task_cleared &&
              s.finish.dungeon.world.ai.pending_completion == 109 &&
              s.finish.event_calls.at(126) == 1 && s.finish.event_calls.at(201) == 1 &&
              s.finish.event_calls.at(92) == 1 && s.scripts.continuations.size() == 3 &&
              s.popularity_queue.empty(),
          "finish126 waits before accumulation;201/92 real continuations defer presentation");
    check(s.scripts.continuations[0].remaining_updates == 10 &&
              s.scripts.continuations[1].remaining_updates == 300 &&
              s.scripts.continuations[2].remaining_updates == 20,
          "real original126/201/92 delays are10/300/20 logical updates");
    check(s.finish.dungeon.world.ai.contexts.at({1}).effects.display ==
                  std::vector<ActorEffectRecord>{{24, 8, 0, 15, 0, 0}, {24, 8, 0, 15, 0, 0}} &&
              s.finish.dungeon.world.ai.growth.at(0).pending.amount == 30 &&
              s.finish.dungeon.world.ai.growth.at(0).pending.counter == -20 &&
              s.finish.dungeon.world.ai.growth.at(0).experience == 0,
          "two real cd24 displays and delayed definition XP; no direct XP or render claim");
    check(
        s.ui.pages.size() == 3 && s.ui.pages[0].id == 1 && s.ui.pages[1].legacy_page == 32 &&
            s.ui.pages[2].legacy_page == 30 && s.ui.pages[1].legacy_f == 2 &&
            s.ui.pages[1].legacy_g == 2 &&
            s.ui.summaries.at(s.ui.pages[1].id).rewards ==
                std::vector<std::array<int, 2>>{{1, 7}} &&
            s.ui.pages[2].legacy_f == 15 && s.ui.summaries.at(s.ui.pages[2].id).task == 10 &&
            s.finish.tasks.count(10),
        "page32/30 actual insertion after executing scene reverses stack order, retains aa/q/X/f");
    check(s.ui.ground_displays.size() == 4 && s.ui.ground_displays.front().kind == 10 &&
              s.ui.ground_displays.front().x_offset == 30 &&
              s.ui.ground_displays.front().y_offset == -15 &&
              s.map.facility_order == std::vector<std::uint64_t>{4, 5} &&
              !s.map.neighbours.count(3) && s.map.refresh_pending &&
              s.finish.surface[index({2, 4})].definition == 7 &&
              s.finish.surface[index({2, 4})].display_definition == 70 &&
              s.finish.surface[index({2, 4})].variant == 6 &&
              s.finish.dungeon.world.map.cells[index({1, 5})].legacy_state == 5,
          "whole site restored, actual map c/d/f and removed order/cache; ground display explicit");
    check(s.map.neighbours.at(5).current == std::array<int, 3>{0, 0, 2} &&
              s.map.neighbours.at(5).notices ==
                  std::vector<std::array<int, 2>>{{4, 0}, {5, 0}, {6, 0}} &&
              s.finish.task_progress.successes == 1 &&
              s.finish.task_progress.task_pool_progress == 325 && s.ui.messages.size() == 1 &&
              s.ui.messages[0].state == std::array<int, 3>{29, -100, 80} &&
              s.ui.messages[0].text == "迷之巨大生物觉醒了!",
          "live neighbour notice queues and real29 S message after success before clear");
    check(r.candidate->map_steps ==
              std::vector<WorldMapRefreshStep>{
                  WorldMapRefreshStep::display, WorldMapRefreshStep::roads_fences,
                  WorldMapRefreshStep::scene_refresh, WorldMapRefreshStep::scene_refresh,
                  WorldMapRefreshStep::neighbours},
          "cross-world actual map trace c/d/innerf/outerf/neighbours preserved");
    // 页关闭只是框架状态4，不能将完成量/人气/经验结算提前。
    for (auto &page : s.ui.pages)
        if (page.legacy_page == 30 || page.legacy_page == 32)
            page.lifecycle = 4;
    s.scripts.context = 55;
    for (int tick = 1; tick <= 10; ++tick) {
        const auto stopped = prepare_world_exploration_continuations(catalog, s, false);
        check(stopped.candidate &&
                  stopped.candidate->state.scripts.continuations[0].remaining_updates ==
                      11 - tick &&
                  stopped.candidate->state.finish.dungeon.world.ai.pending_completion == 109,
              "non-admitted scene freezes continuation even when both summary pages marked close");
        const auto advanced = prepare_world_exploration_continuations(catalog, s, true);
        check(advanced.candidate.has_value(), "real admitted continuation update succeeds");
        s = advanced.candidate->state;
        check(s.finish.event_calls.at(126) == 1 && s.finish.event_calls.at(92) == 1,
              "continuation restoration does not duplicateaM event calls");
        if (tick < 10)
            check(s.finish.dungeon.world.ai.pending_completion == 109 &&
                      s.popularity_queue.empty() && s.scripts.context == 55,
                  "126 waits complete10 before instruction22, keeps source context until resume");
    }
    check(s.finish.dungeon.world.ai.pending_completion == 0 &&
              s.popularity_queue == std::vector<std::array<int, 3>>{{10, 109, 1}} &&
              s.scripts.context == 7 && s.scripts.continuations.size() == 2 &&
              s.scripts.continuations[0].remaining_updates == 291 &&
              s.scripts.continuations[1].remaining_updates == 11,
          "126 tenth update transfers sole pending amount into soleI queue without advancing I");
    // MainScene L159在126恢复轮跳过后续201/92，本组合没有额外倍速轮。
    for (int tick = 11; tick <= 21; ++tick) {
        const auto advanced = prepare_world_exploration_continuations(catalog, s, true);
        check(advanced.candidate.has_value(), "92 continues from real original talk72");
        s = advanced.candidate->state;
    }
    check(s.ui.pages.size() == 4 && s.ui.pages[1].source_record == 72 &&
              !s.ui.pages[1].paragraphs.empty() && s.scripts.continuations.size() == 1 &&
              s.popularity_queue[0][0] == 10,
          "92 actualtalk72 at21 after126 L159, no hidden popularity countdown in script segment");
    for (int tick = 22; tick <= 301; ++tick) {
        const auto advanced = prepare_world_exploration_continuations(catalog, s, true);
        check(advanced.candidate.has_value(), "201 maintains real300-update continuation");
        s = advanced.candidate->state;
        if (tick < 301)
            check(s.ui.pages.size() == 4 && s.scripts.continuations.size() == 1,
                  "201 newspaper cannot appear before actual300th admitted update");
    }
    check(s.ui.pages.size() == 6 && s.ui.pages[1].kind == WorldScriptPageKind::newspaper &&
              s.ui.pages[1].source_record == 1 && !s.ui.pages[1].paragraphs.empty() &&
              s.ui.pages[2].kind == WorldScriptPageKind::simple_message &&
              s.scripts.continuations.empty() && s.finish.event_calls.at(201) == 1 &&
              s.popularity_queue == std::vector<std::array<int, 3>>{{10, 109, 1}},
          "201 at301 accounts for126 L159 and inserts actualnews1, no duplicate call or queue "
          "consumer");
    check(original.finish.active_task == 10 && original.ui.pages.size() == 1 &&
              original.finish.dungeon.world.ai.pending_completion == 100 &&
              original.map.facility_order.size() == 3,
          "full success and many continuations never mutate original snapshot");
}
void branches_and_framework(const WorldScriptCatalog &catalog) {
    for (int lifecycle = 0; lifecycle <= 4; ++lifecycle) {
        auto s = fixture();
        s.ui.pages[0].lifecycle = lifecycle;
        const auto r = prepare_world_exploration_finish(catalog, s, {3, {0, 0}});
        check(r.candidate && r.candidate->state.ui.pages[1].legacy_page ==
                                 (lifecycle == 0 || lifecycle == 4 ? 30 : 32),
              "original framework appends after init/closing anchor, inserts after live anchor");
    }
    auto s = fixture();
    s.scripts.page_mutations_locked = true;
    auto r = prepare_world_exploration_finish(catalog, s, {3, {0, 0}});
    check(r.candidate && r.candidate->state.ui.pages.size() == 1 &&
              r.candidate->state.ui.summaries.empty() &&
              r.candidate->state.scripts.next_page_id == 4 &&
              r.candidate->state.scripts.continuations.size() == 3 && r.candidate->task_cleared,
          "framework locked suppresses insertion only, real scripts/rewards/task cleanup continue");
    s = fixture();
    s.finish.active_task = 11;
    r = prepare_world_exploration_finish(catalog, s, {3, {}});
    check(r.candidate && r.candidate->task_cleared && !r.candidate->site_restored &&
              r.candidate->state.ui.pages.size() == 1 &&
              r.candidate->state.scripts.continuations.empty() &&
              r.candidate->state.map.facility_order.size() == 3,
          "kind1 still only clears task, never map/UI/script/reward pseudo completion");
    s = fixture();
    s.finish.active_task.reset();
    s.finish.dungeon.world.ai.task_active = false;
    r = prepare_world_exploration_finish(catalog, s, {3, {}});
    check(r.candidate && r.candidate->site_restored && !r.candidate->task_cleared &&
              r.candidate->state.ui.ground_displays.empty() &&
              r.candidate->state.map.neighbours.at(5).current == std::array<int, 3>{0, 0, 2} &&
              r.candidate->state.map.neighbours.at(5).notices.empty() &&
              r.candidate->state.scripts.continuations.empty(),
          "no task restores false, real map and neighbours change withoutsuccess display/notices");
}
void late_rollback(const WorldScriptCatalog &source) {
    for (int mutation = 0; mutation < 11; ++mutation) {
        auto s = fixture();
        auto catalog = source;
        switch (mutation) {
        case 0:
            s.map.fence_level = 99;
            break;
        case 1:
            s.map.definitions.erase(55);
            break;
        case 2:
            s.map.neighbours.erase(5);
            break;
        case 3:
            s.map.definitions.at(55).modifiers = {{9, 1}};
            break;
        case 4:
            catalog.events.at(201).commands = {{999}};
            break;
        case 5:
            catalog.events.erase(92);
            break;
        case 6:
            s.scripts.next_page_id = std::numeric_limits<std::uint64_t>::max();
            break;
        case 7:
            s.finish.dungeon.world.ai.contexts.erase({1});
            break;
        case 8:
            s.map.facility_order.push_back(999);
            break;
        case 9:
            s.finish.task_progress.successes = std::numeric_limits<int>::max();
            break;
        case 10:
            s.finish.dungeon.world.ai.task_active = false;
            break;
        }
        const auto r = prepare_world_exploration_finish(catalog, s, {3, {0, 0}});
        check(
            !r.candidate && r.error != WorldExplorationError::none && s.finish.active_task == 10 &&
                s.finish.dungeon.world.facilities.count(3) && s.ui.pages.size() == 1 &&
                s.scripts.continuations.empty() &&
                s.finish.dungeon.world.ai.pending_completion == 100 && !s.map.refresh_pending,
            "late realmap/notice/script/page/stats failures discard entire owner including extras");
    }
    auto s = fixture();
    auto finished = prepare_world_exploration_finish(source, s, {3, {0, 0}});
    s = finished.candidate->state;
    auto broken = source;
    broken.events.at(126).commands[1] = {999};
    const auto r = prepare_world_exploration_continuations(broken, s, true);
    check(!r.candidate && r.script_error == WorldScriptError::unsupported_opcode &&
              s.scripts.continuations[0].remaining_updates == 10 &&
              s.finish.dungeon.world.ai.pending_completion == 109 && s.popularity_queue.empty(),
          "unsupported future continuation opcode rejected without losing sole completion amount");
}
} // namespace
int main() {
    try {
        const auto source = catalog();
        real_finish_and_continuation(source);
        branches_and_framework(source);
        late_rollback(source);
        std::cout << "world exploration checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
