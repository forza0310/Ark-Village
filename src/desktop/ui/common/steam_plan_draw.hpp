#pragma once

// Short-lived GPU/font execution for maintained ordered plans; no Owner or animation clock.
#include "ark/presentation/script_text.hpp"
#include "ark/simulation/presentation/steam_human_skin.hpp"
#include "ark/simulation/presentation/steam_information_skin.hpp"
#include "ark/simulation/presentation/steam_manual_skin.hpp"
#include "skin.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <vector>

namespace ark::desktop::ui {
namespace sim = simulation;
inline Color steam_plan_color(const std::array<int, 3> &c) {
    return {static_cast<unsigned char>(c[0]), static_cast<unsigned char>(c[1]),
            static_cast<unsigned char>(c[2]), 255};
}
inline sim::SteamManualArrowPlan steam_plan_arrow(const sim::SteamFacilityArrowTouch &p,
                                                  const Skin &skin) {
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
class SteamPlanDraw {
  public:
    SteamPlanDraw(Vector2 origin, const Skin &skin, std::string title, std::string down_text = {})
        : origin_(origin), skin_(skin), title_(std::move(title)), down_text_(std::move(down_text)) {
    }
    Vector2 point(std::array<int, 2> p) const { return {origin_.x + p[0], origin_.y + p[1]}; }
    void rectangle(const sim::StartupSkinRect &p) const {
        const auto &r = p.rect;
        const Rectangle box{origin_.x + r[0], origin_.y + r[1], float(r[2]), float(r[3])};
        if (p.outline)
            DrawRectangleLinesEx(box, 1, steam_plan_color(p.rgb));
        else
            DrawRectangleRec(box, steam_plan_color(p.rgb));
    }
    void human_image(const sim::SteamHumanImage &p) const {
        const auto resource = sim::steam_human_resource(p.asset);
        if (!resource)
            throw std::invalid_argument("Missing human skin resource");
        published(resource->published_image, resource->published_sprite, p.position, p.frame,
                  p.crop);
    }
    void facility_image(const sim::SteamFacilityImage &p) const {
        const auto resource = sim::steam_facility_resource(p.asset);
        if (!resource)
            throw std::invalid_argument("Missing human number/label resource");
        published(resource->published_image, resource->published_sprite, p.position, p.frame,
                  p.crop);
    }
    // The published path selects the precise resource family, including differing Steam PNGs.
    void published(const char *png, const char *seb, std::array<int, 2> at, int frame,
                   std::optional<std::array<int, 4>> crop = {}) const {
        const std::string source = png;
        const auto group = source.rfind("original/event/", 0) == 0     ? Sprites::Binding::event
                           : source.rfind("original/effect/", 0) == 0  ? Sprites::Binding::effect
                           : source.rfind("original/common2/", 0) == 0 ? Sprites::Binding::common2
                           : source.rfind("original/", 0) == 0         ? Sprites::Binding::common
                                                               : Sprites::Binding::steam_common;
        image(png, seb, at, frame, crop, group);
    }
    void clip(const sim::SteamFacilityClip &p) {
        if (p.kind == sim::SteamFacilityClipKind::pop) {
            if (clips_.empty())
                throw std::invalid_argument("Unbalanced source clip");
            clips_.pop_back();
        } else {
            const auto &r = p.rectangle;
            Rectangle box{origin_.x + r[0], origin_.y + r[1], float(r[2]), float(r[3])};
            if (!clips_.empty())
                box = intersection(box, clips_.back());
            clips_.push_back(box); // Keep zero-height source clips; they hide their whole subtree.
        }
    }
    void finish() const {
        if (!clips_.empty())
            throw std::invalid_argument("Unclosed source clip");
    }
    // SetClip replaces the current bounds; its matching pop restores the previous bounds.
    void replace_clip(const std::array<int, 4> &r) {
        clips_.push_back({origin_.x + r[0], origin_.y + r[1], float(r[2]), float(r[3])});
    }
    // Raw21 SetClip replaces all prior bounds; ClearClip removes them rather than popping.
    void set_clip(const std::array<int, 4> &r) {
        clips_.assign(1, {origin_.x + r[0], origin_.y + r[1], float(r[2]), float(r[3])});
    }
    void clear_clip() { clips_.clear(); }
    void whole_image(const char *png, std::array<int, 2> position) const {
        const std::string source = png;
        const auto group = source.rfind("original/event/", 0) == 0 ? Sprites::Binding::event
                           : source.rfind("original/common/", 0) == 0
                               ? Sprites::Binding::common
                               : Sprites::Binding::steam_common;
        skin_.sprites.full_image(std::filesystem::path(png).filename().string(), point(position),
                                 group, active_clip());
    }
    void mapchip(const sim::SteamFacilityMapchip2 &p) const {
        const auto pieces = sim::steam_facility_mapchip2_draws(p);
        if (!pieces)
            throw std::invalid_argument("Invalid facility mapchip request");
        for (const auto &piece : *pieces)
            skin_.sprites.draw(piece.sprite, piece.frame, point(piece.position), WHITE,
                               Sprites::Binding::map, 1, -1, active_clip());
    }
    void mapchip(const sim::SteamFacilityMapchipDraw &p) const {
        skin_.sprites.draw(p.sprite, p.frame, point(p.position), WHITE, Sprites::Binding::map, 1,
                           -1, active_clip());
    }
    void monster(int sprite, int image, int frame, std::array<int, 2> position) const {
        skin_.sprites.actor(true, sprite, image, frame, point(position), 1, active_clip());
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
    // Information counts retain their comma digits and independent person/monster units.
    // Steam common binds only its published differences, with the source SEB offsets intact.
    void information_number(const sim::SteamInformationNumber &p) const {
        const auto parts = sim::steam_information_number_draws(p);
        if (!parts)
            throw std::invalid_argument("Information number request rejected");
        for (const auto &part : *parts) {
            if (part.package != sim::StartupSkinPackage::common || part.sprite < 0 ||
                part.frame < 0)
                throw std::invalid_argument("Information number lost its common sprite identity");
            skin_.sprites.indexed_sprite(Sprites::Binding::steam_common, part.sprite, part.frame,
                                         part.layer, part.image, point(part.offset), 1,
                                         active_clip());
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
        auto clip = active_clip();
        if (p.clip) {
            const auto &r = *p.clip;
            const Rectangle local{origin_.x + r[0], origin_.y + r[1], float(r[2]), float(r[3])};
            clip = clip ? intersection(*clip, local) : local;
        }
        const auto anchor = point(p.position);
        if (p.actor.shadow) {
            const auto &s = *p.actor.shadow;
            skin_.sprites.indexed_sprite(Sprites::Binding::common, s.sprite, s.frame, s.layer,
                                         s.image, {anchor.x + s.offset[0], anchor.y + s.offset[1]},
                                         1, clip);
        }
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
                            origin_.y + float(t->rectangle[1]), steam_plan_color(*t->rgb));
        }
    }
    void text(const sim::SteamHumanOverviewText &p, const std::string &measured_value) const {
        const auto &value = measured_value;
        if (p.rich) {
            literal(value, p.rectangle, p.anchor, p.font_size, p.layout, p.rgb, 200, true);
            return;
        }
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
            : p.role == sim::SteamHumanOverviewTextRole::profession_bonus_description ? 128.F
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
        plain(value, x, y, steam_plan_color(p.rgb), size);
    }
    void literal(const std::string &value, const std::array<int, 4> &r, int anchor, int font_size,
                 bool layout, const std::array<int, 3> &rgb, float available_width,
                 bool rich = false) const {
        const auto decoded = rich ? decode_script_text(value) : ScriptText{};
        const auto &visible = rich ? decoded.text : value;
        float size = font_size ? float(font_size) : 12.F;
        const float width = r[2] > 0 ? float(r[2]) : available_width;
        size *= std::min(1.F, width / std::max(1.F, skin_.text.width(visible, size)));
        const float measured = skin_.text.width(visible, size);
        float x = origin_.x + r[0], y = origin_.y + r[1];
        if (layout) {
            if (anchor & 2)
                x += (width - measured) / 2;
            else if (anchor & 4)
                x += width - measured;
            if (anchor & 0x20)
                y += (r[3] - size) / 2;
        } else {
            if (anchor & 2)
                x -= measured / 2;
            else if (anchor & 4)
                x -= measured;
            if (anchor & 0x20)
                y -= size / 2;
        }
        if (!rich) {
            plain(value, x, y, steam_plan_color(rgb), size);
            return;
        }
        for (const auto &run : decoded.runs) {
            const Color color = run.rgb ? Color{static_cast<unsigned char>(*run.rgb >> 16),
                                                static_cast<unsigned char>(*run.rgb >> 8),
                                                static_cast<unsigned char>(*run.rgb), 255}
                                        : steam_plan_color(rgb);
            plain(run.text, x, y, color, size);
            x += skin_.text.width(run.text, size);
        }
    }
    void learning(const sim::SteamHumanLearningText &p) const {
        const std::string name =
            std::array<const char *, 4>{"火魔法", "冰魔法", "雷魔法", "回复魔法"}.at(p.spell);
        const std::string suffix = "习得";
        const float width = skin_.text.width(name) + skin_.text.width(suffix);
        const auto at = point(p.position);
        const float x = at.x - ((p.anchor & 4) ? width : (p.anchor & 2) ? width / 2 : 0);
        skin_.text.draw(name, x, at.y, steam_plan_color(p.name_rgb));
        skin_.text.draw(suffix, x + skin_.text.width(name), at.y, steam_plan_color(p.suffix_rgb));
    }

  private:
    static Rectangle intersection(Rectangle a, Rectangle b) {
        const float x = std::max(a.x, b.x), y = std::max(a.y, b.y);
        return {x, y, std::max(0.F, std::min(a.x + a.width, b.x + b.width) - x),
                std::max(0.F, std::min(a.y + a.height, b.y + b.height) - y)};
    }
    std::optional<Rectangle> active_clip() const {
        return clips_.empty() ? std::nullopt : std::optional{clips_.back()};
    }
    void plain(const std::string &value, float x, float y, Color color, float size) const {
        if (const auto clip = active_clip()) {
            if (clip->width > 0 && clip->height > 0)
                skin_.text.clipped(value, x, y, *clip, color, size);
        } else
            skin_.text.draw(value, x, y, color, size);
    }
    void image(const char *png, const char *seb, std::array<int, 2> at, int frame,
               const std::optional<std::array<int, 4>> &crop, Sprites::Binding binding) const {
        const auto p = point(at);
        if (crop) {
            const auto &r = *crop;
            if (r[2] <= 0 || r[3] <= 0)
                return;
            SpriteBlit blit{{float(r[0]), float(r[1]), float(r[2]), float(r[3])},
                            {p.x, p.y, float(r[2]), float(r[3])}};
            if (const auto clip = active_clip()) {
                const auto cropped = clip_sprite_blit(blit, *clip);
                if (!cropped)
                    return;
                blit = *cropped;
            }
            skin_.sprites.image(std::filesystem::path(png).filename().string(), blit.source,
                                blit.destination, binding);
        } else {
            if (!seb)
                throw std::invalid_argument("Human image requires a crop or SEB");
            if (frame < 0)
                return; // Source-negative numeric frames have no published pixel mapping.
            skin_.sprites.draw(std::filesystem::path(seb).filename().string(), frame, p, WHITE,
                               binding, 1, -1, active_clip());
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
                skin_.text.draw(down_text_, at.x - skin_.text.width(down_text_) / 2, at.y,
                                steam_plan_color(t->rgb));
            }
        }
    }
    Vector2 origin_;
    const Skin &skin_;
    std::string title_, down_text_;
    std::vector<Rectangle> clips_;
};
} // namespace ark::desktop::ui
