// Canonical world owns simulation and page effects; this adapter owns only window/input/raster.
#include "world_view.hpp"
#include "ark/app/simulation_clock.hpp"
#include "desktop_session.hpp"
#include "ui/layout.hpp"
#include "ui/skin.hpp"
#include "world_scene.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace ark::desktop {
namespace {
namespace rules = simulation::rules;
using State = simulation::StartupWorldRuntimeState;
struct WorldWindow {
    explicit WorldWindow(const app::LaunchOptions &options) {
        require_display();
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
        InitWindow(options.width, options.height, "Ark-Village - World");
        if (!IsWindowReady())
            throw std::runtime_error("Cannot initialize world window");
        SetWindowMinSize(240, 256);
        SetExitKey(KEY_NULL);
        SetTargetFPS(0);
    }
    ~WorldWindow() { CloseWindow(); }
};
struct WorldCanvas {
    RenderTexture2D texture{};
    Extent size{};
    ~WorldCanvas() {
        if (texture.id)
            UnloadRenderTexture(texture);
    }
    void resize(Extent next) {
        if (texture.id && next.width == size.width && next.height == size.height)
            return;
        auto created = LoadRenderTexture(next.width, next.height);
        if (!created.id)
            throw std::runtime_error("Cannot allocate world framebuffer");
        SetTextureFilter(created.texture, TEXTURE_FILTER_POINT);
        if (texture.id)
            UnloadRenderTexture(texture);
        texture = created;
        size = next;
    }
};
const rules::WorldScriptPage *active_page(const State &s) {
    const auto found = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                    [](const auto &page) { return page.lifecycle != 4; });
    if (found == s.scripts.pages.rend() || found->kind == rules::WorldScriptPageKind::scene)
        return nullptr;
    return &*found;
}
bool advance(State &s) {
    auto candidate = simulation::prepare_startup_world_runtime(s);
    if (!candidate.candidate ||
        !simulation::update_startup_world_render_cache(*candidate.candidate)) {
        std::cerr << "World update rejected: runtime=" << static_cast<int>(candidate.error)
                  << " scene=" << static_cast<int>(candidate.scene_error)
                  << " world=" << static_cast<int>(candidate.world_error)
                  << " rounds=" << s.simulation_steps << '\n';
        return false;
    }
    s = std::move(*candidate.candidate);
    return true;
}
std::string glyphs(const State &s) {
    std::string result =
        "本月结算打倒怪物获得村子点数收入支出收支成果入手当前活动尚未接入确定姓名打倒数下降倍完成";
    result += s.rules->script_sources.talks + s.rules->script_sources.news +
              s.rules->script_sources.event_messages;
    for (const auto &h : s.rules->humans)
        result += h.name;
    for (const auto &t : s.rules->tasks)
        result += t.name + t.title;
    for (const auto &i : s.rules->items)
        result += i.name;
    for (const auto &e : s.rules->equipment)
        result += e.name;
    return result;
}
std::string plain_text(const std::string &source) {
    std::string text;
    for (std::size_t i = 0; i < source.size();) {
        if (source.compare(i, 4, "<co=") == 0 || source.compare(i, 5, "</co>") == 0) {
            const auto end = source.find('>', i);
            if (end != std::string::npos) {
                i = end + 1;
                continue;
            }
        }
        text += source[i++];
    }
    return text;
}
std::vector<std::string> lines(const Text &text, const std::string &source, float width) {
    std::vector<std::string> result;
    std::string line;
    const auto value = plain_text(source);
    for (std::size_t i = 0; i < value.size();) {
        int bytes{};
        GetCodepointNext(value.c_str() + i, &bytes);
        const auto next = value.substr(i, static_cast<std::size_t>(bytes));
        if (next == "\n" || (!line.empty() && text.width(line + next) > width)) {
            result.push_back(line);
            line.clear();
        }
        if (next != "\n")
            line += next;
        i += static_cast<std::size_t>(bytes);
    }
    result.push_back(line);
    return result;
}
void hud(const State &s, const ui::Layout &layout, const ui::Skin &skin, bool failed) {
    const float w = layout.extent.width, h = layout.extent.height;
    skin.tile("top_bar.png", {22, 0, 90, 24}, {22, 0, w - 150, 24});
    skin.sprites.image("top_bar.png", {0, 0, 22, 24}, {0, 0, 22, 24});
    skin.sprites.image("top_bar.png", {112, 0, 128, 24}, {w - 128, 0, 128, 24});
    skin.text.draw(std::to_string(s.scene.calendar.year + 1) + "年" +
                       std::to_string(s.scene.calendar.month + 1) + "月 " +
                       std::to_string(s.scene.calendar.subperiod + 1),
                   27, 6);
    skin.right(std::to_string(s.scene.world.world.ai.accounting.funds()) + "G", w - 7, 6);
    skin.sprites.image("townPointbar.png", {0, 0, 55, 15}, {w - 55, 24, 55, 15});
    skin.number(s.village_points, {w - 3, 27});
    skin.tile("btmbar.png", {116, 1, 4, 20}, {0, h - 21, w, 20});
    skin.sprites.image("btmbar_popular00.png", {0, 0, 78, 41}, {w / 2 - 39, h - 41, 78, 41});
    skin.sprites.image("btmbar_popular01.png", {0, 0, 77, 14}, {w / 2 - 38, h - 16, 77, 14});
    skin.number(s.popularity, {w / 2 + 35, h - 20});
    skin.button(layout.left_button, s.scene.framework_paused ? "继续" : "暂停", !failed);
    skin.button(layout.right_button, s.scene.speed_setting == 1 ? "2倍" : "1倍", !failed);
    if (failed)
        skin.centered("当前活动尚未接入", {8, 46, w - 16, 20}, MAROON);
    if (s.report_state && !active_page(s)) {
        const Rectangle box{(w - 220) / 2, 72, 220, 112};
        skin.window(box, "本月结算");
        constexpr const char *labels[]{"打倒怪物", "获得村子点数", "收入", "支出", "收支"};
        const int first = s.report_state == 1 ? 0 : 2, count = s.report_state == 1 ? 2 : 3;
        for (int i = 0; i < count; ++i) {
            const auto y = box.y + 28 + i * 24;
            skin.text.draw(labels[first + i], box.x + 10, y);
            skin.right(std::to_string(s.report_snapshot[first + i]) +
                           (s.report_state == 1 ? "" : "G"),
                       box.x + box.width - 10, y);
        }
    }
}
} // namespace

