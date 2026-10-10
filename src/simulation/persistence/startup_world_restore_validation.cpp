// 恢复时只检查身份、完整载荷和已证边界；不调用更新、页面初始化或引用收集来修补输入。
#include "startup_world_restore_validation.hpp"

#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/facilities/startup_world_facility_items.hpp"
#include "ark/simulation/facilities/startup_world_facility_catalog.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "ark/simulation/village/startup_world_information.hpp"
#include "ark/simulation/actors/startup_world_human.hpp"
#include "ark/simulation/ai/rules/actor_control.hpp"
#include "ark/simulation/ai/rules/world_perception.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace ark::simulation::persistence_detail {
namespace {
using State = StartupWorldRuntimeState;
using Page = ref::WorldScriptPage;
template <class Map, class Key>
const typename Map::mapped_type *get(const Map &map, const Key &key) {
    const auto found = map.find(key);
    return found == map.end() ? nullptr : &found->second;
}
template <class Map, class Keys> bool exact_keys(const Map &map, const Keys &keys) {
    if (map.size() != keys.size())
        return false;
    for (const auto &id : keys)
        if (!map.count(id))
            return false;
    return true;
}
template <class Map, class Keys>
bool retained_facility_keys(const Map &map, const Keys &active, std::uint64_t next_identity) {
    for (const auto id : active)
        if (!map.count(id))
            return false;
    for (const auto &[id, value] : map) {
        (void)value;
        if (!id || id >= next_identity)
            return false;
    }
    return true;
}
template <class Range, class Predicate> bool every(const Range &range, Predicate predicate) {
    return std::all_of(range.begin(), range.end(), predicate);
}
bool counter(int n) { return n >= 0 && n < std::numeric_limits<int>::max(); }
bool finite(ref::CombatPoint p) {
    return std::isfinite(p.x) && std::isfinite(p.z) && std::isfinite(p.height);
}
bool selected(int selection, std::size_t count) {
    return selection >= 0 &&
           (count == 0 ? selection == 0 : static_cast<std::size_t>(selection) < count);
}
bool scrolling(int selection, int scroll, std::size_t count, int rows) {
    return selected(selection, count) && scroll >= 0 && scroll <= selection &&
           selection - scroll < rows &&
           static_cast<std::size_t>(scroll) <=
               (count > static_cast<unsigned>(rows) ? count - rows : 0);
}
template <class Range, class Predicate>
bool unique_references(const Range &range, Predicate valid) {
    std::set<typename Range::value_type> seen;
    for (const auto &id : range)
        if (!seen.insert(id).second || !valid(id))
            return false;
    return true;
}
struct Validation {
    const State &s;
    std::string &reason;
    std::set<int> humans, facilities, items, tasks, activities, monsters;
    std::map<std::uint64_t, const Page *> pages;
    std::map<int, const StartupDefinition *> facility_defs;
    bool fail(const std::string &what) {
        reason = what;
        return false;
    }
    bool human(int id) const { return humans.count(id) != 0; }
    bool profession(int id) const {
        return id >= 0 && static_cast<std::size_t>(id) < s.rules->jobs.size();
    }
    const Page *page(std::uint64_t id) const {
        const auto p = get(pages, id);
        return p ? *p : nullptr;
    }
    bool page_kind(std::uint64_t id, std::initializer_list<int> raw) const {
        const auto p = page(id);
        return p && p->kind == ref::WorldScriptPageKind::raw_page &&
               std::find(raw.begin(), raw.end(), p->legacy_page) != raw.end();
    }
    bool live_page(std::uint64_t id) const {
        const auto p = page(id);
        return p && p->lifecycle != 4;
    }
    bool parent(std::uint64_t child, std::uint64_t owner, int raw) const {
        const auto p = page(owner);
        if (!p || !page_kind(owner, {raw}) || p->lifecycle == 4)
            return false;
        for (const auto &entry : s.scripts.pages) {
            if (entry.id == child)
                return false;
            if (entry.id == owner)
                return true;
        }
        return false;
    }
    bool equipment(int slot, int id) const {
        if (slot == 4)
            return items.count(id);
        if (slot < 0 || slot > 3)
            return false;
        return std::any_of(s.rules->equipment.begin(), s.rules->equipment.end(),
                           [&](const auto &e) {
                               const int actual = e.shop.kind == 1   ? 0
                                                  : e.shop.kind == 3 ? 3
                                                  : e.shop.type == 2 ? 1
                                                                     : 2;
                               return actual == slot && e.shop.id == id;
                           });
    }
    bool directories() {
        // codec只能重新绑定当前固定目录；不能接受反序列化的地址或另一份可变原表。
        if (s.rules != &startup_world_rules())
            return fail("rules: 固定目录身份不匹配");
        if (!valid_startup_world_human_profiles(s))
            return fail("human_profiles: 覆盖身份/姓名/性别非法");
        for (const auto &v : s.rules->humans)
            humans.insert(v.identity);
        for (const auto &v : s.rules->facilities) {
            facilities.insert(v.id);
            facility_defs.emplace(v.id, &v);
        }
        for (const auto &v : s.rules->items)
            items.insert(v.identity);
        for (const auto &v : s.rules->tasks)
            tasks.insert(v.factory.identity);
        for (const auto &v : s.rules->activities)
            activities.insert(v.identity);
        for (const auto &v : s.rules->monsters)
            monsters.insert(v.identity);
        const auto &w = s.scene.world.world;
        const auto &ai = w.ai;
#define EXACT(map, keys)                                                                           \
    if (!exact_keys(map, keys))                                                                    \
    return fail(#map ": 目录缺项或多项")
        EXACT(s.shop_humans, humans);
        EXACT(s.human_definition_state, humans);
        EXACT(s.human_homes, humans);
        EXACT(s.human_presence, humans);
        EXACT(s.human_flags, humans);
        EXACT(s.human_calendar, humans);
        EXACT(s.human_profession_changes, humans);
        EXACT(s.human_activity_previous, humans);
        EXACT(ai.growth, humans);
        EXACT(ai.battle.humans, humans);
        EXACT(w.human_spending, humans);
        EXACT(s.scripts.humans, humans);
        EXACT(s.scripts.facilities, facilities);
        EXACT(s.facility_definitions, facilities);
        EXACT(s.facility_presence, facilities);
        EXACT(s.residence_catalog_available, facilities);
        EXACT(w.facility_uses, facilities);
        EXACT(s.facility_free_builds, facilities);
        EXACT(s.facility_unlock_counters, facilities);
        EXACT(s.facility_unlock_notices, facilities);
        EXACT(s.facility_commerce_read, facilities);
        EXACT(s.items, items);
        EXACT(s.shop_item_stock, items);
        EXACT(s.item_commerce_read, items);
        EXACT(ai.monster_growth, monsters);
        EXACT(ai.battle.monsters, monsters);
        EXACT(s.task_progress.definitions, tasks);
        EXACT(s.activity_flags, activities);
        EXACT(s.activity_counts, activities);
        EXACT(s.scripts.activities, activities);
        std::set<int> recipes;
        for (const auto &recipe : s.rules->magic_pot_recipes)
            recipes.insert(recipe.identity);
        EXACT(s.magic_pot_recipes, recipes);
        for (const auto &[id, progress] : s.magic_pot_recipes)
            if (progress.identity != id || progress.status < 0 || progress.status > 1)
                return fail("magic pot: 配方身份或共享状态非法");
        std::set<int> jobs;
        for (std::size_t n = 0; n < s.rules->jobs.size(); ++n)
            jobs.insert(static_cast<int>(n));
        EXACT(s.scripts.professions, jobs);
        if (ai.professions.size() != jobs.size())
            return fail("professions: 长度不匹配");
        std::set<std::pair<int, int>> goods;
        for (const auto &e : s.rules->equipment)
            goods.emplace(e.shop.kind, e.shop.id);
        for (const auto id : items)
            goods.emplace(0, id);
        EXACT(s.catalog, goods);
#undef EXACT
        for (int id : humans) {
            const auto profile = startup_world_human_profile(s, id);
            if (!profile || s.scripts.humans.find(id)->second.name != profile->name)
                return fail("human_profiles: 脚本姓名缓存与唯一定义覆盖失配");
            const auto &g = ai.growth.find(id)->second.definition;
            if (!profession(g.current_profession) || g.profession_levels.size() != jobs.size() ||
                s.human_profession_changes.find(id)->second.size() != jobs.size() ||
                !every(g.profession_levels, [](int n) { return n >= 0 && n <= 10; }) ||
                !every(g.spell_professions, [&](int n) { return profession(n); }))
                return fail("human: 职业索引或成长数组非法");
        }
        for (int id : facilities) {
            if (s.scripts.facilities.find(id)->second.icon != facility_defs.find(id)->second->legacy_icon)
                return fail("facility: 脚本图标与固定定义失配");
            const auto &use = w.facility_uses.find(id)->second;
            if (use.level < 1 || use.level > 5)
                return fail("facility: 共享等级非法");
        }
        for (int id : items) {
            const auto &a = s.items.find(id)->second;
            const auto &b = s.catalog.find({0, id})->second;
            if (a.inventory < 0 || a.inventory > 999 || a.inventory != b.inventory ||
                a.status != b.status || a.unlock_counter != b.unlock_counter ||
                a.newly_unlocked != b.newly_unlocked)
                return fail("item: 目录与库存不同步");
        }
        return true;
    }
    bool map_and_facilities() {
        const auto &w = s.scene.world.world;
        const auto &map = w.map;
        if (!ref::valid_world_schedule_owner(s.scene.world) ||
            s.surface.size() != map.cells.size() || s.road_patches.size() != map.cells.size() ||
            s.base_variants.size() != map.cells.size() || map.width != 24 || map.height != 24 ||
            !facilities.count(s.ground_definition) ||
            !facilities.count(s.special_ground_definition) || s.fence_level < 0 ||
            static_cast<std::size_t>(s.fence_level) >= s.rules->fences.size())
            return fail("map: 地图/共同名单/围栏边界非法");
        std::set<std::uint64_t> ids;
        for (const auto &[id, value] : w.facilities) {
            (void)value;
            ids.insert(id);
        }
#define INST(map)                                                                                  \
    if (!exact_keys(s.map, ids))                                                                   \
    return fail(#map ": 实例辅助记录缺项或多项")
        INST(neighbourhood);
        INST(neighbourhood_details);
        INST(dungeon_facilities);
#undef INST
        // 建设/移动/撤除会完整退休这些辅助记录；任务成果与期限中止只退休地图实体、
        // dungeon_facilities和邻接，以下九表保留旧实例历史，原raw/ordinal也可被新实例复用。
        // 因而活动实例仍逐项必需，历史key只接受已分配的稳定身份；恢复不执行清理。
#define RETAINED(map)                                                                              \
    if (!retained_facility_keys(s.map, ids, s.next_facility_identity))                             \
    return fail(#map ": 活动实例缺载荷或历史身份未分配")
        RETAINED(facility_original_ids);
        RETAINED(facility_ordinals);
        RETAINED(facility_residents);
        RETAINED(facility_difficulties);
        RETAINED(facility_flags);
        RETAINED(facility_monthly_cash);
        RETAINED(facility_month_age);
        RETAINED(facility_item_confirmations);
        RETAINED(facility_details);
#undef RETAINED
        for (const auto &[id, raw] : s.facility_original_ids) {
            (void)id;
            if (raw < 0)
                return fail("facility: 历史原身份非法");
        }
        for (const auto &[id, ordinal] : s.facility_ordinals) {
            (void)id;
            if (ordinal < 0)
                return fail("facility: 历史同定义序号非法");
        }
        for (const auto &[id, resident] : s.facility_residents) {
            (void)id;
            if (resident != -1 && !human(resident))
                return fail("facility: 历史住宅定义非法");
        }
        for (const auto &[id, detail] : s.facility_details) {
            (void)id;
            if (detail.resident_definition != -1 && !human(detail.resident_definition))
                return fail("facility: 历史住宅载荷定义非法");
        }
        std::vector<ref::FacilityPlacement> placements;
        std::set<int> raw_ids;
        std::set<std::pair<int, int>> ordinals;
        std::set<std::size_t> occupied;
        for (const auto &[id, f] : w.facilities) {
            const auto d = get(facility_defs, f.placement.definition_id);
            if (!id || id >= s.next_facility_identity || !d ||
                f.placement.instance_id.value != id || f.status < 0 || f.status > 2 ||
                (*d)->shape != static_cast<int>(f.placement.shape) || f.kind != (*d)->kind ||
                f.category != (*d)->category || f.detail != (*d)->detail ||
                s.facility_original_ids.find(id)->second < 0 ||
                !raw_ids.insert(s.facility_original_ids.find(id)->second).second ||
                s.facility_ordinals.find(id)->second < 0 ||
                !ordinals.emplace(f.placement.definition_id, s.facility_ordinals.find(id)->second)
                     .second)
                return fail("facility: 稳定ID/原ID/同定义序号/定义不合法");
            const auto footprint =
                ref::facility_footprint(f.placement.shape, f.placement.orientation,
                                        f.placement.anchor, map.width, map.height);
            if (footprint.error != ref::GeometryError::none)
                return fail("facility: 非法占地");
            placements.push_back(f.placement);
            for (const auto &cell : footprint.cells) {
                const auto n =
                    static_cast<std::size_t>(cell.position.y) * map.width + cell.position.x;
                const auto &binding = map.cells[n].facility;
                if (!occupied.insert(n).second || !binding || binding->instance_id.value != id ||
                    binding->definition_id != f.placement.definition_id ||
                    binding->fragment_index != cell.fragment_index)
                    return fail("facility: 格绑定或分片失配");
            }
            const int resident = s.facility_residents.find(id)->second;
            if (resident != -1 && !human(resident))
                return fail("facility: 住宅人物定义缺失");
        }
        if (ref::validate_facility_layout(placements, map.width, map.height) !=
            ref::GeometryError::none)
            return fail("facility: 实例占地冲突");
        for (const auto &[id, neighbours] : s.neighbourhood_details) {
            if (s.neighbourhood.find(id)->second != neighbours.current)
                return fail("facility: 邻接值投影失配");
            for (const auto &source : neighbours.sources) {
                const auto f = get(w.facilities, source.instance_id.value);
                if (!f || f->placement.definition_id != source.definition_id)
                    return fail("facility: 邻接源实例/定义悬空");
            }
        }
        // 新局初始设施不一定有sites；新建/任务有。期限中止会保留已退休的旧占地。
        for (const auto &[id, site] : s.sites) {
            if (!id || id >= s.next_facility_identity || site.occupied_cells.empty())
                return fail("site: 身份或占地非法");
            std::set<std::size_t> cells;
            for (const auto cell : site.occupied_cells) {
                if (cell.x < 0 || cell.y < 0 || cell.x >= map.width || cell.y >= map.height ||
                    !cells.insert(static_cast<std::size_t>(cell.y) * map.width + cell.x).second)
                    return fail("site: 占地坐标越界或重复");
            }
            const auto f = get(w.facilities, id);
            if (f) {
                const auto footprint =
                    ref::facility_footprint(f->placement.shape, f->placement.orientation,
                                            f->placement.anchor, map.width, map.height);
                if (cells.size() != footprint.cells.size() ||
                    !every(footprint.cells, [&](const auto &cell) {
                        return cells.count(static_cast<std::size_t>(cell.position.y) * map.width +
                                           cell.position.x) != 0;
                    }))
                    return fail("site: 活动实例占地失配");
            }
        }
        for (std::size_t n = 0; n < map.cells.size(); ++n) {
            const auto &surface = s.surface[n];
            const auto d = get(facility_defs, surface.definition);
            if (!d || !counter(surface.updates) || s.base_variants[n] < -128 ||
                s.base_variants[n] > 127 ||
                !ref::legacy_surface_binding_matches(map.cells[n], surface.definition, (*d)->kind,
                                                     s.ground_definition) ||
                (map.cells[n].facility && !occupied.count(n)))
                return fail("map: 非法地表/混合实例绑定");
        }
        std::set<std::uint64_t> shop_ids;
        for (const auto &[id, facility] : w.facilities)
            if (facility.category == 1 && !s.shops.count(id))
                return fail("shop: 活动商店缺载荷");
        for (const auto &[id, shop] : s.shops) {
            const auto f = get(w.facilities, id);
            if (!f || f->category != 1 || shop.category != f->detail || !shop.notices.empty())
                return fail("shop: 实例绑定或持久通知投影非法");
            shop_ids.insert(id);
        }
        if (s.shop_order.size() != shop_ids.size() ||
            !unique_references(s.shop_order, [&](auto id) { return shop_ids.count(id) != 0; }))
            return fail("shop: 原序缺失或悬空");
        return true;
    }
    bool actors_and_tasks() {
        const auto &w = s.scene.world.world;
        const auto &ai = w.ai;
        const auto actor = [&](ref::CharacterId id) -> const ref::BattleActorRecord * {
            const auto live = get(ai.battle.actors, id);
            return live ? live : get(ai.retired_actors, id);
        };
        const auto encounter = [&](std::uint64_t id) {
            return ai.encounters.count(id) || ai.retired_encounters.count(id);
        };
        if (!ai.next_actor_id || !ai.next_encounter_id || !ai.next_projectile_id ||
            !ai.next_cash_id || !ai.battle.next_object_id || !s.next_facility_identity ||
            !s.next_task_identity)
            return fail("identity: 分配器不能为零");
        std::set<ref::CharacterId> ids;
        std::set<std::pair<int, int>> live_raw;
        for (const auto *list : {&ai.battle.actors, &ai.retired_actors})
            for (const auto &[id, a] : *list) {
                if (!id.value || id.value >= ai.next_actor_id || !(id == a.id) ||
                    !ids.insert(id).second ||
                    (a.kind != ref::ActorKind::human && a.kind != ref::ActorKind::monster) ||
                    (a.kind == ref::ActorKind::human ? !human(a.definition)
                                                     : !monsters.count(a.definition)) ||
                    !finite(a.position) || !finite(a.attack_position) ||
                    !finite(a.attack_destination) || !finite(a.decision_start) ||
                    !std::isfinite(a.vertical_velocity) || !std::isfinite(a.perceived_distance) ||
                    a.control.state < 0 || a.control.state > 20 || a.control.action < 0 ||
                    a.control.action > 11 || a.control.action_counter < 0 ||
                    a.control.alternate_counter < 0 || a.control.facing < 0 ||
                    a.control.facing > 3 || !every(a.control.queue, ref::valid_actor_control))
                    return fail("actor: 身份/定义/位置/控制队列非法");
                if (list == &ai.battle.actors &&
                    (a.legacy_id < 0 ||
                     !live_raw.emplace(static_cast<int>(a.kind), a.legacy_id).second))
                    return fail("actor: 活跃原UID冲突");
                for (auto target : {a.rescue, a.follow, a.perceived_enemy})
                    if (target && !actor(*target))
                        return fail("actor: 悬空人物引用");
                for (auto target : {a.encounter, a.group})
                    if (target && !encounter(*target))
                        return fail("actor: 悬空遭遇引用");
            }
        for (const auto id : ids) {
            if (!ai.contexts.count(id) || !w.actors.count(id) || !s.actor_metadata.count(id))
                return fail("actor: 缺少运行辅助字段");
            if (!ref::valid_actor_effect_state(ai.contexts.find(id)->second.effects) ||
                !finite(s.actor_metadata.find(id)->second.render_position))
                return fail("actor: 表现载荷非法");
            if (actor(id)->kind == ref::ActorKind::human &&
                (!s.dungeon_actors.count(id) || !s.shop_actors.count(id)))
                return fail("actor: 缺少商店/探索辅助载荷");
            if (actor(id)->kind == ref::ActorKind::human) {
                const auto profile = startup_world_human_profile(s, actor(id)->definition);
                if (!profile || s.actor_metadata.find(id)->second.sex != profile->sex)
                    return fail("human_profiles: 人物实例性别缓存失配");
            }
            const auto &route = w.actors.find(id)->second;
            const auto *path = route.journey         ? &route.journey->route
                               : route.unbound_route ? &*route.unbound_route
                                                     : nullptr;
            if (route.journey && route.unbound_route)
                return fail("actor: 两份路线同时存在");
            if (path && route.waypoint > path->steps.size())
                return fail("actor: 路线游标越界");
        }
        for (const auto &[id, value] : ai.contexts) {
            (void)value;
            if (!ids.count(id))
                return fail("actor: 多余感知记录");
        }
        // 现有Owner保留部分辅助历史，不能借恢复偷偷清理；只要求原分配范围及结构合法。
        for (const auto &[id, value] : w.actors) {
            if (!id.value || id.value >= ai.next_actor_id ||
                !std::isfinite(value.horizontal_velocity.x) ||
                !std::isfinite(value.horizontal_velocity.z))
                return fail("actor: 历史路径身份/向量非法");
        }
        for (const auto &[id, value] : s.actor_metadata)
            if (!id.value || id.value >= ai.next_actor_id || !finite(value.render_position))
                return fail("actor: 历史表现身份/向量非法");
        const auto &focus = s.focus_actor.actor;
        if (focus.id.value != std::numeric_limits<std::uint64_t>::max() || focus.legacy_id != -1 ||
            !finite(focus.position) || !finite(focus.attack_position) ||
            !finite(focus.attack_destination) || !finite(focus.decision_start) ||
            !std::isfinite(focus.vertical_velocity) || !std::isfinite(focus.perceived_distance) ||
            !finite(s.focus_actor.metadata.render_position) ||
            !ref::valid_actor_effect_state(s.focus_actor.perception.effects) ||
            !every(focus.control.queue, ref::valid_actor_control))
            return fail("focus: 独立W身份/向量/控制载荷非法");
        for (const auto *list : {&ai.encounters, &ai.retired_encounters})
            for (const auto &[id, e] : *list) {
                if (id >= ai.next_encounter_id || e.runtime.id != id ||
                    (list == &ai.retired_encounters && ai.encounters.count(id)))
                    return fail("encounter: 身份失配");
                for (const auto member : e.members)
                    if (!actor(member))
                        return fail("encounter: 缺少保留成员");
                for (const auto *members : {&e.group.humans, &e.group.monsters})
                    for (const auto &member : *members)
                        if (!actor(member.id))
                            return fail("encounter: 战斗组成员悬空");
                if (e.influence && !ref::valid_combat_influence_field(*e.influence))
                    return fail("encounter: 非法影响场");
            }
        for (const auto &[id, p] : ai.projectiles)
            if (id >= ai.next_projectile_id || !actor(p.caster) || !actor(p.original_target) ||
                !finite(p.position) || !finite(p.velocity) || !finite(p.acceleration))
                return fail("projectile: 身份/引用/向量非法");
        for (const auto &[id, value] : ai.battle.objects) {
            if (!id || id >= ai.battle.next_object_id || value.id.value != id ||
                !finite(value.position) || !finite(value.velocity) || !finite(value.acceleration))
                return fail("object: 稳定ID或向量非法");
        }
        for (auto id : ai.external_actor_roots)
            if (!actor(id))
                return fail("roots: 人物根悬空");
        for (auto id : ai.facility_actor_roots)
            if (!actor(id))
                return fail("roots: 设施人物根悬空");
        for (auto id : ai.external_encounter_roots)
            if (!encounter(id))
                return fail("roots: 遭遇根悬空");
        for (const auto &[id, t] : s.tasks)
            if (!id || id >= s.next_task_identity || id != t.identity ||
                !tasks.count(t.definition) || !s.task_original_ids.count(id))
                return fail("task: 稳定ID/定义/原ID缺失");
        // 原bq允许重复引用，结束任务可保留于tasks中；不强制一一对应或删除历史。
        for (auto id : s.task_order)
            if (!s.tasks.count(id))
                return fail("task: 顺序表悬空");
        if (s.active_task && !s.tasks.count(*s.active_task))
            return fail("task: 活动任务悬空");
        if (!every(s.participants, [&](int id) { return human(id); }))
            return fail("task: 参与定义缺失");
        if (s.task.encounter && !encounter(*s.task.encounter))
            return fail("task: 活动遭遇悬空");
        if (s.build_moving_facility && !w.facilities.count(*s.build_moving_facility))
            return fail("build: 移动对象不存在");
        if (s.build_definition && !facilities.count(*s.build_definition))
            return fail("build: 建设定义不存在");
        return true;
    }
    bool page_payload_keys() {
#define PAGE_MAP(map)                                                                              \
    for (const auto &[id, value] : s.map) {                                                        \
        (void)value;                                                                               \
        if (!page(id))                                                                             \
            return fail(#map ": 孤立页面载荷");                                                    \
    }
        PAGE_MAP(page_counters);
        PAGE_MAP(page_phases);
        PAGE_MAP(information_page_data);
        PAGE_MAP(human_detail_contexts);
        PAGE_MAP(page_human_bindings);
        PAGE_MAP(task_abort_questions);
        PAGE_MAP(task_abort_answers);
        PAGE_MAP(human_page_catalogs);
        PAGE_MAP(equipment_page_catalogs);
        PAGE_MAP(human_page_selections);
        PAGE_MAP(page_job_bindings);
        PAGE_MAP(human_page_parents);
        PAGE_MAP(human_page_answers);
        PAGE_MAP(human_equipment_choices);
        PAGE_MAP(human_gift_scores);
        PAGE_MAP(human_gift_messages);
        PAGE_MAP(tax_page_residents);
        PAGE_MAP(tax_page_selection);
        PAGE_MAP(tax_page_scroll);
        PAGE_MAP(activity_page_bindings);
        PAGE_MAP(activity_page_lists);
        PAGE_MAP(activity_page_display_humans);
        PAGE_MAP(activity_page_parents);
        PAGE_MAP(activity_page_answers);
        PAGE_MAP(activity_page_selections);
        PAGE_MAP(activity_page_scroll);
        PAGE_MAP(crew_summaries);
        PAGE_MAP(task_page_lists);
        PAGE_MAP(task_recruitment_pages);
        PAGE_MAP(task_extra_pages);
        PAGE_MAP(task_page_predictions);
        PAGE_MAP(task_page_acceleration);
        PAGE_MAP(page_secondary_counters);
        PAGE_MAP(award_rankings);
        PAGE_MAP(award_announced);
        PAGE_MAP(award_termination_pending);
        PAGE_MAP(award_pending_humans);
        PAGE_MAP(facility_page_bindings);
        PAGE_MAP(facility_definition_page_bindings);
        PAGE_MAP(facility_page_neighbours);
        PAGE_MAP(build_page_catalogs);
        PAGE_MAP(residence_page_candidates);
        PAGE_MAP(facility_item_page_items);
        PAGE_MAP(facility_item_page_lists);
        PAGE_MAP(facility_item_page_selections);
        PAGE_MAP(commerce_page_data);
        PAGE_MAP(commerce_page_lists);
        PAGE_MAP(facility_catalog_page_data);
        PAGE_MAP(facility_catalog_page_lists);
        PAGE_MAP(facility_catalog_page_parents);
        PAGE_MAP(magic_pot_page_data);
        PAGE_MAP(magic_pot_page_lists);
        PAGE_MAP(magic_pot_page_parents);
        PAGE_MAP(rank_celebration_participants);
        PAGE_MAP(exploration_summaries);
#undef PAGE_MAP
#define PAGE_SET(set, ...)                                                                         \
    for (auto id : s.set)                                                                          \
        if (!page_kind(id, {__VA_ARGS__}))                                                         \
    return fail(#set ": 初始化标记页型不匹配")
        PAGE_SET(human_pages_initialized, 60, 61, 62, 63, 64, 65, 66, 68, 69, 70, 73);
        PAGE_SET(activity_pages_initialized, 51, 52, 53, 54);
        PAGE_SET(facility_item_pages_initialized, 75, 76, 77);
        PAGE_SET(commerce_pages_initialized, 83, 84, 85, 86, 93);
        PAGE_SET(facility_catalog_pages_initialized, 72, 79, 82);
        PAGE_SET(magic_pot_pages_initialized, 41, 42, 43, 44, 45, 46, 47);
        PAGE_SET(task_display_initialized, 99, 100);
        PAGE_SET(facility_upgrade_initialized, 81);
#undef PAGE_SET
        for (const auto &[id, context] : s.human_detail_contexts) {
            (void)context;
            if (!page_kind(id, {60}) || !valid_startup_world_human_detail_context(s, id))
                return fail("human detail: 来源或实例上下文非法");
        }
        for (const auto &[id, data] : s.information_page_data) {
            (void)data;
            if (!page_kind(id, {35, 37, 38}))
                return fail("information page: 目录数据附在错误页型");
        }
        for (const auto &[id, data] : s.magic_pot_page_data) {
            (void)data;
            if (!page_kind(id, {41, 42, 43, 44, 45, 46, 47}))
                return fail("magic pot: 数据附在错误页型");
        }
        for (const auto &[id, list] : s.magic_pot_page_lists) {
            (void)list;
            if (!page_kind(id, {41, 42, 43, 44, 45, 46, 47}))
                return fail("magic pot: 目录附在错误页型");
        }
        for (const auto &[id, owner] : s.magic_pot_page_parents) {
            (void)owner;
            if (!page_kind(id, {42, 43, 44, 47}))
                return fail("magic pot: 父绑定附在错误页型");
        }
        for (const auto &[id, data] : s.facility_catalog_page_data) {
            (void)data;
            if (!page_kind(id, {72, 79, 82}))
                return fail("facility catalogue page: 数据附在错误页型");
        }
        for (const auto &[id, list] : s.facility_catalog_page_lists) {
            (void)list;
            if (!page_kind(id, {72, 79, 82}))
                return fail("facility catalogue page: 目录附在错误页型");
        }
        for (const auto &[id, parent] : s.facility_catalog_page_parents) {
            (void)parent;
            if (!page_kind(id, {72}))
                return fail("facility catalogue page: 父绑定附在错误页型");
        }
        for (const auto &[id, n] : s.page_counters)
            if (!counter(n))
                return fail("page: 计数非法");
        for (const auto &[id, n] : s.page_secondary_counters)
            if (!counter(n))
                return fail("page: 独立计数非法");
        for (const auto &[id, n] : s.page_phases)
            if (n < 0)
                return fail("page: 阶段非法");
        for (const auto &[id, h] : s.page_human_bindings)
            if (!human(h))
                return fail("page: 人物定义悬空");
        for (const auto &[id, j] : s.page_job_bindings)
            if (!profession(j))
                return fail("page: 职业定义悬空");
        for (const auto &[id, f] : s.facility_page_bindings)
            if (live_page(id) && !s.scene.world.world.facilities.count(f))
                return fail("page: 设施实例悬空");
        for (const auto &[id, f] : s.facility_definition_page_bindings)
            if (!facilities.count(f))
                return fail("page: 设施定义悬空");
        for (const auto &[id, p] : s.human_page_parents) {
            if (!live_page(id))
                continue;
            const auto child = page(id);
            const auto owner = page(p);
            if (!child || !owner || !parent(id, p, child->legacy_page == 62 ? 61 : 64) ||
                !s.page_human_bindings.count(id) || !s.page_human_bindings.count(p) ||
                s.page_human_bindings.find(id)->second != s.page_human_bindings.find(p)->second)
                return fail("human page: 错误父页或人物绑定");
            if (child->legacy_page == 65) {
                const auto choice = get(s.human_equipment_choices, id),
                           owner_choice = get(s.human_equipment_choices, p);
                if (!choice || !owner_choice || *choice != *owner_choice)
                    return fail("human page: 装备确认与父页选择失配");
            }
        }
        for (const auto &[id, n] : s.human_page_answers)
            if (!page_kind(id, {61, 64}) || n < 0 || n > 1)
                return fail("human page: 答案无效");
        for (const auto &[id, p] : s.task_abort_questions)
            if (live_page(id) && (!page_kind(id, {1}) || !parent(id, p, 4)))
                return fail("task page: 取消询问父页无效");
        for (const auto &[id, answer] : s.task_abort_answers)
            if (!page_kind(id, {4}) || answer < 0 || answer > 1)
                return fail("task page: 取消答案无效");
        for (const auto &[id, owner] : s.activity_page_parents)
            if (live_page(id) && (!page_kind(id, {52}) || !parent(id, owner, 51)))
                return fail("activity page: 父页引用非法");
        for (const auto &[id, answer] : s.activity_page_answers)
            if (!page_kind(id, {51}) || answer < 0 || answer > 1)
                return fail("activity page: 答案非法");
        for (const auto &[id, list] : s.task_extra_pages)
            if (!page_kind(id, {27}) || !unique_references(list, [&](int n) { return human(n); }))
                return fail("task page: 追加人物目录非法");
        for (const auto &[id, prediction] : s.task_page_predictions)
            if (!page_kind(id, {28}) || prediction < 0 || prediction > 6)
                return fail("task page: 预测值非法");
        for (const auto &[id, records] : s.rank_celebration_participants) {
            if (!page_kind(id, {50}) || !s.page_counters.count(id) || !s.page_phases.count(id) ||
                s.page_phases.find(id)->second > 2)
                return fail("rank page: 已初始化庆典缺载荷");
            for (const auto &record : records)
                if (!human(record[0]))
                    return fail("rank page: 庆典人物缺失");
        }
        for (const auto &[id, summary] : s.exploration_summaries)
            if (!page_kind(id, {30, 32}) || !s.tasks.count(summary.task) ||
                !tasks.count(summary.definition) || summary.legacy_page != page(id)->legacy_page)
                return fail("task page: 成果摘要引用非法");
        for (const auto &[id, list] : s.crew_summaries)
            if (!page_kind(id, {31}) || !every(list, [&](int n) { return human(n); }))
                return fail("task page: 结果队伍定义非法");
        for (const auto &[id, list] : s.residence_page_candidates)
            if (!page_kind(id, {80}) || !unique_references(list, [&](int n) { return human(n); }))
                return fail("residence page: 候选名单非法");
        for (const auto &[id, lists] : s.build_page_catalogs) {
            if (!page_kind(id, {21}))
                return fail("build page: 目录页型错误");
            for (const auto &list : lists)
                if (!unique_references(list, [&](int n) { return facilities.count(n) != 0; }))
                    return fail("build page: 目录定义非法");
        }
        const auto deadline_exists = [&](std::uint64_t id) {
            return page_kind(id, {33}) ||
                   (s.deadline_closed_page && s.deadline_closed_page->id == id && id != 0 &&
                    id < s.scripts.next_page_id &&
                    s.deadline_closed_page->kind == ref::WorldScriptPageKind::raw_page &&
                    s.deadline_closed_page->legacy_page == 33 &&
                    s.deadline_closed_page->lifecycle == 4);
        };
        if (s.deadline_closed_page &&
            (!deadline_exists(s.deadline_closed_page->id) || !s.deadline_page ||
             *s.deadline_page != s.deadline_closed_page->id ||
             (s.deadline_closed_page->task_identity &&
              !s.tasks.count(*s.deadline_closed_page->task_identity))))
            return fail("deadline page: 退休页身份/任务引用非法");
        for (const auto id : s.deadline_initialized)
            if (!deadline_exists(id) || !s.deadline_grades.count(id) ||
                !s.deadline_returns.count(id))
                return fail("deadline page: 初始化标记/保留载荷非法");
        for (const auto &[id, grade] : s.deadline_grades)
            if (!deadline_exists(id) || grade < 0 || grade > 4)
                return fail("deadline page: 等级/保留页非法");
        for (const auto &[id, answer] : s.deadline_returns)
            if (!deadline_exists(id) || answer < -1 || answer > 1)
                return fail("deadline page: 答案/保留页非法");
        if (s.deadline_page && !deadline_exists(*s.deadline_page))
            return fail("deadline page: 原页引用悬空");
        return true;
    }
    bool human_page(const Page &p) {
        const int raw = p.legacy_page;
        const auto id = p.id;
        if (!s.page_human_bindings.count(id))
            return fail("human page: 缺人物绑定");
        if (raw == 60 && !valid_startup_world_human_detail_context(s, id))
            return fail("human detail: 缺来源或实例上下文非法");
        if ((raw == 62 || raw == 63) && !s.page_job_bindings.count(id))
            return fail("human page: 缺职业绑定");
        if (raw == 62 && !s.human_page_parents.count(id))
            return fail("human page: 缺父页");
        if (raw == 65 && !s.human_page_parents.count(id))
            return fail("human page: 缺装备确认父页");
        if (raw == 65 || raw == 66 || raw == 69 || raw == 73) {
            const auto choice = get(s.human_equipment_choices, id);
            if (!choice || !equipment((*choice)[0], (*choice)[1]) ||
                (raw == 65 && (*choice)[0] > 3) || (raw == 69 && (*choice)[0] != 4))
                return fail("human page: 道具/装备选择非法");
        }
        if (raw == 69) {
            const int item = s.human_equipment_choices.find(id)->second[1];
            const auto d = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                        [&](const auto &v) { return v.identity == item; });
            if (d == s.rules->items.end() || d->effect < 0 || d->effect >= 6)
                return fail("human page: 普通道具效果类型非法");
        }
        if (raw == 66 && (!s.human_gift_scores.count(id) || !s.human_gift_messages.count(id)))
            return fail("human page: 缺赠礼结果");
        const bool initialized = s.human_pages_initialized.count(id);
        if (!initialized)
            return p.lifecycle == 0 || fail("human page: 已开始却缺初始化标记");
        const auto phase = get(s.page_phases, id), selection = get(s.human_page_selections, id);
        if (!phase || !selection || !s.page_counters.count(id) || *selection < 0)
            return fail("human page: 已初始化载荷缺失");
        if (raw == 61 || raw == 62) {
            const auto list = get(s.human_page_catalogs, id);
            if (!list || list->empty() || !selected(*selection, list->size()) ||
                !unique_references(*list, [&](int j) { return profession(j); }))
                return fail("human page: 职业目录或选择越界");
            if (raw == 62 && (*list)[*selection] != s.page_job_bindings.find(id)->second)
                return fail("human page: 职业选择与绑定失配");
        }
        if (raw == 64 || raw == 73) {
            const auto lists = get(s.equipment_page_catalogs, id);
            if (!lists || *phase < 0 || *phase > (raw == 64 ? 4 : 3) ||
                !selected(*selection, (*lists)[*phase].size()))
                return fail("human page: 装备分页或选择越界");
            for (int slot = 0; slot < 5; ++slot)
                if (!unique_references((*lists)[slot], [&](int v) { return equipment(slot, v); }))
                    return fail("human page: 装备目录非法");
        }
        if (raw == 63 && !s.human_gift_messages.count(id))
            return fail("human page: 缺转职显示载荷");
        return true;
    }
    bool activity_page(const Page &p) {
        const auto id = p.id;
        const int raw = p.legacy_page;
        if (raw != 51) {
            const auto binding = get(s.activity_page_bindings, id);
            if (!binding || !activities.count(*binding))
                return fail("activity page: 缺活动绑定");
            const auto d = std::find_if(s.rules->activities.begin(), s.rules->activities.end(),
                                        [&](const auto &v) { return v.identity == *binding; });
            if (d->parameters[2] < 0 || d->parameters[2] > 6 || d->parameters[2] == 4 ||
                (raw == 54 && d->parameters[2] > 1))
                return fail("activity page: 未接效果类型");
        }
        if (!s.activity_pages_initialized.count(id))
            return p.lifecycle == 0 || fail("activity page: 缺初始化标记");
        const auto selection = get(s.activity_page_selections, id),
                   scroll = get(s.activity_page_scroll, id);
        if (!selection || !scroll || !s.page_counters.count(id))
            return fail("activity page: 已初始化缺载荷");
        if (raw == 52) {
            const auto owner = get(s.activity_page_parents, id);
            if (!owner || !parent(id, *owner, 51) || !selected(*selection, 2) || *scroll != 0)
                return fail("activity page: 确认父页或选择非法");
            const auto list = get(s.activity_page_lists, *owner);
            const auto choice = get(s.activity_page_selections, *owner);
            if (!list || !choice || !selected(*choice, list->size()) || list->empty() ||
                (*list)[*choice] != s.activity_page_bindings.find(id)->second)
                return fail("activity page: 父页所选活动失配");
            return true;
        }
        if (raw == 53)
            return (*selection == 0 && *scroll == 0) || fail("activity page: 演出选择非法");
        const auto list = get(s.activity_page_lists, id);
        const auto display = get(s.activity_page_display_humans, id);
        if (!list || !display || !scrolling(*selection, *scroll, list->size(), 5) ||
            !unique_references(
                *list, [&](int n) { return raw == 51 ? activities.count(n) != 0 : human(n); }) ||
            !human((*display)[0]) || !human((*display)[1]))
            return fail("activity page: 目录/显示人物/选择非法");
        return true;
    }
    bool commerce_page(const Page &p) {
        const auto id = p.id;
        const int raw = p.legacy_page;
        if (!s.commerce_pages_initialized.count(id))
            return p.lifecycle == 0 || fail("commerce page: 缺初始化标记");
        const auto v = get(s.commerce_page_data, id);
        if (!v || !s.page_counters.count(id))
            return fail("commerce page: 已初始化缺载荷");
        const auto &a = *v;
        if (a[0] < 0 || a[0] > 1 || a[1] < 0 || a[1] > 1 || a[5] < 0 || a[5] > 20 ||
            (raw != 84 && raw != 86 && a[0] != 0) || ((raw != 84 || a[0] == 1) && a[1] != 0) ||
            (raw != 84 && a[5] != 0))
            return fail("commerce page: 模式/分页非法");
        if (raw == 83)
            return (selected(a[2], 3) && a[3] == 0 && a[4] == -1) ||
                   fail("commerce page: 菜单越界");
        if (raw == 86 || raw == 93)
            return (a[2] == 0 && a[3] == 0 && a[4] == p.legacy_s &&
                    (raw == 86 ? items.count(a[4]) && a[0] == p.legacy_f
                               : facilities.count(a[4]) && p.legacy_r == 3)) ||
                   fail("commerce page: 条目绑定非法");
        const auto list = get(s.commerce_page_lists, id);
        if (!list || list->empty() || a[4] != -1 ||
            !scrolling(a[2], a[3], list->size(), raw == 85 ? 3 : 5) ||
            !unique_references(*list, [&](int n) {
                return raw == 84 ? items.count(n) != 0 : facilities.count(n) != 0;
            }))
            return fail("commerce page: 目录/选择非法");
        if (raw == 84 && a[0] != p.legacy_f)
            return fail("commerce page: 模式绑定失配");
        for (const auto definition : *list) {
            if (raw == 84) {
                if ((a[0] == 0 ? s.shop_item_stock.find(definition)->second.quantity
                               : s.items.find(definition)->second.inventory) <= 0)
                    return fail("commerce page: 列表条目没有可消费库存");
            } else {
                const auto *d = facility_defs.find(definition)->second;
                if (s.facility_presence.find(definition)->second == 2 || d->unlock_rank < 0 ||
                    d->unlock_rank > s.rank)
                    return fail("commerce page: 列表设施尚未开放");
            }
        }
        return true;
    }
    bool page_payloads() {
        if (s.scripts.finance || s.scripts.medal_count || s.scripts.pending_completion ||
            !s.scripts.popularity_queue.empty() || !s.scripts.human_order.empty() ||
            s.scripts.scene_mode || s.scripts.scene_updates || s.scripts.exploration_phase)
            return fail("scripts: 临时投影不能成为第二份权威");
        if (ref::validate_world_script_state(startup_world_runtime_catalog(), s.scripts) !=
            ref::WorldScriptError::none)
            return fail("scripts: 页身份/续体校验失败");
        std::size_t scenes{};
        for (const auto &p : s.scripts.pages) {
            pages.emplace(p.id, &p);
            if (p.kind == ref::WorldScriptPageKind::scene)
                ++scenes;
            if (static_cast<int>(p.kind) < 0 ||
                static_cast<int>(p.kind) > static_cast<int>(ref::WorldScriptPageKind::raw_page))
                return fail("page: 非法类型");
            if (p.task_identity && !s.tasks.count(*p.task_identity))
                return fail("page: 任务引用悬空");
            if (p.task_definition && !tasks.count(*p.task_definition))
                return fail("page: 任务定义不存在");
            if (p.task_identity && p.task_definition &&
                s.tasks.find(*p.task_identity)->second.definition != *p.task_definition)
                return fail("page: 任务实例与定义失配");
            if (p.monster_definition && !monsters.count(*p.monster_definition))
                return fail("page: 怪物定义不存在");
            if (p.facility_definition &&
                (p.kind != ref::WorldScriptPageKind::raw_page || p.legacy_page != 82 ||
                 !facilities.count(*p.facility_definition)))
                return fail("page: 设施定义绑定不存在或不属于82");
        }
        if (scenes != 1 || !page_payload_keys())
            return scenes != 1 ? fail("page: 必须有唯一主场景") : false;
        for (const auto &p : s.scripts.pages) {
            if (p.kind == ref::WorldScriptPageKind::raw_page &&
                (p.legacy_page == 9 || (p.legacy_page >= 35 && p.legacy_page <= 38)) &&
                !valid_startup_world_information_page(s, p.id))
                return fail("information page: 初始化/页签/选择/计数载荷非法");
            if (p.lifecycle == 4 || p.kind == ref::WorldScriptPageKind::scene)
                continue;
            const auto id = p.id;
            const int raw = p.legacy_page;
            if (p.kind != ref::WorldScriptPageKind::raw_page) {
                const auto phase = get(s.page_phases, id);
                if (phase && !selected(*phase, p.paragraphs.size()))
                    return fail("page: 文本段选择越界");
                continue;
            }
            if (raw == 11) {
                const auto phase = get(s.page_phases, id);
                if (p.message_commands.empty() ||
                    p.message_commands.size() != p.paragraphs.size() ||
                    !selected(phase ? *phase : 0, p.message_commands.size()))
                    return fail("message page: 段落/命令/选择失配");
            }
            if (raw == 59 && !human(p.legacy_f))
                return fail("human page: 解锁人物不存在");
            if ((raw == 67 || raw == 88) && !s.page_human_bindings.count(id))
                return fail("human page: 奖励显示缺人物绑定");
            if (raw == 94 || raw == 95) {
                const int mode = p.legacy_r, definition = p.legacy_s;
                const bool supported = raw == 94
                                           ? mode == 3 || (mode >= 5 && mode <= 8)
                                           : mode == 0 || mode == 1 || mode == 3 || mode == 4 ||
                                                 mode == 9 || mode == 10 || mode == 11;
                if (!supported || definition < 0 || (mode == 3 && !facilities.count(definition)) ||
                    (mode == 4 && !profession(definition)) ||
                    (mode == 11 && !activities.count(definition)) ||
                    (mode >= 5 && mode <= 8 &&
                     !s.catalog.count({mode == 8 ? 0 : mode - 4, definition})))
                    return fail("gift page: 奖励类型/定义非法");
            }
            if ((raw >= 60 && raw <= 66) || raw == 68 || raw == 69 || raw == 70 || raw == 73) {
                if (!human_page(p))
                    return false;
            } else if (raw >= 51 && raw <= 54) {
                if (!activity_page(p))
                    return false;
            } else if (raw >= 41 && raw <= 47) {
                if (!valid_startup_world_magic_pot_page(s, p))
                    return fail("magic pot page: 初始化/绑定/选择载荷非法");
            } else if (raw == 83 || raw == 84 || raw == 85 || raw == 86 || raw == 93) {
                if (!commerce_page(p))
                    return false;
            } else if (raw == 74) {
                if (!valid_startup_world_facility_page(s, p))
                    return fail("facility page: 详情载荷非法");
            } else if (raw >= 75 && raw <= 77) {
                if (!valid_startup_world_facility_item_page(s, p))
                    return fail("facility item page: 载荷非法");
            } else if (raw == 72 || raw == 79 || raw == 82) {
                if (!valid_startup_world_facility_catalog_page(s, p))
                    return fail("facility catalogue page: 初始化/绑定/选择载荷非法");
            }
            if (raw == 90 && (s.tax_page_selection.count(id) || s.tax_page_scroll.count(id)) &&
                !s.tax_page_residents.count(id))
                return fail("tax page: 初始化名单缺失");
            if (raw == 90 && s.tax_page_residents.count(id)) {
                const auto &list = s.tax_page_residents.find(id)->second;
                const auto sel = get(s.tax_page_selection, id), scroll = get(s.tax_page_scroll, id);
                if (!sel || !scroll || !s.page_counters.count(id) ||
                    !scrolling(*sel, *scroll, list.size(), 5) ||
                    !unique_references(list, [&](int n) { return human(n); }))
                    return fail("tax page: 已初始化名单/选择非法");
            }
            if (raw == 87 &&
                (s.award_announced.count(id) || s.award_termination_pending.count(id) ||
                 s.award_pending_humans.count(id)) &&
                !s.award_rankings.count(id))
                return fail("award page: 初始化排名缺失");
            if (raw == 87 && s.award_rankings.count(id)) {
                const auto &ranked = s.award_rankings.find(id)->second;
                if (!s.page_counters.count(id) || !s.award_announced.count(id) ||
                    !s.award_termination_pending.count(id) ||
                    !unique_references(ranked, [&](int n) { return human(n); }))
                    return fail("award page: 已初始化载荷缺失");
                std::set<int> active;
                for (const auto &[human, status] : s.human_presence)
                    if (status != 0)
                        active.insert(human);
                const auto pending = get(s.award_pending_humans, id);
                if (active.empty() || std::set<int>(ranked.begin(), ranked.end()) != active ||
                    (pending &&
                     (!active.count(*pending) || s.award_termination_pending.find(id)->second)))
                    return fail("award page: 排名/待授予人物与在籍名单失配");
            }
            if (raw == 21) {
                const auto lists = get(s.build_page_catalogs, id);
                const auto phase = get(s.page_phases, id);
                if (!lists || !phase || !s.page_counters.count(id) ||
                    !selected(*phase, lists->size()))
                    return fail("build page: 缺建设目录/分页/计数");
            }
            if (raw == 80 &&
                (!s.facility_page_bindings.count(id) || !s.residence_page_candidates.count(id))) {
                return fail("residence page: 缺原设施/候选名单");
            }
            if (raw == 22) {
                const auto list = get(s.task_page_lists, id);
                if (!list || !s.page_counters.count(id) ||
                    !every(*list, [&](auto n) { return s.tasks.count(n) != 0; }))
                    return fail("task page: 缺任务目录");
            }
            if ((raw >= 23 && raw <= 28) || raw == 4) {
                if (!p.task_identity || !s.page_counters.count(id))
                    return fail("task page: 缺任务/计数绑定");
            }
            if (raw == 24) {
                const auto r = get(s.task_recruitment_pages, id);
                if (!r || !s.page_secondary_counters.count(id) || !counter(r->portrait_timer) ||
                    r->arrival_extent < 0 || r->completion_tick < 0 || r->displayed_count < 0 ||
                    !every(
                        r->portraits, [&](int h) { return human(h); }) ||
                    !every(
                        r->entries,
                        [&](const auto &e) {
                            return human(e.human) && e.phrase >= -1 && e.phrase < 6;
                        }))
                    return fail("task page: 征集动画载荷非法");
            }
            if (raw == 27 && !s.task_extra_pages.count(id))
                return fail("task page: 缺追加人物名单");
            if (raw == 28 &&
                (!s.page_phases.count(id) || !s.task_page_predictions.count(id) ||
                 !s.task_page_acceleration.count(id) || s.page_phases.find(id)->second > 1))
                return fail("task page: 缺出发载荷");
            if ((raw == 99 || raw == 100) && !p.monster_definition)
                return fail("task page: 缺演出怪物定义");
            if (s.task_display_initialized.count(id) && !s.page_counters.count(id))
                return fail("task page: 已初始化演出缺计数");
            if (s.deadline_initialized.count(id) &&
                (!s.page_counters.count(id) || !s.page_phases.count(id) ||
                 s.page_phases.find(id)->second > 1 || !s.deadline_grades.count(id)))
                return fail("task page: 已初始化期限页缺载荷");
            if (s.facility_upgrade_initialized.count(id) &&
                (!s.page_counters.count(id) || !s.page_secondary_counters.count(id) ||
                 !s.page_phases.count(id) || s.page_phases.find(id)->second > 1 ||
                 !facilities.count(p.legacy_f)))
                return fail("upgrade page: 缺升级载荷");
            if ((raw == 30 || raw == 32) && !s.exploration_summaries.count(id))
                return fail("task page: 缺成果摘要");
        }
        return true;
    }
};
} // namespace

// Validate the private decoded candidate before installation; this function never repairs an invalid save.
bool validate_restored_state(const State &s, std::string &reason, bool audit_checkpoint) {
    reason.clear();
    Validation v{s, reason, {}, {}, {}, {}, {}, {}, {}, {}};
    if (!v.directories())
        return false;
    if (!ref::WorldRandomStream::from_snapshot(s.scene.random.snapshot()))
        return v.fail("random: 引擎/磁带/游标非法");
    if (!ref::valid_world_magic_pot_state(s.legacy_n,
            {s.scene.calendar.year, s.scene.calendar.month, s.scene.calendar.subperiod}) ||
        !every(s.magic_pot_output, [](int n) { return n >= 0; }) ||
        !every(s.magic_pot_display, [](const auto &row) {
            return std::all_of(row.begin(), row.end(), [](int n) { return n >= 0; });
        }))
        return v.fail("magic pot: 级别/容量/日期/演出数据非法");
    if (!ref::valid_world_calendar_state(s.scene.calendar) || s.scene.scene_state < 0 ||
        s.scene.scene_state > 7 || !counter(s.scene.frame_counter) ||
        !counter(s.scene.scene_counter) || (!audit_checkpoint && s.scene.processing_phase != -1) ||
        (audit_checkpoint && s.scene.processing_phase != -1 && s.scene.processing_phase != 1) ||
        s.scripts.executing_page || s.scripts.page_mutations_locked || s.clock_parameter != 80 ||
        s.calendar_advance != 27 || s.build_mode < 0 || s.build_mode > 7 ||
        !every(s.camera, [](float n) { return std::isfinite(n); }) ||
        !every(s.previous_camera, [](float n) { return std::isfinite(n); }) ||
        !every(s.camera_velocity, [](float n) { return std::isfinite(n); }))
        return v.fail("runtime: 边界/日期/相机/临时执行根非法");
    return v.map_and_facilities() && v.actors_and_tasks() && v.page_payloads();
}
} // namespace ark::simulation::persistence_detail
