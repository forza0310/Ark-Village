#include "ark/simulation/startup_world_runtime_tasks.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
const ref::WorldScriptPage *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
std::int64_t source_percent(std::int64_t value, std::int64_t low, std::int64_t high, int start,
                            int end) {
    if ((value < low && low < high) || (value > low && low > high))
        return start;
    if ((value > high && low < high) || (value < high && low > high))
        return end;
    return low == high ? start : start + (value - low) * (end - start) / (high - low);
}
std::optional<int> grade(const State &s, const ref::WorldScriptPage &p) {
    if (!p.task_identity)
        return 3; // n.a(null,q)。
    const auto task = s.tasks.find(*p.task_identity);
    if (task == s.tasks.end())
        return {};
    const auto progress = s.task_progress.definitions.find(task->second.definition);
    if (progress == s.task_progress.definitions.end())
        return {};
    std::int64_t percent{};
    if (progress->second.kind == 0 && task->second.facility) {
        const auto f = s.dungeon_facilities.find(*task->second.facility);
        if (f != s.dungeon_facilities.end()) {
            if (f->second.extent < 0)
                return {};
            percent = source_percent(f->second.progress, 0, f->second.extent, 0, 100);
        }
    } else if (progress->second.kind == 1) {
        if (progress->second.flags & 2U) {
            for (const auto id : s.scene.world.world.ai.monster_order) {
                const auto &a = s.scene.world.world.ai.battle.actors.at(id);
                if (!(a.control.flags & 16384U))
                    continue;
                if (a.capacity < 0)
                    return {};
                const int hp = a.control.action == 7 ? 0 : a.hp.target;
                percent = source_percent(hp, 0, a.capacity, 100, 0);
                break;
            }
        } else {
            const auto d =
                std::find_if(s.rules->tasks.begin(), s.rules->tasks.end(), [&](const auto &v) {
                    return v.factory.identity == task->second.definition;
                });
            if (d == s.rules->tasks.end())
                return {};
            const auto quota = static_cast<std::int64_t>(d->encounter_quota) +
                               ((progress->second.flags & 4U)
                                    ? std::min<std::int64_t>(progress->second.completed * 2LL, 10)
                                    : 0);
            if (quota < 0 || quota > std::numeric_limits<int>::max())
                return {};
            std::int64_t remaining = quota;
            if (s.task.encounter) {
                const auto event = s.scene.world.world.ai.encounters.find(*s.task.encounter);
                if (event == s.scene.world.world.ai.encounters.end())
                    return {};
                remaining -= event->second.runtime.spawned - event->second.linked_monsters;
            }
            percent = source_percent(remaining, 1, quota, 100, 0);
        }
    }
    return percent > 80 ? 0 : percent > 60 ? 1 : percent > 40 ? 2 : percent > 20 ? 3 : 4;
}
ref::WorldTaskDeadlinePageState page_state(const State &s, std::uint64_t id) {
    ref::WorldTaskDeadlinePageState p;
    p.initialized = s.deadline_initialized.count(id) != 0;
    const auto get = [id](const auto &values, int initial) {
        const auto value = values.find(id);
        return value == values.end() ? initial : value->second;
    };
    p.phase = get(s.page_phases, 0);
    p.counter = get(s.page_counters, 0);
    p.returned = get(s.deadline_returns, -1);
    p.grade = get(s.deadline_grades, 0);
    return p;
}
bool page(State &s, const ref::WorldScriptPage &p, ref::WorldTaskDeadlinePageAction action,
          int selection, bool &denied) {
    auto current = page_state(s, p.id);
    const auto initial_grade =
        current.initialized ? std::optional<int>{current.grade} : grade(s, p);
    if (!initial_grade)
        return false;
    const auto r = ref::prepare_world_task_deadline_page(
        current,
        {action, selection, p.legacy_f, s.scene.world.world.ai.accounting.funds(), *initial_grade});
    if (!r.candidate)
        return false;
    const auto &c = *r.candidate;
    s.deadline_initialized.insert(p.id);
    s.page_phases[p.id] = c.state.phase;
    s.page_counters[p.id] = c.state.counter;
    s.deadline_returns[p.id] = c.state.returned;
    s.deadline_grades[p.id] = c.state.grade;
    denied = c.insufficient_funds;
    if (denied) {
        const auto script = ref::prepare_world_script(
            startup_world_runtime_catalog(), startup_world_runtime_scripts(s), {11, {}, {}});
        if (!script.candidate || !write_startup_world_runtime_scripts(s, script.candidate->state))
            return false;
    }
    if (c.closed) {
        const auto close =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), p.id);
        if (!close.candidate || !write_startup_world_runtime_scripts(s, close.candidate->state))
            return false;
        const auto original = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                           [&](const auto &v) { return v.id == p.id; });
        if (original == s.scripts.pages.end() || original->lifecycle != 4)
            return false;
        s.deadline_closed_page = *original;
    }
    return true;
}
bool restore_site(State &s, std::uint64_t id) {
    auto &world = s.scene.world.world;
    const auto site = s.sites.find(id);
    const auto facility = world.facilities.find(id);
    if (site == s.sites.end() || facility == world.facilities.end() ||
        site->second.occupied_cells.empty())
        return false;
    // n.o明确传success=true；中止也恢复全占地并排10显示，不能只删任务/改变一格颜色。
    for (const auto cell : site->second.occupied_cells) {
        if (cell.x < 0 || cell.y < 0 || cell.x >= world.map.width || cell.y >= world.map.height)
            return false;
        const auto index = static_cast<std::size_t>(cell.y * world.map.width + cell.x);
        auto &tile = world.map.cells.at(index);
        if (!tile.facility || tile.facility->instance_id.value != id)
            return false;
        tile.facility.reset();
        tile.legacy_state = 4;
        tile.category = ref::RouteCategory::ground;
        auto &surface = s.surface.at(index);
        surface.definition = s.ground_definition;
        surface.updates = 0;
        surface.instance = surface.fragment = -1;
        s.exploration_displays.push_back({cell, 10, 0, 30, -15});
    }
    world.facilities.erase(id);
    s.dungeon_facilities.erase(id);
    s.shops.erase(id);
    s.shop_order.erase(std::remove(s.shop_order.begin(), s.shop_order.end(), id),
                       s.shop_order.end());
    s.scene.world.facility_order.erase(
        std::remove(s.scene.world.facility_order.begin(), s.scene.world.facility_order.end(), id),
        s.scene.world.facility_order.end());
    s.neighbourhood_details.erase(id);
    ref::WorldMapRefreshState map;
    map.map = world.map;
    for (std::size_t i = 0; i < s.surface.size(); ++i) {
        const auto &v = s.surface[i];
        map.surface.push_back({v.definition, v.updates, v.display_definition, v.variant,
                               v.road_mask, v.fragment, v.instance, s.road_patches.at(i)[0],
                               s.road_patches.at(i)[1]});
    }
    for (const auto &d : s.rules->facilities)
        map.definitions.emplace(d.id,
                                ref::WorldMapDefinition{d.display_id, d.kind,
                                                        static_cast<ref::FacilityShape>(d.shape),
                                                        d.neighbour_effects});
    map.ground_definition = s.ground_definition;
    map.special_ground_definition = s.special_ground_definition;
    map.base_variants = s.base_variants;
    map.fence_level = s.fence_level;
    map.fence_levels = s.rules->fences;
    map.neighbours = s.neighbourhood_details;
    for (auto facility_id : s.scene.world.facility_order)
        map.facilities.push_back(world.facilities.at(facility_id).placement);
    const auto rebuilt = ref::prepare_world_map_refresh(map, true);
    if (!rebuilt.candidate)
        return false;
    world.map = rebuilt.candidate->state.map;
    s.neighbourhood_details = rebuilt.candidate->state.neighbours;
    s.neighbourhood.clear();
    for (const auto &n : s.neighbourhood_details)
        s.neighbourhood.emplace(n.first, n.second.current);
    s.scene.first_normal_refresh = rebuilt.candidate->state.refresh_pending;
    s.scene.world.surface.clear();
    for (std::size_t i = 0; i < s.surface.size(); ++i) {
        const auto &v = rebuilt.candidate->state.surface[i];
        s.surface[i] = {v.definition, v.updates, v.instance_field, v.fragment,
                        v.display,    v.variant, v.road_mask};
        s.road_patches[i] = {v.road_quad, v.edge_road_pair};
        s.scene.world.surface.push_back(static_cast<int>(world.map.cells[i].category));
    }
    return true;
}
bool abort_task(State &s) {
    if (!s.active_task)
        return true; // n.h()资格不成立时n.o直接返回，外层仍执行80/-10/162/26。
    const auto task = s.tasks.find(*s.active_task);
    if (task == s.tasks.end())
        return false;
    const auto d = s.task_progress.definitions.find(task->second.definition);
    if (d == s.task_progress.definitions.end())
        return false;
    if (d->second.kind == 0) {
        if (!task->second.facility || !s.scene.world.world.facilities.count(*task->second.facility))
            return true; // 原k.b()==null不调用k.d，不擅自清活动h。
        const auto id = *task->second.facility;
        auto &f = s.scene.world.world.facilities.at(id);
        f.status = 2;
        s.dungeon_facilities.at(id).updates = 0;
        const auto occupants = f.occupants; // 遍历原o序，包含重复身份，不按bl排序。
        for (std::size_t i = 0; i < occupants.size(); ++i) {
            const auto actor = occupants[i];
            s.scene.world.world.ai.battle.actors.at(actor).control.flags &= ~1U;
            const auto reset =
                ref::prepare_world_state_transition(s.scene.world.world, {actor, 0, {}});
            if (!reset.candidate ||
                i > static_cast<std::size_t>(std::numeric_limits<int>::max() / 5))
                return false;
            s.scene.world.world = reset.candidate->state;
            auto &a = s.scene.world.world.ai.battle.actors.at(actor);
            a.control.queue.push_back({1, static_cast<int>(i) * 5, 0});
            a.control.queue.push_back({8, 0});
        }
        if (!restore_site(s, id))
            return false;
    } else if (d->second.kind == 1 && s.task.encounter) {
        const auto event = s.scene.world.world.ai.encounters.find(*s.task.encounter);
        if (event == s.scene.world.world.ai.encounters.end())
            return false;
        const auto legacy = event->second.legacy_id;
        const auto monsters = s.scene.world.world.ai.monster_order;
        for (const auto actor : monsters) {
            const auto &a = s.scene.world.world.ai.battle.actors.at(actor);
            if (!a.encounter)
                continue;
            const auto live = s.scene.world.world.ai.encounters.find(*a.encounter);
            const auto retired = s.scene.world.world.ai.retired_encounters.find(*a.encounter);
            const auto *bound = live != s.scene.world.world.ai.encounters.end() ? &live->second
                                : retired != s.scene.world.world.ai.retired_encounters.end()
                                    ? &retired->second
                                    : nullptr;
            if (!bound || bound->legacy_id != legacy)
                continue;
            const auto cancel =
                ref::prepare_world_state_transition(s.scene.world.world, {actor, 3, {}});
            if (!cancel.candidate)
                return false;
            s.scene.world.world = cancel.candidate->state;
            auto &next = s.scene.world.world.ai.battle.actors.at(actor);
            next.state_parameter = 1;
            next.attack_position = next.position;
        }
        auto &e = s.scene.world.world.ai.encounters.at(*s.task.encounter);
        e.runtime.state = 1;
        e.runtime.counter = 0;
        const auto refresh = ref::prepare_world_event_map(s.scene.world.world.ai,
                                                          ref::world_schedule_facts(s.scene.world));
        if (!refresh.facts)
            return false;
        s.scene.world.map_flags = refresh.facts->flags;
    } else if (d->second.kind != 1)
        return true; // n.o只消费已证种类0/1。
    const bool music = d->second.kind == 1 && s.task.encounter.has_value();
    if (!consume_startup_world_runtime_task_encounter_request(
            s, {ref::EncounterRequestKind::clear_task, {}, 0, 0, 0}))
        return false;
    if (music)
        s.sound_requests.push_back({StartupAudioOperation::replace_bgm, 1}); // 原d/a.g，清h之后恢复背景音乐，不是额外现金/地图刷新。
    return true;
}
} // namespace

