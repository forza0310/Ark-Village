// Deliberately sparse definition/instance IDs catch accidental row-index ownership. These are
// presentation fixtures, not evidence of natural recruitment or successful task completion.
#include "support/checks.hpp"
#include "ui/world_award.hpp"
#include "ui/world_tasks.hpp"
#include <iostream>

namespace {
Vector2 middle(Rectangle r) { return {r.x + r.width / 2, r.y + r.height / 2}; }
bool contains(Rectangle outer, Rectangle inner) {
    return inner.width > 0 && inner.height > 0 && inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
} // namespace
namespace ark::test {
void world_tasks() {
    Checks check{"world_tasks"};
    namespace ui = ark::desktop::ui;
    namespace sim = ark::simulation;
    namespace rules = sim::rules;
    using Action = sim::StartupWorldTaskAction;
    sim::StartupWorldRules catalogue;
    sim::StartupWorldRuntimeState state;
    state.rules = &catalogue;
    catalogue.jobs.resize(3);
    catalogue.jobs[1].sprites = {11, 12};
    catalogue.jobs[2].sprites = {21, 22};
    for (const int id : {42, 7, 19, 5, 66, 14, 3}) {
        sim::StartupWorldHuman person;
        person.identity = id;
        person.name = "human-" + std::to_string(id);
        catalogue.humans.push_back(person);
        state.human_calendar[id].continuation_cost = id * 10;
        state.scene.world.world.ai.growth[id].definition.current_profession = 1;
    }
    for (const int id : {8, 3}) {
        sim::StartupWorldTask task;
        task.factory.identity = id;
        task.name = "task-" + std::to_string(id);
        task.recruitment_fee = id * 100;
        catalogue.tasks.push_back(task);
    }
    state.tasks[900].definition = 3;
    state.tasks[120].definition = 8;
    state.task_order = {120, 900}; // The opened page snapshot intentionally has another order.
    rules::WorldScriptPage page;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.id = 15;
    page.legacy_page = 22;
    check(!ui::world_task_view(state, page).initialized && state.task_page_lists.empty(),
          "Read-only UI must not initialize a task list");
    state.task_page_lists[page.id] = {900, 120, 900};
    auto view = ui::world_task_view(state, page);
    check(view.initialized && view.rows.size() == 3 && view.rows[0].task == 900 &&
              view.rows[0].name == "task-3" && view.rows[1].task == 120 && view.rows[2].task == 900,
          "Menu rows retain snapshot instance identity, source order and duplicates");
    const auto layout = ui::world_task_layout({540, 360});
    ui::WorldTaskSelection selection;
    ui::WorldTaskInput input;
    input.down = true;
    (void)ui::world_task_input(view, layout, selection, input, false);
    input = {};
    input.enter = true;
    auto intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::confirm && intent->selection == 1,
          "Menu sends selected snapshot row, not a task definition or instance as index");
    check(!ui::world_task_input(view, layout, selection, input, true),
          "Paused, failed or pending command blocks another player action");

    page.legacy_page = 23;
    page.task_identity = 900;
    page.task_definition = 3;
    view = ui::world_task_view(state, page);
    check(view.fee == 300 && view.task == 900 && view.task_name == "task-3",
          "Offer binds real task identity and original recruitment fee");
    input = {};
    input.escape = true;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::cancel,
          "Offer Back remains cancellation, not acceptance");

