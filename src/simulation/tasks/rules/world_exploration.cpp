#include "ark/simulation/tasks/rules/world_exploration.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation::rules {
namespace {
WorldScriptState script_projection(const WorldExplorationState &s) {
    WorldScriptState p;
    p.event_calls = s.finish.event_calls;
    p.continuations = s.scripts.continuations;
    p.context = s.scripts.context;
    p.village_name = s.scripts.village_name;
    p.pending_completion = s.finish.dungeon.world.ai.pending_completion;
    p.popularity_queue = s.popularity_queue;
    p.pages = s.ui.pages;
    p.executing_page = s.scripts.executing_page;
    p.next_page_id = s.scripts.next_page_id;
    p.page_mutations_locked = s.scripts.page_mutations_locked;
    p.selected_actor = s.scripts.selected_actor;
    p.selected_facility = s.scripts.selected_facility;
    p.selected_monster = s.scripts.selected_monster;
    for (const auto id : s.finish.dungeon.world.ai.human_order)
        p.human_order.push_back(id.value);
    return p;
}
void save_script(WorldExplorationState &s, const WorldScriptState &p) {
    s.finish.event_calls = p.event_calls;
    s.finish.dungeon.world.ai.pending_completion = p.pending_completion;
    s.popularity_queue = p.popularity_queue;
    s.ui.pages = p.pages;
    s.scripts = {p.continuations,  p.context,           p.village_name,
                 p.executing_page, p.next_page_id,      p.page_mutations_locked,
                 p.selected_actor, p.selected_facility, p.selected_monster};
}
std::optional<WorldMapRefreshState> map_projection(const WorldExplorationState &s) {
    const auto &finish = s.finish;
    const auto size = finish.surface.size();
    if (s.map.road_quad.size() != size || s.map.edge_road_pair.size() != size)
        return {};
    WorldMapRefreshState p;
    p.map = finish.dungeon.world.map;
    for (std::size_t n = 0; n < size; ++n) {
        const auto &cell = finish.surface[n];
        p.surface.push_back({cell.definition, cell.updates, cell.display_definition, cell.variant,
                             cell.road_mask, cell.fragment, cell.instance, s.map.road_quad[n],
                             s.map.edge_road_pair[n]});
    }
    p.definitions = s.map.definitions;
    p.ground_definition = finish.ground_definition;
    p.special_ground_definition = s.map.special_ground_definition;
    p.base_variants = s.map.base_variants;
    p.fence_level = s.map.fence_level;
    p.fence_levels = s.map.fence_levels;
    p.neighbours = s.map.neighbours;
    p.refresh_pending = s.map.refresh_pending;
    std::set<std::uint64_t> added;
    for (const auto id : s.map.facility_order) {
        const auto f = finish.dungeon.world.facilities.find(id);
        if (!id || !added.insert(id).second)
            return {};
        if (f == finish.dungeon.world.facilities.end()) {
            p.neighbours.erase(id); // 本次finish刚移除的对象从原g和邻接缓存一并移除。
            continue;
        }
        if (f->second.placement.instance_id.value != id)
            return {};
        p.facilities.push_back(f->second.placement);
    }
    for (const auto &f : finish.dungeon.world.facilities)
        if (!added.count(f.first))
            return {};
    return p;
}
void save_map(WorldExplorationState &s, const WorldMapRefreshState &p) {
    s.finish.dungeon.world.map = p.map;
    for (std::size_t n = 0; n < p.surface.size(); ++n) {
        const auto &cell = p.surface[n];
        s.finish.surface[n] = {cell.definition, cell.updates, cell.instance_field, cell.fragment,
                               cell.display,    cell.variant, cell.road_mask};
        s.map.road_quad[n] = cell.road_quad;
        s.map.edge_road_pair[n] = cell.edge_road_pair;
    }
    s.map.neighbours = p.neighbours;
    s.map.refresh_pending = p.refresh_pending;
    s.map.facility_order.clear();
    for (const auto &f : p.facilities)
        s.map.facility_order.push_back(f.instance_id.value);
}
bool insert_summary(WorldExplorationState &s, const DungeonFinishEffect &effect) {
    if (!effect.task || !s.finish.tasks.count(*effect.task) || s.scripts.next_page_id == 0 ||
        s.scripts.next_page_id == std::numeric_limits<std::uint64_t>::max())
        return false;
    const auto &task = s.finish.tasks.at(*effect.task);
    WorldScriptPage page;
    page.id = s.scripts.next_page_id++;
    page.lifecycle = 0;
    page.kind = WorldScriptPageKind::raw_page;
    page.legacy_page = effect.kind == DungeonFinishEffectKind::summary30 ? 30 : 32;
    page.legacy_f = effect.first;
    if (page.legacy_page == 32)
        page.legacy_g = effect.second;
    if (s.scripts.page_mutations_locked)
        return true; // 原框架l锁定时不插入；不是忽略脚本/奖励。
    const auto anchor =
        s.scripts.executing_page
            ? s.scripts.executing_page
            : (s.ui.pages.empty() ? std::optional<std::uint64_t>{} : s.ui.pages.back().id);
    auto position = s.ui.pages.end();
    if (anchor) {
        const auto current = std::find_if(s.ui.pages.begin(), s.ui.pages.end(),
                                          [&](const auto &p) { return p.id == *anchor; });
        if (current == s.ui.pages.end())
            return false;
        if (current->lifecycle != 0 && current->lifecycle != 4)
            position = current + 1;
    }
    s.ui.pages.insert(position, page);
    WorldExplorationSummary payload;
    payload.legacy_page = page.legacy_page;
    payload.task = *effect.task;
    payload.definition = task.definition;
    if (page.legacy_page == 32) {
        payload.rewards = effect.summary_rewards;
    }
    return s.ui.summaries.emplace(page.id, std::move(payload)).second;
}
WorldExplorationResult failure(WorldExplorationError error,
                               DungeonFinishError finish = DungeonFinishError::none,
                               WorldMapRefreshError map = WorldMapRefreshError::none,
                               WorldScriptError script = WorldScriptError::none) {
    return {error, finish, map, script, {}};
}
bool valid_summary_refs(const WorldExplorationState &s) {
    for (const auto &page : s.ui.pages)
        if ((page.legacy_page == 30 || page.legacy_page == 32) && !s.ui.summaries.count(page.id))
            return false;
    for (const auto &summary : s.ui.summaries) {
        const auto page = std::find_if(s.ui.pages.begin(), s.ui.pages.end(),
                                       [&](const auto &p) { return p.id == summary.first; });
        if (page == s.ui.pages.end() || page->legacy_page != summary.second.legacy_page ||
            !s.finish.tasks.count(summary.second.task) ||
            !s.finish.task_progress.definitions.count(summary.second.definition))
            return false;
    }
    return true;
}
bool valid_map_owner(const WorldExplorationState &s) {
    std::set<std::uint64_t> ids;
    if (s.map.facility_order.size() != s.finish.dungeon.world.facilities.size() ||
        s.map.neighbours.size() != s.finish.dungeon.world.facilities.size())
        return false;
    for (const auto id : s.map.facility_order)
        if (!id || !ids.insert(id).second || !s.finish.dungeon.world.facilities.count(id) ||
            !s.map.neighbours.count(id))
            return false;
    return true;
}
WorldScriptError script_scope_error(const WorldScriptCatalog &catalog, int event,
                                    std::set<int> &checked) {
    if (!checked.insert(event).second)
        return WorldScriptError::none;
    const auto program = catalog.events.find(event);
    if (program == catalog.events.end())
        return WorldScriptError::missing_event;
    for (const auto &command : program->second.commands) {
        if (command.empty())
            return WorldScriptError::invalid_input;
        const int opcode = command.front();
        if (opcode != 1 && opcode != 2 && opcode != 4 && opcode != 6 && opcode != 13 &&
            opcode != 22)
            return WorldScriptError::unsupported_opcode; // 未接共同所有者回写的领域不能丢弃改变。
        if (opcode == 1) {
            if (command.size() != 2)
                return WorldScriptError::invalid_input;
            const auto nested = script_scope_error(catalog, command[1], checked);
            if (nested != WorldScriptError::none)
                return nested;
        }
    }
    return WorldScriptError::none;
}
} // namespace
WorldExplorationResult prepare_world_exploration_finish(const WorldScriptCatalog &catalog,
                                                        const WorldExplorationState &s,
                                                        const DungeonFinishInput &input) {
    // preflight没有推进续体，确保成果页和后续脚本共享合法的框架栈。
    const auto script_preflight = validate_world_script_state(catalog, script_projection(s));
    if (script_preflight != WorldScriptError::none)
        return failure(WorldExplorationError::script_failed, DungeonFinishError::none,
                       WorldMapRefreshError::none, script_preflight);
    if (!valid_summary_refs(s) || !valid_map_owner(s) ||
        s.finish.dungeon.world.ai.task_active != s.finish.active_task.has_value())
        return failure(WorldExplorationError::invalid_input);
    WorldExplorationCandidate c{s, {}, {}, {}, {}, 0, false, false};
    WorldExplorationError error{WorldExplorationError::none};
    WorldMapRefreshError map_error{WorldMapRefreshError::none};
    WorldScriptError script_error{WorldScriptError::none};
    const auto result = prepare_world_dungeon_finish(
        s.finish, input,
        [&](const DungeonFinishState &finish,
            const DungeonFinishEffect &effect) -> std::optional<DungeonFinishState> {
            c.state.finish = finish;
            switch (effect.kind) {
            case DungeonFinishEffectKind::actor_reward_display: {
                if (!effect.actor || effect.first == std::numeric_limits<int>::min()) {
                    error = WorldExplorationError::display_failed;
                    return {};
                }
                const auto context = c.state.finish.dungeon.world.ai.contexts.find(*effect.actor);
                if (context == c.state.finish.dungeon.world.ai.contexts.end() ||
                    !valid_actor_effect_state(context->second.effects)) {
                    error = WorldExplorationError::display_failed;
                    return {};
                }
                context->second.effects.display.push_back(
                    {24, -effect.first, 0, effect.second, 0, 0});
                if (!valid_actor_effect_state(context->second.effects)) {
                    error = WorldExplorationError::display_failed;
                    return {};
                }
                break;
            }
            case DungeonFinishEffectKind::summary30:
            case DungeonFinishEffectKind::summary32:
                if (!insert_summary(c.state, effect)) {
                    error = WorldExplorationError::page_failed;
                    return {};
                }
                break;
            case DungeonFinishEffectKind::event: {
                std::set<int> checked;
                const auto scope = script_scope_error(catalog, effect.first, checked);
                if (scope != WorldScriptError::none) {
                    error = WorldExplorationError::script_failed;
                    script_error = scope;
                    return {};
                }
                const auto script = prepare_world_script(catalog, script_projection(c.state),
                                                         {effect.first, {}, {}});
                if (!script.candidate) {
                    error = WorldExplorationError::script_failed;
                    script_error = script.error;
                    return {};
                }
                save_script(c.state, script.candidate->state);
                c.script_trace.insert(c.script_trace.end(), script.candidate->executed.begin(),
                                      script.candidate->executed.end());
                break;
            }
            case DungeonFinishEffectKind::site_success_display:
                if (!effect.cell) {
                    error = WorldExplorationError::display_failed;
                    return {};
                }
                c.state.ui.ground_displays.push_back({*effect.cell, 10, 0, 30, -15});
                break;
            case DungeonFinishEffectKind::rebuild_display:
            case DungeonFinishEffectKind::rebuild_roads_fences:
            case DungeonFinishEffectKind::rebuild_neighbours: {
                const auto projection = map_projection(c.state);
                if (!projection) {
                    error = WorldExplorationError::map_failed;
                    map_error = WorldMapRefreshError::invalid_input;
                    return {};
                }
                const auto map = effect.kind == DungeonFinishEffectKind::rebuild_display
                                     ? prepare_world_map_display(*projection)
                                 : effect.kind == DungeonFinishEffectKind::rebuild_roads_fences
                                     ? prepare_world_map_roads(*projection)
                                     : prepare_world_map_neighbours(*projection, effect.first != 0);
                if (!map.candidate) {
                    error = WorldExplorationError::map_failed;
                    map_error = map.error;
                    return {};
                }
                save_map(c.state, map.candidate->state);
                c.map_steps.insert(c.map_steps.end(), map.candidate->steps.begin(),
                                   map.candidate->steps.end());
                break;
            }
            case DungeonFinishEffectKind::refresh_scene:
                c.state.map.refresh_pending = true;
                c.map_steps.push_back(WorldMapRefreshStep::scene_refresh);
                break;
            case DungeonFinishEffectKind::threshold_notice: {
                static const std::array<std::string, 3> texts{"迷之巨大生物觉醒了!",
                                                              "在隔壁街道发现迷之巨大生物!",
                                                              "听见了迷之巨大生物的脚步声"};
                if (effect.first < 29 || effect.first > 31) {
                    error = WorldExplorationError::display_failed;
                    return {};
                }
                c.state.ui.messages.push_back({texts[effect.first - 29], {effect.first, -100, 80}});
                break;
            }
            }
            return c.state.finish;
        });
    if (!result.candidate)
        return failure(error == WorldExplorationError::none ? WorldExplorationError::finish_failed
                                                            : error,
                       result.error, map_error, script_error);
    c.state.finish = result.candidate->state;
    c.requests = result.candidate->requests;
    c.effects = result.candidate->effects;
    c.consumed_summary_tickets = result.candidate->consumed_summary_tickets;
    c.site_restored = result.candidate->site_restored;
    c.task_cleared = result.candidate->task_cleared;
    return {WorldExplorationError::none, DungeonFinishError::none, WorldMapRefreshError::none,
            WorldScriptError::none, std::move(c)};
}
WorldExplorationResult prepare_world_exploration_continuations(const WorldScriptCatalog &catalog,
                                                               const WorldExplorationState &s,
                                                               bool admitted) {
    if (!valid_summary_refs(s) ||
        s.finish.dungeon.world.ai.task_active != s.finish.active_task.has_value())
        return failure(WorldExplorationError::invalid_input);
    for (const auto &continuation : s.scripts.continuations) {
        std::set<int> checked;
        const auto scope = admitted ? script_scope_error(catalog, continuation.event, checked)
                                    : WorldScriptError::none;
        if (scope != WorldScriptError::none)
            return failure(WorldExplorationError::script_failed, DungeonFinishError::none,
                           WorldMapRefreshError::none, scope);
    }
    const auto script = prepare_world_script_continuations(catalog, script_projection(s), admitted);
    if (!script.candidate)
        return failure(WorldExplorationError::script_failed, DungeonFinishError::none,
                       WorldMapRefreshError::none, script.error);
    WorldExplorationCandidate c{s, {}, {}, {}, script.candidate->executed, 0, false, false};
    save_script(c.state, script.candidate->state);
    return {WorldExplorationError::none, DungeonFinishError::none, WorldMapRefreshError::none,
            WorldScriptError::none, std::move(c)};
}
} // namespace ark::simulation::rules
