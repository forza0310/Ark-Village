#include "world_task_inspection.hpp"
#include "ark/simulation/tasks/startup_world_runtime_tasks.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Kind = simulation::rules::WorldScriptPageKind;
using Action = simulation::StartupWorldTaskAction;
const Page *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
bool affordable(const State &s, std::uint64_t id) {
    const int definition = s.tasks.at(id).definition;
    const auto d =
        std::find_if(s.rules->tasks.begin(), s.rules->tasks.end(), [definition](const auto &task) {
            return task.factory.identity == definition;
        });
    if (d == s.rules->tasks.end())
        throw std::runtime_error("Task inspection lost a real task definition");
    return d->recruitment_fee <= s.scene.world.world.ai.accounting.funds();
}
void require(simulation::StartupWorldRuntimeError error) {
    if (error != simulation::StartupWorldRuntimeError::none)
        throw std::runtime_error("Task inspection player command failed: " +
                                 std::to_string(static_cast<int>(error)));
}
} // namespace
bool world_task_inspection_mode(const std::string &mode) {
    return mode == "world-task-team" || mode == "world-task-result" ||
           mode == "world-task-recruitment" || mode == "world-task-victory" ||
           mode == "world-task-popularity";
}
bool world_task_inspection_ready(const State &s, const std::string &mode,
                                 const WorldTaskInspection &inspection) {
    const auto *page = top(s);
    if (!page || page->kind != Kind::raw_page || !inspection.accepted_task ||
        page->task_identity != inspection.accepted_task)
        return false;
    if (mode == "world-task-recruitment") {
        const auto recruitment = s.task_recruitment_pages.find(page->id);
        return page->legacy_page == 24 && s.page_counters.count(page->id) &&
               recruitment != s.task_recruitment_pages.end() &&
               !recruitment->second.portraits.empty();
    }
    if (mode == "world-task-victory" || mode == "world-task-popularity") {
        const auto counter = s.page_counters.find(page->id);
        const auto phase = s.page_phases.find(page->id);
        return page->legacy_page == 30 && s.exploration_summaries.count(page->id) &&
               counter != s.page_counters.end() && counter->second >= 40 &&
               phase != s.page_phases.end() &&
               phase->second == (mode == "world-task-victory" ? 0 : 1) &&
               inspection.departed_task == inspection.accepted_task;
    }
    return (mode == "world-task-team" && page->legacy_page == 25 && page->task_identity &&
            !s.participants.empty()) ||
           (mode == "world-task-result" && page->legacy_page == 31 && page->task_identity &&
            s.crew_summaries.count(page->id) &&
            inspection.departed_task == inspection.accepted_task);
}
bool apply_world_task_inspection_input(State &s, WorldTaskInspection &inspection) {
    if (inspection.accepted_task && s.active_task == inspection.accepted_task)
        inspection.departed_task = s.active_task;
    const auto *current = top(s);
    if (!current)
        return false;
    // Copy before command: a real page consumer may reallocate/remove its owning vector.
    const auto page = *current;
    if (page.kind == Kind::scene) {
        if (!s.active_task && std::any_of(s.task_order.begin(), s.task_order.end(),
                                          [&](auto id) { return affordable(s, id); }))
            require(simulation::open_startup_world_runtime_task_menu(s));
        return true;
    }
    if (page.kind != Kind::raw_page)
        return false;
    // The source initializes result rows on the next framework call. Generic acknowledgement
    // can initialize and close raw31 in one command, bypassing the observable result entirely.
    if (page.legacy_page == 31 && page.task_identity == inspection.accepted_task &&
        inspection.departed_task == inspection.accepted_task && inspection.accepted_task)
        return true;
    if (page.legacy_page == 22) {
        const auto &list = s.task_page_lists.at(page.id);
        const auto chosen =
            std::find_if(list.begin(), list.end(), [&](auto id) { return affordable(s, id); });
        const auto result = simulation::act_startup_world_runtime_task_page(
            s, page.id, chosen == list.end() ? Action::cancel : Action::confirm,
            chosen == list.end() ? 0 : static_cast<int>(chosen - list.begin()));
        require(result.error);
        return true;
    }
    if (page.legacy_page == 23 || page.legacy_page == 25 || page.legacy_page == 28 ||
        page.legacy_page == 33) {
        const auto phase = s.page_phases.find(page.id);
        if ((page.legacy_page == 28 || page.legacy_page == 33) && phase != s.page_phases.end() &&
            phase->second == 1)
            return true; // Let source animation ticks run; no per-frame fast-forward presses.
        if (page.legacy_page == 33 && !s.deadline_initialized.count(page.id))
            return true;
        const int choice =
            page.legacy_page == 33 && s.scene.world.world.ai.accounting.funds() < page.legacy_f ? 1
                                                                                                : 0;
        const auto result = simulation::act_startup_world_runtime_task_page(
            s, page.id, page.legacy_page == 25 ? Action::depart : Action::confirm, choice);
        require(result.error);
        if (result.denial != simulation::rules::TaskCommandDenial::none)
            throw std::runtime_error("Task inspection command was denied by actual world state");
        if (page.legacy_page == 23 && result.accepted) {
            inspection.accepted_task = page.task_identity;
            inspection.departed_task.reset();
        }
        return true;
    }
    if (page.legacy_page == 26) {
        require(simulation::act_startup_world_runtime_task_page(s, page.id, Action::cancel).error);
        return true;
    }
    if (page.legacy_page == 24)
        return true; // Natural recruitment, no direct participants or held-input injection.
    if (page.legacy_page == 83) {
        require(simulation::cancel_startup_world_runtime_page(s, page.id));
        return true;
    }
    if (page.legacy_page == 87) {
        const auto pending = s.award_termination_pending.find(page.id);
        if (pending != s.award_termination_pending.end())
            require(simulation::act_startup_world_runtime_award_page(
                s, page.id,
                pending->second ? simulation::rules::WorldAwardAction::confirm_termination
                                : simulation::rules::WorldAwardAction::request_termination));
        return true;
    }
    return false;
}
} // namespace ark::desktop
