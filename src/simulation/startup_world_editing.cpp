#include "ark/simulation/startup_world_editing.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Denial = StartupBuildDenial;
const StartupDefinition *definition(const State &s, int id) {
    if (!s.rules)
        return nullptr;
    const auto d = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                [id](const auto &value) { return value.id == id; });
    return d == s.rules->facilities.end() ? nullptr : &*d;
}
bool main(const State &s, int mode) {
    const auto page = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                   [](const auto &p) { return p.lifecycle != 4; });
    return s.rules && !s.scene.framework_paused && s.scene.scene_state == mode &&
           page != s.scripts.pages.rend() && page->kind == ref::WorldScriptPageKind::scene &&
           ref::valid_legacy_map(s.scene.world.world.map);
}
bool within(const State &s, ref::Position p) {
    const auto &m = s.scene.world.world.map;
    return p.x >= 0 && p.y >= 0 && p.x < m.width && p.y < m.height;
}
bool town(const State &s, ref::Position p) {
    if (s.fence_level < 0 || static_cast<std::size_t>(s.fence_level) >= s.rules->fences.size())
        return false;
    const auto &f = s.rules->fences[s.fence_level];
    return p.x > f[0].x && p.x < f[1].x && p.y < f[0].y && p.y > f[1].y;
}
std::size_t index(const State &s, ref::Position p) {
    return static_cast<std::size_t>(p.y * s.scene.world.world.map.width + p.x);
}
void mode(State &s, int next) {
    constexpr const char *prompts[]{"要建在哪里呢",   "从哪里开始铺呢", "铺到哪里呢", "撤除哪里呢",
                                    "从哪里开始撤除", "撤到哪里",       "移动哪个",   "移动去哪里"};
    s.build_mode = next;
    s.build_feedback_counter = 0;
    s.build_feedback_message = prompts[next];
}
void enter(State &s, int next) {
    mode(s, next);
    s.build_anchor.reset();
    s.build_moving_facility.reset();
    s.scene.scene_state = 1;
    s.scene.scene_counter = 0;
    s.scene.first_normal_refresh = true;
    s.scripts.selected_facility.reset();
}
bool event(State &s, int record, int speaker = -1) {
    const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                                             startup_world_runtime_scripts(s), {record, {}, {}});
    if (!r.candidate || !write_startup_world_runtime_scripts(s, r.candidate->state))
        return false;
    if (speaker >= 0) {
        if (r.candidate->inserted_pages.empty())
            return false;
        const auto id = r.candidate->inserted_pages.front().id;
        const auto p = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                    [id](const auto &page) { return page.id == id; });
        if (p == s.scripts.pages.end())
            return false;
        p->speaker_kind = 1;
        p->speaker_definition = speaker;
    }
    return true;
}
bool charge(State &s, std::int64_t amount) {
    if (amount < 0 || amount > std::numeric_limits<int>::max())
        return false;
    auto &ai = s.scene.world.world.ai;
    const auto cash = ai.accounting.funds();
    auto &spent = s.monthly_cash.at(s.scene.calendar.month)[0][1];
    if (cash < std::numeric_limits<std::int64_t>::min() + amount ||
        amount > std::numeric_limits<int>::max() - spent)
        return false;
    if (amount && (ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
                   ai.accounting.post_cash(
                       {ai.next_cash_id, s.simulation_steps + 1, ref::CashCategory::facilities,
                        ref::CashDirection::expense, amount}) != ref::AccountingError::none))
        return false;
    if (amount)
        ++ai.next_cash_id;
    spent += static_cast<int>(amount);
    return true;
}
bool references_allow_removal(const State &s, std::uint64_t id) {
    // 正常编辑只在主场景操作；不以擦掉仍存活的详情页载荷来满足资源计数。
    for (const auto &p : s.scripts.pages) {
        if (p.lifecycle == 4)
            continue;
        const auto binding = s.facility_page_bindings.find(p.id);
        if (binding != s.facility_page_bindings.end() && binding->second == id)
            return false;
    }
    return true;
}
bool valid_instance(const State &s, std::uint64_t id) {
    return s.scene.world.world.facilities.count(id) && s.facility_original_ids.count(id) &&
           s.facility_ordinals.count(id) && s.facility_residents.count(id) &&
           s.facility_details.count(id) && s.facility_monthly_cash.count(id) &&
           s.facility_month_age.count(id) && s.facility_item_confirmations.count(id) &&
           s.dungeon_facilities.count(id) && references_allow_removal(s, id);
}
bool release_residence(State &s, std::uint64_t id) {
    const int resident = s.facility_residents.at(id);
    if (resident < 0)
        return true;
    const auto home = s.human_homes.find(resident);
    const auto definition = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                         [](const auto &d) { return d.kind == 12; });
    if (home == s.human_homes.end() || definition == s.rules->facilities.end())
        return false;
    if (!event(s, 135, resident) || !event(s, 136))
        return false;
    // 原D3不清零；解除居住并返还首个kind12的一次重新建设资格。
    auto &current = s.human_homes.at(resident);
    current[0] = current[1] = 0;
    current[2] = 2;
    const int d = definition->id;
    const auto free = s.facility_free_builds.find(d);
    const auto presence = s.facility_presence.find(d);
    if (free == s.facility_free_builds.end() || presence == s.facility_presence.end() ||
        free->second < 0 || free->second > 99)
        return false;
    free->second = std::min(free->second + 1, 99);
    if (presence->second == 0) {
        presence->second = 2;
        s.facility_unlock_notices.at(d) = true;
    }
    return true;
}
bool road(State &s, const std::vector<ref::Position> &cells, bool remove, int d, int price) {
    const auto primary = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                      [](const auto &value) { return value.kind == 6; });
    const auto *ground = definition(s, s.ground_definition);
    if (primary == s.rules->facilities.end() || !s.build_anchor || !ground || ground->kind != 7 ||
        s.surface.size() != s.scene.world.world.map.cells.size())
        return false;
    const bool vertical = cells.size() < 2 || cells.front().x == cells.back().x;
    int changed{};
    for (const auto p : cells) {
        if (!within(s, p))
            return false;
        if (!town(s, p) && (remove || !vertical || d == primary->id))
            continue;
        const auto n = index(s, p);
        auto &tile = s.scene.world.world.map.cells[n];
        if (remove ? tile.legacy_state != 3
                   : (tile.legacy_state == 1 || tile.legacy_state == 2 || tile.legacy_state == 3))
            continue;
        const auto *surface = definition(s, s.surface[n].definition);
        if (!surface || !ref::legacy_surface_binding_matches(tile, surface->id, surface->kind,
                                                             s.ground_definition))
            return false;
        // 原i.b()/i.a()保留x及m：实例/占地和入口方向不退休，只有地表与路径字段改变。
        // 随后的完整地图刷新继续验证实例定义、全部占地和分片，不放过悬空引用。
        tile.legacy_state = remove ? 4 : 3;
        tile.category = remove ? ref::RouteCategory::ground : ref::RouteCategory::road;
        s.surface.at(n).definition = remove ? s.ground_definition : d;
        s.surface.at(n).updates = 0;
        ++changed;
    }
    if (!refresh_startup_world_map(s, true) ||
        !charge(s, static_cast<std::int64_t>(changed) * price))
        return false;
    mode(s, remove ? 3 : 1);
    s.build_anchor.reset();
    s.build_feedback_message = remove ? "撤除完毕" : "路铺好了";
    s.build_feedback_counter = 20;
    s.sound_requests.push_back(remove ? 21 : 11);
    return true;
}
StartupBuildResult move_facility(State &s, ref::Position target,
                                 ref::FacilityOrientation orientation) {
    if (!s.build_anchor || !s.build_moving_facility || !s.build_definition ||
        !valid_instance(s, *s.build_moving_facility))
        return {Error::missing_source};
    const auto old_id = *s.build_moving_facility;
    const auto &old = s.scene.world.world.facilities.at(old_id);
    const auto *d = definition(s, *s.build_definition);
    if (!d || old.placement.definition_id != d->id || !(old.placement.anchor == *s.build_anchor))
        return {Error::missing_source};
    const auto &map = s.scene.world.world.map;
    const auto before = ref::facility_footprint(old.placement.shape, old.placement.orientation,
                                                old.placement.anchor, map.width, map.height);
    const auto after =
        ref::facility_footprint(old.placement.shape, orientation, target, map.width, map.height);
    if (after.error == ref::GeometryError::outside_map)
        return {Error::none, Denial::outside_map};
    if (before.error != ref::GeometryError::none || after.error != ref::GeometryError::none)
        return {Error::missing_source};
    for (const auto &cell : after.cells) {
        if (std::any_of(before.cells.begin(), before.cells.end(),
                        [&](const auto &c) { return c.position == cell.position; }))
            return {Error::none, Denial::occupied};
        const auto &tile = map.cells[index(s, cell.position)];
        if (tile.facility || tile.legacy_state == 1 || tile.legacy_state == 2 ||
            tile.legacy_state == 10)
            return {Error::none, Denial::occupied};
        if (!town(s, cell.position))
            return {Error::none, Denial::outside_town};
    }
    const int raw = s.facility_original_ids.at(old_id), ordinal = s.facility_ordinals.at(old_id);
    const int status = old.status, sales = old.sales;
    const int updates = s.dungeon_facilities.at(old_id).updates;
    const auto details = s.facility_details.at(old_id);
    const auto monthly = s.facility_monthly_cash.at(old_id);
    const int month_age = s.facility_month_age.at(old_id);
    const int confirmations = s.facility_item_confirmations.at(old_id);
    const int resident = s.facility_residents.at(old_id);
    if (!retire_startup_world_facility(s, old_id) || !refresh_startup_world_map(s, false))
        return {Error::missing_source};
    const auto installed = install_startup_world_facility(s, d->id, target, orientation);
    if (!installed.created)
        return installed;
    const auto id = *installed.created;
    s.facility_original_ids.at(id) = raw;
    s.facility_ordinals.at(id) = ordinal;
    s.scene.world.world.facilities.at(id).status = status;
    s.scene.world.world.facilities.at(id).sales = sales;
    s.dungeon_facilities.at(id).updates = updates;
    s.facility_details.at(id).completion_popularity = details.completion_popularity;
    s.facility_details.at(id).residence_mode = details.residence_mode;
    s.facility_details.at(id).resident_definition = resident;
    s.facility_residents.at(id) = resident;
    s.facility_monthly_cash.at(id) = monthly;
    s.facility_month_age.at(id) = month_age;
    s.facility_item_confirmations.at(id) = confirmations;
    if (resident >= 0) {
        const auto home = s.human_homes.find(resident);
        if (home == s.human_homes.end())
            return {Error::missing_source};
        home->second[0] = target.x;
        home->second[1] = target.y;
        home->second[2] = 1;
    }
    if (!refresh_startup_world_connections(s) || !charge(s, 300))
        return {Error::missing_source};
    mode(s, 6);
    s.build_anchor.reset();
    s.build_moving_facility.reset();
    s.sound_requests.push_back(11);
    return {Error::none, Denial::none, id};
}
} // namespace

