// Window/update orchestration only. Original page composition and mouse routing live in ui/.
#include "game_view.hpp"
#include "ark/app/game.hpp"
#include "desktop_session.hpp"
#include "resources.hpp"
#include "scene.hpp"
#include "ui/controller.hpp"
#include "ui/hud.hpp"
#include "ui/pages.hpp"
#include <cmath>
#include <random>
#include <stdexcept>

namespace ark::desktop {
namespace {
struct Window {
    explicit Window(const app::LaunchOptions &options) {
        require_display();
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(options.width, options.height, "Ark-Village");
        if (!IsWindowReady())
            throw std::runtime_error("Cannot initialize raylib window");
        SetWindowMinSize(240, 256);
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);
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
            throw std::runtime_error("Cannot create logical canvas");
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
    if (page == "arrival" || page == "visitor") {
        game.set_paused(false);
        for (int i = 0; i < 420; ++i)
            game.update();
        if (page == "visitor") {
            game.acknowledge_talk();
            game.acknowledge_talk();
            game.finish_camera();
            view.page = ui::Page::roster;
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
    app::Game game(std::random_device{}());
    game.set_paused(options.paused);
    const auto &data = app::startup_data();
    ui::State view;
    view.camera = {static_cast<float>(data.camera.x), static_cast<float>(data.camera.y)};
    view.zoom = options.zoom_percent / 100.0F;
    inspect(game, view, options.inspect_page);
    int frames{};
    while (!WindowShouldClose() && (options.frames == 0 || frames < options.frames)) {
        canvas.resize(canvas_extent(GetScreenWidth(), GetScreenHeight()));
        const ui::Layout layout(canvas.extent);
        const auto destination = viewport(GetScreenWidth(), GetScreenHeight(), canvas.extent);
        const auto mouse = logical_mouse(GetMousePosition(), destination, canvas.extent);
        if (IsKeyPressed(KEY_ESCAPE))
            ui::back(game, view);
        if (IsKeyPressed(KEY_ENTER))
            ui::confirm(game, view);
        if (IsKeyPressed(KEY_SPACE) && game.state().mode == app::Mode::normal)
            game.set_paused(!game.state().paused);
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
            zoom_at(*mouse, canvas.extent, wheel, view.camera, view.zoom);
        if (mouse && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (game.state().mode == app::Mode::research_boundary &&
                CheckCollisionPointRec(*mouse,
                                       {layout.dialogue.x + 48, layout.dialogue.y + 66, 106, 29})) {
                game = app::Game(std::random_device{}());
                view = ui::State{};
                view.camera = {static_cast<float>(data.camera.x),
                               static_cast<float>(data.camera.y)};
            } else {
                ui::click(game, view, layout, *mouse);
            }
        }
        std::optional<world::Cell> hovered;
        if (!ui::blocks_world(view) && mouse && CheckCollisionPointRec(*mouse, layout.scene) &&
            (game.state().mode == app::Mode::normal || game.state().mode == app::Mode::placement)) {
            hovered = pick(*mouse, view.camera, data.map, canvas.extent, view.zoom);
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                const float scale = destination.width / canvas.extent.width;
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
        } else if (!ui::blocks_world(view)) {
            game.update(view.speed);
        }
        if (view.notice_frames)
            --view.notice_frames;
        BeginTextureMode(canvas.value);
        ClearBackground(Color{145, 211, 247, 255});
        draw_scene(game, sprites, view.camera, canvas.extent, view.zoom);
        draw_preview(game, view, layout, sprites, hovered);
        ui::draw_hud(game, view, layout, skin);
        ui::draw_pages(game, view, layout, skin);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.value.texture,
                       {0, 0, static_cast<float>(canvas.extent.width),
                        -static_cast<float>(canvas.extent.height)},
                       destination, {0, 0}, 0, WHITE);
        text.flush(destination.width / canvas.extent.width, {destination.x, destination.y});
        EndDrawing();
        ++frames;
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
