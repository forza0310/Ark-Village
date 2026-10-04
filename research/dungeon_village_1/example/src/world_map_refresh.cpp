#include "dungeon_village_reference/world_map_refresh.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
// 固定APK c/h.F和c/d.b；越界邻接被原a(x,y,n)视为同定义，不是空格。
constexpr std::array<int, 16> road_variants{0, 13, 15, 3, 14, 11, 5, 7, 12, 9, 1, 10, 4, 8, 6, 2};
constexpr std::array<Position, 4> cardinal{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
bool within(const LegacyMap &map, Position p) {
    return p.x >= 0 && p.y >= 0 && p.x < map.width && p.y < map.height;
}
std::size_t index(const LegacyMap &map, Position p) {
    return static_cast<std::size_t>(p.y) * map.width + p.x;
}
WorldMapRefreshResult failure(WorldMapRefreshError error) { return {error, {}}; }
WorldMapRefreshError validate_surface(const WorldMapRefreshState &s) {
    if (!valid_legacy_map(s.map) || s.surface.size() != s.map.cells.size() ||
        s.base_variants.size() != s.map.cells.size() || s.ground_definition < 0 ||
        s.special_ground_definition < 0)
        return WorldMapRefreshError::invalid_input;
    if (!s.definitions.count(s.ground_definition) ||
        !s.definitions.count(s.special_ground_definition))
        return WorldMapRefreshError::missing_definition;
    for (const auto &d : s.definitions)
        if (d.first < 0 || d.second.display < -1 || d.second.category < 0 || d.second.category > 13)
            return WorldMapRefreshError::invalid_input;
    for (std::size_t n = 0; n < s.surface.size(); ++n) {
        if (!s.definitions.count(s.surface[n].definition))
            return WorldMapRefreshError::missing_definition;
        if (s.surface[n].updates < 0 || s.base_variants[n] < -128 || s.base_variants[n] > 127)
            return WorldMapRefreshError::invalid_input;
        const auto &binding = s.map.cells[n].facility;
        if (binding && binding->definition_id != s.surface[n].definition)
            return WorldMapRefreshError::stale_facility;
    }
    return WorldMapRefreshError::none;
}
bool same_definition(const WorldMapRefreshState &s, Position p, int definition) {
    return !within(s.map, p) || s.surface[index(s.map, p)].definition == definition;
}
void fence(WorldMapRefreshState &s, Position p, int fragment) {
    const auto n = index(s.map, p);
    s.surface[n].fragment = fragment;
    s.surface[n].updates = 0;
    s.map.cells[n].category = RouteCategory::blocked; // 原g3。
    s.map.cells[n].legacy_state = 5;
}
bool fence_ground(const WorldMapRefreshState &s, Position p) {
    const auto state = s.map.cells[index(s.map, p)].legacy_state;
    return state == 4 || state == 5;
}
} // namespace
WorldMapRefreshResult prepare_world_map_display(const WorldMapRefreshState &s) {
    const auto error = validate_surface(s);
    if (error != WorldMapRefreshError::none)
        return failure(error);
    WorldMapRefreshCandidate c{s, {WorldMapRefreshStep::display}, {}};
    for (std::size_t n = 0; n < c.state.surface.size(); ++n) {
        auto &cell = c.state.surface[n];
        cell.fragment = -1;
        const int state = c.state.map.cells[n].legacy_state;
        if (state == 4 || state == 5 || state == 7) {
            cell.variant = 0;
            cell.display = s.definitions.at(s.ground_definition).display;
        } else if (state == 11) {
            cell.variant = 0;
            cell.display = s.definitions.at(s.special_ground_definition).display;
        } else {
            cell.display = s.definitions.at(cell.definition).display;
        }
    }
    return {WorldMapRefreshError::none, std::move(c)};
}
WorldMapRefreshResult prepare_world_map_roads(const WorldMapRefreshState &s) {
    const auto error = validate_surface(s);
    if (error != WorldMapRefreshError::none)
        return failure(error);
    if (s.fence_level < 0 || static_cast<std::size_t>(s.fence_level) >= s.fence_levels.size())
        return failure(WorldMapRefreshError::invalid_input);
    const auto bounds = s.fence_levels[s.fence_level];
    const Position left_bottom = bounds[0], right_top = bounds[1];
    if (!within(s.map, left_bottom) || !within(s.map, right_top) || left_bottom.x >= right_top.x ||
        right_top.y >= left_bottom.y)
        return failure(WorldMapRefreshError::invalid_input);
    WorldMapRefreshCandidate c{s, {WorldMapRefreshStep::roads_fences}, {}};
    for (int y = 0; y < s.map.height; ++y)
        for (int x = 0; x < s.map.width; ++x) {
            const auto n = index(s.map, {x, y});
            auto &cell = c.state.surface[n];
            const int state = c.state.map.cells[n].legacy_state;
            cell.road_mask = 0;
            cell.fragment = -1;
            if (state == 4 || state == 5)
                cell.variant = s.base_variants[n];
            if ((state == 3 || state == 5) && s.definitions.at(cell.definition).category == 6) {
                for (std::size_t direction = 0; direction < cardinal.size(); ++direction)
                    if (same_definition(s, {x + cardinal[direction].x, y + cardinal[direction].y},
                                        cell.definition))
                        cell.road_mask |= 1 << direction;
                cell.variant = road_variants[cell.road_mask];
            }
        }
    for (int x = left_bottom.x; x <= right_top.x; ++x) {
        if (fence_ground(c.state, {x, left_bottom.y}))
            fence(c.state, {x, left_bottom.y}, 1);
        if (fence_ground(c.state, {x, right_top.y}))
            fence(c.state, {x, right_top.y}, 1);
    }
    for (int y = left_bottom.y; y >= right_top.y; --y) {
        if (fence_ground(c.state, {left_bottom.x, y}))
            fence(c.state, {left_bottom.x, y}, 0);
        if (fence_ground(c.state, {right_top.x, y}))
            fence(c.state, {right_top.x, y}, 0);
    }
    // 四角无状态守卫，覆盖原状态/类别，但保留定义、实例和原绑定。
    fence(c.state, left_bottom, 4);
    fence(c.state, {right_top.x, left_bottom.y}, 2);
    fence(c.state, {left_bottom.x, right_top.y}, 3);
    fence(c.state, right_top, 5);
    // 原扫描只处理y>0且x<width-1，未扫描格保留旧标志。
    for (int y = s.map.height - 1; y > 0; --y)
        for (int x = 0; x < s.map.width - 1; ++x) {
            const auto n = index(s.map, {x, y});
            c.state.surface[n].road_quad = false;
            if (c.state.map.cells[n].legacy_state == 3 &&
                c.state.definitions.at(c.state.surface[n].definition).category == 6) {
                const auto definition = c.state.surface[n].definition;
                if (same_definition(c.state, {x + 1, y}, definition) &&
                    same_definition(c.state, {x + 1, y - 1}, definition) &&
                    same_definition(c.state, {x, y - 1}, definition))
                    c.state.surface[n].road_quad = true;
            }
        }
    for (int y : {0, s.map.height - 1})
        for (int x = 0; x < s.map.width - 1; ++x) {
            const auto n = index(s.map, {x, y});
            c.state.surface[n].edge_road_pair = false;
            if (c.state.map.cells[n].legacy_state == 3 &&
                c.state.definitions.at(c.state.surface[n].definition).category == 6 &&
                same_definition(c.state, {x + 1, y}, c.state.surface[n].definition))
                c.state.surface[n].edge_road_pair = true;
        }
    c.state.refresh_pending = true;
    c.steps.push_back(WorldMapRefreshStep::scene_refresh);
    return {WorldMapRefreshError::none, std::move(c)};
}
WorldMapRefreshResult prepare_world_map_neighbours(const WorldMapRefreshState &s, bool success) {
    const auto error = validate_surface(s);
    if (error != WorldMapRefreshError::none)
        return failure(error);
    if (s.neighbours.size() != s.facilities.size() ||
        validate_facility_layout(s.facilities, s.map.width, s.map.height) != GeometryError::none)
        return failure(WorldMapRefreshError::stale_facility);
    std::vector<NeighbourDefinition> definitions;
    std::vector<Position> roads;
    std::optional<BuildingId> last_source;
    std::set<std::uint64_t> owners;
    std::set<std::size_t> occupied;
    for (const auto &f : s.facilities) {
        const auto d = s.definitions.find(f.definition_id);
        if (d == s.definitions.end())
            return failure(WorldMapRefreshError::missing_definition);
        if (!s.neighbours.count(f.instance_id.value) ||
            !owners.insert(f.instance_id.value).second || d->second.shape != f.shape)
            return failure(WorldMapRefreshError::stale_facility);
        if (d->second.category == 2 || d->second.category == 3)
            last_source = f.instance_id;
        for (const auto &cell :
             facility_footprint(f.shape, f.orientation, f.anchor, s.map.width, s.map.height)
                 .cells) {
            const auto &binding = s.map.cells[index(s.map, cell.position)].facility;
            occupied.insert(index(s.map, cell.position));
            if (!binding || !(binding->instance_id == f.instance_id) ||
                binding->definition_id != f.definition_id ||
                binding->fragment_index != cell.fragment_index)
                return failure(WorldMapRefreshError::stale_facility);
        }
    }
    for (std::size_t n = 0; n < s.map.cells.size(); ++n)
        if (s.map.cells[n].facility &&
            (!owners.count(s.map.cells[n].facility->instance_id.value) || !occupied.count(n)))
            return failure(WorldMapRefreshError::stale_facility);
    for (const auto &d : s.definitions)
        definitions.push_back({d.first, d.second.shape, d.second.category, d.second.modifiers});
    for (int y = 0; y < s.map.height; ++y)
        for (int x = 0; x < s.map.width; ++x)
            if (s.map.cells[index(s.map, {x, y})].legacy_state == 3)
                roads.push_back({x, y}); // 原道路魅力检查状态3，不额外要求类别6。
    const auto derived =
        derive_facility_neighbourhood(definitions, s.facilities, roads, s.map.width, s.map.height);
    if (derived.error != NeighbourhoodError::none)
        return failure(derived.error == NeighbourhoodError::numeric_overflow
                           ? WorldMapRefreshError::numeric_overflow
                           : WorldMapRefreshError::neighbourhood_failed);
    WorldMapRefreshCandidate c{s, {WorldMapRefreshStep::neighbours}, {}};
    for (const auto &f : derived.facilities) {
        auto &cache = c.state.neighbours.at(f.instance_id.value);
        // 原s是int逐项相加；不能让后续负值抵消先前溢出来伪造合法终值。
        std::array<std::int64_t, 3> partial{};
        for (const auto &source : f.sources)
            for (const auto &modifier : s.definitions.at(source.definition_id).modifiers) {
                auto &value = partial[modifier.attribute_slot];
                if (modifier.delta < std::numeric_limits<int>::min() ||
                    modifier.delta > std::numeric_limits<int>::max())
                    return failure(WorldMapRefreshError::numeric_overflow);
                value += modifier.delta;
                if (value < std::numeric_limits<int>::min() ||
                    value > std::numeric_limits<int>::max())
                    return failure(WorldMapRefreshError::numeric_overflow);
            }
        cache.previous = cache.current;
        cache.sources = f.sources;
        if (last_source)
            cache.visited =
                std::any_of(f.sources.begin(), f.sources.end(),
                            [&](const auto &source) { return source.instance_id == *last_source; });
        for (std::size_t slot = 0; slot < f.modifiers.size(); ++slot) {
            const auto value = f.modifiers[slot];
            if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
                return failure(WorldMapRefreshError::numeric_overflow);
            cache.current[slot] = static_cast<int>(value);
            const auto delta = value - cache.previous[slot];
            if (delta < std::numeric_limits<int>::min() || delta > std::numeric_limits<int>::max())
                return failure(WorldMapRefreshError::numeric_overflow);
            if (success && delta != 0) {
                const auto notice = static_cast<int>(slot + (delta > 0 ? 1 : 4));
                const auto existing = std::find_if(cache.notices.begin(), cache.notices.end(),
                                                   [&](const auto &n) { return n[0] == notice; });
                if (existing == cache.notices.end()) {
                    cache.notices.push_back({notice, 0});
                    c.added_notices.emplace_back(f.instance_id, notice);
                }
            }
        }
    }
    return {WorldMapRefreshError::none, std::move(c)};
}
WorldMapRefreshResult prepare_world_map_refresh(const WorldMapRefreshState &s, bool success) {
    auto display = prepare_world_map_display(s);
    if (!display.candidate)
        return display;
    auto roads = prepare_world_map_roads(display.candidate->state);
    if (!roads.candidate)
        return roads;
    auto neighbours = prepare_world_map_neighbours(roads.candidate->state, success);
    if (!neighbours.candidate)
        return neighbours;
    auto &steps = neighbours.candidate->steps;
    steps.insert(steps.begin(), WorldMapRefreshStep::scene_refresh); // a/o外层第二次f。
    steps.insert(steps.begin(), roads.candidate->steps.begin(), roads.candidate->steps.end());
    steps.insert(steps.begin(), display.candidate->steps.begin(), display.candidate->steps.end());
    return neighbours;
}
} // namespace dungeon_village_reference
