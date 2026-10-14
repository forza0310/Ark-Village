#pragma once

// Raw21 reads its frozen directory and cursor from the Owner. Only OS gestures and
// last-painted controls live here; a complete source input crosses the session FIFO.
#include "../common/layout.hpp"
#include "ark/simulation/presentation/steam_build_menu_skin.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include <optional>

namespace ark::desktop { class Sprites; }
namespace ark::desktop::ui {
class Skin;
struct WorldBuildMenuView {
    std::uint64_t page{};
    bool initialized{}, interactive{};
    simulation::SteamBuildMenuSkinInput source;
    std::optional<simulation::SteamBuildMenuSkinPlan> skin;
};
struct WorldBuildMenuControls {
    std::uint64_t page{}, generation{}, epoch{};
    simulation::StartupBuildMenuCursor cursor;
    std::array<std::vector<int>, 3> catalogs;
    Extent extent;
    std::array<Rectangle, 5> rows;
    std::array<Rectangle, 3> categories;
    Rectangle previous{}, next{}, cancel{}, details{}, track{};
    std::optional<Rectangle> thumb;
    float drag_grid{};
};
struct WorldBuildMenuGesture {
    std::uint64_t page{}, generation{};
    int category{-1};
    std::optional<int> marked_row;
    bool scrolling{}, consumed{};
    float start_y{}, grid{};
    int start_first{};
};
struct WorldBuildMenuInput {
    std::optional<Vector2> click, pointer;
    bool enter{}, escape{}, up{}, down{}, left{}, right{}, inspect{}, keyboard_event{}, focus_lost{};
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
    int wheel_rows{};
};
bool world_build_menu_page(const simulation::rules::WorldScriptPage &page);
WorldBuildMenuView world_build_menu_view(const simulation::StartupWorldRuntimeState &state,
                                        const simulation::rules::WorldScriptPage &page);
Vector2 world_build_menu_origin(Extent extent);
void prepare_world_build_menu(WorldBuildMenuView &view, const Skin &skin, bool scrolling = false);
// Controls come from this same measured source plan, including empty rows and SEB bounds.
WorldBuildMenuControls world_build_menu_controls(const WorldBuildMenuView &view, Extent extent,
                                                Sprites &sprites, std::uint64_t generation,
                                                std::uint64_t epoch);
std::optional<simulation::StartupBuildMenuInput>
world_build_menu_input(const WorldBuildMenuView &view, Extent extent, std::uint64_t generation,
                       std::uint64_t epoch, const std::optional<WorldBuildMenuControls> &painted,
                       WorldBuildMenuGesture &gesture, const WorldBuildMenuInput &input,
                       bool blocked);
void draw_world_build_menu(const WorldBuildMenuView &view, Extent extent, const Skin &skin,
                           bool controls, bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
