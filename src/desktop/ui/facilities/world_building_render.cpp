// Render a frozen building view; no input polling, world writes or animation clock updates.
#include "ark/simulation/actors/startup_world_profile.hpp"
// Page21/74/80/81 fields and actions follow the frozen 2b479f6 maintained prototype.
// Drawing never initializes a page, charges money or changes a facility's shared level.
#include "../../scene/world_overlay_render.hpp"
#include "../common/skin.hpp"
#include "world_building.hpp"
#include "world_facility_detail.hpp"
#include "world_facility_upgrade.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
void fitted(const Skin &skin, const std::string &text, Rectangle box, Color color = ink) {
    const float size = std::min(12.F, 12.F * box.width / std::max(1.F, skin.text.width(text)));
    skin.text.draw(text, box.x, box.y, color, size);
}
void detail_field(Rectangle box, Color fill, Color border) {
    DrawRectangleRec(box, fill);
    DrawRectangleLinesEx(box, 1, border);
}
void draw_facility_footer(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                          const Skin &skin) {
    if (!view.initialized || !view.facility || view.definition_preview)
        return;
    const auto name = layout.footer_name, profit = layout.footer_profit;
    // Cover only the footer strip, leaving the return key and the world above it intact.
    skin.tile("btmbar.png", {116, 1, 4, 20},
              {name.x, name.y, profit.x + profit.width - name.x, name.height});
    skin.content(name, {255, 255, 222, 255});
    fitted(skin, view.title, {name.x + 3, name.y + 2, name.width - 6, 15});
    if (!view.cumulative_profit)
        return; // A non-business facility has no invented zero-profit field.
    fitted(skin, "收益", {profit.x + 2, profit.y + 2, 26, 15});
    const auto value = *view.cumulative_profit;
    auto digits = std::to_string(value);
    if (value < 0)
        digits.erase(0, 1); // Magnitude without signed negation, including INT64_MIN.
    const char *sprite = value < 0 ? "number12.seb" : "number05.seb";
    const float width = 8.F * (digits.size() + 1);
    const float scale = std::min(1.F, (profit.width - 31) / width);
    float x = profit.x + profit.width - 2 - width * scale;
    const float y = profit.y + (profit.height - 10 * scale) / 2;
    for (const char digit : digits) {
        skin.sprites.draw(sprite, digit - '0', {x, y}, WHITE, Sprites::Binding::common, scale);
        x += 8 * scale;
    }
    skin.sprites.draw(sprite, 20, {x, y}, WHITE, Sprites::Binding::common, scale);
}
// Home retains its existing static subset; other74 templates require the maintained plan.
void draw_home_detail(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                      const Skin &skin) {
    if (view.detail_type != app::WorldFacilityTemplate::home)
        throw std::invalid_argument("Facility detail requires its measured source plan");
    const auto boxes = world_building_detail_layout(layout, view.detail_type);
    if (view.category_icon)
        draw_world_visuals({*view.category_icon}, skin.sprites,
                           {layout.panel.x + 13, layout.panel.y + 22}, 1);
    fitted(skin, view.title, boxes.name, ink);
    detail_field(boxes.picture, {226, 247, 212, 255}, {184, 211, 168, 255});
    const auto pieces = simulation::steam_facility_mapchip2_draws({view.mapchip, {49, 37}, 0});
    if (!pieces)
        throw std::invalid_argument("Home detail has invalid mapchip");
    for (const auto &piece : *pieces)
        skin.sprites.draw(
            piece.sprite, piece.frame,
            {boxes.picture.x + piece.position[0], boxes.picture.y + piece.position[1]}, WHITE,
            Sprites::Binding::map, 1, -1, boxes.picture);
    skin.centered("住宅", boxes.remaining, ink, 11);
}
} // namespace
void draw_world_building(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                         const Skin &skin, const WorldBuildingSelection &selection, bool enabled,
                         const std::string &feedback) {
    if (view.detail_skin) {
        draw_world_facility_detail(view, layout, skin, selection);
        draw_facility_footer(view, layout, skin);
        skin.button(layout.cancel, "返回", enabled);
        if (!feedback.empty())
            fitted(skin, feedback, {layout.body.x, layout.panel.y + 132, layout.body.width, 14},
                   MAROON);
        return;
    }
    if (view.raw == 81 && view.initialized) {
        draw_world_facility_upgrade(view, layout, skin);
        return;
    }
    // S043/S047 label the page, while the real facility name remains in its body.
    const auto title =
        view.raw == 74 && !view.definition_preview
            ? "设施信息" + (view.page_count > 1 ? " " + std::to_string(view.phase + 1) + "/" +
                                                      std::to_string(view.page_count)
                                                : "")
            : view.title;
    if (view.raw == 21) {
        DrawRectangleRec(layout.panel, {222, 230, 144, 255});
        DrawRectangleLinesEx(layout.panel, 1, {59, 66, 18, 255});
        DrawRectangleLinesEx({layout.panel.x + 1, layout.panel.y + 1, layout.panel.width - 2,
                              layout.panel.height - 2},
                             1, {139, 142, 53, 255});
        DrawRectangleRec(layout.body, {250, 254, 248, 255});
    } else {
        skin.window(layout.panel, title);
        skin.content(layout.body);
    }
    if (view.initialized) {
        if (view.raw == 21 || view.raw == 80) {
            if (view.raw == 21) {
                constexpr const char *titles[]{"设备", "一般", "饮食"}; // S057 display wording.
                for (int tab = 0; tab < 3; ++tab) {
                    DrawRectangleRec(layout.tabs[tab], tab == selection.tab
                                                           ? Color{0, 241, 115, 255}
                                                           : Color{225, 255, 69, 255});
                    DrawRectangleLinesEx(layout.tabs[tab], 1, {60, 96, 6, 255});
                    skin.centered(titles[tab], layout.tabs[tab], ink, 10);
                }
                const auto p = layout.panel;
                DrawTriangle({p.x - 8, p.y + 11}, {p.x, p.y + 18}, {p.x, p.y + 4}, GOLD);
                DrawTriangle({p.x + p.width + 8, p.y + 11}, {p.x + p.width, p.y + 4},
                             {p.x + p.width, p.y + 18}, GOLD);
            }
            const auto &list = world_building_rows(view, selection.tab);
            if (list.empty())
                skin.text.draw("暂无候选", layout.rows.x + 4, layout.rows.y + 4, ink);
            const int first = std::clamp(selection.first_row, 0, static_cast<int>(list.size()));
            for (int row = 0; row < world_building_visible_rows(layout) &&
                              row + first < static_cast<int>(list.size());
                 ++row) {
                Rectangle box{layout.rows.x, layout.rows.y + row * layout.row_height,
                              layout.rows.width, layout.row_height};
                if (view.raw != 21 && first + row == selection.selected)
                    DrawRectangleRec(box, {255, 236, 174, 255});
                const auto &item = list[first + row];
                float label_x = box.x + 4;
                if (!item.graphic.frames.empty() || !item.common_image.empty()) {
                    const auto icon = world_building_icon(layout, row);
                    DrawRectangleRec(icon.clip, icon.background);
                    for (const auto &[frame, offset] : item.graphic.frames)
                        skin.sprites.draw(item.graphic.sprite, frame,
                                          {icon.anchor.x + offset.x, icon.anchor.y + offset.y},
                                          WHITE, Sprites::Binding::map, 1, -1, icon.clip);
                    if (!item.common_image.empty()) {
                        const Rectangle target{icon.clip.x + item.image_offset.x,
                                               icon.clip.y + item.image_offset.y,
                                               item.image_source.width, item.image_source.height};
                        const auto clipped =
                            clip_sprite_blit({item.image_source, target}, icon.clip);
                        if (clipped)
                            skin.sprites.image(item.common_image, clipped->source,
                                               clipped->destination, Sprites::Binding::common);
                    }
                    label_x = box.x + 72;
                }
                const auto &name = item.name;
                if (view.raw == 21) {
                    DrawRectangleLinesEx(box, 1, {11, 181, 0, 255});
                    if (first + row == selection.selected) {
                        const float label_width =
                            std::min(box.x + box.width - label_x - 3, skin.text.width(name) + 4);
                        DrawRectangleRec({label_x - 1, box.y + 2, label_width, 15},
                                         {255, 185, 87, 255});
                        skin.sprites.draw("finger_r.seb", 0, {label_x - 8, box.y + 9}, WHITE,
                                          Sprites::Binding::common);
                    }
                }
                fitted(skin, name, {label_x, box.y + 3, box.x + box.width - label_x - 4, 14});
                if (item.residence_qualifications)
                    skin.text.draw("H " + std::to_string(*item.residence_qualifications), label_x,
                                   box.y + 22, blue, 10);
                if (view.raw == 21) {
                    skin.number(item.cost, {box.x + box.width - 13, box.y + 23}, "number05.seb",
                                Sprites::Binding::steam_common);
                    skin.sprites.draw("number05.seb", 20, {box.x + box.width - 12, box.y + 23},
                                      WHITE, Sprites::Binding::steam_common);
                } else
                    skin.right(std::to_string(item.cost) + "G", box.x + box.width - 4, box.y + 22,
                               ink, 10);
            }
            if (view.raw == 21) {
                const Rectangle track{layout.panel.x + layout.panel.width - 7, layout.rows.y, 5,
                                      layout.rows.height};
                DrawRectangleRec(track, {33, 20, 91, 255});
                const int count = std::max(1, static_cast<int>(list.size()));
                const float visible = std::min(count, world_building_visible_rows(layout));
                const float thumb_height = track.height * visible / count;
                const float thumb_y = track.y + (track.height - thumb_height) * first /
                                                    std::max(1, count - static_cast<int>(visible));
                DrawRectangleRec({track.x, thumb_y, track.width, thumb_height},
                                 {50, 164, 234, 255});
            }
        } else if (view.raw == 74) {
            draw_home_detail(view, layout, skin);
        } else {
            constexpr const char *labels[]{"价格", "品质", "魅力", "维护费"};
            for (int row = 0; row < 3; ++row) {
                const float y = layout.body.y + 8 + row * 25;
                std::string label, value;
                if (view.raw == 81) {
                    label = labels[row];
                    value = std::to_string(view.upgrade[0][row]) + " > " +
                            std::to_string(view.upgrade[1][row]);
                } else if (view.phase == 0) {
                    label = labels[row];
                    value = std::to_string(view.attributes[row]);
                } else {
                    label = row == 0 ? "维护费" : row == 1 ? "收入" : "加成";
                    value = row == 0   ? std::to_string(view.attributes[3])
                            : row == 1 ? std::to_string(view.income) + "G"
                                       : std::to_string(view.neighbours);
                }
                skin.text.draw(label, layout.body.x + 4, y);
                skin.right(value, layout.body.x + layout.body.width - 4, y);
            }
        }
    }
    const bool active = enabled && view.initialized;
    if (view.raw == 74)
        draw_facility_footer(view, layout, skin);
    if (view.raw != 81)
        skin.button(layout.cancel, "返回", active);
    if (view.raw == 74 && view.page_count > 1) {
        // arrow02 frames2/3 crop the gold left/right pair; 0/1 are the grey pair.
        skin.sprites.draw("arrow02.seb", active ? 2 : 0,
                          {layout.previous.x + 4, layout.previous.y + 9}, WHITE,
                          Sprites::Binding::common);
        skin.sprites.draw("arrow02.seb", active ? 3 : 1, {layout.next.x + 4, layout.next.y + 9},
                          WHITE, Sprites::Binding::common);
    }
    if (view.raw != 21 && (view.raw != 74 || view.can_confirm))
        skin.choice(layout.confirm,
                    view.raw == 21            ? "建设"
                    : view.raw == 81          ? "确定"
                    : view.definition_preview ? "关闭"
                    : view.can_use_items      ? "使用道具"
                    : view.can_view_products  ? "商品"
                                              : "入住希望者",
                    active && view.can_confirm &&
                        (view.raw != 21 || !world_building_rows(view, selection.tab).empty()));
    if (!feedback.empty())
        fitted(skin, feedback,
               {layout.body.x, layout.panel.y + layout.panel.height - 45, layout.body.width, 14},
               MAROON);
}
} // namespace ark::desktop::ui
