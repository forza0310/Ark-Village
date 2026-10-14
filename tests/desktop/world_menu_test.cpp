// Menu input owns desktop navigation only; freeze/atomic task opening belongs to session tests.
#include "../../src/desktop/application/world_title.hpp"
#include "../../src/desktop/input/world_save_menu.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/information/world_information.hpp"
#include "../../src/desktop/ui/system/world_manual.hpp"
#include "../../src/desktop/ui/system/world_menu.hpp"
#include "../../src/desktop/ui/system/world_new_game.hpp"
#include "../../src/desktop/ui/system/world_startup.hpp"
#include "../support/checks.hpp"
#include "../support/world_fixture.hpp"
#include "ark/simulation/world/startup_world_save.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>

namespace ark::test {
namespace {
void information_ui(Checks &check) {
    namespace ui = desktop::ui;
    namespace sim = simulation;
    auto s = initial_world();
    s.scripts.pages.back().lifecycle = 2; // Ready-scene boundary for this input projection fixture.
    check(sim::open_startup_world_information_menu(s) == sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_information_pages(s),
          "Information projection starts with the actual source menu catalogue");
    auto &page = s.scripts.pages.back();
    page.lifecycle = 2;
    const auto v = ui::world_information_view(s, page);
    check(v.interactive && v.source && v.source->entries[0].target_raw == 35 &&
              v.source->entries[0].implemented && v.source->entries[1].implemented &&
              v.source->entries[4].target_raw == 38,
          "Information keeps all five original rows and opens the delivered town information");
    const auto l = ui::world_information_layout({240, 256}, 9);
    ui::WorldInformationInput input;
    input.click = Vector2{l.menu_rows[1].x + 5, l.menu_rows[1].y + 5};
    check(ui::world_information_input(v, l, input, false)->select_row == 1,
          "Town row emits only a selection before its independent confirmation");
    input.click = Vector2{l.menu_rows[0].x + 5, l.menu_rows[0].y + 5};
    input.confirm = true;
    auto action = ui::world_information_input(v, l, input, false);
    check(
        action && action->select_row == 0 && !action->confirm &&
            !ui::world_information_input(v, l, input, true),
        "Mouse selection is one source action, never mixed with confirmation or a pending command");
    input = {};
    input.up = input.down = input.right = input.confirm = true;
    action = ui::world_information_input(v, l, input, false);
    check(action && action->up && action->down && action->right && action->confirm,
          "Keyboard masks preserve original precedence for the Owner to consume");
    for (const int raw : {34, 35, 36, 37, 38, 39, 40}) {
        // These snapshots isolate desktop input mapping; source suites own list contents/NEW.
        auto directory = v;
        directory.raw = directory.source->raw = raw;
        directory.source->selection = 1;
        directory.source->first_visible = 0;
        directory.source->items = std::vector<sim::StartupItemInformation>(6);
        const auto boxes = ui::world_information_layout({240, 256}, raw);
        input = {};
        input.confirm = true;
        action = ui::world_information_input(directory, boxes, input, false);
        check(action && action->confirm && !action->select_row,
              "Information confirmation stays a source information action, never gift/use/equip");
        input = {};
        input.cancel = true;
        check(ui::world_information_input(directory, boxes, input, false)->cancel,
              "All information pages return through their own Owner consumer");
        if (raw != 34 && raw != 36) {
            input = {};
            input.click = Vector2{boxes.rows.x + 10, boxes.rows.y + (raw == 38 ? 24.F : 19.F) + 2};
            action = ui::world_information_input(directory, boxes, input, false);
            check(action && action->select_row == 1 && !action->confirm,
                  "Visible row hit maps to one absolute frozen-list selection");
        }
        directory.interactive = false;
        check(!ui::world_information_input(directory, boxes, input, false),
              "Suspended or initializing information pages cannot accept input");
        check(boxes.back.y >= 0 && boxes.back.y + boxes.back.height <= 256 &&
                  boxes.confirm.x >= 0 && boxes.rows.x + boxes.rows.width <= 240,
              "Information rows and independent footer controls fit the minimum window");
    }
    check(s.scene.random.draws() == initial_world().scene.random.draws(),
          "Read-only information UI does not advance the random stream");
    {
        auto town = v;
        town.raw = town.source->raw = 34;
        input = {};
        input.up = input.down = input.left = input.right = true;
        input.wheel_rows = 2;
        check(!ui::world_information_input(town, ui::world_information_layout({240, 256}, 34),
                                           input, false),
              "Town statistics has no fictional row selection, scrolling or tabs");
        auto facilities = v;
        facilities.raw = facilities.source->raw = 39;
        input = {};
        input.left = input.right = true;
        check(!ui::world_information_input(facilities, ui::world_information_layout({240, 256}, 39),
                                           input, false),
              "Facility directory ignores left/right instead of sending invalid source input");
    }
}
void source_navigation_ui(Checks &check) {
    namespace ui = desktop::ui;
    namespace sim = simulation;
    for (bool magic : {false, true}) {
        auto s = initial_world();
        s.scripts.pages.back().lifecycle = 2;
        if (magic)
            s.scripts.user_flags |= 1U;
        check(sim::open_startup_world_main_menu(s) == sim::StartupWorldRuntimeError::none &&
                  sim::initialize_startup_world_menu_pages(s),
              "Main navigation opens a real Owner page");
        const auto view = sim::inspect_startup_world_menu_page(s, s.scripts.pages.back().id);
        check(view && view->tags.size() == (magic ? 6U : 5U) && view->tags.back() == 6,
              "Only the actual magic flag inserts the sixth source main-menu row");
        for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{540, 360}}) {
            const auto boxes = ui::world_source_menu_layout(*view, extent);
            check(boxes.rows.size() == view->tags.size() &&
                      boxes.rows.back().y + 28 <= extent.height,
                  "Source five/six-row geometry remains inside the smallest supported viewport");
            ui::WorldMenuInput input;
            input.click = Vector2{boxes.rows.back().x + 5, boxes.rows.back().y + 5};
            input.enter = true;
            const auto action =
                ui::world_source_menu_input(*view, boxes, input, false, false, false);
            check(action && action->select_row == int(view->tags.size() - 1) && !action->confirm &&
                      !ui::world_source_menu_input(*view, boxes, input, false, false, true),
                  "Source-menu pointer selection cannot confirm or bypass pending input");
        }
    }
    sim::StartupWorldMenuView system;
    system.raw = 10;
    system.tags = {20, 21, 22, 23, 24};
    system.stored_position = {210, 210};
    const auto boxes = ui::world_source_menu_layout(system, {240, 256});
    check(boxes.rows.front().x == 150 && boxes.rows.back().y + 28 <= 256,
          "Submenu bounds use source stored position and native row dimensions");
    for (int row = 0; row < 5; ++row) {
        system.selection = row;
        ui::WorldMenuInput input;
        input.enter = true;
        const auto action = ui::world_source_menu_input(system, boxes, input, false, false, false);
        check(row == 0 || row == 2 ? bool(action && action->confirm) : !action,
              "System save and manual are enabled; settings/ranking/exit lack consumers");
        input = {};
        input.escape = true;
        check(ui::world_source_menu_input(system, boxes, input, false, false, false)->cancel,
              "Disabled system selection still returns without invoking a placeholder page");
    }
    auto save = initial_world();
    save.scripts.pages.back().lifecycle = 2;
    check(sim::open_startup_world_main_menu(save) == sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_menu_pages(save),
          "Save UI fixture opens actual main menu");
    save.scripts.pages.back().lifecycle = 2;
    save.scripts.executing_page = save.scripts.pages.back().id;
    check(sim::open_startup_world_save_page(save) == sim::StartupWorldRuntimeError::none,
          "Save UI fixture enters raw14 through its real source callback");
    save.scripts.executing_page.reset();
    check(sim::initialize_startup_world_save_pages(save), "Initialize source save phase");
    save.scripts.pages.back().lifecycle = 2;
    const auto id = save.scripts.pages.back().id;
    check(!desktop::world_source_save_ready(save, id),
          "Save phase0 cannot close from desktop input");
    auto saving = sim::update_startup_world_save_page(save, id);
    check(saving && !desktop::world_source_save_ready(*saving, id),
          "Save phase1 waits for file consumer");
    auto completed = sim::complete_startup_world_save_page(*saving, id, false, "test failure");
    check(completed && desktop::world_source_save_ready(*completed, id),
          "Save result can close after one completion");
    completed->scene.framework_paused = true;
    check(!desktop::world_source_save_ready(*completed, id),
          "Manual pause also blocks raw14 confirmation");
}
void manual_ui(Checks &check) {
    namespace ui = desktop::ui;
    namespace sim = simulation;
    auto state = initial_world();
    state.scripts.pages.back().lifecycle = 2;
    check(sim::open_startup_world_main_menu(state) == sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_menu_pages(state),
          "Manual fixture starts with actual main menu");
    state.scripts.pages.back().lifecycle = 2;
    sim::StartupWorldMenuInput input;
    input.select_row = 4;
    check(sim::input_startup_world_menu_page(state, state.scripts.pages.back().id, input) ==
              sim::StartupWorldRuntimeError::none,
          "Select actual system tag6");
    input = {};
    input.confirm = true;
    check(sim::input_startup_world_menu_page(state, state.scripts.pages.back().id, input) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_menu_pages(state),
          "Open actual raw10");
    state.scripts.pages.back().lifecycle = 2;
    input = {};
    input.select_row = 2;
    check(sim::input_startup_world_menu_page(state, state.scripts.pages.back().id, input) ==
              sim::StartupWorldRuntimeError::none,
          "Select actual manual tag22");
    input = {};
    input.confirm = true;
    check(sim::input_startup_world_menu_page(state, state.scripts.pages.back().id, input) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::initialize_startup_world_manual_pages(state),
          "Open and initialize actual raw13 once");
    state.scripts.pages.back().lifecycle = 2;
    const auto page = state.scripts.pages.back();
    auto view = ui::world_manual_view(state, page);
    check(view.interactive && view.source && !view.source->about && view.source->index == 0,
          "Desktop reads the actual manual body and lifetime");
    const auto draws = state.scene.random.draws();
    const auto decorations = view.source->decorations;
    ui::WorldManualInput key;
    key.confirm = key.right = true;
    auto action = ui::world_manual_input(view, {240, 256}, {}, key, false);
    check(action && action->confirm && action->right && !action->left &&
              !ui::world_manual_input(view, {240, 256}, {}, key, true),
          "One typed manual input preserves simultaneous confirm/right and blocks pending");
    check(sim::input_startup_world_manual_page(state, page.id, *action) ==
              sim::StartupWorldRuntimeError::none,
          "Manual keyboard projection reaches the source consumer");
    view = ui::world_manual_view(state, state.scripts.pages.back());
    check(view.source->index == 1 && state.scene.random.draws() == draws &&
              view.source->decorations == decorations,
          "Confirm/right advances once without reshuffling the frozen display cast");
    key.left = true;
    action = ui::world_manual_input(view, {240, 256}, {}, key, false);
    check(action && action->left &&
              sim::input_startup_world_manual_page(state, page.id, *action) ==
                  sim::StartupWorldRuntimeError::none &&
              sim::inspect_startup_world_manual_page(state, page.id)->index == 1,
          "Independent left follows the merged forward pulse without changing the net page");
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{1080, 720}}) {
        ui::WorldManualLayout painted;
        painted.page = page.id;
        painted.index = 1;
        painted.extent = extent;
        painted.arrows = {Rectangle{10, 50, 12, 15}, Rectangle{210, 50, 12, 15}};
        painted.back = {extent.width - 61.F, extent.height - 25.F, 53, 20};
        key = {};
        key.click = Vector2{216, 56};
        key.confirm = key.left = true;
        action = ui::world_manual_input(view, extent, painted, key, false);
        check(action && action->right && !action->confirm && !action->left,
              "Click uses the painted arrow and emits one action without mixing keyboard input");
        ++painted.index;
        check(!ui::world_manual_input(view, extent, painted, key, false),
              "Page change rejects stale painted manual controls");
        --painted.index;
        --painted.extent.width;
        check(!ui::world_manual_input(view, extent, painted, key, false),
              "Resize rejects stale arrow coordinates");
        painted.extent = extent;
        ++painted.page;
        check(!ui::world_manual_input(view, extent, painted, key, false),
              "Replacement page cannot consume retired controls");
    }
    key = {};
    key.left = true;
    for (int n = 0; n < 2; ++n) {
        action = ui::world_manual_input(view, {240, 256}, {}, key, false);
        check(action && sim::input_startup_world_manual_page(state, page.id, *action) ==
                            sim::StartupWorldRuntimeError::none,
              "Previous page follows source wrap");
        view = ui::world_manual_view(state, state.scripts.pages.back());
    }
    check(view.source->about && view.source->decorations == decorations &&
              state.scene.random.draws() == draws,
          "About page wraps through the same Owner and retains frozen identity/random state");
    auto paused = view;
    paused.interactive = false;
    check(!ui::world_manual_input(paused, {240, 256}, {}, key, false),
          "Paused or uninitialized manual rejects input");
    key = {};
    key.cancel = true;
    action = ui::world_manual_input(view, {240, 256}, {}, key, false);
    check(action && action->cancel &&
              sim::input_startup_world_manual_page(state, page.id, *action) ==
                  sim::StartupWorldRuntimeError::none,
          "Manual back uses its typed close and returns to the real scene");
}
// Resolve only resource geometry; fixtures do not duplicate record Init, clocks or selection.
std::array<simulation::SteamManualArrowPlan, 2>
record_arrows(const simulation::SteamRecordSkinPlan &source, desktop::Sprites &sprites) {
    std::array<simulation::SteamManualArrowPlan, 2> result;
    int count{};
    for (const auto &operation : source.draws)
        if (const auto *arrow = std::get_if<simulation::SteamManualArrowRequest>(&operation)) {
            const auto bounds = sprites.frame_bounds("arrow02.seb", arrow->bounds_frame);
            const auto plan = simulation::steam_manual_arrow_skin(
                *arrow, {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
            if (!plan || count >= int(result.size()))
                throw std::runtime_error("Record fixture cannot resolve the published arrow");
            result[count++] = *plan;
        }
    if (count != int(result.size()))
        throw std::runtime_error("Record fixture lacks two source arrows");
    return result;
}
void static_records_ui(Checks &check, desktop::Sprites &sprites) {
    namespace ui = desktop::ui;
    namespace sim = simulation;
    sim::StartupSystemRecords records;
    records.high_score = 1234;
    records.cash_peak = 987654321;
    records.score_village = "积分纪录村";
    records.cash_village = "资金纪录村";
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{541, 361}})
        for (int page = 0; page < 2; ++page) {
            const auto source = ui::world_records_plan(records, page, extent, {84, 70, 45.5F});
            check(source.has_value(), "Record pages project system2 fields through the shared adapter");
            std::vector<sim::SteamRecordText> texts;
            int windows{}, actors{};
            for (const auto &operation : source->draws) {
                if (const auto *text = std::get_if<sim::SteamRecordText>(&operation))
                    texts.push_back(*text);
                windows += std::holds_alternative<sim::SteamRecordWindow>(operation);
                actors += std::holds_alternative<sim::SteamManualActorRequest>(operation);
            }
            check(windows == 2 && actors == 0 && texts.size() == 4 &&
                      texts[1].value ==
                          (page == 0 ? records.score_village : records.cash_village) &&
                      texts[1].position[0] == 97.25F &&
                      texts.back().value == (page == 0 ? "1,234" : "987,654,321Ｇ"),
                  "Static record projection keeps system value/village pairing, float width and no "
                  "invented Init list");
            const auto layout =
                ui::world_records_layout(*source, page, extent, record_arrows(*source, sprites));
            check(layout.origin.x == (extent.width - 240) / 2 &&
                      layout.origin.y == (extent.height - 240) / 2 + 13,
                  "Record helper translates source geometry once, including odd viewports");
            for (const auto &arrow : layout.arrows) {
                const Vector2 click{arrow.x + 1, arrow.y + 1};
                check(ui::world_records_input(&layout, click, false, false, false) ==
                          ui::WorldRecordsAction::turn,
                      "Each actual arrow registration rectangle turns the record page");
            }
            const Vector2 gap{layout.origin.x + 120, layout.arrows[0].y + 1};
            check(ui::world_records_input(&layout, gap, false, false, false) ==
                          ui::WorldRecordsAction::none &&
                      ui::world_records_input(nullptr, gap, false, false, false) ==
                          ui::WorldRecordsAction::none &&
                      ui::world_records_input(&layout, {}, true, true, true) ==
                          ui::WorldRecordsAction::turn,
                  "Record arrows exclude the old full-width band and direction wins over return");
        }
}
} // namespace
void world_menu() {
    Checks check{"world_menu"};
    information_ui(check);
    source_navigation_ui(check);
    manual_ui(check);
    namespace ui = desktop::ui;
    const auto middle = [](Rectangle r) { return Vector2{r.x + r.width / 2, r.y + r.height / 2}; };
    desktop::Sprites sprites(ARK_TEST_ASSETS); // SEB metadata only, without a window/texture.
    static_records_ui(check, sprites);
    // Title/slot navigation shares this menu suite. Rule and codec combinations remain in
    // world_save; this verifies the new cold-entry boundary and its actual file re-read.
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{541, 361}}) {
        using Page = desktop::WorldTitlePage;
        using Action = desktop::WorldTitleAction;
        auto boxes = desktop::world_title_layout(extent);
        desktop::WorldTitleSelection selection;
        const auto source = ui::world_new_game_plan(selection.draft, 0, 0, 0, {48, 48}, 48);
        check(source.has_value(), "Title input fixture supplies the actual raw91 source plan");
        std::array<simulation::SteamManualArrowPlan, 2> arrows;
        for (const auto &operation : source->operations)
            if (const auto *arrow = std::get_if<simulation::SteamNewGameArrow>(&operation)) {
                const auto bounds = sprites.frame_bounds("arrow02.seb", arrow->source.bounds_frame);
                const auto plan = simulation::steam_manual_arrow_skin(
                    arrow->source,
                    {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
                check(plan.has_value(), "Title arrow geometry resolves the real source SEB bounds");
                arrows[arrow->value == 16 ? 0 : 1] = *plan;
            }
        const auto configuration = ui::world_new_game_layout(*source, extent, arrows);
        boxes.fields = configuration.fields;
        boxes.sex_arrows = configuration.arrows;
        check(configuration.origin.x == (extent.width - 240) / 2 &&
                  configuration.origin.y == (extent.height - 256) / 2 &&
                  boxes.fields[0].x == configuration.origin.x + 106 &&
                  boxes.fields[0].width == 114 && boxes.fields[3].height == 56,
              "Raw91 Touch uses one integer-centred origin and keeps source-expanded start bounds");
        for (const auto &arrow : boxes.sex_arrows)
            check(arrow.x >= 0 && arrow.y >= 0 && arrow.x + arrow.width <= extent.width &&
                      arrow.y + arrow.height <= extent.height,
                  "New-game arrow margins stay inside the current viewport");
        std::array<app::WorldSaveSlotInfo, 2> slots{};
        desktop::WorldTitleInput input;
        const auto records = ui::world_records_plan({}, 0, extent, {84, 70, 45.5F});
        check(records.has_value(), "Title fixture uses the shared static record plan");
        const auto record_layout =
            ui::world_records_layout(*records, 0, extent, record_arrows(*records, sprites));
        boxes.previous = record_layout.arrows[0];
        boxes.next = record_layout.arrows[1];
        check(boxes.background.width == 600 && boxes.background.height == 380 &&
                  boxes.background.y + boxes.background.height == extent.height &&
                  boxes.background.x == (extent.width - 240) / 2 - 180 && boxes.book.width == 98 &&
                  boxes.book.height == 68,
              "Steam background stays native size and bottom anchored, including odd viewports");
        const auto cropped =
            desktop::clip_sprite_blit({{0, 0, 600, 380}, boxes.background}, boxes.viewport);
        check(cropped && cropped->destination.x == 0 && cropped->destination.y == 0 &&
                  cropped->destination.width == extent.width &&
                  cropped->destination.height == extent.height &&
                  std::abs(cropped->source.x + boxes.background.x) < .001F &&
                  std::abs(cropped->source.y + boxes.background.y) < .001F &&
                  std::abs(cropped->source.width - cropped->destination.width) < .001F &&
                  std::abs(cropped->source.height - cropped->destination.height) < .001F,
              "Small title viewport clips source pixels without stretching the background");
        {
            desktop::WorldTitleSelection edge_selection;
            desktop::WorldTitleInput edge_input;
            edge_input.click = Vector2{boxes.start.x + 1, boxes.start.y + 1};
            check(!CheckCollisionPointRec(*edge_input.click, boxes.menu_rows[0].highlight) &&
                      !desktop::world_title_input(edge_selection, boxes, slots, edge_input) &&
                      edge_selection.page == Page::slots,
                  "Title row touch margin activates outside the narrower painted highlight");
            edge_selection = {};
            edge_input.click = Vector2{boxes.start.x - 1, boxes.start.y + 1};
            check(!desktop::world_title_input(edge_selection, boxes, slots, edge_input) &&
                      edge_selection.page == Page::title,
                  "A click outside the source title touch region leaves navigation unchanged");
        }
        input.up = input.down = input.confirm = true;
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.title_selection == 0 && selection.page == Page::slots,
              "Title applies up then down independently before the same confirmation");
        selection = {};
        input = {};
        input.click = middle(boxes.records);
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::records,
              "One record click enters records without starting the world");
        input.click = middle(boxes.previous);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.record_page == 1, "Title record left uses the actual source helper bounds");
        input.click = middle(boxes.next);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.record_page == 0, "Title record right uses the same shared geometry");
        input = {};
        input.right = true;
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.record_page == 1, "Record arrow changes to cash page");
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.record_page == 0, "Record pages cycle");
        input.left = input.back = true;
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.page == Page::records && selection.record_page == 1,
              "Record left/right/back in one input turns once and preserves the parent");
        input = {};
        input.back = true;
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.page == Page::title && selection.title_selection == 1,
              "Back preserves record title selection");
        input = {};
        input.click = middle(boxes.start);
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::slots,
              "Start opens slots without advancing or starting a world");
        input.click = middle(boxes.slots[1]);
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::configure && selection.slot == 1,
              "An empty slot enters the four-field configuration");
        input = {};
        input.click = Vector2{boxes.sex_arrows[0].x + boxes.sex_arrows[0].width - 1,
                              boxes.sex_arrows[0].y + boxes.sex_arrows[0].height - 2};
        check(CheckCollisionPointRec(*input.click, boxes.fields[2]) &&
                  !desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.field == 2 && selection.draft.human.sex == 1 &&
                  selection.page == Page::configure,
              "One overlapping gender-arrow release changes sex once before the wider field Touch");
        input.click = middle(boxes.sex_arrows[1]);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.draft.human.sex == 0 && selection.draft.human.name == "冒险太郎",
              "The other source AddTouch arrow returns to the male default draft");
        input.click = Vector2{boxes.fields[2].x + boxes.fields[2].width / 2,
                              boxes.fields[2].y + boxes.fields[2].height - 2};
        check(CheckCollisionPointRec(*input.click, boxes.fields[2]) &&
                  CheckCollisionPointRec(*input.click, boxes.fields[3]) &&
                  !CheckCollisionPointRec(*input.click, boxes.sex_arrows[0]) &&
                  !CheckCollisionPointRec(*input.click, boxes.sex_arrows[1]),
              "Source gender/start Touch overlap is separate from either AddTouch arrow");
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::configure && selection.field == 2 &&
                  selection.draft.human.sex == 1,
              "A gender-field release in the start margin cannot install a new world");
        input.click = middle(boxes.sex_arrows[1]);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.draft.human.sex == 0,
              "Return to the male draft before the independent gender-field click");
        input.click = middle(boxes.fields[2]);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.draft.human.sex == 1 && selection.draft.human.name == "冒险花子",
              "Default name follows sex");
        const auto &jobs = simulation::startup_world_rules().jobs;
        const auto adventure = std::find_if(jobs.begin(), jobs.end(),
                                            [](const auto &job) { return (job.flags & 4U) != 0; });
        const auto preview =
            ui::world_new_game_plan(selection.draft, selection.field, 0, 0, {48, 48}, 48);
        check(
            adventure != jobs.end() && preview.has_value(),
            "Configuration binds the published adventure profession and an explicit static frame");
        const auto portrait = std::find_if(
            preview->operations.begin(), preview->operations.end(), [](const auto &op) {
                return std::holds_alternative<simulation::SteamHumanPortrait>(op);
            });
        check(portrait != preview->operations.end(), "New-game plan supplies its actor request");
        const auto &actor = std::get<simulation::SteamHumanPortrait>(*portrait).actor;
        check(!actor.shadow && actor.weapon && actor.body.image == adventure->sprites[1] &&
                  actor.body.sprite == 2 && actor.body.frame == 0,
              "Configuration reads draft sex and source weapon0 without creating a live actor");
        input.click = middle(boxes.fields[1]);
        desktop::world_title_input(selection, boxes, slots, input);
        selection.edit_text = "UI测试甲乙";
        input = {};
        input.erase = true;
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.edit_text == "UI测试甲", "Backspace removes one complete UTF8 character");
        input = {};
        input.confirm = true;
        desktop::world_title_input(selection, boxes, slots, input);
        input = {};
        input.click = middle(boxes.fields[2]);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.draft.human.sex == 0 && selection.draft.human.name == "UI测试甲" &&
                  selection.draft.human.custom_name,
              "Custom name survives sex switch");
        const auto male_preview =
            ui::world_new_game_plan(selection.draft, selection.field, 0, 0, {48, 48}, 48);
        check(male_preview.has_value(), "Committed male draft retains a valid source skin");
        const auto male_portrait = std::find_if(
            male_preview->operations.begin(), male_preview->operations.end(), [](const auto &op) {
                return std::holds_alternative<simulation::SteamHumanPortrait>(op);
            });
        check(male_portrait != male_preview->operations.end() &&
                  std::get<simulation::SteamHumanPortrait>(*male_portrait).actor.body.image ==
                      adventure->sprites[0],
              "The committed draft sex change also updates the static character preview");
        {
            auto empty_selection = selection;
            empty_selection.page = Page::text_edit;
            empty_selection.field = 1;
            empty_selection.edit_text.clear();
            input = {};
            input.confirm = true;
            desktop::world_title_input(empty_selection, boxes, slots, input);
            check(empty_selection.page == Page::configure &&
                      empty_selection.draft.human.name.empty() &&
                      empty_selection.draft.human.custom_name,
                  "The approved PC editor can still commit an empty custom name");
            empty_selection.draft.village.clear();
            const auto empty =
                ui::world_new_game_plan(empty_selection.draft, 1, 0, 0, {48, 48}, 48);
            check(empty.has_value(), "Legal empty draft values do not crash new-game drawing");
            int empty_values{};
            for (const auto &operation : empty->operations)
                if (const auto *text = std::get_if<simulation::SteamHumanOverviewText>(&operation))
                    if (text->rectangle[0] == 163 &&
                        (text->rectangle[1] == 135 || text->rectangle[1] == 157) &&
                        text->value.empty())
                        ++empty_values;
            check(empty_values == 2 && empty_selection.draft.village.empty() &&
                      empty_selection.draft.human.name.empty(),
                  "Drawing-only placeholders are invisible and never written back to the draft");
        }
        input = {};
        input.back = true;
        desktop::world_title_input(selection, boxes, slots, input);
        input = {};
        input.click = middle(boxes.slots[1]);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.page == Page::configure && selection.draft.human.name == "UI测试甲",
              "Cancel and reenter retains draft");
        input.click = middle(boxes.fields[3]);
        check(desktop::world_title_input(selection, boxes, slots, input) == Action::new_game &&
                  selection.draft.slot == 1,
              "Only explicit final start requests world installation");
        input = {};
        input.back = true;
        desktop::world_title_input(selection, boxes, slots, input);
        slots[1].exists = true;
        slots[1].metadata = app::WorldSaveMetadata{};
        input = {};
        input.click = middle(boxes.slots[1]);
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::actions,
              "Occupied slot opens actions");
        input.click = middle(boxes.actions[2]);
        check(!desktop::world_title_input(selection, boxes, slots, input),
              "Deletion remains disabled");
        input.click = middle(boxes.actions[0]);
        check(desktop::world_title_input(selection, boxes, slots, input) == Action::load,
              "Continue selects this file");
        slots[1].error = app::WorldSaveError::malformed;
        check(!desktop::world_title_input(selection, boxes, slots, input),
              "Invalid file cannot continue");
        input.click = middle(boxes.actions[1]);
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::overwrite,
              "Restart first requests explicit confirmation");
        input = {};
        input.confirm = true;
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::configure,
              "Restart confirmation edits a draft without writing the slot");
        check(boxes.book.x + boxes.book.width <= extent.width &&
                  boxes.book.y + boxes.book.height <= extent.height &&
                  boxes.slots[1].y + boxes.slots[1].height <= boxes.back.y,
              "Book and slots fit the smallest viewport without footer overlap");
    }
    {
        const auto directory =
            std::filesystem::current_path() /
            ("title-save-test-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        auto saved = initial_world(1);
        const auto image = app::capture_world_save(saved);
        check(image.image.has_value(), "Title test uses a valid player image");
        check(app::write_world_save_slot(directory, 1, *image.image).error ==
                  app::WorldSaveError::none,
              "Write isolated title manual slot");
        auto initial = initial_world(77);
        initial.scene.framework_paused = true;
        initial.scene.random.draw(19);
        auto expected_random = initial.scene.random;
        std::string reason;
        check(
            desktop::load_world_title_slot(directory, 1, initial, reason) &&
                initial.scene.framework_paused && same_world_clock(initial, saved) &&
                initial.scene.random.draw(197).ticket == expected_random.draw(197).ticket,
            "Cold continue installs validated save without updates and retains fresh random/pause");
        const auto before = initial;
        std::ofstream(app::world_save_slot_path(directory, 1), std::ios::binary | std::ios::trunc)
            << "corrupt";
        check(!desktop::load_world_title_slot(directory, 1, initial, reason) &&
                  same_world_clock(initial, before) &&
                  initial.scene.random.draws() == before.scene.random.draws() &&
                  initial.scene.world.world.ai.accounting.funds() ==
                      before.scene.world.world.ai.accounting.funds(),
              "Continue re-reads changed file and leaves the initial owner intact on failure");
        std::filesystem::remove_all(directory);
    }
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{960, 640}}) {
        const ui::Layout layout(extent);
        const auto button = ui::world_menu_button(extent);
        check(!CheckCollisionRecs(button, layout.left_button) &&
                  !CheckCollisionRecs(button, layout.right_button) &&
                  button.x + button.width <= layout.right_button.x - 77,
              "Menu button clears pause and original popularity artwork at every size");
    }
    // These are explicit read-only slot view fixtures. Their paused worker has no file overlay,
    // so submitted actions are rejected without writing a file; this suite owns only the UI's
    // confirmation and duplicate-input boundary. File/Owner roundtrips have separate contracts.
    auto initial = initial_world();
    initial.scene.framework_paused = true;
    app::WorldSession session(std::move(initial),
                              std::filesystem::path(ARK_TEST_OUTPUT) / "menu-input-no-writes");
    app::WorldFrame frame = *session.frame();
    frame.save_menu_open = true;
    frame.save_slots[0].exists = true;
    frame.save_slots[0].metadata = app::WorldSaveMetadata{"测试村", 0, 3, 1, 540, 5000};
    desktop::WorldSaveMenu menu;
    menu.observe(frame);
    const desktop::Extent extent{240, 256};
    const auto slots = desktop::world_save_menu_layout(extent);
    {
        // Source-save selection is save-only, including after a stale load draft.
        desktop::WorldSaveMenu source_menu;
        source_menu.observe(frame);
        desktop::WorldSaveMenuInput input;
        input.click = middle(slots.load[0]);
        source_menu.input(frame, extent, input, session);
        auto source_frame = frame;
        source_frame.save_source_page = 987;
        source_menu.observe(source_frame);
        input = {};
        input.click = middle(slots.load[0]);
        source_menu.input(source_frame, extent, input, session);
        input = {};
        input.right = true;
        input.enter = true;
        source_menu.input(source_frame, extent, input, session);
        check(!source_menu.pending() && session.frame()->last_command_serial == 0,
              "Source selector clears stale load confirmation and ignores load toggle/click");
        input = {};
        input.escape = true;
        source_menu.input(source_frame, extent, input, session);
        input = {};
        input.click = middle(slots.records);
        source_menu.input(source_frame, extent, input, session);
        input = {};
        input.enter = true;
        source_menu.input(source_frame, extent, input, session);
        input = {};
        input.escape = true;
        source_menu.input(source_frame, extent, input, session);
        check(!source_menu.pending() && session.frame()->last_command_serial == 0,
              "Source selector record hit cannot hide save confirmation or submit a command");
    }
    for (int slot = 0; slot < 2; ++slot) {
        check(slots.slots[slot].x >= slots.panel.x &&
                  slots.slots[slot].x + slots.slots[slot].width <=
                      slots.panel.x + slots.panel.width &&
                  slots.load[slot].y + slots.load[slot].height <=
                      slots.slots[slot].y + slots.slots[slot].height &&
                  slots.slots[slot].y + slots.slots[slot].height <= slots.message.y,
              "Two-slot summary/actions fit the minimum viewport without overlapping status");
    }
    desktop::WorldSaveMenuInput click;
    click.click = middle(slots.records);
    menu.input(frame, extent, click, session);
    desktop::WorldSaveMenuInput record_arrow;
    record_arrow.left = record_arrow.right = record_arrow.escape = true;
    menu.input(frame, extent, record_arrow, session);
    click.click = middle(slots.save[0]);
    menu.input(frame, extent, click, session);
    desktop::WorldSaveMenuInput record_confirm;
    record_confirm.enter = true;
    menu.input(frame, extent, record_confirm, session);
    check(!menu.pending() && session.frame()->last_command_serial == 0,
          "Record direction wins over simultaneous back; unpainted clicks cannot reach save "
          "confirmation");
    desktop::WorldSaveMenuInput record_back;
    record_back.escape = true;
    menu.input(frame, extent, record_back, session);
    check(!menu.pending() && session.frame()->last_command_serial == 0,
          "In-game records navigation and return submit no world/save command");
    click.click = middle(slots.save[0]);
    menu.input(frame, extent, click, session);
    check(!menu.pending() && session.frame()->last_command_serial == 0,
          "Existing save first opens confirmation without submitting overwrite");
    desktop::WorldSaveMenuInput cancel;
    cancel.escape = true;
    menu.input(frame, extent, cancel, session);
    check(!menu.pending(), "Escape cancels overwrite without closing the slot menu or submitting");
    click.click = middle(slots.load[0]);
    menu.input(frame, extent, click, session);
    check(!menu.pending(), "Load also requires explicit confirmation before worker command");
    menu.input(frame, extent, cancel, session);
    click.click = middle(slots.load[1]);
    menu.input(frame, extent, click, session);
    check(!menu.pending(), "Empty-slot Load cannot submit a command");
    frame.save_slots[0].error = app::WorldSaveError::malformed;
    click.click = middle(slots.load[0]);
    menu.input(frame, extent, click, session);
    check(!menu.pending(), "Invalid-slot Load cannot bypass validation status");
    frame.save_slots[0].error = app::WorldSaveError::none;
    frame.save_busy = true;
    click.click = middle(slots.save[1]);
    menu.input(frame, extent, click, session);
    menu.input(frame, extent, cancel, session);
    check(!menu.pending(), "Busy overlay rejects save and close input");
    frame.save_busy = false;
    click.click = middle(slots.load[0]);
    menu.input(frame, extent, click, session);
    desktop::WorldSaveMenuInput confirm;
    confirm.enter = true;
    menu.input(frame, extent, confirm, session);
    check(menu.pending(), "Confirmed Load submits one FIFO action");
    menu.input(frame, extent, confirm, session);
    menu.input(frame, extent, cancel, session);
    auto result = session.frame();
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (result->last_command_serial == 0 && std::chrono::steady_clock::now() < limit)
        result = session.wait_for_frame_after(result->revision, std::chrono::milliseconds(100));
    check(result->last_command_serial == 1,
          "Pending confirmation blocks duplicate Enter and Escape before acknowledgement");
    session.stop();
    std::cout << "World menu: " << check.count() << " checks\n";
}
// Optional record-window samples; no record Init list, clock, world or save writes.
void world_records_render_fixture() {
    namespace ui = desktop::ui;
    Checks check{"world_records_render_fixture"};
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "records";
    std::filesystem::create_directories(output);
    struct Window {
        Window() {
            InitWindow(720, 600, "Ark-Village UI fixture / static records");
            if (!IsWindowReady())
                throw std::runtime_error("Record fixture could not create a window");
            SetTargetFPS(60);
        }
        ~Window() { CloseWindow(); }
    } window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    desktop::Text text(ARK_TEST_FONT, "UI测试城镇名称比较长ハイスコアＧ");
    ui::Skin skin(sprites, text);
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (int page = 0; page < 2; ++page) {
            simulation::StartupSystemRecords records;
            records.cash_peak = std::numeric_limits<std::int64_t>::max();
            records.cash_village = "UI测试城镇名称比较长UI测试城镇名称比较长";
            const auto paint = ui::prepare_world_records(records, page, extent, skin);
            for (const auto &arrow : paint.layout.arrows)
                check(arrow.width > 0 && arrow.height > 0 && arrow.x >= 0 && arrow.y >= 0 &&
                          arrow.x + arrow.width <= extent.width &&
                          arrow.y + arrow.height <= extent.height,
                      "Actual source record arrow Touch fits the displayed viewport");
            int text_index{};
            for (const auto &operation : paint.source.draws)
                if (const auto *label = std::get_if<simulation::SteamRecordText>(&operation)) {
                    const auto size = paint.text_sizes.at(text_index++);
                    check(size > 0 && label->position[0] >= 20 &&
                              label->position[0] + text.width(label->value, size) <= 220,
                          "Full record village/value text fits its source window after PC fitting");
                }
            for (int frame = 0; frame < 8; ++frame) {
                check(!WindowShouldClose(), "Record fixture remains open until capture");
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                ui::draw_world_records(paint, skin);
                skin.button(paint.layout.back, "返回");
                text.flush(1, {});
                EndDrawing();
            }
            const auto file = output / ("records-" + std::to_string(extent.width) +
                                        (page == 0 ? "-zero-score.png" : "-long-cash.png"));
            auto image = LoadImageFromScreen();
            const bool exported = image.data && ExportImage(image, file.string().c_str());
            UnloadImage(image);
            check(exported && std::filesystem::file_size(file) > 0,
                  "Actual static record rendering exports a nonempty PNG");
            std::cout << "UI fixture / conditional static records page=" << page
                      << " extent=" << extent.width << "x" << extent.height
                      << " frames=8 screenshot=" << file.string() << '\n';
        }
    }
    std::cout << "Static record rendering: " << check.count() << " checks\n";
}

