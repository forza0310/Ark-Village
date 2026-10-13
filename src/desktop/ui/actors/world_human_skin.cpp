// Execute maintained human page streams. Font/GPU adaptation stays outside the Owner.
#include "../common/skin.hpp"
#include "ark/simulation/presentation/steam_manual_skin.hpp"
#include "world_human_detail.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
Color color(const std::array<int, 3> &c) {
    return {static_cast<unsigned char>(c[0]), static_cast<unsigned char>(c[1]),
            static_cast<unsigned char>(c[2]), 255};
}
std::string label(const sim::SteamHumanOverviewText &p, bool recovery) {
    if (!p.value.empty())
        return p.value;
    using Role = sim::SteamHumanOverviewTextRole;
    switch (p.role) {
    case Role::satisfaction:
        return "满意度";
    case Role::effort:
        return "努力";
    case Role::gift:
        return recovery ? "赠送回复道具" : "赠送礼物";
    case Role::profession_change:
        return "转职";
    case Role::attribute:
        return std::array<const char *, 6>{"体力", "力量", "灵巧", "耐力", "魔力", "运气"}.at(
            p.argument);
    case Role::equipment_slot:
        return std::array<const char *, 4>{"武器", "防具1", "防具2", "饰品"}.at(p.argument);
    case Role::equipment_empty:
        return "未装备";
    case Role::equipment_hint:
        return "按确认键赠送礼物";
    case Role::magic_title:
        return "习得魔法";
    case Role::spell:
        return std::array<const char *, 4>{"火魔法", "冰魔法", "雷魔法", "回复魔法"}.at(p.argument);
    case Role::magic_status:
        return std::array<const char *, 5>{"精通特定职业可以习得", "可以使用一种魔法",
                                           "可以使用两种魔法", "可以使用三种魔法",
                                           "可以使用全部魔法"}
            .at(p.argument);
    case Role::gift_change_prompt:
        return "赠礼更换装备";
    case Role::gift_accept:
        return "赠送礼物";
    case Role::profession_current:
        return "当前职业";
    case Role::profession_insufficient:
        return "村子点数不足";
    case Role::gift_result_suffix:
        return "作为礼物赠送";
    default:
        throw std::invalid_argument("Human skin text has no bound value");
    }
}
sim::SteamManualArrowPlan arrow(const sim::SteamFacilityArrowTouch &p, const Skin &skin) {
    const auto resource = sim::steam_facility_resource(sim::SteamFacilityAsset::title_arrows);
    if (!resource || !resource->published_sprite)
        throw std::invalid_argument("Human arrow resource missing");
    const auto bounds = skin.sprites.frame_bounds(
        std::filesystem::path(resource->published_sprite).filename().string(), p.frame,
        Sprites::Binding::common);
    const auto result = sim::steam_manual_arrow_skin(
        {p.position, p.frame, false},
        {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
    if (!result)
        throw std::invalid_argument("Human arrow helper rejected source bounds");
    return *result;
}
// This small executor owns no state or animation clock; every page shares its portrait helper.
class HumanSkinDraw {
  public:
    HumanSkinDraw(Vector2 origin, const Skin &skin, std::string title)
        : origin_(origin), skin_(skin), title_(std::move(title)) {}
    Vector2 point(std::array<int, 2> p) const { return {origin_.x + p[0], origin_.y + p[1]}; }
    void rectangle(const sim::StartupSkinRect &p) const {
        const auto &r = p.rect;
        const Rectangle box{origin_.x + r[0], origin_.y + r[1], float(r[2]), float(r[3])};
        if (p.outline)
            DrawRectangleLinesEx(box, 1, color(p.rgb));
        else
            DrawRectangleRec(box, color(p.rgb));
    }
    void human_image(const sim::SteamHumanImage &p) const {
        const auto resource = sim::steam_human_resource(p.asset);
        if (!resource)
            throw std::invalid_argument("Missing human skin resource");
        const std::string source = resource->published_image;
        const auto group = source.rfind("original/event/", 0) == 0    ? Sprites::Binding::event
                           : source.rfind("original/effect/", 0) == 0 ? Sprites::Binding::effect
                           : source.rfind("original/", 0) == 0        ? Sprites::Binding::common
                                                               : Sprites::Binding::steam_common;
        image(resource->published_image, resource->published_sprite, p.position, p.frame, p.crop,
              group);
    }
    void facility_image(const sim::SteamFacilityImage &p) const {
        const auto resource = sim::steam_facility_resource(p.asset);
        if (!resource)
            throw std::invalid_argument("Missing human number/label resource");
        const auto group = std::string(resource->published_image).rfind("original/", 0) == 0
                               ? Sprites::Binding::common
                               : Sprites::Binding::steam_common;
        image(resource->published_image, resource->published_sprite, p.position, p.frame, p.crop,
              group);
    }
    void number(const sim::SteamFacilityNumber &p) const {
        const auto resource = sim::steam_facility_resource(p.asset);
        if (!resource || !resource->published_sprite)
            throw std::invalid_argument("Human number has no sprite");
        const auto digits = sim::steam_facility_number_draws(
            p, skin_.sprites.common_digit_width(
                   std::filesystem::path(resource->published_sprite).filename().string()));
        if (!digits)
            throw std::invalid_argument("Human number request rejected");
        for (const auto &digit : *digits) {
            // Ordinary source numbers can request a negative SEB frame. No published glyph
            // binds it; keep that request invisible, as in the information-page executor.
            if (digit.frame >= 0)
                facility_image(digit);
        }
    }
    void portrait(const sim::SteamHumanDetailSkin &p) const {
        parts(p.medals);
        const auto anchor = point(p.portrait_position);
        if (p.portrait.weapon) {
            const auto &w = *p.portrait.weapon;
            skin_.sprites.indexed_sprite(Sprites::Binding::weapon, w.sprite, w.frame, w.layer,
                                         w.image, {anchor.x + w.offset[0], anchor.y + w.offset[1]});
        }
        const auto &b = p.portrait.body;
        skin_.sprites.actor(false, b.sprite, b.image, b.frame, anchor);
        parts(p.after_portrait);
    }
    void portrait(const sim::SteamHumanPortrait &p) const {
        // Source clip belongs to this portrait alone. Keep logical coordinates through
        // the normal blit helper so canvas/DPI applies once without a nested OS scissor.
        std::optional<Rectangle> clip;
        if (p.clip) {
            const auto &r = *p.clip;
            clip = Rectangle{origin_.x + r[0], origin_.y + r[1], float(r[2]), float(r[3])};
        }
        const auto anchor = point(p.position);
        if (p.actor.weapon) {
            const auto &w = *p.actor.weapon;
            skin_.sprites.indexed_sprite(Sprites::Binding::weapon, w.sprite, w.frame, w.layer,
                                         w.image, {anchor.x + w.offset[0], anchor.y + w.offset[1]},
                                         1, clip);
        }
        const auto &body = p.actor.body;
        skin_.sprites.actor(false, body.sprite, body.image, body.frame, anchor, 1, clip);
    }
    void frame(const sim::SteamStartupFrameDraw &p) const {
        if (const auto *r = std::get_if<sim::StartupSkinRect>(&p))
            rectangle(*r);
        else if (const auto *i = std::get_if<sim::StartupSkinDraw>(&p)) {
            const auto at = point(i->offset);
            if (i->sprite >= 0)
                skin_.sprites.indexed_sprite(Sprites::Binding::common, i->sprite, i->frame,
                                             i->layer, i->image, at);
            else if (i->crop[2] > 0 && i->crop[3] > 0)
                skin_.sprites.indexed_image(
                    Sprites::Binding::common, i->image,
                    {float(i->crop[0]), float(i->crop[1]), float(i->crop[2]), float(i->crop[3])},
                    {at.x, at.y, float(i->crop[2]), float(i->crop[3])});
        } else if (const auto *t = std::get_if<sim::SteamStartupText>(&p)) {
            if (t->role != sim::SteamStartupTextRole::message_title || !t->rgb)
                throw std::invalid_argument("Unsupported human frame text");
            skin_.text.draw(title_, origin_.x + float(t->rectangle[0]),
                            origin_.y + float(t->rectangle[1]), color(*t->rgb));
        }
    }
    void text(const sim::SteamHumanOverviewText &p, bool recovery) const {
        const auto value = label(p, recovery);
        const auto &r = p.rectangle;
        float size = p.font_size ? float(p.font_size) : 12.F;
        // Desktop Chinese typography fits the complete text into source columns.
        const float width =
            r[2] > 0                                                ? float(r[2])
            : p.role == sim::SteamHumanOverviewTextRole::name       ? 86.F
            : p.role == sim::SteamHumanOverviewTextRole::profession ? (r[0] >= 119 ? 53.F : 80.F)
            : p.role == sim::SteamHumanOverviewTextRole::attribute  ? 38.F
            : p.role == sim::SteamHumanOverviewTextRole::result_message ||
                    p.role == sim::SteamHumanOverviewTextRole::profession_insufficient
                ? skin_.text.width(value, size)
            : p.role == sim::SteamHumanOverviewTextRole::magic_status ||
                    p.role == sim::SteamHumanOverviewTextRole::gift_change_prompt ||
                    p.role == sim::SteamHumanOverviewTextRole::profession_transition ||
                    p.role == sim::SteamHumanOverviewTextRole::gift_result_name ||
                    p.role == sim::SteamHumanOverviewTextRole::gift_result_suffix
                ? 200.F
            : p.role == sim::SteamHumanOverviewTextRole::profession_bonus_description
                ? 128.F
                : 60.F;
        size *= std::min(1.F, width / std::max(1.F, skin_.text.width(value, size)));
        const float measured = skin_.text.width(value, size);
        float x = origin_.x + r[0], y = origin_.y + r[1];
        if (p.layout) {
            if (p.anchor & 2)
                x += (width - measured) / 2;
            else if (p.anchor & 4)
                x += width - measured;
            if (p.anchor & 0x20)
                y += (r[3] - size) / 2;
        } else {
            if (p.anchor & 2)
                x -= measured / 2;
            else if (p.anchor & 4)
                x -= measured;
            if (p.anchor & 0x20)
                y -= size / 2;
        }
        skin_.text.draw(value, x, y, color(p.rgb), size);
    }
    void learning(const sim::SteamHumanLearningText &p) const {
        const std::string name =
            std::array<const char *, 4>{"火魔法", "冰魔法", "雷魔法", "回复魔法"}.at(p.spell);
        const std::string suffix = "习得";
        const float width = skin_.text.width(name) + skin_.text.width(suffix);
        const auto at = point(p.position);
        const float x = at.x - ((p.anchor & 4) ? width : (p.anchor & 2) ? width / 2 : 0);
        skin_.text.draw(name, x, at.y, color(p.name_rgb));
        skin_.text.draw(suffix, x + skin_.text.width(name), at.y, color(p.suffix_rgb));
    }

  private:
    void image(const char *png, const char *seb, std::array<int, 2> at, int frame,
               const std::optional<std::array<int, 4>> &crop, Sprites::Binding binding) const {
        const auto p = point(at);
        if (crop) {
            const auto &r = *crop;
            if (r[2] > 0 && r[3] > 0)
                skin_.sprites.image(std::filesystem::path(png).filename().string(),
                                    {float(r[0]), float(r[1]), float(r[2]), float(r[3])},
                                    {p.x, p.y, float(r[2]), float(r[3])}, binding);
        } else {
            if (!seb)
                throw std::invalid_argument("Human image requires a crop or SEB");
            skin_.sprites.draw(std::filesystem::path(seb).filename().string(), frame, p, WHITE,
                               binding);
        }
    }
    void parts(const sim::SteamHumanSkinPlan &plan) const {
        for (const auto &p : plan.draws) {
            if (const auto *r = std::get_if<sim::StartupSkinRect>(&p))
                rectangle(*r);
            else if (const auto *i = std::get_if<sim::SteamHumanImage>(&p))
                human_image(*i);
            else if (const auto *t = std::get_if<sim::SteamHumanText>(&p)) {
                if (t->role != sim::SteamHumanTextRole::down || t->anchor != 2)
                    throw std::invalid_argument("Unsupported human portrait text");
                const auto at = point(t->position);
                skin_.text.draw(world_human_down_text,
                                at.x - skin_.text.width(world_human_down_text) / 2, at.y,
                                color(t->rgb));
            }
        }
    }
    Vector2 origin_;
    const Skin &skin_;
    std::string title_;
};
} // namespace

void prepare_world_human_detail(WorldHumanView &v, const sim::StartupWorldRuntimeState &state,
                                const Skin &skin, bool scrollbar_active) {
    v.detail_page_skin.reset();
    if (!v.initialized || (v.raw != 60 && !world_human_source_child(v)))
        return;
    sim::SteamHumanOverviewOptions o;
    o.view_y = 23; // Published helper input; outer desktop origin is applied once when drawing.
    o.finger_frame = 0; // No Owner animation clock mapping: never animate by drawing FPS.
    o.catalogue_scroll_first_touch = scrollbar_active; // Explicit PC capture, no Owner cursor.
    std::string title = v.raw == 73                  ? "装备信息"
                        : v.raw == 61                ? "职业一览 " + std::to_string(v.phase + 1) + "/2"
                        : v.raw == 64                ? "赠送礼物 " + std::to_string(v.phase + 1) + "/5"
                        : v.raw == 62                ? "职业一览"
                        : v.raw == 63                ? "转职"
                        : v.raw == 68                ? "装备变更"
                        : v.raw == 69                ? "能力上升"
                        : v.raw == 65 || v.raw == 66 ? "赠送礼物"
                                                     : "冒险者信息";
    if ((v.raw == 73 && v.rows.size() > 1) || v.raw == 62)
        title += " " + std::to_string(v.selection + 1) + "/" + std::to_string(v.rows.size());
    o.title_widths = std::array<int, 2>{int(skin.text.width(title)), int(skin.text.width(title))};
    o.profession_width = int(skin.text.width(v.profession));
    if (*o.profession_width >= 53)
        o.profession_repeat_width = int(skin.text.width(v.profession));
    o.bottom_widths = {int(skin.text.width("赠送礼物")), int(skin.text.width("赠送回复道具"))};
    o.profession_change_width = int(skin.text.width("转职"));
    o.equipment_hint_width = int(skin.text.width("按确认键赠送礼物"));
    o.magic_hint_width = int(skin.text.width("精通特定职业可以习得"));
    o.down_text_width = int(std::ceil(skin.text.width(world_human_down_text)));
    o.character_frame = 0; // Explicit static reward pose; original UserData clock is not modeled.
    o.insufficient_points_width = int(skin.text.width("村子点数不足"));
    std::string message = v.message;
    if (v.raw == 66 && v.choice && v.choice->slot == 4) {
        const auto item =
            std::find_if(state.rules->items.begin(), state.rules->items.end(),
                         [&](const auto &d) { return d.identity == v.choice->identity; });
        if (item == state.rules->items.end())
            throw std::invalid_argument("Gift result has no bound item");
        if (item->recovery > 0 && v.counter >= 115)
            message = "HP回复!";
        else if (item->recovery == 0 && item->effect == 9)
            message = std::string(
                          std::array<const char *, 4>{"火魔法", "冰魔法", "雷魔法", "回复魔法"}.at(
                              item->spell)) +
                      "习得";
    }
    o.message_width = int(skin.text.width(message));
    if (v.raw == 61)
        v.detail_page_skin = sim::steam_human_profession_catalogue_skin(state, v.page, o);
    else if (v.raw == 64) {
        if (v.phase == 4 && v.choice) {
            const auto item = std::find_if(state.rules->items.begin(), state.rules->items.end(),
                                           [&](const auto &d) { return d.identity == v.choice->identity; });
            if (item == state.rules->items.end())
                throw std::invalid_argument("Gift catalogue item is missing");
            if (item->effect < 6) {
                o.item_explain_widths[0] = int(skin.text.width(
                    std::array<const char *, 6>{"体力", "力量", "灵巧", "耐久", "魔力", "运气"}.at(item->effect)));
                o.item_explain_widths[1] = int(skin.text.width(item->effect == 0 ? "最大值上升" : "上升"));
            } else if (item->effect == 9)
                o.item_explain_widths[2] = int(skin.text.width(std::string(
                    std::array<const char *, 4>{"火魔法", "冰魔法", "雷魔法", "回复魔法"}.at(item->spell)) + "习得"));
        }
        v.detail_page_skin = sim::steam_human_gift_catalogue_skin(state, v.page, o);
    } else if (v.raw == 68 || v.raw == 69) {
        if (!v.choice)
            throw std::invalid_argument("Attribute result requires its committed gift binding");
        const float size = v.raw == 68 ? 11.F : 12.F;
        o.result_name_width = int(skin.text.width(v.choice->name, size));
        o.result_suffix_width = int(skin.text.width(v.raw == 68 ? "装备" : "赠送", size));
        v.detail_page_skin = sim::steam_human_attribute_result_skin(state, v.page, o);
    } else if (v.raw == 73)
        v.detail_page_skin = sim::steam_human_equipment_detail_skin(state, v.page, o);
    else if (v.raw == 65 && v.choice) {
        const int slot = v.choice->slot;
        o.comparison_name_widths = {
            int(skin.text.width(v.details.equipment[slot] ? v.equipment_names[slot] : "未装备",
                                10)),
            int(skin.text.width(v.choice->name, 10))};
        o.gift_accept_width = int(skin.text.width("赠送礼物"));
        v.detail_page_skin = sim::steam_human_gift_confirmation_skin(state, v.page, o);
    } else if (v.raw == 62 && v.choice) {
        o.profession_change_width =
            int(skin.text.width(v.details.profession == v.choice->identity ? "当前职业" : "转职"));
        v.detail_page_skin = sim::steam_human_profession_preview_skin(state, v.page, o);
    } else if (v.raw == 63)
        v.detail_page_skin = sim::steam_human_profession_result_skin(state, v.page, o);
    else if (v.raw == 66)
        v.detail_page_skin = sim::steam_human_gift_result_skin(state, v.page, o);
    else
        v.detail_page_skin = sim::steam_human_information_skin(state, v.page, o);
    if (!v.detail_page_skin)
        throw std::invalid_argument("Measured human page rejected its bound Owner");
}
void draw_world_human_skin(const sim::SteamHumanOverviewPlan &plan, Vector2 origin,
                           const Skin &skin) {
    const HumanSkinDraw draw(origin, skin, plan.title.empty() ? "冒险者信息" : plan.title);
    for (const auto &op : plan.operations) {
        if (const auto *p = std::get_if<sim::SteamStartupFrameDraw>(&op))
            draw.frame(*p);
        else if (const auto *p = std::get_if<sim::SteamHumanImage>(&op))
            draw.human_image(*p);
        else if (const auto *p = std::get_if<sim::SteamFacilityImage>(&op))
            draw.facility_image(*p);
        else if (const auto *p = std::get_if<sim::SteamFacilityNumber>(&op))
            draw.number(*p);
        else if (const auto *p = std::get_if<sim::SteamHumanOverviewText>(&op))
            draw.text(*p, plan.recovery_gift);
        else if (const auto *p = std::get_if<sim::SteamHumanDetailSkin>(&op))
            draw.portrait(*p);
        else if (const auto *p = std::get_if<sim::SteamHumanPortrait>(&op))
            draw.portrait(*p);
        else if (const auto *p = std::get_if<sim::SteamHumanOverviewScroll>(&op))
            for (const auto &r : p->rectangles)
                draw.rectangle(r);
        else if (const auto *p = std::get_if<sim::SteamHumanCatalogueScroll>(&op))
            for (const auto &r : p->rectangles)
                draw.rectangle(r);
        else if (const auto *p = std::get_if<sim::SteamHumanLearningText>(&op))
            draw.learning(*p);
        else if (const auto *p = std::get_if<sim::SteamFacilityArrowTouch>(&op)) {
            const auto expanded = arrow(*p, skin);
            const auto &c = expanded.crop;
            const auto &d = expanded.destination;
            skin.sprites.image(
                "buttoneffect.png", {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                {origin.x + d[0], origin.y + d[1], d[2], d[3]}, Sprites::Binding::steam_common);
        }
        // Source touch components are metadata. PC intents stay in world_human_input.
    }
}
std::optional<WorldHumanPaintedControls> world_human_painted_controls(const WorldHumanView &v,
                                                                      const WorldHumanLayout &base,
                                                                      Extent extent,
                                                                      const Skin &skin) {
    if (!v.initialized || (v.raw != 60 && !world_human_source_child(v)) || !v.detail_page_skin)
        return {};
    const auto layout = v.raw == 60 ? world_human_detail_layout(base)
                                   : world_human_subwindow_layout(base, v.raw);
    const auto origin = world_human_skin_origin(layout);
    WorldHumanPaintedControls out;
    out.page = v.page;
    out.raw = v.raw;
    out.phase = v.phase;
    out.selection = v.selection;
    out.human = v.human;
    out.choice = v.choice ? v.choice->identity : -1;
    out.extent = extent;
    out.first = v.first_visible;
    out.count = int(v.rows.size());
    out.back = layout.cancel;
    if (v.raw == 73)
        out.other_back = layout.confirm;
    if (v.raw == 61 || v.raw == 64 || v.raw == 63 || v.raw == 66 || v.raw == 68 || v.raw == 69)
        out.primary = layout.confirm;
    if (v.raw == 64 && v.phase != 4)
        out.inspect = layout.inspect;
    for (const auto &op : v.detail_page_skin->operations) {
        if (const auto *p = std::get_if<sim::SteamHumanOverviewText>(&op);
            p && (p->role == sim::SteamHumanOverviewTextRole::gift_accept ||
                  p->role == sim::SteamHumanOverviewTextRole::profession_change)) {
            const auto &r = p->rectangle;
            out.primary = Rectangle{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
        } else if (const auto *p = std::get_if<sim::SteamFacilityArrowTouch>(&op)) {
            const auto expanded = arrow(*p, skin);
            const auto &r = expanded.touch_rectangle;
            const Rectangle hit{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            if (p->value == 16)
                out.previous = hit;
            else if (p->value == 18)
                out.next = hit;
            else
                throw std::invalid_argument("Unsupported human arrow intent");
        } else if (const auto *p = std::get_if<sim::SteamHumanEquipmentRow>(&op); p && p->touch.rectangle) {
            const auto &r = *p->touch.rectangle;
            // The PC row is the painted source row clipped to its window. Original platform
            // margin/focus processing remains separate from this explicit desktop mapping.
            const float left = std::max(layout.panel.x, origin.x + r[0]);
            const float right = std::min(layout.panel.x + layout.panel.width, origin.x + r[0] + r[2]);
            const int index = p->touch.value & 0xFFFF;
            const int identity = v.raw == 60 ? index : v.rows.at(index).identity;
            out.rows.push_back({index, identity, {left, origin.y + r[1], right - left, float(r[3])}});
        } else if (const auto *p = std::get_if<sim::SteamHumanCatalogueScroll>(&op)) {
            const auto &r = p->rectangles[0].rect;
            out.scroll = Rectangle{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            out.scroll_count = p->arguments[0];
            out.scroll_visible = p->arguments[1];
        } else if (const auto *p = std::get_if<sim::SteamHumanOverviewScroll>(&op)) {
            const auto &r = p->rectangles[0].rect;
            out.scroll = Rectangle{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            out.scroll_count = 4;
            out.scroll_visible = 1;
        }
    }
    return out; // Stores painted coordinates and semantic tags, never a snapshot/Owner or clock.
}
} // namespace ark::desktop::ui
