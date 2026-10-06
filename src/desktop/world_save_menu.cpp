#include "world_save_menu.hpp"
#include "ui/script_text.hpp"
#include "ui/skin.hpp"
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
    out.cancel = out.back;
    out.message = {p.x + 10, p.y + height - 44, width - 20, 16};
    return out;
}
void WorldSaveMenu::observe(const app::WorldFrame &frame) {
    if (frame.save_menu_open != opened_) {
        opened_ = frame.save_menu_open;
        confirmation_ = Confirmation::none;
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
    if (!frame.save_menu_open || frame.save_busy || pending_ || frame.failed)
        return;
    const auto layout = world_save_menu_layout(extent);
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
    if (input.left || input.right)
        load_selected_ = !load_selected_;
    bool activate = input.enter;
    for (int slot = 0; slot < 2; ++slot) {
        if (hit(input, layout.slots[slot]))
            selected_ = slot;
        if (hit(input, layout.save[slot]) || hit(input, layout.load[slot])) {
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
    skin.window(layout.panel, "系统 - 保存 / 读取");
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
        skin.button(layout.load[n], n == selected_ && load_selected_ ? ">读取" : "读取",
                    enabled && loadable(slot));
    }
    const auto &message = !feedback_.empty() ? feedback_ : frame.save_message;
    const Color message_color =
        feedback_.empty() && frame.save_message == "保存完毕" ? ui::blue : MAROON;
    fitted(skin, frame.save_busy || pending_ ? "处理中，请稍候" : message, layout.message,
           message.empty() ? ui::ink : message_color, 10);
    skin.button(layout.back, "返回", enabled);
}
} // namespace ark::desktop
