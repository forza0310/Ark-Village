// First-play adapter based on research/prototype/startup_view.cpp. Source projection, hit areas
// and easing are adapter policies; core commands never receive pixels, textures or frame times.
#include "game_view.hpp"
#include "ark/app/game.hpp"
#include "desktop_session.hpp"
#include "projection.hpp"
#include "resources.hpp"
#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace ark::desktop {
namespace {
const Color paper{250, 250, 247, 255}, ink{48, 44, 46, 255}, line{153, 161, 151, 255};
struct Window {
    explicit Window(const app::LaunchOptions &options) {
        require_display();
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(options.width, options.height, "Ark-Village");
        if (!IsWindowReady())
            throw std::runtime_error("Cannot initialize raylib window");
        SetWindowMinSize(canvas_width, canvas_height);
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);
    }
    ~Window() { CloseWindow(); }
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
};
struct Canvas {
    RenderTexture2D value{LoadRenderTexture(canvas_width, canvas_height)};
    Canvas() {
        if (!value.id)
            throw std::runtime_error("Cannot create logical canvas");
        SetTextureFilter(value.texture, TEXTURE_FILTER_POINT);
    }
    ~Canvas() { UnloadRenderTexture(value); }
    Canvas(const Canvas &) = delete;
    Canvas &operator=(const Canvas &) = delete;
};
class Panel {
  public:
    explicit Panel(const std::filesystem::path &root) {
        texture_ = LoadTexture((root / "ui/wnd_back.png").string().c_str());
        if (!texture_.id)
            throw std::runtime_error("Missing window background");
        SetTextureFilter(texture_, TEXTURE_FILTER_POINT);
    }
    ~Panel() { UnloadTexture(texture_); }
    Panel(const Panel &) = delete;
    Panel &operator=(const Panel &) = delete;
    void draw(Rectangle box) const {
        DrawTexturePro(
            texture_,
            {0, 0, static_cast<float>(texture_.width), static_cast<float>(texture_.height)}, box,
            {0, 0}, 0, WHITE);
        DrawRectangleLinesEx(box, 1, ink);
        if (box.height > 25)
            DrawRectangle(static_cast<int>(box.x + 4), static_cast<int>(box.y + 21),
                          static_cast<int>(box.width - 8), static_cast<int>(box.height - 25),
                          paper);
    }

