#pragma once

// Desktop menu metadata never masquerades as a source raw3 page or a second game owner.
#include "layout.hpp"
#include <optional>
#include <string>

namespace ark::desktop::ui {
class Skin;
enum class WorldMenuIntent { open, close, tasks, build, village, system };
struct WorldMenuInput {
    std::optional<Vector2> click;
    bool toggle{}, escape{}, up{}, down{}, enter{};
};
Rectangle world_menu_button(Extent extent);
// Selection is local; every resulting action must be acknowledged by WorldSession FIFO.
std::optional<WorldMenuIntent> world_menu_input(const Layout &layout, bool opened, bool can_open,
                                                bool can_manage, bool pending, int &selected,
                                                const WorldMenuInput &input);
void draw_world_menu(const Layout &layout, const Skin &skin, int selected, bool can_manage,
                     const std::string &feedback = {}, bool can_system = true);
} // namespace ark::desktop::ui
