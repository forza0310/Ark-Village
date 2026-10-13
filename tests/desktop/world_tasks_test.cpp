// Deliberately sparse definition/instance IDs catch accidental row-index ownership. These are
// presentation fixtures, not evidence of natural recruitment or successful task completion.
#include "../../src/desktop/ui/common/world_reports.hpp"
#include "../../src/desktop/ui/tasks/world_task_tracking.hpp"
#include "../../src/desktop/ui/tasks/world_tasks.hpp"
#include "../../src/desktop/ui/village/world_award.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
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
    page.lifecycle = 2;
    check(!ui::world_task_view(state, page).initialized && state.task_page_lists.empty(),
          "Read-only UI must not initialize a task list");
    state.task_page_lists[page.id] = {900, 120, 900};
    state.page_phases[page.id] = 0;
    state.page_secondary_counters[page.id] = 0;
    auto view = ui::world_task_view(state, page);
    check(view.initialized && view.rows.size() == 3 && view.rows[0].task == 900 &&
              view.rows[0].name == "task-3" && view.rows[1].task == 120 && view.rows[2].task == 900,
          "Menu rows retain snapshot instance identity, source order and duplicates");
    const auto layout = ui::world_task_layout({540, 360});
    ui::WorldTaskSelection selection;
    ui::WorldTaskInput input;
    input.down = true;
    const auto move = ui::world_task_input(view, layout, selection, input, false);
    check(move && move->input && move->input->down && selection.selected == 0,
          "22 forwards direction without keeping a second desktop cursor");
    state.page_phases.at(page.id) = 1; // Published Owner acknowledgement, not UI mutation.
    view = ui::world_task_view(state, page);
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

    {
        auto owner = initial_world();
        const auto created =
            rules::prepare_world_task_creation(sim::startup_world_runtime_factory(owner), 0);
        check(created.candidate &&
                  sim::write_startup_world_runtime_factory(owner, created.candidate->state),
              "Tracking UI creates its task with the actual factory");
        check(sim::open_startup_world_runtime_task_menu(owner) ==
                  sim::StartupWorldRuntimeError::none,
              "Tracking UI enters the source22 catalogue");
        for (int n = 0; n < 6 && owner.scripts.pages.back().lifecycle != 2; ++n) {
            auto next = sim::prepare_startup_world_runtime(owner);
            check(next.candidate.has_value(), "22 real Init ready");
            owner = std::move(*next.candidate);
        }
        const auto id = owner.scripts.pages.back().id;
        const auto list = ui::world_task_view(owner, owner.scripts.pages.back());
        ui::WorldTaskPaintedControls painted;
        painted.geometry.page = id;
        painted.geometry.raw = 22;
        painted.geometry.selection = list.selected;
        painted.geometry.first = list.first;
        painted.geometry.count = int(list.rows.size());
        painted.geometry.extent = {240, 256};
        painted.geometry.rows = {{0, 0, {25, 95, 180, 16}}};
        for (const auto &r : list.rows)
            painted.tasks.push_back(*r.task);
        ui::WorldTaskSelection device;
        ui::WorldTaskInput click;
        click.click = Vector2{30, 100};
        const auto picked = ui::world_task_input(list, ui::world_task_layout({240, 256}, 22),
                                                 device, click, false, painted);
        check(
            picked && picked->input && picked->input->selection == 0 && picked->input->confirm,
            "22 row release keeps64-bit source identities and submits selection/confirm together");
        painted.tasks.front() += 1;
        check(!ui::world_task_input(list, ui::world_task_layout({240, 256}, 22), device, click,
                                    false, painted),
              "A replaced frozen task identity rejects old geometry even at the same row");
        sim::StartupTaskPageInput track;
        track.track = true;
        check(sim::input_startup_world_runtime_task_page(owner, id, track).error ==
                  sim::StartupWorldRuntimeError::none,
              "22 enters actual tracking scene5");
        const auto before = owner;
        const auto tracking = ui::world_task_tracking_view(owner);
        check(tracking && !tracking->ready && tracking->name == list.rows.front().name,
              "Tracking UI reads the Owner and waits during source camera movement");
        click = {};
        click.enter = true;
        check(!ui::world_task_tracking_input(*tracking, ui::world_task_tracking_layout({240, 256}),
                                             click, false) &&
                  owner.camera == before.camera &&
                  owner.previous_camera == before.previous_camera &&
                  owner.scene.random.draws() == before.scene.random.draws(),
              "Scene5 input/query cannot start recruitment or move the source camera");
        const auto site = *owner.tasks.at(tracking->tasks.front()).site;
        owner.camera = {float((site.x + site.y) * 30 + 30), float((site.y - site.x) * 15 + 10)};
        const auto arrived = sim::update_startup_world_runtime_task_tracking_scene(owner);
        check(arrived && arrived->scene.scene_state == 4, "Source camera arrival enters scene4");
        auto live = *ui::world_task_tracking_view(*arrived);
        const auto info_layout = ui::world_task_tracking_layout({240, 256});
        ui::WorldTaskTrackingPaintedControls drawn;
        drawn.geometry.page = live.scene;
        drawn.geometry.raw = live.raw;
        drawn.geometry.selection = live.selected;
        drawn.geometry.extent = {240, 256};
        drawn.geometry.confirm = {85, 170, 70, 56};
        drawn.tasks = live.tasks;
        drawn.available_count = live.available_count;
        click = {};
        click.click = middle(drawn.geometry.confirm);
        check(ui::world_task_tracking_input(live, info_layout, click, false, drawn)->confirm,
              "Source information touch confirms from last-painted geometry");
        for (int invalidation = 0; invalidation < 4; ++invalidation) {
            auto stale = drawn;
            if (invalidation == 0)
                ++stale.geometry.page;
            if (invalidation == 1)
                ++stale.geometry.selection;
            if (invalidation == 2)
                ++stale.tasks.front();
            if (invalidation == 3)
                ++stale.geometry.extent.width;
            check(!ui::world_task_tracking_input(live, info_layout, click, false, stale),
                  "Information rejects stale page, cursor, task binding or viewport");
        }
        check(!ui::world_task_tracking_input(live, info_layout, click, true, drawn),
              "Pending input rejects a valid painted information touch");
        auto changed_directory = drawn;
        ++changed_directory.available_count;
        check(!ui::world_task_tracking_input(live, info_layout, click, false, changed_directory),
              "Information arrow availability follows the actual current task directory");
        live.raw = 29;
        click = {};
        click.left = click.right = click.track = true;
        check(!ui::world_task_tracking_input(live, info_layout, click, false),
              "Raw29 does not borrow tracking arrows or create an unproved entrance");
    }
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
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 26;
    view = ui::world_task_view(state, page);
    selection = {};
    input = {};
    input.click = middle(layout.inspect);
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(view.rows.size() == 4 && intent && intent->action == Action::inspect &&
              intent->selection == 0,
          "Active team exposes member inspection by participant index");
    input = {};
    input.enter = true;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::confirm,
          "Active team confirm closes its page instead of starting a second departure");
    selection.selected = 3;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::add_member,
          "Active team retains real extra recruitment action");
    page.legacy_page = 60;
    state.page_human_bindings[page.id] = 42;
    check(!ui::world_task_page(state, page) && !ui::world_page_regular_confirmation(page),
          "Task member raw60 delegates to shared human management, never the old task Back page");
    // Current shared fields and all four raw60 tabs are now owned by world_human tests.
    page.legacy_page = 98;
    check(ui::world_page_automatic(page) && !ui::world_page_regular_confirmation(page),
          "Tax payment is an automatic source consumer, never a generic confirmation");
    page.legacy_page = 4;
    view = ui::world_task_view(state, page);
    input = {};
    input.enter = true;
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(intent && intent->action == Action::request_abort,
          "Active task management requests a question, never directly aborts the task");
    page.legacy_page = 1;
    check(!ui::world_task_page(state, page), "Ordinary raw1 is not an abort question");
    state.task_abort_questions[page.id] = 444;
    view = ui::world_task_view(state, page);
    selection = {};
    intent = ui::world_task_input(view, layout, selection, input, false);
    check(view.abort_question && intent && intent->selection == 1 &&
              intent->action == Action::confirm &&
              !ui::world_task_related_confirmation(state, page),
          "Bound abort question defaults No and returns the answer for the parent to consume");
    input = {};
    input.escape = true;
    check(!ui::world_task_input(view, layout, selection, input, false),
          "Abort question cannot fabricate an unpublished Back action");
    page.kind = rules::WorldScriptPageKind::dialogue;
    check(!ui::world_task_page(state, page),
          "Dialogue with raw1 number stays a dialogue even when an old binding exists");
    catalogue.tasks.front().factory.pending_completion_value = 23;
    rules::WorldScriptPage victory;
    victory.id = 123;
    victory.kind = rules::WorldScriptPageKind::raw_page;
    victory.legacy_page = 30;
    victory.task_identity = 999;
    victory.task_definition = 8;
    victory.legacy_f = 17;
    state.exploration_summaries.emplace(123, rules::WorldExplorationSummary{30, 999, 8, {}});
    state.active_task.reset();
    state.participants = {42, 42};
    for (const int phase : {0, 1})
        for (const int counter : {0, 39, 40}) {
            state.page_phases[123] = phase;
            state.page_counters[123] = counter;
            const auto result = ui::world_victory_view(state, victory);
            check(result.initialized && result.task == 999 && result.phase == phase &&
                      result.counter == counter && result.experience == 17 &&
                      result.members.size() == 2 && result.members[0].definition == 42 &&
                      result.members[1].definition == 42 && result.popularity == 23 &&
                      result.name == "task-8",
                  "Victory uses retained sparse task identity, display experience and ordered m "
                  "after retirement");
        }
    std::cout << "PASS task UI identity, source-page eligibility, layout and input contracts\n";
}
} // namespace ark::test
