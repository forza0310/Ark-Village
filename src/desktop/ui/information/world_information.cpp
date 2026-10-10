// Source page state owns selection, NEW and modal lifetime. This adapter only
// projects published snapshots and executes the ordered Steam skin requests.
#include "world_information.hpp"
#include "../common/skin.hpp"
#include "ark/presentation/script_text.hpp"
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
           (p.legacy_page == 9 || (p.legacy_page >= 35 && p.legacy_page <= 38));
}
WorldInformationView world_information_view(const sim::StartupWorldRuntimeState &state,
                                            const sim::rules::WorldScriptPage &page) {
    WorldInformationView v;
    v.raw = page.legacy_page;
    if (!world_information_page(page))
        return v;
    v.source = sim::inspect_startup_world_information_page(state, page.id);
    v.interactive = v.source.has_value() && page.lifecycle == 2 && !state.scene.framework_paused;
    return v;
}
WorldInformationLayout world_information_layout(Extent extent, int raw) {
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
    // The product main menu is an overlay, not a fabricated source raw3. Its
    // information submenu reuses native 89x28 menu strips in the same visible area.
    for (int i = 0; i < 5; ++i)
        l.menu_rows[i] = box(70, 28.F + i * 28, 89, 28);
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
        } else if (v.raw != 36 && hit(in.click, l.rows)) {
            const float pitch = v.raw == 38 ? 24.F : 19.F;
            const int row =
                source.first_visible + static_cast<int>((in.click->y - l.rows.y) / pitch);
            if (row < count(source)) {
                out.select_row = row;
                return out;
            }
        }
        if (v.raw != 9 && v.raw != 37 && hit(in.click, l.previous)) {
            out.left = true;
            return out;
        }
        if (v.raw != 9 && v.raw != 37 && hit(in.click, l.next)) {
            out.right = true;
            return out;
        }
        if (hit(in.click, l.confirm) && (v.raw == 9 || v.raw == 35)) {
            if (v.raw == 9 && !source.entries.at(source.selection_or_period).implemented)
                return {};
            out.confirm = true;
            return out;
        }
        return {}; // Never combine pointer selection with another source input.
    }
    if (in.wheel_rows && v.raw != 36 && count(source) > 0) {
        out.select_row =
            std::clamp((v.raw == 9 ? source.selection_or_period : source.selection) + in.wheel_rows,
                       0, count(source) - 1);
        return out;
    }
    out.up = in.up;
    out.down = in.down;
    out.left = in.left;
    out.right = in.right;
    out.confirm = in.confirm;
    if (v.raw == 9 && !in.up && !in.down && (in.confirm || in.right) &&
        !source.entries.at(source.selection_or_period).implemented)
        return {};
    if (out.up || out.down || out.left || out.right || out.confirm)
        return out;
    return {};
}
void draw_world_information(const sim::StartupWorldRuntimeState &state,
                            const WorldInformationView &v, const WorldInformationLayout &l,
                            const Skin &skin, bool enabled, const std::string &feedback) {
    if (!v.source)
        return;
    const auto &source = *v.source;
    enabled = enabled && v.interactive;
    if (v.raw == 9) {
        for (int i = 0; i < 5; ++i) {
            const auto r = l.menu_rows[i];
            skin.sprites.draw("menu.seb", source.selection_or_period == i ? 2 : 3, {r.x, r.y},
                              WHITE, Sprites::Binding::common);
            const Color tint = source.entries[i].implemented
                                   ? (source.selection_or_period == i ? ink : WHITE)
                                   : Color{182, 174, 147, 255};
            skin.text.draw(std::string(source.entries[i].label), r.x + 9, r.y + 8, tint, 11);
            if (source.selection_or_period == i)
                skin.sprites.draw("finger_r.seb", 0, {r.x + r.width - 1, r.y + 11}, WHITE,
                                  Sprites::Binding::common);
        }
    } else {
        sim::SteamInformationSkinOptions options;
        const auto caption = title(source);
        options.title_widths = std::array<int, 2>{static_cast<int>(skin.text.width(caption)),
                                                  static_cast<int>(skin.text.width(caption))};
        const auto plan =
            v.raw == 35   ? sim::steam_adventurer_information_skin(state, source.page_id, options)
            : v.raw == 36 ? sim::steam_income_information_skin(state, source.page_id, options)
            : v.raw == 37 ? sim::steam_item_information_skin(state, source.page_id, options)
                          : sim::steam_equipment_information_skin(state, source.page_id, options);
        if (!plan)
            throw std::runtime_error("Information skin rejected initialized page");
        std::optional<Rectangle> clip;
        for (const auto &request : plan->draws) {
            if (const auto *p = std::get_if<sim::StartupSkinRect>(&request)) {
                const auto &r = p->rect;
                const Rectangle box{l.origin.x + r[0], l.origin.y + r[1], float(r[2]), float(r[3])};
                if (p->outline)
                    DrawRectangleLinesEx(box, 1, color(p->rgb));
                else
                    DrawRectangleRec(box, color(p->rgb));
            } else if (const auto *p = std::get_if<sim::StartupSkinDraw>(&request))
                image(*p, l.origin, skin, clip);
            else if (const auto *p = std::get_if<sim::SteamInformationText>(&request))
                draw_text(*p, source, l.origin, skin, clip);
            else if (const auto *p = std::get_if<sim::SteamInformationLine>(&request)) {
                const Vector2 from{l.origin.x + p->from[0], l.origin.y + p->from[1]},
                    to{l.origin.x + p->to[0], l.origin.y + p->to[1]};
                if (p->width == 1 && from.y == to.y)
                    DrawRectangleRec(
                        {std::min(from.x, to.x), from.y, std::abs(to.x - from.x) + 1, 1},
                        color(p->rgb));
                else
                    DrawLineEx(from, to, float(p->width), color(p->rgb));
            } else if (const auto *p = std::get_if<sim::SteamInformationNumber>(&request)) {
                const auto parts = sim::steam_information_number_draws(*p);
                if (!parts)
                    throw std::runtime_error("Invalid information numeric plan");
                for (const auto &part : *parts)
                    image(part, l.origin, skin, clip);
            } else if (const auto *p = std::get_if<sim::SteamFacilityClip>(&request)) {
                if (p->kind == sim::SteamFacilityClipKind::pop)
                    clip.reset();
                else
                    clip = Rectangle{l.origin.x + p->rectangle[0], l.origin.y + p->rectangle[1],
                                     float(p->rectangle[2]), float(p->rectangle[3])};
            } else if (const auto *p = std::get_if<sim::SteamInformationHumanBody>(&request)) {
                skin.sprites.actor(false, p->body.sprite, p->body.image, p->body.frame,
                                   {l.origin.x + p->position[0], l.origin.y + p->position[1]}, 1,
                                   clip);
            } else if (const auto *p = std::get_if<sim::SteamFacilityNumber>(&request))
                number(*p, l.origin, skin, clip);
        }
    }
    skin.button(l.back, "返回", enabled);
    if (v.raw == 9 || v.raw == 35)
        skin.choice(l.confirm, v.raw == 9 ? "选择" : "详情",
                    enabled &&
                        (v.raw != 9 || source.entries.at(source.selection_or_period).implemented));
    if (!feedback.empty())
        skin.text.draw(feedback, l.origin.x + 4, l.origin.y + 207, MAROON, 9);
}
} // namespace ark::desktop::ui
