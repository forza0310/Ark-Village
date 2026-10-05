#include "dungeon_village_prototype/startup_view.hpp"
#include "dungeon_village_prototype/road_render.hpp"
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_visuals.hpp"
#include "dungeon_village_reference/world_notices.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include "dungeon_village_tools/table.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
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
    enum class Binding { map, farmer, secretary, human, monster, common };
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
        for (const auto *group : {"human", "monster", "common"}) {
            for (const auto &row :
                 dungeon_village_tools::parse_tsv(read_bytes(root_ / group / "img.inf"))) {
                if (row.size() != 2)
                    throw std::runtime_error("人物图片索引列数错误");
                std::filesystem::path path(row[1]);
                if (path.has_parent_path())
                    throw std::runtime_error("人物图片索引路径不安全");
                path.replace_extension(".png");
                if (!actor_images_[group]
                         .emplace(dungeon_village_tools::parse_table_integer(row[0]), path)
                         .second)
                    throw std::runtime_error("人物图片索引重复");
            }
            for (const auto &row :
                 dungeon_village_tools::parse_tsv(read_bytes(root_ / group / "seb.inf"))) {
                if (row.size() != 1 || std::filesystem::path(row[0]).has_parent_path())
                    throw std::runtime_error("人物精灵索引错误");
                actor_sprites_[group].push_back(row[0]);
            }
        }
    }
    ~SourceSprites() {
        for (const auto &v : textures_)
            UnloadTexture(v.second);
    }
    SourceSprites(const SourceSprites &) = delete;
    SourceSprites &operator=(const SourceSprites &) = delete;
    void draw(const std::string &sprite, int frame, Vector2 anchor, Color tint = WHITE,
              Binding binding = Binding::map, float factor = 1, int image_override = -1) {
        const char *group = binding == Binding::farmer || binding == Binding::human ? "human"
                            : binding == Binding::monster                           ? "monster"
                            : binding == Binding::secretary || binding == Binding::common ? "common"
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
                const auto path =
                    image_override >= 0 ? root_ / group / actor_images_.at(group).at(image_override)
                    : binding == Binding::farmer    ? root_ / "human/chara_flower00.png"
                    : binding == Binding::secretary ? root_ / "common/chara_hishoko01.png"
                    : binding == Binding::common
                        ? root_ / "common" / actor_images_.at("common").at(part.image_index)
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
    void actor(bool monster, int sprite_index, int image_index, int frame, Vector2 anchor) {
        const char *group = monster ? "monster" : "human";
        draw(actor_sprites_.at(group).at(static_cast<std::size_t>(sprite_index)), frame, anchor,
             WHITE, monster ? Binding::monster : Binding::human, 1, image_index);
    }
    void portrait(const StartupPortrait &plan, Vector2 position) {
        BeginScissorMode(static_cast<int>(position.x), static_cast<int>(position.y),
                         plan.clip_width, plan.clip_height);
        actor(false, plan.sprite, plan.image, plan.frame,
              {position.x + plan.anchor_x, position.y + plan.anchor_y});
        EndScissorMode();
    }
    void crop(const std::filesystem::path &relative, Rectangle source, Vector2 position) {
        const auto path = root_ / relative;
        auto found = textures_.find(path.string());
        if (found == textures_.end()) {
            const auto texture = LoadTexture(path.string().c_str());
            if (!texture.id)
                throw std::runtime_error("裁剪资源无法读取");
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
            found = textures_.emplace(path.string(), texture).first;
        }
        if (source.x < 0 || source.y < 0 || source.width <= 0 || source.height <= 0 ||
            source.x + source.width > found->second.width ||
            source.y + source.height > found->second.height)
            throw std::runtime_error("裁剪资源矩形越界");
        DrawTextureRec(found->second, source, position, WHITE);
    }
    void image(const std::filesystem::path &relative, Vector2 position) {
        if (relative.is_absolute() || relative.string().find("..") != std::string::npos)
            throw std::runtime_error("整图资源路径无效");
        const auto path = root_ / relative;
        auto found = textures_.find(path.string());
        if (found == textures_.end()) {
            const auto texture = LoadTexture(path.string().c_str());
            if (!texture.id)
                throw std::runtime_error("整图资源无法读取");
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);
            found = textures_.emplace(path.string(), texture).first;
        }
        DrawTextureV(found->second, position, WHITE);
    }

  private:
    std::filesystem::path root_;
    std::map<int, std::filesystem::path> images_;
    std::map<std::string, std::map<int, std::filesystem::path>> actor_images_;
    std::map<std::string, std::vector<std::string>> actor_sprites_;
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
    explicit ChineseFont(const std::filesystem::path &path, const std::string &extra = {}) {
        if (!std::filesystem::is_regular_file(path))
            throw std::runtime_error("请用 --font 指定可用中文TTF字体");
        std::string glyphs = "本月结算打倒怪物获得村子收入支出收支成果姓名数下降完成"
                             "建设返回确定撤除道路植物商店饮食金币点数人气年月份倍暂停继续月末"
                             "请选择街道内地域有建筑物金钱不足没有设施不可撤除状态异常施工"
                             "设施情报品质魅力维护费收入建设完毕加成返回图范围";
        for (int i = 32; i < 127; ++i)
            glyphs += static_cast<char>(i);
        for (const auto &v : startup_evidence().definitions)
            glyphs += v.name;
        for (const auto &v : startup_evidence().first_talk)
            glyphs += v;
        glyphs += startup_evidence().first_character.name;
        glyphs += extra;
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
    float measure(const std::string &value, float size = 12) const {
        return MeasureTextEx(font_, value.c_str(), size, 0).x;
    }
    // Wrap at measured codepoint boundaries, preserving Chinese without inserting broken UTF-8.
    float paragraph(const std::string &value, float x, float y, float max_width) const {
        const float start_y = y;
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
        return y - start_y + 17;
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

void check_startup_world() {
    StartupSession initial;
    // 明确研究seed，不认证固定APK默认seed或捕获轨迹。
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream::from_java_seed(1));
    const auto result = session.update();
    if (result.error != StartupWorldRuntimeError::none)
        throw std::runtime_error("共同世界新局首个更新失败");
    std::cout << "world check passed: state=" << session.state().scene.scene_state
              << " pages=" << session.state().scripts.pages.size()
              << " actors=" << session.state().scene.world.world.ai.human_order.size() << '\n';
}

int run_startup_world_window(const std::filesystem::path &assets,
                             const std::filesystem::path &font_path, bool paused, int frames,
                             const std::optional<std::filesystem::path> &screenshot,
                             const std::string &inspect_page) {
    Window window;
    Canvas canvas;
    SourceSprites sprites(assets / "original");
    const auto &rules = startup_world_rules();
    std::string glyphs = rules.script_sources.talks + rules.script_sources.news +
                         rules.script_sources.event_messages;
    for (const auto &human : rules.humans)
        glyphs += human.name;
    for (const auto &task : rules.tasks)
        glyphs += task.name + task.title;
    for (const auto &item : rules.items)
        glyphs += item.name;
    for (const auto &equipment : rules.equipment)
        glyphs += equipment.name;
    glyphs += "任务列表征集队伍征集费出发追加取消候选队伍评价休息成果商店追加"
              "年度贡献勋章授予终止是非满足努力能力上升自宅完成设施升级城镇等级晋级申请"
              "月收入设施数居住数指定建设任务完成数街道人气举办活动达成未暂不可用";
    ChineseFont font(font_path, glyphs);
    // 只在接管前存在旧启动快照；runtime构造后释放，禁止两个可写世界并存。
    std::unique_ptr<StartupSession> initial = std::make_unique<StartupSession>();
    if (inspect_page == "visitor") {
        for (int n = 0; n < 420; ++n)
            initial->update();
    }
    StartupWorldRuntimeSession session(initial->state(), ref::WorldRandomStream::from_java_seed(1));
    initial.reset();
    if (inspect_page == "world-building") {
        if (session.open_build_menu() != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实新局建设目录检查入口失败");
    } else if (inspect_page == "world-details") {
        if (session.open_facility_page(4) != StartupWorldRuntimeError::none)
            throw std::runtime_error("真实初局旅店详情检查入口失败");
    } else if (inspect_page == "world-month" || inspect_page == "world-active" ||
               inspect_page == "task-team" || inspect_page == "world-award") {
        bool reached{};
        // 显式窗口检查策略：只给真实页栈逐轮确认，不改人物/日期/随机/资金。
        for (int step = 0; step < (inspect_page == "world-award" ? 50000 : 20000); ++step) {
            const auto result = session.update();
            if (!result.candidate)
                throw std::runtime_error("共同世界检查预运行失败，step=" + std::to_string(step));
            const auto &s = session.state();
            const auto top = s.scripts.pages.back();
            if (inspect_page == "world-award" && top.lifecycle != 4 &&
                top.kind == ref::WorldScriptPageKind::raw_page && top.legacy_page == 87) {
                reached = true;
                break;
            }
            if (top.kind != ref::WorldScriptPageKind::scene && top.lifecycle != 4 &&
                !(top.kind == ref::WorldScriptPageKind::raw_page &&
                  (top.legacy_page == 56 || top.legacy_page == 57 || top.legacy_page == 16 ||
                   top.legacy_page == 97))) {
                auto error = StartupWorldRuntimeError::none;
                if (inspect_page == "task-team" && (top.legacy_page == 22 || top.legacy_page == 23))
                    error = session.act_task_page(top.id, StartupWorldTaskAction::confirm).error;
                else if (top.legacy_page != 24 && top.legacy_page != 25 && top.legacy_page != 28)
                    error = session.acknowledge_page(top.id);
                if (error != StartupWorldRuntimeError::none)
                    throw std::runtime_error("共同世界检查预运行页面消费者失败");
            }
            if (inspect_page == "task-team" && top.kind == ref::WorldScriptPageKind::scene &&
                !session.state().task_order.empty() && !session.state().active_task &&
                session.open_task_menu() != StartupWorldRuntimeError::none)
                throw std::runtime_error("任务检查预运行菜单入口失败");
            const auto &current = session.state();
            reached = inspect_page == "world-month" ? current.report_state != 0
                      : inspect_page == "task-team"
                          ? current.scripts.pages.back().legacy_page == 25
                          : inspect_page == "world-active" &&
                                current.scene.world.world.ai.human_order.size() >= 3;
            if (reached)
                break;
        }
        if (!reached)
            throw std::runtime_error("共同世界有界检查未到达真实目标状态");
    }
    session.set_paused(paused);
    ref::WorldRenderClock clock;
    Vector2 camera{static_cast<float>(startup_evidence().camera.x),
                   static_cast<float>(startup_evidence().camera.y)};
    Vector2 view_offset{};
    int frame_count{}, paragraph_index{};
    int moved_frames{};
    int confirmation_inputs{}, pause_inputs{}, speed_inputs{};
    std::size_t world_pixel_colors{};
    std::map<ref::CharacterId, ref::CombatPoint> previous_positions;
    float body_scroll{}, body_extent{};
    std::uint64_t viewed_page{};
    int task_selection{}, task_scroll{};
    int award_selection{}, prompt_selection{}, rank_selection{};
    int build_tab{}, build_selection{};
    auto build_orientation = ref::FacilityOrientation::first;
    std::string command_feedback;
    while (!WindowShouldClose()) {
        const Vector2 mouse{GetMousePosition().x / scale, GetMousePosition().y / scale};
        const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const auto hit = [&](Rectangle r) { return pressed && CheckCollisionPointRec(mouse, r); };
        if (hit({4, 295, 44, 22})) {
            ++pause_inputs;
            session.set_paused(!session.state().scene.framework_paused);
        }
        if (hit({176, 295, 58, 22})) {
            ++speed_inputs;
            session.set_speed(session.state().scene.speed_setting == 1 ? 0 : 1);
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            const auto delta = GetMouseDelta();
            view_offset.x -= delta.x / scale;
            view_offset.y += delta.y / scale;
        }
        const auto top_page = [&]() -> const ref::WorldScriptPage * {
            const auto &pages = session.state().scripts.pages;
            for (auto it = pages.rbegin(); it != pages.rend(); ++it)
                if (it->kind != ref::WorldScriptPageKind::scene && it->lifecycle != 4)
                    return &*it;
            return nullptr;
        };
        if (const auto *page = top_page()) {
            if (viewed_page != page->id) {
                viewed_page = page->id;
                paragraph_index = 0;
                body_scroll = body_extent = 0;
                task_selection = task_scroll = 0;
                build_tab = build_selection = 0;
                award_selection = prompt_selection = 0;
                if (page->legacy_page == 1 && session.state().task_abort_questions.count(page->id))
                    prompt_selection = 1; // 原主动中止询问默认“否”，不因新页重置变成“是”。
                rank_selection = 0;
                command_feedback.clear();
            }
            const int raw = page->legacy_page;
            const bool task_page = raw >= 22 && raw <= 28;
            if (raw == 48) {
                if (IsKeyPressed(KEY_UP))
                    rank_selection = (rank_selection + 4) % 5;
                if (IsKeyPressed(KEY_DOWN))
                    rank_selection = (rank_selection + 1) % 5;
                if (hit({12, 77, 216, 20}))
                    rank_selection = 0;
                for (int n = 0; n < 4; ++n)
                    if (hit({12, 101.F + n * 24, 216, 20}))
                        rank_selection = n + 1;
                const bool cancel = IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24});
                if ((cancel || IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) &&
                    session.act_rank_page(page->id, rank_selection, cancel) !=
                        StartupWorldRuntimeError::none)
                    throw std::runtime_error("晋级页面输入失败");
            } else if (raw == 87) {
                const auto &s = session.state();
                const bool ending = s.award_termination_pending.count(page->id) &&
                                    s.award_termination_pending.at(page->id);
                const bool awarding = s.award_pending_humans.count(page->id) != 0;
                std::optional<ref::WorldAwardAction> action;
                if (ending || awarding) {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
                        prompt_selection = 1 - prompt_selection;
                    if (hit({24, 194, 96, 28}))
                        prompt_selection = 0;
                    if (hit({120, 194, 96, 28}))
                        prompt_selection = 1;
                    if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))
                        action =
                            ending ? (prompt_selection ? ref::WorldAwardAction::reject_termination
                                                       : ref::WorldAwardAction::confirm_termination)
                                   : (prompt_selection ? ref::WorldAwardAction::reject_award
                                                       : ref::WorldAwardAction::confirm_award);
                } else if (s.award_rankings.count(page->id)) {
                    const int count = static_cast<int>(s.award_rankings.at(page->id).size());
                    if (count && IsKeyPressed(KEY_UP))
                        award_selection = (award_selection + count - 1) % count;
                    if (count && IsKeyPressed(KEY_DOWN))
                        award_selection = (award_selection + 1) % count;
                    const int start = std::max(0, award_selection - 4);
                    for (int n = start; n < count && n < start + 5; ++n)
                        if (hit({12, 72.F + (n - start) * 22, 216, 20}))
                            award_selection = n;
                    if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) {
                        action = ref::WorldAwardAction::request_termination;
                        prompt_selection = 1; // 原终止询问默认“否”。
                    } else if (count && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))) {
                        action = ref::WorldAwardAction::request_award;
                        prompt_selection = 0;
                    }
                }
                if (action && session.act_award_page(page->id, *action, award_selection) !=
                                  StartupWorldRuntimeError::none)
                    throw std::runtime_error("授勋页面输入失败");
            } else if (raw == 4) {
                if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) {
                    if (session.act_task_page(page->id, StartupWorldTaskAction::cancel).error !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("任务管理返回失败");
                } else if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) {
                    if (session.act_task_page(page->id, StartupWorldTaskAction::request_abort)
                            .error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("主动中止询问失败");
                }
            } else if (raw == 1 && session.state().task_abort_questions.count(page->id)) {
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
                    prompt_selection = 1 - prompt_selection;
                if (hit({24, 194, 96, 28}))
                    prompt_selection = 0;
                if (hit({120, 194, 96, 28}))
                    prompt_selection = 1;
                if ((IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) &&
                    session.act_task_page(page->id, StartupWorldTaskAction::confirm,
                                          prompt_selection)
                            .error != StartupWorldRuntimeError::none)
                    throw std::runtime_error("主动中止答案失败");
            } else if (raw == 80) {
                const auto &list = session.state().residence_page_candidates.at(page->id);
                if (!list.empty()) {
                    if (IsKeyPressed(KEY_UP))
                        task_selection = (task_selection + static_cast<int>(list.size()) - 1) %
                                         static_cast<int>(list.size());
                    if (IsKeyPressed(KEY_DOWN))
                        task_selection = (task_selection + 1) % static_cast<int>(list.size());
                }
                const bool cancel = IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24});
                if (cancel ||
                    (!list.empty() && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})))) {
                    const auto r = session.act_residence_page(
                        page->id, list.empty() ? -1 : list.at(task_selection), cancel);
                    if (r.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("入住选择事务失败");
                }
            } else if (raw == 60 && session.state().page_human_bindings.count(page->id)) {
                if ((IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) &&
                    session.act_task_page(page->id, StartupWorldTaskAction::cancel).error !=
                        StartupWorldRuntimeError::none)
                    throw std::runtime_error("队员详情返回失败");
            } else if (raw == 21) {
                const auto &groups = session.state().build_page_catalogs.at(page->id);
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                    build_tab = (build_tab + (IsKeyPressed(KEY_LEFT) ? 2 : 1)) % 3;
                    build_selection = 0;
                }
                for (int tab = 0; tab < 3; ++tab)
                    if (hit({8.F + tab * 74, 45, 74, 20})) {
                        build_tab = tab;
                        build_selection = 0;
                    }
                const auto &list = groups.at(build_tab);
                const auto count = static_cast<int>(list.size());
                if (count && IsKeyPressed(KEY_UP))
                    build_selection = (build_selection + count - 1) % count;
                if (count && IsKeyPressed(KEY_DOWN))
                    build_selection = (build_selection + 1) % count;
                for (int row = 0; row < count && row < 5; ++row)
                    if (hit({12, 72.F + row * 20, 216, 20}))
                        build_selection = row;
                const auto pid = page->id;
                if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) {
                    if (session.cancel_build_menu(pid) != StartupWorldRuntimeError::none)
                        throw std::runtime_error("建设目录返回失败");
                } else if (count && (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24}))) {
                    const auto r = session.select_build_menu(pid, list.at(build_selection));
                    if (r.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("建设选择失败");
                    command_feedback =
                        r.denial == StartupBuildDenial::insufficient_funds ? "金钱不足" : "";
                    build_orientation = ref::FacilityOrientation::first;
                }
            } else if (raw == 74) {
                const auto pid = page->id;
                if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
                    if (session.act_facility_page(pid, StartupFacilityPageAction::next) !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施情报翻页失败");
                }
                if (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) {
                    if (session.act_facility_page(pid, StartupFacilityPageAction::cancel) !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施情报返回失败");
                }
                if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) {
                    const auto error =
                        session.act_facility_page(pid, StartupFacilityPageAction::confirm);
                    if (error != StartupWorldRuntimeError::none)
                        command_feedback = "暂不可用";
                }
            } else if (raw == 33) {
                const auto phase = session.state().page_phases.find(page->id);
                const bool animating =
                    phase != session.state().page_phases.end() && phase->second == 1;
                if (!animating) {
                    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
                        task_selection = 1 - task_selection;
                    if (hit({24, 194, 96, 28}))
                        task_selection = 0;
                    if (hit({120, 194, 96, 28}))
                        task_selection = 1;
                }
                // 页33无按钮2/Back分支；Esc不伪造取消，选择中止仍走同一50计数演出。
                if (IsKeyPressed(KEY_ENTER) || hit({176, 265, 58, 24})) {
                    ++confirmation_inputs;
                    if (session
                            .act_task_page(page->id, StartupWorldTaskAction::confirm,
                                           task_selection)
                            .error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("任务期限输入消费者失败");
                }
            } else if (task_page) {
                const auto &s = session.state();
                const int count =
                    raw == 22 ? static_cast<int>(s.task_page_lists.at(page->id).size())
                    : (raw == 25 || raw == 26) ? static_cast<int>(s.participants.size()) + 1
                    : raw == 27 ? static_cast<int>(s.task_extra_pages.at(page->id).size())
                                : 0;
                if (count > 0) {
                    if (IsKeyPressed(KEY_UP))
                        task_selection = (task_selection + count - 1) % count;
                    if (IsKeyPressed(KEY_DOWN))
                        task_selection = (task_selection + 1) % count;
                    for (int row = 0; row < 5 && row + task_scroll < count; ++row)
                        if (hit({12, 62.0F + row * 20, 216, 20}))
                            task_selection = row + task_scroll;
                    task_scroll = std::clamp(task_scroll, task_selection - 4, task_selection);
                    task_scroll = std::clamp(task_scroll, 0, std::max(count - 5, 0));
                }
                if (raw != 24 && (IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24}))) {
                    if (session.act_task_page(page->id, StartupWorldTaskAction::cancel).error !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("任务取消消费者失败");
                } else if ((raw == 25 || raw == 26) && hit({86, 265, 68, 24}) &&
                           task_selection < static_cast<int>(s.participants.size())) {
                    if (session
                            .act_task_page(page->id, StartupWorldTaskAction::inspect,
                                           task_selection)
                            .error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("队员详情打开失败");
                } else if (raw != 24 && (hit({176, 265, 58, 24}) || IsKeyPressed(KEY_ENTER))) {
                    auto action = StartupWorldTaskAction::confirm;
                    int selection = task_selection;
                    if (raw == 25)
                        action = selection == static_cast<int>(s.participants.size())
                                     ? StartupWorldTaskAction::add_member
                                     : StartupWorldTaskAction::depart;
                    else if (raw == 26 && selection == static_cast<int>(s.participants.size()))
                        action = StartupWorldTaskAction::add_member;
                    else if (raw == 27) {
                        action = StartupWorldTaskAction::hire;
                        selection = s.task_extra_pages.at(page->id).at(task_selection);
                    }
                    ++confirmation_inputs;
                    if (session.act_task_page(page->id, action, selection).error !=
                        StartupWorldRuntimeError::none)
                        throw std::runtime_error("任务输入消费者失败");
                }
            } else if (page->legacy_page == 83) {
                if ((IsKeyPressed(KEY_ESCAPE) || hit({8, 265, 58, 24})) &&
                    session.cancel_page(page->id) != StartupWorldRuntimeError::none)
                    throw std::runtime_error("商店追加菜单返回消费者失败");
            } else if (page->legacy_page != 97 &&
                       (hit({176, 265, 58, 24}) || IsKeyPressed(KEY_ENTER))) {
                ++confirmation_inputs;
                if (paragraph_index + 1 < static_cast<int>(page->paragraphs.size())) {
                    ++paragraph_index;
                    body_scroll = body_extent = 0;
                } else {
                    const auto error = session.acknowledge_page(page->id);
                    if (error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("共同世界页面输入消费者失败");
                }
            }
            if (CheckCollisionPointRec(mouse, {12, 196, 216, 64}))
                body_scroll = std::clamp(body_scroll - GetMouseWheelMove() * 17, 0.0F,
                                         std::max(body_extent - 64, 0.0F));
        }
        if (!top_page() && session.state().scene.scene_state == 1) {
            if (IsKeyPressed(KEY_R))
                build_orientation = build_orientation == ref::FacilityOrientation::first
                                        ? ref::FacilityOrientation::second
                                        : ref::FacilityOrientation::first;
            if (IsKeyPressed(KEY_ESCAPE) || hit({52, 295, 52, 22})) {
                if (session.cancel_build() != StartupWorldRuntimeError::none)
                    throw std::runtime_error("建设返回失败");
                command_feedback.clear();
            } else if (pressed && mouse.y >= 35 && mouse.y < 265) {
                if (const auto cell = pick(mouse, camera)) {
                    const auto r = session.confirm_build(*cell, build_orientation);
                    if (r.error != StartupWorldRuntimeError::none)
                        throw std::runtime_error("建设事务失败");
                    command_feedback =
                        r.denial == StartupBuildDenial::none                 ? "建设完毕"
                        : r.denial == StartupBuildDenial::insufficient_funds ? "金钱不足"
                        : r.denial == StartupBuildDenial::occupied           ? "有建筑物"
                                                                             : "请选择街道内地域";
                }
            }
        } else if (!top_page() && session.state().scene.scene_state == 0 &&
                   !session.state().scene.framework_paused) {
            if (hit({52, 295, 52, 22}) || IsKeyPressed(KEY_B)) {
                if (session.open_build_menu() != StartupWorldRuntimeError::none)
                    throw std::runtime_error("建设目录打开失败");
            } else if (pressed && mouse.y >= 35 && mouse.y < 265) {
                if (const auto cell = pick(mouse, camera)) {
                    const auto &map = session.state().scene.world.world.map;
                    const auto &binding = map.cells.at(cell->y * map.width + cell->x).facility;
                    if (binding &&
                        session.state()
                                .scene.world.world.facilities.at(binding->instance_id.value)
                                .status != 0 &&
                        session.open_facility_page(binding->instance_id.value) !=
                            StartupWorldRuntimeError::none)
                        throw std::runtime_error("设施情报打开失败");
                }
            }
        }
        if (!top_page() && session.state().scene.scene_state == 0 &&
            (hit({108, 295, 60, 22}) || IsKeyPressed(KEY_T)) &&
            session.open_task_menu() != StartupWorldRuntimeError::none)
            throw std::runtime_error("任务菜单消费者失败");
        if (!top_page() && session.state().scene.scene_state == 0 && session.state().active_task &&
            IsKeyPressed(KEY_X) &&
            session.open_task_control_menu() != StartupWorldRuntimeError::none)
            throw std::runtime_error("任务管理菜单打开失败");
        const auto *held_page = top_page();
        session.set_page_confirm_held(
            held_page && held_page->legacy_page == 24 &&
            (IsKeyDown(KEY_ENTER) || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
                                      CheckCollisionPointRec(mouse, {176, 265, 58, 24}))));
        // 框架绘制门槛与逻辑轮数分开；长停顿不补算，倍速由MainScene保存轮数。
        const auto gate =
            ref::prepare_world_render_gate(clock, static_cast<std::int64_t>(GetTime() * 1000));
        if (gate.error != ref::WorldRenderGateError::none)
            throw std::runtime_error("共同世界绘制时钟失败");
        if (gate.clock) {
            clock = *gate.clock;
            const auto result = session.update();
            if (result.error != StartupWorldRuntimeError::none)
                throw std::runtime_error(
                    "共同世界更新失败：" + std::to_string(static_cast<int>(result.error)) + "/" +
                    std::to_string(static_cast<int>(result.scene_error)) + "/" +
                    std::to_string(static_cast<int>(result.world_error)));
        }
        const auto &state = session.state();
        camera = {state.camera[0] + view_offset.x, state.camera[1] + view_offset.y};
        const auto &world = state.scene.world.world;
        bool moved{};
        for (const auto &entry : world.ai.battle.actors) {
            const auto old = previous_positions.find(entry.first);
            const auto &p = entry.second.position;
            if (old != previous_positions.end() &&
                (old->second.x != p.x || old->second.z != p.z || old->second.height != p.height))
                moved = true;
            previous_positions[entry.first] = p;
        }
        moved_frames += moved ? 1 : 0;
        BeginTextureMode(canvas.value);
        ClearBackground(Color{83, 141, 77, 255});
        const auto display = [&](int id) -> const StartupDisplay & {
            const auto &list = startup_evidence().displays;
            const auto found =
                std::find_if(list.begin(), list.end(), [id](const auto &d) { return d.id == id; });
            if (found == list.end())
                throw std::runtime_error("共同世界缺少真实显示定义");
            return *found;
        };
        struct Overlay {
            float depth;
            std::function<void()> draw;
        };
        std::vector<Overlay> overlays;
        std::vector<Overlay> patches; // 原道路补块在地表第一遍之后提交。
        constexpr std::array<ref::Position, 6> fence_offsets{
            {{13, 22}, {16, 22}, {13, 21}, {42, 22}, {15, 23}, {16, 9}}};
        constexpr std::array<ref::Position, 4> entry_offsets{
            {{15, 24}, {26, 19}, {15, 18}, {28, 24}}};
        for (int y = world.map.height - 1; y >= 0; --y)
            for (int x = 0; x < world.map.width; ++x) {
                const auto index = static_cast<std::size_t>(y * world.map.width + x);
                const auto &cell = state.surface.at(index);
                const auto p = project({x, y}, camera);
                if (cell.display_definition < 0)
                    continue;
                const auto &record = display(cell.display_definition);
                const float base_depth =
                    ((record.flags & 1U) ? p.y - 50 : p.y + 15) + record.offset_y;
                // c/i.i已经是占地分片帧；逐格绘制双格/四格，不能再只画设施锚点。
                overlays.push_back(
                    {base_depth, [&, p, sprite = record.sprite, frame = cell.variant] {
                         sprites.draw(sprite, frame, p);
                     }});
                if (cell.fragment >= 0 && cell.fragment < 6 &&
                    world.map.cells[index].category == ref::RouteCategory::blocked) {
                    const auto offset = fence_offsets.at(cell.fragment);
                    overlays.push_back(
                        {base_depth + 60, [&, p, offset, frame = cell.fragment] {
                             sprites.draw("fence01" + std::to_string(state.fence_level) + ".seb",
                                          frame, {p.x + offset.x, p.y + offset.y}, WHITE,
                                          SourceSprites::Binding::common);
                         }});
                }
                if (cell.instance >= 0) { // 维护旧字段名instance实际保存原c/i.m外部入口方向。
                    const auto offset = entry_offsets.at(static_cast<std::size_t>(cell.instance));
                    overlays.push_back({p.y + offset.y, [&, p, offset, frame = cell.instance / 2] {
                                            sprites.draw("door00.seb", frame,
                                                         {p.x + offset.x, p.y + offset.y}, WHITE,
                                                         SourceSprites::Binding::common);
                                        }});
                }
                LoadedStartupCell patch_cell;
                patch_cell.display_id = cell.display_definition;
                patch_cell.road_quad = state.road_patches.at(index)[0];
                patch_cell.edge_road_pair = state.road_patches.at(index)[1];
                if (const auto patch = road_patch_draw(patch_cell)) {
                    patches.push_back({p.y + patch->depth_offset, [&, p, patch = *patch] {
                                           sprites.image(patch.asset_path, {p.x + patch.offset_x,
                                                                            p.y + patch.offset_y});
                                       }});
                }
            }
        overlays.insert(overlays.end(), patches.begin(), patches.end());
        const auto draw_actor = [&](ref::CharacterId id) {
            const auto &actor = world.ai.battle.actors.at(id);
            if (actor.control.flags & 1U)
                return; // 原Character.a(o,int)首个绘制守卫，不从状态号猜隐身。
            const auto &p = state.actor_metadata.at(id).render_position; // 原o，不是攻击备份au。
            const Vector2 anchor{120 + (p.x + p.z) * 0.3F - camera.x,
                                 160 + (p.x - p.z) * 0.15F - p.height + camera.y};
            const int direction = actor.control.facing;
            const int action = actor.control.action;
            const int tick = (actor.control.flags & 2U) ? actor.control.action_counter : 0;
            if (actor.kind == ref::ActorKind::human) {
                const auto &meta = state.actor_metadata.at(id);
                const int image = rules.jobs.at(meta.profession).sprites.at(meta.sex);
                constexpr std::array<int, 12> offsets{{0, 4, 4, 4, 20, 4, 8, 12, 16, 20, 24, 20}};
                int phase{};
                if (action == 0 || action == 8)
                    phase = (tick % 16) / 4; // c.b.aZ=4，四个阈值4/8/12/16。
                else if (action == 1)
                    phase = tick % 12 < 4 ? 0 : 1;
                else if (action == 2 || action == 5)
                    phase = 1;
                else if (action == 3)
                    phase = tick % 12 < 6 ? 0 : 1;
                else if (action == 4)
                    phase = tick % 42 < 26 ? 0 : 1;
                else if (action == 10)
                    phase = tick % 18 < 12 ? 0 : 1;
                const int sprite = offsets.at(action) + (action == 5 ? tick % 16 % 4 : direction);
                sprites.actor(false, sprite, image, phase, anchor);
            } else {
                const auto &definition = world.ai.monster_growth.at(actor.definition);
                constexpr std::array<int, 4> lengths{{6, 6, 8, 10}};
                const int body = definition.body;
                int phase = (tick % (lengths.at(body) * 4)) / lengths.at(body);
                constexpr std::array<int, 4> attacks{{2, 4, 6, 8}};
                if (action == 3) {
                    if (tick < 10 || (tick >= 28 && tick < 34))
                        phase = 0;
                    else if (tick < 28)
                        phase = (tick % (attacks.at(body) * 4)) / attacks.at(body);
                } else if (action == 6) {
                    if (tick < 6)
                        phase = (tick % (attacks.at(body) * 4)) / attacks.at(body);
                    else if (tick < 12)
                        phase = 0;
                }
                sprites.actor(true, body * 4 + direction, body * 30 + definition.sprite_variant,
                              action == 9 ? 0 : phase, anchor);
            }
        };
        for (const auto *roster : {&world.ai.human_order, &world.ai.monster_order})
            for (const auto id : *roster) {
                const auto &position = world.ai.battle.actors.at(id).position;
                const float depth = 160 + (position.x - position.z) * 0.15F + camera.y;
                overlays.push_back({depth, [&, id] { draw_actor(id); }});
            }
        for (const auto id : state.scene.world.facility_order) {
            const auto rows = startup_world_inn_rows(state, id);
            if (!rows)
                throw std::runtime_error("旅馆占用人物显示投影无效");
            if (rows->empty())
                continue;
            const auto target = startup_world_runtime_facility_target(state, id);
            if (!target)
                throw std::runtime_error("旅馆真实占地锚点无效");
            // f()是原投影；a.a(f,L)在此研究画布转换为同一镜头，不能取设施anchor格。
            const Vector2 anchor{120 + (*target)[0] - camera.x, 160 - (*target)[1] + camera.y};
            overlays.push_back(
                {anchor.y, [&, anchor, rows = *rows] {
                     for (const auto &row : rows) {
                         const Vector2 p{anchor.x - 26, anchor.y - 42 - row.row * 20};
                         sprites.crop("common/restBar00.png", {0, 0, row.healing ? 17.F : 48.F, 17},
                                      p);
                         if (!row.healing) {
                             DrawRectangle(static_cast<int>(p.x) + 17, static_cast<int>(p.y) + 13,
                                           row.bar_width, 2, {255, 83, 152, 255});
                         } else {
                             const auto value = std::to_string(row.capacity);
                             float x = p.x + 47 - value.size() * 7;
                             for (const auto c : value) {
                                 sprites.draw("number11.seb", c - '0', {x, p.y + 1}, WHITE,
                                              SourceSprites::Binding::common);
                                 x += 7;
                             }
                             DrawRectangle(static_cast<int>(p.x) + 20, static_cast<int>(p.y) + 10,
                                           29, 5, {246, 246, 246, 255});
                             DrawRectangle(static_cast<int>(p.x) + 21, static_cast<int>(p.y) + 11,
                                           27, 3, {39, 53, 74, 255});
                             DrawRectangle(static_cast<int>(p.x) + 22, static_cast<int>(p.y) + 12,
                                           row.bar_width, 2, {83, 255, 0, 255});
                             DrawRectangle(static_cast<int>(p.x) + 22 + row.bar_width,
                                           static_cast<int>(p.y) + 12, 26 - row.bar_width, 2,
                                           {68, 100, 104, 255});
                         }
                         sprites.portrait(row.portrait, {p.x + 1, p.y + 1});
                     }
                 }});
        }
        std::stable_sort(overlays.begin(), overlays.end(),
                         [](const auto &a, const auto &b) { return a.depth < b.depth; });
        for (const auto &overlay : overlays)
            overlay.draw();
        if (state.scene.scene_state == 1 && state.build_definition) {
            if (const auto cell = pick(mouse, camera)) {
                const auto &d = rules.facilities.at(*state.build_definition);
                const auto footprint = ref::facility_footprint(
                    static_cast<ref::FacilityShape>(d.shape), build_orientation, *cell,
                    world.map.width, world.map.height);
                for (const auto &part : footprint.cells) {
                    const auto p = project(part.position, camera);
                    DrawTriangle({p.x - 30, p.y}, {p.x, p.y + 15}, {p.x + 30, p.y},
                                 {250, 220, 55, 120});
                    DrawTriangle({p.x - 30, p.y}, {p.x + 30, p.y}, {p.x, p.y - 15},
                                 {250, 220, 55, 120});
                }
            }
            DrawRectangle(5, 265, 230, 24, paper);
            font.text(command_feedback.empty() ? state.build_feedback_message : command_feedback,
                      12, 270);
        }
        for (const auto &effect : state.visual_effects) {
            if (effect.size() < 2 || effect[0] != 2 || effect[1] < 0)
                continue;
            if (effect.size() != 7)
                throw std::runtime_error("现金显示载荷不完整");
            float y = 160 - effect[3] + camera.y;
            y +=
                effect[1] < 6
                    ? -16 + ((effect[5] * effect[1] + effect[6] * effect[1] * (effect[1] + 1) / 2) /
                             1000)
                    : -26;
            const auto amount = std::to_string(effect[4]);
            float x = 120 + effect[2] - camera.x - (amount.size() * 8 + 9) / 2.0F + 28;
            for (const auto digit : amount) {
                sprites.draw("number05.seb", digit - '0', {x, y - 10}, WHITE,
                             SourceSprites::Binding::common);
                x += 8;
            }
            sprites.draw("number05.seb", 20, {x, y - 10}, WHITE, SourceSprites::Binding::common);
        }
        const auto notices = ref::world_notice_placements(state.scripts.notices);
        if (!notices)
            throw std::runtime_error("通知绘制投影无效");
        // 皮肤/成长属性拼片仍另验；这里仅消费已证布局和Owner文本，不从60FPS推进队列。
        BeginScissorMode(0, 35, width, height - 35);
        for (const auto &placement : *notices) {
            const auto &notice = state.scripts.notices.at(placement.index);
            const int y = 294 + placement.offset;
            if (notice.message != 1) {
                DrawRectangle(0, y, width, placement.height, paper);
                font.text(notice.text, 4, y + 3, ink, 10);
            }
        }
        EndScissorMode();
        DrawRectangle(0, 0, width, 35, paper);
        font.text(std::to_string(state.scene.calendar.year + 1) + "年" +
                      std::to_string(state.scene.calendar.month + 1) + "月",
                  6, 5);
        font.text(std::to_string(world.ai.accounting.funds()) + "G", 145, 5);
        font.text("点数 " + std::to_string(state.village_points), 6, 21);
        font.text("人气 " + std::to_string(state.popularity), 116, 21);
        if (state.report_state != 0) {
            // 原r/s月报不占框架页栈、不暂停世界；只展示已提交快照，不再扣款。
            DrawRectangle(10, 45, 220, 112, paper);
            font.text("本月结算", 84, 51);
            if (state.report_state == 1) {
                font.text("打倒怪物", 20, 76);
                font.text(std::to_string(state.report_snapshot[0]), 177, 76);
                font.text("获得村子点数", 20, 102);
                font.text(std::to_string(state.report_snapshot[1]), 177, 102);
            } else {
                constexpr std::array<const char *, 3> labels{{"收入", "支出", "收支"}};
                for (std::size_t n = 0; n < labels.size(); ++n) {
                    font.text(labels[n], 20, 75 + static_cast<int>(n) * 23);
                    const auto amount = std::to_string(state.report_snapshot[n + 2]) + "G";
                    const auto extent = font.measure(amount);
                    font.text(amount, 220 - extent, 75 + static_cast<int>(n) * 23);
                }
            }
        }
        if (const auto *page = top_page()) {
            DrawRectangle(5, 170, 230, 119, paper);
            std::string title = page->title;
            if (title.empty() && page->kind == ref::WorldScriptPageKind::raw_page) {
                if (page->legacy_page == 30 || page->legacy_page == 31 || page->legacy_page == 32)
                    title = "成果";
                else if (page->legacy_page == 94)
                    title = "入手!";
                else if (page->legacy_page == 33)
                    title = "任务期限";
                else if (page->legacy_page == 83)
                    title = "商店追加";
                else if (page->legacy_page == 59)
                    title = "活动";
                else if (page->legacy_page >= 22 && page->legacy_page <= 28)
                    title = page->legacy_page == 22   ? "任务列表"
                            : page->legacy_page == 27 ? "追加队员"
                                                      : "征集队伍";
                else if ((page->legacy_page == 99 || page->legacy_page == 100) &&
                         page->monster_definition)
                    title = rules.monsters.at(*page->monster_definition).name;
            }
            font.text(title, 12, 177, ink, font.measure(title) > 208 ? 10 : 12);
            if (page->legacy_page == 21) {
                DrawRectangle(5, 42, 230, 126, paper);
                constexpr std::array<const char *, 3> tabs{{"道路植物", "商店", "饮食"}};
                for (int tab = 0; tab < 3; ++tab) {
                    if (tab == build_tab)
                        DrawRectangle(8 + tab * 74, 45, 74, 20, {210, 229, 195, 255});
                    font.text(tabs[tab], 12 + tab * 74, 48, ink, 10);
                }
                const auto &list = state.build_page_catalogs.at(page->id).at(build_tab);
                for (std::size_t row = 0; row < list.size() && row < 5; ++row) {
                    const auto &d = rules.facilities.at(list[row]);
                    if (static_cast<int>(row) == build_selection)
                        DrawRectangle(10, 72 + static_cast<int>(row) * 20, 220, 20,
                                      {255, 236, 174, 255});
                    font.text(d.name, 14, 76 + row * 20);
                    const auto quote = startup_world_build_quote(state, d.id);
                    if (!quote)
                        throw std::runtime_error("建设目录报价缺失");
                    font.text(std::to_string(quote->construction_cost) + "G", 175, 76 + row * 20,
                              ink, 10);
                }
                font.text(command_feedback, 12, 199, ink, 10);
                font.text("返回", 18, 272);
            } else if (page->legacy_page == 74) {
                const auto id = state.facility_page_bindings.at(page->id);
                const auto &facility = world.facilities.at(id);
                const auto &d = rules.facilities.at(facility.placement.definition_id);
                const auto values = startup_world_facility_values(state, id);
                if (!values)
                    throw std::runtime_error("设施情报经营投影失败");
                DrawRectangle(5, 42, 230, 126, paper);
                font.text(d.name, 12, 48);
                const auto phase = state.page_phases.at(page->id);
                font.text(std::to_string(phase + 1) + "/" +
                              std::to_string(startup_world_facility_page_count(state, *page)),
                          192, 48, ink, 10);
                constexpr std::array<const char *, 4> labels{{"价格", "品质", "魅力", "维护费"}};
                const int start = phase == 0 ? 0 : 3;
                const int end = phase == 0 ? 3 : 4;
                for (int n = start; n < end; ++n) {
                    const float y = 74 + (n - start) * 23;
                    font.text(labels[n], 16, y);
                    font.text(std::to_string(values->instance_attributes[n]), 170, y);
                }
                if (phase == 1) {
                    font.text("收入", 16, 102);
                    font.text(std::to_string(state.facility_monthly_cash.at(
                                  id)[state.scene.calendar.month][0]) +
                                  "G",
                              160, 102);
                    font.text("加成", 16, 126);
                    font.text(std::to_string(state.facility_page_neighbours.at(page->id).size()),
                              170, 126);
                }
                font.text("返回", 18, 272);
                if (d.detail == 6)
                    font.text("入住", 190, 272);
                font.text(command_feedback, 12, 199, ink, 10);
            }
            if (page->kind == ref::WorldScriptPageKind::raw_page && page->legacy_page == 59) {
                // 定义已解锁但实例尚未到访；这里只读当前职业/性别和页面计数绘制。
                const auto &human = rules.humans.at(page->legacy_f);
                const auto profession =
                    world.ai.growth.at(human.identity).definition.current_profession;
                const auto &job = rules.jobs.at(profession);
                const auto count = state.page_counters.find(page->id);
                const int tick = count == state.page_counters.end() ? 0 : count->second;
                DrawRectangle(8, 42, 224, 126, paper);
                sprites.actor(false, 2, job.sprites.at(human.sex), (tick % 16) / 4, {120, 144});
                if (tick >= 60)
                    font.text("多指教", 98, 91);
                BeginScissorMode(12, 196, 212, 64);
                font.paragraph(job.name + "的\n" + human.name + "可以到访了!", 12, 198, 208);
                EndScissorMode();
            }
            if (!page->paragraphs.empty()) {
                BeginScissorMode(12, 196, 212, 64);
                body_extent =
                    font.paragraph(page->paragraphs.at(static_cast<std::size_t>(paragraph_index)),
                                   12, 198 - body_scroll, 208);
                EndScissorMode();
                if (body_extent > 64) {
                    DrawRectangle(226, 198, 2, 60, LIGHTGRAY);
                    DrawRectangle(226,
                                  198 + static_cast<int>(body_scroll / (body_extent - 64) * 48), 2,
                                  12, GRAY);
                }
            }
            if (page->kind == ref::WorldScriptPageKind::raw_page && page->legacy_page == 31) {
                const auto crew = state.crew_summaries.find(page->id);
                if (crew != state.crew_summaries.end()) {
                    DrawRectangle(10, 48, 220, 119, paper);
                    font.text("姓名", 18, 54);
                    font.text("打倒数", 133, 54);
                    font.text("下降", 196, 54);
                    for (std::size_t n = 0; n < crew->second.size() && n < 5; ++n) {
                        const auto definition = crew->second[n];
                        const auto &human = world.ai.battle.humans.at(definition);
                        const auto &name = rules.humans.at(definition).name;
                        const auto y = 77 + static_cast<int>(n) * 17;
                        font.text(name, 18, y, ink, font.measure(name) > 110 ? 10 : 12);
                        font.text(std::to_string(human.task_kills), 151, y);
                        font.text(std::to_string(human.participant_downs), 209, y);
                    }
                }
            }
            if (page->kind == ref::WorldScriptPageKind::raw_page &&
                (page->legacy_page == 30 || page->legacy_page == 31 || page->legacy_page == 32) &&
                page->task_definition) {
                const auto &task = rules.tasks.at(*page->task_definition);
                BeginScissorMode(12, 196, 212, 64);
                font.paragraph(task.name + "完成!", 12, 198, 208);
                EndScissorMode();
            }
            if (page->legacy_page == 87) {
                const bool ending = state.award_termination_pending.count(page->id) &&
                                    state.award_termination_pending.at(page->id);
                const bool awarding = state.award_pending_humans.count(page->id) != 0;
                DrawRectangle(8, 42, 224, 190, paper);
                font.text("年度贡献 / 勋章 " + std::to_string(state.medal_count), 17, 48);
                if (ending || awarding) {
                    font.paragraph(
                        ending ? "终止授勋仪式？"
                               : "授予" +
                                     rules.humans.at(state.award_pending_humans.at(page->id)).name +
                                     "勋章？",
                        17, 83, 204);
                    DrawRectangle(prompt_selection == 0 ? 24 : 120, 194, 96, 28,
                                  {219, 232, 204, 255});
                    font.text("是", 62, 202);
                    font.text("否", 158, 202);
                } else if (state.award_rankings.count(page->id)) {
                    const auto &list = state.award_rankings.at(page->id);
                    const int start = std::max(0, award_selection - 4);
                    for (int n = start; n < static_cast<int>(list.size()) && n < start + 5; ++n) {
                        const int human = list[n];
                        const int y = 75 + (n - start) * 22;
                        if (n == award_selection)
                            DrawRectangle(12, y - 3, 216, 20, {219, 232, 204, 255});
                        font.text(rules.humans.at(human).name, 17, y);
                        font.text(std::to_string(state.human_calendar.at(human).contribution), 188,
                                  y);
                    }
                    font.text("终止", 18, 272);
                }
                font.text(ending || awarding ? "确定" : "授予", 190, 272);
            } else if (page->legacy_page == 4) {
                font.paragraph("任务实施中", 17, 66, 204);
                font.text("返回", 18, 272);
                font.text("中止", 190, 272);
            } else if (page->legacy_page == 1 && state.task_abort_questions.count(page->id)) {
                DrawRectangle(prompt_selection == 0 ? 24 : 120, 194, 96, 28, {219, 232, 204, 255});
                font.text("是", 62, 202);
                font.text("否", 158, 202);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 80) {
                DrawRectangle(8, 42, 224, 190, paper);
                const auto &list = state.residence_page_candidates.at(page->id);
                const int start = std::max(0, task_selection - 3);
                for (int n = start; n < static_cast<int>(list.size()) && n < start + 4; ++n) {
                    const auto &human = rules.humans.at(list[n]);
                    const int y = 65 + (n - start) * 24;
                    if (n == task_selection)
                        DrawRectangle(12, y - 3, 216, 22, {219, 232, 204, 255});
                    font.text(human.name, 17, y);
                    font.text(std::to_string(human.residence_fee) + "G", 178, y);
                }
                font.text("返回", 18, 272);
                font.text("入住", 190, 272);
            } else if (page->legacy_page == 60 && state.page_human_bindings.count(page->id)) {
                const int human = state.page_human_bindings.at(page->id);
                const auto &g = world.ai.growth.at(human);
                font.text(rules.humans.at(human).name, 17, 66);
                font.text(rules.jobs.at(g.definition.current_profession).name, 17, 90);
                font.text("努力 " + std::to_string(g.definition.legacy_u), 17, 114);
                font.text("满足 " + std::to_string(state.shop_humans.at(human).satisfaction), 17,
                          138);
                for (int n = 0; n < 4; ++n)
                    font.text(std::to_string(g.derived.combat[n]), 17 + n * 50, 170);
                font.text("返回", 18, 272);
            } else if (page->legacy_page == 81) {
                font.text("设施升级", 17, 66);
                for (int n = 0; n < 3; ++n)
                    font.text(std::to_string(state.facility_upgrade_display[0][n]) + " > " +
                                  std::to_string(state.facility_upgrade_display[1][n]),
                              17, 100 + n * 24);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 50) {
                font.text("城镇等级 " + std::to_string(state.rank), 17, 66);
                const auto cast = state.rank_celebration_participants.find(page->id);
                if (cast != state.rank_celebration_participants.end())
                    for (std::size_t n = 0; n < cast->second.size(); ++n)
                        font.text(rules.humans.at(cast->second[n][0]).name, 17 + (n % 2) * 110,
                                  100 + (n / 2) * 24);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 48 || page->legacy_page == 49) {
                font.text("城镇等级 " + std::to_string(state.rank), 17, 66);
                if (page->legacy_page == 48) {
                    DrawRectangle(12, rank_selection == 0 ? 77 : 101 + (rank_selection - 1) * 24,
                                  216, 20, {219, 232, 204, 255});
                    font.text("晋级申请", 17, 80);
                    font.text("返回", 18, 272);
                }
                if (state.rank < 5) {
                    static const std::array<std::string, 7> labels{{"月收入", "设施数", "居住数",
                                                                    "指定建设", "任务完成数",
                                                                    "街道人气", "举办活动数"}};
                    const auto terms = ref::fixed_calendar_task_rank_terms().at(state.rank);
                    for (int n = 0; n < 4; ++n)
                        font.text(labels.at(terms[n].type) + " " +
                                      std::to_string(state.rank_values[n]) +
                                      (state.rank_met[n] ? " 达成" : " 未达成"),
                                  17, 104 + n * 24);
                }
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 67 || page->legacy_page == 88 ||
                       page->legacy_page == 96) {
                if (state.page_human_bindings.count(page->id))
                    font.text(rules.humans.at(state.page_human_bindings.at(page->id)).name, 17, 66);
                font.text(page->legacy_page == 67   ? "能力上升"
                          : page->legacy_page == 88 ? "勋章授予"
                                                    : "自宅完成",
                          17, 90);
                for (int n = 0; n < 2; ++n)
                    font.text(std::string(n == 0 ? "满足 " : "努力 ") +
                                  std::to_string(state.reward_display[0][n]) + " > " +
                                  std::to_string(state.reward_display[1][n]),
                              17, 118 + n * 24);
                font.text("确定", 190, 272);
            } else if (page->legacy_page == 33) {
                DrawRectangle(8, 42, 224, 126, paper);
                if (page->task_definition)
                    font.paragraph(rules.tasks.at(*page->task_definition).name, 17, 65, 204);
                font.text("延长费用 " + std::to_string(page->legacy_f) + "G", 17, 100);
                const auto grade = state.deadline_grades.find(page->id);
                if (grade != state.deadline_grades.end()) {
                    static const std::string grades[]{"再加把劲!!", "再加加油!!", "需要补充战力!",
                                                      "重新来过比较好", "应该撤退…"};
                    font.text(grades[grade->second], 17, 122);
                }
                const auto phase = state.page_phases.find(page->id);
                const bool animating = phase != state.page_phases.end() && phase->second == 1;
                if (animating) {
                    const auto counter = state.page_counters.find(page->id);
                    const float ratio = counter == state.page_counters.end()
                                            ? 0.0F
                                            : std::clamp(counter->second / 50.0F, 0.0F, 1.0F);
                    DrawRectangle(24, 199, 192, 8, GRAY);
                    DrawRectangle(24, 199, static_cast<int>(192 * ratio), 8, GREEN);
                } else {
                    DrawRectangle(task_selection == 0 ? 24 : 120, 194, 96, 28,
                                  Color{219, 232, 204, 255});
                    font.text("继续", 52, 202);
                    font.text("中止", 148, 202);
                }
                font.text("确定", 190, 270);
            } else if (page->legacy_page >= 22 && page->legacy_page <= 28) {
                DrawRectangle(8, 42, 224, 126, paper);
                std::vector<std::string> rows;
                if (page->legacy_page == 22) {
                    for (const auto id : state.task_page_lists.at(page->id))
                        rows.push_back(rules.tasks.at(state.tasks.at(id).definition).name);
                } else if (page->legacy_page == 25 || page->legacy_page == 26 ||
                           page->legacy_page == 27) {
                    const auto &list = page->legacy_page != 27
                                           ? state.participants
                                           : state.task_extra_pages.at(page->id);
                    for (const auto id : list) {
                        auto name = rules.humans.at(id).name;
                        if (page->legacy_page == 27)
                            name += " " +
                                    std::to_string(state.human_calendar.at(id).continuation_cost) +
                                    "G";
                        rows.push_back(name);
                    }
                    if (page->legacy_page == 25 || page->legacy_page == 26)
                        rows.emplace_back("追加队员");
                }
                for (int row = 0; row < 5 && row + task_scroll < static_cast<int>(rows.size());
                     ++row) {
                    const auto &text = rows[row + task_scroll];
                    const int y = 65 + row * 20;
                    if (row + task_scroll == task_selection)
                        DrawRectangle(12, y - 3, 216, 19, Color{219, 232, 204, 255});
                    font.text(text, 17, y, ink, font.measure(text) > 204 ? 10 : 12);
                }
                if (page->task_definition && (page->legacy_page == 23 || page->legacy_page == 28)) {
                    const auto &task = rules.tasks.at(*page->task_definition);
                    font.paragraph(task.name, 17, 65, 204);
                    font.text(page->legacy_page == 23
                                  ? "征集费 " + std::to_string(task.recruitment_fee) + "G"
                                  : "队伍评价 " +
                                        std::to_string(state.task_page_predictions.at(page->id)),
                              17, 104);
                }
                if (page->legacy_page == 24) {
                    const auto &animation = state.task_recruitment_pages.at(page->id);
                    font.text(std::to_string(animation.displayed_count) + "人", 17, 66);
                    const float ratio =
                        std::clamp(state.page_counters.at(page->id) /
                                       static_cast<float>(animation.completion_tick),
                                   0.0F, 1.0F);
                    DrawRectangle(17, 100, 198, 7, GRAY);
                    DrawRectangle(17, 100, static_cast<int>(198 * ratio), 7, GREEN);
                    if (!animation.portraits.empty()) {
                        const auto &human = rules.humans.at(animation.portraits.front());
                        font.text(human.name, 17, 118);
                        const auto profession =
                            world.ai.growth.at(human.identity).definition.current_profession;
                        const auto image = rules.jobs.at(profession).sprites.at(human.sex);
                        sprites.actor(false, 0, image, 0, {196, 141});
                    }
                }
                if (page->legacy_page != 24)
                    font.text("取消", 18, 270);
                if ((page->legacy_page == 25 || page->legacy_page == 26) &&
                    task_selection < static_cast<int>(state.participants.size()))
                    font.text("详情", 100, 270);
                font.text(page->legacy_page == 25
                              ? task_selection == static_cast<int>(state.participants.size())
                                    ? "追加"
                                    : "出发"
                          : page->legacy_page == 26 &&
                                  task_selection == static_cast<int>(state.participants.size())
                              ? "追加"
                              : "确定",
                          190, 270);
            } else if (page->legacy_page == 83)
                font.text("取消", 18, 270);
            else if (page->legacy_page != 97 &&
                     (page->legacy_page != 59 || (state.page_counters.count(page->id) &&
                                                  state.page_counters.at(page->id) >= 70)))
                font.text("确定", 190, 270);
            if ((page->legacy_page == 99 || page->legacy_page == 100) && page->monster_definition) {
                const auto &monster = world.ai.monster_growth.at(*page->monster_definition);
                sprites.actor(true, monster.body * 4 + 1,
                              monster.body * 30 + monster.sprite_variant, 0, {120, 128});
            }
        }
        DrawRectangle(0, 294, width, 26, paper);
        font.text(state.scene.framework_paused ? "继续" : "暂停", 8, 301);
        font.text(state.scene.speed_setting == 1 ? "2倍" : "1倍", 193, 301);
        font.text(state.scene.scene_state == 1 ? "返回" : "建设", 60, 301);
        font.text("任务", 121, 301);
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
                    throw std::runtime_error("共同世界窗口像素读取失败");
                auto *pixels = LoadImageColors(image);
                if (!pixels) {
                    UnloadImage(image);
                    throw std::runtime_error("共同世界窗口像素解码失败");
                }
                std::set<std::uint32_t> colors;
                // 验证地图区，排除HUD、事件正文和月报主体，防止仅有文字也被当作非空地图。
                for (int y = 160 * scale; y < 170 * scale; ++y)
                    for (int x = 0; x < image.width; ++x) {
                        const auto color = pixels[y * image.width + x];
                        colors.insert((static_cast<std::uint32_t>(color.r) << 16) |
                                      (static_cast<std::uint32_t>(color.g) << 8) | color.b);
                    }
                UnloadImageColors(pixels);
                world_pixel_colors = colors.size();
                if (world_pixel_colors < 8) {
                    UnloadImage(image);
                    throw std::runtime_error("共同世界窗口地图像素疑似空白");
                }
                const bool saved = ExportImage(image, screenshot->string().c_str());
                UnloadImage(image);
                if (!saved)
                    throw std::runtime_error("共同世界截图导出失败");
            }
            break;
        }
    }
    std::cout << "world window closed: frames=" << frame_count
              << " updates=" << session.state().scene.world.updates
              << " actors=" << session.state().scene.world.world.ai.human_order.size()
              << " random_draws=" << session.state().scene.random.draws()
              << " funds=" << session.state().scene.world.world.ai.accounting.funds()
              << " moved_frames=" << moved_frames << " map_colors=" << world_pixel_colors
              << " confirm_inputs=" << confirmation_inputs << " pause_inputs=" << pause_inputs
              << " speed_inputs=" << speed_inputs
              << " paused=" << session.state().scene.framework_paused
              << " speed=" << session.state().scene.speed_setting << '\n';
    return 0;
}
} // namespace dungeon_village_prototype