// Optional actual-window raw91 snapshots; no new world, save, original OS input or page clock.
void world_new_game_render_fixture() {
    namespace ui = desktop::ui;
    Checks check{"world_new_game_render_fixture"};
    const auto output = std::filesystem::path(ARK_TEST_OUTPUT) / "new-game";
    std::filesystem::create_directories(output);
    struct Window {
        Window() {
            InitWindow(720, 600, "Ark-Village UI fixture / new-game draft");
            if (!IsWindowReady())
                throw std::runtime_error("New-game fixture could not create a window");
            SetTargetFPS(60);
        }
        ~Window() { CloseWindow(); }
    } window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    desktop::Text text(ARK_TEST_FONT, "UI测试甲乙村城镇冒险者姓名比较长");
    ui::Skin skin(sprites, text);
    struct Sample {
        const char *name;
        int selected, sex;
        bool long_name{}, empty{};
    };
    constexpr Sample samples[]{{"village-male", 0, 0},    {"name-male", 1, 0},
                               {"sex-male", 2, 0},        {"start-male", 3, 0},
                               {"village-female", 0, 1},  {"name-female", 1, 1},
                               {"sex-female", 2, 1},      {"start-female", 3, 1},
                               {"long-name", 1, 1, true}, {"empty-values", 0, 0, false, true}};
    for (const auto extent : {desktop::Extent{240, 256}, desktop::Extent{720, 600}}) {
        SetWindowSize(extent.width, extent.height);
        for (const auto &sample : samples) {
            app::WorldNewGameDraft draft;
            app::change_world_draft_sex(draft, sample.sex);
            if (sample.long_name) {
                draft.village = "UI测试城镇名称比较长";
                draft.human.name = "UI测试冒险者姓名比较长";
                draft.human.custom_name = true;
            } else if (sample.empty) {
                draft.village.clear();
                draft.human.name.clear();
                draft.human.custom_name = true;
            }
            const auto before = draft;
            const auto paint =
                ui::prepare_world_new_game(draft, sample.selected, extent, skin, 0, 0);
            for (const auto &field : paint.layout.fields)
                check(field.width > 0 && field.height > 0 && field.x >= 0 && field.y >= 0 &&
                          field.x + field.width <= extent.width &&
                          field.y + field.height <= extent.height,
                      "Actual measured raw91 Touch fits the displayed viewport");
            for (int frame = 0; frame < 8; ++frame) {
                check(!WindowShouldClose(), "New-game fixture remains open until capture");
                BeginDrawing();
                ClearBackground({145, 211, 247, 255});
                DrawText("CONDITIONAL draft UI", 4, 4, 12, BLACK);
                ui::draw_world_new_game(paint, skin);
                text.flush(1, {});
                EndDrawing();
            }
            const auto file =
                output / ("raw91-" + std::to_string(extent.width) + "-" + sample.name + ".png");
            auto image = LoadImageFromScreen();
            const bool exported = image.data && ExportImage(image, file.string().c_str());
            UnloadImage(image);
            check(exported && std::filesystem::file_size(file) > 0,
                  "Actual new-game rendering exports a nonempty PNG");
            check(draft.village == before.village && draft.human.name == before.human.name &&
                      draft.human.sex == before.human.sex &&
                      draft.human.custom_name == before.human.custom_name &&
                      draft.slot == before.slot,
                  "Repeated source drawing preserves every approved draft field");
            std::cout << "UI fixture / conditional raw91 sample=" << sample.name
                      << " extent=" << extent.width << "x" << extent.height
                      << " frames=8 screenshot=" << file.string() << '\n';
        }
    }
    std::cout << "New-game rendering: " << check.count() << " checks\n";
}
} // namespace ark::test
