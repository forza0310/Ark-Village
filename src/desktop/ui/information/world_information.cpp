// Source page state owns selection, NEW and modal lifetime. This adapter only
// projects published snapshots and executes the ordered Steam skin requests.
#include "world_information.hpp"
#include "../common/skin.hpp"
#include "../system/world_menu.hpp"
#include "ark/presentation/script_text.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "ark/simulation/presentation/steam_information_skin.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
using Source = sim::StartupInformationPageView;
bool hit(std::optional<Vector2> p, Rectangle r) { return p && CheckCollisionPointRec(*p, r); }
int count(const Source &v) {
    if (v.raw == 9)
        return 5;
    if (v.humans)
        return static_cast<int>(v.humans->size());
    if (v.facilities)
        return static_cast<int>(v.facilities->size());
    if (v.items)
        return static_cast<int>(v.items->size());
    if (v.equipment)
        return static_cast<int>(v.equipment->rows.size());
    return 0;
}
std::string title(const Source &v) {
    const auto suffix = std::to_string(v.selection_or_period + 1);
    if (v.raw == 35)
        return "冒险者一览 " + suffix + "/4";
    if (v.raw == 34)
        return "村情报";
    if (v.raw == 39)
        return "设施收支一览";
    if (v.raw == 40)
        return "赠送礼物 " + suffix + "/3";
    if (v.raw == 36)
        return "收支情报 " + suffix + "/2";
    if (v.raw == 37)
        return "持有物品";
    if (v.raw == 38)
        return "装备一览 " + suffix + "/4";
    return "信息";
}
Color color(const std::array<int, 3> &rgb) {
    return {static_cast<unsigned char>(rgb[0]), static_cast<unsigned char>(rgb[1]),
            static_cast<unsigned char>(rgb[2]), 255};
}
std::string text(const sim::SteamInformationText &part, const Source &v) {
    using Role = sim::SteamInformationTextRole;
    if (part.role == Role::facility_name)
        return part.value + std::to_string(part.argument);
    if (!part.value.empty())
        return part.value;
    switch (part.role) {
    case Role::title:
        return title(v);
    case Role::period:
        return part.period == 0 ? "月间" : "年间";
    case Role::income_header:
        return "收入";
    case Role::expense_header:
        return "支出";
    case Role::profit_label:
        return "利润";
    case Role::category:
        return v.income ? std::string(v.income->rows.at(part.slot).label) : "";
    case Role::description:
        return part.period == 0 ? "本月的收支情报" : "本年的收支情报";
    case Role::name_header:
        return "名称";
    case Role::inventory_header:
        return "持有";
    case Role::unknown_row:
        return "？？？？？";
    case Role::empty_directory:
        return "没有持有物品";
    case Role::known_count:
        return "现在持有 <co=0064FF>" + std::to_string(part.argument) + "</co> 种";
    case Role::satisfaction_header:
        return "满意";
    case Role::effort_header:
        return "努力";
    case Role::equipment_header:
        return "装备";
    case Role::contribution_header:
        return "贡献";
    case Role::town_points_header:
        return "获得";
    case Role::spending_header:
        return "消费";
    case Role::adventurer_count:
        return "共有 <co=0064FF>" + std::to_string(part.argument) + "</co> 名冒险者到访";
    case Role::menu_entry:
        return world_source_menu_label(part.argument);
    case Role::village_name:
        return v.village_name;
    case Role::town_stat_label:
        return std::array<const char *, 5>{"冒险者", "居民", "设施", "完成任务", "举办活动"}.at(
            part.slot);
    case Role::town_adventurer_count:
        return std::to_string(part.argument) + "人";
    case Role::town_equipment_count:
        return std::to_string(part.argument);
    case Role::town_equipment_kinds:
        return std::to_string(part.argument) + "种";
    case Role::facility_income_list:
        return "设施收支一览";
    case Role::facility_profit_header:
        return "累计收益";
    case Role::facility_tracking_hint:
        return "确认定位设施";
    default:
        return {};
    }
}
// Same actual font measures title shadow and foreground independently. The source
// frame and rows keep logical pixels; Chinese long text fits within its own box.
void draw_text(const sim::SteamInformationText &part, const Source &v, Vector2 origin,
               const Skin &skin, std::optional<Rectangle> clip) {
    const auto value = text(part, v);
    const auto parsed = decode_script_text(value);
    float size = part.font_size ? float(part.font_size) : 12.F;
    float available = 0;
    if (part.mode == sim::SteamInformationTextMode::layout)
        available = float(part.extent[0]);
    else if (part.role == sim::SteamInformationTextRole::item_description ||
             part.role == sim::SteamInformationTextRole::known_count ||
             part.role == sim::SteamInformationTextRole::adventurer_count)
        available = 218;
    if (available > 0)
        size *= std::min(1.F, available / std::max(1.F, skin.text.width(parsed.text, size)));
    const float width = skin.text.width(parsed.text, size);
    const int anchor = part.anchor.value_or(0x11);
    float x = origin.x + part.position[0], y = origin.y + part.position[1];
    if (part.mode == sim::SteamInformationTextMode::layout) {
        if (anchor & 2)
            x += (part.extent[0] - width) / 2;
        else if (anchor & 4)
            x += part.extent[0] - width;
        if (anchor & 0x20)
            y += (part.extent[1] - size) / 2;
    } else {
        if (anchor & 2)
            x -= width / 2;
        else if (anchor & 4)
            x -= width;
    }
    for (const auto &run : parsed.runs) {
        Color tint = color(part.rgb);
        if (run.rgb)
            tint = {static_cast<unsigned char>(*run.rgb >> 16),
                    static_cast<unsigned char>(*run.rgb >> 8), static_cast<unsigned char>(*run.rgb),
                    255};
        if (clip)
            skin.text.clipped(run.text, x, y, *clip, tint, size);
        else
            skin.text.draw(run.text, x, y, tint, size);
        x += skin.text.width(run.text, size);
    }
}
void image(const sim::StartupSkinDraw &part, Vector2 origin, const Skin &skin,
           std::optional<Rectangle> clip) {
    const Vector2 anchor{origin.x + part.offset[0], origin.y + part.offset[1]};
    if (part.sprite >= 0) {
        // Current-frame finger has no published Owner animation counter yet. Keep
        // the existing desktop static pointer; never advance it from drawing FPS.
        const int frame = part.frame < 0 ? 0 : part.frame;
        skin.sprites.indexed_sprite(Sprites::Binding::steam_common, part.sprite, frame, part.layer,
                                    part.image, anchor, 1, clip);
    } else if (part.crop[2] > 0 && part.crop[3] > 0) {
        const auto &c = part.crop;
        skin.sprites.indexed_image(Sprites::Binding::steam_common, part.image,
                                   {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                                   {anchor.x, anchor.y, float(c[2]), float(c[3])}, clip);
    }
}
void number(const sim::SteamFacilityNumber &part, Vector2 origin, const Skin &skin,
            std::optional<Rectangle> clip) {
    const auto resource = sim::steam_facility_resource(part.asset);
    if (!resource || !resource->published_sprite)
        throw std::runtime_error("Missing information number resource");
    const auto sprite = std::filesystem::path(resource->published_sprite).filename().string();
    const auto parts =
        sim::steam_facility_number_draws(part, skin.sprites.common_digit_width(sprite));
    if (!parts)
        throw std::runtime_error("Invalid information number request");
    for (const auto &digit : *parts) {
        if (digit.frame < 0)
            continue; // Source negative-frame pixels have not been certified.
        const auto asset = sim::steam_facility_resource(digit.asset);
        if (!asset)
            throw std::runtime_error("Missing information number image");
        const auto binding = digit.asset == sim::SteamFacilityAsset::number05 ||
                                     digit.asset == sim::SteamFacilityAsset::number08
                                 ? Sprites::Binding::steam_common
                                 : Sprites::Binding::common;
        skin.sprites.draw(std::filesystem::path(asset->published_sprite).filename().string(),
                          digit.frame, {origin.x + digit.position[0], origin.y + digit.position[1]},
                          WHITE, binding, 1, -1, clip);
    }
}
} // namespace

