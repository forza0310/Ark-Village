#include "world_save_menu.hpp"
#include "../ui/common/skin.hpp"
#include "../ui/system/world_startup.hpp"
#include "ark/presentation/script_text.hpp"
#include "ark/simulation/presentation/steam_startup_skin.hpp"
#include "ark/simulation/world/startup_world_save.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
namespace {
bool hit(const WorldSaveMenuInput &input, Rectangle box) {
    return input.click && CheckCollisionPointRec(*input.click, box);
}
bool loadable(const app::WorldSaveSlotInfo &slot) {
    return slot.exists && slot.metadata && slot.error == app::WorldSaveError::none;
}
void fitted(const ui::Skin &skin, const std::string &value, Rectangle box, Color color = ui::ink,
            float size = 11) {
    skin.text.draw(value, box.x, box.y, color,
                   std::min(size, 12.F * box.width / std::max(1.F, skin.text.width(value))));
}
void wrapped(const ui::Skin &skin, const std::string &value, Rectangle box, Color color = ui::ink,
             float size = 11) {
    const auto lines = ui::wrap_plain_text(value, box.width, [&](const std::string &text) {
        return skin.text.width(text) * size / 12.F;
    });
    const int count = std::min(static_cast<int>(lines.size()),
                               std::max(1, static_cast<int>(box.height / (size + 3))));
    for (int line = 0; line < count; ++line)
        skin.text.draw(lines[line], box.x, box.y + line * (size + 3), color, size);
}
} // namespace
Rectangle world_source_save_confirm(Extent extent) {
    return {(extent.width - 240.F) / 2 + 80, (extent.height - 240.F) / 2 + 162, 80, 22};
}
bool world_source_save_ready(const app::WorldState &state, std::uint64_t page) {
    const auto view = simulation::inspect_startup_world_save_page(state, page);
    return view && view->stage == 2 && !state.scene.framework_paused;
}
void draw_world_source_save(const app::WorldState &state, std::uint64_t page, Extent extent,
                            const ui::Skin &skin) {
    namespace sim = simulation;
    const auto view = sim::inspect_startup_world_save_page(state, page);
    if (!view)
        return;
    sim::SteamSavePageSkinInput input;
    input.width = extent.width;
    input.height = extent.height;
    input.stage = view->stage;
    if (view->stage == 2)
        input.measured_button_width = skin.text.width("了解");
    const auto plan = sim::steam_save_page_skin(input);
    if (!plan)
        throw std::runtime_error("Invalid source save skin");
    const Vector2 origin{float(plan->origin[0]), float(plan->origin[1])};
    const auto image = [&](const sim::StartupSkinDraw &p) {
        if (p.sprite >= 0)
            skin.sprites.indexed_sprite(Sprites::Binding::steam_common, p.sprite,
                                        std::max(0, p.frame), p.layer, p.image,
                                        {origin.x + p.offset[0], origin.y + p.offset[1]});
        else if (p.crop[2] > 0 && p.crop[3] > 0)
            skin.sprites.indexed_image(
                Sprites::Binding::steam_common, p.image,
                {float(p.crop[0]), float(p.crop[1]), float(p.crop[2]), float(p.crop[3])},
                {origin.x + p.offset[0], origin.y + p.offset[1], float(p.crop[2]),
                 float(p.crop[3])});
    };
    const auto text = [&](const sim::SteamStartupText &p) {
        const auto value = p.role == sim::SteamStartupTextRole::message_body ? view->text
                           : p.role == sim::SteamStartupTextRole::message_title
                               ? std::string("消息")
                               : std::string("了解");
        const auto rgb = p.rgb.value_or(std::array<int, 3>{92, 51, 31});
        const Color tint{static_cast<unsigned char>(rgb[0]), static_cast<unsigned char>(rgb[1]),
                         static_cast<unsigned char>(rgb[2]), 255};
        const auto &r = p.rectangle;
        const float size = p.font_size ? float(p.font_size) : 12.F;
        if (p.placement == sim::SteamStartupTextPlacement::layout) {
            const auto decoded = ui::decode_script_text(value);
            const auto lines =
                ui::wrap_plain_text(decoded.text, float(r[2]),
                                    [&](const auto &line) { return skin.text.width(line, size); });
            const float height = float(lines.size()) * size +
                                 float(std::max(0, int(lines.size()) - 1)) * p.line_space;
            float y = origin.y + float(r[1]) + std::max(0.F, (float(r[3]) - height) / 2);
            for (const auto &line : lines) {
                skin.text.clipped(
                    line, origin.x + float(r[0]) + (float(r[2]) - skin.text.width(line, size)) / 2,
                    y, {origin.x + float(r[0]), origin.y + float(r[1]), float(r[2]), float(r[3])},
                    tint, size);
                y += size + p.line_space;
            }
        } else {
            float x = origin.x + float(r[0]);
            const int anchor = p.anchor.value_or(1);
            if (anchor & 2)
                x -= skin.text.width(value, size) / 2;
            else if (anchor & 4)
                x -= skin.text.width(value, size);
            skin.text.draw(value, x, origin.y + float(r[1]), tint, size);
        }
    };
    const auto frame = [&](const sim::SteamStartupFramePlan &parts) {
        for (const auto &draw : parts.draws) {
            if (const auto *p = std::get_if<sim::StartupSkinRect>(&draw)) {
                const auto &r = p->rect;
                const auto &c = p->rgb;
                const Color tint{static_cast<unsigned char>(c[0]), static_cast<unsigned char>(c[1]),
                                 static_cast<unsigned char>(c[2]), 255};
                const Rectangle box{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
                if (p->outline)
                    DrawRectangleLinesEx(box, 1, tint);
                else
                    DrawRectangleRec(box, tint);
            } else if (const auto *p = std::get_if<sim::StartupSkinDraw>(&draw))
                image(*p);
            else if (const auto *p = std::get_if<sim::SteamStartupText>(&draw))
                text(*p);
        }
    };
    for (const auto &draw : plan->draws) {
        if (const auto *p = std::get_if<sim::SteamStartupWindow>(&draw)) {
            const auto parts = sim::steam_startup_window_skin(
                *p, 23,
                std::array<int, 2>{int(skin.text.width("消息")), int(skin.text.width("消息"))});
            if (!parts)
                throw std::runtime_error("Invalid save window frame");
            frame(*parts);
        } else if (const auto *p = std::get_if<sim::SteamStartupBox>(&draw)) {
            const auto parts = sim::steam_startup_box_skin(*p, 23);
            if (!parts)
                throw std::runtime_error("Invalid save content frame");
            frame(*parts);
        } else if (const auto *p = std::get_if<sim::SteamStartupFill>(&draw)) {
            const auto &r = p->rectangle;
            const auto &c = p->rgb;
            DrawRectangleRec(
                {origin.x + float(r[0]), origin.y + float(r[1]), float(r[2]), float(r[3])},
                {static_cast<unsigned char>(c[0]), static_cast<unsigned char>(c[1]),
                 static_cast<unsigned char>(c[2]), 255});
        } else if (const auto *p = std::get_if<sim::SteamStartupText>(&draw))
            text(*p);
    }
}
WorldSaveMenuLayout world_save_menu_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Save menu requires supported logical viewport");
    const float width = std::min(350.F, extent.width - 16.F);
    const float height = std::min(270.F, extent.height - 40.F);
    WorldSaveMenuLayout out;
    out.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto p = out.panel;
    const float row_height = (height - 82) / 2;
    for (int i = 0; i < 2; ++i) {
        out.slots[i] = {p.x + 10, p.y + 31 + i * (row_height + 3), width - 20, row_height};
        const auto row = out.slots[i];
        out.save[i] = {row.x + row.width - 45, row.y + 3, 42, 20};
        out.load[i] = {row.x + row.width - 45, row.y + 27, 42, 20};
    }
    out.back = {p.x + 10, p.y + height - 25, 48, 20};
    out.confirm = {p.x + width - 64, p.y + height - 25, 54, 20};
    out.records = {p.x + width - 64, p.y + height - 25, 54, 20};
    out.cancel = out.back;
    out.message = {p.x + 10, p.y + height - 44, width - 20, 16};
    return out;
}
void WorldSaveMenu::observe(const app::WorldFrame &frame) {
    if (frame.save_menu_open != opened_ || frame.save_source_page != source_page_) {
        opened_ = frame.save_menu_open;
        source_page_ = frame.save_source_page;
        confirmation_ = Confirmation::none;
        records_open_ = false;
        if (frame.save_source_page)
            load_selected_ = false;
        feedback_.clear();
    }
    if (pending_ && frame.last_command_serial >= pending_) {
        const auto result = std::find_if(frame.command_results.begin(), frame.command_results.end(),
                                         [&](const auto &r) { return r.serial == pending_; });
        if (result != frame.command_results.end() &&
            result->outcome == app::WorldCommandOutcome::rejected && frame.save_message.empty())
            feedback_ = "当前无法操作，请稍后重试";
        pending_ = 0;
    }
}
void WorldSaveMenu::input(const app::WorldFrame &frame, Extent extent,
                          const WorldSaveMenuInput &input, app::WorldSession &session) {
    // A fresh source parent invalidates any previous desktop load/record draft.
    observe(frame);
    if (!frame.save_menu_open || frame.save_busy || pending_ || frame.failed ||
        !frame.system_error.empty())
        return;
    const auto layout = world_save_menu_layout(extent);
    if (records_open_) {
        if (input.escape || hit(input, layout.back))
            records_open_ = false;
        else if (input.left || input.right ||
                 (input.click &&
                  CheckCollisionPointRec(*input.click, {layout.panel.x + 8, layout.panel.y + 24,
                                                        layout.panel.width - 16, 24})))
            record_page_ = 1 - record_page_;
        return;
    }
    if (!frame.save_source_page && confirmation_ == Confirmation::none &&
        hit(input, layout.records)) {
        records_open_ = true;
        record_page_ = 0;
        return;
    }
    if (confirmation_ != Confirmation::none) {
        if (input.escape || hit(input, layout.cancel)) {
            confirmation_ = Confirmation::none;
            return;
        }
        if (input.enter || hit(input, layout.confirm)) {
            pending_ = confirmation_ == Confirmation::overwrite ? session.save_slot(selected_)
                                                                : session.load_slot(selected_);
            confirmation_ = Confirmation::none;
            feedback_.clear();
        }
        return;
    }
    if (input.escape || hit(input, layout.back)) {
        pending_ = session.close_save_menu();
        return;
    }
    if (input.up || input.down)
        selected_ = 1 - selected_;
    if (!frame.save_source_page && (input.left || input.right))
        load_selected_ = !load_selected_;
    bool activate = input.enter;
    for (int slot = 0; slot < 2; ++slot) {
        if (hit(input, layout.slots[slot]))
            selected_ = slot;
        if (hit(input, layout.save[slot]) ||
            (!frame.save_source_page && hit(input, layout.load[slot]))) {
            selected_ = slot;
            load_selected_ = hit(input, layout.load[slot]);
            activate = true;
        }
    }
    if (!activate)
        return;
    const auto &slot = frame.save_slots[selected_];
    feedback_.clear();
    if (load_selected_) {
        if (loadable(slot))
            confirmation_ = Confirmation::load;
        else
            feedback_ = slot.exists ? "此存档无法读取" : "此栏位没有存档";
    } else if (slot.exists)
        confirmation_ = Confirmation::overwrite;
    else
        pending_ = session.save_slot(selected_);
}
void WorldSaveMenu::draw(const app::WorldFrame &frame, Extent extent, const ui::Skin &skin) const {
    if (!frame.save_menu_open)
        return;
    const auto layout = world_save_menu_layout(extent);
    if (records_open_) {
        ui::draw_world_records(frame.system.records, record_page_, skin, layout.panel);
        skin.button(layout.back, "返回");
        return;
    }
    skin.window(layout.panel, frame.save_source_page ? "保存游戏" : "保存 / 读取");
    const bool enabled = !frame.save_busy && !pending_ && !frame.failed;
    if (confirmation_ != Confirmation::none) {
        const auto &p = layout.panel;
        const Rectangle body{p.x + 12, p.y + 40, p.width - 24, p.height - 76};
        skin.content(body);
        const auto title = std::string("手动存档 ") + std::to_string(selected_ + 1);
        fitted(skin, title, {body.x + 5, body.y + 8, body.width - 10, 18}, ui::blue);
        wrapped(skin,
                confirmation_ == Confirmation::overwrite ? "覆盖此栏位的已有存档？原存档将被替换。"
                                                         : "读取此存档？当前未保存的进度将被替换。",
                {body.x + 5, body.y + 35, body.width - 10, body.height - 40});
        skin.button(layout.cancel, "取消", enabled);
        skin.button(layout.confirm, "确定", enabled);
        return;
    }
    for (int n = 0; n < 2; ++n) {
        const auto row = layout.slots[n];
        const auto &slot = frame.save_slots[n];
        skin.content(row, n == selected_ ? Color{255, 236, 174, 255} : Color{250, 254, 248, 255});
        const float text_width = row.width - 54;
        fitted(skin, "手动存档 " + std::to_string(n + 1), {row.x + 4, row.y + 3, text_width, 12},
               ui::blue, 10);
        if (slot.metadata) {
            const auto &m = *slot.metadata;
            fitted(skin, m.village, {row.x + 4, row.y + 17, text_width, 12}, ui::ink, 10);
            fitted(skin,
                   std::to_string(m.year + 1) + "年" + std::to_string(m.month + 1) + "月" +
                       std::to_string(m.week + 1) + "周  " + std::to_string(m.funds) + "G",
                   {row.x + 4, row.y + 32, text_width, 12}, ui::ink, 10);
        } else
            fitted(skin, slot.exists ? "存档不可读取" : "空栏位",
                   {row.x + 4, row.y + 23, text_width, 14}, slot.exists ? MAROON : ui::ink, 10);
        if (slot.error != app::WorldSaveError::none)
            fitted(skin, slot.message.empty() ? "读取失败" : slot.message,
                   {row.x + 4, row.y + row.height - 14, text_width, 12}, MAROON, 9);
        skin.button(layout.save[n], n == selected_ && !load_selected_ ? ">保存" : "保存", enabled);
        if (!frame.save_source_page)
            skin.button(layout.load[n], n == selected_ && load_selected_ ? ">读取" : "读取",
                        enabled && loadable(slot));
    }
    const auto &message = !feedback_.empty() ? feedback_ : frame.save_message;
    const Color message_color =
        feedback_.empty() && frame.save_message == "保存完毕" ? ui::blue : MAROON;
    fitted(skin, frame.save_busy || pending_ ? "处理中，请稍候" : message, layout.message,
           message.empty() ? ui::ink : message_color, 10);
    skin.button(layout.back, "返回", enabled);
    if (!frame.save_source_page)
        skin.button(layout.records, "纪录", enabled);
}
} // namespace ark::desktop
