#include "dungeon_village_reference/world_task_creation.hpp"

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>

namespace dungeon_village_reference {
namespace {
int integer(std::int64_t value) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        throw TaskCreationError::numeric_overflow;
    return static_cast<int>(value);
}
int interpolate(int value, int maximum, int first, int last) {
    return first + (std::int64_t(std::clamp(value, 0, maximum)) * (last - first)) / maximum;
}
int draw(TaskCreationCandidate &c, int bound) {
    if (bound <= 0)
        throw TaskCreationError::empty_pool;
    const auto r = c.state.random.draw(bound);
    if (r.error != WorldRandomError::none)
        throw TaskCreationError::random_failed;
    c.random_bounds.push_back(bound);
    return r.ticket;
}
const TaskCreationDefinition &definition(const WorldTaskCreationState &s, int id) {
    const auto found = std::find_if(s.definitions.begin(), s.definitions.end(),
                                    [id](const auto &d) { return d.identity == id; });
    if (found == s.definitions.end())
        throw TaskCreationError::missing_definition;
    return *found;
}
TaskCreationMonster &monster(WorldTaskCreationState &s, int id) {
    const auto found = s.monsters.find(id);
    if (found == s.monsters.end())
        throw TaskCreationError::missing_definition;
    return found->second;
}
const TaskCreationDefinition &select(TaskCreationCandidate &c, int kind) {
    auto &s = c.state;
    const int stage = s.finish.task_progress.exploration_stage;
    if (s.special_selection_mode == 1 && kind == 1) {
        if (s.special_selection_index < 0 ||
            static_cast<std::size_t>(s.special_selection_index) >= s.special_selection.size())
            throw TaskCreationError::invalid_input;
        return definition(s, s.special_selection[s.special_selection_index]);
    }
    if (s.finish.task_progress.task_pool_progress >= 500 && kind == 1) {
        const TaskCreationDefinition *current = nullptr;
        std::vector<int> special;
        for (const auto &d : s.definitions)
            if (d.flags & 2U) {
                special.push_back(d.identity);
                if (d.exploration_stage == stage)
                    current = &d; // 原正序扫描保留最后一个。
            }
        if (!current || special.empty())
            throw TaskCreationError::empty_pool;
        if (monster(s, current->monster).victories <= 0)
            return *current;
        const int cap = monster(s, definition(s, special.front()).monster).victories;
        for (const int id : special) {
            auto &m = monster(s, definition(s, id).monster);
            m.victories = std::min(m.victories, cap);
            if (auto growth =
                    s.finish.dungeon.world.ai.monster_growth.find(definition(s, id).monster);
                growth != s.finish.dungeon.world.ai.monster_growth.end())
                growth->second.growth = m.victories;
        }
        const TaskCreationDefinition *best = &definition(s, special.front());
        for (const int id : special)
            if (monster(s, definition(s, id).monster).victories <
                monster(s, best->monster).victories)
                best = &definition(s, id);
        return *best;
    }
    std::vector<int> pool;
    if (kind == 0) {
        constexpr std::array<int, 6> rare_chances{0, 0, 10, 20, 30, 100};
        const int count = s.finish.task_progress.ordinary_explorations;
        if (count < 0)
            throw TaskCreationError::invalid_input;
        const bool rare = draw(c, 100) < rare_chances[std::min(count, 5)];
        for (const auto &d : s.definitions)
            if (d.exploration_stage == stage && d.kind == kind && !(d.flags & 2U)) {
                if (!(d.flags & 8U))
                    pool.push_back(d.identity);
                else if (rare) {
                    s.finish.task_progress.ordinary_explorations = 0;
                    return d; // UserData低层L133前立即return，不能继续抽普通目录。
                }
            }
    } else {
        const bool replay =
            draw(c, 100) < 40 && s.finish.event_calls.count(60) && s.finish.event_calls.at(60) > 0;
        if (replay) {
            for (auto it = s.replay_order.rbegin(); it != s.replay_order.rend(); ++it) {
                const auto &d = definition(s, *it);
                if (d.exploration_stage <= stage && monster(s, d.monster).replay_available) {
                    pool.push_back(d.identity);
                    if (pool.size() >= 6)
                        break;
                }
            }
        }
        if (pool.empty())
            for (const auto &d : s.definitions)
                if (d.exploration_stage == stage && d.kind == kind && !(d.flags & (2U | 4U)))
                    pool.push_back(d.identity);
    }
    return definition(s, pool.at(draw(c, integer(pool.size()))));
}
bool nearby(Position a, Position b) {
    return std::abs(std::int64_t(a.x) - b.x) <= 3 && std::abs(std::int64_t(a.y) - b.y) <= 3;
}
std::optional<Position> site(TaskCreationCandidate &c) {
    const auto &s = c.state;
    const auto &map = s.finish.dungeon.world.map;
    const auto bounds = s.generation_bounds;
    const auto town = s.fence_levels.at(s.fence_level);
    const int width = integer(std::int64_t(bounds[1].x) - bounds[0].x);
    const int height = integer(std::int64_t(bounds[0].y) - bounds[1].y);
    for (int attempt = 0; attempt < 20; ++attempt) {
        const Position p{integer(std::int64_t(bounds[0].x) + draw(c, width)),
                         integer(std::int64_t(bounds[1].y) + draw(c, height))};
        if (p.x < 0 || p.y < 0 || p.x >= map.width || p.y >= map.height ||
            (p.x > town[0].x && p.x < town[1].x && p.y > town[1].y && p.y < town[0].y) ||
            map.cells[static_cast<std::size_t>(p.y) * map.width + p.x].legacy_state != 4)
            continue;
        bool admitted = true;
        for (const auto id : s.finish.task_order) {
            const auto &task = s.finish.tasks.at(id);
            if (!task.site)
                throw TaskCreationError::invalid_input;
            if (nearby(p, *task.site)) {
                admitted = false;
                break;
            }
        }
        if (admitted)
            for (const auto id : s.finish.dungeon.world.ai.encounter_order)
                if (nearby(p, s.finish.dungeon.world.ai.encounters.at(id).runtime.center)) {
                    admitted = false;
                    break;
                }
        if (admitted)
            return p;
    }
    return {};
}
std::uint64_t stable_id(const std::set<std::uint64_t> &ids, std::uint64_t &next) {
    if (!next)
        throw TaskCreationError::invalid_input;
    std::uint64_t id = next;
    while (ids.count(id)) {
        if (id == std::numeric_limits<std::uint64_t>::max())
            throw TaskCreationError::numeric_overflow;
        ++id;
    }
    if (id == std::numeric_limits<std::uint64_t>::max())
        throw TaskCreationError::numeric_overflow;
    next = id + 1;
    return id;
}
int first_free(const std::set<int> &ids) {
    int id = 0;
    while (ids.count(id))
        id = integer(std::int64_t(id) + 1);
    return id;
}
std::uint64_t create_facility(TaskCreationCandidate &c, int id, Position p) {
    auto &s = c.state;
    auto &world = s.finish.dungeon.world;
    const auto info = s.facility_definitions.find(id);
    if (info == s.facility_definitions.end())
        throw TaskCreationError::missing_definition;
    const auto &d = info->second;
    if (d.category != 5 ||
        (d.kind != 1 && d.kind != 3 && d.kind != 8 && d.kind != 9 && d.kind != 12 && d.kind != 13))
        throw TaskCreationError::invalid_input;
    std::set<std::uint64_t> identities;
    std::set<int> raw_ids, ordinals;
    for (const auto existing : s.facility_order) {
        const auto found = world.facilities.find(existing);
        if (found == world.facilities.end() || !s.facility_original_ids.count(existing) ||
            !s.facility_ordinals.count(existing))
            throw TaskCreationError::invalid_input;
        identities.insert(existing);
        raw_ids.insert(s.facility_original_ids.at(existing));
        if (found->second.placement.definition_id == id)
            ordinals.insert(s.facility_ordinals.at(existing));
    }
    for (const auto &existing : world.facilities)
        identities.insert(existing.first);
    const auto identity = stable_id(identities, s.next_facility_identity);
    const FacilityPlacement placement{{identity}, id, d.map.shape, FacilityOrientation::first, p};
    const auto footprint = facility_footprint(d.map.shape, FacilityOrientation::first, p,
                                              world.map.width, world.map.height);
    if (footprint.error != GeometryError::none)
        throw TaskCreationError::map_failed;
    RescueFacility f;
    f.placement = placement;
    f.kind = d.kind;
    f.category = d.category;
    f.detail = d.detail;
    f.price = d.price;
    f.definition_wait = d.wait;
    f.upgrade_uses = d.upgrade_uses;
    f.status = 1;
    world.facilities.emplace(identity, f);
    s.finish.dungeon.facilities.emplace(identity, DungeonFacilityProgress{});
    s.facility_original_ids.emplace(identity, first_free(raw_ids));
    s.facility_ordinals.emplace(identity, first_free(ordinals));
    s.facility_residents.emplace(identity, -1);
    s.facility_order.push_back(identity);
    DungeonFinishSite occupied;
    for (const auto &fragment : footprint.cells) {
        const auto n =
            static_cast<std::size_t>(fragment.position.y) * world.map.width + fragment.position.x;
        auto &cell = world.map.cells[n];
        cell.facility = FacilityTileBinding{{identity}, id, fragment.fragment_index};
        // 原c/i.a(m,definition,fragment)：e1/8/9分别state8/9/10，e3/12/13为1；g全0。
        cell.legacy_state = d.kind == 1 ? 8 : d.kind == 8 ? 9 : d.kind == 9 ? 10 : 1;
        cell.category = RouteCategory::terminal;
        s.finish.surface[n].definition = id;
        s.finish.surface[n].updates = 0; // c/i.a(1)重置f。
        s.finish.surface[n].variant = fragment.fragment_index;
        occupied.occupied_cells.push_back(fragment.position);
    }
    s.finish.sites.emplace(identity, std::move(occupied));
    WorldMapRefreshState refresh;
    refresh.map = world.map;
    for (const auto &surface : s.finish.surface)
        refresh.surface.push_back({surface.definition, surface.updates, surface.display_definition,
                                   surface.variant, surface.road_mask, surface.fragment,
                                   surface.instance, false, false});
    for (const auto &definition : s.facility_definitions)
        refresh.definitions.emplace(definition.first, definition.second.map);
    refresh.ground_definition = s.finish.ground_definition;
    refresh.special_ground_definition = s.special_ground_definition;
    refresh.base_variants = s.base_variants;
    refresh.fence_level = s.fence_level;
    refresh.fence_levels = s.fence_levels;
    auto displayed = prepare_world_map_display(refresh);
    if (!displayed.candidate)
        throw TaskCreationError::map_failed;
    auto rebuilt = prepare_world_map_roads(displayed.candidate->state);
    if (!rebuilt.candidate)
        throw TaskCreationError::map_failed;
    c.map_steps = {WorldMapRefreshStep::display, WorldMapRefreshStep::roads_fences,
                   WorldMapRefreshStep::scene_refresh}; // a/o.a(z=true,z2=false)：没有邻接重算。
    world.map = rebuilt.candidate->state.map;
    s.refresh_pending = rebuilt.candidate->state.refresh_pending;
    s.road_patches.clear();
    for (std::size_t n = 0; n < refresh.surface.size(); ++n) {
        const auto &cell = rebuilt.candidate->state.surface[n];
        s.finish.surface[n] = {cell.definition, cell.updates, cell.instance_field, cell.fragment,
                               cell.display,    cell.variant, cell.road_mask};
        s.road_patches.push_back({cell.road_quad, cell.edge_road_pair});
    }
    return identity;
}
using Reward = std::array<int, 2>;
Reward take(TaskCreationCandidate &c, std::vector<Reward> &pool, bool remove) {
    const auto index = draw(c, integer(pool.size()));
    const auto value = pool.at(index);
    if (remove)
        pool.erase(pool.begin() + index);
    return value;
}
std::vector<DungeonChallenge> challenges(TaskCreationCandidate &c, const TaskCreationDefinition &d,
                                         int difficulty) {
    const int count =
        integer(std::int64_t(d.minimum_rewards) +
                draw(c, integer(std::int64_t(d.maximum_rewards) - d.minimum_rewards + 1)));
    if (count <= 0)
        throw TaskCreationError::invalid_input;
    std::vector<Reward> ordinary_items, rare_items, ordinary_equipment, rare_equipment;
    for (const auto &r : c.state.rewards) {
        if (!(r.flags & 2U) || (r.kind != 0 && r.status == 1))
            continue;
        auto &ordinary = r.kind == 0 ? ordinary_items : ordinary_equipment;
        auto &rare = r.kind == 0 ? rare_items : rare_equipment;
        if (r.difficulty <= difficulty)
            ordinary.push_back({r.kind, r.identity});
        else if (r.difficulty == difficulty + 1)
            rare.push_back({r.kind, r.identity});
    }
    std::vector<Reward> rewards(static_cast<std::size_t>(count));
    for (int index = 0; index < count; ++index) {
        bool chosen = false;
        if (index == count - 1) {
            if (c.state.finish.task_progress.successes == 0) {
                rewards[index] = {1, c.state.first_reward_weapon};
                chosen = true;
            } else if (draw(c, 100) < 95 && !rare_equipment.empty()) {
                rewards[index] = take(c, rare_equipment, true);
                chosen = true;
            }
            if (!chosen && !rare_items.empty()) {
                rewards[index] = take(c, rare_items, false);
                chosen = true;
            }
            if (chosen)
                break;
        }
        if (draw(c, 100) < 35 && !ordinary_equipment.empty()) {
            rewards[index] = take(c, ordinary_equipment, true);
            chosen = true;
        }
        if (!chosen)
            rewards[index] = take(c, ordinary_items, false);
    }
    int monster_chance = 40;
    if (d.completions > 0 && !(d.flags & 8U)) {
        if (draw(c, 100) < 5)
            monster_chance = 100;
        else if (draw(c, 100) < 5)
            monster_chance = 0;
    }
    std::vector<DungeonChallenge> result;
    for (int index = 0; index < count; ++index) {
        int progress = interpolate(index, count, 10, 93) + draw(c, 10) - 5;
        if (index == count - 1)
            progress = 97; // 低层L369先draw10，L387才覆盖最终进度。
        if (index != count - 1 && draw(c, 100) < monster_chance) {
            const auto &pool = c.state.challenge_monsters[std::min(difficulty / 2, 5)];
            const int id = pool.at(draw(c, integer(pool.size())));
            result.push_back({progress, 1, 0, 0, id, monster(c.state, id).challenge_strength});
        } else {
            result.push_back({progress, 0, 0, 0, rewards[index][0], rewards[index][1]});
        }
    }
    return result;
}
void create_task(TaskCreationCandidate &c, const TaskCreationDefinition &d, Position p,
                 int difficulty, std::optional<std::uint64_t> facility) {
    auto &s = c.state;
    std::set<int> originals;
    std::set<std::uint64_t> identities;
    for (const auto id : s.finish.task_order) {
        if (!s.finish.tasks.count(id) || !s.task_original_ids.count(id))
            throw TaskCreationError::invalid_input;
        originals.insert(s.task_original_ids.at(id));
    }
    for (const auto &task : s.finish.tasks)
        identities.insert(task.first);
    const auto raw = std::int64_t(s.task_sequence) + 1;
    // 原Java先有符号32位加法，再%MAX_VALUE；显式保留极限溢出的负号。
    int sequence = static_cast<int>(
        raw > std::numeric_limits<int>::max() ? raw - (std::int64_t(1) << 32) : raw);
    sequence %= std::numeric_limits<int>::max();
    while (originals.count(sequence))
        sequence = integer(std::int64_t(sequence) + 1);
    s.task_sequence = sequence;
    const auto identity = stable_id(identities, s.next_task_identity);
    s.task_original_ids.emplace(identity, sequence);
    s.finish.tasks.emplace(identity, DungeonFinishTask{identity, d.identity, difficulty,
                                                       d.pending_completion_value, facility, p, true});
    s.finish.task_order.push_back(identity);
    c.created_task = identity;
}
} // namespace
TaskCreationResult prepare_world_task_creation(const WorldTaskCreationState &state, int kind) {
    try {
        if ((kind != 0 && kind != 1) || state.rank < 0 ||
            state.finish.task_progress.exploration_stage < 0 ||
            state.finish.task_progress.exploration_stage > 5 ||
            !valid_legacy_map(state.finish.dungeon.world.map) || state.fence_level < 0 ||
            static_cast<std::size_t>(state.fence_level) >= state.fence_levels.size())
            throw TaskCreationError::invalid_input;
        std::set<int> definition_ids;
        for (const auto &d : state.definitions)
            if (d.identity < 0 || !definition_ids.insert(d.identity).second)
                throw TaskCreationError::invalid_input;
        TaskCreationCandidate c{state, {}, {}, 0, {}, {}};
        const TaskCreationDefinition d = select(c, kind);
        c.selected_definition = d.identity;
        int score = interpolate(state.rank, 5, 0, 50) +
                    interpolate(state.finish.task_progress.exploration_stage, 5, 0, 50) +
                    draw(c, 30) - (kind == 0 ? 10 : 20);
        if (kind == 0) {
            std::int64_t knowledge = 0;
            int count = 0;
            for (const auto &h : state.humans)
                if (h.status != 0) {
                    knowledge += h.knowledge;
                    ++count;
                }
            score += interpolate(count > 0 ? integer(knowledge / count) : 0, 30, 0, 20);
        }
        c.difficulty = interpolate(score, 100, 1, 9);
        if (d.flags & 8U)
            c.difficulty = std::min(c.difficulty + 1, 9);
        const auto position = site(c);
        if (!position)
            return {TaskCreationError::none, std::move(c)};
        std::optional<std::uint64_t> facility;
        if (kind == 0) {
            if (c.state.finish.surface.size() != c.state.finish.dungeon.world.map.cells.size())
                throw TaskCreationError::invalid_input;
            const int id = d.facilities.at(draw(c, integer(d.facilities.size())));
            facility = create_facility(c, id, *position);
            auto &progress = c.state.finish.dungeon.facilities.at(*facility);
            progress.challenges = challenges(c, d, c.difficulty);
            c.state.facility_difficulties[*facility] = c.difficulty;
            // Tenant.l是难度；实际h在之后b(1)开始探索才根据Q设置，不提前初始化。
        }
        create_task(c, d, *position, c.difficulty, facility);
        return {TaskCreationError::none, std::move(c)};
    } catch (TaskCreationError error) {
        return {error, {}};
    } catch (const std::out_of_range &) {
        return {TaskCreationError::invalid_input, {}};
    }
}
} // namespace dungeon_village_reference
