// raylib adapter for inputs, projection and RAII GPU resources; Village owns all gameplay state.
// Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_prototype/asset_manifest.hpp"
#include "dungeon_village_prototype/village.hpp"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using namespace dungeon_village_prototype;
using namespace dungeon_village_reference;
constexpr int canvas_size = 240;
constexpr int scale = 3;
constexpr int grid_size = 7;
enum class Tool { inn, pair, cafe, move, rotate, remove };

struct Options {
    std::filesystem::path assets;
    std::filesystem::path table;
    std::optional<std::filesystem::path> screenshot;
    bool demo{};
    bool check{};
    int frames{};
    bool paused{};
    FacilityOrientation orientation{FacilityOrientation::first};
};

// Screenshots require a bounded window run; validation mode never opens the desktop.
Options parse_options(int argc, char **argv) {
    const auto directory = std::filesystem::absolute(argv[0]).parent_path();
    Options options{directory / "assets", directory / "data/tenantData.txt", std::nullopt, false,
                    false};
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--asset-root" && i + 1 < argc)
            options.assets = argv[++i];
        else if (arg == "--data-file" && i + 1 < argc)
            options.table = argv[++i];
        else if (arg == "--screenshot" && i + 1 < argc)
            options.screenshot = argv[++i];
        else if (arg == "--demo")
            options.demo = true;
        else if (arg == "--check")
            options.check = true;
        else if (arg == "--paused")
            options.paused = true;
        else if ((arg == "--frames" || arg == "--orientation") && i + 1 < argc) {
            const std::string text = argv[++i];
            std::size_t consumed = 0;
            const int value = std::stoi(text, &consumed);
            if (consumed != text.size())
                throw std::invalid_argument("验收参数必须为整数");
            if (arg == "--frames") {
                if (value < 1 || value > 3600)
                    throw std::invalid_argument("帧数必须在 1–3600 内");
                options.frames = value;
            } else {
                if (value < 0 || value > 1)
                    throw std::invalid_argument("朝向只能为 0/1");
                options.orientation = static_cast<FacilityOrientation>(value);
            }
        } else
            throw std::invalid_argument("未知或不完整参数: " + arg);
    }
    if (options.screenshot && ((!options.demo && options.frames == 0) || options.check))
        throw std::invalid_argument("截图参数只能用于有界窗口验收");
    if (options.demo && (options.paused || options.frames != 0))
        throw std::invalid_argument("自主演示不能暂停或使用独立帧数上限");
    return options;
}

// This projection belongs to the 7x7 demo, not the recovered original map/camera transform.
Vector2 center(Position cell) {
    return {120.0F + static_cast<float>(cell.x - cell.y) * 30.0F,
            46.0F + static_cast<float>(cell.x + cell.y) * 14.5F};
}

// Hit-test the same fixed diamond used by the adapter; never pass screen coordinates to Village.
std::optional<Position> pick(Vector2 point) {
    for (int y = 0; y < grid_size; ++y) {
        for (int x = 0; x < grid_size; ++x) {
            const auto p = center({x, y});
            if (std::abs(point.x - p.x) / 30.0F + std::abs(point.y - p.y) / 14.5F <= 1.0F)
                return Position{x, y};
        }
    }
    return std::nullopt;
}

// Deliberate fixture: synthetic roads, three facilities and two actors, not a new-game save.
Village make_village(const std::filesystem::path &table, FacilityOrientation orientation) {
    LegacyMap map{grid_size, grid_size, std::vector<LegacyMapCell>(grid_size * grid_size)};
    for (int y = 0; y < grid_size; ++y) {
        for (int x = 0; x < grid_size; ++x) {
            if (x == 3 || y == 3)
                map.cells[static_cast<std::size_t>(y * grid_size + x)] = {3, RouteCategory::road,
                                                                          std::nullopt};
        }
    }
    Village village(load_prototype_catalog(table), std::move(map));
    for (const auto &item :
         std::array<std::pair<int, Position>, 3>{{{28, {2, 3}}, {29, {4, 3}}, {36, {3, 2}}}}) {
        if (village.place(item.first, item.second, orientation).error != VillageError::none)
            throw std::runtime_error("初始设施无法创建");
    }
    if (village.add_actor({1}, {0, 3}) != VillageError::none ||
        village.add_actor({2}, {6, 3}) != VillageError::none)
        throw std::runtime_error("初始人物无法创建");
    return village;
}