bool world_information_page(const sim::rules::WorldScriptPage &p) {
    return p.kind == sim::rules::WorldScriptPageKind::raw_page &&
           (p.legacy_page == 9 || (p.legacy_page >= 34 && p.legacy_page <= 40));
}
WorldInformationView world_information_view(const sim::StartupWorldRuntimeState &state,
                                            const sim::rules::WorldScriptPage &page) {
    WorldInformationView v;
    v.raw = page.legacy_page;
    if (!world_information_page(page))
        return v;
    v.source = sim::inspect_startup_world_information_page(state, page.id);
    if (const auto pos = state.menu_page_positions.find(page.id);
        pos != state.menu_page_positions.end())
        v.menu_position = pos->second;
    v.interactive = v.source.has_value() && page.lifecycle == 2 && !state.scene.framework_paused;
    return v;
}
WorldInformationLayout world_information_layout(Extent extent, int raw,
                                                std::array<int, 2> menu_position) {
    const Vector2 o{(extent.width - 240.F) / 2, (extent.height - 240.F) / 2};
    const auto box = [&](float x, float y, float w, float h) {
        return Rectangle{o.x + x, o.y + y, w, h};
    };
    WorldInformationLayout l{o,
                             box(3, 94, 231, raw == 38 ? 96.F : 95.F),
                             box(13, 43, 23, 19),
                             box(205, 43, 23, 19),
                             box(179, 218, 53, 20),
                             box(8, 218, 58, 20),
                             {}};
    // Menu positions are stored source coordinates, distinct from centered modal pages.
    const float mx = float(menu_position[0]), my = float(menu_position[1]);
    const float dx = mx + 90 > extent.width ? extent.width - 90 - mx : 0;
    const float dy = my + 145 > extent.height ? extent.height - 145 - my : 0;
    for (int i = 0; i < 5; ++i)
        l.menu_rows[i] = {mx + dx, my + dy + i * 28, 90, 28};
    if (raw == 34)
        l.confirm = box(20, 194, 200, 18);
    return l;
}
std::optional<sim::StartupInformationInput> world_information_input(const WorldInformationView &v,
                                                                    const WorldInformationLayout &l,
                                                                    const WorldInformationInput &in,
                                                                    bool blocked) {
    if (blocked || !v.interactive || !v.source)
        return {};
    const auto &source = *v.source;
    sim::StartupInformationInput out;
    if (in.cancel || hit(in.click, l.back)) {
        out.cancel = true;
        return out;
    }
    if (in.click) {
        if (v.raw == 9) {
            for (int row = 0; row < 5; ++row)
                if (hit(in.click, l.menu_rows[row]) && source.entries[row].implemented) {
                    out.select_row = row;
                    return out;
                }
        } else if (v.raw != 36 && v.raw != 34 && hit(in.click, l.rows)) {
            const float pitch = v.raw == 38 ? 24.F : 19.F;
            const int row =
                source.first_visible + static_cast<int>((in.click->y - l.rows.y) / pitch);
            if (row < count(source)) {
                out.select_row = row;
                return out;
            }
        }
        if ((v.raw == 35 || v.raw == 36 || v.raw == 38 || v.raw == 40) &&
            hit(in.click, l.previous)) {
            out.left = true;
            return out;
        }
        if ((v.raw == 35 || v.raw == 36 || v.raw == 38 || v.raw == 40) && hit(in.click, l.next)) {
            out.right = true;
            return out;
        }
        if (hit(in.click, l.confirm) &&
            (v.raw == 9 || v.raw == 34 || v.raw == 35 || v.raw == 39 || v.raw == 40)) {
            if (v.raw == 9 && !source.entries.at(source.selection_or_period).implemented)
                return {};
            out.confirm = true;
            return out;
        }
        return {}; // Never combine pointer selection with another source input.
    }
    if (in.wheel_rows && v.raw != 34 && v.raw != 36 && count(source) > 0) {
        out.select_row =
            std::clamp((v.raw == 9 ? source.selection_or_period : source.selection) + in.wheel_rows,
                       0, count(source) - 1);
        return out;
    }
    out.up = v.raw != 34 && v.raw != 36 && in.up;
    out.down = v.raw != 34 && v.raw != 36 && in.down;
    out.left = v.raw != 34 && v.raw != 37 && v.raw != 39 && in.left;
    out.right = v.raw != 34 && v.raw != 37 && v.raw != 39 && in.right;
    out.confirm = in.confirm;
    if (v.raw == 9 && !in.up && !in.down && (in.confirm || in.right) &&
        !source.entries.at(source.selection_or_period).implemented)
        return {};
    if (out.up || out.down || out.left || out.right || out.confirm)
        return out;
    return {};
}
void draw_information_number(const sim::SteamFacilityNumber &request, Vector2 origin,
                             const Skin &skin) {
    number(request, origin, skin, {});
}
void draw_information_plan(const sim::SteamInformationSkinPlan &plan, const Source &source,
                           Vector2 origin, const Skin &skin) {
    std::optional<Rectangle> clip, image_only;
    std::vector<std::optional<Rectangle>> clips;
    const auto intersect = [](std::optional<Rectangle> a, Rectangle b) -> std::optional<Rectangle> {
        if (!a)
            return b;
        const float x = std::max(a->x, b.x), y = std::max(a->y, b.y);
        return Rectangle{x, y, std::max(0.F, std::min(a->x + a->width, b.x + b.width) - x),
                         std::max(0.F, std::min(a->y + a->height, b.y + b.height) - y)};
    };
    const auto image_clip = [&] { return image_only ? intersect(clip, *image_only) : clip; };
    for (const auto &request : plan.draws) {
        if (const auto *p = std::get_if<sim::StartupSkinRect>(&request)) {
            const auto &r = p->rect;
            const Rectangle box{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            if (p->outline)
                DrawRectangleLinesEx(box, 1, color(p->rgb));
            else
                DrawRectangleRec(box, color(p->rgb));
        } else if (const auto *p = std::get_if<sim::StartupSkinDraw>(&request))
            image(*p, origin, skin, image_clip());
        else if (const auto *p = std::get_if<sim::SteamInformationText>(&request))
            draw_text(*p, source, origin, skin, clip);
        else if (const auto *p = std::get_if<sim::SteamInformationLine>(&request)) {
            const Vector2 from{origin.x + p->from[0], origin.y + p->from[1]},
                to{origin.x + p->to[0], origin.y + p->to[1]};
            if (p->width == 1 && from.y == to.y)
                DrawRectangleRec({std::min(from.x, to.x), from.y, std::abs(to.x - from.x) + 1, 1},
                                 color(p->rgb));
            else
                DrawLineEx(from, to, float(p->width), color(p->rgb));
        } else if (const auto *p = std::get_if<sim::SteamInformationNumber>(&request)) {
            const auto parts = sim::steam_information_number_draws(*p);
            if (!parts)
                throw std::runtime_error("Invalid information numeric plan");
            for (const auto &part : *parts)
                image(part, origin, skin, image_clip());
        } else if (const auto *p = std::get_if<sim::SteamFacilityClip>(&request)) {
            if (p->kind == sim::SteamFacilityClipKind::pop) {
                if (clips.empty())
                    throw std::runtime_error("Unbalanced source information clip");
                clip = clips.back();
                clips.pop_back();
            } else {
                clips.push_back(clip);
                clip = intersect(clip,
                                 Rectangle{origin.x + p->rectangle[0], origin.y + p->rectangle[1],
                                           float(p->rectangle[2]), float(p->rectangle[3])});
            }
        } else if (const auto *p = std::get_if<sim::SteamInformationImageClip>(&request)) {
            if (p->kind == sim::SteamInformationImageClipKind::clear)
                image_only.reset();
            else
                image_only = Rectangle{origin.x + p->rectangle[0], origin.y + p->rectangle[1],
                                       float(p->rectangle[2]), float(p->rectangle[3])};
        } else if (const auto *p = std::get_if<sim::SteamInformationHumanBody>(&request)) {
            skin.sprites.actor(false, p->body.sprite, p->body.image, p->body.frame,
                               {origin.x + p->position[0], origin.y + p->position[1]}, 1,
                               image_clip());
        } else if (const auto *p = std::get_if<sim::SteamFacilityNumber>(&request))
            number(*p, origin, skin, image_clip());
    }
}
namespace {
// raw40 has a complete business contract but no dedicated published Steam skin.
// This explicitly static PC table uses its three source tabs, not the raw35 plan.
void draw_present_directory(const sim::StartupWorldRuntimeState &state, const Source &v,
                            const WorldInformationLayout &l, const Skin &skin) {
    skin.window({l.origin.x + 10, l.origin.y + 36, 220, 168}, title(v));
    skin.content({l.origin.x + 17, l.origin.y + 74, 202, 110});
    skin.text.draw(v.selection_or_period == 0   ? "姓名 / 等级"
                   : v.selection_or_period == 1 ? "满意 / 努力 / 装备"
                                                : "体力 / 力量 / 防御 / 魔力",
                   l.origin.x + 25, l.origin.y + 68, ink, 10);
    if (!v.humans)
        return;
    for (int row = v.first_visible; row < std::min(int(v.humans->size()), v.first_visible + 5);
         ++row) {
        const auto &h = v.humans->at(row).details;
        const float y = l.origin.y + 97 + (row - v.first_visible) * 19;
        if (row == v.selection)
            DrawRectangleRec({l.origin.x + 23, y - 3, 191, 18}, {255, 153, 55, 255});
        if (v.humans->at(row).newly_unlocked)
            skin.text.draw("N", l.origin.x + 20, y, blue, 9);
        std::string value;
        if (v.selection_or_period == 0)
            value = h.name + "  Lv." + std::to_string(h.level) + "  " +
                    std::to_string(h.experience) + "/" + std::to_string(h.threshold);
        else if (v.selection_or_period == 1) {
            value = std::to_string(h.satisfaction) + " / " + std::to_string(h.effort);
            for (int slot = 0; slot < 4; ++slot) {
                const float x = l.origin.x + 141 + slot * 18;
                if (h.equipment[slot]) {
                    const auto parts = sim::startup_world_equipment_icon_draws(state,
                                                                               slot == 0   ? 1
                                                                               : slot == 3 ? 3
                                                                                           : 2,
                                                                               *h.equipment[slot]);
                    if (!parts)
                        throw std::runtime_error("Missing gift directory equipment icon");
                    for (const auto &p : *parts) {
                        const sim::StartupSkinDraw draw{sim::StartupSkinPackage::common,
                                                        p.image,
                                                        p.sprite,
                                                        p.frame,
                                                        p.layer,
                                                        p.crop,
                                                        {p.offset[0], p.offset[1]}};
                        image(draw, {x, y - 3}, skin, {});
                    }
                } else
                    skin.sprites.indexed_image(Sprites::Binding::common, 24, {126, 0, 18, 18},
                                               {x, y - 3, 18, 18});
            }
        } else
            for (int n = 0; n < 4; ++n)
                value += (n ? " / " : "") + std::to_string(h.combat[n]);
        const float size = 11 * std::min(1.F, (v.selection_or_period == 1 ? 105.F : 180.F) /
                                                  std::max(1.F, skin.text.width(value, 11)));
        skin.text.draw(value, l.origin.x + 30, y, ink, size);
    }
    if (v.selection >= 0 && v.selection < int(v.humans->size())) {
        const auto caption = v.humans->at(v.selection).details.name + "  " +
                             std::to_string(v.selection + 1) + "/" +
                             std::to_string(v.humans->size());
        skin.text.draw(caption, l.origin.x + 23, l.origin.y + 188, blue,
                       10 * std::min(1.F, 190.F / std::max(1.F, skin.text.width(caption, 10))));
    }
    skin.choice(l.previous, "<");
    skin.choice(l.next, ">");
}
} // namespace
void draw_world_information(const sim::StartupWorldRuntimeState &state,
                            const WorldInformationView &v, const WorldInformationLayout &l,
                            const Skin &skin, bool enabled, const std::string &feedback) {
    if (!v.source)
        return;
    const auto &source = *v.source;
    enabled = enabled && v.interactive;
    if (v.raw == 9) {
        sim::SteamInformationMenuSkinOptions options;
        // Canvas comes from the same centered reference used by modal controls.
        options.canvas = {int(l.origin.x * 2 + 240), int(l.origin.y * 2 + 240)};
        options.origin = v.menu_position;
        options.view_y = 23;
        std::array<int, 5> widths{};
        for (int n = 0; n < 5; ++n)
            widths[n] = int(skin.text.width(std::string(source.entries[n].label)));
        options.measured_text_widths = widths;
        const auto plan = sim::steam_information_menu_skin(state, source.page_id, options);
        const auto status = sim::steam_information_menu_status_skin(state, source.page_id, options);
        if (!plan || !status)
            throw std::runtime_error("Invalid information menu plan");
        const Vector2 origin{float(options.origin[0]), float(options.origin[1])};
        draw_information_plan(*plan, source, origin, skin);
        draw_information_plan(*status, source, origin, skin);
    } else if (v.raw == 40)
        draw_present_directory(state, source, l, skin);
    else {
        sim::SteamInformationSkinOptions options;
        const auto caption = title(source);
        options.title_widths =
            std::array<int, 2>{int(skin.text.width(caption)), int(skin.text.width(caption))};
        if (v.raw == 34)
            options.bottom_width = int(skin.text.width("设施收支一览"));
        const auto plan =
            v.raw == 34   ? sim::steam_town_information_skin(state, source.page_id, options)
            : v.raw == 39 ? sim::steam_facility_information_skin(state, source.page_id, options)
            : v.raw == 35 ? sim::steam_adventurer_information_skin(state, source.page_id, options)
            : v.raw == 36 ? sim::steam_income_information_skin(state, source.page_id, options)
            : v.raw == 37 ? sim::steam_item_information_skin(state, source.page_id, options)
                          : sim::steam_equipment_information_skin(state, source.page_id, options);
        if (!plan)
            throw std::runtime_error("Information skin rejected initialized page");
        draw_information_plan(*plan, source, l.origin, skin);
    }
    skin.button(l.back, "返回", enabled);
    if (v.raw == 9 || v.raw == 35 || v.raw == 39 || v.raw == 40)
        skin.choice(l.confirm,
                    v.raw == 9    ? "选择"
                    : v.raw == 39 ? "定位"
                    : v.raw == 40 ? "赠礼"
                                  : "详情",
                    enabled);
    if (!feedback.empty())
        skin.text.draw(feedback, l.origin.x + 4, l.origin.y + 207, MAROON, 9);
}
} // namespace ark::desktop::ui
