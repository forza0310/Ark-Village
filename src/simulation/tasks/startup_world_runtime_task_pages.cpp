#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Command = ref::WorldTaskCommandState;
using Effect = ref::TaskCommandEffect;
const ref::WorldScriptPage *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
bool close(State &s, std::uint64_t page) {
    const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), page);
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
std::optional<std::uint64_t> open(State &s, int raw, std::optional<std::uint64_t> task) {
    ref::WorldScriptPage p;
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = raw;
    p.task_identity = task;
    if (task) {
        const auto t = s.tasks.find(*task);
        if (t == s.tasks.end())
            return {};
        p.task_definition = t->second.definition;
    }
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(s), p);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, r.candidate->state))
        return {};
    const auto id = r.candidate->inserted_pages.front().id;
    s.page_counters[id] = 0;
    return id;
}
Command project(const State &s, const ref::WorldScriptPage &page) {
    Command c;
    c.finish = startup_world_runtime_finish(s);
    c.random = s.scene.random;
    for (const auto &d : s.rules->tasks)
        c.definitions.emplace(d.factory.identity,
                              ref::TaskCommandDefinition{d.factory.identity, d.recruitment_fee,
                                                         d.crew_rating_penalty, d.name});
    for (const auto &h : s.rules->humans) {
        const auto &g = s.scene.world.world.ai.growth.at(h.identity).definition;
        const auto &extra = s.human_calendar.at(h.identity);
        c.humans.push_back({h.identity, s.human_presence.at(h.identity), g.legacy_u,
                            extra.celebrations, g.profession_levels, extra.continuation_cost,
                            s.human_definition_state.at(h.identity)});
    }
    c.facility_difficulties = s.facility_difficulties;
    c.selected_task = page.task_identity;
    using Phase = ref::TaskCommandPhase;
    c.phase = page.legacy_page == 24   ? Phase::recruiting
              : page.legacy_page == 25 ? Phase::team
              : page.legacy_page == 26 ? Phase::active_team
              : page.legacy_page == 27 ? Phase::extra
              : page.legacy_page == 28 ? Phase::departure_prompt
                                       : Phase::offer;
    const auto counter = s.page_counters.find(page.id);
    if (counter != s.page_counters.end())
        c.page_counter = counter->second;
    const auto secondary = s.page_secondary_counters.find(page.id);
    if (secondary != s.page_secondary_counters.end())
        c.secondary_counter = secondary->second;
    const auto recruitment = s.task_recruitment_pages.find(page.id);
    if (recruitment != s.task_recruitment_pages.end())
        c.recruitment = recruitment->second;
    const auto extras = s.task_extra_pages.find(page.id);
    if (extras != s.task_extra_pages.end())
        c.extra_candidates = extras->second;
    const auto phase = s.page_phases.find(page.id);
    if (page.legacy_page == 28 && phase != s.page_phases.end() && phase->second == 1)
        c.phase = Phase::departing;
    const auto acceleration = s.task_page_acceleration.find(page.id);
    if (acceleration != s.task_page_acceleration.end())
        c.accelerate_departure = acceleration->second;
    const auto prediction = s.task_page_predictions.find(page.id);
    if (prediction != s.task_page_predictions.end())
        c.predicted_result = prediction->second;
    c.active_task_updates = s.scene.world.updates;
    c.task_subperiods = s.task_subperiods;
    c.clock_parameter = s.clock_parameter;
    c.cash_period = s.simulation_steps + 1;
    c.event9_seen = ref::world_script_seen(s.scripts, 9);
    return c;
}
bool write(State &s, const Command &c) {
    if (!write_startup_world_runtime_finish(s, c.finish))
        return false;
    s.scene.random = c.random;
    for (const auto &h : c.humans) {
        s.human_calendar.at(h.identity).continuation_cost = h.extra_fee;
        s.human_definition_state.at(h.identity) = h.absence;
    }
    s.scene.world.updates = c.active_task_updates;
    s.task_subperiods = c.task_subperiods;
    // h的kind/site实时投影给人物P/F；不凭页28的显示预测制造遭遇或人物。
    if (s.active_task) {
        const auto &t = s.tasks.at(*s.active_task);
        if (!t.site)
            return false;
        s.task.kind = s.task_progress.definitions.at(t.definition).kind;
        s.task.center = *t.site;
    }
    return true;
}
// 回调只修改整轮私有Owner；每个副作用前同步领域候选，之后返还脚本改变的共享字段。
ref::TaskCommandConsumer consumer(State &s, std::uint64_t source, std::uint64_t &target) {
    return [&, source](const Command &c, const Effect &e) -> std::optional<Command> {
        if (!write(s, c))
            return {};
        using Kind = ref::TaskCommandEffectKind;
        if (e.kind == Kind::open_page) {
            const auto id = open(s, e.first, e.task);
            if (!id)
                return {};
            target = *id;
        } else if (e.kind == Kind::close_page) {
            // 页27无候选时先关刚插入页；其余关闭实际执行的原页面。
            const auto p =
                std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(), [&](const auto &v) {
                    return v.id == source && v.legacy_page == e.first;
                });
            if (!close(s, p == s.scripts.pages.end() ? target : source))
                return {};
        } else if (e.kind == Kind::reset_main_scene) {
            s.scene.scene_state = 0;
            s.scene.scene_counter = 0;
        } else if (e.kind == Kind::event) {
            const auto r =
                ref::prepare_world_script(startup_world_runtime_catalog(),
                                          startup_world_runtime_scripts(s), {e.first, e.text, {}});
            if (!r.candidate || !write_startup_world_runtime_scripts(s, r.candidate->state))
                return {};
            if (e.human) {
                for (const auto &page : r.candidate->inserted_pages) {
                    auto found = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                              [&](const auto &p) { return p.id == page.id; });
                    if (found == s.scripts.pages.end())
                        return {};
                    found->speaker_kind = 1;
                    found->speaker_definition = *e.human;
                }
            }
        } else if (e.kind == Kind::activate_main_scene) {
            // kairo/android/a/b.a(a)：移出scene、标其余页关闭，再将scene放回栈顶。
            const auto scene =
                std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(), [](const auto &p) {
                    return p.kind == ref::WorldScriptPageKind::scene;
                });
            if (scene == s.scripts.pages.end() || s.scripts.page_mutations_locked)
                return {};
            auto main = *scene;
            s.scripts.pages.erase(scene);
            for (auto &p : s.scripts.pages)
                p.lifecycle = 4;
            main.lifecycle = 2;
            s.scripts.pages.push_back(main);
        } else if (e.kind == Kind::notice15) {
            // 原av.b(15,name)是横幅通知，不是音乐。
            s.scripts.notices.push_back(
                {15, -1, 80, e.text, "<co=0064FF>" + e.text + "</co> 开始"});
        }
        auto out = c;
        out.finish = startup_world_runtime_finish(s);
        out.random = s.scene.random;
        out.task_subperiods = s.task_subperiods;
        out.event9_seen = ref::world_script_seen(s.scripts, 9);
        return out;
    };
}
StartupWorldTaskPageResult commit(State &state, State &next, std::uint64_t target,
                                  const ref::TaskCommandResult &r) {
    if (!r.candidate || !write(next, r.candidate->state))
        return {Error::missing_source};
    const auto &c = r.candidate->state;
    next.page_counters[target] = c.page_counter;
    next.page_secondary_counters[target] = c.secondary_counter;
    if (c.phase == ref::TaskCommandPhase::recruiting)
        next.task_recruitment_pages[target] = c.recruitment;
    else if (c.phase == ref::TaskCommandPhase::extra)
        next.task_extra_pages[target] = c.extra_candidates;
    else if (c.phase == ref::TaskCommandPhase::departure_prompt ||
             c.phase == ref::TaskCommandPhase::departing) {
        next.page_phases[target] = c.phase == ref::TaskCommandPhase::departing ? 1 : 0;
        next.task_page_predictions[target] = c.predicted_result;
        next.task_page_acceleration[target] = c.accelerate_departure;
    }
    next.scripts.executing_page.reset();
    state = std::move(next);
    return {Error::none, r.candidate->denial, r.candidate->accepted, r.candidate->departed};
}
} // namespace

