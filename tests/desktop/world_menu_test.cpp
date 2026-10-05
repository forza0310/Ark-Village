// Menu input owns desktop navigation only; freeze/atomic task opening belongs to session tests.
#include "support/checks.hpp"
#include "ui/world_menu.hpp"
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
                                                 : !action.has_value()),
                  "Original five row hit areas select faithfully; construction and adventure are "
                  "executable");
            check(!ui::world_menu_input(layout, true, true, false, false, selected, input),
                  "Pause prevents mouse activation without fabricating a page");
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
        check(selected == 4, "Up wraps to system without activating disabled row");
        input = {};
        input.down = true;
        (void)ui::world_menu_input(layout, true, true, true, false, selected, input);
        check(selected == 0, "Down wraps to construction without activating disabled row");
    }
    std::cout << "World menu: " << check.count() << " checks\n";
}
} // namespace ark::test