  private:
    Texture2D texture_{};
};
const char *error_text(app::Error error) {
    switch (error) {
    case app::Error::none:
        return "";
    case app::Error::insufficient_funds:
        return "金钱不足";
    case app::Error::outside_map:
    case app::Error::outside_town:
        return "请选择街道内地域";
    case app::Error::occupied:
        return "有建筑物";
    case app::Error::unavailable:
        return "不可用";
    default:
        return "状态异常";
    }
}
struct View {
    Vector2 camera{};
    int tab{1}, speed{1};
    bool roster{};
    std::optional<world::Cell> selection;
    std::optional<facilities::InstanceId> detail;
    app::Error error{app::Error::none};
};
// Buildings and characters share depth ordering. Grass under source seeds is the research
// prototype's finite projection, not a certified original post-load terrain refresh.
void draw_scene(const app::Game &game, Sprites &sprites, Vector2 camera) {
    const auto &data = app::startup_data();
    struct Tile {
        std::string sprite;
        int frame{};
        Vector2 point{};
        Color tint{};
        bool farmer{};
    };
    std::vector<Tile> ground, objects;
    for (int y = 0; y < data.map.height; ++y)
        for (int x = 0; x < data.map.width; ++x) {
            const auto source = data.map.cells[data.map.index({x, y})];
            const bool occupied = game.facility_at({x, y}).has_value();
            ground.push_back({game.display(occupied ? 27 : source.display_id).sprite,
                              occupied ? 0 : source.variant, project({x, y}, camera), WHITE,
                              false});
        }
    for (const auto &entry : game.state().facilities) {
        const auto &v = entry.second;
        const auto &item = game.definition(v.definition_id);
        for (const auto &part : facilities::footprint(item.shape, v.orientation, v.anchor))
            objects.push_back({game.display(item.display_id).sprite, part.fragment,
                               project(part.cell, camera),
                               v.remaining_ticks ? Color{180, 180, 180, 255} : WHITE, false});
    }
    if (game.state().adventurer)
        objects.push_back(
            {"walk00.seb", 0, project(game.state().adventurer->cell, camera), WHITE, true});
    const auto draw = [&](std::vector<Tile> &tiles) {
        std::stable_sort(tiles.begin(), tiles.end(),
                         [](const auto &a, const auto &b) { return a.point.y < b.point.y; });
        for (const auto &tile : tiles)
            sprites.draw(tile.sprite, tile.frame, tile.point, tile.tint,
                         tile.farmer ? Sprites::Binding::farmer : Sprites::Binding::map);
    };
    draw(ground);
    draw(objects);
}
void outline(world::Cell cell, Vector2 camera, Color color) {
    const auto p = project(cell, camera);
    DrawLineEx({p.x - 30, p.y}, {p.x, p.y - 15}, 1, color);
    DrawLineEx({p.x, p.y - 15}, {p.x + 30, p.y}, 1, color);
    DrawLineEx({p.x + 30, p.y}, {p.x, p.y + 15}, 1, color);
    DrawLineEx({p.x, p.y + 15}, {p.x - 30, p.y}, 1, color);
}
void draw_interface(const app::Game &game, const View &view, const Text &text, const Panel &panel,
                    Sprites &sprites) {
    const auto &s = game.state();
    DrawRectangle(0, 0, 240, 40, paper);
    text.draw(TextFormat("%d年 %d月", s.calendar[0] + 1, s.calendar[1] + 1), 4, 4);
    text.draw(TextFormat("金币 %lldG", static_cast<long long>(s.money)), 112, 4);
    text.draw(TextFormat("点数 %d  人气 %d", s.points, s.popularity), 4, 24);
    text.draw(view.speed == 1 ? "1x" : "2x", 210, 24);
    DrawRectangle(0, 306, 240, 24, paper);
    DrawLine(0, 306, 240, 306, line);
    if (s.mode == app::Mode::normal && !view.roster && !view.detail) {
        text.draw("建设", 20, 313);
        text.draw("冒险者", 91, 313);
        text.draw(s.paused ? "继续" : "暂停", 184, 313);
    }
    if (s.mode == app::Mode::catalog) {
        panel.draw({5, 48, 230, 247});
        text.draw("建设", 104, 52);
        const char *tabs[] = {"道路植物", "商店", "饮食"};
        for (int i = 0; i < 3; ++i) {
            if (i == view.tab)
                DrawRectangle(9 + i * 74, 74, 72, 21, Color{255, 153, 55, 255});
            text.draw(tabs[i], static_cast<float>(14 + i * 74), 79);
        }
        int row{};
        for (const auto &item : app::startup_data().definitions) {
            if (item.tab != view.tab)
                continue;
            const float y = static_cast<float>(103 + row++ * 32);
            sprites.draw(game.display(item.display_id).sprite, 0, {26, y + 19}, WHITE,
                         Sprites::Binding::map, 0.4F);
            text.draw(item.name, 57, y + 4, (item.kind == 6 || item.kind == 13) ? GRAY : ink);
            text.draw(std::to_string(item.price) + "G", 179, y + 4,
                      s.money < item.price ? RED : ink);
            DrawLine(12, static_cast<int>(y + 30), 228, static_cast<int>(y + 30), line);
        }
        text.draw("返回", 191, 313);
    } else if (s.mode == app::Mode::placement) {
        text.draw("旋转", 13, 313);
        text.draw("确定", 95, 313);
        text.draw("返回", 185, 313);
        panel.draw({14, 44, 212, 23});
        text.draw(game.definition(*s.selection).name, 24, 49);
    } else if (s.mode == app::Mode::tutorial) {
        panel.draw({6, 190, 228, 110});
        text.draw("冒险者到访", 81, 195);
        sprites.draw("chara_hishoko01.seb", 0, {22, 190}, WHITE, Sprites::Binding::secretary);
        text.paragraph(app::startup_data().first_talk.at(s.talk_line), 14, 221, 206);
        text.draw("确定", 189, 281);
    } else if (s.mode == app::Mode::research_boundary) {
        panel.draw({14, 115, 212, 78});
        text.draw("本轮结束", 94, 121);
        text.draw("重新开始", 94, 162);
    }
    if (view.roster) {
        panel.draw({8, 70, 224, 210});
        text.draw("到访冒险者", 83, 76);
        if (!s.adventurer)
            text.draw("尚无到访者", 83, 120);
        else {
            const auto &a = *s.adventurer;
            sprites.draw("walk00.seb", 0, {31, 129}, WHITE, Sprites::Binding::farmer);
            text.draw(a.name, 50, 108);
            text.draw(TextFormat("农家  等级 %d", a.level), 50, 132);
            text.draw(TextFormat("体力 %d  攻击 %d", a.combat[0], a.combat[1]), 24, 165);
            text.draw(TextFormat("防御 %d  魔法 %d", a.combat[2], a.combat[3]), 24, 189);
        }
        text.draw("返回", 191, 313);
    } else if (view.detail) {
        const auto &v = s.facilities.at(*view.detail);
        const auto &item = game.definition(v.definition_id);
        panel.draw({10, 80, 220, 185});
        text.draw("设施", 106, 86);
        text.draw(item.name, 35, 122);
        sprites.draw(game.display(item.display_id).sprite, v.orientation, {50, 191}, WHITE,
                     Sprites::Binding::map, 0.8F);
        text.draw(TextFormat("等级 %d", s.definition_progress.at(item.id).level), 120, 155);
        if (v.remaining_ticks)
            text.draw(TextFormat("施工 %d", v.remaining_ticks), 120, 182);
        text.draw("返回", 191, 313);
    }
    if (view.error != app::Error::none) {
        DrawRectangle(6, 285, 228, 18, paper);
        text.draw(error_text(view.error), 12, 288, RED);
    }
}
} // namespace