Error open_startup_world_runtime_task_menu(State &state) {
    const auto p = top(state);
    if (!state.rules || state.scene.framework_paused || !p ||
        p->kind != ref::WorldScriptPageKind::scene)
        return Error::invalid_page;
    auto next = state;
    next.scripts.executing_page = p->id;
    const auto id = open(next, state.active_task ? 26 : 22, state.active_task);
    if (!id)
        return Error::script_failed;
    if (!state.active_task)
        next.task_page_lists[*id] = next.task_order;
    if (!state.active_task && next.task_order.empty()) {
        const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                 startup_world_runtime_scripts(next), {29, {}, {}});
        if (!r.candidate || !write_startup_world_runtime_scripts(next, r.candidate->state) ||
            !close(next, *id))
            return Error::script_failed;
    }
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}
Error open_startup_world_runtime_task_control_menu(State &state) {
    const auto *p = top(state);
    if (!state.rules || state.scene.framework_paused || !p ||
        p->kind != ref::WorldScriptPageKind::scene || !state.active_task)
        return Error::invalid_page;
    auto next = state;
    next.scripts.executing_page = p->id;
    if (!open(next, 4, state.active_task))
        return Error::script_failed;
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}

StartupWorldTaskPageResult act_startup_world_runtime_task_page(State &state, std::uint64_t id,
                                                               StartupWorldTaskAction action,
                                                               int selection) {
    const auto p = top(state);
    if (!state.rules || state.scene.framework_paused || !p || p->id != id ||
        p->kind != ref::WorldScriptPageKind::raw_page)
        return {Error::invalid_page};
    auto next = state;
    next.scripts.executing_page = id;
    const int raw = p->legacy_page;
    if (raw == 60 && next.page_human_bindings.count(id)) {
        if (action != StartupWorldTaskAction::cancel || !close(next, id))
            return {Error::invalid_page};
        next.scripts.executing_page.reset();
        state = std::move(next);
        return {};
    }
    if (raw == 1 && next.task_abort_questions.count(id)) {
        if (action != StartupWorldTaskAction::confirm || selection < 0 || selection > 1)
            return {Error::invalid_page};
        const auto parent = next.task_abort_questions.at(id);
        const auto owner =
            std::find_if(next.scripts.pages.begin(), next.scripts.pages.end(), [&](const auto &v) {
                return v.id == parent && v.lifecycle != 4 && v.legacy_page == 4;
            });
        if (owner == next.scripts.pages.end() || next.task_abort_answers.count(parent) ||
            !close(next, id))
            return {Error::invalid_page};
        next.task_abort_answers[parent] = selection;
        next.scripts.executing_page.reset();
        state = std::move(next);
        return {};
    }
    if (raw == 4) {
        if (action == StartupWorldTaskAction::cancel) {
            if (!close(next, id))
                return {Error::script_failed};
        } else if (action == StartupWorldTaskAction::request_abort) {
            if (std::any_of(next.task_abort_questions.begin(), next.task_abort_questions.end(),
                            [&](const auto &q) { return q.second == id; }) ||
                next.task_abort_answers.count(id))
                return {Error::invalid_page};
            const auto question = open(next, 1, state.active_task);
            if (!question)
                return {Error::script_failed};
            next.task_abort_questions[*question] = id;
            next.scripts.pages.back().paragraphs = {"要终止任务吗"};
        } else
            return {Error::invalid_page};
        next.scripts.executing_page.reset();
        state = std::move(next);
        return {};
    }
    if (raw == 33)
        return action == StartupWorldTaskAction::confirm
                   ? act_startup_world_runtime_deadline_page(state, id, selection)
                   : StartupWorldTaskPageResult{Error::invalid_page};
    if (raw == 22 || raw == 25 || raw == 26) {
        if (action == StartupWorldTaskAction::cancel) {
            if (!close(next, id))
                return {Error::script_failed};
        } else if (raw == 22 && action == StartupWorldTaskAction::confirm) {
            const auto list = next.task_page_lists.find(id);
            if (list == next.task_page_lists.end() || selection < 0 ||
                selection >= static_cast<int>(list->second.size()) ||
                !open(next, 23, list->second[selection]))
                return {Error::invalid_page};
        } else if (raw == 25 || raw == 26) {
            auto target = id;
            const auto c = project(next, *p);
            const auto consume = consumer(next, id, target);
            if (action == StartupWorldTaskAction::confirm &&
                (selection < 0 || selection > static_cast<int>(state.participants.size())))
                return {Error::invalid_page};
            if (action == StartupWorldTaskAction::add_member ||
                (action == StartupWorldTaskAction::confirm &&
                 selection == static_cast<int>(state.participants.size()))) {
                const auto r = ref::prepare_world_task_extra_candidates(c, consume);
                return commit(state, next, target, r);
            }
            if (action == StartupWorldTaskAction::inspect && selection >= 0 &&
                selection < static_cast<int>(state.participants.size())) {
                const auto detail = open(next, 60, {});
                if (!detail)
                    return {Error::script_failed};
                next.page_human_bindings[*detail] = state.participants.at(selection);
            } else if (raw == 26 && action == StartupWorldTaskAction::confirm) {
                if (!close(next, id))
                    return {Error::script_failed};
            } else {
                if (action == StartupWorldTaskAction::depart ||
                    (action == StartupWorldTaskAction::confirm && raw == 25)) {
                    if (raw == 26)
                        return {Error::invalid_page};
                    const auto r = ref::prepare_world_task_departure_page(c, consume);
                    return commit(state, next, target, r);
                }
                return {Error::invalid_page};
            }
        } else
            return {Error::invalid_page};
        next.scripts.executing_page.reset();
        state = std::move(next);
        return {};
    }
    auto target = id;
    if (action != StartupWorldTaskAction::confirm && action != StartupWorldTaskAction::cancel &&
        !(raw == 27 && action == StartupWorldTaskAction::hire))
        return {Error::invalid_page};
    auto c = project(next, *p);
    const auto consume = consumer(next, id, target);
    const ref::TaskCommandInput input{action == StartupWorldTaskAction::confirm ||
                                          action == StartupWorldTaskAction::hire,
                                      action == StartupWorldTaskAction::cancel, selection};
    ref::TaskCommandResult result;
    if (raw == 23 && p->task_identity)
        result = ref::prepare_world_task_offer(c, *p->task_identity, input, consume);
    else if (raw == 27)
        result = ref::prepare_world_task_hire(c, selection, input, consume);
    else if (raw == 28)
        result = ref::prepare_world_task_departure(c, input, consume);
    else
        return {Error::invalid_page};
    return commit(state, next, target, result);
}

