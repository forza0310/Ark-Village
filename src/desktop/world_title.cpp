// S038–S040 and STEAM_INTERACTIONS establish title -> slots -> manual actions. Dimensions
// adapt published APK artwork to PC windows; two Ark slots remain manual and deletion is disabled.
#include "world_title.hpp"
#include "resources.hpp"
#include "ui/skin.hpp"
#include "world_canvas.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace ark::desktop {
namespace {
bool hit(const WorldTitleInput &input, Rectangle box) {
    return input.click && CheckCollisionPointRec(*input.click, box);
}
bool loadable(const app::WorldSaveSlotInfo &slot) {
    return slot.exists && slot.metadata && slot.error == app::WorldSaveError::none;
}
void label(const ui::Skin &skin, const std::string &text, Rectangle box, Color color = ui::ink) {
    skin.centered(text, box, color,
                  std::min(12.F, 12.F * (box.width - 8) / std::max(1.F, skin.text.width(text))));
}
void draw(const WorldTitleSelection &selection, const WorldTitleLayout &layout,
          const std::array<app::WorldSaveSlotInfo, 2> &slots, const ui::Skin &skin,
          const Camera2D &camera) {
    skin.sprites.image("title00.png", {0, 0, 240, 330}, layout.background, Sprites::Binding::title);
    skin.sprites.image("title_logo.png", {0, 0, 236, 115}, layout.logo, Sprites::Binding::title);
    if (selection.page == WorldTitlePage::title) {
        skin.sprites.image("title_window.png", {0, 0, 98, 68}, layout.book,
                           Sprites::Binding::title);
        DrawRectangleRec(layout.start, {255, 155, 100, 255});
        label(skin, "开始游戏", layout.start);
        label(skin, "纪录", layout.records, GRAY);
        return;
    }
    skin.window(layout.panel, "选择存档");
    for (int n = 0; n < 2; ++n) {
        const auto box = layout.slots[n];
        skin.content(box,
                     n == selection.slot ? Color{224, 218, 255, 255} : Color{250, 254, 248, 255});
        if (n == selection.slot)
            skin.sprites.draw("finger_r.seb", 0, {box.x - 4, box.y + 14}, WHITE,
                              Sprites::Binding::common);
        const auto &slot = slots[n];
        skin.text.draw("手动存档 " + std::to_string(n + 1), box.x + 6, box.y + 5, ui::blue, 10);
        if (slot.metadata) {
            const auto &m = *slot.metadata;
            label(skin, m.village, {box.x + 5, box.y + 20, box.width - 10, 16});
            label(skin,
                  std::to_string(m.year + 1) + "年" + std::to_string(m.month + 1) + "月" +
                      std::to_string(m.week + 1) + "周  " + std::to_string(m.funds) + "G",
                  {box.x + 5, box.y + 38, box.width - 10, 16});
        } else
            label(skin, slot.exists ? "存档无法读取" : "新游戏",
                  {box.x + 5, box.y + 24, box.width - 10, 24}, slot.exists ? MAROON : ui::ink);
    }
    if (selection.page == WorldTitlePage::actions) {
        // Text is deferred. Flush the underlying slots before the opaque child menu so
        // their labels cannot reappear over Continue/New/Delete at the end of the frame.
        EndMode2D();
        skin.text.flush(camera.zoom, camera.offset);
        BeginMode2D(camera);
        const auto first = layout.actions.front(), last = layout.actions.back();
        skin.content(
            {first.x - 3, first.y - 3, first.width + 6, last.y + last.height - first.y + 6},
            {227, 188, 91, 255});
        for (int n = 0; n < 3; ++n) {
            const auto box = layout.actions[n];
            if (selection.action == n)
                DrawRectangleRec(box, {255, 219, 93, 255});
            const bool enabled = n == 1 || (n == 0 && loadable(slots[selection.slot]));
            label(skin,
                  n == 0   ? "继续游戏"
                  : n == 1 ? "新游戏"
                           : "删除",
                  box, enabled ? ui::ink : GRAY);
        }
    }
    skin.button(layout.back, "返回");
    if (!selection.feedback.empty())
        label(skin, selection.feedback,
              {layout.panel.x, layout.panel.y + layout.panel.height + 2, layout.panel.width, 16},
              MAROON);
}
} // namespace
WorldTitleLayout world_title_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Title requires supported viewport");
    const float w = extent.width, h = extent.height;
    const float background_scale = std::max(w / 240, h / 330);
    const float logo_width = std::min({w - 24, 320.F, h * .48F * 236 / 115});
    WorldTitleLayout out;
    out.background = {(w - 240 * background_scale) / 2, h - 330 * background_scale,
                      240 * background_scale, 330 * background_scale};
    out.logo = {(w - logo_width) / 2, 12, logo_width, logo_width * 115 / 236};
    out.book = {w / 2, h - 94, 112, 78};
    out.start = {out.book.x + 13, out.book.y + 15, 86, 21};
    out.records = {out.book.x + 13, out.book.y + 42, 86, 21};
    out.panel = {(w - 222) / 2, (h - 190) / 2, 222, 190};
    for (int n = 0; n < 2; ++n)
        out.slots[n] = {out.panel.x + 10, out.panel.y + 30 + n * 73, 202, 66};
    out.back = {w - 58, h - 25, 54, 21};
    for (int n = 0; n < 3; ++n)
        out.actions[n] = {out.panel.x + 109, out.panel.y + 102 + n * 23, 100, 23};
    return out;
}
std::optional<WorldTitleAction>
world_title_input(WorldTitleSelection &selection, const WorldTitleLayout &layout,
                  const std::array<app::WorldSaveSlotInfo, 2> &slots,
                  const WorldTitleInput &input) {
    if (input.back || (selection.page != WorldTitlePage::title && hit(input, layout.back))) {
        selection.page = selection.page == WorldTitlePage::actions ? WorldTitlePage::slots
                                                                   : WorldTitlePage::title;
        selection.feedback.clear();
        return {};
    }
    if (selection.page == WorldTitlePage::title) {
        if (input.confirm || hit(input, layout.start))
            selection.page = WorldTitlePage::slots;
        return {};
    }
    if (selection.page == WorldTitlePage::slots) {
        if (input.up || input.down)
            selection.slot = 1 - selection.slot;
        bool activate = input.confirm;
        for (int n = 0; n < 2; ++n)
            if (hit(input, layout.slots[n])) {
                selection.slot = n;
                activate = true;
            }
        if (activate) {
            selection.feedback.clear();
            if (!slots[selection.slot].exists)
                return WorldTitleAction::new_game;
            selection.page = WorldTitlePage::actions;
            selection.action = loadable(slots[selection.slot]) ? 0 : 1;
        }
        return {};
    }
    if (input.up || input.down)
        selection.action = 1 - selection.action;
    bool activate = input.confirm;
    for (int n = 0; n < 2; ++n)
        if (hit(input, layout.actions[n])) {
            selection.action = n;
            activate = true;
        }
    if (!activate)
        return {};
    if (selection.action == 1)
        return WorldTitleAction::new_game; // Starting does not write, overwrite or delete a slot.
    if (loadable(slots[selection.slot]))
        return WorldTitleAction::load;
    selection.feedback = "此存档无法读取";
    return {};
}
bool load_world_title_slot(const std::filesystem::path &directory, int slot,
                           simulation::StartupWorldRuntimeState &initial, std::string &reason) {
    auto loaded = app::read_world_save_slot(directory, slot);
    if (!loaded.state) {
        reason = loaded.message;
        return false;
    }
    if (app::prepare_world_save_candidate(*loaded.state, initial, reason) !=
        app::WorldSaveError::none)
        return false;
    initial = std::move(*loaded.state);
    return true;
}
bool run_world_title(const app::LaunchOptions &options, const std::filesystem::path &assets,
                     simulation::StartupWorldRuntimeState &initial) {
    const auto directory = options.save_directory.empty()
                               ? app::default_world_save_directory()
                               : std::filesystem::path(options.save_directory);
    auto slots = app::inspect_world_save_slots(directory);
    std::string extra;
    for (const auto &slot : slots)
        if (slot.metadata)
            extra += slot.metadata->village;
    Sprites sprites(assets);
    Text text(desktop_font_path(assets, options.font), extra);
    ui::Skin skin(sprites, text);
    WorldCanvas canvas;
    WorldTitleSelection selection;
    if (options.inspect_page == "world-title-slots" ||
        options.inspect_page == "world-title-actions")
        selection.page = options.inspect_page == "world-title-slots" ? WorldTitlePage::slots
                                                                     : WorldTitlePage::actions;
    SetTargetFPS(60);
    int frames{};
    while (!WindowShouldClose() && (!options.frames || frames < options.frames)) {
        const auto extent = canvas_extent(GetScreenWidth(), GetScreenHeight());
        const auto layout = world_title_layout(extent);
        const auto destination = viewport(GetScreenWidth(), GetScreenHeight(), extent);
        canvas.resize({GetRenderWidth(), GetRenderHeight()});
        const auto camera =
            canvas_camera(viewport(canvas.size.width, canvas.size.height, extent), extent);
        WorldTitleInput input;
        if (IsWindowFocused() && options.inspect_page.empty()) {
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
                input.click = logical_mouse(GetMousePosition(), destination, extent);
            input.confirm =
                IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE);
            input.back = IsKeyPressed(KEY_ESCAPE) || IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
            input.up = IsKeyPressed(KEY_UP);
            input.down = IsKeyPressed(KEY_DOWN);
        }
        // Refresh at navigation/activation boundaries, not every draw. Continue always re-reads
        // and validates file bytes, even if another process changed a previously displayed slot.
        if (input.confirm || input.click || input.back)
            slots = app::inspect_world_save_slots(directory);
        const auto action = world_title_input(selection, layout, slots, input);
        if (action == WorldTitleAction::new_game) {
            // A title release/confirm must not also select the first world tile or page.
            PollInputEvents();
            SetTargetFPS(0);
            return true;
        }
        if (action == WorldTitleAction::load) {
            std::string reason;
            if (load_world_title_slot(directory, selection.slot, initial, reason)) {
                PollInputEvents();
                SetTargetFPS(0);
                return true;
            }
            selection.feedback = "读取失败：" + reason;
            slots = app::inspect_world_save_slots(directory);
        }
        text.prepare(camera.zoom);
        BeginTextureMode(canvas.texture);
        ClearBackground(BLACK);
        BeginMode2D(camera);
        draw(selection, layout, slots, skin, camera);
        EndMode2D();
        text.flush(camera.zoom, camera.offset);
        EndTextureMode();
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(
            canvas.texture.texture,
            {0, 0, static_cast<float>(canvas.size.width), -static_cast<float>(canvas.size.height)},
            {0, 0, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
            {0, 0}, 0, WHITE);
        EndDrawing();
        ++frames;
    }
    if (options.frames && frames >= options.frames && !options.screenshot.empty()) {
        auto image = LoadImageFromScreen();
        const bool saved = image.data && ExportImage(image, options.screenshot.c_str());
        if (image.data)
            UnloadImage(image);
        if (!saved)
            throw std::runtime_error("Cannot export title screenshot");
    }
    SetTargetFPS(0);
    std::cout << "World title: frames=" << frames << " world_updates=0\n";
    return false;
}
} // namespace ark::desktop