void run_world_game(const app::LaunchOptions &options, const std::filesystem::path &assets) {
    WorldWindow window(options);
    float zoom = options.zoom_percent / 100.F;
    Extent extent = canvas_extent(GetScreenWidth(), GetScreenHeight());
    // Initialization is temporary; this value is the sole persistent canonical world.
    State state = [] {
        simulation::StartupSession initial;
        simulation::StartupWorldRuntimeSession session(initial.state(),
                                                       rules::WorldRandomStream::from_java_seed(1));
        return session.state();
    }();
    // Visibility affects source decisions, so inspection uses the actual window before any round.
    state.reference_viewport = world_viewport(extent, zoom);
    if (options.inspect_page == "world-active" || options.inspect_page == "world-month") {
        bool reached{};
        for (int step = 0; step < 20000; ++step) {
            if (!advance(state))
                throw std::runtime_error("World inspection failed before its real target state");
            if (const auto *page = active_page(state); page && page->legacy_page != 56) {
                if (simulation::acknowledge_startup_world_runtime_page(state, page->id) !=
                    simulation::StartupWorldRuntimeError::none)
                    throw std::runtime_error("World inspection page consumer rejected input");
            }
            reached = options.inspect_page == "world-month"
                          ? state.report_state != 0
                          : state.scene.world.world.ai.human_order.size() >= 3;
            if (reached)
                break;
        }
        if (!reached)
            throw std::runtime_error("World inspection did not reach its bounded target");
    }
    WorldCanvas canvas;
    Sprites sprites(assets);
    Text text(options.font.empty()
                  ? std::filesystem::path("/System/Library/Fonts/Supplemental/Arial Unicode.ttf")
                  : std::filesystem::path(options.font),
              glyphs(state));
    ui::Skin skin(sprites, text);
    state.scene.framework_paused = options.paused;
    app::SimulationClock clock;
    int frames{}, paragraph{}, scroll{};
    std::uint64_t viewed_page{};
    bool failed{};
    const auto started = GetTime();
    auto next_render = started;
    const auto update = [&](double now) {
        if (clock.advance(now, true) && !failed)
            failed = !advance(state);
    };
    while (!WindowShouldClose() && (options.frames == 0 || frames < options.frames)) {
        const auto now = GetTime();
        if (now >= next_render) {
            next_render = now + 1.0 / 60;
            extent = canvas_extent(GetScreenWidth(), GetScreenHeight());
            state.reference_viewport = world_viewport(extent, zoom);
            canvas.resize({GetRenderWidth(), GetRenderHeight()});
            const auto destination = viewport(GetScreenWidth(), GetScreenHeight(), extent);
            const auto raster =
                canvas_camera(viewport(canvas.size.width, canvas.size.height, extent), extent);
            text.prepare(raster.zoom);
            const ui::Layout layout(extent);
            const auto mouse = logical_mouse(GetMousePosition(), destination, extent);
            const bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
            const auto hit = [&](Rectangle rectangle) {
                return mouse && click && CheckCollisionPointRec(*mouse, rectangle);
            };
            if (!failed && (hit(layout.left_button) || IsKeyPressed(KEY_SPACE)))
                state.scene.framework_paused = !state.scene.framework_paused;
            if (!failed && hit(layout.right_button))
                state.scene.speed_setting = state.scene.speed_setting == 1 ? 0 : 1;
            if (mouse && CheckCollisionPointRec(*mouse, layout.scene) && !active_page(state)) {
                if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                    const float factor = destination.width / extent.width * zoom;
                    state.camera[0] -= GetMouseDelta().x / factor;
                    state.camera[1] += GetMouseDelta().y / factor;
                }
                if (const auto wheel = GetMouseWheelMove(); wheel)
                    world_zoom_at(state, extent, *mouse, wheel, zoom);
            }
            const Rectangle panel{(extent.width - std::min(310, extent.width - 16)) / 2.F, 70,
                                  static_cast<float>(std::min(310, extent.width - 16)),
                                  static_cast<float>(std::min(152, extent.height - 112))};
            const Rectangle confirm{panel.x + panel.width - 64, panel.y + panel.height - 28, 58,
                                    23};
            if (const auto *page = active_page(state)) {
                if (viewed_page != page->id) {
                    viewed_page = page->id;
                    paragraph = scroll = 0;
                }
                if (!state.scene.framework_paused && !failed && page->legacy_page != 56 &&
                    (hit(confirm) || IsKeyPressed(KEY_ENTER))) {
                    if (paragraph + 1 < static_cast<int>(page->paragraphs.size())) {
                        ++paragraph;
                        scroll = 0;
                    } else {
                        const auto page_id = page->id;
                        const auto error =
                            simulation::acknowledge_startup_world_runtime_page(state, page_id);
                        if (error != simulation::StartupWorldRuntimeError::none) {
                            failed = true;
                            std::cerr << "World page rejected: page=" << page_id
                                      << " error=" << static_cast<int>(error) << '\n';
                        }
                    }
                }
                if (mouse && CheckCollisionPointRec(*mouse, panel))
                    scroll = std::max(0, scroll - static_cast<int>(GetMouseWheelMove() * 2));
            }
            update(now);
            BeginTextureMode(canvas.texture);
            ClearBackground(Color{145, 211, 247, 255});
            BeginMode2D(raster);
            const auto clip = layout.scene_clip;
            BeginScissorMode(static_cast<int>(std::floor(raster.offset.x + clip.x * raster.zoom)),
                             static_cast<int>(std::floor(raster.offset.y + clip.y * raster.zoom)),
                             static_cast<int>(std::ceil(clip.width * raster.zoom)),
                             static_cast<int>(std::ceil(clip.height * raster.zoom)));
            draw_world_scene(state, sprites, zoom);
            EndScissorMode();
            hud(state, layout, skin, failed);
            if (const auto *page = active_page(state)) {
                std::string title = page->title;
                if (title.empty())
                    title = page->legacy_page == 94 ? "入手!" : "";
                skin.window(panel, plain_text(title));
                std::string body;
                if (!page->paragraphs.empty())
                    body = page->paragraphs.at(
                        std::min(paragraph, static_cast<int>(page->paragraphs.size()) - 1));
                if (page->task_definition &&
                    (page->legacy_page == 30 || page->legacy_page == 31 || page->legacy_page == 32))
                    body += state.rules->tasks.at(*page->task_definition).name + "完成!";
                const auto wrapped = lines(text, body, panel.width - 20);
                const int visible = std::max(1, static_cast<int>((panel.height - 59) / 17));
                scroll =
                    std::clamp(scroll, 0, std::max(0, static_cast<int>(wrapped.size()) - visible));
                for (int row = 0; row < visible && scroll + row < static_cast<int>(wrapped.size());
                     ++row)
                    text.draw(wrapped[scroll + row], panel.x + 10, panel.y + 24 + row * 17);
                skin.button(confirm, "确定",
                            !state.scene.framework_paused && !failed && page->legacy_page != 56);
            }
            EndMode2D();
            text.flush(raster.zoom, raster.offset);
            EndTextureMode();
            BeginDrawing();
            ClearBackground(BLACK);
            DrawTexturePro(
                canvas.texture.texture,
                {0, 0, static_cast<float>(canvas.size.width),
                 -static_cast<float>(canvas.size.height)},
                {0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
                {0, 0}, 0, WHITE);
            EndDrawing();
            ++frames;
        } else
            update(now);
        const auto delay = std::min(next_render - GetTime(), clock.remaining_seconds(GetTime()));
        if (delay > 0)
            WaitTime(delay);
    }
    if (!options.screenshot.empty()) {
        if (frames != options.frames)
            throw std::runtime_error("World window closed before bounded capture");
        auto screenshot = LoadImageFromScreen();
        auto pixels = screenshot.data ? LoadImageColors(screenshot) : nullptr;
        bool nonblank{};
        if (pixels)
            for (std::int64_t i = 0;
                 i < static_cast<std::int64_t>(screenshot.width) * screenshot.height; ++i)
                if (pixels[i].r || pixels[i].g || pixels[i].b) {
                    nonblank = true;
                    break;
                }
        if (pixels)
            UnloadImageColors(pixels);
        const bool saved = nonblank && ExportImage(screenshot, options.screenshot.c_str());
        if (screenshot.data)
            UnloadImage(screenshot);
        if (!saved)
            throw std::runtime_error("World screenshot is blank or cannot be exported");
    }
    std::cout << "World render: window=" << GetScreenWidth() << 'x' << GetScreenHeight()
              << " framebuffer=" << GetRenderWidth() << 'x' << GetRenderHeight()
              << " canvas=" << canvas.size.width << 'x' << canvas.size.height << '\n';
    std::cout << "World window: frames=" << frames << " rounds=" << state.simulation_steps
              << " cash=" << state.scene.world.world.ai.accounting.funds()
              << " humans=" << state.scene.world.world.ai.human_order.size()
              << " monsters=" << state.scene.world.world.ai.monster_order.size()
              << " random=" << state.scene.random.draws() << " failed=" << failed
              << " elapsed=" << GetTime() - started << '\n';
    if (failed)
        throw std::runtime_error("World runtime rejected an update or page consumer");
}
} // namespace ark::desktop
