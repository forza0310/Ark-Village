#include "ark/simulation/startup_world_runtime.hpp"
#include "ark/simulation/startup_world_runtime_tasks.hpp"
#include "ark/simulation/rules/world_gift_page.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
ref::WorldGiftPageState gift(const State &s, const ref::WorldRuntimeAdapter<State> &adapter) {
    ref::WorldGiftPageState g;
    g.facility = adapter.facilities.read(s);
    g.facility_order = s.scene.world.facility_order;
    for (const auto &d : s.rules->facilities) {
        const auto value = [](const auto &map, int id) {
            const auto found = map.find(id);
            return found == map.end() ? 0 : static_cast<int>(found->second);
        };
        g.facility_unlocks.emplace(
            d.id, ref::WorldGiftFacilityDefinition{s.facility_presence.at(d.id),
                                                   value(s.facility_unlock_notices, d.id) != 0,
                                                   value(s.facility_unlock_counters, d.id),
                                                   value(s.facility_free_builds, d.id)});
    }
    return g;
}
bool write_gift(State &s, const ref::WorldGiftPageState &g,
                const ref::WorldRuntimeAdapter<State> &adapter) {
    if (!adapter.facilities.write(s, g.facility))
        return false;
    for (const auto &d : g.facility_unlocks) {
        s.facility_presence.at(d.first) = d.second.status;
        s.facility_unlock_notices[d.first] = d.second.pending_notice;
        s.facility_unlock_counters[d.first] = d.second.unlock_counter;
        s.facility_free_builds[d.first] = d.second.free_builds;
    }
    return true;
}
bool initialize_rank_page(State &s, std::uint64_t id) {
    if (s.page_counters.count(id))
        return true;
    if (s.rank >= 5) {
        const auto script = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                     startup_world_runtime_scripts(s), {48, {}, {}});
        if (!script.candidate || !write_startup_world_runtime_scripts(s, script.candidate->state))
            return false;
        const auto closed = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
        return closed.candidate && write_startup_world_runtime_scripts(s, closed.candidate->state);
    }
    if (!refresh_startup_world_runtime_rank(s))
        return false;
    s.page_counters[id] = 0;
    return true;
}
} // namespace

Error acknowledge_startup_world_runtime_page(State &state, std::uint64_t id) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->id != id ||
        top->kind == ref::WorldScriptPageKind::scene)
        return Error::invalid_page;
    auto next = state;
    next.scripts.executing_page = id;
    if (top->kind == ref::WorldScriptPageKind::raw_page) {
        const auto adapter = startup_world_runtime_adapter();
        if (top->legacy_page == 49) {
            if (!initialize_rank_page(next, id))
                return Error::missing_source;
            if (next.rank < 5) {
                // b/g.g L6e/L1a8：确认置u8后关闭，页49绝不走页48的晋级消费者。
                next.scripts.user_flags |= 8;
                const auto result = ref::prepare_world_script_close_page(
                    startup_world_runtime_scripts(next), id);
                if (!result.candidate ||
                    !write_startup_world_runtime_scripts(next, result.candidate->state))
                    return Error::script_failed;
            }
        } else if (top->legacy_page == 31) {
            // 输入可能先于下一框架入口；仍须初始化X/H，再关页，不能绕过原初始化。
            if (!initialize_startup_world_runtime_task_result_page(next, id))
                return Error::missing_source;
            const auto result =
                ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
            if (!result.candidate ||
                !write_startup_world_runtime_scripts(next, result.candidate->state))
                return Error::script_failed;
        } else if (top->legacy_page == 94) {
            const auto result = ref::prepare_world_gift_page(
                gift(next, adapter), {id, next.page_counters[id], true}, adapter.catalog);
            if (!result.candidate || !write_gift(next, result.candidate->state, adapter))
                return Error::script_failed;
            next.page_counters[id] = result.candidate->counter;
        } else if (top->legacy_page == 30 && next.page_counters[id] < 40) {
            next.page_counters[id] = 40;
        } else if (top->legacy_page == 30 && next.page_phases[id] == 0) {
            next.page_phases[id] = 1;
            next.page_counters[id] = 0;
        } else if (top->legacy_page == 30 || top->legacy_page == 32) {
            // 阶段2已经提交奖励；成果页只展示与关闭，绝不再支付一次。
            if (!next.exploration_summaries.count(id))
                return Error::missing_source;
            const auto result =
                ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
            if (!result.candidate ||
                !write_startup_world_runtime_scripts(next, result.candidate->state))
                return Error::script_failed;
        } else if (top->legacy_page == 97) {
            const auto result =
                ref::prepare_world_popularity_unlock_page(adapter.popularity.read(next), id);
            if (!result.candidate || !adapter.popularity.write(next, result.candidate->state))
                return Error::script_failed;
        } else
            return Error::missing_source; // 任务结果/税收等页有独立域动作，不能只关页。
    } else {
        const auto result =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
        if (!result.candidate ||
            !write_startup_world_runtime_scripts(next, result.candidate->state))
            return Error::script_failed;
    }
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}

std::optional<State> update_startup_world_runtime_page(const State &state) {
    if (state.scene.framework_paused)
        return state;
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->kind == ref::WorldScriptPageKind::scene)
        return {};
    auto next = state;
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 49) {
        if (!initialize_rank_page(next, top->id))
            return {};
        if (next.rank >= 5)
            return next;
    }
    auto &counter = next.page_counters[top->id];
    if (counter == std::numeric_limits<int>::max())
        return {};
    ++counter;
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 30 && counter == 1 &&
        next.page_phases[top->id] == 0)
        next.sound_requests.push_back(4);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 56) {
        ref::WorldScriptCameraFocusInput input;
        input.page = top->id;
        input.camera = next.camera;
        input.previous_camera = next.previous_camera;
        input.previous_velocity = next.camera_velocity;
        if (!next.scene.world.world.ai.monster_order.empty()) {
            const auto id = next.scene.world.world.ai.monster_order.front();
            const auto meta = next.actor_metadata.find(id);
            if (meta == next.actor_metadata.end())
                return {};
            input.first_monster_cached_view = {static_cast<float>(meta->second.cached_view.x),
                                               static_cast<float>(meta->second.cached_view.y)};
        }
        const auto result =
            ref::prepare_world_script_camera_focus(startup_world_runtime_scripts(next), input);
        if (!result.candidate ||
            !write_startup_world_runtime_scripts(next, result.candidate->state))
            return {};
        next.camera = result.candidate->camera;
        next.previous_camera = result.candidate->previous_camera;
        next.camera_velocity = result.candidate->velocity;
    } else if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 94) {
        const auto adapter = startup_world_runtime_adapter();
        const auto result = ref::prepare_world_gift_page(
            gift(next, adapter), {top->id, counter, false}, adapter.catalog);
        if (!result.candidate || !write_gift(next, result.candidate->state, adapter))
            return {};
        if (result.candidate->sound)
            next.sound_requests.push_back(*result.candidate->sound);
    }
    return next;
}
} // namespace ark::simulation
