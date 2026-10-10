// 村庄扩张只改变原围栏等级，不改变地图大小；实例、地格与旧人物ax各自保持来源语义。
#include "ark/simulation/map/startup_world_expansion.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/combat/rules/rescue_commit.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Bounds = std::array<ref::Position, 2>;
bool within(const ref::LegacyMap &map, ref::Position p) {
    return p.x >= 0 && p.y >= 0 && p.x < map.width && p.y < map.height;
}
bool inside(ref::Position p, const Bounds &b, bool inclusive) {
    return inclusive ? p.x >= b[0].x && p.x <= b[1].x && p.y >= b[1].y && p.y <= b[0].y
                     : p.x > b[0].x && p.x < b[1].x && p.y > b[1].y && p.y < b[0].y;
}
std::size_t index(const ref::LegacyMap &map, ref::Position p) {
    return static_cast<std::size_t>(p.y) * map.width + p.x;
}
bool erase(State &s, std::uint64_t id) {
    // 固定h.m[0]任务选址y>=15，最大扩张内圈y<=11，不会自然触及任务设施。
    // 原Java会保留k.f对象；维护层不把越出该已证几何域的任务引用变成悬空稳定ID。
    if (std::any_of(s.tasks.begin(), s.tasks.end(),
                    [&](const auto &task) { return task.second.facility == id; }))
        return false;
    // 原o.a(anchor,false)：c→d(含场景f)→f→邻接false；两次f都只置同一刷新位。
    return retire_startup_world_facility(s, id) && refresh_startup_world_map(s, false);
}
bool erase_at(State &s, ref::Position p) {
    const auto &map = s.scene.world.world.map;
    if (!within(map, p))
        return false;
    const auto binding = map.cells[index(map, p)].facility;
    return !binding || erase(s, binding->instance_id.value);
}
bool install(State &s, int definition, ref::Position p, bool refresh) {
    const auto result = install_startup_world_map_facility(s, definition, p, refresh);
    return result.error == StartupWorldRuntimeError::none &&
           result.denial == StartupBuildDenial::none && result.created.has_value();
}
} // namespace

