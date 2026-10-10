// Frozen researche8f66d9 task input/data contract; responsive windows and list scrolling are
// desktop adaptations. All counters below are source-page counters, never render-frame timers.
#include "world_tasks.hpp"
#include "../common/skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using State = simulation::StartupWorldRuntimeState;
using Page = simulation::rules::WorldScriptPage;
using Action = simulation::StartupWorldTaskAction;
bool hit(std::optional<Vector2> point, Rectangle box) {
    return point && point->x >= box.x && point->y >= box.y && point->x < box.x + box.width &&
           point->y < box.y + box.height;
}
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
        return "冒险队伍";
    case 27:
        return "追加队员";
    case 28:
        return "准备出发";
    case 33:
        return "任务期限";
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
           raw == 28 || raw == 33;
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
    view.raw = page.legacy_page;
    view.title = title(view.raw);
    view.task = page.task_identity;
    if (page.task_definition)
        view.task_name = task_definition(s, *page.task_definition).name;
    const auto counter = s.page_counters.find(page.id);
    if (counter != s.page_counters.end())
        view.counter = counter->second;
    const auto phase = s.page_phases.find(page.id);
    view.animating = phase != s.page_phases.end() && phase->second == 1;
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
        for (const int id : s.participants)
            view.rows.push_back({{}, id, human(s, id).name, {}, false});
        view.rows.push_back({{}, {}, "追加队员", {}, true});
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
WorldTaskLayout world_task_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Task page requires the supported logical viewport");
    const float width = std::min(310.F, extent.width - 16.F);
    const float height = std::min(250.F, extent.height - 68.F);
    WorldTaskLayout layout;
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
    return std::clamp(static_cast<int>(layout.rows.height / 20), 1, 5);
}
std::optional<WorldTaskIntent> world_task_input(const WorldTaskView &view,
                                                const WorldTaskLayout &layout,
                                                WorldTaskSelection &selection,
                                                const WorldTaskInput &input, bool blocked) {
    if (blocked || !view.initialized)
        return {};
    if (view.abort_question) {
        selection.prompt = std::clamp(selection.prompt, 0, 1);
        if (input.left || input.right)
            selection.prompt = 1 - selection.prompt;
        if (hit(input.click, layout.continue_choice))
            selection.prompt = 0;
        if (hit(input.click, layout.stop_choice))
            selection.prompt = 1;
        if (input.enter || hit(input.click, layout.confirm))
            return WorldTaskIntent{Action::confirm, selection.prompt};
        return {}; // The source question has no Back/cancel action.
    }
    const int count = static_cast<int>(view.rows.size());
    const int visible = world_task_visible_rows(layout);
    if (count) {
        selection.selected = std::clamp(selection.selected, 0, count - 1);
        selection.first_row =
            std::clamp(selection.first_row + input.wheel_rows, 0, std::max(0, count - visible));
        if (input.up)
            selection.selected = (selection.selected + count - 1) % count;
        if (input.down)
            selection.selected = (selection.selected + 1) % count;
        if (input.up || input.down)
            selection.first_row =
                std::clamp(selection.first_row, std::max(0, selection.selected - visible + 1),
                           std::min(selection.selected, std::max(0, count - visible)));
        for (int row = 0; row < visible && selection.first_row + row < count; ++row)
            if (hit(input.click, {layout.rows.x, layout.rows.y + row * 20, layout.rows.width, 20}))
                selection.selected = selection.first_row + row;
    }
    if (view.raw == 24)
        return {}; // Recruitment consumes held edges through a separate page-bound command.
    if (view.raw == 33 && !view.animating) {
        selection.selected = std::clamp(selection.selected, 0, 1);
        if (input.left || input.right)
            selection.selected = 1 - selection.selected;
        if (hit(input.click, layout.continue_choice))
            selection.selected = 0;
        if (hit(input.click, layout.stop_choice))
            selection.selected = 1;
    }
    // Source33 has no Back branch. Source28 cannot cancel after departure animation begins.
    if (view.raw != 33 && !(view.raw == 28 && view.animating) &&
        (input.escape || hit(input.click, layout.cancel)))
        return WorldTaskIntent{Action::cancel, 0};
    if ((view.raw == 25 || view.raw == 26) && count &&
        !view.rows.at(selection.selected).add_member && hit(input.click, layout.inspect))
        return WorldTaskIntent{Action::inspect, selection.selected};
    if (!input.enter && !hit(input.click, layout.confirm))
        return {};
    if ((view.raw == 22 || view.raw == 25 || view.raw == 26 || view.raw == 27) && count == 0)
        return {};
    if (view.raw == 4)
        return WorldTaskIntent{Action::request_abort, 0};
    if (view.raw == 25 || view.raw == 26)
        return WorldTaskIntent{view.rows.at(selection.selected).add_member ? Action::add_member
                               : view.raw == 25                            ? Action::depart
                                                                           : Action::confirm,
                               selection.selected};
    if (view.raw == 27)
        return WorldTaskIntent{Action::hire, *view.rows.at(selection.selected).human};
    return WorldTaskIntent{Action::confirm, view.raw == 28 ? 0 : selection.selected};
}
void draw_world_task(const WorldTaskView &view, const WorldTaskLayout &layout, const Skin &skin,
                     const WorldTaskSelection &selection, bool enabled,
                     const std::string &feedback) {
    skin.window(layout.panel, view.title);
    skin.content(
        {layout.body.x - 5, layout.body.y - 5, layout.body.width + 10, layout.body.height + 10});
    const auto &body = layout.body;
    const bool interactive = enabled && view.initialized;
    if (!view.rows.empty()) {
        const int visible = world_task_visible_rows(layout);
        const int first = std::clamp(selection.first_row, 0,
                                     std::max(0, static_cast<int>(view.rows.size()) - visible));
        for (int row = first; row < static_cast<int>(view.rows.size()) && row < first + visible;
             ++row) {
            Rectangle box{body.x, body.y + (row - first) * 20, body.width, 19};
            if (row == selection.selected)
                DrawRectangleRec(box, Color{219, 232, 204, 255});
            const auto &entry = view.rows[row];
            fitted(skin, entry.name,
                   {box.x + 3, box.y + 3, box.width - (entry.fee ? 70.F : 6.F), 14});
            if (entry.fee)
                skin.right(std::to_string(*entry.fee) + "G", box.x + box.width - 3, box.y + 3);
        }
        if (static_cast<int>(view.rows.size()) > visible)
            skin.text.draw(
                std::to_string(first + 1) + "-" +
                    std::to_string(std::min(first + visible, static_cast<int>(view.rows.size()))) +
                    "/" + std::to_string(view.rows.size()),
                body.x, layout.panel.y + layout.panel.height - 44, ink, 9);
    } else if (view.initialized) {
        fitted(skin, view.task_name, {body.x, body.y, body.width, 17});
        for (std::size_t n = 0; n < view.details.size(); ++n)
            fitted(skin, view.details[n], {body.x, body.y + 22 + n * 20.F, body.width, 17});
        if (view.abort_question)
            fitted(skin, "要终止任务吗？", {body.x, body.y + 24, body.width, 17});
        if (view.fee)
            skin.text.draw(std::string(view.raw == 33 ? "延长费用 " : "征集费 ") +
                               std::to_string(*view.fee) + "G",
                           body.x, body.y + 24);
        if (view.prediction)
            skin.text.draw("队伍评价 " + std::to_string(*view.prediction), body.x, body.y + 24);
        if (view.deadline_grade) {
            static constexpr const char *grades[]{"再加把劲!!", "再加加油!!", "需要补充战力!",
                                                  "重新来过比较好", "应该撤退…"};
            fitted(skin, grades[*view.deadline_grade], {body.x, body.y + 45, body.width, 17});
        }
        if (view.raw == 24) {
            skin.text.draw(std::to_string(view.recruited_count) + "人", body.x, body.y + 24);
            if (!view.recruitment_names.empty())
                skin.text.paragraph(view.recruitment_names.front(), layout.recruitment_name.x,
                                    layout.recruitment_name.y, layout.recruitment_name.width);
            if (view.recruitment_actor) {
                // research33ee056 startup_view: Y.front(), current profession/sex, actor0/frame0.
                // The source page ticks switch Y; rendering does not advance that clock.
                // This restores the delivered prototype's character switch only. Exact X-entry
                // movement/expressions for the full original recruitment animation remain
                // unshipped.
                skin.sprites.actor(
                    false, 0, view.recruitment_actor->image, 0,
                    {layout.recruitment_actor.x + 20, layout.recruitment_actor.y + 36});
            }
        }
    }
    if (view.initialized && (view.raw == 24 || view.animating)) {
        const float ratio =
            view.extent > 0 ? std::clamp(view.counter / static_cast<float>(view.extent), 0.F, 1.F)
                            : 0.F;
        DrawRectangleRec(layout.progress, GRAY);
        auto fill = layout.progress;
        fill.width *= ratio;
        DrawRectangleRec(fill, Color{97, 174, 68, 255});
    }
    if ((view.raw == 33 && !view.animating) || view.abort_question) {
        skin.button(layout.continue_choice, view.abort_question ? "是" : "继续", interactive);
        skin.button(layout.stop_choice, view.abort_question ? "否" : "中止", interactive);
        DrawRectangleLinesEx((view.abort_question ? selection.prompt : selection.selected) == 0
                                 ? layout.continue_choice
                                 : layout.stop_choice,
                             1, blue);
    }
    if (!view.abort_question && view.raw != 24 && view.raw != 33 &&
        !(view.raw == 28 && view.animating))
        skin.button(layout.cancel, "取消", interactive);
    std::string confirm = view.raw == 24 ? "加速" : "确定";
    if ((view.raw == 25 || view.raw == 26) && !view.rows.empty())
        confirm =
            view.rows.at(std::clamp(selection.selected, 0, static_cast<int>(view.rows.size()) - 1))
                    .add_member
                ? "追加"
            : view.raw == 25 ? "出发"
                             : "确定";
    if (view.raw == 4)
        confirm = "中止";
    skin.button(layout.confirm, confirm, interactive);
    if ((view.raw == 25 || view.raw == 26) && !view.rows.empty())
        skin.button(layout.inspect, "详情",
                    interactive && !view.rows
                                        .at(std::clamp(selection.selected, 0,
                                                       static_cast<int>(view.rows.size()) - 1))
                                        .add_member);
    if (!feedback.empty())
        fitted(skin, feedback, {body.x, layout.panel.y + layout.panel.height - 44, body.width, 13},
               MAROON);
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
void draw_world_task_monster(const State &s, const Page &page, Rectangle body, const Skin &skin) {
    if ((page.legacy_page != 99 && page.legacy_page != 100) || !page.monster_definition ||
        !s.task_display_initialized.count(page.id))
        return;
    const auto &monster = s.scene.world.world.ai.monster_growth.at(*page.monster_definition);
    skin.sprites.actor(true, monster.body * 4 + 1, monster.body * 30 + monster.sprite_variant, 0,
                       {body.x + body.width / 2, body.y - 13});
}
} // namespace ark::desktop::ui