void construction_check(Village &village) {
    const auto extra = village.place(29, {5, 5}, FacilityOrientation::first);
    if (!extra.instance ||
        village.relocate(*extra.instance, {5, 5}, FacilityOrientation::second) !=
            VillageError::none ||
        village.demolish(*extra.instance) != VillageError::none ||
        village.place(36, {4, 3}, FacilityOrientation::first).error !=
            VillageError::invalid_placement)
        throw std::runtime_error("多格建移转拆自检失败");
}

// Smoke-check the integration without a window; it cannot certify real input or original UI parity.
void bounded_check(Village &village) {
    construction_check(village);
    if (village.advance(60000) != VillageError::none)
        throw std::runtime_error("自主模拟自检失败");
    std::uint64_t completed = 0;
    for (const auto &entry : village.state().actors)
        completed += entry.second.completed;
    if (completed == 0 || village.state().accounting.reports().empty())
        throw std::runtime_error("自主访问或周期报表自检未闭环");
    std::cout << "R2 prototype check passed: completed=" << completed
              << " periods=" << village.state().accounting.reports().size()
              << " funds=" << village.state().accounting.funds() << '\n';
}

struct Window {
    Window() {
        InitWindow(canvas_size * scale, canvas_size * scale, "Dungeon Village R2");
        if (!IsWindowReady())
            throw std::runtime_error("窗口初始化失败，需要可用桌面会话");
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);
    }
    ~Window() { CloseWindow(); }
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
};

class Textures {
  public:
    explicit Textures(AssetManifest manifest) : manifest_(std::move(manifest)) {
        try {
            for (const auto &entry : manifest_) {
                const auto image = LoadTexture(entry.second.path.string().c_str());
                if (image.id == 0U)
                    throw std::runtime_error("纹理读取失败: " + entry.first);
                textures_.emplace(entry.first, image);
                const auto &frame = entry.second.frame;
                if (frame.x > image.width || frame.y > image.height ||
                    frame.width > image.width - frame.x || frame.height > image.height - frame.y)
                    throw std::runtime_error("纹理帧越界: " + entry.first);
                SetTextureFilter(image, TEXTURE_FILTER_POINT);
            }
        } catch (...) {
            release();
            throw;
        }
    }
    ~Textures() { release(); }
    Textures(const Textures &) = delete;
    Textures &operator=(const Textures &) = delete;
    void draw(const std::string &key, Vector2 anchor, Color tint = WHITE, bool flip = false) const {
        const auto &definition = manifest_.at(key);
        const auto &f = definition.frame;
        DrawTextureRec(textures_.at(key),
                       {static_cast<float>(f.x), static_cast<float>(f.y),
                        static_cast<float>(flip ? -f.width : f.width),
                        static_cast<float>(f.height)},
                       {anchor.x - static_cast<float>(definition.anchor.x),
                        anchor.y - static_cast<float>(definition.anchor.y)},
                       tint);
    }
    void icon(const std::string &key, ::Rectangle destination) const {
        const auto &f = manifest_.at(key).frame;
        DrawTexturePro(textures_.at(key),
                       {static_cast<float>(f.x), static_cast<float>(f.y),
                        static_cast<float>(f.width), static_cast<float>(f.height)},
                       destination, {0, 0}, 0, WHITE);
    }

  private:
    void release() {
        for (const auto &entry : textures_)
            UnloadTexture(entry.second);
        textures_.clear();
    }
    AssetManifest manifest_;
    std::map<std::string, Texture2D> textures_;
};

struct Canvas {
    RenderTexture2D target{LoadRenderTexture(canvas_size, canvas_size)};
    Canvas() {
        if (target.id == 0U)
            throw std::runtime_error("画布创建失败");
        SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    }
    ~Canvas() { UnloadRenderTexture(target); }
    Canvas(const Canvas &) = delete;
    Canvas &operator=(const Canvas &) = delete;
};

