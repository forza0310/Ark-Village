// Read-only task projection and logical layouts; all cursors/counters belong to the Owner.
#include "world_tasks.hpp"
#include "../common/skin.hpp"
#include "world_crew_summary.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupWorldTaskAction;
const simulation::StartupWorldHuman &human(const State &s, int id) {
    const auto it = std::find_if(s.rules->humans.begin(), s.rules->humans.end(),
                                 [id](const auto &value) { return value.identity == id; });
    if (it == s.rules->humans.end())
        throw std::invalid_argument("Task page references an unknown human definition");
    return *it;
}
const simulation::StartupWorldTask &task_definition(const State &s, int id) {
    const auto it = std::find_if(s.rules->tasks.begin(), s.rules->tasks.end(),
                                 [id](const auto &value) { return value.factory.identity == id; });
    if (it == s.rules->tasks.end())
        throw std::invalid_argument("Task page references an unknown task definition");
    return *it;
}
std::string title(int raw) {
    switch (raw) {
    case 1:
        return "中止任务";
    case 4:
        return "任务管理";
    case 22:
    case 26:
        return "任务";
    case 23:
        return "参加任务";
    case 24:
        return "征集队员";
    case 25:
        return "参加者名单";
    case 27:
        return "参加委托";
    case 28:
        return "任务开始";
    case 31:
    case 32:
        return "成果";
    case 33:
        return "任务继续确认";
    default:
        throw std::invalid_argument("Unsupported task page presentation");
    }
}
void fitted(const Skin &skin, const std::string &text, Rectangle box, Color color = ink) {
    const float size = std::min(12.F, 12.F * box.width / std::max(1.F, skin.text.width(text)));
    skin.text.draw(text, box.x, box.y, color, size);
}
} // namespace

