// PAGES/S001 proves five rows and their artwork. Open/close/freeze is desktop adaptation.
#include "world_menu.hpp"
#include "script_text.hpp"
#include "skin.hpp"
#include <algorithm>

namespace ark::desktop::ui {
Rectangle world_menu_button(Extent extent) {
    // At minimum width popularity begins at x103; pause ends at x60.
    return {62, extent.height - 25.F, 40, 22};
}
std::optional<WorldMenuIntent> world_menu_input(const Layout &layout, bool opened, bool can_open,
                                                bool can_manage, bool pending, int &selected,
                                                const WorldMenuInput &input) {
    if (pending)
        return std::nullopt;
    const bool toggle =
        input.toggle ||
        (input.click && CheckCollisionPointRec(*input.click, world_menu_button(layout.extent)));
    if (!opened)
        return can_open && toggle ? std::optional{WorldMenuIntent::open} : std::nullopt;
    if (toggle || input.escape)
        return WorldMenuIntent::close;
    selected = std::clamp(selected, 0, 4);
    if (input.up)
        selected = (selected + 4) % 5;
    if (input.down)
        selected = (selected + 1) % 5;
    bool activate = input.enter;
    if (input.click)
        for (int i = 0; i < 5; ++i)
            if (CheckCollisionPointRec(*input.click, layout.menu_rows[i])) {
                selected = i;
                activate = true;
                break;
            }
    if (activate && selected < 2 && can_manage)
        return selected == 0 ? WorldMenuIntent::build : WorldMenuIntent::tasks;
    if (activate && selected == 4)
        return WorldMenuIntent::system;
    return std::nullopt;
}
void draw_world_menu(const Layout &layout, const Skin &skin, int selected, bool can_manage,
                     const std::string &feedback, bool can_system) {
    constexpr const char *names[]{"建设", "冒险", "村办", "情报", "系统"};
    constexpr int icons[]{0, 1, 2, 5, 6};
    for (int i = 0; i < 5; ++i) {
        const auto row = layout.menu_rows[i];
        skin.sprites.draw("menu.seb", selected == i ? 2 : 3, {row.x, row.y}, WHITE,
                          Sprites::Binding::common);
        skin.sprites.draw("wnd_menuIcon.seb", icons[i], {row.x + 4, row.y + 5}, WHITE,
                          Sprites::Binding::common);
        skin.text.draw(names[i], row.x + 26, row.y + 8,
                       ((i < 2 && can_manage) || (i == 4 && can_system))
                           ? (selected == i ? ink : WHITE)
                           : Color{182, 174, 147, 255});
        if (i == selected)
            skin.sprites.draw("finger_r.seb", 0, {row.x + row.width - 1, row.y + 11}, WHITE,
                              Sprites::Binding::common);
    }
    if (!feedback.empty()) {
        const auto &row = layout.menu_rows[4];
        const float width = layout.extent.width - 12.F;
        const auto lines = wrap_plain_text(feedback, width, [&](const auto &value) {
            return skin.text.width(value) * 11.F / 12.F;
        });
        const int visible =
            std::min(static_cast<int>(lines.size()),
                     std::max(1, static_cast<int>((layout.extent.height - row.y - 65) / 14)));
        skin.content({row.x, row.y + 31, width + 4, visible * 14.F + 4});
        for (int line = 0; line < visible; ++line)
            skin.text.draw(lines[line], row.x + 2, row.y + 33 + line * 14.F, MAROON, 11);
    }
}
} // namespace ark::desktop::ui
