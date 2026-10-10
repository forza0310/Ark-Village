#pragma once

// Desktop projects the Owner menu tree and submits typed input.
#include "../common/layout.hpp"
#include "ark/simulation/world/startup_world_menu.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldMenuInput {
    std::optional<Vector2> click;
    bool toggle{}, escape{}, up{}, down{}, enter{};
};
Rectangle world_menu_button(Extent extent);
std::string world_source_menu_label(int tag);
bool world_source_menu_page(const simulation::rules::WorldScriptPage &page);
struct WorldSourceMenuLayout {
    Vector2 origin;
    std::vector<Rectangle> rows;
    Rectangle confirm, back;
};
WorldSourceMenuLayout world_source_menu_layout(const simulation::StartupWorldMenuView &view,
                                               Extent extent);
bool world_source_menu_enabled(int raw, int tag);
std::optional<simulation::StartupWorldMenuInput>
world_source_menu_input(const simulation::StartupWorldMenuView &view,
                        const WorldSourceMenuLayout &layout, const WorldMenuInput &input, bool left,
                        bool right, bool blocked);
void draw_world_source_menus(const simulation::StartupWorldRuntimeState &state, Extent extent,
                             const Skin &skin, bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
