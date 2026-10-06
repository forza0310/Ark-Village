// Tests source identity/order and the actual click-to-action path without a window or fake award.
#include "support/checks.hpp"
#include "ui/world_award.hpp"
#include "ui/world_progression.hpp"
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
void world_award_ui() {
    Checks check{"world_award_ui"};
    namespace ui = ark::desktop::ui;
    namespace sim = ark::simulation;
    namespace rules = sim::rules;
    using Action = rules::WorldAwardAction;
    rules::WorldScriptPage page;
    page.kind = rules::WorldScriptPageKind::raw_page;
    for (const int id : {16, 56, 57, 97}) {
        page.legacy_page = id;
        check(ui::world_page_automatic(page) && !ui::world_page_regular_confirmation(page),
              "Timed waits and camera pages must never submit ordinary confirmation");
    }
    page.legacy_page = 87;
    check(!ui::world_page_automatic(page) && !ui::world_page_regular_confirmation(page),
          "Annual page must await an explicit annual action rather than ordinary acknowledgement");
    for (const int raw : {51, 52, 53, 54}) {
        page.legacy_page = raw;
        check(!ui::world_page_regular_confirmation(page),
              "Village pages require their own action and cannot enter the generic ack path");
    }
    page.legacy_page = 89;
    check(ui::world_page_regular_confirmation(page),
          "Introduction page must keep its source early-confirm and later-close inputs");

    sim::StartupWorldRuntimeState state;
    sim::StartupWorldRules catalogue;
    for (const int id : {42, 7, 19}) {
        sim::StartupWorldHuman human;
        human.identity = id;
        human.name = "human-" + std::to_string(id);
        catalogue.humans.push_back(human);
        state.human_calendar[id].contribution = id == 7 ? 60 : 80;
    }
    state.rules = &catalogue;
    state.medal_count = 7;
    const auto uninitialized = ui::world_award_view(state, 123);
    check(!uninitialized.initialized && state.award_rankings.empty(),
          "Reading a newly inserted page must not initialize ranks or grant a medal");
    state.award_rankings[123] = {19, 42, 7};
    state.award_termination_pending[123] = false;
    const auto view = ui::world_award_view(state, 123);
    check(view.initialized && view.medals == 7 && view.rows.size() == 3 &&
              view.rows[0].definition == 19 && view.rows[0].name == "human-19" &&
              view.rows[1].definition == 42 && view.rows[2].contribution == 60,
          "Display must resolve definition identities and preserve source order including tied "
          "ranks");
    check(state.medal_count == 7 && !state.award_termination_pending.at(123),
          "Rendering data must not consume or offer awards");
    ui::WorldAwardSelection selection;
    const auto click_action = [&](const auto &v, const auto &layout, std::optional<Vector2> click,
                                  bool enter, bool escape, bool blocked) -> std::optional<Action> {
        const auto intent =
            ui::world_award_input(v, layout, selection, {click, enter, escape}, blocked);
        return intent ? std::optional<Action>{intent->action} : std::nullopt;
    };
    for (const auto extent : {ark::desktop::Extent{240, 256}, ark::desktop::Extent{540, 360}}) {
        const auto layout = ui::world_award_layout(extent, false);
        check(contains({0, 24, static_cast<float>(extent.width), extent.height - 53.F},
                       layout.panel) &&
                  contains(layout.panel, layout.rows) && contains(layout.panel, layout.terminate),
              "Annual content and command must remain within the playable viewport");
        check(click_action(view, layout, middle(layout.terminate), false, false, false) ==
                  Action::request_termination,
              "End button requests a question rather than immediately closing the page");
        check(!click_action(view, layout, middle(layout.terminate), true, false, true),
              "Pause, failure or an input awaiting its serial must block repeated actions");
        check(!click_action(uninitialized, layout, {}, true, false, false),
              "Uninitialized annual page must not accept an action");
        check(!click_action(view, layout, Vector2{0, 0}, false, false, false),
              "Clicks outside an annual command must not confirm");
        state.award_termination_pending[123] = true;
        const auto pending = ui::world_award_view(state, 123);
        const auto question = ui::world_award_layout(extent, true);
        check(contains(question.panel, question.prompt) && contains(question.panel, question.yes) &&
                  contains(question.panel, question.no) &&
                  question.rows.y + question.rows.height + 4 < question.prompt.y,
              "Termination prompt and ranked rows must not overlap");
        check(click_action(pending, question, middle(question.yes), false, false, false) ==
                      Action::confirm_termination &&
                  click_action(pending, question, middle(question.no), false, false, false) ==
                      Action::reject_termination,
              "Yes and No must select separate source actions from actual pending state");
        check(click_action(pending, question, {}, false, true, false) == Action::reject_termination,
              "Escape cancels the question rather than ending the annual page");
        state.award_termination_pending[123] = false;
    }
    const auto layout = ui::world_award_layout({540, 360}, false);
    selection = {};
    ui::WorldAwardInput input;
    input.down = true;
    (void)ui::world_award_input(view, layout, selection, input, false);
    input = {};
    input.enter = true;
    auto award = ui::world_award_input(view, layout, selection, input, false);
    check(award && award->action == Action::request_award && award->selection == 1 &&
              selection.prompt == 0 && state.medal_count == 7,
          "Award selection sends ranking index, defaults question to Yes and never pays in UI");
    state.award_pending_humans[123] = 42;
    const auto award_pending = ui::world_award_view(state, 123);
    check(award_pending.pending_human == 42 && award_pending.pending_name == "human-42",
          "Award question is bound to the runtime human, not the currently selected row");
    input = {};
    input.escape = true;
    award = ui::world_award_input(award_pending, layout, selection, input, false);
    check(award && award->action == Action::reject_award,
          "Award question rejection cannot terminate the ceremony");
    input = {};
    input.click = middle(ui::world_award_layout({540, 360}, true).yes);
    award = ui::world_award_input(award_pending, ui::world_award_layout({540, 360}, true),
                                  selection, input, false);
    check(award && award->action == Action::confirm_award && state.medal_count == 7,
          "Award confirmation is a separate FIFO intent and does not decrement displayed medals");

    // These source-backed presentation pages share UI dependencies; test their decision gates
    // here instead of introducing another executable for the same fixture lifetime.
    page.id = 321;
    page.legacy_page = 48;
    state.rank = 1;
    auto progression = ui::world_progression_view(state, page);
    check(!progression.initialized && !ui::world_page_regular_confirmation(page),
          "Rank choice must wait for initialization and never become generic confirmation");
    state.page_counters[321] = 0;
    state.rank_values = {123, 4, 5, 6};
    state.rank_met = {true, false, true, false};
    progression = ui::world_progression_view(state, page);
    const auto rank_layout = ui::world_progression_layout({540, 360});
    int rank_selection = 0;
    ui::WorldProgressionInput rank_input;
    rank_input.down = true;
    (void)ui::world_progression_input(progression, rank_layout, rank_selection, rank_input, false);
    rank_input = {};
    rank_input.enter = true;
    auto rank_intent =
        ui::world_progression_input(progression, rank_layout, rank_selection, rank_input, false);
    check(progression.rows.size() == 5 && rank_intent && rank_intent->rank_action &&
              rank_intent->selection == 1 && state.rank == 1,
          "Condition row requests source explanation, without promoting from rendering");
    check(!ui::world_progression_input(progression, rank_layout, rank_selection, rank_input, true),
          "Pending or paused progression does not repeat a command");
    page.legacy_page = 49;
    progression = ui::world_progression_view(state, page);
    rank_intent =
        ui::world_progression_input(progression, rank_layout, rank_selection, rank_input, false);
    check(progression.rows.size() == 4 && rank_intent && !rank_intent->rank_action,
          "Rank status49 remains acknowledgement only, never promotion");
    page.legacy_page = 50;
    state.rank_celebration_participants[321] = {{42, 0, 0, 0, 0}, {7, 0, 0, 0, 0}};
    state.page_phases[321] = 2;
    state.page_counters[321] = 139;
    progression = ui::world_progression_view(state, page);
    check(progression.rows == std::vector<std::string>{"human-42", "human-7"} &&
              !ui::world_progression_input(progression, rank_layout, rank_selection, rank_input,
                                           false),
          "Celebration uses source cast and cannot close before phase2 counter140");
    state.page_counters[321] = 140;
    check(ui::world_progression_view(state, page).confirm_enabled,
          "Celebration enables confirmation exactly at source close threshold");
    page.legacy_page = 88;
    state.page_human_bindings[321] = 19;
    state.page_phases[321] = 0;
    state.reward_display[0] = {10, 20};
    state.reward_display[1] = {15, 25};
    progression = ui::world_progression_view(state, page);
    check(progression.initialized && progression.human == "human-19" &&
              progression.rows[0] == "满足 10 > 15" && !progression.confirm_enabled,
          "Award display reads bound person/shared reward values, with phase0 input blocked");
    state.page_phases[321] = 1;
    check(ui::world_progression_view(state, page).confirm_enabled,
          "Award phase1 confirmation delegates skip/close to the simulation");
    page.legacy_page = 67;
    for (const auto &[counter, accepted] :
         std::vector<std::pair<int, bool>>{{66, true}, {67, false}, {72, false}, {73, true}}) {
        state.page_counters[321] = counter;
        check(ui::world_progression_view(state, page).confirm_enabled == accepted,
              "Effort display preserves early skip and delayed close thresholds");
    }
    page.kind = rules::WorldScriptPageKind::dialogue;
    check(!ui::world_progression_page(page) && !ui::world_page_automatic(page),
          "A script dialogue with the same legacy number is never treated as a raw page");
    for (const auto extent : {ark::desktop::Extent{240, 256}, ark::desktop::Extent{540, 360}}) {
        const auto boxes = ui::world_progression_layout(extent);
        check(contains(boxes.panel, boxes.body) && contains(boxes.panel, boxes.cancel) &&
                  contains(boxes.panel, boxes.confirm),
              "Progression contents and commands remain in the supported viewport");
    }
    std::cout << "PASS annual UI source projection and input contracts\n";
}
} // namespace ark::test
