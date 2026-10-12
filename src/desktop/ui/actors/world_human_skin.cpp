// Execute the formal raw60 page0/1 stream. Font/GPU adaptation stays outside the Owner.
#include "../common/skin.hpp"
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
    default:
        throw std::invalid_argument("Human skin text has no bound value");
    }
}
// This small executor owns no state or animation clock. It also serves the old page2/3 portrait.
class HumanSkinDraw {
  public:
    HumanSkinDraw(Vector2 origin, const Skin &skin) : origin_(origin), skin_(skin) {}
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
        const auto group = std::string(resource->published_image).rfind("original/", 0) == 0
                               ? Sprites::Binding::common
                               : Sprites::Binding::steam_common;
        image(resource->published_image, resource->published_sprite, p.position, p.frame, p.crop,
              group);
    }
    void facility_image(const sim::SteamFacilityImage &p) const {
        const auto resource = sim::steam_facility_resource(p.asset);
        if (!resource)
            throw std::invalid_argument("Missing human number/label resource");
        const auto group = p.asset == sim::SteamFacilityAsset::number05 ||
                                   p.asset == sim::SteamFacilityAsset::number08
                               ? Sprites::Binding::steam_common
                               : Sprites::Binding::common;
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
        for (const auto &digit : *digits)
            facility_image(digit);
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
            skin_.text.draw("冒险者信息", origin_.x + float(t->rectangle[0]),
                            origin_.y + float(t->rectangle[1]), color(*t->rgb));
        }
    }
    void text(const sim::SteamHumanOverviewText &p, bool recovery) const {
        const auto value = label(p, recovery);
        const auto &r = p.rectangle;
        float size = p.font_size ? float(p.font_size) : 12.F;
        // Desktop Chinese typography fits the complete text into source columns.
        const float width = r[2] > 0                                                ? float(r[2])
                            : p.role == sim::SteamHumanOverviewTextRole::name       ? 86.F
                            : p.role == sim::SteamHumanOverviewTextRole::profession ? 53.F
                            : p.role == sim::SteamHumanOverviewTextRole::attribute  ? 38.F
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
        }
        skin_.text.draw(value, x, y, color(p.rgb), size);
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
};
} // namespace

void prepare_world_human_detail(WorldHumanView &v, const sim::StartupWorldRuntimeState &state,
                                const Skin &skin) {
    v.detail_page_skin.reset();
    if (!v.initialized || v.raw != 60 || v.phase > 1)
        return;
    sim::SteamHumanOverviewOptions o;
    o.view_y = 23; // Published helper input; outer desktop origin is applied once when drawing.
    o.finger_frame = 0; // No Owner animation clock mapping: never animate by drawing FPS.
    o.title_widths =
        std::array<int, 2>{int(skin.text.width("冒险者信息")), int(skin.text.width("冒险者信息"))};
    o.profession_width = int(skin.text.width(v.profession));
    if (*o.profession_width >= 53)
        o.profession_repeat_width = int(skin.text.width(v.profession));
    o.bottom_widths = {int(skin.text.width("赠送礼物")), int(skin.text.width("赠送回复道具"))};
    o.profession_change_width = int(skin.text.width("转职"));
    o.down_text_width = int(std::ceil(skin.text.width(world_human_down_text)));
    v.detail_page_skin = v.phase == 0 ? sim::steam_human_overview_skin(state, v.page, o)
                                      : sim::steam_human_attributes_skin(state, v.page, o);
    if (!v.detail_page_skin)
        throw std::invalid_argument("Measured human page rejected its bound Owner");
}
void draw_world_human_portrait(const sim::SteamHumanDetailSkin &plan, Vector2 origin,
                               const Skin &skin) {
    HumanSkinDraw(origin, skin).portrait(plan);
}
void draw_world_human_skin(const sim::SteamHumanOverviewPlan &plan, Vector2 origin,
                           const Skin &skin) {
    const HumanSkinDraw draw(origin, skin);
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
        else if (const auto *p = std::get_if<sim::SteamHumanOverviewScroll>(&op))
            for (const auto &r : p->rectangles)
                draw.rectangle(r);
        // Source touch components are metadata. PC intents stay in world_human_input.
    }
}
} // namespace ark::desktop::ui