std::string fragment_key(int definition, int fragment) {
    if (definition == 28)
        return "building.inn";
    if (definition == 36)
        return "building.cafe";
    return fragment < 2 ? "building.inn.pair.front" : "building.inn.pair.back";
}

// Translate controls into aggregate commands and draw immutable state; resource lifetimes end here.
int run_window(const Options &options, Village village, AssetManifest manifest) {
    Window window;
    Textures textures(std::move(manifest));
    Canvas canvas;
    Tool tool = Tool::inn;
    FacilityOrientation orientation = options.orientation;
    std::optional<BuildingId> moving;
    VillageError last_error = VillageError::none;
    Vector2 camera{};
    double remainder = 0;
    const auto deadline = GetTime() + 15.0;
    bool captured = false;
    bool finished = false;
    bool previous_using = false;
    bool previous_report = false;
    int frames = 0;
    village.set_paused(options.paused);
    if (options.demo)
        construction_check(village);
    const std::array<const char *, 7> names{"Inn",    "Pair Inn", "Cafe", "Move",
                                            "Rotate", "Remove",   "Pause"};
    while (!WindowShouldClose()) {
        if (options.demo && GetTime() > deadline)
            throw std::runtime_error("窗口演示超时");
        const auto raw_mouse = GetMousePosition();
        const Vector2 mouse{raw_mouse.x / scale, raw_mouse.y / scale};
        if (!options.demo && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            const auto delta = GetMouseDelta();
            camera.x = std::clamp(camera.x - delta.x / scale, -120.0F, 120.0F);
            camera.y = std::clamp(camera.y - delta.y / scale, -20.0F, 60.0F);
        }
        if (IsKeyPressed(KEY_ESCAPE))
            moving.reset();
        std::optional<Position> hovered;
        if (mouse.y >= 42)
            hovered = pick({mouse.x + camera.x, mouse.y + camera.y});
        int hovered_button = -1;
        for (int i = 0; i < 7; ++i) {
            if (CheckCollisionPointRec(mouse, {static_cast<float>(2 + i * 29), 2, 26, 24}))
                hovered_button = i;
        }
        if (!options.demo && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (hovered_button >= 0) {
                if (hovered_button == 6)
                    village.set_paused(!village.state().paused);
                else {
                    tool = static_cast<Tool>(hovered_button);
                    if (tool == Tool::rotate)
                        orientation = orientation == FacilityOrientation::first
                                          ? FacilityOrientation::second
                                          : FacilityOrientation::first;
                    moving.reset();
                }
                last_error = VillageError::none;
            } else if (hovered) {
                const auto hit = village.facility_at(*hovered);
                if (tool == Tool::inn || tool == Tool::pair || tool == Tool::cafe) {
                    const int id = tool == Tool::inn ? 28 : (tool == Tool::pair ? 29 : 36);
                    last_error = village.place(id, *hovered, orientation).error;
                } else if (tool == Tool::remove && hit)
                    last_error = village.demolish(*hit);
                else if (tool == Tool::move) {
                    if (!moving) {
                        moving = hit;
                        if (moving)
                            orientation = village.state().facilities.at(*moving).orientation;
                    } else {
                        last_error = village.relocate(*moving, *hovered, orientation);
                        if (last_error == VillageError::none)
                            moving.reset();
                    }
                } else if (tool == Tool::rotate && hit) {
                    const auto &p = village.state().facilities.at(*hit);
                    last_error = village.relocate(*hit, p.anchor,
                                                  p.orientation == FacilityOrientation::first
                                                      ? FacilityOrientation::second
                                                      : FacilityOrientation::first);
                }
            }
            std::cout << "input tool=" << static_cast<int>(tool)
                      << " result=" << static_cast<int>(last_error)
                      << " facilities=" << village.state().facilities.size()
                      << " funds=" << village.state().accounting.funds()
                      << " paused=" << village.state().paused << " moving=" << moving.has_value()
                      << std::endl;
        }
        remainder += std::min(1000.0, static_cast<double>(GetFrameTime()) * 1000.0);
        const int elapsed = options.demo ? 500 : static_cast<int>(remainder);
        remainder -= static_cast<int>(remainder);
        if (village.advance(elapsed) != VillageError::none)
            throw std::runtime_error("原型模拟推进失败");
        BeginTextureMode(canvas.target);
        ClearBackground(Color{67, 120, 84, 255});
        for (int sum = 0; sum <= 12; ++sum) {
            for (int y = 0; y < grid_size; ++y) {
                const int x = sum - y;
                if (x < 0 || x >= grid_size)
                    continue;
                const auto p = center({x, y});
                textures.draw(x == 3 || y == 3 ? "terrain.road" : "terrain.grass",
                              {p.x - camera.x, p.y - camera.y});
            }
        }
        struct RenderItem {
            std::string key;
            Vector2 position;
            Color tint;
            bool flip;
        };
        std::vector<RenderItem> items;
        for (const auto &entry : village.state().facilities) {
            const auto &p = entry.second;
            for (const auto &cell :
                 facility_footprint(p.shape, p.orientation, p.anchor, grid_size, grid_size).cells) {
                items.push_back({fragment_key(p.definition_id, cell.fragment_index),
                                 center(cell.position),
                                 moving == std::optional<BuildingId>{entry.first} ? YELLOW : WHITE,
                                 cell.fragment_index % 2 == 1});
            }
        }
        for (const auto &entry : village.state().actors) {
            const auto &actor = entry.second;
            auto p = center(actor.cell);
            if (actor.activity == ActivityState::travelling && actor.cursor < actor.path.size()) {
                const auto next = center(actor.path[actor.cursor]);
                const float fraction =
                    static_cast<float>(actor.phase_ticks) / village.config().step_ticks;
                p.x += (next.x - p.x) * fraction;
                p.y += (next.y - p.y) * fraction;
            }
            items.push_back({"character.night", p,
                             actor.activity == ActivityState::in_use ? YELLOW : WHITE, false});
        }
        std::stable_sort(items.begin(), items.end(),
                         [](const auto &a, const auto &b) { return a.position.y < b.position.y; });
        for (const auto &item : items)
            textures.draw(item.key, {item.position.x - camera.x, item.position.y - camera.y},
                          item.tint, item.flip);
        if (hovered && (tool == Tool::inn || tool == Tool::pair || tool == Tool::cafe ||
                        (tool == Tool::move && moving))) {
            const int id = moving ? village.state().facilities.at(*moving).definition_id
                                  : (tool == Tool::inn ? 28 : (tool == Tool::pair ? 29 : 36));
            auto preview = village;
            const auto error = moving ? preview.relocate(*moving, *hovered, orientation)
                                      : preview.place(id, *hovered, orientation).error;
            for (const auto &cell : facility_footprint(village.definition(id).shape, orientation,
                                                       *hovered, grid_size, grid_size)
                                        .cells) {
                const auto p = center(cell.position);
                textures.draw(fragment_key(id, cell.fragment_index),
                              {p.x - camera.x, p.y - camera.y},
                              error == VillageError::none ? Color{160, 255, 160, 180}
                                                          : Color{255, 100, 100, 180},
                              cell.fragment_index % 2 == 1);
            }
        }
        DrawRectangle(0, 0, 240, 42, Color{28, 34, 37, 255});
        for (int i = 0; i < 7; ++i) {
            const ::Rectangle r{static_cast<float>(2 + i * 29), 2, 26, 24};
            const bool selected = i == 6 ? village.state().paused : static_cast<int>(tool) == i;
            DrawRectangleRec(r, selected ? Color{236, 190, 72, 255} : Color{74, 82, 84, 255});
            if (i < 3)
                textures.icon(i == 0 ? "building.inn"
                                     : (i == 1 ? "building.inn.pair.front" : "building.cafe"),
                              {r.x + 3, 4, 20, 20});
            else if (i == 3) {
                DrawLine(static_cast<int>(r.x + 5), 14, static_cast<int>(r.x + 21), 14, WHITE);
                DrawLine(static_cast<int>(r.x + 13), 6, static_cast<int>(r.x + 13), 22, WHITE);
            } else if (i == 4) {
                DrawCircleLines(static_cast<int>(r.x + 13), 14, 7, WHITE);
                DrawTriangle({r.x + 20, 6}, {r.x + 20, 13}, {r.x + 14, 9}, WHITE);
            } else if (i == 5) {
                DrawRectangleLines(static_cast<int>(r.x + 8), 10, 10, 12, WHITE);
                DrawLine(static_cast<int>(r.x + 5), 8, static_cast<int>(r.x + 21), 8, WHITE);
            } else if (village.state().paused)
                DrawTriangle({r.x + 8, 6}, {r.x + 8, 22}, {r.x + 20, 14}, WHITE);
            else {
                DrawRectangle(static_cast<int>(r.x + 8), 7, 4, 14, WHITE);
                DrawRectangle(static_cast<int>(r.x + 16), 7, 4, 14, WHITE);
            }
        }
        DrawText(TextFormat("$%lld  M%llu",
                            static_cast<long long>(village.state().accounting.funds()),
                            static_cast<unsigned long long>(village.state().period)),
                 4, 30, 10, WHITE);
        if (!village.state().accounting.reports().empty()) {
            const auto net = village.state().accounting.reports().rbegin()->second.displayed_net;
            DrawText(TextFormat("Net %lld", static_cast<long long>(net)), 138, 30, 10, WHITE);
        }
        if (hovered_button >= 0) {
            DrawRectangle(4, 220, 110, 16, Color{28, 34, 37, 240});
            DrawText(names[static_cast<std::size_t>(hovered_button)], 8, 224, 10, WHITE);
        }
        EndTextureMode();
        SetWindowTitle(TextFormat("Dungeon Village R2 | %s | facing %d | result %d",
                                  names[static_cast<std::size_t>(tool)],
                                  static_cast<int>(orientation), static_cast<int>(last_error)));
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.target.texture, {0, 0, 240, -240}, {0, 0, 720, 720}, {0, 0}, 0,
                       WHITE);
        EndDrawing();
        bool using_facility = false;
        std::uint64_t completed = 0;
        for (const auto &entry : village.state().actors) {
            using_facility = using_facility || entry.second.activity == ActivityState::in_use;
            completed += entry.second.completed;
        }
        ++frames;
        const bool final_frame = options.frames > 0 && frames >= options.frames;
        const bool has_report = !village.state().accounting.reports().empty();
        // Cocoa swaps buffers at EndDrawing; require both frames to show the verified state.
        if (options.screenshot && ((options.demo && using_facility && previous_using &&
                                    has_report && previous_report && !captured) ||
                                   final_frame)) {
            if (!options.screenshot->parent_path().empty())
                std::filesystem::create_directories(options.screenshot->parent_path());
            auto screenshot = LoadImageFromScreen();
            if (screenshot.data == nullptr)
                throw std::runtime_error("无法读取窗口像素");
            const bool exported = ExportImage(screenshot, options.screenshot->string().c_str());
            UnloadImage(screenshot);
            if (!exported || !std::filesystem::is_regular_file(*options.screenshot))
                throw std::runtime_error("截图未生成");
            captured = true;
        }
        previous_using = using_facility;
        previous_report = has_report;
        if (final_frame) {
            finished = true;
            break;
        }
        if (options.demo && completed > 0 && !village.state().accounting.reports().empty() &&
            (!options.screenshot || captured)) {
            finished = true;
            std::cout << "R2 window demo passed\n";
            break;
        }
    }
    if (options.demo && !finished)
        throw std::runtime_error("窗口演示提前关闭");
    return 0;
}
} // namespace

int main(int argc, char **argv) {
    try {
        const auto options = parse_options(argc, argv);
        const auto manifest = load_asset_manifest(options.assets);
        for (const auto *key :
             {"terrain.grass", "terrain.road", "building.inn", "building.cafe",
              "building.inn.pair.front", "building.inn.pair.back", "character.night"})
            if (manifest.count(key) == 0)
                throw std::runtime_error(std::string("缺少素材键: ") + key);
        auto village = make_village(options.table, options.orientation);
        if (options.check) {
            bounded_check(village);
            return 0;
        }
        return run_window(options, std::move(village), manifest);
    } catch (const std::exception &error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
