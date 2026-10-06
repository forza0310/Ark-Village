// Read-only activity projection and input routing. Frozen source tests own cost/effect timing,
// quarter resets, random draws, denial order and rollback; these fixtures do not recalculate them.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_village_activity.hpp"
#include <algorithm>
#include <iostream>

namespace ark::test {
namespace {
namespace sim = simulation;
namespace ui = desktop::ui;
using State = sim::StartupWorldRuntimeState;
using Page = sim::rules::WorldScriptPage;
using Action = sim::StartupVillageActivityAction;
Page attach(State &s, int raw, std::uint64_t id) {
    Page page;
    page.id = id;
    page.kind = sim::rules::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    page.lifecycle = 1;
    s.scripts.pages.push_back(page);
    s.activity_pages_initialized.insert(id);
    s.activity_page_selections[id] = s.activity_page_scroll[id] = s.page_counters[id] = 0;
    return page;
}
Page catalogue(State &s) {
    const auto page = attach(s, 51, 901);
    // Deliberately sparse/source-order identities make index-vs-definition mistakes observable.
    s.activity_page_lists[page.id] = {23, 4, 0, 25, 1, 6};
    s.activity_page_display_humans[page.id] = {1, 1};
    s.human_presence.at(1) = 1;
    return page;
}
Vector2 center(Rectangle box) { return {box.x + box.width / 2, box.y + box.height / 2}; }
bool contains(Rectangle outer, Rectangle inner) {
    return inner.x >= outer.x && inner.y >= outer.y && inner.width > 0 && inner.height > 0 &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
} // namespace

void world_village_activity() {
    Checks check{"world_village_activity"};
    auto state = initial_world();
    const auto page = catalogue(state);
    const auto layout = ui::world_village_activity_layout({240, 256});
    ui::WorldVillageActivityInput input;
    input.enter = true;
    state.activity_pages_initialized.erase(page.id);
    const auto unopened = state;
    auto view = ui::world_village_activity_view(state, page);
    check(
        !view.initialized && !ui::world_village_activity_input(view, layout, input, false) &&
            state.activity_pages_initialized == unopened.activity_pages_initialized &&
            same_world_clock(state, unopened) &&
            state.scene.random.draws() == unopened.scene.random.draws(),
        "Uninitialized activity pages cannot initialize, confirm or consume world/random from UI");
    state.activity_pages_initialized.insert(page.id);
    state.activity_page_selections[page.id] = 5;
    state.activity_page_scroll[page.id] = 1;
    state.scripts.activities.at(4).pending_notice = true;
    const auto before = state;
    view = ui::world_village_activity_view(state, page);
    check(view.initialized && view.page == 901 && view.rows.size() == 6 &&
              view.rows[0].definition == 23 && view.rows[1].definition == 4 &&
              view.rows[1].points == 60 && view.rows[1].fresh && view.first_visible == 1 &&
              view.selection == 5,
          "Activity projection preserves source rows, quote, notice and five-row scroll position");
    check(view.portrait_images[0] && view.portrait_images[0] == view.portrait_images[1],
          "With-replacement display identities remain two portraits without deduplication");
    input = {};
    input.click = center({layout.rows.x, layout.rows.y + 4 * layout.row_height, layout.rows.width,
                          layout.row_height});
    auto intent = ui::world_village_activity_input(view, layout, input, false);
    check(intent && intent->action == Action::select && intent->selection == 5 &&
              intent->page == page.id,
          "Scrolled fifth row emits source position five, not activity definition six");
    check(!ui::world_village_activity_input(view, layout, input, true),
          "Pending or paused barrier blocks selection without changing source selection");
    input = {};
    input.up = true;
    check(ui::world_village_activity_input(view, layout, input, false)->action == Action::previous,
          "Previous action lets the source own wrapping and scroll updates");
    input = {};
    input.wheel_rows = 20;
    check(ui::world_village_activity_input(view, layout, input, false)->selection == 5,
          "Wheel clamps to final source list position");
    input.wheel_rows = -20;
    check(ui::world_village_activity_input(view, layout, input, false)->selection == 0,
          "Wheel clamps to first source list position");
    state.activity_page_selections[page.id] = 3;
    state.activity_page_scroll[page.id] = 0;
    view = ui::world_village_activity_view(state, page);
    input = {};
    input.enter = true;
    check(view.rows[3].definition == 25 && view.rows[3].points == 100 && view.rows[3].supported &&
              view.status.empty() && view.can_confirm &&
              ui::world_village_activity_input(view, layout, input, false)->action ==
                  Action::confirm,
          "Published expansion keeps its source quote and uses the same confirmation transport");
    for (const int unsupported : {27, 28, 30}) {
        state.activity_page_lists.at(page.id)[3] = unsupported;
        view = ui::world_village_activity_view(state, page);
        check(view.rows[3].definition == unsupported && view.rows[3].points == 100 &&
                  !view.rows[3].supported && !view.status.empty() && !view.can_confirm &&
                  !ui::world_village_activity_input(view, layout, input, false),
              "Unpublished type4/5/6 stay visible with true quote and cannot submit missing-source "
              "work");
    }
    input = {};
    input.escape = true;
    check(ui::world_village_activity_input(view, layout, input, false)->action == Action::cancel,
          "Unsupported selection still allows returning from catalogue");
    state.activity_page_lists.at(page.id)[3] = 25;
    state.activity_page_selections[page.id] = 0;
    state.quarter_counter = state.village_points = 0;
    view = ui::world_village_activity_view(state, page);
    input = {};
    input.enter = true;
    check(view.can_confirm && view.quarter_slots == 0 && view.points == 0 &&
              ui::world_village_activity_input(view, layout, input, false)->action ==
                  Action::confirm,
          "UI preserves source quarter-before-points denial rather than bypassing refusal events");
    state.activity_page_answers[page.id] = 0;
    view = ui::world_village_activity_view(state, page);
    check(!view.initialized && !ui::world_village_activity_input(view, layout, input, false),
          "Parent answer awaiting real resume update cannot be confirmed again through stale UI");
    state.activity_page_answers.erase(page.id);

    // raw52 requires a living matching raw51 parent; test that UI retains the two choices.
    auto confirmation = before;
    confirmation.activity_page_selections[page.id] = 1;
    confirmation.activity_page_scroll[page.id] = 0;
    const auto child = attach(confirmation, 52, 905);
    confirmation.activity_page_bindings[child.id] = 4;
    confirmation.activity_page_parents[child.id] = page.id;
    view = ui::world_village_activity_view(confirmation, child);
    check(view.initialized && view.name == "马拉松大会" && !view.detail.empty() && view.can_cancel,
          "Confirmation reads exact bound activity and published detail");
    input = {};
    input.click = center(layout.choices[1]);
    intent = ui::world_village_activity_input(view, layout, input, false);
    check(intent && intent->action == Action::select && intent->selection == 1,
          "Clicking Return choice selects source K1 choice without prematurely closing parent");
    confirmation.activity_page_selections[child.id] = 1;
    view = ui::world_village_activity_view(confirmation, child);
    input = {};
    input.enter = true;
    check(ui::world_village_activity_input(view, layout, input, false)->action == Action::confirm,
          "Confirm preserves the source-selected Return choice rather than forcing activity start");
    input = {};
    input.escape = true;
    check(ui::world_village_activity_input(view, layout, input, false)->action == Action::cancel,
          "Escape uses explicit child cancel so only the parent consumer handles its answer");

    auto animation = initial_world();
    const auto animated_page = attach(animation, 53, 909);
    for (const int activity : {4, 25}) {
        animation.activity_page_bindings[animated_page.id] = activity;
        for (const int counter : {0, 40, 69, 70, 119, 120, 121}) {
            animation.page_counters[animated_page.id] = counter;
            view = ui::world_village_activity_view(animation, animated_page);
            input = {};
            input.enter = true;
            const auto confirm = ui::world_village_activity_input(view, layout, input, false);
            check(view.initialized && view.counter == counter &&
                      view.can_confirm == (counter >= 120) &&
                      static_cast<bool>(confirm) == (counter >= 120),
                  "Activity animation confirmation only opens at source counter120 without early "
                  "fast-forward");
            input = {};
            input.escape = input.up = true;
            check(!view.can_cancel && !ui::world_village_activity_input(view, layout, input, false),
                  "Animation has no cancel/navigation action that could skip committed start");
        }
    }

    auto result = initial_world();
    const auto result_page = attach(result, 54, 913);
    result.activity_page_bindings[result_page.id] = 0;
    result.activity_page_lists[result_page.id] = {3, 1, 2};
    result.activity_page_display_humans[result_page.id] = {1, 1};
    result.human_presence.at(1) = 1;
    result.human_presence.at(3) = 0; // Frozen result reference survives later presence changes.
    result.human_activity_previous.at(3) = 17;
    result.shop_humans.at(3).satisfaction = 29;
    result.scene.world.world.ai.growth.at(1).definition.current_profession = 0;
    const auto result_before = result;
    view = ui::world_village_activity_view(result, result_page);
    check(view.initialized && view.rows[0].definition == 3 && view.rows[0].before == 17 &&
              view.rows[0].current == 29 && view.result_attribute == "满足" &&
              view.rows.size() == 3,
          "Result retains frozen human order and projects previous/current satisfaction, not "
          "reward replay");
    check(view.portrait_images[0] ==
              result.rules->jobs.at(0).sprites.at(result.rules->humans.at(1).sex),
          "Activity portraits follow current profession/sex rather than old spawn metadata");
    result.activity_page_bindings[result_page.id] = 6;
    result.human_activity_previous.at(3) = 9;
    result.scene.world.world.ai.growth.at(3).derived.attributes[1] = 21;
    view = ui::world_village_activity_view(result, result_page);
    check(view.rows[0].before == 9 && view.rows[0].current == 21 && view.result_attribute == "力量",
          "Attribute activity result reads selected current derived attribute and stored ao");
    input = {};
    input.enter = true;
    check(ui::world_village_activity_input(view, layout, input, false)->action == Action::confirm,
          "Result confirm only forwards close to source consumer");
    input = {};
    input.escape = true;
    check(ui::world_village_activity_input(view, layout, input, false)->action == Action::cancel,
          "Result Back forwards source cancel without an effect operation");
    result.human_presence.at(1) = 0;
    view = ui::world_village_activity_view(result, result_page);
    check(!view.portrait_images[0] && !view.portrait_images[1] && view.rows.size() == 3,
          "Presence-zero display portraits hide without rebuilding frozen result references");
    check(same_world_clock(result, result_before) &&
              result.scene.random.draws() == result_before.scene.random.draws() &&
              result.sound_requests == result_before.sound_requests &&
              result.activity_counts == result_before.activity_counts &&
              result.events_held == result_before.events_held &&
              result.quarter_counter == result_before.quarter_counter,
          "Repeated view/input projection never draws random, plays sounds or repeats activity "
          "counters");
    for (const auto size :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
        const auto l = ui::world_village_activity_layout(size);
        check(contains({0, 24, static_cast<float>(size.width), size.height - 53.F}, l.panel) &&
                  contains(l.panel, l.rows) && contains(l.panel, l.description) &&
                  contains(l.panel, l.confirm) && contains(l.panel, l.cancel) &&
                  l.row_height >= 17 && !CheckCollisionRecs(l.rows, l.heading) &&
                  !CheckCollisionRecs(l.rows, l.status) &&
                  !CheckCollisionRecs(l.feedback, l.confirm),
              "Minimum and desktop layouts preserve five readable rows, description and footer hit "
              "targets");
        for (const auto portrait : l.portraits)
            check(contains(l.panel, portrait) && !CheckCollisionRecs(portrait, l.feedback) &&
                      !CheckCollisionRecs(portrait, l.status),
                  "Frozen display portraits do not cover status or feedback at any supported size");
    }
    std::cout << "World village activity: " << check.count() << " checks\n";
}
} // namespace ark::test
