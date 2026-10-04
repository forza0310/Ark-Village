// First-play map reconstruction from published definitions, not translated Java control flow.
// Identity allocation and logical/display states are deliberately separate. See ../README.md.
#include "ark/simulation/startup.hpp"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace ark::simulation {
namespace {
const StartupDefinition &definition(const StartupEvidence &data, int id) {
    const auto it = std::find_if(data.definitions.begin(), data.definitions.end(),
                                 [id](const auto &d) { return d.id == id; });
    if (it == data.definitions.end())
        throw std::invalid_argument("加载后地图缺少设施定义");
    return *it;
}
int definition_id(const StartupEvidence &data, int display) {
    const auto it = std::find_if(data.displays.begin(), data.displays.end(),
                                 [display](const auto &d) { return d.id == display; });
    if (it == data.displays.end())
        throw std::invalid_argument("加载后地图缺少显示记录");
    return it->definition_id;
}
std::size_t index_of(const LoadedStartupMap &map, ref::Position p) {
    if (p.x < 0 || p.y < 0 || p.x >= map.width || p.y >= map.height)
        throw std::invalid_argument("加载后地图坐标越界");
    return static_cast<std::size_t>(p.y * map.width + p.x);
}
std::pair<int, ref::RouteCategory> binding_kind(int kind) {
    switch (kind) {
    case 3:
        return {1, ref::RouteCategory::terminal};
    case 2:
        return {2, ref::RouteCategory::blocked};
    case 4:
        return {6, ref::RouteCategory::access};
    case 5:
        return {7, ref::RouteCategory::access};
    default:
        throw std::invalid_argument("加载后地图含未支持实例种类");
    }
}
} // namespace