    page.legacy_page = 24;
    check(!ui::world_task_view(state, page).initialized,
          "Recruitment page cannot invent counters before its source initializer");
    state.page_counters[page.id] = 61;
    auto &animation = state.task_recruitment_pages[page.id];
    animation.completion_tick = 200;
    animation.displayed_count = 1;
    animation.portraits = {7, 42};
    animation.portrait_timer = 12;
    const auto recruitment_draws = state.scene.random.draws();
    const auto recruitment_cash = state.scene.world.world.ai.accounting.funds();
    view = ui::world_task_view(state, page);
    check(view.initialized && view.counter == 61 && view.extent == 200 &&
              view.recruited_count == 1 && view.recruitment_names[0] == "human-7",
          "Recruitment reads displayed count separately from actual Y queue size");
    check(view.recruitment_actor && view.recruitment_actor->human == 7 &&
              view.recruitment_actor->profession == 1 && view.recruitment_actor->sex == 0 &&
              view.recruitment_actor->image == 11,
          "Recruitment selects sparse Y-front identity and live profession, not catalogue row or "
          "initial profession");
    state.scene.world.world.ai.growth.at(7).definition.current_profession = 2;
    catalogue.humans.at(1).sex = 1;
    view = ui::world_task_view(state, page);
    check(view.recruitment_actor && view.recruitment_actor->human == 7 &&
              view.recruitment_actor->profession == 2 && view.recruitment_actor->sex == 1 &&
              view.recruitment_actor->image == 22,
          "Recruitment resolves current profession and sex afresh from the owner and catalogue");
    animation.portraits = {42, 7};
    view = ui::world_task_view(state, page);
    check(view.recruitment_actor && view.recruitment_actor->human == 42 &&
              view.recruitment_actor->image == 11 && view.recruitment_names.front() == "human-42",
          "A source Y-front change switches the displayed person without a render-time timer");
    animation.portraits.clear();
    view = ui::world_task_view(state, page);
    check(view.initialized && !view.recruitment_actor && view.recruitment_names.empty() &&
              view.recruited_count == 1,
          "An empty source Y shows no person even when displayed count is nonzero");
    animation.portraits = {7, 42};
    view = ui::world_task_view(state, page);
    (void)ui::world_task_view(state, page);
    check(state.scene.random.draws() == recruitment_draws &&
              state.scene.world.world.ai.accounting.funds() == recruitment_cash &&
              state.page_counters.at(page.id) == 61 && animation.portrait_timer == 12 &&
              animation.portraits == std::vector<int>({7, 42}) && animation.displayed_count == 1 &&
              animation.completion_tick == 200 && state.participants.empty() &&
              state.scene.world.world.ai.growth.size() == 7 &&
              state.scene.world.world.ai.growth.at(7).definition.current_profession == 2 &&
              catalogue.humans.at(1).sex == 1,
          "Repeated recruitment projection leaves source queue, timing, identity, cash and random "
          "untouched");
    input = {};
    input.enter = true;
    check(!ui::world_task_input(view, layout, selection, input, false),
          "Recruitment has no confirm edge; held acceleration uses separate FIFO input");