bool world_task_page(const Page &page) {
    if (page.kind != simulation::rules::WorldScriptPageKind::raw_page)
        return false;
    const int raw = page.legacy_page;
    return raw == 4 || raw == 22 || raw == 23 || raw == 24 || raw == 25 || raw == 26 || raw == 27 ||
           raw == 28 || raw == 31 || raw == 32 || raw == 33;
}
bool world_task_page(const State &s, const Page &page) {
    return world_task_page(page) ||
           (page.kind == simulation::rules::WorldScriptPageKind::raw_page &&
            (page.legacy_page == 1 && s.task_abort_questions.count(page.id)));
}
WorldTaskView world_task_view(const State &s, const Page &page) {
    if (!world_task_page(s, page) || !s.rules)
        throw std::invalid_argument("Task view requires a supported page and catalogue");
    WorldTaskView view;
    view.page = page.id;
    view.raw = page.legacy_page;
    view.interactive = page.lifecycle == 2 && !s.scene.framework_paused;
    view.title = title(view.raw);
    view.task = page.task_identity;
    if (page.task_definition)
        view.task_name = task_definition(s, *page.task_definition).name;
    const auto counter = s.page_counters.find(page.id);
    if (counter != s.page_counters.end())
        view.counter = counter->second;
    const auto phase = s.page_phases.find(page.id);
    view.phase = phase == s.page_phases.end() ? 0 : phase->second;
    view.animating = view.phase == 1;
    if (view.raw >= 22 && page.lifecycle != 2)
        return view;
    if (view.raw == 22 || view.raw == 23) {
        const auto selected = s.page_phases.find(page.id);
        if (selected == s.page_phases.end())
            return view;
        view.selected = selected->second;
        if (view.raw == 22) {
            const auto first = s.page_secondary_counters.find(page.id);
            if (first == s.page_secondary_counters.end())
                return view;
            view.first = first->second;
        }
    }
    if ((view.raw >= 25 && view.raw <= 28) || (view.raw >= 31 && view.raw <= 33)) {
        const auto ui = s.task_page_ui.find(page.id);
        if (ui == s.task_page_ui.end())
            return view;
        view.selected = ui->second.selection;
        view.first = ui->second.first;
        view.tab = ui->second.tab;
    }
    if (view.raw == 1) {
        view.abort_question = true;
    } else if (view.raw == 4) {
        view.details = {"任务实施中"};
    } else if (view.raw == 22) {
        const auto list = s.task_page_lists.find(page.id);
        if (list == s.task_page_lists.end())
            return view;
        for (const auto id : list->second)
            view.rows.push_back(
                {id, {}, task_definition(s, s.tasks.at(id).definition).name, {}, false});
    } else if (view.raw == 23) {
        if (!page.task_definition || !page.task_identity)
            return view;
        view.fee = task_definition(s, *page.task_definition).recruitment_fee;
    } else if (view.raw == 24) {
        const auto animation = s.task_recruitment_pages.find(page.id);
        if (animation == s.task_recruitment_pages.end() || counter == s.page_counters.end())
            return view;
        view.extent = animation->second.completion_tick;
        view.recruited_count = animation->second.displayed_count;
        for (const int id : animation->second.portraits)
            view.recruitment_names.push_back(human(s, id).name);
        if (!animation->second.portraits.empty()) {
            const auto &person = human(s, animation->second.portraits.front());
            const int profession =
                s.scene.world.world.ai.growth.at(person.identity).definition.current_profession;
            view.recruitment_actor =
                WorldTaskRecruitmentActor{person.identity, profession, person.sex,
                                          s.rules->jobs.at(profession).sprites.at(person.sex)};
        }
    } else if ((view.raw == 25 || view.raw == 26)) {
        if (!page.task_identity)
            return view;
        for (const int id : s.task_page_ui.at(page.id).member_rows) {
            if (id == -1)
                view.rows.push_back({{}, {}, "追加队员", {}, true});
            else
                view.rows.push_back({{}, id, human(s, id).name, {}, false});
        }
    } else if (view.raw == 27) {
        const auto extra = s.task_extra_pages.find(page.id);
        if (extra == s.task_extra_pages.end())
            return view;
        for (const int id : extra->second)
            view.rows.push_back(
                {{}, id, human(s, id).name, s.human_calendar.at(id).continuation_cost, false});
    } else if (view.raw == 28) {
        const auto prediction = s.task_page_predictions.find(page.id);
        if (prediction == s.task_page_predictions.end() || phase == s.page_phases.end())
            return view;
        view.prediction = prediction->second;
        view.extent = 96;
    } else if (view.raw == 31) {
        const auto summary = world_crew_summary_view(s, page.id);
        if (!summary.initialized)
            return view;
        for (const auto &h : summary.rows)
            view.rows.push_back({{}, h.definition, h.name, {}, false});
    } else if (view.raw == 32) {
        const auto summary = s.exploration_summaries.find(page.id);
        if (summary == s.exploration_summaries.end())
            return view;
        view.reward_ids = summary->second.rewards;
        view.rows.resize(view.reward_ids.size());
    } else if (view.raw == 33) {
        const auto grade = s.deadline_grades.find(page.id);
        if (!s.deadline_initialized.count(page.id) || grade == s.deadline_grades.end())
            return view;
        if (grade->second < 0 || grade->second > 4)
            throw std::invalid_argument("Task deadline grade is outside the published range");
        view.deadline_grade = grade->second;
        view.fee = page.legacy_f;
        view.extent = 50;
    }
    view.initialized = true;
    return view;
}
WorldTaskLayout world_task_layout(Extent extent, int raw) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Task page requires the supported logical viewport");
    const float width = std::min(310.F, extent.width - 16.F);
    const float height = std::min(250.F, extent.height - 68.F);
    WorldTaskLayout layout;
    layout.viewport = extent;
    if ((raw >= 22 && raw <= 28) || (raw >= 31 && raw <= 33)) {
        const bool large = raw == 23 || raw == 24 || raw == 28;
        const float w = large ? 222.F : 220.F, h = large ? 170.F : 168.F;
        layout.panel = {(extent.width - w) / 2, (extent.height - h) / 2, w, h};
        const auto o = world_list_skin_origin(layout.panel);
        layout.rows = {o.x + 23, o.y + 95, 191, 95};
        layout.body = layout.rows;
        layout.cancel = {layout.panel.x + w - 50, layout.panel.y + h + 4, 50, 18};
        layout.confirm = {layout.panel.x, layout.panel.y + h + 4, 50, 18};
        layout.inspect = {layout.panel.x + w / 2 - 25, layout.panel.y + h + 4, 50, 18};
        return layout;
    }
    layout.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &p = layout.panel;
    layout.body = {p.x + 12, p.y + 30, width - 24, height - 75};
    layout.rows = layout.body;
    layout.progress = {p.x + 12, p.y + height - 65, width - 24, 7};
    layout.cancel = {p.x + 10, p.y + height - 28, 58, 20};
    layout.confirm = {p.x + width - 68, p.y + height - 28, 58, 20};
    layout.inspect = {p.x + width / 2 - 29, p.y + height - 28, 58, 20};
    layout.continue_choice = {p.x + 12, p.y + height - 92, (width - 28) / 2, 22};
    layout.stop_choice = {p.x + width / 2 + 2, p.y + height - 92, (width - 28) / 2, 22};
    // Reserve a separate area for the source 18x24 actor. Names keep the normal text size
    // and may wrap; neither image nor name overlaps the count or the progress bar.
    layout.recruitment_name = {layout.body.x, layout.body.y + 48, layout.body.width - 44, 40};
    layout.recruitment_actor = {layout.body.x + layout.body.width - 40, layout.body.y + 40, 40, 40};
    return layout;
}
Rectangle world_task_menu_button(Extent extent) {
    // At minimum width the original popularity art begins at x103. This leaves a 40-unit
    // task command between pause and that art, without shrinking the source HUD assets.
    return {62, extent.height - 25.F, 40, 22};
}
int world_task_visible_rows(const WorldTaskLayout &layout) {
    return std::clamp(static_cast<int>(layout.rows.height / 19), 1, 5);
}
void draw_world_task(const WorldTaskView &v, const WorldTaskLayout &l, const Skin &skin,
                     const WorldTaskSelection &device, bool enabled, const std::string &feedback) {
    if (v.raw >= 22 && v.raw <= 24) {
        draw_world_task_selection(v, l, skin, enabled, feedback);
        return;
    }
    if (v.raw >= 25) {
        draw_world_task_followup(v, l, skin, enabled, feedback);
        return;
    }
    skin.window(l.panel, v.title);
    skin.content(l.body);
    const bool interactive = enabled && v.initialized;
    if (v.abort_question) {
        fitted(skin, "要终止任务吗？", {l.body.x, l.body.y + 24, l.body.width, 17});
        skin.button(l.continue_choice, "是", interactive);
        skin.button(l.stop_choice, "否", interactive);
        DrawRectangleLinesEx(device.prompt == 0 ? l.continue_choice : l.stop_choice, 1, blue);
    } else {
        for (std::size_t n = 0; n < v.details.size(); ++n)
            fitted(skin, v.details[n], {l.body.x, l.body.y + 22 + n * 20.F, l.body.width, 17});
        skin.button(l.cancel, "取消", interactive);
    }
    skin.button(l.confirm, v.abort_question ? "确定" : "中止", interactive);
    if (!feedback.empty())
        fitted(skin, feedback, {l.body.x, l.confirm.y - 16, l.body.width, 13}, MAROON);
}
bool world_task_related_confirmation(const State &s, const Page &page) {
    if (page.kind != simulation::rules::WorldScriptPageKind::raw_page)
        return true;
    if (page.legacy_page == 83 || page.legacy_page == 97 || world_task_page(s, page))
        return false;
    if (page.legacy_page == 59) {
        const auto counter = s.page_counters.find(page.id);
        return counter != s.page_counters.end() && counter->second >= 70;
    }
    if (page.legacy_page == 99 || page.legacy_page == 100)
        return s.task_display_initialized.count(page.id) != 0;
    return true;
}
} // namespace ark::desktop::ui
