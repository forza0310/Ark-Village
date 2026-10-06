// Menu input owns desktop navigation only; freeze/atomic task opening belongs to session tests.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_menu.hpp"
#include "world_save_menu.hpp"
#include <chrono>
#include <iostream>

namespace ark::test {
void world_menu() {
    Checks check{"world_menu"};
    namespace ui = desktop::ui;
    using Intent = ui::WorldMenuIntent;
    const auto middle = [](Rectangle r) { return Vector2{r.x + r.width / 2, r.y + r.height / 2}; };
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