StartupBuildResult begin_startup_world_road(State &s, int d) {
    if (!main(s, 0))
        return {Error::invalid_page};
    const auto *source = definition(s, d);
    const auto availability = s.facility_presence.find(d);
    if (!source || availability == s.facility_presence.end())
        return {Error::missing_source};
    if (source->kind != 6 || !(source->flags & 4) || availability->second == 0)
        return {Error::none, Denial::unavailable};
    const auto quote = startup_world_build_quote(s, d);
    if (!quote)
        return {Error::missing_source};
    if (quote->construction_cost > s.scene.world.world.ai.accounting.funds())
        return {Error::none, Denial::insufficient_funds};
    auto next = s;
    next.build_definition = d;
    next.facility_unlock_notices.at(d) = false;
    enter(next, 1);
    s = std::move(next);
    return {};
}
StartupBuildResult begin_startup_world_edit(State &s, bool move) {
    if (!main(s, 0))
        return {Error::invalid_page};
    if (move && !(s.scripts.user_flags & 32U))
        return {Error::none, Denial::unavailable};
    if (move && s.scene.world.world.ai.accounting.funds() < 300)
        return {Error::none, Denial::insufficient_funds};
    enter(s, move ? 6 : 3);
    return {};
}
std::optional<std::vector<ref::Position>> startup_world_edit_segment(const State &s,
                                                                     ref::Position end) {
    if (!s.build_anchor || !within(s, *s.build_anchor) || !within(s, end))
        return {};
    const auto start = *s.build_anchor;
    std::vector<ref::Position> cells;
    if (std::abs(end.y - start.y) >= std::abs(end.x - start.x)) {
        for (int y = std::min(start.y, end.y); y <= std::max(start.y, end.y); ++y)
            cells.push_back({start.x, y});
    } else {
        for (int x = std::min(start.x, end.x); x <= std::max(start.x, end.x); ++x)
            cells.push_back({x, start.y});
    }
    return cells;
}
StartupBuildResult confirm_startup_world_edit(State &s, ref::Position p,
                                              ref::FacilityOrientation orientation) {
    if (!main(s, 1) || (s.build_mode != 1 && s.build_mode != 2 && s.build_mode != 3 &&
                        s.build_mode != 5 && s.build_mode != 6 && s.build_mode != 7))
        return {Error::invalid_page};
    if (!within(s, p))
        return {Error::none, Denial::outside_map};
    if ((s.build_mode == 1 || s.build_mode == 3) && !town(s, p))
        return {Error::none, Denial::outside_town};
    int price{};
    if (s.build_mode == 1 || s.build_mode == 2) {
        if (!s.build_definition)
            return {Error::missing_source};
        const auto *d = definition(s, *s.build_definition);
        const auto quote = startup_world_build_quote(s, *s.build_definition);
        if (!d || d->kind != 6 || !quote || quote->construction_cost < 0 ||
            quote->construction_cost > std::numeric_limits<int>::max())
            return {Error::missing_source};
        price = static_cast<int>(quote->construction_cost);
    } else if (s.build_mode == 6 || s.build_mode == 7)
        price = 300;
    if (price > 0 && s.scene.world.world.ai.accounting.funds() < price)
        return {Error::none, Denial::insufficient_funds};
    auto next = s;
    const auto tile = next.scene.world.world.map.cells[index(next, p)];
    if (s.build_mode == 1) {
        next.build_anchor = p;
        mode(next, 2);
    } else if (s.build_mode == 2 || s.build_mode == 5) {
        const auto cells = startup_world_edit_segment(next, p);
        if (!cells ||
            !road(next, *cells, s.build_mode == 5, next.build_definition.value_or(-1), price))
            return {Error::missing_source};
    } else if (s.build_mode == 6) {
        if (!tile.facility)
            return {};
        const auto id = tile.facility->instance_id.value;
        if (!valid_instance(next, id))
            return {Error::missing_source};
        const auto &facility = next.scene.world.world.facilities.at(id);
        if (facility.kind != 2 && facility.kind != 3 && facility.kind != 12 && facility.kind != 13)
            return {Error::none, Denial::unavailable};
        next.build_definition = facility.placement.definition_id;
        next.build_anchor = facility.placement.anchor;
        next.build_moving_facility = id;
        mode(next, 7);
    } else if (s.build_mode == 7) {
        const auto r = move_facility(next, p, orientation);
        if (r.error != Error::none || r.denial != Denial::none || !r.created)
            return r;
        s = std::move(next);
        return r;
    } else if (tile.legacy_state == 3) {
        next.build_anchor = p;
        mode(next, 5);
    } else if (tile.legacy_state == 1 || tile.legacy_state == 2) {
        if (!tile.facility || !valid_instance(next, tile.facility->instance_id.value))
            return {Error::missing_source};
        const auto id = tile.facility->instance_id.value;
        const auto kind = next.scene.world.world.facilities.at(id).kind;
        const int popularity = next.facility_details.at(id).completion_popularity;
        if (kind == 12 && !release_residence(next, id))
            return {Error::script_failed};
        if (kind != 13) {
            if (popularity == std::numeric_limits<int>::min())
                return {Error::missing_source};
            next.scene.world.popularity_queue.insert(next.scene.world.popularity_queue.begin(),
                                                     {10, -popularity, 1});
        }
        if (!retire_startup_world_facility(next, id) || !refresh_startup_world_map(next, true) ||
            !refresh_startup_world_connections(next) || !charge(next, 0))
            return {Error::missing_source};
        next.scripts.selected_facility.reset();
        next.build_feedback_message = "撤除完毕";
        next.build_feedback_counter = 20;
        next.sound_requests.push_back(21);
    }
    s = std::move(next);
    return {};
}
Error cancel_startup_world_edit(State &s) {
    if (!main(s, 1))
        return Error::invalid_page;
    if (s.build_mode == 2 || s.build_mode == 5 || s.build_mode == 7) {
        mode(s, s.build_mode == 2 ? 1 : s.build_mode == 5 ? 3 : 6);
        s.build_anchor.reset();
        s.build_moving_facility.reset();
        return Error::none;
    }
    const auto result = cancel_startup_world_build(s);
    if (result != Error::none)
        return result;
    s.build_anchor.reset();
    s.build_moving_facility.reset();
    for (auto &flags : s.scene.world.map_flags)
        flags &= ~1U;
    return Error::none;
}
} // namespace ark::simulation
