#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_facility_items.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_reference/world_map_refresh.hpp"
#include "dungeon_village_reference/world_residence.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
const StartupDefinition *definition(const State &s, int id) {
    if (!s.rules)
        return nullptr;
    const auto found = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                    [id](const auto &d) { return d.id == id; });
    return found == s.rules->facilities.end() ? nullptr : &*found;
}
const ref::WorldScriptPage *top(const State &s) {
    const auto found = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                    [](const auto &p) { return p.lifecycle != 4; });
    return found == s.scripts.pages.rend() ? nullptr : &*found;
}
bool main(const State &s) {
    const auto *p = top(s);
    return s.rules && !s.scene.framework_paused && p && p->kind == ref::WorldScriptPageKind::scene;
}
ref::FacilityEconomyInput input(const State &s, int id) {
    ref::FacilityEconomyInput in;
    const auto &use = s.scene.world.world.facility_uses.at(id);
    in.level = use.level;
    in.completed_definition_uses = use.completed_uses;
    in.definition_improvements = s.scripts.facilities.at(id).improvements;
    // h.d()从所有p非零定义按当前职业统计，不能沿用初期3人的缓存。
    for (const auto &h : s.rules->humans)
        if (s.human_presence.at(h.identity) != 0) {
            const auto job =
                s.scene.world.world.ai.growth.at(h.identity).definition.current_profession;
            ++in.legacy_job_counts.at(s.rules->jobs.at(job).type);
        }
    return in;
}
bool refresh_map(State &s, bool initial_neighbours = false, bool notices = true,
                 bool include_neighbours = true) {
    auto &world = s.scene.world.world;
    if (!s.rules || s.surface.size() != world.map.cells.size() ||
        s.road_patches.size() != s.surface.size())
        return false;
    ref::WorldMapRefreshState m;
    m.map = world.map;
    for (std::size_t n = 0; n < s.surface.size(); ++n) {
        const auto &c = s.surface[n];
        m.surface.push_back({c.definition, c.updates, c.display_definition, c.variant, c.road_mask,
                             c.fragment, c.instance, s.road_patches.at(n)[0],
                             s.road_patches.at(n)[1]});
    }
    for (const auto &d : s.rules->facilities)
        m.definitions.emplace(d.id,
                              ref::WorldMapDefinition{d.display_id, d.kind,
                                                      static_cast<ref::FacilityShape>(d.shape),
                                                      d.neighbour_effects});
    m.ground_definition = s.ground_definition;
    m.special_ground_definition = s.special_ground_definition;
    m.base_variants = s.base_variants;
    m.fence_level = s.fence_level;
    m.fence_levels = s.rules->fences;
    for (const auto id : s.scene.world.facility_order) {
        if (!world.facilities.count(id) || !s.neighbourhood.count(id))
            return false;
        auto &cache = m.neighbours[id];
        const auto existing = s.neighbourhood_details.find(id);
        if (existing != s.neighbourhood_details.end())
            cache = existing->second;
        cache.current = s.neighbourhood.at(id);
        const auto details = s.facility_details.find(id);
        if (details == s.facility_details.end())
            return false;
        cache.notices = details->second.notices; // m.p由实际设施计时，不能回填旧缓存通知。
    }
    for (const auto id : s.scene.world.facility_order)
        m.facilities.push_back(world.facilities.at(id).placement);
    // a/o.a(z=true,z2=true)：c→d(含f)→邻接，不插入额外f调用。
    if (!initial_neighbours) {
        auto display = ref::prepare_world_map_display(m);
        if (!display.candidate)
            return false;
        auto roads = ref::prepare_world_map_roads(display.candidate->state);
        if (!roads.candidate)
            return false;
        m = roads.candidate->state;
    }
    if (include_neighbours) {
        auto neighbours = ref::prepare_world_map_neighbours(m, !initial_neighbours && notices);
        if (!neighbours.candidate)
            return false;
        m = std::move(neighbours.candidate->state);
    }
    const auto &r = m;
    world.map = r.map;
    s.neighbourhood_details = r.neighbours;
    if (!initial_neighbours)
        s.scene.first_normal_refresh = r.refresh_pending;
    s.scene.world.surface.clear();
    for (std::size_t n = 0; n < s.surface.size(); ++n) {
        const auto &c = r.surface[n];
        s.surface[n] = {c.definition, c.updates, c.instance_field, c.fragment,
                        c.display,    c.variant, c.road_mask};
        s.road_patches[n] = {c.road_quad, c.edge_road_pair};
        s.scene.world.surface.push_back(static_cast<int>(world.map.cells[n].category));
    }
    for (const auto &entry : r.neighbours) {
        s.neighbourhood[entry.first] = entry.second.current;
        s.facility_details.at(entry.first).notices = entry.second.notices;
    }
    return true;
}
int free_number(const std::set<int> &used) {
    int n{};
    while (used.count(n)) {
        if (n == std::numeric_limits<int>::max())
            return -1;
        ++n;
    }
    return n;
}
} // namespace
bool initialize_startup_world_neighbours(State &s) { return refresh_map(s, true); }
bool refresh_startup_world_map(State &s, bool notices) { return refresh_map(s, false, notices); }
bool refresh_startup_world_surface(State &s) { return refresh_map(s, false, false, false); }
bool refresh_startup_world_profession_economy(State &s) {
    if (!s.rules)
        return false;
    for (const auto &d : s.rules->facilities) {
        if (d.kind != 3)
            continue;
        const auto current_jobs = input(s, d.id);
        s.scripts.job_counts = current_jobs.legacy_job_counts; // 原o.e()->h.d()也更新共享h.C。
        const auto economy = ref::derive_facility_economy(d.economy, current_jobs);
        if (!economy.values)
            return false;
        for (std::size_t n = 0; n < 4; ++n) {
            const auto value = economy.values->definition_attributes[n];
            if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
                return false;
            s.scripts.facilities.at(d.id).attributes[n] = static_cast<int>(value);
        }
        for (auto &instance : s.scene.world.world.facilities) {
            if (instance.second.placement.definition_id != d.id)
                continue;
            auto in = input(s, d.id);
            const auto &modifiers = s.neighbourhood.at(instance.first);
            std::copy(modifiers.begin(), modifiers.end(), in.instance_modifiers.begin());
            const auto current = ref::derive_facility_economy(d.economy, in);
            if (!current.values ||
                current.values->instance_attributes[0] < std::numeric_limits<int>::min() ||
                current.values->instance_attributes[0] > std::numeric_limits<int>::max())
                return false;
            instance.second.price = static_cast<int>(current.values->instance_attributes[0]);
        }
    }
    return true;
}
std::optional<std::array<std::vector<int>, 3>> startup_world_build_catalog(const State &s) {
    if (!s.rules)
        return {};
    std::array<std::vector<int>, 3> groups;
    for (const auto &d : s.rules->facilities) {
        const auto presence = s.facility_presence.find(d.id);
        if (presence == s.facility_presence.end())
            return {};
        // 页21使用p与bit4，不用O（商店新设施标志）或截图目录。
        const auto remaining = s.facility_free_builds.find(d.id);
        if (remaining == s.facility_free_builds.end() || remaining->second < 0 ||
            remaining->second > 99)
            return {};
        if (presence->second != 0 && (d.flags & 4) && (d.kind != 12 || remaining->second > 0) &&
            d.kind != 6) {
            if (d.tab < 0 || d.tab > 2)
                return {};
            groups[d.tab].push_back(d.id);
        }
    }
    return groups;
}
StartupBuildResult begin_startup_world_build(State &s, int id) {
    if (!main(s) || s.scene.scene_state != 0)
        return {Error::invalid_page};
    const auto catalog = startup_world_build_catalog(s);
    const auto *d = definition(s, id);
    if (!catalog || !d)
        return {Error::missing_source};
    if (std::none_of(catalog->begin(), catalog->end(), [&](const auto &group) {
            return std::find(group.begin(), group.end(), id) != group.end();
        }))
        return {Error::none, StartupBuildDenial::unavailable};
    const auto quote = ref::derive_facility_economy(d->economy, input(s, id));
    if (!quote.values)
        return {Error::missing_source};
    if (quote.values->construction_cost > s.scene.world.world.ai.accounting.funds())
        return {Error::none, StartupBuildDenial::insufficient_funds};
    s.build_definition = id;
    s.build_mode = 0;
    s.build_feedback_counter = 0;
    s.build_feedback_message = "要建在哪里呢";
    s.facility_unlock_notices[id] = false;
    s.scripts.selected_facility.reset();
    s.scene.scene_state = 1;
    s.scene.scene_counter = 0;
    s.scene.first_normal_refresh = true;
    return {};
}
std::optional<ref::FacilityEconomyValues> startup_world_build_quote(const State &s, int id) {
    const auto *d = definition(s, id);
    return d ? ref::derive_facility_economy(d->economy, input(s, id)).values : std::nullopt;
}
int startup_world_facility_page_count(const State &s, const ref::WorldScriptPage &page) {
    const auto *d = definition(s, page.legacy_f);
    if (!d || page.legacy_page != 74)
        return 0;
    if (s.facility_definition_page_bindings.count(page.id))
        return 1;
    return d->kind != 2 && d->kind != 12 && d->detail != 1 && d->detail != 4 && d->detail != 5 &&
                   d->detail != 6
               ? 2
               : 1;
}
Error open_startup_world_build_menu(State &s) {
    if (!main(s) || s.scene.scene_state != 0)
        return Error::invalid_page;
    const auto catalog = startup_world_build_catalog(s);
    if (!catalog)
        return Error::missing_source;
    auto next = s;
    ref::WorldScriptPage page;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 21;
    page.title = "建设";
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(next), page);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(next, r.candidate->state))
        return Error::script_failed;
    const auto id = r.candidate->inserted_pages.front().id;
    next.build_page_catalogs[id] = *catalog;
    next.page_phases[id] = next.page_counters[id] = 0;
    s = std::move(next);
    return Error::none;
}
Error cancel_startup_world_build_menu(State &s, std::uint64_t id) {
    const auto *page = top(s);
    if (s.scene.framework_paused || !page || page->id != id || page->legacy_page != 21 ||
        !s.build_page_catalogs.count(id))
        return Error::invalid_page;
    auto next = s;
    const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
    if (!r.candidate || !write_startup_world_runtime_scripts(next, r.candidate->state))
        return Error::script_failed;
    s = std::move(next);
    return Error::none;
}
StartupBuildResult select_startup_world_build_menu(State &s, std::uint64_t page,
                                                   int definition_id) {
    const auto listing = s.build_page_catalogs.find(page);
    if (listing == s.build_page_catalogs.end() ||
        std::none_of(listing->second.begin(), listing->second.end(), [&](const auto &group) {
            return std::find(group.begin(), group.end(), definition_id) != group.end();
        }))
        return {Error::invalid_page};
    auto next = s;
    if (cancel_startup_world_build_menu(next, page) != Error::none)
        return {Error::invalid_page};
    const auto result = begin_startup_world_build(next, definition_id);
    if (result.error != Error::none || result.denial != StartupBuildDenial::none)
        return result; // 拒绝时菜单仍在，不提交候选close。
    s = std::move(next);
    return result;
}
bool refresh_startup_world_connections(State &s) {
    const auto search =
        ref::search_legacy_map(s.scene.world.world.map, startup_evidence().spawn_points.at(0));
    if (!search.field)
        return false;
    std::vector<ref::FacilityPlacement> placements;
    for (const auto id : s.scene.world.facility_order)
        placements.push_back(s.scene.world.world.facilities.at(id).placement);
    const auto access = ref::inspect_facility_access(*search.field, placements);
    if (access.error != ref::MapAccessError::none)
        return false;
    bool disconnected{};
    for (const auto &entry : access.facilities) {
        const auto *d = definition(s, entry.definition_id);
        if (!d)
            return false;
        if (d->kind != 3 && d->kind != 12)
            continue;
        auto &flags = s.facility_flags[entry.instance_id.value];
        flags &= ~1U;
        if (entry.cells.empty()) {
            flags |= 1U;
            disconnected = true;
        }
    }
    if (disconnected && !ref::world_script_seen(s.scripts, 115)) {
        const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                 startup_world_runtime_scripts(s), {115, {}, {}});
        if (!r.candidate || !write_startup_world_runtime_scripts(s, r.candidate->state))
            return false;
    }
    return true;
}
// 原a/o.a只安装实例与刷新；普通放置调用点才扣造价/播放11，入住页80已先扣人物h。
static StartupBuildResult install_facility(State &s, ref::Position anchor,
                                           ref::FacilityOrientation orientation, bool charge,
                                           int map_definition = -1, bool surface_refresh = true) {
    const bool map_creation = map_definition >= 0;
    if (!s.rules || (!map_creation && !s.build_definition))
        return {Error::missing_source};
    const auto *d = definition(s, map_creation ? map_definition : *s.build_definition);
    if (!d || (map_creation && d->kind != 4 && d->kind != 5))
        return {Error::missing_source};
    const auto &old = s.scene.world.world;
    // 地图内部创建不能通过map.at抛错掩盖缺失共享字段；仍使用真实经营定义初值。
    if (!ref::valid_legacy_map(old.map) || s.surface.size() != old.map.cells.size() ||
        !old.facility_uses.count(d->id) || !s.scripts.facilities.count(d->id) ||
        s.fence_level < 0 || static_cast<std::size_t>(s.fence_level) >= s.rules->fences.size())
        return {Error::missing_source};
    for (const auto &h : s.rules->humans) {
        const auto presence = s.human_presence.find(h.identity);
        if (presence == s.human_presence.end())
            return {Error::missing_source};
        if (presence->second == 0)
            continue;
        const auto growth = old.ai.growth.find(h.identity);
        if (growth == old.ai.growth.end())
            return {Error::missing_source};
        const int profession = growth->second.definition.current_profession;
        if (profession < 0 || static_cast<std::size_t>(profession) >= s.rules->jobs.size() ||
            s.rules->jobs[profession].type < 0 || s.rules->jobs[profession].type >= 10)
            return {Error::missing_source};
    }
    const auto footprint =
        ref::facility_footprint(static_cast<ref::FacilityShape>(d->shape), orientation, anchor,
                                old.map.width, old.map.height);
    if (footprint.error == ref::GeometryError::outside_map)
        return {Error::none, StartupBuildDenial::outside_map};
    if (footprint.error != ref::GeometryError::none)
        return {Error::missing_source};
    const auto bounds = s.rules->fences.at(s.fence_level);
    for (const auto &c : footprint.cells) {
        const auto &tile = old.map.cells.at(c.position.y * old.map.width + c.position.x);
        if (tile.facility || (!map_creation && (tile.legacy_state == 1 || tile.legacy_state == 10 ||
                                                tile.legacy_state == 2)))
            return {Error::none, StartupBuildDenial::occupied};
        if (!map_creation && (c.position.x <= bounds[0].x || c.position.x >= bounds[1].x ||
                              c.position.y >= bounds[0].y || c.position.y <= bounds[1].y))
            return {Error::none, StartupBuildDenial::outside_town};
    }
    const auto quote = ref::derive_facility_economy(d->economy, input(s, d->id));
    if (!quote.values || quote.values->construction_cost > std::numeric_limits<int>::max() ||
        quote.values->construction_ticks > std::numeric_limits<int>::max() ||
        quote.values->definition_attributes[0] < std::numeric_limits<int>::min() ||
        quote.values->definition_attributes[0] > std::numeric_limits<int>::max() ||
        s.next_facility_identity == 0 ||
        s.next_facility_identity == std::numeric_limits<std::uint64_t>::max())
        return {Error::missing_source};
    if (charge && quote.values->construction_cost > old.ai.accounting.funds())
        return {Error::none, StartupBuildDenial::insufficient_funds};
    auto next = s;
    auto &world = next.scene.world.world;
    std::set<int> raw_ids, ordinals;
    for (const auto id : next.scene.world.facility_order) {
        if (!next.facility_original_ids.count(id) || !next.facility_ordinals.count(id) ||
            !world.facilities.count(id))
            return {Error::missing_source};
        raw_ids.insert(next.facility_original_ids.at(id));
        if (world.facilities.at(id).placement.definition_id == d->id)
            ordinals.insert(next.facility_ordinals.at(id));
    }
    const int raw = free_number(raw_ids), ordinal = free_number(ordinals);
    const auto id = next.next_facility_identity++;
    if (raw < 0 || ordinal < 0 || world.facilities.count(id))
        return {Error::missing_source};
    ref::RescueFacility facility;
    facility.placement = {
        {id}, d->id, static_cast<ref::FacilityShape>(d->shape), orientation, anchor};
    facility.kind = d->kind;
    facility.category = d->category;
    facility.detail = d->detail;
    facility.price = static_cast<int>(quote.values->definition_attributes[0]);
    facility.definition_wait = d->use_wait;
    facility.upgrade_uses = d->economy.upgrade_uses;
    facility.status = !map_creation && (d->flags & 64) ? 0 : 1;
    world.facilities.emplace(id, facility);
    next.scene.world.facility_order.push_back(id);
    // 与真实新局接管同一类别投影；新建商店也必须进入有序物品消费者。
    if (d->category == 1) {
        next.shops.emplace(id, ref::ObjectShopRecord{d->detail, {}});
        next.shop_order.push_back(id);
    }
    next.facility_original_ids[id] = raw;
    next.facility_ordinals[id] = ordinal;
    next.facility_residents[id] = -1;
    next.facility_difficulties[id] = 0;
    next.facility_flags[id] = 0;
    next.facility_month_age[id] = 0;
    next.facility_item_confirmations[id] = 0;
    next.facility_monthly_cash[id] = {};
    next.facility_details[id] = {};
    next.facility_details[id].construction_limit =
        static_cast<int>(quote.values->construction_ticks);
    next.dungeon_facilities[id] = {};
    next.neighbourhood[id] = {};
    next.neighbourhood_details[id] = {};
    ref::DungeonFinishSite site;
    for (const auto &c : footprint.cells) {
        const auto n = static_cast<std::size_t>(c.position.y * world.map.width + c.position.x);
        auto &tile = world.map.cells[n];
        tile.facility = ref::FacilityTileBinding{{id}, d->id, c.fragment_index};
        tile.legacy_state = d->kind == 1 ? 8 : d->kind == 8 ? 9 : d->kind == 9 ? 10 : 1;
        tile.category = ref::RouteCategory::terminal;
        if (map_creation) {
            tile.legacy_state = d->kind == 4 ? 6 : 7;
            tile.category = ref::RouteCategory::access;
            if (d->kind == 5) {
                next.surface[n].instance = d->direction; // 原i.f(o.m)，不是实例ID。
                next.surface[n].fragment = -1;
            }
        }
        next.surface[n].definition = d->id;
        next.surface[n].updates = 0;
        next.surface[n].variant = c.fragment_index;
        site.occupied_cells.push_back(c.position);
    }
    next.sites.emplace(id, site);
    if ((!map_creation && !refresh_map(next)) ||
        (map_creation && surface_refresh && !refresh_startup_world_surface(next)))
        return {Error::missing_source};
    if (charge) {
        // 原g(price,0)只记全局建设支出，不把造价塞进设施月经营数组。
        auto &ai = world.ai;
        const int price = static_cast<int>(quote.values->construction_cost);
        auto &month = next.monthly_cash.at(next.scene.calendar.month).at(0).at(1);
        if (price > std::numeric_limits<int>::max() - month ||
            ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
            ai.accounting.post_cash({ai.next_cash_id++, next.simulation_steps + 1,
                                     ref::CashCategory::facilities, ref::CashDirection::expense,
                                     price}) != ref::AccountingError::none)
            return {Error::missing_source};
        month += price;
        next.sound_requests.push_back(11);
        next.build_feedback_message = "建设完毕";
        next.build_feedback_counter = 20;
    }
    if (!map_creation && !refresh_startup_world_connections(next))
        return {Error::missing_source};
    s = std::move(next);
    return {Error::none, StartupBuildDenial::none, id};
}
StartupBuildResult install_startup_world_map_facility(State &s, int definition,
                                                      ref::Position anchor, bool refresh) {
    if (definition < 0)
        return {Error::missing_source};
    return install_facility(s, anchor, ref::FacilityOrientation::first, false, definition, refresh);
}
bool retire_startup_world_facility(State &s, std::uint64_t old_id) {
    const auto found = s.scene.world.world.facilities.find(old_id);
    if (found == s.scene.world.world.facilities.end())
        return false;
    const auto anchor = found->second.placement.anchor;
    const auto footprint = ref::facility_footprint(
        found->second.placement.shape, found->second.placement.orientation, anchor,
        s.scene.world.world.map.width, s.scene.world.world.map.height);
    if (footprint.error != ref::GeometryError::none)
        return false;
    auto &world = s.scene.world.world;
    if (!ref::valid_legacy_map(world.map) || s.surface.size() != world.map.cells.size())
        return false;
    for (const auto &c : footprint.cells) {
        const auto n = static_cast<std::size_t>(c.position.y * world.map.width + c.position.x);
        const auto &binding = world.map.cells[n].facility;
        if (!binding || binding->instance_id.value != old_id ||
            binding->definition_id != found->second.placement.definition_id ||
            binding->fragment_index != c.fragment_index)
            return false;
    }
    for (const auto &c : footprint.cells) {
        const auto n = static_cast<std::size_t>(c.position.y * world.map.width + c.position.x);
        world.map.cells.at(n).facility.reset();
        world.map.cells.at(n).legacy_state = 4;
        world.map.cells.at(n).category = ref::RouteCategory::ground;
        s.surface.at(n).definition = s.ground_definition;
        s.surface.at(n).updates = 0;
        s.surface.at(n).instance = s.surface.at(n).fragment = -1;
    }
    world.facilities.erase(old_id);
    s.scene.world.facility_order.erase(std::remove(s.scene.world.facility_order.begin(),
                                                   s.scene.world.facility_order.end(), old_id),
                                       s.scene.world.facility_order.end());
    s.neighbourhood_details.erase(old_id);
    // 实例已从地图退休；逐实例缓存不属于新实例或合法经营历史。
    s.facility_original_ids.erase(old_id);
    s.facility_ordinals.erase(old_id);
    s.facility_residents.erase(old_id);
    s.facility_difficulties.erase(old_id);
    s.facility_flags.erase(old_id);
    s.facility_details.erase(old_id);
    s.facility_monthly_cash.erase(old_id);
    s.facility_month_age.erase(old_id);
    s.facility_item_confirmations.erase(old_id);
    s.neighbourhood.erase(old_id);
    s.dungeon_facilities.erase(old_id);
    s.sites.erase(old_id);
    s.shops.erase(old_id);
    s.shop_order.erase(std::remove(s.shop_order.begin(), s.shop_order.end(), old_id),
                       s.shop_order.end());
    return true;
}
StartupBuildResult confirm_startup_world_build(State &s, ref::Position anchor,
                                               ref::FacilityOrientation orientation) {
    if (!main(s) || s.scene.scene_state != 1 || !s.build_definition || s.build_mode != 0)
        return {Error::invalid_page};
    const auto *d = definition(s, *s.build_definition);
    if (!d)
        return {Error::missing_source};
    if (d->kind != 12)
        return install_facility(s, anchor, orientation, true);
    const auto remaining = s.facility_free_builds.find(d->id);
    if (remaining == s.facility_free_builds.end() || remaining->second <= 0)
        return {Error::none, StartupBuildDenial::unavailable};
    auto next = s;
    const auto installed = install_facility(next, anchor, orientation, true);
    if (!installed.created)
        return installed;
    const auto id = *installed.created;
    next.facility_details.at(id).residence_mode = 0; // 原复建不重复首次入住奖励。
    for (const auto &human : next.rules->humans) {
        const auto presence = next.human_presence.find(human.identity);
        const auto home = next.human_homes.find(human.identity);
        if (presence == next.human_presence.end() || home == next.human_homes.end())
            return {Error::missing_source};
        if (presence->second != 0 && home->second[2] == 2) {
            --next.facility_free_builds.at(d->id);
            next.facility_residents.at(id) = human.identity;
            next.facility_details.at(id).resident_definition = human.identity;
            home->second = {anchor.x, anchor.y, 1, 0};
            break;
        }
    }
    if (next.facility_free_builds.at(d->id) <= 0) {
        next.facility_free_builds.at(d->id) = 0;
        next.scene.scene_state = 0;
        next.scene.scene_counter = 0;
        next.build_definition.reset();
        next.scripts.selected_facility.reset();
        for (auto &flags : next.scene.world.map_flags)
            flags &= ~1U;
    }
    s = std::move(next);
    return installed;
}
StartupBuildResult install_startup_world_facility(State &s, int id, ref::Position anchor,
                                                  ref::FacilityOrientation orientation) {
    const auto saved = s.build_definition;
    s.build_definition = id;
    const auto result = install_facility(s, anchor, orientation, false);
    s.build_definition = saved;
    return result;
}
Error cancel_startup_world_build(State &s) {
    if (!main(s) || s.scene.scene_state != 1)
        return Error::invalid_page;
    s.scene.scene_state = 0;
    s.scene.scene_counter = 0;
    s.build_definition.reset();
    s.scripts.selected_facility.reset();
    return Error::none;
}
std::optional<ref::FacilityEconomyValues> startup_world_facility_values(const State &s,
                                                                        std::uint64_t id) {
    const auto f = s.scene.world.world.facilities.find(id);
    if (f == s.scene.world.world.facilities.end())
        return {};
    const auto *d = definition(s, f->second.placement.definition_id);
    const auto n = s.neighbourhood.find(id);
    if (!d || n == s.neighbourhood.end())
        return {};
    auto in = input(s, d->id);
    std::copy(n->second.begin(), n->second.end(), in.instance_modifiers.begin());
    return ref::derive_facility_economy(d->economy, in).values;
}
bool valid_startup_world_facility_page(const State &s, const ref::WorldScriptPage &p) {
    const auto counter = s.page_counters.find(p.id);
    const auto phase = s.page_phases.find(p.id);
    if (p.legacy_page != 74 || counter == s.page_counters.end() || counter->second < 0 ||
        phase == s.page_phases.end() || phase->second < 0 ||
        phase->second >= startup_world_facility_page_count(s, p))
        return false;
    const auto preview = s.facility_definition_page_bindings.find(p.id);
    if (preview != s.facility_definition_page_bindings.end()) {
        const auto *d = definition(s, preview->second);
        const auto status = s.facility_presence.find(preview->second);
        if (!d || preview->second != p.legacy_f || p.legacy_g != 1 ||
            s.facility_page_bindings.count(p.id) || s.facility_page_neighbours.count(p.id) ||
            status == s.facility_presence.end() || status->second == 2 || d->unlock_rank < 0 ||
            d->unlock_rank > s.rank || !s.scripts.facilities.count(d->id))
            return false;
        const ref::WorldScriptPage *parent = nullptr;
        for (const auto &page : s.scripts.pages) {
            if (page.id == p.id)
                break;
            if (page.lifecycle != 4)
                parent = &page;
        }
        if (!parent || parent->kind != ref::WorldScriptPageKind::raw_page ||
            parent->legacy_page != 85 || !s.commerce_pages_initialized.count(parent->id))
            return false;
        const auto list = s.commerce_page_lists.find(parent->id);
        const auto data = s.commerce_page_data.find(parent->id);
        return list != s.commerce_page_lists.end() && data != s.commerce_page_data.end() &&
               data->second[2] >= 0 &&
               static_cast<std::size_t>(data->second[2]) < list->second.size() &&
               list->second[data->second[2]] == d->id;
    }
    if (p.legacy_g == 1)
        return false;
    const auto binding = s.facility_page_bindings.find(p.id);
    if (p.legacy_page != 74 || binding == s.facility_page_bindings.end() ||
        !s.facility_page_neighbours.count(p.id))
        return false;
    const auto f = s.scene.world.world.facilities.find(binding->second);
    return f != s.scene.world.world.facilities.end() && f->second.status != 0 &&
           f->second.placement.definition_id == p.legacy_f &&
           startup_world_runtime_facility_target(s, binding->second).has_value();
}
std::optional<std::vector<StartupFacilityBonusRow>>
startup_world_facility_bonus_rows(const State &s, std::uint64_t page_id) {
    if (!s.rules)
        return {};
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
        [=](const auto &p) { return p.id == page_id; });
    const auto sources = s.facility_page_neighbours.find(page_id);
    if (page == s.scripts.pages.end() || page->kind != ref::WorldScriptPageKind::raw_page ||
        page->legacy_page != 74 || page->lifecycle == 4 ||
        sources == s.facility_page_neighbours.end() ||
        s.facility_definition_page_bindings.count(page_id) ||
        !valid_startup_world_facility_page(s, *page))
        return {};
    std::vector<StartupFacilityBonusRow> result;
    result.reserve(sources->second.size());
    for (const auto &source : sources->second) {
        const auto instance = s.scene.world.world.facilities.find(source.instance_id.value);
        const auto ordinal = s.facility_ordinals.find(source.instance_id.value);
        const auto *d = definition(s, source.definition_id);
        if (!d || instance == s.scene.world.world.facilities.end() ||
            instance->second.placement.definition_id != source.definition_id ||
            ordinal == s.facility_ordinals.end() || ordinal->second < 0 ||
            ordinal->second == std::numeric_limits<int>::max() ||
            d->legacy_icon < 0 || d->legacy_icon > 6)
            return {};
        const std::size_t count = d->kind == 2 ? 2 : 1;
        if (d->neighbour_effects.size() < count)
            return {}; // 原源y不足不能伪造0；空Y才是“没有奖励”。
        StartupFacilityBonusRow row;
        row.instance = source.instance_id.value;
        row.definition = source.definition_id;
        row.ordinal = ordinal->second;
        row.icon = d->legacy_icon;
        row.name = d->name + std::to_string(ordinal->second + 1);
        for (std::size_t n = 0; n < count; ++n) {
            // 固定85定义x/y长度相等，维护pairs保持原位置；这里明确不读x槽标签。
            const auto value = d->neighbour_effects[n].delta;
            const int attribute = d->kind == 2 ? static_cast<int>(n) : 2;
            const char *label = attribute == 0 ? "价格" : attribute == 1 ? "品质" : "魅力";
            row.values.push_back({attribute, label, value, "+" + std::to_string(value)});
        }
        result.push_back(std::move(row)); // 不按定义或实例去重，保留原Y逐行顺序。
    }
    return result;
}
std::optional<StartupFacilityBonusWindow>
startup_world_facility_bonus_window(const State &s, std::uint64_t page, std::size_t first) {
    auto rows = startup_world_facility_bonus_rows(s, page);
    if (!rows || first > (rows->size() > 5 ? rows->size() - 5 : 0))
        return {};
    StartupFacilityBonusWindow window{first, rows->size(), {}};
    const auto end = std::min(first + 5, rows->size());
    for (auto n = first; n < end; ++n)
        window.rows.push_back(std::move((*rows)[n]));
    return window;
}
Error open_startup_world_facility_definition(State &s, int d) {
    const auto *parent = top(s);
    if (!s.rules || s.scene.framework_paused || !parent ||
        parent->kind != ref::WorldScriptPageKind::raw_page || parent->legacy_page != 85 ||
        !s.commerce_pages_initialized.count(parent->id))
        return Error::invalid_page;
    const auto *source = definition(s, d);
    const auto presence = s.facility_presence.find(d);
    const auto list = s.commerce_page_lists.find(parent->id);
    const auto data = s.commerce_page_data.find(parent->id);
    if (!source || presence == s.facility_presence.end() || presence->second == 2 ||
        source->unlock_rank < 0 || source->unlock_rank > s.rank || !s.scripts.facilities.count(d) ||
        list == s.commerce_page_lists.end() || data == s.commerce_page_data.end() ||
        data->second[2] < 0 || static_cast<std::size_t>(data->second[2]) >= list->second.size() ||
        list->second[data->second[2]] != d)
        return Error::missing_source;
    auto next = s;
    next.scripts.executing_page = parent->id;
    ref::WorldScriptPage page;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 74;
    page.legacy_f = d; // 维护绑定的定义索引；原f=1另记g，不能冒充地图实例。
    page.legacy_g = 1;
    page.title = "设施情报";
    const auto result = ref::prepare_world_script_page(startup_world_runtime_scripts(next), page);
    if (!result.candidate || result.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(next, result.candidate->state))
        return Error::script_failed;
    const auto id = result.candidate->inserted_pages.front().id;
    next.facility_definition_page_bindings[id] = d;
    next.page_counters[id] = next.page_phases[id] = 0;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
Error open_startup_world_facility_page(State &s, std::uint64_t id) {
    if (!main(s) || s.scene.scene_state != 0)
        return Error::invalid_page;
    const auto f = s.scene.world.world.facilities.find(id);
    const auto neighbours = s.neighbourhood_details.find(id);
    if (f == s.scene.world.world.facilities.end() ||
        (f->second.status == 0 &&
         !s.scene.world.world.facility_uses.at(f->second.placement.definition_id)
              .upgrade_pending) ||
        neighbours == s.neighbourhood_details.end() ||
        !startup_world_runtime_facility_target(s, id))
        return Error::missing_source;
    auto next = s;
    const auto *d = definition(next, f->second.placement.definition_id);
    if (!d)
        return Error::missing_source;
    if (s.scene.world.world.facility_uses.at(d->id).upgrade_pending) {
        ref::WorldScriptPage upgrade;
        upgrade.kind = ref::WorldScriptPageKind::raw_page;
        upgrade.legacy_page = 81;
        upgrade.legacy_f = d->id;
        const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(next), upgrade);
        if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
            !write_startup_world_runtime_scripts(next, r.candidate->state))
            return Error::script_failed;
        next.facility_page_bindings[r.candidate->inserted_pages.front().id] = id;
        s = std::move(next);
        return Error::none;
    }
    // b/c:1638：已绑定住宅走居民60，不进入普通设施74；这里没有创建人物实例。
    if (d->kind == 12 && s.facility_residents.at(id) != -1) {
        const auto human = s.facility_residents.at(id);
        if (!next.human_calendar.count(human))
            return Error::missing_source;
        ref::WorldScriptPage resident;
        resident.kind = ref::WorldScriptPageKind::raw_page;
        resident.legacy_page = 60;
        const auto page =
            ref::prepare_world_script_page(startup_world_runtime_scripts(next), resident);
        if (!page.candidate || page.candidate->inserted_pages.size() != 1 ||
            !write_startup_world_runtime_scripts(next, page.candidate->state))
            return Error::script_failed;
        next.page_human_bindings[page.candidate->inserted_pages.front().id] = human;
        s = std::move(next);
        return Error::none;
    }
    // b/c:1616：初次说明脚本先推入栈，随后raw74在它上方；返回才读下方说明。
    const int event = d->kind == 13      ? 81
                      : d->category == 2 ? 87
                      : d->detail == 1   ? 84
                      : d->detail == 4   ? 85
                      : d->detail == 5   ? 86
                                         : 0;
    if (event && !ref::world_script_seen(next.scripts, event)) {
        const auto tutorial = ref::prepare_world_script(
            startup_world_runtime_catalog(), startup_world_runtime_scripts(next), {event, {}, {}});
        if (!tutorial.candidate ||
            !write_startup_world_runtime_scripts(next, tutorial.candidate->state))
            return Error::script_failed;
    }
    ref::WorldScriptPage page;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = 74;
    page.legacy_f = f->second.placement.definition_id;
    page.title = "设施情报";
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(next), page);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(next, r.candidate->state))
        return Error::script_failed;
    const auto pid = r.candidate->inserted_pages.front().id;
    next.facility_page_bindings[pid] = id;
    next.facility_page_neighbours[pid] = neighbours->second.sources;
    next.page_counters[pid] = next.page_phases[pid] = 0;
    next.scripts.selected_facility = id;
    s = std::move(next);
    return Error::none;
}
Error act_startup_world_facility_page(State &s, std::uint64_t id,
                                      StartupFacilityPageAction action) {
    if (action != StartupFacilityPageAction::previous &&
        action != StartupFacilityPageAction::next && action != StartupFacilityPageAction::confirm &&
        action != StartupFacilityPageAction::cancel)
        return Error::invalid_page;
    const auto *page = top(s);
    if (!s.rules || s.scene.framework_paused || !page || page->id != id ||
        !valid_startup_world_facility_page(s, *page))
        return Error::invalid_page;
    const auto *d = definition(s, page->legacy_f);
    if (!d)
        return Error::missing_source;
    auto next = s;
    if (action == StartupFacilityPageAction::previous ||
        action == StartupFacilityPageAction::next) {
        // 原普通类别3实例两页，商品/道具/植物/住宅模板不伪造第二页。
        if (startup_world_facility_page_count(s, *page) == 2)
            next.page_phases[id] = 1 - next.page_phases.at(id);
    } else if (action == StartupFacilityPageAction::cancel ||
               next.facility_definition_page_bindings.count(id)) {
        const auto r =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
        if (!r.candidate || !write_startup_world_runtime_scripts(next, r.candidate->state))
            return Error::script_failed;
    } else if (next.page_phases.at(id) == 0) {
        if (d->detail == 1 || d->detail == 4 || d->detail == 5) {
            next.scripts.executing_page = id;
            ref::WorldScriptPage catalogue;
            catalogue.kind = ref::WorldScriptPageKind::raw_page;
            catalogue.legacy_page = 79;
            catalogue.legacy_f = d->detail;
            catalogue.title = "商品";
            const auto opened = ref::prepare_world_script_page(startup_world_runtime_scripts(next), catalogue);
            if (!opened.candidate || opened.candidate->inserted_pages.size() != 1 ||
                !write_startup_world_runtime_scripts(next, opened.candidate->state) ||
                !initialize_startup_world_facility_catalog_pages(next))
                return Error::script_failed;
            next.scripts.executing_page.reset();
            s = std::move(next);
            return Error::none;
        }
        if (d->kind != 2 && d->kind != 12 && d->detail != 1 && d->detail != 4 && d->detail != 5 &&
            d->detail != 6)
            return open_startup_world_facility_items(s, id);
        if (d->detail != 6)
            return Error::missing_source; // 75/79/人物60尚有各自操作，不能通用关页。
        ref::WorldScriptPage p;
        p.kind = ref::WorldScriptPageKind::raw_page;
        p.legacy_page = 80;
        const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(next), p);
        if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
            !write_startup_world_runtime_scripts(next, r.candidate->state))
            return Error::script_failed;
        const auto page = r.candidate->inserted_pages.front().id;
        next.facility_page_bindings[page] = next.facility_page_bindings.at(id);
        auto &list = next.residence_page_candidates[page];
        for (const auto &h : next.rules->humans)
            if (next.human_presence.at(h.identity) != 0 &&
                next.shop_humans.at(h.identity).satisfaction >= h.residence_threshold &&
                next.human_homes.at(h.identity)[2] == 0)
                list.push_back(h.identity);
        if (list.empty()) {
            const auto event = ref::prepare_world_script(
                startup_world_runtime_catalog(), startup_world_runtime_scripts(next), {16, {}, {}});
            const auto closed =
                event.candidate ? ref::prepare_world_script_close_page(event.candidate->state, page)
                                : ref::WorldScriptResult{};
            if (!closed.candidate ||
                !write_startup_world_runtime_scripts(next, closed.candidate->state))
                return Error::script_failed;
        }
    }
    s = std::move(next);
    return Error::none;
}

