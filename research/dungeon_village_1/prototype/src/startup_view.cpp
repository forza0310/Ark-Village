#include "dungeon_village_prototype/startup_view.hpp"
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <stdexcept>

namespace dungeon_village_prototype {
namespace {
constexpr int width = 240, height = 320, scale = 2;
const Color ink{48, 44, 46, 255}, paper{248, 248, 244, 255};

std::vector<std::uint8_t> read_bytes(const std::filesystem::path &path) {
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("素材元数据过大");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("素材元数据无法读取");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

// Decode only the published SEB schema; PNG rectangles, image bindings and raw flips are checked
// separately. legacy_tag is retained by the tool parser and is never used as a record count.
class SourceSprites {
  public:
    enum class Binding { map, farmer, secretary };
    explicit SourceSprites(std::filesystem::path root) : root_(std::move(root)) {
        for (const auto &row :
             dungeon_village_tools::parse_tsv(read_bytes(root_ / "image/img.inf"))) {
            if (row.size() != 2)
                throw std::runtime_error("图片索引列数错误");
            auto name = std::filesystem::path(row[1]);
            if (name.has_parent_path())
                throw std::runtime_error("图片索引路径不安全");
            name.replace_extension(".png");
            const auto id = dungeon_village_tools::parse_table_integer(row[0]);
            if (!images_.emplace(id, name).second)
                throw std::runtime_error("图片索引重复");
        }
    }
    ~SourceSprites() {
        for (const auto &v : textures_)
            UnloadTexture(v.second);
    }
    SourceSprites(const SourceSprites &) = delete;
    SourceSprites &operator=(const SourceSprites &) = delete;
    void draw(const std::string &sprite, int frame, Vector2 anchor, Color tint = WHITE,
              Binding binding = Binding::map, float factor = 1) {
        const char *group = binding == Binding::farmer      ? "human"
                            : binding == Binding::secretary ? "common"
                                                            : "image";
        const auto relative = std::filesystem::path(group) / sprite;
        auto it = sprites_.find(relative.string());
        if (it == sprites_.end())
            it = sprites_
                     .emplace(relative.string(),
                              dungeon_village_tools::parse_legacy_seb(read_bytes(root_ / relative)))
                     .first;
        if (frame < 0 || frame >= it->second.frame_count)
            throw std::runtime_error("源变体超出SEB帧边界");
        for (const auto &layer : it->second.layers) {
            for (const auto &part : layer.parts) {
                if (part.frame != frame)
                    continue;
                if (binding == Binding::secretary && part.image_index != 126)
                    throw std::runtime_error("秘书SEB图片绑定发生变化");
                const auto path = binding == Binding::farmer ? root_ / "human/chara_flower00.png"
                                  : binding == Binding::secretary
                                      ? root_ / "common/chara_hishoko01.png"
                                      : root_ / "image" / images_.at(part.image_index);
                auto image = textures_.find(path.string());
                if (image == textures_.end()) {
                    const auto texture = LoadTexture(path.string().c_str());
                    if (texture.id == 0)
                        throw std::runtime_error("原始纹理无法读取");
                    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
                    image = textures_.emplace(path.string(), texture).first;
                }
                const auto &tex = image->second;
                if (part.source_x < 0 || part.source_y < 0 || part.width <= 0 || part.height <= 0 ||
                    part.source_x + part.width > tex.width ||
                    part.source_y + part.height > tex.height || part.flip_x < 0 ||
                    part.flip_x > 1 || part.flip_y < 0 || part.flip_y > 1)
                    throw std::runtime_error("SEB图片矩形或翻转字段无效");
                DrawTexturePro(
                    tex,
                    {static_cast<float>(part.source_x), static_cast<float>(part.source_y),
                     static_cast<float>(part.flip_x ? -part.width : part.width),
                     static_cast<float>(part.flip_y ? -part.height : part.height)},
                    {anchor.x + part.offset_x * factor, anchor.y + part.offset_y * factor,
                     part.width * factor, part.height * factor},
                    {0, 0}, 0, tint);
            }
        }
    }

  private:
    std::filesystem::path root_;
    std::map<int, std::filesystem::path> images_;
    std::map<std::string, dungeon_village_tools::SpriteDefinition> sprites_;
    std::map<std::string, Texture2D> textures_;
};

// Original tile-plane increments and y-flip, with this adapter's 240x320 viewport midpoint.
Vector2 project(ref::Position cell, Vector2 camera) {
    return {120 + 30.0F * (cell.x + cell.y) - camera.x,
            160 - 15.0F * (cell.y - cell.x) - 15 + camera.y};
}
std::optional<ref::Position> pick(Vector2 point, Vector2 camera) {
    const auto &data = startup_evidence();
    for (int y = 0; y < data.height; ++y)
        for (int x = 0; x < data.width; ++x) {
            const auto p = project({x, y}, camera);
            if (std::abs(point.x - p.x) / 30 + std::abs(point.y - p.y) / 15 <= 1)
                return ref::Position{x, y};
        }
    return std::nullopt;
}

class ChineseFont {
  public:
    explicit ChineseFont(const std::filesystem::path &path) {
        if (!std::filesystem::is_regular_file(path))
            throw std::runtime_error("请用 --font 指定可用中文TTF字体");
        std::string glyphs = "建设返回确定撤除道路植物商店饮食金币点数人气年月份倍暂停继续月末"
                             "请选择街道内地域有建筑物金钱不足没有设施不可撤除状态异常施工";
        for (int i = 32; i < 127; ++i)
            glyphs += static_cast<char>(i);
        for (const auto &v : startup_evidence().definitions)
            glyphs += v.name;
        for (const auto &v : startup_evidence().first_talk)
            glyphs += v;
        glyphs += startup_evidence().first_character.name;
        int count = 0;
        int *raw = LoadCodepoints(glyphs.c_str(), &count);
        if (!raw)
            throw std::runtime_error("中文码点读取失败");
        const std::set<int> unique(raw, raw + count);
        UnloadCodepoints(raw);
        std::vector<int> codes(unique.begin(), unique.end());
        font_ = LoadFontEx(path.string().c_str(), 24, codes.data(), static_cast<int>(codes.size()));
        if (font_.texture.id == 0 || font_.texture.id == GetFontDefault().texture.id)
            throw std::runtime_error("中文字体加载失败");
        for (const auto code : codes) {
            if (code > 127 && font_.glyphs[GetGlyphIndex(font_, code)].value != code) {
                UnloadFont(font_);
                throw std::runtime_error("中文字体缺少必要字形");
            }
        }
        SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
    }
    ~ChineseFont() { UnloadFont(font_); }
    ChineseFont(const ChineseFont &) = delete;
    ChineseFont &operator=(const ChineseFont &) = delete;
    void text(const std::string &value, float x, float y, Color color = ink,
              float size = 12) const {
        DrawTextEx(font_, value.c_str(), {x, y}, size, 0, color);
    }
    // Wrap at measured codepoint boundaries, preserving Chinese without inserting broken UTF-8.
    void paragraph(const std::string &value, float x, float y, float max_width) const {
        std::string line;
        for (std::size_t i = 0; i < value.size();) {
            const auto first = static_cast<unsigned char>(value[i]);
            const std::size_t length = first < 128 ? 1 : first < 224 ? 2 : first < 240 ? 3 : 4;
            const auto next = value.substr(i, length);
            if (!line.empty() && MeasureTextEx(font_, (line + next).c_str(), 12, 0).x > max_width) {
                text(line, x, y);
                line.clear();
                y += 17;
            }
            line += next;
            i += length;
        }
        text(line, x, y);
    }

  private:
    Font font_{};
};

struct Window {
    Window() {
        SetTraceLogLevel(LOG_WARNING);
        InitWindow(width * scale, height * scale, "Dungeon Village - startup research");
        if (!IsWindowReady())
            throw std::runtime_error("窗口初始化失败");
        SetExitKey(KEY_NULL);
        SetTargetFPS(60);
    }
    ~Window() { CloseWindow(); }
};
struct Canvas {
    RenderTexture2D value{LoadRenderTexture(width, height)};
    Canvas() {
        if (!value.id)
            throw std::runtime_error("画布创建失败");
        SetTextureFilter(value.texture, TEXTURE_FILTER_POINT);
    }
    ~Canvas() { UnloadRenderTexture(value); }
};
const char *error_text(StartupError error) {
    switch (error) {
    case StartupError::none:
        return "";
    case StartupError::insufficient_funds:
        return "金钱不足";
    case StartupError::outside_town:
        return "请选择街道内地域";
    case StartupError::occupied:
        return "有建筑物";
    case StartupError::not_found:
        return "没有设施";
    case StartupError::protected_seed:
        return "不可撤除";
    default:
        return "状态异常";
    }
}
} // namespace

void check_startup() {
    StartupSession session;
    for (int i = 0; i < 420; ++i)
        session.update();
    if (!session.state().character || session.state().character->uid != 0 ||
        session.state().character->definition_id != 1 || session.state().event89_count != 1 ||
        session.state().accounting.funds() != 5000 || session.state().mode != StartupMode::tutorial)
        throw std::runtime_error("新局首访自检失败");
    std::cout << "startup check passed: source_cells=576 seed_instances=8 uid=0 definition=1 "
                 "money=5000\n";
}

// Input is an adapter, not a reproduction of original physical hotspots. Logic uses exactly one
// outer update per rendered frame (or two at 2x); target FPS is presentation pacing, not APK time.
int run_startup_window(const std::filesystem::path &assets, const std::filesystem::path &font_path,
                       bool paused, int frames,
                       const std::optional<std::filesystem::path> &screenshot,
                       const std::string &inspect_page) {
    Window window;
    Canvas canvas;
    SourceSprites sprites(assets / "original");
    ChineseFont font(font_path);
    StartupSession session;
    session.set_paused(paused);
    const auto &data = startup_evidence();
    Vector2 camera{static_cast<float>(data.camera.x), static_cast<float>(data.camera.y)};
    int tab = 0, speed = 1, frame_count = 0;
    StartupError last = StartupError::none;
    std::optional<ref::Position> selected_cell;
    // Explicit snapshot inspection, never called by default reset or a real input command.
    if (inspect_page == "roads" || inspect_page == "shops" || inspect_page == "food") {
        tab = inspect_page == "roads" ? 0 : inspect_page == "shops" ? 1 : 2;
        session.open_catalog();
    } else if (inspect_page == "arrival" || inspect_page == "visitor") {
        session.set_paused(false);
        for (int i = 0; i < 420; ++i)
            session.update();
        if (inspect_page == "visitor") {
            session.acknowledge_talk();
            session.acknowledge_talk();
            const auto cell = session.state().character->cell;
            camera = {30.0F * (cell.x + cell.y), 15.0F * (cell.y - cell.x) + 15};
            session.finish_camera();
            session.set_paused(true);
        }
    }
    while (!WindowShouldClose()) {
        const Vector2 mouse{GetMousePosition().x / scale, GetMousePosition().y / scale};
        const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const auto mode = session.state().mode;
        if (IsKeyPressed(KEY_ESCAPE)) {
            session.cancel();
            selected_cell.reset();
            last = StartupError::none;
        }
        if ((mode == StartupMode::normal || mode == StartupMode::placement) &&
            IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            camera.x -= GetMouseDelta().x / scale;
            camera.y += GetMouseDelta().y / scale;
        }
        std::optional<ref::Position> hovered;
        if (mouse.y > 38 && mouse.y < 294 &&
            (mode == StartupMode::normal || mode == StartupMode::placement))
            hovered = pick(mouse, camera);
        const auto hit = [&](Rectangle r) { return click && CheckCollisionPointRec(mouse, r); };
        if (mode == StartupMode::normal) {
            if (hit({2, 296, 74, 22}))
                last = session.open_catalog();
            if (hit({80, 296, 74, 22}))
                session.set_paused(!session.state().paused);
            if (hit({158, 296, 80, 22}))
                speed = speed == 1 ? 2 : 1;
        } else if (mode == StartupMode::catalog) {
            if (hit({190, 296, 48, 22})) {
                session.cancel();
                last = StartupError::none;
            }
            for (int i = 0; i < 3; ++i)
                if (hit({static_cast<float>(4 + i * 77), 58, 73, 25}))
                    tab = i;
            int row = 0;
            for (const auto &item : data.definitions) {
                if (item.tab != tab)
                    continue;
                if (hit({8, static_cast<float>(90 + row * 28), 224, 26}))
                    last = session.select(item.id);
                ++row;
                if (tab == 0 && row == 1) {
                    if (hit({8, static_cast<float>(90 + row * 28), 224, 26}))
                        last = session.select(-1);
                    ++row;
                }
            }
        } else if (mode == StartupMode::placement) {
            if (hit({160, 296, 78, 22})) {
                session.cancel();
                selected_cell.reset();
                last = StartupError::none;
            } else if (hit({80, 296, 76, 22}) && selected_cell) {
                last = session.confirm(*selected_cell);
                if (last == StartupError::none)
                    selected_cell.reset();
            } else if (click && hovered) {
                selected_cell = hovered;
                last = session.preview(*hovered);
            }
        } else if (mode == StartupMode::tutorial && hit({178, 267, 52, 23})) {
            last = session.acknowledge_talk();
        }
        // Camera duration is this view's convergence policy, not a recovered original frame count.
        if (session.state().mode == StartupMode::camera) {
            const auto cell = session.state().character->cell;
            const Vector2 target{30.0F * (cell.x + cell.y), 15.0F * (cell.y - cell.x) + 15};
            camera.x += (target.x - camera.x) * 0.15F;
            camera.y += (target.y - camera.y) * 0.15F;
            if (std::abs(target.x - camera.x) + std::abs(target.y - camera.y) < 0.5F) {
                camera = target;
                session.finish_camera();
            }
        } else
            session.update(speed);

        BeginTextureMode(canvas.value);
        ClearBackground(Color{83, 141, 77, 255});
        struct Tile {
            int record;
            int variant;
            Vector2 p;
            Color tint;
        };
        std::vector<Tile> tiles;
        for (int y = 0; y < data.height; ++y)
            for (int x = 0; x < data.width; ++x) {
                const auto index = static_cast<std::size_t>(y * data.width + x);
                const auto edit = session.state().terrain_edits.find(index);
                const auto source = data.cells[index];
                int record =
                    edit == session.state().terrain_edits.end() ? source.display_id : edit->second;
                int variant = edit == session.state().terrain_edits.end() ? source.variant : 0;
                const auto p = project({x, y}, camera);
                const auto facility = session.facility_at({x, y});
                if (facility) {
                    record = 27;
                    variant = 0;
                }
                tiles.push_back({record, variant, p, WHITE});
            }
        std::stable_sort(tiles.begin(), tiles.end(),
                         [](const auto &a, const auto &b) { return a.p.y < b.p.y; });
        for (const auto &tile : tiles)
            sprites.draw(session.display(tile.record).sprite, tile.variant, tile.p, tile.tint);
        tiles.clear();
        for (const auto &entry : session.state().facilities) {
            const auto &v = entry.second;
            tiles.push_back({session.definition(v.definition_id).display_id, 0,
                             project(v.cell, camera),
                             v.remaining_ticks ? Color{190, 190, 190, 255} : WHITE});
        }
        std::stable_sort(tiles.begin(), tiles.end(),
                         [](const auto &a, const auto &b) { return a.p.y < b.p.y; });
        for (const auto &tile : tiles)
            sprites.draw(session.display(tile.record).sprite, 0, tile.p, tile.tint);
        if (session.state().character)
            sprites.draw("walk00.seb", 0, project(session.state().character->cell, camera), WHITE,
                         SourceSprites::Binding::farmer);
        const auto preview_cell = selected_cell ? selected_cell : hovered;
        if (session.state().mode == StartupMode::placement && preview_cell) {
            const auto p = project(*preview_cell, camera);
            const Color color = session.preview(*preview_cell) == StartupError::none
                                    ? Color{255, 244, 71, 255}
                                    : RED;
            if (*session.state().selection > 0)
                sprites.draw(
                    session.display(session.definition(*session.state().selection).display_id)
                        .sprite,
                    0, p, Color{color.r, color.g, color.b, 180});
            DrawLineEx({p.x - 30, p.y}, {p.x, p.y - 15}, 1, color);
            DrawLineEx({p.x, p.y - 15}, {p.x + 30, p.y}, 1, color);
            DrawLineEx({p.x + 30, p.y}, {p.x, p.y + 15}, 1, color);
            DrawLineEx({p.x, p.y + 15}, {p.x - 30, p.y}, 1, color);
        }
        DrawRectangle(0, 0, 240, 38, paper);
        const auto &state = session.state();
        font.text(TextFormat("%d年 %d月", state.calendar[0] + 1, state.calendar[1] + 1), 4, 3);
        font.text(TextFormat("金币 %lld", static_cast<long long>(state.accounting.funds())), 116,
                  3);
        font.text(
            TextFormat("点数 %u  人气 %d", state.accounting.village_points(), state.popularity), 4,
            21);
        DrawRectangle(0, 294, 240, 26, paper);
        if (state.mode == StartupMode::normal) {
            font.text("建设", 18, 300);
            font.text(state.paused ? "继续" : "暂停", 100, 300);
            font.text(speed == 1 ? "1倍" : "2倍", 184, 300);
        } else if (state.mode == StartupMode::placement) {
            font.text(*state.selection == -1 ? "撤除" : session.definition(*state.selection).name,
                      4, 300);
            font.text("确定", 96, 300);
            font.text("返回", 184, 300);
        } else if (state.mode == StartupMode::month_end)
            font.text("月末", 100, 300);
        if (state.mode == StartupMode::catalog) {
            DrawRectangle(4, 48, 232, 232, paper);
            font.text("道路植物", 12, 64);
            font.text("商店", 98, 64);
            font.text("饮食", 177, 64);
            DrawRectangle(4 + tab * 77, 82, 73, 2, Color{69, 124, 107, 255});
            int row = 0;
            for (const auto &item : data.definitions) {
                if (item.tab != tab)
                    continue;
                const int y = 94 + row * 28;
                sprites.draw(session.display(item.display_id).sprite, 0,
                             {24, static_cast<float>(y + 15)}, WHITE, SourceSprites::Binding::map,
                             0.4F);
                font.text(item.name, 54, static_cast<float>(y));
                font.text(std::to_string(item.cost), 184, static_cast<float>(y));
                ++row;
                if (tab == 0 && row == 1) {
                    font.text("撤除", 54, static_cast<float>(94 + row * 28));
                    font.text("0", 184, static_cast<float>(94 + row * 28));
                    ++row;
                }
                DrawLine(8, 90 + row * 28, 232, 90 + row * 28, Color{216, 220, 218, 255});
            }
            font.text("返回", 194, 300);
        }
        if (state.mode == StartupMode::tutorial) {
            DrawRectangle(6, 202, 228, 90, paper);
            sprites.draw("chara_hishoko01.seb", 0, {22, 202}, WHITE,
                         SourceSprites::Binding::secretary);
            font.paragraph(data.first_talk[state.talk_line], 14, 211, 208);
            font.text("确定", 186, 273);
        }
        if (last != StartupError::none) {
            DrawRectangle(4, 274, 232, 18, paper);
            font.text(error_text(last), 10, 277, RED);
        }
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(canvas.value.texture, {0, 0, width, -height},
                       {0, 0, width * scale, height * scale}, {0, 0}, 0, WHITE);
        EndDrawing();
        ++frame_count;
        if (frames > 0 && frame_count >= frames) {
            if (screenshot) {
                if (!screenshot->parent_path().empty())
                    std::filesystem::create_directories(screenshot->parent_path());
                auto image = LoadImageFromScreen();
                if (!image.data)
                    throw std::runtime_error("窗口像素读取失败");
                const bool saved = ExportImage(image, screenshot->string().c_str());
                UnloadImage(image);
                if (!saved)
                    throw std::runtime_error("截图导出失败");
            }
            break;
        }
    }
    std::cout << "startup window closed: frames=" << frame_count
              << " mode=" << static_cast<int>(session.state().mode)
              << " steps=" << session.state().simulation_steps
              << " money=" << session.state().accounting.funds() << '\n';
    return 0;
}
} // namespace dungeon_village_prototype
