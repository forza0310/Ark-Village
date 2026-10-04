// Window/update orchestration only. Original page composition and mouse routing live in ui/.
#include "game_view.hpp"
#include "ark/app/game.hpp"
#include "ark/app/simulation_clock.hpp"
#include "ark/people/motion.hpp"
#include "character_animation.hpp"
#include "character_visibility.hpp"
#include "desktop_session.hpp"
#include "playability_probe.hpp"
#include "resources.hpp"
#include "scene.hpp"
#include "ui/controller.hpp"
#include "ui/hud.hpp"
#include "ui/pages.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>

namespace ark::desktop {
namespace {
struct Window {
    explicit Window(const app::LaunchOptions &options) {
        require_display();
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
        InitWindow(options.width, options.height,
                   options.ai_preview ? "Ark-Village - AI Preview" : "Ark-Village");
        if (!IsWindowReady())
            throw std::runtime_error("Cannot initialize raylib window");
        SetWindowMinSize(240, 256);
        SetExitKey(KEY_NULL);
        SetTargetFPS(0); // The event loop waits for the earliest logical or 60 FPS render deadline.
    }
    ~Window() { CloseWindow(); }
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
};
class Canvas {
  public:
    ~Canvas() {
        if (value.id)
            UnloadRenderTexture(value);
    }
    Canvas() = default;
    Canvas(const Canvas &) = delete;
    Canvas &operator=(const Canvas &) = delete;
    void resize(Extent size) {
        if (value.id && extent.width == size.width && extent.height == size.height)
            return;
        auto next = LoadRenderTexture(size.width, size.height);
        if (!next.id)
            throw std::runtime_error("Cannot create native framebuffer canvas");
        SetTextureFilter(next.texture, TEXTURE_FILTER_POINT);
        if (value.id)
            UnloadRenderTexture(value);
        value = next;
        extent = size;
    }
    RenderTexture2D value{};
    Extent extent{};
};
// Inspection arranges genuine model states but bypasses native input; it is rendering evidence.
void inspect(app::Game &game, ui::State &view, const std::string &page) {
    if (page.empty())
        return;
    if (page == "arrival" || page == "visitor" || page == "motion" || page == "ai") {
        game.set_paused(false);
        for (int i = 0; i < 420; ++i)
            game.update();
        if (page == "visitor" || page == "motion" || page == "ai") {
            game.acknowledge_talk();
            game.acknowledge_talk();
            game.finish_camera();
            view.page = page == "visitor" ? ui::Page::roster : ui::Page::village;
        }
    } else if (page == "menu") {
        view.page = ui::Page::menu;
    } else if (page == "detail" || page == "bonuses" || page == "equipment" || page == "booster") {
        const int definition = page == "equipment" ? 30 : page == "booster" ? 66 : 28;
        for (const auto &entry : game.state().facilities)
            if (entry.second.definition_id == definition) {
                view.detail = entry.first;
                view.page = ui::Page::facility;
                view.facility_page = page == "bonuses" ? 1 : 0;
                break;
            }
    } else {
        game.open_catalog();
        view.tab = page == "plants" ? 0 : page == "food" ? 2 : 1;
        if (page == "placement")
            ui::confirm(game, view);
    }
    if (page == "ai")
        return; // Arrange the real snapshot, then advance Game's AI each admitted window update.
    game.set_paused(true);
}
} // namespace
void run_game(const app::LaunchOptions &options, const std::filesystem::path &assets) {
    const Window window(options);
    Canvas canvas;
    Sprites sprites(assets);
    std::filesystem::path font = options.font;
#ifdef __APPLE__
    if (font.empty())
        font = "/System/Library/Fonts/Supplemental/Arial Unicode.ttf";
#endif
    Text text(font);
    const ui::Skin skin(sprites, text);
    const auto play = options.ai_preview ? app::PlayMode::ai_preview : app::PlayMode::startup;
    // Repeat only the diagnostic's random stream, preserving normal startup and rule draws.
    app::Game game(options.inspect_page == "ai" || options.verify_play ? 20261004U
                                                                       : std::random_device{}(),
                   play);
    game.set_paused(options.paused);
    const auto &data = app::startup_data();
    ui::State view;
    view.camera = {static_cast<float>(data.camera.x), static_cast<float>(data.camera.y)};
    view.zoom = options.zoom_percent / 100.0F;
    inspect(game, view,
            options.ai_preview && options.inspect_page.empty() ? "ai" : options.inspect_page);
    if (options.ai_preview)
        game.set_paused(options.paused);
    // A bounded, explicit-target rendering probe. It does not install an AI goal, mutate the
    // actor's domain state, charge a visit or infer that the first visitor chooses the inn.
    std::optional<people::Travel> inspected_travel;
    if (options.inspect_page == "motion") {
        for (const auto &[id, instance] : game.state().facilities)
            if (instance.definition_id == 28)
                inspected_travel =
                    people::plan_travel(game.route_map(), game.state().adventurer->cell,
                                        {instance.anchor, id, instance.definition_id});
        if (!inspected_travel)
            throw std::runtime_error("Inspection travel could not be planned");
        game.set_paused(options.paused);
    }
    int frames{};
    std::uint64_t motion_ticks{}, outer_updates{};
    app::SimulationClock simulation_clock(options.tick_rate);
    std::optional<std::int64_t> last_update_ms;
    std::int64_t minimum_gap_ms = std::numeric_limits<std::int64_t>::max();
    CharacterAnimation actor_animation;
    const auto animation_position = [&]() -> std::optional<world::WorldPosition> {
        if (!game.state().adventurer)
            return std::nullopt;
        return inspected_travel ? inspected_travel->position : game.state().adventurer->position;
    };
    const auto animation_tick = [&]() -> std::uint64_t {
        if (inspected_travel)
            return motion_ticks;
        if (game.life_state())
            return game.life_state()->rounds;
        return game.ai_state() ? game.ai_state()->rounds : game.state().simulation_steps;
    };
    actor_animation.observe(animation_position(), animation_tick());
    const auto can_simulate = [&]() {
        return !ui::blocks_world(view) && !game.state().paused &&
               game.state().mode == app::Mode::normal;
    };
    const auto advance_simulation = [&](double observed) {
        const auto due = simulation_clock.advance(observed, can_simulate());
        for (int n = 0; n < due; ++n) {
            if (!can_simulate()) {
                break;
            }
            if (inspected_travel) {
                for (int step = 0; step < view.speed; ++step) {
                    inspected_travel = people::advance_travel(game.route_map(), *inspected_travel,
                                                              game.state().adventurer->flags)
                                           .travel;
                    ++motion_ticks;
                    actor_animation.observe(animation_position(), animation_tick());
                }
            } else {
                game.update(view.speed);
                actor_animation.observe(animation_position(), animation_tick());
            }
            const auto observed_ms = static_cast<std::int64_t>(observed * 1000);
            if (last_update_ms)
                minimum_gap_ms = std::min(minimum_gap_ms, observed_ms - *last_update_ms);
            last_update_ms = observed_ms;
            ++outer_updates;
        }
        if (!can_simulate() && !simulation_clock.original_pacing())
            // Keep the observation so blocked fixed pacing has no deadline and cannot spin.
            simulation_clock.advance(observed, false);
    };
    const auto loop_start = GetTime();
    std::optional<PlayabilityProbe> playability;
    if (options.verify_play)
        playability.emplace();
    auto next_render_time = loop_start;
    const auto wait_for_work = [&]() {
        const auto now = GetTime();
        const auto remaining =
            std::min(next_render_time - now, simulation_clock.remaining_seconds(now));
        if (remaining > 0)
            WaitTime(remaining); // Yield to the OS; no busy wait or second simulation thread.
    };
    while (!WindowShouldClose() && (options.frames == 0 || frames < options.frames)) {
        const auto now = GetTime();
        if (playability && now - loop_start >= 60)
            break; // The diagnostic must terminate even if rendering misses its frame deadline.
        if (now < next_render_time) {
            advance_simulation(now);
            wait_for_work();
            continue;
        }
        next_render_time = now + 1.0 / 60;
        // Window points drive layout/input; framebuffer pixels drive rasterization. A Retina
        // window may have twice as many physical pixels on each axis. Never rasterize a zoomed
        // sprite into the small logical layout before enlarging it to the actual framebuffer.
        const auto extent = canvas_extent(GetScreenWidth(), GetScreenHeight());
        canvas.resize({GetRenderWidth(), GetRenderHeight()});
        const ui::Layout layout(extent);
        const auto destination = viewport(GetScreenWidth(), GetScreenHeight(), extent);
        const auto pixels = viewport(canvas.extent.width, canvas.extent.height, extent);
        const auto raster_camera = canvas_camera(pixels, extent);
        text.prepare(raster_camera.zoom);
        const auto mouse = logical_mouse(GetMousePosition(), destination, extent);
        if (IsKeyPressed(KEY_ESCAPE))
            ui::back(game, view);
        if (IsKeyPressed(KEY_ENTER))
            ui::confirm(game, view);
        if (IsKeyPressed(KEY_SPACE))
            ui::toggle_pause(game, view);
        if (IsKeyPressed(KEY_TAB))
            view.speed = view.speed == 1 ? 2 : 1;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
            ui::turn_facility_page(game, view);
        const float wheel = GetMouseWheelMove();
        if (game.state().mode == app::Mode::catalog && view.page == ui::Page::village)
            ui::scroll(view, -static_cast<int>(wheel));
        else if (view.page == ui::Page::facility)
            ui::scroll_sources(game, view, -static_cast<int>(wheel));
        else if (wheel && !ui::blocks_world(view) && mouse &&
                 CheckCollisionPointRec(*mouse, layout.scene) &&
                 (game.state().mode == app::Mode::normal ||
                  game.state().mode == app::Mode::placement))
            zoom_at(*mouse, extent, wheel, view.camera, view.zoom);
        if (mouse && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (game.state().mode == app::Mode::research_boundary &&
                CheckCollisionPointRec(*mouse,
                                       {layout.dialogue.x + 48, layout.dialogue.y + 66, 106, 29})) {
                game = app::Game(std::random_device{}(), play);
                simulation_clock.reset();
                last_update_ms.reset();
                motion_ticks = 0;
                actor_animation.reset();
                view = ui::State{};
                view.camera = {static_cast<float>(data.camera.x),
                               static_cast<float>(data.camera.y)};
                if (options.ai_preview) {
                    inspect(game, view, "ai");
                    game.set_paused(options.paused);
                }
            } else {
                ui::click(game, view, layout, *mouse);
            }
        }
        if (playability)
            playability->drive(game, view, layout);
        std::optional<world::Cell> hovered;
        if (!ui::blocks_world(view) && mouse && CheckCollisionPointRec(*mouse, layout.scene) &&
            (game.state().mode == app::Mode::normal || game.state().mode == app::Mode::placement)) {
            hovered = pick(*mouse, view.camera, data.map, extent, view.zoom);
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                const float scale = destination.width / extent.width;
                view.camera.x -= GetMouseDelta().x / (scale * view.zoom);
                view.camera.y += GetMouseDelta().y / (scale * view.zoom);
            }
        }
        // STARTUP mode6 does not run the world. Duration/easing remain desktop adapter policy.
        if (game.state().mode == app::Mode::camera) {
            const auto cell = game.state().adventurer->cell;
            const Vector2 target{30.0F * (cell.x + cell.y), 15.0F * (cell.y - cell.x) + 15};
            view.camera.x += (target.x - view.camera.x) * 0.15F;
            view.camera.y += (target.y - view.camera.y) * 0.15F;
            if (std::abs(target.x - view.camera.x) + std::abs(target.y - view.camera.y) < 0.5F) {
                view.camera = target;
                game.finish_camera();
            }
        }
        // Input is consumed once per render branch; logical work can also run between renders.
        advance_simulation(now);
        if (view.notice_frames)
            --view.notice_frames;
        if (inspected_travel && !game.state().adventurer)
            inspected_travel.reset(); // A manual new-game reset also clears the rendering probe.
        actor_animation.observe(animation_position(), animation_tick());
        BeginTextureMode(canvas.value);
        ClearBackground(Color{145, 211, 247, 255});
        BeginMode2D(raster_camera);
        // Scissor consumes framebuffer pixels, while the shared scene rectangle is logical.
        // Clip full sprites to the scene viewport after the boundary's submission gate.
        const auto &scene_clip = layout.scene_clip;
        const auto scene_left = static_cast<int>(
            std::floor(raster_camera.offset.x + scene_clip.x * raster_camera.zoom));
        const auto scene_top = static_cast<int>(
            std::floor(raster_camera.offset.y + scene_clip.y * raster_camera.zoom));
        const auto scene_right = static_cast<int>(std::ceil(
            raster_camera.offset.x + (scene_clip.x + scene_clip.width) * raster_camera.zoom));
        const auto scene_bottom = static_cast<int>(std::ceil(
            raster_camera.offset.y + (scene_clip.y + scene_clip.height) * raster_camera.zoom));
        BeginScissorMode(scene_left, scene_top, scene_right - scene_left, scene_bottom - scene_top);
        draw_scene(game, sprites, view.camera, extent, view.zoom,
                   inspected_travel
                       ? std::optional<world::WorldPosition>{inspected_travel->position}
                       : std::nullopt,
                   actor_animation.frame());
        draw_preview(game, view, layout, sprites, hovered);
        EndScissorMode();
        ui::draw_hud(game, view, layout, skin);
        ui::draw_pages(game, view, layout, skin);
        EndMode2D();
        text.flush(raster_camera.zoom, raster_camera.offset);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            canvas.value.texture,
            {0, 0, static_cast<float>(canvas.extent.width),
             -static_cast<float>(canvas.extent.height)},
            {0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
            {0, 0}, 0, WHITE);
        EndDrawing();
        ++frames;
        wait_for_work();
    }
    if (options.frames)
        std::cout << "Render: window=" << GetScreenWidth() << 'x' << GetScreenHeight()
                  << " framebuffer=" << GetRenderWidth() << 'x' << GetRenderHeight()
                  << " canvas=" << canvas.extent.width << 'x' << canvas.extent.height << '\n';
    if (options.frames)
        std::cout << "Simulation: pacing="
                  << (simulation_clock.original_pacing() ? "original47ms" : "fixed")
                  << " tick_rate_override=" << options.tick_rate
                  << " outer_updates=" << outer_updates
                  << " elapsed_seconds=" << GetTime() - loop_start
                  << " minimum_gap_ms=" << (outer_updates > 1 ? minimum_gap_ms : 0) << '\n';
    if (const auto *ai = game.ai_state()) {
        std::cout << "AI preview: rounds=" << ai->rounds << " position=" << ai->position.x << ','
                  << ai->position.z << " arrivals=" << ai->arrivals
                  << " completions=" << ai->completions << " funds=" << ai->accounting.funds()
                  << " error=" << static_cast<int>(game.ai_error())
                  << " actor_frame=" << actor_animation.frame()
                  << " actor_visible=" << character_visible(game, inspected_travel.has_value())
                  << '\n';
    }
    if (options.frames && !options.ai_preview) {
        const auto &state = game.state();
        const auto *life = game.life_state();
        std::cout << "Village: steps=" << state.simulation_steps << " date=" << state.calendar[0]
                  << ',' << state.calendar[1] << ',' << state.calendar[2] << ','
                  << state.calendar[3] << " arrival=" << state.event89_count
                  << " life_rounds=" << (life ? life->rounds : 0)
                  << " visits=" << (life ? life->arrivals : 0)
                  << " completions=" << (life ? life->completions : 0) << " funds=" << state.money
                  << " ledger_funds=" << state.accounting.funds()
                  << " error=" << (life ? static_cast<int>(life->error) : 0) << " pending_category="
                  << (life && life->pending_category ? *life->pending_category : -1)
                  << " actor_frame=" << actor_animation.frame()
                  << " actor_visible=" << character_visible(game, inspected_travel.has_value())
                  << '\n';
    }
    if (playability) {
        playability->report(std::cout, game);
        if (!playability->passed(game))
            throw std::runtime_error("Normal-game playability probe did not meet all assertions");
    }
    if (!options.screenshot.empty()) {
        if (frames != options.frames)
            throw std::runtime_error("Window closed before bounded capture");
        auto capture = LoadImageFromScreen();
        const bool saved = capture.data && ExportImage(capture, options.screenshot.c_str());
        if (capture.data)
            UnloadImage(capture);
        if (!saved)
            throw std::runtime_error("Cannot export window screenshot");
    }
}
} // namespace ark::desktop
