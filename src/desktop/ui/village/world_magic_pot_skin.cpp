// Ordered GPU execution and last-painted hit geometry; no business or timing side effects.
#include "../common/steam_plan_draw.hpp"
#include "world_magic_pot.hpp"
#include <type_traits>

namespace ark::desktop::ui {
namespace {
Rectangle translated(std::array<int, 4> r, Vector2 o) {
    return {o.x + r[0], o.y + r[1], float(r[2]), float(r[3])};
}
Rectangle row_box(std::array<int, 4> r, Vector2 o, Rectangle panel) {
    const auto b = translated(r, o);
    const float x = std::max(b.x, panel.x);
    return {x, b.y, std::max(0.F, std::min(b.x + b.width, panel.x + panel.width) - x), b.height};
}
} // namespace
std::optional<WorldFacilityPaintedControls> draw_world_magic_pot(const WorldMagicPotView &v,
                                                                 const WorldMagicPotLayout &l,
                                                                 const Skin &skin, bool enabled,
                                                                 const std::string &feedback) {
    if (!v.initialized) {
        skin.window(l.panel, v.title);
        return {};
    }
    if (!v.skin)
        throw std::invalid_argument("Magic pot needs a measured source plan");
    WorldFacilityPaintedControls out;
    out.page = v.page;
    out.raw = v.raw;
    out.phase = v.phase;
    out.selection = v.selection;
    out.first = v.first_visible;
    out.count = int(v.rows.size());
    out.choice = v.binding;
    out.extent = l.extent;
    const auto origin = world_list_skin_origin(l.panel);
    SteamPlanDraw draw(origin, skin, v.skin->title);
    for (const auto &op : v.skin->operations)
        std::visit(
            [&](const auto &p) {
                using T = std::decay_t<decltype(p)>;
                if constexpr (std::is_same_v<T, sim::SteamStartupFrameDraw>)
                    draw.frame(p);
                else if constexpr (std::is_same_v<T, sim::SteamHumanImage>)
                    draw.human_image(p);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityImage>) {
                    const auto r = sim::steam_facility_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing pot label resource");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.facility_image(p);
                } else if constexpr (std::is_same_v<T, sim::SteamFacilityNumber>)
                    draw.number(p);
                else if constexpr (std::is_same_v<T, sim::SteamHumanPortrait>)
                    draw.portrait(p);
                else if constexpr (std::is_same_v<T, sim::SteamFacilityEffectImage>) {
                    const auto r = sim::steam_facility_effect_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing pot effect resource");
                    draw.published(r->published_image, r->published_sprite, p.position, p.frame);
                } else if constexpr (std::is_same_v<T, sim::SteamMagicPotImage>) {
                    const auto r = sim::steam_magic_pot_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing magic pot resource");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.published(r->published_image, r->published_sprite, p.position, p.frame,
                                       p.crop);
                } else if constexpr (std::is_same_v<T, sim::SteamMagicPotText>) {
                    const float available = p.rectangle[0] == 120   ? 200.F
                                            : p.rectangle[0] == 126 ? 55.F
                                                                    : 90.F;
                    draw.literal(p.value, p.rectangle, p.anchor, p.font_size, p.layout, p.rgb,
                                 available, p.rich);
                } else if constexpr (std::is_same_v<T, sim::SteamMagicPotClip>) {
                    if (p.kind == sim::SteamMagicPotClipKind::replace)
                        draw.replace_clip(p.rectangle);
                    else
                        draw.clip({p.kind == sim::SteamMagicPotClipKind::pop
                                       ? sim::SteamFacilityClipKind::pop
                                       : sim::SteamFacilityClipKind::push_intersect,
                                   p.rectangle});
                } else if constexpr (std::is_same_v<T, sim::SteamStartupTouch>) {
                    if (p.component == 3 && p.rectangle) {
                        const int index = p.value & 0xffff;
                        out.rows.push_back(
                            {index, v.rows.at(index).identity, translated(*p.rectangle, origin)});
                    } else if (p.component == 2 && p.rectangle && v.raw == 42) {
                        if ((*p.rectangle)[1] < 100)
                            out.previous = translated(*p.rectangle, origin);
                        else
                            out.next = translated(*p.rectangle, origin);
                    } else if (p.component == 4 && p.rectangle)
                        out.confirm = translated(*p.rectangle, origin);
                } else if constexpr (std::is_same_v<T, sim::SteamFacilityEffectTouch>) {
                    if (p.touch.component == 11 && p.touch.rectangle) {
                        const int index = p.touch.value & 0xffff;
                        // PC releases use the visible source row; oversized platform margins stay
                        // research evidence.
                        out.rows.push_back({index, v.rows.at(index).identity,
                                            row_box(*p.touch.rectangle, origin, l.panel)});
                    }
                } else if constexpr (std::is_same_v<T, sim::SteamHumanCatalogueScroll>) {
                    for (const auto &part : p.rectangles)
                        draw.rectangle(part);
                    const auto &r = *p.touch.rectangle;
                    out.scroll = Rectangle{origin.x + r[0] - 3, origin.y + r[1], float(r[2] + 3),
                                           float(r[3])};
                    out.scroll_visible = p.arguments[1];
                } else if constexpr (std::is_same_v<T, sim::SteamFacilityArrowTouch>) {
                    const auto expanded = steam_plan_arrow(p, skin);
                    const auto &c = expanded.crop;
                    const auto &d = expanded.destination;
                    skin.sprites.image("buttoneffect.png",
                                       {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                                       {origin.x + d[0], origin.y + d[1], d[2], d[3]},
                                       Sprites::Binding::steam_common);
                    if (p.value == 16)
                        out.previous = translated(expanded.touch_rectangle, origin);
                    else if (p.value == 18)
                        out.next = translated(expanded.touch_rectangle, origin);
                }
            },
            op);
    draw.finish();
    if (v.can_cancel) {
        out.cancel = l.cancel;
        skin.choice(l.cancel, "返回", enabled);
    }
    // 46/47 use their in-panel source button. Lists and timed reports keep a PC soft confirm.
    if (v.raw <= 45) {
        out.confirm = l.confirm;
        skin.choice(l.confirm, v.raw == 42 ? "投入" : "确定", enabled && v.can_confirm);
    }
    if (!feedback.empty())
        skin.text.draw(feedback, l.feedback.x, l.feedback.y, MAROON, 9);
    return out;
}
} // namespace ark::desktop::ui