bool abort_startup_world_runtime_task_entities(State &s) { return abort_task(s); }

StartupWorldTaskPageResult act_startup_world_runtime_deadline_page(State &state, std::uint64_t id,
                                                                   int selection) {
    const auto p = top(state);
    if (!state.rules || state.scene.framework_paused || !p || p->id != id || p->legacy_page != 33)
        return {Error::invalid_page};
    auto next = state;
    next.scripts.executing_page = id;
    bool denied{};
    if (!page(next, *p, ref::WorldTaskDeadlinePageAction::confirm, selection, denied))
        return {Error::missing_source};
    next.scripts.executing_page.reset();
    state = std::move(next);
    return {Error::none,
            denied ? ref::TaskCommandDenial::insufficient_funds : ref::TaskCommandDenial::none};
}
std::optional<State> update_startup_world_runtime_deadline_page(const State &state,
                                                                std::uint64_t id) {
    const auto p = top(state);
    if (!state.rules || state.scene.framework_paused || !p || p->id != id || p->legacy_page != 33)
        return {};
    auto next = state;
    next.scripts.executing_page = id;
    bool denied{};
    if (!page(next, *p, ref::WorldTaskDeadlinePageAction::update, 0, denied))
        return {};
    next.scripts.executing_page.reset();
    return next;
}
std::optional<State> prepare_startup_world_runtime_deadline_result(const State &state) {
    if (!state.deadline_page)
        return state;
    const auto returned = state.deadline_returns.find(*state.deadline_page);
    if (returned == state.deadline_returns.end())
        return {};
    auto next = state;
    const auto adapter = startup_world_runtime_adapter();
    const auto result = ref::prepare_world_task_deadline_result(
        adapter.tasks.read(next), returned->second, next.simulation_steps + 1,
        [&](const ref::WorldCalendarTasksState &current, const ref::WorldTaskDeadlineEffect &effect)
            -> std::optional<ref::WorldCalendarTasksState> {
            if (!adapter.tasks.write(next, current))
                return {};
            using Kind = ref::WorldTaskDeadlineEffectKind;
            if (effect.kind == Kind::abort_task) {
                if (!abort_task(next))
                    return {};
            } else if (effect.kind == Kind::event) {
                const auto script = ref::prepare_world_script(
                    adapter.catalog, startup_world_runtime_scripts(next), {effect.id, {}, {}});
                if (!script.candidate ||
                    !write_startup_world_runtime_scripts(next, script.candidate->state))
                    return {};
            } else
                // c/n.ay[25/26]横幅，不是evtmsgs/opcode5消息页面目录。
                next.scripts.notices.push_back(
                    {effect.id, -1, 80, "",
                     effect.id == 25 ? "继续任务！" : "任务中止。街道人气<co=FF0E01>-10</co>"});
            return adapter.tasks.read(next);
        },
        next.deadline_closed_page);
    if (!result.candidate || !adapter.tasks.write(next, result.candidate->state))
        return {};
    const auto old = *state.deadline_page;
    next.deadline_closed_page.reset();
    next.deadline_initialized.erase(old);
    next.deadline_grades.erase(old);
    next.deadline_returns.erase(old);
    return next;
}
} // namespace ark::simulation
