#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_reference/world_gift_page.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
bool unlock_human_valid(const State &s, const ref::WorldScriptPage &p) {
    // b/g:3467的X仅含bv[f]；aq由到访排序读取，不在此页直接创建人物实例。
    return s.rules && s.human_calendar.count(p.legacy_f) &&
           std::any_of(s.rules->humans.begin(), s.rules->humans.end(),
                       [&](const auto &h) { return h.identity == p.legacy_f; });
}
std::optional<int> event_message_command(const State &s, const ref::WorldScriptPage &p) {
    const auto phase = s.page_phases.find(p.id);
    const int index = phase == s.page_phases.end() ? 0 : phase->second;
    if (p.message_commands.empty() || p.message_commands.size() != p.paragraphs.size() ||
        index < 0 || static_cast<std::size_t>(index) >= p.message_commands.size())
        return {};
    return p.message_commands.at(static_cast<std::size_t>(index));
}
bool consume_task_display(State &s, const ref::WorldScriptPage &page,
                          ref::WorldTaskDisplayAction action) {
    if (!page.monster_definition || *page.monster_definition < 0 ||
        *page.monster_definition >= static_cast<int>(s.rules->monsters.size()))
        return false;
    const ref::WorldTaskDisplayState display{
        page.legacy_page, s.task_display_initialized.count(page.id) != 0, s.task_display_table};
    const auto result = ref::prepare_world_task_display_page(
        display, {action, s.page_counters[page.id]}, s.scene.random);
    if (!result.candidate)
        return false;
    s.task_display_table = result.candidate->state.bd;
    s.scene.random = result.candidate->random;
    s.task_display_initialized.insert(page.id);
    if (result.candidate->closed) {
        const auto r =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), page.id);
        if (!r.candidate || !write_startup_world_runtime_scripts(s, r.candidate->state))
            return false;
    }
    return true;
}
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
        const auto script = ref::prepare_world_script(
            startup_world_runtime_catalog(), startup_world_runtime_scripts(s), {48, {}, {}});
        if (!script.candidate || !write_startup_world_runtime_scripts(s, script.candidate->state))
            return false;
        const auto closed =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
        return closed.candidate && write_startup_world_runtime_scripts(s, closed.candidate->state);
    }
    if (!refresh_startup_world_runtime_rank(s))
        return false;
    s.page_counters[id] = 0;
    return true;
}
ref::WorldAwardPageState award_projection(const State &s, std::uint64_t id) {
    ref::WorldAwardPageState a;
    a.medal_count = s.medal_count;
    const auto counter = s.page_counters.find(id);
    if (counter != s.page_counters.end())
        a.page_counter = counter->second;
    const auto rankings = s.award_rankings.find(id);
    if (rankings != s.award_rankings.end()) {
        a.initialized = true;
        a.ranked_definitions = rankings->second;
        a.announced = s.award_announced.at(id);
        a.termination_pending = s.award_termination_pending.at(id);
    }
    for (const auto &h : s.rules->humans) {
        const auto &extra = s.human_calendar.at(h.identity);
        auto totals = extra.yearly_totals;
        totals[1] = s.scene.world.world.ai.battle.humans.at(h.identity).killed_stat1;
        totals[2] = s.scene.world.world.human_spending.at(h.identity);
        a.humans.push_back(
            {h.identity, s.human_presence.at(h.identity), totals, extra.contribution});
    }
    return a;
}
bool consume_award(State &s, std::uint64_t id, ref::WorldAwardAction action) {
    const auto initialized = ref::prepare_world_award_page_initialization(award_projection(s, id));
    if (!initialized.candidate)
        return false;
    const auto result = ref::prepare_world_award_page(initialized.candidate->state, action);
    if (!result.candidate)
        return false;
    const auto &a = result.candidate->state;
    s.medal_count = a.medal_count;
    s.award_rankings[id] = a.ranked_definitions;
    s.award_announced[id] = a.announced;
    s.award_termination_pending[id] = a.termination_pending;
    for (const auto &h : a.humans)
        s.human_calendar.at(h.definition).contribution = h.contribution;
    for (const auto &effect : result.candidate->effects) {
        using Kind = ref::WorldAwardEffectKind;
        if (effect.kind == Kind::sound)
            s.sound_requests.push_back(effect.value);
        else if (effect.kind == Kind::refresh)
            s.sound_requests.push_back(s.active_task && s.task.encounter ? 2 : 1);
        else if (effect.kind == Kind::event) {
            const auto script = ref::prepare_world_script(
                startup_world_runtime_catalog(), startup_world_runtime_scripts(s),
                {effect.value,
                 effect.event_argument ? std::to_string(*effect.event_argument) : "",
                 {}});
            if (!script.candidate ||
                !write_startup_world_runtime_scripts(s, script.candidate->state))
                return false;
        } else if (effect.kind == Kind::close) {
            const auto closed =
                ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
            if (!closed.candidate ||
                !write_startup_world_runtime_scripts(s, closed.candidate->state))
                return false;
        }
        // termination_prompt由显式测试输入消费，窗口尚未接按钮10/是非弹窗。
    }
    return true;
}
} // namespace

Error act_startup_world_runtime_award_page(State &state, std::uint64_t id,
                                           ref::WorldAwardAction action) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (state.scene.framework_paused || top == state.scripts.pages.rend() || top->id != id ||
        top->kind != ref::WorldScriptPageKind::raw_page || top->legacy_page != 87)
        return Error::invalid_page;
    auto next = state;
    next.scripts.executing_page = id;
    auto &counter = next.page_counters[id];
    if (counter == std::numeric_limits<int>::max())
        return Error::missing_source;
    ++counter;
    if (!consume_award(next, id, action))
        return Error::missing_source;
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}

