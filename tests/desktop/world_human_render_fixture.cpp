// Explicit renderer fixture, never a natural-world or mastery-reward acceptance result.
// This case is invoked manually and is deliberately absent from standard CTest registration.
#include "../../src/desktop/resources/resources.hpp"
#include "../support/world_fixture.hpp"
#include "../../src/desktop/ui/common/skin.hpp"
#include "../../src/desktop/ui/actors/world_human.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace ark::test {
namespace {
class FixtureWindow {
  public:
    FixtureWindow() {
        InitWindow(720, 600, "Ark-Village UI fixture / synthetic - raw70 mastery");
        if (!IsWindowReady())
            throw std::runtime_error("Synthetic UI fixture could not create a window");
        SetTargetFPS(60);
    }
    ~FixtureWindow() { CloseWindow(); }
    FixtureWindow(const FixtureWindow &) = delete;
    FixtureWindow &operator=(const FixtureWindow &) = delete;
};
} // namespace

void world_human_render_fixture() {
    namespace ui = desktop::ui;
    namespace rules = simulation::rules;
    const std::filesystem::path output{ARK_TEST_OUTPUT};
    std::filesystem::create_directories(output);
    auto state = initial_world();
    const int human = state.rules->humans.front().identity;
    auto &growth = state.scene.world.world.ai.growth.at(human);
    // Synthetic display input only: no experience, mastery reward or profession consumer runs.
    growth.definition.profession_levels.at(growth.definition.current_profession) = 10;
    rules::WorldScriptPage page;
    page.id = 901;
    page.kind = rules::WorldScriptPageKind::raw_page;
    page.legacy_page = 70;
    page.lifecycle = 1;
    state.scripts.pages.push_back(page);
    state.page_human_bindings[page.id] = human;
    state.human_pages_initialized.insert(page.id);
    state.page_counters[page.id] = 40;
    state.human_page_selections[page.id] = 1; // Show the actual keep-current option in phase2.
    const auto untouched = state;
    const auto cash = state.scene.world.world.ai.accounting.funds();
    const auto draws = state.scene.random.draws();

    FixtureWindow window;
    desktop::Sprites sprites(ARK_TEST_ASSETS);
    std::string glyphs = "职业大师体力力量灵活结实魔力运气魔法转职保持现状确定";
    glyphs += state.rules->humans.at(human).name;
    glyphs += state.rules->jobs.at(growth.definition.current_profession).name;
    desktop::Text text(ARK_TEST_FONT, glyphs);
    ui::Skin skin(sprites, text);
    const auto layout = ui::world_human_layout({720, 600});
    for (int phase = 0; phase < 3; ++phase) {
        // Explicit fixture selection; this is not elapsed source animation time.
        state.page_phases[page.id] = phase;
        const auto view = ui::world_human_view(state, page);
        if (!view.initialized || view.details.level != 10 || view.phase != phase)
            throw std::runtime_error("Synthetic raw70 fixture did not produce its requested view");
        for (int frame = 0; frame < 8; ++frame) {
            if (WindowShouldClose())
                throw std::runtime_error("Synthetic UI fixture was closed before all captures");
            BeginDrawing();
            ClearBackground({145, 211, 247, 255});
            DrawRectangle(0, 0, 720, 62, {52, 55, 41, 255});
            DrawText("UI fixture / synthetic - renderer only", 14, 10, 18, WHITE);
            DrawText(TextFormat("raw70 phase %d / counter 40 / no simulation or rewards", phase),
                     14, 36, 16, YELLOW);
            ui::draw_world_human(view, layout, skin, true);
            text.flush(1, {});
            EndDrawing();
        }
        const auto file = output / ("synthetic-raw70-phase" + std::to_string(phase) + ".png");
        auto capture = LoadImageFromScreen();
        const bool saved = capture.data && ExportImage(capture, file.string().c_str());
        UnloadImage(capture);
        if (!saved || !std::filesystem::is_regular_file(file) ||
            std::filesystem::file_size(file) == 0)
            throw std::runtime_error("Synthetic UI fixture failed to write its screenshot");
        if (!same_world_clock(state, untouched) ||
            state.scene.world.world.ai.accounting.funds() != cash ||
            state.scene.random.draws() != draws || state.page_counters.at(page.id) != 40 ||
            state.scene.world.world.ai.growth.at(human).definition.extra !=
                untouched.scene.world.world.ai.growth.at(human).definition.extra)
            throw std::runtime_error(
                "Rendering synthetic mastery unexpectedly changed world state");
        std::cout << "UI fixture / synthetic raw70 phase=" << phase
                  << " frames=8 screenshot=" << file.string() << '\n';
    }
}
} // namespace ark::test