void run_game(const app::LaunchOptions &options, const std::filesystem::path &assets) {
    const Window window(options);
    const Canvas canvas;
    Sprites sprites(assets);
    std::filesystem::path font = options.font;
#ifdef __APPLE__
    if (font.empty())
        font = "/System/Library/Fonts/Supplemental/Arial Unicode.ttf";
#endif
    Text text(font);
    const Panel panel(assets);
    app::Game game(std::random_device{}());
    game.set_paused(options.paused);
    const auto &data = app::startup_data();
    View view;
    view.camera = {static_cast<float>(data.camera.x), static_cast<float>(data.camera.y)};
    // As in research/prototype --inspect-page: prepare a real model state for bounded visual
    // checks. This bypasses input and is explicitly not mouse or normal-new-game acceptance.
    if (!options.inspect_page.empty()) {
        if (options.inspect_page == "arrival" || options.inspect_page == "visitor") {
            game.set_paused(false);
            for (int i = 0; i < 420; ++i)
                game.update();
            if (options.inspect_page == "visitor") {
                game.acknowledge_talk();
                game.acknowledge_talk();
                game.finish_camera();
                view.roster = true;
            }
        } else {
            game.open_catalog();
            view.tab = options.inspect_page == "plants" ? 0
                       : options.inspect_page == "food" ? 2
                                                        : 1;
        }
        game.set_paused(true);
    }
    int frames{};
    while (!WindowShouldClose() && (options.frames == 0 || frames < options.frames)) {
        const auto destination = viewport(GetScreenWidth(), GetScreenHeight());
        const auto mouse = logical_mouse(GetMousePosition(), destination);
        const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const auto hit = [&](Rectangle box) {
            return mouse && click && CheckCollisionPointRec(*mouse, box);
        };
        const auto mode = game.state().mode;
        if (IsKeyPressed(KEY_ESCAPE)) {
            game.cancel();
            view.selection.reset();
            view.detail.reset();
            view.roster = false;
            view.error = app::Error::none;
        }
        std::optional<world::Cell> hovered;
        if (!view.roster && !view.detail &&
            (mode == app::Mode::normal || mode == app::Mode::placement)) {
            if (mouse && mouse->y > 40 && mouse->y < 306)
                hovered = pick(*mouse, view.camera, data.map);
            if (mouse && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                const float scale = destination.width / canvas_width;
                view.camera.x -= GetMouseDelta().x / scale;
                view.camera.y += GetMouseDelta().y / scale;
            }
        }
        if (view.roster || view.detail) {
            if (hit({174, 306, 65, 24})) {
                view.roster = false;
                view.detail.reset();
            }
        } else if (mode == app::Mode::normal) {
            if (hit({0, 306, 80, 24}))
                view.error = game.open_catalog();
            else if (hit({80, 306, 80, 24}))
                view.roster = true;
            else if (hit({160, 306, 80, 24}))
                game.set_paused(!game.state().paused);
            else if (hit({202, 20, 38, 20}))
                view.speed = view.speed == 1 ? 2 : 1;
            else if (click && hovered)
                view.detail = game.facility_at(*hovered);
        } else if (mode == app::Mode::catalog) {
            if (hit({174, 306, 65, 24})) {
                game.cancel();
                view.error = app::Error::none;
            }
            for (int i = 0; i < 3; ++i)
                if (hit({static_cast<float>(9 + i * 74), 74, 72, 21})) {
                    view.tab = i;
                    view.error = app::Error::none;
                }
            int row{};
            for (const auto &item : data.definitions)
                if (item.tab == view.tab) {
                    if (hit({12, static_cast<float>(103 + row * 32), 216, 30}))
                        view.error = game.select(item.id);
                    ++row;
                }
        } else if (mode == app::Mode::placement) {
            if (hit({160, 306, 80, 24})) {
                game.cancel();
                view.selection.reset();
                view.error = app::Error::none;
            } else if (hit({0, 306, 80, 24}))
                view.error = game.rotate();
            else if (hit({80, 306, 80, 24}) && view.selection) {
                view.error = game.confirm(*view.selection);
                if (view.error == app::Error::none)
                    view.selection.reset();
            } else if (click && hovered) {
                view.selection = hovered;
                view.error = game.preview(*hovered);
            }
        } else if (mode == app::Mode::tutorial &&
                   (hit({177, 275, 54, 24}) || IsKeyPressed(KEY_ENTER))) {
            view.error = game.acknowledge_talk();
        } else if (mode == app::Mode::research_boundary && hit({76, 152, 102, 32})) {
            game = app::Game(std::random_device{}());
            view = View{};
            view.camera = {static_cast<float>(data.camera.x), static_cast<float>(data.camera.y)};
        }
        // Camera completion is this adapter's convergence policy, not an original frame count.
        if (game.state().mode == app::Mode::camera) {
            const auto cell = game.state().adventurer->cell;
            const Vector2 target{30.0F * (cell.x + cell.y), 15.0F * (cell.y - cell.x) + 15};
            view.camera.x += (target.x - view.camera.x) * 0.15F;
            view.camera.y += (target.y - view.camera.y) * 0.15F;
            if (std::abs(target.x - view.camera.x) + std::abs(target.y - view.camera.y) < 0.5F) {
                view.camera = target;
                game.finish_camera();
            }
        } else if (!view.roster && !view.detail)
            game.update(view.speed);
        BeginTextureMode(canvas.value);
        ClearBackground(Color{83, 141, 77, 255});
        draw_scene(game, sprites, view.camera);
        const auto preview = view.selection ? view.selection : hovered;
        if (game.state().mode == app::Mode::placement && preview) {
            const auto &item = game.definition(*game.state().selection);
            const Color tint = game.preview(*preview) == app::Error::none ? YELLOW : RED;
            for (const auto &part :
                 facilities::footprint(item.shape, game.state().orientation, *preview))
                outline(part.cell, view.camera, tint);
        }
        draw_interface(game, view, text, panel, sprites);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(Color{35, 38, 37, 255});
        DrawTexturePro(canvas.value.texture, {0, 0, canvas_width, -canvas_height}, destination,
                       {0, 0}, 0, WHITE);
        EndDrawing();
        ++frames;
    }
    if (!options.screenshot.empty()) {
        if (frames != options.frames)
            throw std::runtime_error("Window closed before bounded capture");
        auto image = LoadImageFromScreen();
        const bool saved = image.data && ExportImage(image, options.screenshot.c_str());
        if (image.data)
            UnloadImage(image);
        if (!saved)
            throw std::runtime_error("Cannot export window screenshot");
    }
}
} // namespace ark::desktop