std::optional<State> update_startup_world_runtime_task_control_page(const State &state,
                                                                    std::uint64_t id) {
    const auto *p = top(state);
    if (!p || p->id != id || p->legacy_page != 4 || state.scene.framework_paused)
        return {};
    auto next = state;
    next.scripts.executing_page = id;
    const auto answer = next.task_abort_answers.find(id);
    if (answer == next.task_abort_answers.end())
        return next;
    if (answer->second == 0) {
        if (!abort_startup_world_runtime_task_entities(next))
            return {};
        for (const int event : {80, 162}) {
            if (event == 162 && ref::world_script_seen(next.scripts, 162))
                continue;
            const auto result =
                ref::prepare_world_script(startup_world_runtime_catalog(),
                                          startup_world_runtime_scripts(next), {event, {}, {}});
            if (!result.candidate ||
                !write_startup_world_runtime_scripts(next, result.candidate->state))
                return {};
            if (event == 80) {
                auto &pending = next.scene.world.world.ai.pending_completion;
                if (pending < std::numeric_limits<int>::min() + 10)
                    return {};
                pending -= 10; // 请求完成量，不即时减人气或改金币。
            }
        }
        next.scripts.notices.push_back({26, -1, 80, "", "任务中止。街道人气<co=FF0E01>-10</co>"});
        if (!close(next, id))
            return {};
    }
    next.task_abort_answers.erase(id);
    return next;
}

std::optional<State> update_startup_world_runtime_task_page(const State &state, std::uint64_t id,
                                                            bool held) {
    const auto p = top(state);
    if (!p || p->id != id || state.scene.framework_paused)
        return {};
    auto next = state;
    next.scripts.executing_page = id;
    auto target = id;
    const auto c = project(next, *p);
    const auto consume = consumer(next, id, target);
    const auto r = p->legacy_page == 24 ? ref::prepare_world_task_recruitment(c, held, consume)
                                        : ref::prepare_world_task_departure(c, {}, consume);
    State committed;
    const auto result = commit(committed, next, target, r);
    return result.error == Error::none ? std::optional<State>(std::move(committed)) : std::nullopt;
}
} // namespace ark::simulation
