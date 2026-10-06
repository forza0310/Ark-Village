// Rendering acceptance at the published editing callsite, not natural flag32 unlock or OS input.
// Only the gate flag is fixture input. Original map, buildings, prices and source transactions
// remain real; no world updates are manufactured to obtain the two requested frames.
#include "ark/simulation/startup_world_editing.hpp"
#include "support/world_fixture.hpp"
#include "ui/skin.hpp"
#include "world_editing.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace ark::test {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(std::string("Map-edit rendering callsite fixture: ") + message);
}
class EditFixtureWindow {
  public:
    EditFixtureWindow() {
        SetConfigFlags(FLAG_WINDOW_HIDDEN);
        InitWindow(720, 600, "Ark-Village map-edit callsite fixture / flags32 supplied");
        require(IsWindowReady(), "cannot create hidden rendering window");
    }
    ~EditFixtureWindow() { CloseWindow(); }
    EditFixtureWindow(const EditFixtureWindow &) = delete;
    EditFixtureWindow &operator=(const EditFixtureWindow &) = delete;
};
void accepted(const simulation::StartupBuildResult &result, const char *message) {
    require(result.error == simulation::StartupWorldRuntimeError::none &&
                result.denial == simulation::StartupBuildDenial::none,
            message);
}
} // namespace
void render_world_edit_fixture() {
    namespace rules = simulation::rules;
    namespace ui = desktop::ui;
    auto state = initial_world();
    const auto initial = state;
    const auto cash = state.scene.world.world.ai.accounting.funds();
    const auto draws = state.scene.random.draws();
    const auto old = std::find_if(
        state.scene.world.world.facilities.begin(), state.scene.world.world.facilities.end(),
        [](const auto &entry) { return entry.second.placement.definition_id == 33; });
    require(old != state.scene.world.world.facilities.end(), "real startup bun shop33 is absent");
    const auto old_id = old->first;
    const auto old_placement = old->second.placement;
    state.scripts.user_flags |= 32U; // Explicit callsite gate; no claimed natural first-star event.
    accepted(simulation::begin_startup_world_edit(state, true), "cannot enter source move mode6");
    accepted(simulation::confirm_startup_world_edit(state, old_placement.anchor,
                                                    old_placement.orientation),
             "cannot select real bun shop");
    require(state.build_mode == 7 && state.build_moving_facility == old_id,
            "source selection did not retain its real moving instance");

    // Pick the closest genuinely empty legal footprint through the existing advisory view.
    // It neither edits the map nor substitutes a cheaper definition to obtain a screenshot.
    std::optional<rules::Position> target;
    int nearest = std::numeric_limits<int>::max();
    const auto &map = state.scene.world.world.map;
    for (int y = 0; y < map.height; ++y)
        for (int x = 0; x < map.width; ++x) {
            const int distance =
                std::abs(x - old_placement.anchor.x) + std::abs(y - old_placement.anchor.y);
            if (distance < nearest &&
                desktop::world_build_preview(state, 33, {x, y}, old_placement.orientation)
                    .valid()) {
                target = rules::Position{x, y};
                nearest = distance;
            }
        }
    require(target.has_value(), "real map has no legal vacant moving destination");
    const auto ghost = desktop::world_build_preview(state, 33, *target, old_placement.orientation);
    require(ghost.graphic_visible && ghost.cost == 300 && !ghost.graphic.frames.empty(),
            "mode7 must show its source ghost and fixed300G fee");
    const std::filesystem::path output = std::filesystem::path(ARK_TEST_OUTPUT) / "map-editing";
    std::filesystem::create_directories(output);
    desktop::WorldCameraView camera{{}, desktop::world_viewport({720, 600}, 1)};
    camera.camera = {
        15.F * (old_placement.anchor.x + old_placement.anchor.y + target->x + target->y) + 30,
        7.5F * (old_placement.anchor.y - old_placement.anchor.x + target->y - target->x)};
    EditFixtureWindow window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    desktop::Text text(ARK_TEST_FONT, "移动哪个去哪里中止从开始撤除铺呢");
    ui::Skin skin(sprites, text);
    const auto capture = [&](const char *name, bool show_ghost) {
        const auto before = state;
        for (int frame = 0; frame < 4; ++frame) {
            BeginDrawing();
            ClearBackground({145, 211, 247, 255});
            desktop::draw_world_scene(state, sprites, 1, nullptr, 1, &camera);
            const auto editing = desktop::world_edit_view(state, target);
            if (show_ghost)
                desktop::draw_world_build_preview(ghost, camera, 1, sprites);
            desktop::draw_world_edit_controls(editing, {720, 600}, skin, true,
                                              show_ghost ? "300G  Enter移动" : "移动完毕");
            DrawRectangle(0, 0, 720, 54, {52, 55, 41, 255});
            DrawText("Rendering callsite fixture / flags32 supplied / no world updates", 8, 8, 16,
                     WHITE);
            DrawText(
                TextFormat("%s / cash %lldG / old (%d,%d) -> new (%d,%d)",
                           show_ghost ? "mode7 ghost" : "committed move",
                           static_cast<long long>(state.scene.world.world.ai.accounting.funds()),
                           old_placement.anchor.x, old_placement.anchor.y, target->x, target->y),
                8, 31, 15, YELLOW);
            text.flush(1, {});
            EndDrawing();
        }
        const auto file = output / name;
        auto image = LoadImageFromScreen();
        const bool saved = image.data && ExportImage(image, file.string().c_str());
        if (image.data)
            UnloadImage(image);
        require(saved && std::filesystem::is_regular_file(file) && std::filesystem::file_size(file),
                "could not save real rendered capture");
        require(same_world_clock(state, before) &&
                    state.scene.scene_counter == before.scene.scene_counter &&
                    state.scene.random.draws() == before.scene.random.draws() &&
                    state.scene.world.world.ai.accounting.funds() ==
                        before.scene.world.world.ai.accounting.funds(),
                "drawing advanced the source counters or changed cash/random");
        std::cout << "Map-edit rendering callsite fixture / flags32 supplied: " << name
                  << " frames=4 cash=" << state.scene.world.world.ai.accounting.funds()
                  << " screenshot=" << file.string() << '\n';
    };
    capture("callsite-mode7-ghost.png", true);
    const auto moved =
        simulation::confirm_startup_world_edit(state, *target, old_placement.orientation);
    accepted(moved, "real source move could not commit its previously legal destination");
    require(moved.created && !state.scene.world.world.facilities.count(old_id) &&
                state.scene.world.world.facilities.at(*moved.created).placement.anchor == *target &&
                state.scene.world.world.facilities.at(*moved.created).placement.orientation ==
                    old_placement.orientation &&
                state.scene.world.world.ai.accounting.funds() == cash - 300 &&
                state.scene.random.draws() == draws && same_world_clock(state, initial),
            "source move did not preserve orientation/time/random and charge exactly300G");
    capture("callsite-moved-map.png", false);
}
} // namespace ark::test
