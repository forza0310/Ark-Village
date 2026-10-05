// Tests source identity/order and the actual click-to-action path without a window or fake award.
#include "ui/world_award.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
Vector2 middle(Rectangle r) { return {r.x + r.width / 2, r.y + r.height / 2}; }
bool contains(Rectangle outer, Rectangle inner) {
    return inner.width > 0 && inner.height > 0 && inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
} // namespace
int main() {
    namespace ui = ark::desktop::ui;
    namespace sim = ark::simulation;
    namespace rules = sim::rules;
    using Action = rules::WorldAwardAction;
    rules::WorldScriptPage page;
    page.kind = rules::WorldScriptPageKind::raw_page;
    for (const int id : {16, 56, 57}) {
        page.legacy_page = id;
        check(ui::world_page_automatic(page) && !ui::world_page_regular_confirmation(page),
              "Timed waits and camera pages must never submit ordinary confirmation");
    }
    page.legacy_page = 87;
    check(!ui::world_page_automatic(page) && !ui::world_page_regular_confirmation(page),
          "Annual page must await an explicit annual action rather than ordinary acknowledgement");
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
    for (const auto extent : {ark::desktop::Extent{240, 256}, ark::desktop::Extent{540, 360}}) {
        const auto layout = ui::world_award_layout(extent, false);
        check(contains({0, 24, static_cast<float>(extent.width), extent.height - 53.F},
                       layout.panel) &&
                  contains(layout.panel, layout.rows) && contains(layout.panel, layout.terminate),
              "Annual content and command must remain within the playable viewport");
        check(ui::world_award_input(view, layout, middle(layout.terminate), false, false, false) ==
                  Action::request_termination,
              "End button requests a question rather than immediately closing the page");
        check(!ui::world_award_input(view, layout, middle(layout.terminate), true, false, true),
              "Pause, failure or an input awaiting its serial must block repeated actions");
        check(!ui::world_award_input(uninitialized, layout, {}, true, false, false),
              "Uninitialized annual page must not accept an action");
        check(!ui::world_award_input(view, layout, Vector2{0, 0}, false, false, false),
              "Clicks outside an annual command must not confirm");
        state.award_termination_pending[123] = true;
        const auto pending = ui::world_award_view(state, 123);
        const auto question = ui::world_award_layout(extent, true);
        check(contains(question.panel, question.prompt) && contains(question.panel, question.yes) &&
                  contains(question.panel, question.no) &&
                  question.rows.y + question.rows.height + 4 < question.prompt.y,
              "Termination prompt and ranked rows must not overlap");
        check(ui::world_award_input(pending, question, middle(question.yes), false, false, false) ==
                      Action::confirm_termination &&
                  ui::world_award_input(pending, question, middle(question.no), false, false,
                                        false) == Action::reject_termination,
              "Yes and No must select separate source actions from actual pending state");
        check(ui::world_award_input(pending, question, {}, false, true, false) ==
                  Action::reject_termination,
              "Escape cancels the question rather than ending the annual page");
        state.award_termination_pending[123] = false;
    }
    std::cout << "PASS annual UI source projection and input contracts\n";
}
