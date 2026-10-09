// S038–S040 and STEAM_INTERACTIONS establish title -> slots -> manual actions. Dimensions
// adapt published APK artwork to PC windows; two Ark slots remain manual and deletion is disabled.
#include "world_title.hpp"
#include "resources.hpp"
#include "ui/skin.hpp"
#include "ui/world_startup.hpp"
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
          const Camera2D &camera, const app::WorldSystemState &system) {
    skin.sprites.image("title00.png", {0, 0, 240, 330}, layout.background, Sprites::Binding::title);
    skin.sprites.image("title_logo.png", {0, 0, 236, 115}, layout.logo, Sprites::Binding::title);
    if (selection.page == WorldTitlePage::title) {
        skin.sprites.image("title_window.png", {0, 0, 98, 68}, layout.book,
                           Sprites::Binding::title);
        const auto chosen = selection.title_selection == 0 ? layout.start : layout.records;
        DrawRectangleRec(chosen, {255, 155, 100, 255});
        skin.sprites.indexed_image(Sprites::Binding::title, 3, {0, 0, 28, 23},
                                   {chosen.x - 20, chosen.y, 28, 23});
        label(skin, "开始游戏", layout.start);
        label(skin, "纪录", layout.records);
        return;
    }
    if (selection.page == WorldTitlePage::records) {
        ui::draw_world_records(system.records, selection.record_page, skin, layout.panel);
        skin.button(layout.back, "返回");
        return;
    }
    if (selection.page == WorldTitlePage::configure ||
        selection.page == WorldTitlePage::text_edit) {
        ui::draw_world_configuration(selection.draft, selection.field, layout.fields, layout.panel,
                                     skin);
        skin.button(layout.back, "返回");
        if (selection.page == WorldTitlePage::text_edit) {
            EndMode2D();
            skin.text.flush(camera.zoom, camera.offset);
            BeginMode2D(camera);
            skin.content({layout.panel.x + 5, layout.panel.y + 50, layout.panel.width - 10, 100},
                         {40, 45, 60, 255});
            label(skin, selection.field == 0 ? "城镇名称" : "冒险者姓名",
                  {layout.panel.x + 10, layout.panel.y + 55, layout.panel.width - 20, 20}, WHITE);
            label(skin, selection.edit_text + "|",
                  {layout.panel.x + 10, layout.panel.y + 83, layout.panel.width - 20, 30}, WHITE);
            skin.button(layout.accept, "确定");
        }
        if (!selection.feedback.empty())
            label(
                skin, selection.feedback,
                {layout.panel.x, layout.panel.y + layout.panel.height + 2, layout.panel.width, 16},
                MAROON);
        return;
    }
    if (selection.page == WorldTitlePage::overwrite) {
        skin.window(layout.panel, "重新开始");
        label(skin, "重新配置新游戏？",
              {layout.panel.x + 8, layout.panel.y + 50, layout.panel.width - 16, 24});
        label(skin, "存档仅在手动保存时替换",
              {layout.panel.x + 8, layout.panel.y + 80, layout.panel.width - 16, 24});
        skin.button(layout.accept, "确定");
        skin.button(layout.back, "返回");
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
    for (int n = 0; n < 4; ++n)
        out.fields[n] = {out.panel.x + 100, out.panel.y + 105 + n * 20.F, 108, 19};
    out.fields[3] = {out.panel.x + 75, out.panel.y + 165, 75, 19};
    out.previous = {out.panel.x + 8, out.panel.y + 24, 24, 24};
    out.next = {out.panel.x + 190, out.panel.y + 24, 24, 24};
    out.accept = {out.panel.x + 130, out.panel.y + 125, 68, 20};
    return out;
}
std::optional<WorldTitleAction>
world_title_input(WorldTitleSelection &selection, const WorldTitleLayout &layout,
                  const std::array<app::WorldSaveSlotInfo, 2> &slots,
                  const WorldTitleInput &input) {
    const bool back =
        input.back || (selection.page != WorldTitlePage::title && hit(input, layout.back));
    if (back) {
        const auto page = selection.page;
        selection.page = page == WorldTitlePage::text_edit   ? WorldTitlePage::configure
                         : page == WorldTitlePage::configure ? WorldTitlePage::slots
                         : page == WorldTitlePage::overwrite ? WorldTitlePage::actions
                         : page == WorldTitlePage::actions   ? WorldTitlePage::slots
                                                             : WorldTitlePage::title;
        selection.feedback.clear();
        return {};
    }
    if (selection.page == WorldTitlePage::text_edit) {
        if (input.erase && !selection.edit_text.empty()) {
            auto pos = selection.edit_text.size() - 1;
            while (pos && (static_cast<unsigned char>(selection.edit_text[pos]) & 0xc0) == 0x80)
                --pos;
            selection.edit_text.resize(pos);
        }
        if (!input.text.empty()) {
            auto value = selection.edit_text + input.text;
            if (simulation::valid_startup_world_human_profile({value, 0, false}))
                selection.edit_text = std::move(value);
        }
        if (input.confirm || hit(input, layout.accept)) {
            if (selection.field == 0)
                selection.draft.village = selection.edit_text;
            else {
                selection.draft.human.name = selection.edit_text;
                selection.draft.human.custom_name = true;
            }
            selection.page = WorldTitlePage::configure;
        }
        return {};
    }
    if (selection.page == WorldTitlePage::title) {
        if (input.up || input.down)
            selection.title_selection = 1 - selection.title_selection;
        if (hit(input, layout.start))
            selection.title_selection = 0;
        if (hit(input, layout.records))
            selection.title_selection = 1;
        if (input.confirm || hit(input, layout.start) || hit(input, layout.records))
            selection.page =
                selection.title_selection == 0 ? WorldTitlePage::slots : WorldTitlePage::records;
        return {};
    }
    if (selection.page == WorldTitlePage::records) {
        if (input.left || input.right || hit(input, layout.previous) || hit(input, layout.next))
            selection.record_page = 1 - selection.record_page;
        return {};
    }
    if (selection.page == WorldTitlePage::overwrite) {
        if (input.confirm || hit(input, layout.accept))
            selection.page = WorldTitlePage::configure;
        return {};
    }
    if (selection.page == WorldTitlePage::configure) {
        if (input.up)
            selection.field = (selection.field + 3) % 4;
        if (input.down)
            selection.field = (selection.field + 1) % 4;
        if (selection.field == 2 && (input.left || input.right))
            app::change_world_draft_sex(selection.draft, 1 - selection.draft.human.sex);
        bool activate = input.confirm;
        for (int n = 0; n < 4; ++n)
            if (hit(input, layout.fields[n])) {
                selection.field = n;
                activate = true;
            }
        if (!activate)
            return {};
        if (selection.field < 2) {
            selection.edit_text =
                selection.field == 0 ? selection.draft.village : selection.draft.human.name;
            selection.page = WorldTitlePage::text_edit;
        } else if (selection.field == 2)
            app::change_world_draft_sex(selection.draft, 1 - selection.draft.human.sex);
        else {
            selection.draft.slot = selection.slot;
            return WorldTitleAction::new_game;
        }
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
            selection.page =
                slots[selection.slot].exists ? WorldTitlePage::actions : WorldTitlePage::configure;
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
    if (selection.action == 1) {
        selection.page = WorldTitlePage::overwrite;
        return {};
    }
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
    const auto records = app::read_world_system(directory);
    if (!records.records) {
        reason = records.error;
        return false;
    }
    auto next_records = *records.records;
    next_records.last_slot = slot;
    loaded.state->cash_peak = next_records.cash_peak;
    loaded.state->cash_peak_village = next_records.cash_village;
    reason = app::write_world_system(directory, next_records);
    if (!reason.empty())
        return false;
    initial = std::move(*loaded.state);
    return true;
}
bool run_world_title(const app::LaunchOptions &options, const std::filesystem::path &assets,
                     simulation::StartupWorldRuntimeState &initial) {
    const auto directory = options.save_directory.empty()
                               ? app::default_world_save_directory()
                               : std::filesystem::path(options.save_directory);
    const auto loaded_system = app::read_world_system(directory);
    if (!loaded_system.records)
        throw std::runtime_error("系统纪录读取失败：" + loaded_system.error);
    app::WorldSystemState system{*loaded_system.records, {}};
    auto slots = app::inspect_world_save_slots(directory);
    std::string extra = system.records.score_village + system.records.cash_village;
    for (const auto &slot : slots)
        if (slot.metadata)
            extra += slot.metadata->village;
    Sprites sprites(assets);
    Text text(desktop_font_path(assets, options.font), extra);
    ui::Skin skin(sprites, text);
    WorldCanvas canvas;
    WorldTitleSelection selection;
    selection.slot = system.records.last_slot;
    if (options.inspect_page == "world-title-slots" ||
        options.inspect_page == "world-title-actions")
        selection.page = options.inspect_page == "world-title-slots" ? WorldTitlePage::slots
                                                                     : WorldTitlePage::actions;
    if (options.inspect_page == "world-title-records" ||
        options.inspect_page == "world-title-cash") {
        selection.page = WorldTitlePage::records;
        selection.record_page = options.inspect_page == "world-title-cash";
    }
    if (options.inspect_page == "world-title-configure" ||
        options.inspect_page == "world-title-text") {
        selection.page = options.inspect_page == "world-title-text" ? WorldTitlePage::text_edit
                                                                    : WorldTitlePage::configure;
        selection.field = 1;
        selection.edit_text = "UI测试甲";
    }
    std::optional<app::WorldClearPage> clear_inspection;
    if (options.inspect_page == "world-title-clear") {
        const auto rows = simulation::startup_world_clear_score(initial);
        if (!rows.candidate)
            throw std::runtime_error("Clear inspection projection failed");
        clear_inspection = app::WorldClearPage{*rows.candidate, {}, {}, 0};
        // Explicit bounded presentation fixture from the real new world's counts, not a natural
        // clear.
        while (clear_inspection->score.stage != 3 || clear_inspection->score.counter != 45) {
            const auto next = simulation::prepare_startup_clear_score_page(
                clear_inspection->rows, clear_inspection->score, true);
            if (!next.candidate)
                throw std::runtime_error("Clear inspection stage failed");
            clear_inspection->score = *next.candidate;
            ++clear_inspection->animation_counter;
            if (clear_inspection->score.stage == 3 && clear_inspection->score.counter == 44) {
                clear_inspection->score =
                    *simulation::prepare_startup_clear_score_page(clear_inspection->rows,
                                                                  clear_inspection->score, false)
                         .candidate;
                ++clear_inspection->animation_counter;
            }
        }
    }
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
                IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) ||
                (selection.page != WorldTitlePage::text_edit && IsKeyPressed(KEY_SPACE));
            input.back = IsKeyPressed(KEY_ESCAPE) || IsMouseButtonReleased(MOUSE_BUTTON_RIGHT);
            input.up = IsKeyPressed(KEY_UP);
            input.down = IsKeyPressed(KEY_DOWN);
            input.left = IsKeyPressed(KEY_LEFT);
            input.right = IsKeyPressed(KEY_RIGHT);
            input.erase = IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE);
            for (int code = GetCharPressed(); code; code = GetCharPressed()) {
                int length{};
                const char *encoded = CodepointToUTF8(code, &length);
                input.text.append(encoded, static_cast<std::size_t>(length));
            }
        }
        // Refresh at navigation/activation boundaries, not every draw. Continue always re-reads
        // and validates file bytes, even if another process changed a previously displayed slot.
        if (input.confirm || input.click || input.back)
            slots = app::inspect_world_save_slots(directory);
        const auto old_selection = selection;
        const auto action = world_title_input(selection, layout, slots, input);
        if (!text.include_text(selection.draft.village + selection.draft.human.name +
                               selection.edit_text)) {
            selection = old_selection;
            selection.feedback = "当前字体不支持此文字";
        }
        if (action == WorldTitleAction::new_game) {
            selection.feedback =
                app::start_world_draft(initial, system, selection.draft, directory);
            if (selection.feedback.empty()) {
                // A title release/confirm must not also select the first world tile or page.
                PollInputEvents();
                SetTargetFPS(0);
                return true;
            }
            // A failed file write still renders feedback and pumps input this frame.
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
        if (clear_inspection)
            ui::draw_world_clear(*clear_inspection, extent, skin);
        else
            draw(selection, layout, slots, skin, camera, system);
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
