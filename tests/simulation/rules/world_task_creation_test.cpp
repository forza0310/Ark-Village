#include "ark/simulation/rules/world_task_creation.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation::rules;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
// 显式非原新局夹具；所有目录/边界/初值由测试输入，不用于启动程序。
WorldTaskCreationState fixture(std::vector<std::int32_t> tape = std::vector<std::int32_t>(300)) {
    WorldTaskCreationState s;
    s.random = WorldRandomStream::from_raw(std::move(tape));
    s.rank = 1;
    s.finish.dungeon.world.map = {12, 12, std::vector<LegacyMapCell>(144)};
    s.finish.surface.assign(144, {7, 0, -1, -1, 70, 0, 0});
    s.finish.ground_definition = 7;
    s.special_ground_definition = 8;
    s.base_variants.assign(144, 2);
    s.generation_bounds = {{{0, 12}, {12, 0}}};
    s.fence_level = 0;
    s.fence_levels = {{{{4, 7}, {7, 4}}}};
    s.definitions = {{1, 0, 0, 0, 0, 10, {12}, 2, 2}, {2, 0, 0, 8, 0, 10, {12}, 2, 2},
                     {3, 1, 0, 0, 0, 10, {}, 0, 0},   {4, 1, 1, 0, 0, 11, {}, 0, 0},
                     {5, 1, 0, 2, 0, 10, {}, 0, 0},   {6, 1, 1, 2, 0, 11, {}, 0, 0}};
    s.monsters = {{10, {0, true, 21}}, {11, {0, true, 22}}};
    s.challenge_monsters[0] = {10};
    s.challenge_monsters[1] = {10};
    s.challenge_monsters[2] = {10};
    s.challenge_monsters[3] = {10};
    s.challenge_monsters[4] = {10};
    s.facility_definitions[7].map = {70, 0, FacilityShape::single, {}};
    s.facility_definitions[8].map = {80, 10, FacilityShape::single, {}};
    s.facility_definitions[12].map = {120, 12, FacilityShape::single, {}};
    s.facility_definitions[12].kind = 12;
    s.facility_definitions[12].category = 5;
    s.first_reward_weapon = 99;
    s.rewards = {{0, 20, 1, 2, 1}, {0, 21, 2, 2, 0}, {1, 30, 1, 2, 0},
                 {1, 31, 2, 2, 0}, {2, 40, 1, 2, 2}, {3, 50, 1, 2, 1}};
    return s;
}
void ordinary() {
    const auto s = fixture();
    const auto r = prepare_world_task_creation(s, 0);
    check(r.candidate.has_value(), "ordinary task factory succeeds");
    const auto &c = *r.candidate;
    check(c.selected_definition == 1 && c.difficulty == 1 && c.created_task == 1,
          "ordinary pool and rank difficulty create stable task");
    check(c.random_bounds == std::vector<int>({100, 1, 30, 12, 12, 1, 1, 100, 2, 10, 100, 1, 10}),
          "source lazy draws include final97 discarded jitter");
    check(c.state.finish.tasks.at(1).facility == 1 &&
              c.state.finish.tasks.at(1).site == std::optional<Position>({0, 0}) &&
              c.state.finish.task_order == std::vector<std::uint64_t>{1} &&
              c.state.task_original_ids.at(1) == 1 && c.state.task_sequence == 1,
          "task identity and actual facility site committed together");
    const auto &world = c.state.finish.dungeon.world;
    check(world.facilities.at(1).category == 5 && world.facilities.at(1).kind == 12 &&
              world.facilities.at(1).status == 1 && world.map.cells[0].legacy_state == 1 &&
              world.map.cells[0].facility->instance_id.value == 1 &&
              world.map.cells[0].category == RouteCategory::terminal,
          "real exploration site writes state1,g0,binding");
    check(c.state.facility_original_ids.at(1) == 0 && c.state.facility_ordinals.at(1) == 0 &&
              c.state.facility_residents.at(1) == -1 && c.state.facility_difficulties.at(1) == 1,
          "first free original facility0 ordinal0 difficulty and t-1");
    check(c.state.finish.sites.at(1).occupied_cells == std::vector<Position>{{0, 0}} &&
              c.state.finish.dungeon.facilities.at(1).challenges ==
                  std::vector<DungeonChallenge>{{5, 1, 0, 0, 10, 21}, {97, 0, 0, 0, 1, 99}},
          "separate reward draw and monster substitution; first finalweapon");
    check(c.state.refresh_pending &&
              c.map_steps == std::vector<WorldMapRefreshStep>{WorldMapRefreshStep::display,
                                                              WorldMapRefreshStep::roads_fences,
                                                              WorldMapRefreshStep::scene_refresh} &&
              c.state.road_patches.size() == 144 &&
              c.state.finish.dungeon.facilities.at(1).extent == 0,
          "site c/d scene refresh not neighbour or premature exploration extent");
    check(s.random.draws() == 0 && s.finish.tasks.empty() &&
              s.finish.dungeon.world.facilities.empty(),
          "input remains unchanged");
}
void rare_and_no_site() {
    auto s = fixture();
    s.finish.task_progress.ordinary_explorations = 2;
    auto r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->selected_definition == 2 && r.candidate->difficulty == 2 &&
              r.candidate->state.finish.task_progress.ordinary_explorations == 0 &&
              r.candidate->random_bounds[1] == 30,
          "rare immediately returns and clears w; no ordinary-pool ticket");
    for (auto &cell : s.finish.dungeon.world.map.cells)
        cell.legacy_state = 5;
    r = prepare_world_task_creation(s, 0);
    check(r.candidate && !r.candidate->created_task && r.candidate->random_bounds.size() == 42 &&
              r.candidate->state.finish.task_progress.ordinary_explorations == 0 &&
              r.candidate->state.random.draws() == 42 &&
              r.candidate->state.finish.dungeon.world.facilities.empty(),
          "20 denied x/y attempts preserve earlier rarity mutation and consumed stream");
    s = fixture();
    s.generation_bounds = {{{5, 6}, {6, 5}}};
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && !r.candidate->created_task && r.candidate->random_bounds.size() == 43,
          "h.b excludes strict current town interior");
    s.generation_bounds = {{{4, 6}, {5, 5}}};
    r = prepare_world_task_creation(s, 1);
    check(r.candidate &&
              r.candidate->state.finish.tasks.at(1).site == std::optional<Position>({4, 5}),
          "town edge is admitted; random rectangle upper bounds excluded");
}
void combat_selection() {
    auto s = fixture();
    auto r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->selected_definition == 3 &&
              r.candidate->random_bounds == std::vector<int>{100, 1, 30, 12, 12} &&
              !r.candidate->state.finish.tasks.at(1).facility &&
              r.candidate->state.finish.dungeon.world.facilities.empty(),
          "normal kind1 fallback uses equal exploration stage and no facility");
    s.finish.event_calls[60] = 1;
    s.replay_order = {3, 4};
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->selected_definition == 3,
          "replay ignores future stage despite reverse order");
    s.finish.task_progress.exploration_stage = 1;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->selected_definition == 4,
          "replay selects reverse original order");
    s.special_selection_mode = 1;
    s.special_selection = {3, 4};
    s.special_selection_index = 1;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->selected_definition == 4 &&
              r.candidate->random_bounds == std::vector<int>{30, 12, 12},
          "special u[w] skips definition gate and pool draws");
    s = fixture();
    s.finish.task_progress.task_pool_progress = 500;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->selected_definition == 5 &&
              r.candidate->random_bounds == std::vector<int>{30, 12, 12},
          "G500 fresh current special monster selected without randomness");
    s.monsters[10].victories = 3;
    s.monsters[11].victories = 5;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->selected_definition == 5 &&
              r.candidate->state.monsters.at(11).victories == 3 && s.monsters.at(11).victories == 5,
          "G500 clamps all specials to first value, strict-less first tie, private mutation");
    s.monsters[11].victories = 1;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->selected_definition == 6,
          "G500 minimum after original clamp may select other exploration stage");
}
void rewards_and_difficulty() {
    auto s = fixture();
    s.humans = {{1, 30}, {2, 0}, {0, 10000}};
    auto r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->difficulty == 1,
          "all nonzero definitions knowledge mean uses integer division; p0 ignored");
    s.rank = 5;
    s.finish.task_progress.exploration_stage = 5;
    s.definitions[0].exploration_stage = 5;
    s.definitions[1].exploration_stage = 5;
    s.humans = {{1, 30}};
    r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->difficulty == 9,
          "rank exploration knowledge difficulty clamps1..9");
    s = fixture();
    s.finish.task_progress.successes = 1;
    std::vector<std::int32_t> tape(300, 0);
    // after ordinary first reward: rare gate0→equipment31; final jitter still consumed。
    s.random = WorldRandomStream::from_raw(tape);
    r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->state.finish.dungeon.facilities.at(1).challenges.back() ==
                             DungeonChallenge{97, 0, 0, 0, 1, 31},
          "later task last rare equipment chosen from difficulty+1");
    s.rewards.erase(s.rewards.begin() + 3);
    r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->state.finish.dungeon.facilities.at(1).challenges.back() ==
                             DungeonChallenge{97, 0, 0, 0, 0, 21},
          "rare equipment empty still consumes100 then falls to rare item");
    s = fixture();
    s.definitions[0].completions = 1;
    r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->random_bounds[9] == 100 &&
              r.candidate->state.finish.dungeon.facilities.at(1).challenges.front()[1] == 1,
          "completed nonrare first chance0 makes monster chance100");
    s.definitions[0].minimum_rewards = s.definitions[0].maximum_rewards = 1;
    r = prepare_world_task_creation(s, 0);
    check(r.candidate &&
              r.candidate->state.finish.dungeon.facilities.at(1).challenges ==
                  std::vector<DungeonChallenge>{{97, 0, 0, 0, 1, 99}} &&
              r.candidate->random_bounds.back() == 10,
          "one record skips monster gate but still draws chance and final jitter");
}
void ids_and_failures() {
    auto s = fixture();
    s.finish.tasks.emplace(7, DungeonFinishTask{7, 3, 1, 0, {}, Position{11, 11}});
    s.finish.task_order = {7, 7};
    s.task_original_ids[7] = 1;
    auto r = prepare_world_task_creation(s, 1);
    check(r.candidate &&
              r.candidate->state.finish.task_order == std::vector<std::uint64_t>{7, 7, 1} &&
              r.candidate->state.task_original_ids.at(1) == 2,
          "raw task collision skipped and duplicate source prefix preserved");
    s.task_sequence = std::numeric_limits<int>::max() - 1;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->state.task_original_ids.at(1) == 0,
          "source modulo max produces valid raw task0 independently of stable1");
    s = fixture({0, 0, 0});
    r = prepare_world_task_creation(s, 0);
    check(r.error == TaskCreationError::random_failed && !r.candidate && s.random.draws() == 0,
          "exhausted stream rolls back whole private factory");
    s = fixture();
    s.facility_definitions.erase(8);
    r = prepare_world_task_creation(s, 0);
    check(r.error == TaskCreationError::map_failed && !r.candidate &&
              s.finish.dungeon.world.facilities.empty() && s.random.draws() == 0,
          "late map rebuild failure cannot leave bound site or random changes");
    s = fixture();
    s.challenge_monsters[0].clear();
    r = prepare_world_task_creation(s, 0);
    check(r.error == TaskCreationError::empty_pool && !r.candidate,
          "required real monster pool cannot silently become reward");
    s = fixture();
    s.finish.tasks.emplace(1, DungeonFinishTask{1, 3, 1, 0, {}, Position{3, 3}});
    s.finish.task_order = {1};
    s.task_original_ids[1] = 1;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && !r.candidate->created_task,
          "task spacing includes diagonal distance3 and consumes20 trials");
    s = fixture();
    RewardEncounter event;
    event.runtime.center = {3, 3};
    s.finish.dungeon.world.ai.encounters[1] = event;
    s.finish.dungeon.world.ai.encounter_order = {1};
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && !r.candidate->created_task,
          "event spacing also includes diagonal distance3");
    check(prepare_world_task_creation(fixture(), 2).error == TaskCreationError::invalid_input,
          "unsupported kind explicitly rejects");
}
void stable_lifetimes() {
    const auto first = prepare_world_task_creation(fixture(), 0);
    check(first.candidate && first.candidate->state.next_facility_identity == 2 &&
              first.candidate->state.next_task_identity == 2,
          "maintenance stable allocators advance independently of source counters");
    auto retired = first.candidate->state;
    // 显式模拟原阶段2移除g/bq，但成果页仍持有旧task/设施对象引用。
    const auto reset = fixture();
    retired.finish.dungeon.world.map = reset.finish.dungeon.world.map;
    retired.finish.surface = reset.finish.surface;
    retired.finish.dungeon.world.facilities.clear();
    retired.finish.dungeon.facilities.clear();
    retired.facility_order.clear();
    retired.finish.task_order.clear();
    const auto second = prepare_world_task_creation(retired, 0);
    check(second.candidate && second.candidate->created_task == 2 &&
              second.candidate->state.finish.tasks.at(2).facility == 2 &&
              second.candidate->state.finish.tasks.at(1).facility == 1 &&
              second.candidate->state.facility_original_ids.at(2) == 0 &&
              second.candidate->state.facility_ordinals.at(2) == 0,
          "raw/ordinal0 may reuse, stable2 never aliases retired task1 facility1");
    retired.next_facility_identity = std::numeric_limits<std::uint64_t>::max();
    const auto failure = prepare_world_task_creation(retired, 0);
    check(failure.error == TaskCreationError::numeric_overflow && !failure.candidate &&
              retired.finish.tasks.at(1).facility == 1 &&
              retired.finish.dungeon.world.facilities.empty(),
          "allocator exhaustion refuses whole task without altering retired references");
}
void source_exploration_kinds() {
    for (const auto pair : std::array<std::array<int, 2>, 3>{{{{1, 8}}, {{8, 9}}, {{9, 10}}}}) {
        auto s = fixture();
        s.facility_definitions.at(12).kind = pair[0];
        s.facility_definitions.at(12).map.category = pair[0];
        const auto result = prepare_world_task_creation(s, 0);
        check(
            result.candidate && result.candidate->created_task &&
                result.candidate->state.finish.dungeon.world.map.cells.front().legacy_state ==
                    pair[1] &&
                result.candidate->state.finish.dungeon.world.map.cells.front().category ==
                    RouteCategory::terminal,
            "original cave/fort/tasksite e1/8/9 use actual state8/9/10 and g0, not uniformstate1");
    }
}
void precise_random_branches() {
    auto s = fixture();
    s.finish.task_progress.successes = 1;
    s.rewards = {{0, 20, 1, 2, 0}, {0, 21, 1, 2, 0}, {1, 30, 1, 2, 0}};
    s.definitions[0].minimum_rewards = s.definitions[0].maximum_rewards = 3;
    auto r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->random_bounds ==
                             std::vector<int>{100, 1,   30, 12, 12,  1, 1,  100, 1, 100, 2,
                                              100, 100, 2,  10, 100, 1, 10, 100, 1, 10},
          "equipment removal reduces pool; unavailable last rare still draws both gates");
    check(r.candidate->state.finish.dungeon.facilities.at(1).challenges.back()[4] == 0 &&
              r.candidate->state.finish.dungeon.facilities.at(1).challenges.back()[5] == 20,
          "ordinary items may repeat after unique equipment depleted");
    s = fixture();
    s.definitions[0].completions = 1;
    std::vector<std::int32_t> tape(100, 99);
    tape[0] = tape[1] = tape[2] = tape[3] = tape[4] = tape[5] = tape[6] = 0;
    // first ordinary gate99→item0; completed chance5→not100, second0→chance0。
    tape[7] = 99;
    tape[8] = 0;
    tape[9] = 5;
    tape[10] = 0;
    tape[11] = 0;
    tape[12] = 0;
    tape[13] = 0;
    s.random = WorldRandomStream::from_raw(tape);
    r = prepare_world_task_creation(s, 0);
    check(r.candidate &&
              r.candidate->random_bounds ==
                  std::vector<int>{100, 1, 30, 12, 12, 1, 1, 100, 1, 100, 100, 10, 100, 10} &&
              r.candidate->state.finish.dungeon.facilities.at(1).challenges.front() ==
                  DungeonChallenge{5, 0, 0, 0, 0, 20},
          "completed second5-percent branch makes0; still draws actual nonlast monster gate");
    s = fixture();
    s.task_sequence = std::numeric_limits<int>::max();
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->state.task_original_ids.at(1) == -1,
          "source Java overflow then signed modulo retained separately from stable identity");
    s = fixture();
    s.finish.event_calls[60] = 1;
    s.replay_order.assign(9, 3); // 实际向量可重复，不能按定义去重。
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->random_bounds[1] == 6,
          "reverse replay at most6 retains repeated object references");
    s.monsters[10].replay_available = false;
    r = prepare_world_task_creation(s, 1);
    check(r.candidate && r.candidate->random_bounds[1] == 1 &&
              r.candidate->selected_definition == 3,
          "event60 true but replay pool empty falls to same-stage ordinary pool");
    s = fixture();
    s.definitions[0].minimum_rewards = s.definitions[0].maximum_rewards = 1;
    for (std::size_t length = 0; length < 8; ++length) {
        s.random = WorldRandomStream::from_raw(std::vector<std::int32_t>(length, 0));
        r = prepare_world_task_creation(s, 0);
        check(r.error == TaskCreationError::random_failed && !r.candidate &&
                  s.random.draws() == 0 && s.finish.dungeon.world.facilities.empty(),
              "every incomplete prefix including late final jitter rolls back site and stream");
    }
    s = fixture();
    s.finish.surface[0].updates = 19;
    for (const auto entry : {std::array<int, 4>{10, 2, 2, 1}, std::array<int, 4>{20, 0, 0, 3}}) {
        RescueFacility existing;
        existing.placement = {{static_cast<std::uint64_t>(entry[0])},
                              12,
                              FacilityShape::single,
                              FacilityOrientation::first,
                              {10, entry[3]}};
        existing.kind = 12;
        existing.category = 5;
        s.finish.dungeon.world.facilities.emplace(entry[0], existing);
        s.facility_order.push_back(entry[0]);
        s.facility_original_ids[entry[0]] = entry[1];
        s.facility_ordinals[entry[0]] = entry[2];
        const auto n = static_cast<std::size_t>(entry[3] * 12 + 10);
        s.finish.surface[n].definition = 12;
        auto &cell = s.finish.dungeon.world.map.cells[n];
        cell.legacy_state = 1;
        cell.category = RouteCategory::terminal;
        cell.facility = FacilityTileBinding{{static_cast<std::uint64_t>(entry[0])}, 12, 0};
    }
    r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->state.facility_original_ids.at(1) == 1 &&
              r.candidate->state.facility_ordinals.at(1) == 1 &&
              r.candidate->state.facility_order == std::vector<std::uint64_t>{10, 20, 1} &&
              r.candidate->state.finish.surface[0].updates == 0,
          "source first-free allocator fills raw/ordinal gaps; site state setter resetsf");
    s = fixture();
    s.facility_definitions[12].map.shape = FacilityShape::pair;
    // a/o.ai的orientation0按(0,1)→(0,0)绑定，不能按屏幕方向猜占地。
    r = prepare_world_task_creation(s, 0);
    check(r.candidate && r.candidate->state.finish.sites.at(1).occupied_cells.size() == 2 &&
              r.candidate->state.finish.dungeon.world.map.cells[12].facility &&
              r.candidate->state.finish.dungeon.world.map.cells[12].facility->instance_id.value ==
                  1,
          "selected definition shape binds full actual footprint");
}
} // namespace
int main() {
    try {
        ordinary();
        rare_and_no_site();
        combat_selection();
        rewards_and_difficulty();
        ids_and_failures();
        stable_lifetimes();
        source_exploration_kinds();
        precise_random_branches();
        std::cout << "world task creation checks: " << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
