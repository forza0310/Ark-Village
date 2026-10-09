// Menu input owns desktop navigation only; freeze/atomic task opening belongs to session tests.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_menu.hpp"
#include "ui/world_startup.hpp"
#include "world_save_menu.hpp"
#include "world_title.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>

namespace ark::test {
void world_menu() {
    Checks check{"world_menu"};
    namespace ui = desktop::ui;
    using Intent = ui::WorldMenuIntent;
    const auto middle = [](Rectangle r) { return Vector2{r.x + r.width / 2, r.y + r.height / 2}; };
    // Title/slot navigation shares this menu suite. Rule and codec combinations remain in
    // world_save; this verifies the new cold-entry boundary and its actual file re-read.
    for (const auto extent :
         {desktop::Extent{240, 256}, desktop::Extent{540, 360}, desktop::Extent{541, 361}}) {
        using Page = desktop::WorldTitlePage;
        using Action = desktop::WorldTitleAction;
        auto boxes = desktop::world_title_layout(extent);
        desktop::WorldTitleSelection selection;
        std::array<app::WorldSaveSlotInfo, 2> slots{};
        desktop::WorldTitleInput input;
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
        input.click = middle(boxes.records);
        check(!desktop::world_title_input(selection, boxes, slots, input) &&
                  selection.page == Page::records,
              "One record click enters records without starting the world");
        input = {};
        input.right = true;
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.record_page == 1, "Record arrow changes to cash page");
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.record_page == 0, "Record pages cycle");
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
        input.click = middle(boxes.fields[2]);
        desktop::world_title_input(selection, boxes, slots, input);
        check(selection.draft.human.sex == 1 && selection.draft.human.name == "冒险花子",
              "Default name follows sex");
        const auto &main_definition = simulation::startup_world_rules().humans.front();
        const auto preview = ui::world_configuration_actor(selection.draft);
        check(main_definition.identity == 0 && preview && !preview->shadow &&
                  preview->body.image == simulation::startup_world_rules()
                                             .jobs.at(main_definition.definition.current_profession)
                                             .sprites[1] &&
                  preview->body.sprite == 2 && preview->body.frame == 0,
              "Configuration previews definition0 and draft sex without a live actor or clock");
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
        const auto male_preview = ui::world_configuration_actor(selection.draft);
        check(male_preview && male_preview->body.image ==
                                  simulation::startup_world_rules()
                                      .jobs.at(main_definition.definition.current_profession)
                                      .sprites[0],
              "The committed draft sex change also updates the static character preview");
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
              "Menu button clears pause, speed and original popularity artwork at every size");
        int selected = 1;
        ui::WorldMenuInput input;
        input.click = middle(button);
        check(ui::world_menu_input(layout, false, true, false, false, selected, input) ==
                  Intent::open,
              "Menu can open while explicitly paused; task availability is independent");
        check(!ui::world_menu_input(layout, false, false, true, false, selected, input),
              "Source modal/report blocks opening");
        check(ui::world_menu_input(layout, true, false, false, false, selected, input) ==
                  Intent::close,
              "Same menu button closes even while paused or task entry disabled");
        for (int row = 0; row < 5; ++row) {
            input.click = middle(layout.menu_rows[row]);
            const auto action =
                ui::world_menu_input(layout, true, true, true, false, selected, input);
            check(selected == row && (row == 0   ? action == Intent::build
                                      : row == 1 ? action == Intent::tasks
                                      : row == 2 ? action == Intent::village
                                      : row == 4 ? action == Intent::system
                                                 : !action.has_value()),
                  "Original five row hit areas select faithfully; construction, adventure and "
                  "system are "
                  "executable");
            const auto paused_action =
                ui::world_menu_input(layout, true, true, false, false, selected, input);
            check(row == 4 ? paused_action == Intent::system : !paused_action,
                  "Explicit pause permits system files while blocking gameplay menu actions");
            check(layout.menu_rows[row].x >= 0 &&
                      layout.menu_rows[row].x + layout.menu_rows[row].width <= extent.width &&
                      layout.menu_rows[row].y + layout.menu_rows[row].height <= button.y,
                  "All rows fit small and large viewports above footer");
        }
        input = {};
        input.down = true;
        selected = 0;
        check(!ui::world_menu_input(layout, true, true, true, false, selected, input) &&
                  selected == 1,
              "Arrow navigation changes selection without opening task page");
        input = {};
        input.enter = true;
        check(ui::world_menu_input(layout, true, true, true, false, selected, input) ==
                  Intent::tasks,
              "Enter invokes atomic menu-to-task action");
        check(!ui::world_menu_input(layout, true, true, false, false, selected, input),
              "Enter cannot bypass disabled adventure");
        input = {};
        input.escape = true;
        check(ui::world_menu_input(layout, true, true, false, false, selected, input) ==
                  Intent::close,
              "Escape closes independently of explicit pause");
        input = {};
        input.toggle = true;
        check(ui::world_menu_input(layout, true, true, true, false, selected, input) ==
                      Intent::close &&
                  ui::world_menu_input(layout, false, true, true, false, selected, input) ==
                      Intent::open,
              "M toggles both directions through one input contract");
        input.up = input.enter = true;
        input.click = middle(layout.menu_rows[0]);
        selected = 1;
        check(!ui::world_menu_input(layout, true, true, true, true, selected, input) &&
                  selected == 1,
              "Pending/failed barrier rejects duplicate input and leaves selection unchanged");
        input = {};
        input.up = true;
        selected = 0;
        (void)ui::world_menu_input(layout, true, true, true, false, selected, input);
        check(selected == 4, "Up wraps to system without activating it");
        input = {};
        input.down = true;
        (void)ui::world_menu_input(layout, true, true, true, false, selected, input);
        check(selected == 0, "Down wraps to construction without activating disabled row");
        using Village = ui::WorldVillageMenuIntent;
        for (const bool unlocked : {false, true}) {
            input = {};
            input.click = middle(layout.menu_rows[2]);
            const auto action =
                ui::world_village_menu_input(layout, true, false, false, selected, input, unlocked);
            check(selected == 2 && (unlocked ? action == Village::magic_pot : !action),
                  "Magic pot entry uses its own unlock flag, independent of commerce");
            check(!ui::world_village_menu_input(layout, false, true, false, selected, input,
                                                unlocked),
                  "Pause still gates the unlocked magic pot entry");
        }
        for (const bool unlocked : {false, true}) {
            input = {};
            input.click = middle(layout.menu_rows[1]);
            const auto action =
                ui::world_village_menu_input(layout, true, unlocked, false, selected, input);
            check(selected == 1 && (unlocked ? action == Village::commerce : !action),
                  "Desktop management chooser gates commerce on the real unlock flag");
            check(!ui::world_village_menu_input(layout, false, unlocked, false, selected, input),
                  "Explicit pause cannot bypass the commerce gate");
        }
        input = {};
        input.click = middle(layout.menu_rows[0]);
        check(ui::world_village_menu_input(layout, true, false, false, selected, input) ==
                  Village::activities,
              "Activity entry does not depend on commerce unlock");
        input = {};
        input.escape = true;
        check(ui::world_village_menu_input(layout, false, false, false, selected, input) ==
                  Village::back,
              "Escape returns from management chooser to its parent while paused");
        input.toggle = true;
        check(ui::world_village_menu_input(layout, true, true, false, selected, input) ==
                      Village::close &&
                  !ui::world_village_menu_input(layout, true, true, true, selected, input),
              "Menu toggle closes the chooser; pending barrier blocks duplicate navigation");
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
    record_arrow.right = true;
    menu.input(frame, extent, record_arrow, session);
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
} // namespace ark::test