StartupBuildResult act_startup_world_residence_page(State &s, std::uint64_t id, int human,
                                                    bool cancel) {
    const auto *p = top(s);
    if (s.scene.framework_paused || !p || p->id != id || p->legacy_page != 80 ||
        !s.residence_page_candidates.count(id) || !s.facility_page_bindings.count(id))
        return {Error::invalid_page};
    auto next = s;
    if (cancel) {
        const auto close =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
        if (!close.candidate || !write_startup_world_runtime_scripts(next, close.candidate->state))
            return {Error::script_failed};
        s = std::move(next);
        return {};
    }
    const auto &list = s.residence_page_candidates.at(id);
    const auto h = std::find_if(s.rules->humans.begin(), s.rules->humans.end(),
                                [&](const auto &v) { return v.identity == human; });
    const auto old_id = s.facility_page_bindings.at(id);
    const auto old = s.scene.world.world.facilities.find(old_id);
    if (std::find(list.begin(), list.end(), human) == list.end() || h == s.rules->humans.end() ||
        old == s.scene.world.world.facilities.end() || old->second.kind != 13 ||
        old->second.status != 1 || !old->second.occupants.empty() ||
        s.human_homes.at(human)[2] != 0)
        return {Error::invalid_page};
    if (s.scene.world.world.ai.accounting.funds() < h->residence_fee) {
        const auto event = ref::prepare_world_script(
            startup_world_runtime_catalog(), startup_world_runtime_scripts(next), {11, {}, {}});
        if (!event.candidate || !write_startup_world_runtime_scripts(next, event.candidate->state))
            return {Error::script_failed};
        s = std::move(next);
        return {Error::none, StartupBuildDenial::insufficient_funds};
    }
    auto &ai = next.scene.world.world.ai;
    auto &monthly = next.monthly_cash.at(next.scene.calendar.month)[2][1];
    if (h->residence_fee < 0 || monthly > std::numeric_limits<int>::max() - h->residence_fee ||
        ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
        ai.accounting.post_cash({ai.next_cash_id++, next.simulation_steps + 1,
                                 ref::CashCategory::adventurers, ref::CashDirection::expense,
                                 h->residence_fee}) != ref::AccountingError::none)
        return {Error::missing_source};
    monthly += h->residence_fee;
    const auto anchor = old->second.placement.anchor;
    if (!retire_startup_world_facility(next, old_id))
        return {Error::missing_source};
    if (!refresh_map(next, false, false))
        return {Error::missing_source}; // 原撤除false先刷新，再创建新住宅true刷新。
    const auto home = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                   [](const auto &d) { return d.kind == 12; });
    if (home == s.rules->facilities.end())
        return {Error::missing_source};
    const auto old_selection = next.build_definition;
    next.build_definition = home->id;
    const auto installed = install_facility(next, anchor, ref::FacilityOrientation::first, false);
    if (!installed.created)
        return installed;
    next.build_definition = old_selection;
    next.facility_residents.at(*installed.created) = human;
    auto &details = next.facility_details.at(*installed.created);
    details.residence_mode = 1;
    details.resident_definition = human;
    next.human_homes.at(human) = {anchor.x, anchor.y, 1, 0};
    const auto requests = refresh_startup_world_residence_requests(next, false);
    if (!requests)
        return {Error::missing_source};
    next = *requests;
    for (auto &page : next.scripts.pages)
        page.lifecycle = page.kind == ref::WorldScriptPageKind::scene ? 2 : 4;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return installed;
}
std::optional<State> prepare_startup_world_residence_completion(const State &s, std::uint64_t id) {
    if (!s.rules)
        return {};
    const auto adapter = startup_world_runtime_adapter();
    ref::WorldResidenceState r;
    r.facility = adapter.facilities.read(s);
    r.professions = s.scene.world.world.ai.professions;
    r.reward_display = s.reward_display;
    r.effort_display = s.effort_display;
    for (const auto &h : s.rules->humans) {
        const auto &g = s.scene.world.world.ai.growth.at(h.identity);
        r.humans.emplace(h.identity, ref::WorldResidenceHuman{
                                         g.definition, g.derived, h.residence_completion_program,
                                         h.name, s.human_calendar.at(h.identity).celebrations});
    }
    const auto completed = ref::prepare_world_residence(r, id, adapter.catalog);
    if (!completed.candidate)
        return {};
    auto next = s;
    const auto &c = completed.candidate->state;
    if (!adapter.facilities.write(next, c.facility))
        return {};
    for (const auto &h : c.humans) {
        auto &g = next.scene.world.world.ai.growth.at(h.first);
        g.definition = h.second.definition;
        g.derived = h.second.derived;
        if (!synchronize_startup_world_human_capacity(next, h.first))
            return {};
    }
    next.reward_display = c.reward_display;
    next.effort_display = c.effort_display;
    for (const auto &p : c.page_bindings) {
        next.page_human_bindings[p.first] = p.second.human;
        if (p.second.facility_definition) {
            const auto page = std::find_if(next.scripts.pages.begin(), next.scripts.pages.end(),
                                           [&](const auto &v) { return v.id == p.first; });
            if (page == next.scripts.pages.end())
                return {};
            page->legacy_f = *p.second.facility_definition;
        }
    }
    return next;
}
bool consume_startup_world_facility_upgrade(State &s, std::uint64_t page, bool confirm) {
    const auto binding = s.facility_page_bindings.find(page);
    if (binding == s.facility_page_bindings.end())
        return false;
    const auto f = s.scene.world.world.facilities.find(binding->second);
    if (f == s.scene.world.world.facilities.end())
        return false;
    const int definition_id = f->second.placement.definition_id;
    const auto *d = definition(s, definition_id);
    if (!d)
        return false;
    auto &progress = s.scene.world.world.facility_uses.at(definition_id);
    if (!s.facility_upgrade_initialized.count(page)) {
        if (!progress.upgrade_pending)
            return false;
        auto in = input(s, definition_id);
        std::copy(s.neighbourhood.at(binding->second).begin(),
                  s.neighbourhood.at(binding->second).end(), in.instance_modifiers.begin());
        const auto r = ref::prepare_facility_upgrade(d->economy, in);
        if (!r || r->remaining_uses > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
            return false;
        progress.level = r->level;
        progress.completed_uses = static_cast<int>(r->remaining_uses);
        s.facility_upgrade_display = r->display;
        // 原o.k()更新共享经营值；既有到达消费者读price投影，所有同定义实例同步派生，
        // 各实例邻接仍分别保留，不能只更新被点击建筑或复制它的价格到其他建筑。
        for (auto &instance : s.scene.world.world.facilities) {
            if (instance.second.placement.definition_id != definition_id)
                continue;
            auto current = input(s, definition_id);
            const auto &neighbours = s.neighbourhood.at(instance.first);
            std::copy(neighbours.begin(), neighbours.end(), current.instance_modifiers.begin());
            const auto economy = ref::derive_facility_economy(d->economy, current);
            if (!economy.values ||
                economy.values->instance_attributes[0] < std::numeric_limits<int>::min() ||
                economy.values->instance_attributes[0] > std::numeric_limits<int>::max())
                return false;
            instance.second.price = static_cast<int>(economy.values->instance_attributes[0]);
        }
        for (std::size_t n = 0; n < 4; ++n) {
            const auto value = r->values.definition_attributes[n];
            if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
                return false;
            s.scripts.facilities.at(definition_id).attributes[n] = static_cast<int>(value);
        }
        s.facility_upgrade_initialized.insert(page);
        s.page_phases[page] = 0;
    }
    auto &phase = s.page_phases[page];
    auto &counter = s.page_counters[page];
    if (phase < 0 || phase > 1)
        return false;
    if (!confirm) {
        if (phase == 0 && counter == 1)
            s.sound_requests.push_back(20);
        return true;
    }
    if (phase == 0) {
        if (counter < 40)
            counter = 40;
        else {
            phase = 1;
            counter = 0;
        }
    } else if (counter < 55)
        counter = 55;
    else {
        progress.upgrade_pending = false;
        const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), page);
        if (!r.candidate || !write_startup_world_runtime_scripts(s, r.candidate->state))
            return false;
    }
    return true;
}
} // namespace dungeon_village_prototype