bool expand_startup_world_map(State &s) {
    if (!s.rules || s.fence_level < 0 ||
        static_cast<std::size_t>(s.fence_level) >= s.rules->fences.size())
        return false;
    if (static_cast<std::size_t>(s.fence_level) + 1 == s.rules->fences.size())
        return true; // 原f到顶直接返回，外层53仍完成本次活动。
    const auto &old_map = s.scene.world.world.map;
    if (!ref::valid_legacy_map(old_map) || s.surface.size() != old_map.cells.size() ||
        s.road_patches.size() != s.surface.size() || s.base_variants.size() != s.surface.size() ||
        s.scene.world.surface.size() != s.surface.size() ||
        s.scene.world.map_flags.size() != s.surface.size() || s.rules->generation_bounds.empty())
        return false;
    const int level = s.fence_level + 1;
    const auto bounds = s.rules->fences[level];
    if (!within(old_map, bounds[0]) || !within(old_map, bounds[1]) || bounds[0].x >= bounds[1].x ||
        bounds[1].y >= bounds[0].y)
        return false;
    std::map<int, const StartupDefinition *> definitions;
    const StartupDefinition *v = nullptr, *w = nullptr;
    for (const auto &d : s.rules->facilities) {
        if (!definitions.emplace(d.id, &d).second)
            return false;
        // 原c/n.J按定义顺序选择V(category4,m2)和W(kind4,m2)，不是硬编码显示图号。
        if (!v && d.category == 4 && d.direction == 2)
            v = &d;
        if (!w && d.kind == 4 && d.direction == 2)
            w = &d;
    }
    if (!v || !w || !definitions.count(s.ground_definition) ||
        v->id == std::numeric_limits<int>::max() || !definitions.count(v->id + 1))
        return false;
    // h.k记录原地图加载扫描(y逆序、x正序)的kind5种子；首项x和方向不随扩张重算。
    const StartupFacility *seed = nullptr;
    for (const auto &candidate : startup_evidence().seeds) {
        const auto d = definitions.find(candidate.definition_id);
        if (d == definitions.end())
            return false;
        if (d->second->kind == 5 &&
            (!seed || candidate.cell.y > seed->cell.y ||
             (candidate.cell.y == seed->cell.y && candidate.cell.x < seed->cell.x)))
            seed = &candidate;
    }
    if (!seed || seed->cell.x == std::numeric_limits<int>::max())
        return false;
    const int direction = definitions.find(seed->definition_id)->second->direction;
    if (direction == std::numeric_limits<int>::max())
        return false;
    const std::array<ref::Position, 2> entrances{
        {{seed->cell.x, bounds[0].y}, {seed->cell.x + 1, bounds[0].y}}};
    if (!within(old_map, entrances[0]) || !within(old_map, entrances[1]))
        return false;
    std::set<std::uint64_t> seen;
    std::set<int> raw_ids;
    std::set<std::pair<int, int>> ordinals;
    for (const auto id : s.scene.world.facility_order) {
        const auto facility = s.scene.world.world.facilities.find(id);
        if (!seen.insert(id).second || facility == s.scene.world.world.facilities.end() ||
            !definitions.count(facility->second.placement.definition_id) ||
            !s.facility_original_ids.count(id) || !s.facility_ordinals.count(id) ||
            !s.neighbourhood.count(id) || !s.neighbourhood_details.count(id) ||
            !s.facility_details.count(id) || facility->second.placement.instance_id.value != id)
            return false;
        const int raw = s.facility_original_ids.find(id)->second;
        const int ordinal = s.facility_ordinals.find(id)->second;
        if (raw < 0 || ordinal < 0 || !raw_ids.insert(raw).second ||
            !ordinals.emplace(facility->second.placement.definition_id, ordinal).second)
            return false;
    }
    if (seen.size() != s.scene.world.world.facilities.size())
        return false;

    auto next = s;
    next.fence_level = level;
    next.scene.world.town = {bounds[0].x, bounds[1].x, bounds[1].y, bounds[0].y};
    struct Replacement {
        ref::Position anchor;
        int definition;
    };
    std::vector<Replacement> replacements;
    const auto old_order = next.scene.world.facility_order;
    for (auto it = old_order.rbegin(); it != old_order.rend(); ++it) {
        const auto &f = next.scene.world.world.facilities.find(*it)->second;
        const auto *d = definitions.find(f.placement.definition_id)->second;
        if (d->kind != 4)
            continue;
        const auto replacement = static_cast<std::int64_t>(w->id) + level * 2LL + d->direction % 2;
        if (replacement < 0 || replacement > std::numeric_limits<int>::max() ||
            !definitions.count(static_cast<int>(replacement)) ||
            definitions.find(static_cast<int>(replacement))->second->kind != 4)
            return false;
        replacements.push_back({f.placement.anchor, static_cast<int>(replacement)});
        if (!erase(next, *it))
            return false;
    }
    for (const auto &replacement : replacements)
        if (!erase_at(next, replacement.anchor) ||
            !install(next, replacement.definition, replacement.anchor, true))
            return false;

    const auto after_replacement = next.scene.world.facility_order;
    for (auto it = after_replacement.rbegin(); it != after_replacement.rend(); ++it) {
        const auto &f = next.scene.world.world.facilities.find(*it)->second;
        const auto *d = definitions.find(f.placement.definition_id)->second;
        if (!(d->flags & 16))
            continue;
        const auto footprint =
            ref::facility_footprint(f.placement.shape, f.placement.orientation, f.placement.anchor,
                                    old_map.width, old_map.height);
        if (footprint.error != ref::GeometryError::none)
            return false;
        if (std::any_of(footprint.cells.begin(), footprint.cells.end(),
                        [&](const auto &cell) { return inside(cell.position, bounds, false); }) &&
            !erase(next, *it))
            return false;
    }
    auto &map = next.scene.world.world.map;
    for (int y = bounds[1].y; y <= bounds[0].y; ++y)
        for (int x = bounds[0].x; x <= bounds[1].x; ++x) {
            const auto n = index(map, {x, y});
            auto &cell = map.cells[n];
            auto &surface = next.surface[n];
            if (cell.legacy_state == 12) {
                if (cell.facility)
                    return false; // a(false)不能留下悬空占地；正常原特殊地面没有实例。
                cell.facility.reset();
                surface.definition = next.ground_definition;
                surface.instance = surface.fragment = -1;
                cell.category = ref::RouteCategory::ground;
                cell.legacy_state = 4;
                surface.updates = 0;
            }
            if (cell.legacy_state == 5 && inside({x, y}, bounds, false)) {
                cell.category = ref::RouteCategory::ground;
                cell.legacy_state = 4;
                surface.updates = 0; // 原a(4)保留定义、x和l，最终c/d才重建显示。
            }
        }
    for (auto &surface : next.surface)
        surface.instance = -1;
    // 原顺序先删除两处旧引用，再创建两处，不能交替删除/创建改变最小空闲数字ID。
    if (!erase_at(next, entrances[0]) || !erase_at(next, entrances[1]) ||
        !install(next, v->id, entrances[0], false) ||
        !install(next, v->id + 1, entrances[1], false))
        return false;
    for (int n = 0; n < 2; ++n) {
        const auto at = index(map, entrances[n]);
        next.surface[at].instance = direction + n;
        next.surface[at].fragment = -1;
        next.surface[at].updates = 0;
        map.cells[at].legacy_state = 7;
        map.cells[at].category = ref::RouteCategory::access;
    }
    // bl是人物名单。用旧缓存ax筛选，不能先按新村界重算ax；r()不删除、不清G路线/N。
    const auto humans = next.scene.world.world.ai.human_order;
    std::set<ref::CharacterId> seen_humans;
    for (const auto id : humans) {
        const auto actor = next.scene.world.world.ai.battle.actors.find(id);
        const auto context = next.scene.world.world.ai.contexts.find(id);
        if (!seen_humans.insert(id).second ||
            actor == next.scene.world.world.ai.battle.actors.end() ||
            actor->second.kind != ref::ActorKind::human ||
            context == next.scene.world.world.ai.contexts.end() ||
            !next.scene.world.world.actors.count(id))
            return false;
        if (!context->second.inside_town && inside(context->second.cell, bounds, true)) {
            auto cleaned = ref::prepare_world_actor_cleanup(next.scene.world.world, id);
            if (!cleaned.candidate)
                return false;
            next.scene.world.world = std::move(cleaned.candidate->state);
        }
    }
    if (!refresh_startup_world_surface(next))
        return false;
    s = std::move(next);
    return true;
}
} // namespace ark::simulation
