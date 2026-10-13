// Tests source identity/order and the actual click-to-action path without a window or fake award.
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/village/world_award.hpp"
#include "../../src/desktop/ui/village/world_progression.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
#include <cstdlib>
#include <filesystem>
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
// Explicit roster-at-page callsite fixture: it checks S067/S068 composition without replaying
// an entire year. Eligibility and ranks are supplied, never claimed as a natural annual result.
void world_award_render_fixture() {
    using namespace ark::desktop;
    auto state = initial_world();
    constexpr std::uint64_t page = 1;
    for (int id = 0; id < 7; ++id)
        state.award_rankings[page].push_back(id);
    state.award_termination_pending[page] = false;
    state.page_counters[page] = 8;
    const auto before = state;
    InitWindow(540, 360, "Ark-Village annual UI / supplied roster fixture");
    struct WindowScope {
        ~WindowScope() { CloseWindow(); }
    } window;
    Sprites sprites(ARK_TEST_ASSETS);
    std::string glyphs = "信息勋章授予式中止授予还有个姓名贡献努力获得消费居住要仪吗是否恭喜!"
                         "太好了!的能力上升基础奖励满意度确定→";
    for (const auto &h : state.rules->humans)
        glyphs += h.name;
    Text text(ARK_TEST_FONT, glyphs);
    ui::Skin skin(sprites, text);
    const auto *isolated = std::getenv("ARK_TEST_OUTPUT_DIR");
    const auto output = std::filesystem::path(isolated ? isolated : ARK_TEST_OUTPUT) / "award";
    std::filesystem::create_directories(output);
    for (const auto extent : {Extent{540, 360}, Extent{240, 256}}) {
        SetWindowSize(extent.width, extent.height);
        for (int tab = 0; tab < 4; ++tab) {
            const bool pending = tab == 3;
            ui::WorldAwardSelection selection;
            selection.tab = pending ? 0 : tab;
            selection.selected = 6;
            selection.first_row = 2;
            auto view = ui::world_award_view(state, page);
            view.termination_pending = pending; // Read-only display variant, no award command.
            for (int frame = 0; frame < 4; ++frame) {
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                ui::draw_world_award(view, ui::world_award_layout(extent, pending), skin, selection,
                                     true);
                text.flush(1, {});
                EndDrawing();
            }
            const auto file =
                output /
                (std::string(pending ? "question-" : "candidates-" + std::to_string(tab) + "-") +
                 std::to_string(extent.width) + ".png");
            auto image = LoadImageFromScreen();
            const bool saved = image.data && ExportImage(image, file.string().c_str());
            if (image.data)
                UnloadImage(image);
            if (!saved)
                throw std::runtime_error("Annual display fixture capture failed");
            std::cout << "Annual supplied-roster fixture: " << file.string() << '\n';
        }
    }
    // Source-bound display conditions cover the real desktop dispatcher, not natural awards.
    auto ceremony = initial_world();
    simulation::rules::WorldScriptPage result;
    result.id = ceremony.scripts.next_page_id++;
    result.kind = simulation::rules::WorldScriptPageKind::raw_page;
    result.lifecycle = 2;
    ceremony.scripts.pages.push_back(result);
    ceremony.page_human_bindings[result.id] = 1;
    ceremony.page_phases[result.id] = 0;
    ceremony.reward_display = {{{19, 19}, {25, 25}, {6, 6}}};
    ceremony.effort_display = {{{40, 50, 60, 70}, {42, 51, 59, 80}, {2, 1, -1, 10}}};
    for (const auto extent : {Extent{240, 256}, Extent{720, 600}})
        for (int raw : {67, 88})
            for (int counter : raw == 67 ? std::vector<int>{0, 42, 52, 67, 73}
                                         : std::vector<int>{0, 35, 65, 100, 135, 202, 212}) {
                auto s = ceremony;
                auto &p = s.scripts.pages.back();
                p.legacy_page = raw;
                p.legacy_f = 19;
                p.legacy_g = 25;
                s.page_counters[p.id] = counter;
                s.page_phases[p.id] = raw == 88 && counter >= 135 ? 1 : 0;
                const auto frozen = s;
                auto v = ui::world_progression_view(s, p);
                ui::prepare_world_progression(v, s, skin);
                if (!v.ceremony)
                    throw std::runtime_error("Desktop ceremony lost strict source bridge");
                SetWindowSize(extent.width, extent.height);
                for (int frame = 0; frame < 4; ++frame) {
                    BeginDrawing();
                    ClearBackground({145, 211, 247, 255});
                    ui::draw_world_progression(v, ui::world_progression_layout(extent, raw), skin,
                                               true);
                    text.flush(1, {});
                    EndDrawing();
                }
                const auto file = output / (std::to_string(raw) + "-" + std::to_string(counter) +
                                            "-" + std::to_string(extent.width) + ".png");
                auto image = LoadImageFromScreen();
                const bool saved = image.data && ExportImage(image, file.string().c_str());
                if (image.data)
                    UnloadImage(image);
                if (!saved || !same_world_clock(s, frozen) ||
                    s.page_counters != frozen.page_counters ||
                    s.page_phases != frozen.page_phases ||
                    s.sound_requests != frozen.sound_requests ||
                    s.scene.random.draws() != frozen.scene.random.draws())
                    throw std::runtime_error("Ceremony render changed state or screenshot failed");
                std::cout << "Annual conditional ceremony: " << file.string() << '\n';
            }
    if (!same_world_clock(state, before) || state.medal_count != before.medal_count ||
        state.scene.random.draws() != before.scene.random.draws())
        throw std::runtime_error("Annual rendering changed world time, medals or randomness");
}
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
    catalogue.humans.resize(43);
    catalogue.jobs.resize(2);
    catalogue.jobs[0].sprites = {11, 12};
    catalogue.jobs[1].sprites = {21, 22};
    for (const int id : {42, 7, 19}) {
        sim::StartupWorldHuman human;
        human.identity = id;
        human.name = "human-" + std::to_string(id);
        catalogue.humans[id] = human;
        state.scene.world.world.ai.growth[id].definition.current_profession = id == 19 ? 1 : 0;
        state.scene.world.world.ai.growth[id].definition.legacy_u = id + 5;
        state.human_calendar[id].contribution = id == 7 ? 60 : 80;
    }
    state.rules = &catalogue;
    state.medal_count = 7;
    const auto uninitialized = ui::world_award_view(state, 123);
    check(!uninitialized.initialized && state.award_rankings.empty(),
          "Reading a newly inserted page must not initialize ranks or grant a medal");
    state.award_rankings[123] = {19, 42, 7};
    state.award_termination_pending[123] = false;
    state.page_counters[123] = 8;
    for (int id : {19, 42, 7}) {
        state.scene.world.world.ai.battle.humans[id].killed_stat1 = id + 100;
        state.scene.world.world.human_spending[id] = id + 200;
        state.human_homes[id] = {0, 0, id == 19 ? 1 : 0};
        state.human_calendar[id].celebrations = id + 1;
    }
    const auto view = ui::world_award_view(state, 123);
    check(view.initialized && view.medals == 7 && view.rows.size() == 3 &&
              view.rows[0].definition == 19 && view.rows[0].name == "human-19" &&
              view.rows[1].definition == 42 && view.rows[2].contribution == 60 &&
              view.rows[0].effort == 24 && view.rows[0].portrait_image == 21 &&
              view.rows[1].portrait_image == 11 && view.rows[0].year_acquired == 119 &&
              view.rows[0].year_spent == 219 && view.rows[0].residence == 1 &&
              view.rows[0].medals == 20,
          "Display must resolve definition identities and preserve source order including tied "
          "ranks");
    check(state.medal_count == 7 && !state.award_termination_pending.at(123),
          "Rendering data must not consume or offer awards");
    ui::WorldAwardSelection selection;
    const auto navigation = ui::world_award_layout({240, 256}, false);
    ui::WorldAwardInput both;
    both.left = both.right = both.up = both.down = true;
    check(!ui::world_award_input(view, navigation, selection, both, false) && selection.tab == 0 &&
              selection.selected == 0,
          "Source annual cursor processes both directions in order without submitting an award");
    both = {};
    both.left = true;
    ui::world_award_input(view, navigation, selection, both, false);
    check(selection.tab == 2 && selection.selected == 0 && state.medal_count == 7,
          "Annual three-page wrap preserves selected identity and never spends a medal");
    both = {};
    both.right = true;
    ui::world_award_input(view, navigation, selection, both, true);
    check(selection.tab == 2, "Pending input cannot turn an annual page");
    const auto click_action = [&](const auto &v, const auto &layout, std::optional<Vector2> click,
                                  bool enter, bool escape, bool blocked) -> std::optional<Action> {
        const auto intent =
            ui::world_award_input(v, layout, selection, {click, enter, escape}, blocked);
        return intent ? std::optional<Action>{intent->action} : std::nullopt;
    };
    for (const auto extent : {ark::desktop::Extent{240, 256}, ark::desktop::Extent{540, 360}}) {
        const auto layout = ui::world_award_layout(extent, false);
        check(layout.row_height >= 16 && layout.rows.height == 5 * layout.row_height,
              "S067 keeps five readable candidates at the minimum and ordinary viewport");
        check(contains({0, 24, static_cast<float>(extent.width), extent.height - 53.F},
                       layout.panel) &&
                  contains(layout.panel, layout.rows) &&
                  contains({0, 24, float(extent.width), extent.height - 24.F}, layout.terminate) &&
                  !CheckCollisionRecs(layout.panel, layout.terminate),
              "Annual source content and separate PC command fit within the viewport");
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
                  question.prompt.y + question.prompt.height < question.yes.y,
              "S068 separate question keeps its text clear of the Yes/No commands");
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
    state = initial_world(); // The new rank bridge requires real definitions and complete Init.
    page.id = 321;
    page.legacy_page = 48;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.lifecycle = 0;
    state.scripts.pages.push_back(page);
    state.scripts.next_page_id = 322;
    state.rank = 1;
    auto progression = ui::world_progression_view(state, page);
    check(!progression.initialized && !ui::world_page_regular_confirmation(page),
          "Rank choice must wait for initialization and never become generic confirmation");
    check(simulation::initialize_startup_world_town_progress_pages(state),
          "Rank source initializes the actual selection and counter");
    page = state.scripts.pages.back();
    state.rank_values = {123, 4, 5, 6};
    state.rank_met = {true, false, true, false};
    progression = ui::world_progression_view(state, page);
    const auto rank_layout = ui::world_progression_layout({540, 360});
    ui::WorldProgressionInput rank_input;
    rank_input.down = true;
    const auto moved = ui::world_progression_input(progression, rank_layout, rank_input, false);
    check(moved && moved->input && moved->input->down &&
              simulation::input_startup_world_runtime_rank_page(state, page.id, *moved->input) ==
                  simulation::StartupWorldRuntimeError::none,
          "Direction changes the source selection, not a desktop-only cursor");
    progression = ui::world_progression_view(state, page);
    rank_input = {};
    rank_input.enter = true;
    auto rank_intent = ui::world_progression_input(progression, rank_layout, rank_input, false);
    check(progression.rows.size() == 5 && rank_intent && rank_intent->rank_action &&
              rank_intent->selection == 1 && state.rank == 1,
          "Condition row requests source explanation, without promoting from rendering");
    check(!ui::world_progression_input(progression, rank_layout, rank_input, true),
          "Pending or paused progression does not repeat a command");
    page.legacy_page = 49;
    state.page_phases[321] = 0;
    progression = ui::world_progression_view(state, page);
    rank_intent = ui::world_progression_input(progression, rank_layout, rank_input, false);
    check(progression.rows.size() == 4 && rank_intent && !rank_intent->rank_action,
          "Rank status49 remains acknowledgement only, never promotion");
    page.legacy_page = 50;
    state.rank_celebration_participants[321] = {{19, 0, 0, 0, 0}, {7, 0, 0, 0, 0}};
    state.page_secondary_counters[321] = 0;
    state.page_phases[321] = 2;
    state.page_counters[321] = 139;
    progression = ui::world_progression_view(state, page);
    check(progression.rows == std::vector<std::string>{state.rules->humans.at(19).name,
                                                       state.rules->humans.at(7).name} &&
              !ui::world_progression_input(progression, rank_layout, rank_input, false),
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
    check(progression.initialized && progression.human == state.rules->humans.at(19).name &&
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
        const Rectangle viewport{0, 24, float(extent.width), extent.height - 24.F};
        check(contains(viewport, boxes.panel) && contains(boxes.panel, boxes.body) &&
                  contains(viewport, boxes.cancel) && contains(viewport, boxes.confirm) &&
                  !CheckCollisionRecs(boxes.panel, boxes.cancel) &&
                  !CheckCollisionRecs(boxes.cancel, boxes.confirm),
              "Source progress body and distinct PC soft keys fit without overlap");
    }
    std::cout << "PASS annual UI source projection and input contracts\n";
}
} // namespace ark::test