    page.legacy_page = 25;
    state.participants = {7, 42, 7};
    view = ui::world_task_view(state, page);
    check(view.rows.size() == 4 && view.rows[0].human == 7 && view.rows[2].human == 7 &&
              view.rows.back().add_member && !view.rows.back().human,
          "Team preserves participant definitions and has a separate non-person add row");
    selection = {3, 0};
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::add_member,
          "Add row opens extra recruitment, not departure");
    selection.selected = 1;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::depart,
          "Participant selection opens the source departure prompt");

    page.legacy_page = 27;
    state.task_extra_pages[page.id] = {66, 14, 3, 42, 19, 5, 7};
    view = ui::world_task_view(state, page);
    check(view.rows[0].human == 66 && view.rows[0].fee == 660,
          "Extra row uses actual definition and initialized extra fee");
    selection = {};
    input = {};
    input.up = true;
    (void)ui::world_task_input(view, layout, selection, input, false);
    check(selection.selected == 6 && selection.first_row == 2,
          "Keyboard wrap scrolls final actual source row into view");
    input = {};
    input.enter = true;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::hire && intent->selection == 7,
          "Hire sends human definition identity, never visible row number");
    input = {};
    input.wheel_rows = -2;
    (void)ui::world_task_input(view, layout, selection, input, false);
    check(selection.first_row == 0 && selection.selected == 6,
          "Mouse scroll does not silently alter selected source identity");
    input = {};
    input.click = Vector2{layout.rows.x + 2, layout.rows.y + 21};
    (void)ui::world_task_input(view, layout, selection, input, false);
    check(selection.selected == 1, "Mouse picking shares the source-row layout");

    page.legacy_page = 28;
    state.task_page_predictions[page.id] = 4;
    state.page_phases[page.id] = 0;
    view = ui::world_task_view(state, page);
    check(view.prediction == 4 && !view.animating && view.extent == 96,
          "Departure rating is display data; source animation extent is96");
    input = {};
    input.enter = true;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::confirm && intent->selection == 0,
          "Departure confirmation cannot inherit stale list selection as cancel choice");
    state.page_phases[page.id] = 1;
    view = ui::world_task_view(state, page);
    input = {};
    input.escape = true;
    check(!ui::world_task_input(view, layout, selection, input, false),
          "Source departure animation has no cancellation branch");

    page.legacy_page = 33;
    page.legacy_f = 500;
    state.deadline_initialized.insert(page.id);
    state.deadline_grades[page.id] = 2;
    state.page_phases[page.id] = 0;
    view = ui::world_task_view(state, page);
    check(view.fee == 500 && view.deadline_grade == 2 && view.extent == 50,
          "Deadline reads actual extension fee, initialized grade and50-count animation");
    check(!ui::world_task_input(view, layout, selection, input, false),
          "Deadline Esc must not invent a Back branch");
    input = {};
    input.click = middle(layout.stop_choice);
    (void)ui::world_task_input(view, layout, selection, input, false);
    input = {};
    input.enter = true;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->selection == 1 && intent->action == Action::confirm,
          "Stop uses deadline choice1 through the same real confirmation consumer");
    state.page_phases[page.id] = 1;
    view = ui::world_task_view(state, page);
    input = {};
    input.left = true;
    (void)ui::world_task_input(view, layout, selection, input, false);
    check(selection.selected == 1, "Deadline choice cannot change after source animation starts");

    const auto draws = state.scene.random.draws();
    const auto cash = state.scene.world.world.ai.accounting.funds();
    (void)ui::world_task_view(state, page);
    check(draws == state.scene.random.draws() &&
              cash == state.scene.world.world.ai.accounting.funds() &&
              state.page_counters.at(page.id) == 61 && state.participants.size() == 3 &&
              !state.active_task,
          "View/input never pays, activates tasks, recruits or advances a source counter");
    page.legacy_page = 59;
    state.page_counters[page.id] = 69;
    check(!ui::world_task_related_confirmation(state, page), "Unlock59 waits through69");
    state.page_counters[page.id] = 70;
    check(ui::world_task_related_confirmation(state, page), "Unlock59 accepts at70");
    for (const int raw : {99, 100}) {
        page.legacy_page = raw;
        state.task_display_initialized.clear();
        check(!ui::world_task_related_confirmation(state, page),
              "Task display initialization cannot be fabricated by confirmation UI");
        state.task_display_initialized.insert(page.id);
        check(ui::world_task_related_confirmation(state, page),
              "Initialized task display uses its source confirmation consumer");
    }
    page.legacy_page = 26;
    view = ui::world_task_view(state, page);
    input = {};
    input.escape = true;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::cancel && !ui::world_page_regular_confirmation(page),
          "Source26 returns through the task consumer, never generic acknowledgement");
    page.legacy_page = 83;
    check(!ui::world_task_related_confirmation(state, page) &&
              !ui::world_page_regular_confirmation(page),
          "Shop83 exposes only cancellation");
    page.legacy_page = 97;
    check(ui::world_page_automatic(page) && !ui::world_page_regular_confirmation(page),
          "Unlock97 has an automatic consumer and no fake confirmation modal");
    for (const auto extent : {ark::desktop::Extent{240, 256}, ark::desktop::Extent{540, 360}}) {
        const auto boxes = ui::world_task_layout(extent);
        const Rectangle scene{0, 24, static_cast<float>(extent.width), extent.height - 53.F};
        check(contains(scene, boxes.panel) && contains(boxes.panel, boxes.rows) &&
                  contains(boxes.panel, boxes.confirm) && contains(boxes.panel, boxes.cancel) &&
                  contains(boxes.panel, boxes.continue_choice) &&
                  contains(boxes.panel, boxes.stop_choice) &&
                  ui::world_task_visible_rows(boxes) == 5,
              "Five task rows and controls fit both minimum and normal desktop layouts");
        check(contains(boxes.body, boxes.recruitment_name) &&
                  contains(boxes.body, boxes.recruitment_actor) &&
                  boxes.recruitment_name.x + boxes.recruitment_name.width <=
                      boxes.recruitment_actor.x &&
                  boxes.recruitment_actor.y >= boxes.body.y + 38 &&
                  boxes.recruitment_actor.y + boxes.recruitment_actor.height < boxes.progress.y,
              "Recruitment character has separate space from the normal-size name, count and "
              "progress");
        const auto menu = ui::world_task_menu_button(extent);
        check(menu.x >= 60 && menu.x + menu.width < extent.width - 137,
              "Task entry leaves source popularity and both playback buttons unobscured");
    }
    std::cout << "PASS task UI identity, source-page eligibility, layout and input contracts\n";
}
} // namespace ark::test