Error acknowledge_startup_world_runtime_page(State &state, std::uint64_t id) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->id != id ||
        top->kind == ref::WorldScriptPageKind::scene)
        return Error::invalid_page;
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 33)
        return act_startup_world_runtime_deadline_page(state, id, 0).error;
    auto next = state;
    next.scripts.executing_page = id;
    if (top->kind == ref::WorldScriptPageKind::raw_page) {
        const auto adapter = startup_world_runtime_adapter();
        if (top->legacy_page == 11) {
            // b/g:11367：确认只快进到40或关闭；不再次执行奖励/指令22续体。
            if (!event_message_command(next, *top))
                return Error::missing_source;
            if (next.page_counters[id] < 40)
                next.page_counters[id] = 40;
            else {
                const auto closed =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
                if (!closed.candidate ||
                    !write_startup_world_runtime_scripts(next, closed.candidate->state))
                    return Error::script_failed;
            }
        } else if (top->legacy_page == 59) {
            if (!unlock_human_valid(next, *top))
                return Error::missing_source;
            // b/g:4879、aM={60,70}：早确认不快进，满70才写aq10并关闭。
            if (next.page_counters[id] >= 70) {
                next.human_calendar.at(top->legacy_f).absent_months = 10;
                const auto closed =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
                if (!closed.candidate ||
                    !write_startup_world_runtime_scripts(next, closed.candidate->state))
                    return Error::script_failed;
            }
        } else if (top->legacy_page == 99 || top->legacy_page == 100) {
            if (!consume_task_display(next, *top, ref::WorldTaskDisplayAction::confirm))
                return Error::missing_source;
        } else if (top->legacy_page == 49) {
            if (!initialize_rank_page(next, id))
                return Error::missing_source;
            if (next.rank < 5) {
                // b/g.g L6e/L1a8：确认置u8后关闭，页49绝不走页48的晋级消费者。
                next.scripts.user_flags |= 8;
                const auto result =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
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
        } else if (top->legacy_page == 89) {
            // b/g.g:6003：早确认仅快进；40之后关闭，不再重复执行介绍脚本。
            if (next.page_counters[id] < 40)
                next.page_counters[id] = 40;
            else {
                const auto result =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
                if (!result.candidate ||
                    !write_startup_world_runtime_scripts(next, result.candidate->state))
                    return Error::script_failed;
            }
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

Error cancel_startup_world_runtime_page(State &state, std::uint64_t id) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (state.scene.framework_paused || top == state.scripts.pages.rend() || top->id != id ||
        top->kind != ref::WorldScriptPageKind::raw_page || top->legacy_page != 83)
        return Error::invalid_page;
    // b/g:5745：按钮2走m()；仅退出商店追加菜单，购买84/85未接，不虚构确认。
    auto next = state;
    next.scripts.executing_page = id;
    const auto closed =
        ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
    if (!closed.candidate || !write_startup_world_runtime_scripts(next, closed.candidate->state))
        return Error::script_failed;
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
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 33)
        return update_startup_world_runtime_deadline_page(state, top->id);
    auto next = state;
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 97) {
        // b/g.b:6172：实际更新清R并m()；不是玩家确认页，不再执行奖励脚本。
        const auto adapter = startup_world_runtime_adapter();
        const auto result =
            ref::prepare_world_popularity_unlock_page(adapter.popularity.read(next), top->id);
        if (!result.candidate || !adapter.popularity.write(next, result.candidate->state))
            return {};
        return next;
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 24 || top->legacy_page == 28))
        return update_startup_world_runtime_task_page(state, top->id, state.page_confirm_held);
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
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 59) {
        if (!unlock_human_valid(next, *top))
            return {};
        if (counter == 1)
            next.sound_requests.push_back(5);
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 11) {
        const auto command = event_message_command(next, *top);
        if (!command)
            return {};
        // b/g:11368：首轮0播放4、1播放6，其余代码没有此音效。
        if (counter == 1 && (*command == 0 || *command == 1))
            next.sound_requests.push_back(*command == 0 ? 4 : 6);
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 99 || top->legacy_page == 100) &&
        !consume_task_display(next, *top, ref::WorldTaskDisplayAction::update))
        return {};
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 87 &&
        !consume_award(next, top->id, ref::WorldAwardAction::update))
        return {};
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 30 && counter == 1 &&
        next.page_phases[top->id] == 0)
        next.sound_requests.push_back(4);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 16) {
        // d/a opcode7→g.a(L)，b/g.b先--L再走f--；至少一次更新，不接受确认跳过。
        if (counter >= std::max(1, top->legacy_l)) {
            const auto result =
                ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), top->id);
            if (!result.candidate ||
                !write_startup_world_runtime_scripts(next, result.candidate->state))
                return {};
        }
    } else if (top->kind == ref::WorldScriptPageKind::raw_page &&
               (top->legacy_page == 56 || top->legacy_page == 57)) {
        ref::WorldScriptCameraFocusInput input;
        input.page = top->id;
        input.camera = next.camera;
        input.previous_camera = next.previous_camera;
        input.previous_velocity = next.camera_velocity;
        if (top->legacy_page == 57 && !next.task_order.empty()) {
            const auto task = next.tasks.find(next.task_order.front());
            if (task == next.tasks.end())
                return {};
            if (task->second.facility) {
                input.first_task_facility_view =
                    startup_world_runtime_facility_target(next, *task->second.facility);
                if (!input.first_task_facility_view)
                    return {};
            }
        } else if (top->legacy_page == 56 && !next.scene.world.world.ai.monster_order.empty()) {
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
} // namespace dungeon_village_prototype
