// Explicit presentation inputs; this manual window is not a natural level-up or reward test.
#include "resources.hpp"
#include "support/world_fixture.hpp"
#include "world_combat_visuals.hpp"
#include "world_overlay_render.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace ark::test {
void world_combat_render_fixture() {
    namespace rules = simulation::rules;
    simulation::StartupWorldRuntimeState state;
    auto &ai = state.scene.world.world.ai;
    rules::BattleActorRecord actor;
    actor.id = {1};
    actor.definition = 7;
    actor.kind = rules::ActorKind::human;
    ai.battle.actors.emplace(actor.id, actor);
    ai.contexts.emplace(actor.id, rules::RewardActorContext{});
    rules::RewardHumanDefinition growth;
    growth.definition.profession_levels = {1};
    growth.pending = {10, -1};
    growth.experience = 6;
    growth.notice_pending = true;
    ai.growth.emplace(7, growth);
    ai.professions.resize(1);
    const auto before = state;
    std::array<desktop::OverlayPlan, 4> plans;
    constexpr std::array<int, 4> counts{0, 6, 55, 72};
    for (std::size_t i = 0; i < counts.size(); ++i) {
        auto observed = state;
        observed.scene.world.world.ai.contexts.at({1}).effects.display = {
            {24, counts[i], 0, 10, 0, 0}};
        plans[i] = desktop::world_actor_combat_visuals(observed, {1});
    }
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(800, 400, "Ark-Village pending badge / synthetic presentation fixture");
    if (!IsWindowReady())
        throw std::runtime_error("Pending badge fixture could not create its window");
    struct WindowGuard {
        ~WindowGuard() { CloseWindow(); }
    } window;
    {
        desktop::Sprites sprites(ARK_TEST_ASSETS);
        for (int frame = 0; frame < 4; ++frame) {
            BeginDrawing();
            ClearBackground({52, 55, 41, 255});
            DrawText("Synthetic P/cd24 fixture / no world updates or reward consumer", 10, 12, 18,
                     WHITE);
            for (std::size_t i = 0; i < plans.size(); ++i) {
                const float x = static_cast<float>(i * 200);
                DrawRectangle(static_cast<int>(x + 5), 55, 190, 285, {145, 211, 247, 255});
                DrawText(TextFormat("cd24=%d / P=true", counts[i]), static_cast<int>(x + 12), 70,
                         16, BLACK);
                DrawCircle(static_cast<int>(x + 100), 260, 4, RED);
                desktop::draw_world_overlay(plans[i], sprites, {x + 100, 260}, 4);
            }
            DrawText("Source frames 0 / 1 / 1 / expired; red dot = ordinary body anchor", 10, 363,
                     17, WHITE);
            EndDrawing();
        }
        const std::filesystem::path output =
            std::filesystem::path(ARK_TEST_OUTPUT) / "synthetic-pending-level-badge.png";
        std::filesystem::create_directories(output.parent_path());
        auto capture = LoadImageFromScreen();
        const bool saved = capture.data && ExportImage(capture, output.string().c_str());
        if (capture.data)
            UnloadImage(capture);
        if (!saved || !same_world_clock(state, before) ||
            state.scene.random.draws() != before.scene.random.draws() ||
            !ai.growth.at(7).notice_pending || ai.growth.at(7).experience != 6 ||
            ai.growth.at(7).pending.amount != 10 || !ai.contexts.at({1}).effects.display.empty())
            throw std::runtime_error("Pending badge capture failed or mutated fixture Owner");
        std::cout << "PASS synthetic pending-level fixture: " << output.string() << '\n';
    }
}
} // namespace ark::test