LoadedStartupMap reconstruct_startup_map(const StartupEvidence &data) {
    if (data.width != 24 || data.height != 24 || data.cells.size() != 576 ||
        data.build_bounds != std::array<int, 4>{7, 16, 3, 9} || data.seeds.size() != 8)
        throw std::invalid_argument("仅支持固定无继承首局地图");
    LoadedStartupMap map{data.width, data.height, {}, {}};
    std::set<ref::Position, bool (*)(const ref::Position &, const ref::Position &)> occupied(
        [](const auto &a, const auto &b) { return a.y == b.y ? a.x < b.x : a.y < b.y; });
    auto seeds = data.seeds;
    std::sort(seeds.begin(), seeds.end(), [](const auto &a, const auto &b) {
        return a.cell.y == b.cell.y ? a.cell.x < b.cell.x : a.cell.y > b.cell.y;
    });
    for (const auto &seed : seeds) {
        const auto &d = definition(data, seed.definition_id);
        if (d.shape != 0 || seed.remaining_ticks != 0 || !seed.seed ||
            definition_id(data, data.cells[index_of(map, seed.cell)].display_id) != d.id ||
            !occupied.insert(seed.cell).second)
            throw std::invalid_argument("加载后地图源种子不一致");
        binding_kind(d.kind);
        map.instances.push_back({static_cast<int>(map.instances.size()), d.id, seed.cell});
    }
    // Original vector-order deletion plus smallest-free allocation determines identity, but not
    // prototype lifetime IDs. Preserve unaffected instances and replace entrances in source order.
    std::vector<LoadedStartupInstance> town, external;
    for (const auto &item : map.instances) {
        const int kind = definition(data, item.definition_id).kind;
        if (kind == 4)
            town.push_back(item);
        if (kind == 5)
            external.push_back(item);
    }
    if (town.size() != 2 || external.size() != 2 ||
        !(external[0].anchor == ref::Position{11, 10}) ||
        !(external[1].anchor == ref::Position{12, 10}))
        throw std::invalid_argument("加载后地图入口布局变化");
    const auto erase_kind = [&](int kind) {
        map.instances.erase(std::remove_if(map.instances.begin(), map.instances.end(),
                                           [&](const auto &i) {
                                               return definition(data, i.definition_id).kind ==
                                                      kind;
                                           }),
                            map.instances.end());
    };
    const auto insert = [&](int id, ref::Position anchor) {
        int raw = 0;
        while (std::any_of(map.instances.begin(), map.instances.end(),
                           [raw](const auto &i) { return i.legacy_id == raw; }))
            ++raw;
        map.instances.push_back({raw, id, anchor});
    };
    erase_kind(4);
    for (auto it = town.rbegin(); it != town.rend(); ++it)
        insert(75 + definition(data, it->definition_id).direction % 2, it->anchor);
    // No flag16 seed overlaps the strict interior in this input. Other inputs must be researched.
    for (const auto &item : map.instances)
        if ((definition(data, item.definition_id).flags & 16) && item.anchor.x > 6 &&
            item.anchor.x < 17 && item.anchor.y > 2 && item.anchor.y < 10)
            throw std::invalid_argument("首局内出现需清除的实例");
    erase_kind(5);
    insert(83, {external[0].anchor.x, 10});
    insert(84, {external[0].anchor.x + 1, 10});

    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            const auto source = data.cells[index_of(map, {x, y})];
            const auto &d = definition(data, definition_id(data, source.display_id));
            LoadedStartupCell cell;
            cell.definition_id = d.id;
            cell.display_id = d.display_id;
            cell.variant = source.variant;
            switch (d.kind) {
            case 6:
                cell.legacy_state = 3;
                cell.category = ref::RouteCategory::road;
                break;
            case 7:
                cell.legacy_state = 4;
                cell.category = ref::RouteCategory::ground;
                break;
            case 10:
                cell.legacy_state = 11;
                cell.category = ref::RouteCategory::blocked;
                cell.variant = 0;
                break;
            case 11:
                cell.legacy_state = 12;
                cell.category = ref::RouteCategory::blocked;
                // Town expansion clears special ground inside the INCLUSIVE boundary.
                if (x >= 6 && x <= 17 && y >= 2 && y <= 10) {
                    cell.definition_id = 17;
                    cell.display_id = definition(data, 17).display_id;
                    cell.legacy_state = 4;
                    cell.category = ref::RouteCategory::ground;
                }
                break;
            default: {
                const auto [state, category] = binding_kind(d.kind);
                cell.legacy_state = state;
                cell.category = category;
                cell.variant = 0; // Single-cell orientation zero binds fragment zero.
                if (d.kind == 5)
                    cell.display_id = definition(data, 17).display_id;
                break;
            }
            }
            map.cells.push_back(cell);
        }
    }
    // The final exterior deletion refreshes road masks BEFORE the two unrefreshed creations.
    for (const auto &item : map.instances) {
        auto &cell = map.cells[index_of(map, item.anchor)];
        cell.legacy_instance_id = item.legacy_id;
        if (definition(data, item.definition_id).kind == 5) {
            cell.definition_id = 17;
            cell.legacy_state = 4;
            cell.category = ref::RouteCategory::ground;
        }
    }
    constexpr std::array<ref::Position, 4> directions = {{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
    constexpr std::array<int, 16> frames = {{0, 13, 15, 3, 14, 11, 5, 7, 12, 9, 1, 10, 4, 8, 6, 2}};
    const auto same = [&](ref::Position p, int id) {
        return p.x < 0 || p.y < 0 || p.x >= map.width || p.y >= map.height ||
               map.cells[index_of(map, p)].definition_id == id;
    };
    for (int y = 0; y < map.height; ++y) {
        for (int x = 0; x < map.width; ++x) {
            auto &cell = map.cells[index_of(map, {x, y})];
            if (cell.legacy_state == 3) {
                for (std::size_t i = 0; i < directions.size(); ++i)
                    if (same({x + directions[i].x, y + directions[i].y}, cell.definition_id))
                        cell.road_mask |= 1 << i;
                cell.variant = frames[static_cast<std::size_t>(cell.road_mask)];
                cell.road_quad = y > 0 && x < map.width - 1 &&
                                 same({x + 1, y}, cell.definition_id) &&
                                 same({x + 1, y - 1}, cell.definition_id) &&
                                 same({x, y - 1}, cell.definition_id);
                cell.edge_road_pair = (y == 0 || y == map.height - 1) && x < map.width - 1 &&
                                      same({x + 1, y}, cell.definition_id);
            }
            if (cell.legacy_state == 4 && x >= 6 && x <= 17 && y >= 2 && y <= 10) {
                if (y == 2 || y == 10)
                    cell.boundary_fragment = 1;
                if (x == 6 || x == 17)
                    cell.boundary_fragment = 0;
            }
            if (x == 6 && y == 10)
                cell.boundary_fragment = 4;
            if (x == 17 && y == 10)
                cell.boundary_fragment = 2;
            if (x == 6 && y == 2)
                cell.boundary_fragment = 3;
            if (x == 17 && y == 2)
                cell.boundary_fragment = 5;
            if (cell.boundary_fragment != -1) {
                cell.legacy_state = 5;
                cell.category = ref::RouteCategory::blocked;
            }
        }
    }
    for (const auto &item : map.instances) {
        const auto &d = definition(data, item.definition_id);
        auto &cell = map.cells[index_of(map, item.anchor)];
        if (d.kind == 5) {
            cell.definition_id = d.id;
            cell.legacy_state = 7;
            cell.category = ref::RouteCategory::access;
            cell.boundary_fragment = -1;
            cell.external_direction = d.direction;
        }
    }
    return map;
}

ref::LegacyMap startup_route_map(const LoadedStartupMap &map) {
    if (map.width != 24 || map.height != 24 || map.cells.size() != 576)
        throw std::invalid_argument("加载后地图投影尺寸错误");
    ref::LegacyMap result{map.width, map.height, {}};
    for (const auto &cell : map.cells) {
        std::optional<ref::FacilityTileBinding> binding;
        if (cell.legacy_instance_id) {
            if (*cell.legacy_instance_id < 0)
                throw std::invalid_argument("原始实例身份为负");
            binding = ref::FacilityTileBinding{
                ref::BuildingId{static_cast<std::uint64_t>(*cell.legacy_instance_id) + 1},
                cell.definition_id, 0};
        }
        result.cells.push_back({cell.legacy_state, cell.category, binding});
    }
    if (!ref::valid_legacy_map(result))
        throw std::invalid_argument("加载后地图寻路投影无效");
    return result;
}
} // namespace ark::simulation
