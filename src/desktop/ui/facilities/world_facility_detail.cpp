#include "world_facility_detail.hpp"
#include "../common/skin.hpp"
#include "../common/steam_plan_draw.hpp"
#include "ark/simulation/presentation/steam_manual_skin.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string_view>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
using Role = sim::SteamFacilityDetailTextRole;
Color color(const std::array<int, 3> &rgb) {
    return {static_cast<unsigned char>(rgb[0]), static_cast<unsigned char>(rgb[1]),
            static_cast<unsigned char>(rgb[2]), 255};
}
std::string label(const sim::SteamFacilityDetailText &part, const WorldBuildingView &view) {
    switch (part.role) {
    case Role::title:
        return view.definition_preview || view.page_count == 1
                   ? "设施信息"
                   : "设施信息 " + std::to_string(part.argument) + "/2";
    case Role::name:
    case Role::bonus_name:
        return part.value;
    case Role::price:
        return "价格";
    case Role::parameter:
        return part.argument == 1 ? "品质" : "魅力";
    case Role::remaining:
        return "距离下个等级还有";
    case Role::maximum_level:
        return "达到最高等级";
    case Role::no_effects:
        return "没有效果";
    case Role::bonus_title:
        return "设施加成";
    case Role::maintenance:
        return "维护费";
    case Role::no_bonus:
        return "没有奖励";
    case Role::bottom:
        return part.value.empty() ? view.phase == 0 ? "使用道具" : "周围设施的加成" : part.value;
    case Role::bonus_value:
        return std::array<const char *, 3>{"价格", "品质", "魅力"}.at(part.argument);
    case Role::equipment_count:
        return "种类";
    case Role::sale_description:
    case Role::plant_description:
    case Role::recruitment_status:
        return part.value;
    case Role::plant_price:
        return part.argument == 0 ? "价格" : part.value;
    case Role::plant_quality:
        return part.argument == 0 ? "品质" : part.value;
    }
    throw std::invalid_argument("Unknown ordinary facility detail text role");
}
} // namespace

void prepare_world_building_detail(WorldBuildingView &view,
                                   const sim::StartupWorldRuntimeState &state,
                                   const WorldBuildingSelection &selection, const Skin &skin) {
    view.detail_skin.reset();
    if (!view.initialized || view.raw != 74 || view.detail_type == app::WorldFacilityTemplate::home)
        return;
    const auto title =
        view.page_count == 2 ? "设施信息 " + std::to_string(view.phase + 1) + "/2" : "设施信息";
    const std::array<int, 2> title_widths{int(skin.text.width(title)), int(skin.text.width(title))};
    if (view.detail_type != app::WorldFacilityTemplate::ordinary) {
        const auto page = std::find_if(state.scripts.pages.begin(), state.scripts.pages.end(),
                                       [&](const auto &p) { return p.id == view.page; });
        if (page == state.scripts.pages.end())
            throw std::invalid_argument("Special facility page disappeared during projection");
        const auto &definition = state.rules->facilities.at(page->legacy_f);
        sim::SteamFacilitySpecialSkinOptions options;
        options.view_y = 23;
        options.finger_frame = 0;
        options.title_widths = title_widths;
        if (view.detail_type == app::WorldFacilityTemplate::equipment) {
            options.sale_text = std::string(definition.detail == 1   ? "武器"
                                            : definition.detail == 4 ? "防具"
                                                                     : "饰品") +
                                "出售中";
            if (!view.definition_preview)
                options.bottom_text = "查看商品";
        } else if (view.detail_type == app::WorldFacilityTemplate::recruitment) {
            const auto candidates = sim::startup_world_residence_candidates(state);
            if (!candidates)
                throw std::invalid_argument("Recruitment detail lost its actual candidate source");
            options.applicant_text =
                candidates->empty() ? "现在没有希望入住的人"
                                    : "现在有" + std::to_string(candidates->size()) + "人希望入住";
            if (!view.definition_preview)
                options.bottom_text = "入住希望者";
        } else {
            if (definition.neighbour_effects.size() < 2)
                throw std::invalid_argument("Plant detail lost its adjacency definition");
            auto money = std::to_string(definition.neighbour_effects[0].delta);
            const std::size_t first = money.front() == '-' ? 1 : 0;
            for (auto offset = money.size(); offset > first + 3;) {
                offset -= 3;
                money.insert(offset, ",");
            }
            options.plant_money_text = money + "G";
            if (!view.definition_preview)
                options.bottom_text = "会给周围带来好的影响";
        }
        if (options.bottom_text)
            options.bottom_width = int(skin.text.width(*options.bottom_text));
        view.detail_skin = sim::steam_facility_special_skin(state, view.page, options);
        if (!view.detail_skin)
            throw std::invalid_argument("Special facility detail rejected its real page payload");
        return;
    }
    sim::SteamFacilityDetailSkinOptions options;
    // The published VIEW_Y23 is translated into the approved 224x172 desktop panel.
    // The hand stays at static frame0 until a source animation-clock contract is wired.
    options.view_y = 23;
    options.cursor = {selection.selected, selection.first_row};
    options.title_widths = title_widths;
    options.bottom_width =
        static_cast<int>(skin.text.width(view.phase == 0 ? "使用道具" : "周围设施的加成"));
    view.detail_skin = view.definition_preview
                           ? sim::steam_facility_definition_skin(state, view.page, options)
                           : sim::steam_facility_detail_skin(state, view.page, options);
    if (!view.detail_skin)
        throw std::invalid_argument("Ordinary facility detail plan rejected its bound Owner");
}

WorldBuildingLayout world_building_source_layout(const WorldBuildingView &view,
                                                 WorldBuildingLayout layout, const Skin &skin) {
    if (!view.detail_skin)
        return layout;
    layout.confirm = layout.previous = layout.next = {};
    const Vector2 origin{layout.panel.x - 8, layout.panel.y - 45};
    for (const auto &operation : view.detail_skin->operations) {
        if (const auto *touch = std::get_if<sim::SteamStartupTouch>(&operation);
            touch && touch->component == 4 && touch->value == 20 && touch->rectangle &&
            !view.definition_preview && view.can_confirm) {
            const auto &r = *touch->rectangle;
            layout.confirm = {origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
        } else if (const auto *arrow = std::get_if<sim::SteamFacilityArrowTouch>(&operation)) {
            const auto geometry = steam_plan_arrow(*arrow, skin);
            const auto &r = geometry.touch_rectangle;
            const Rectangle rectangle{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            if (arrow->value == 16)
                layout.previous = rectangle;
            else if (arrow->value == 18)
                layout.next = rectangle;
        }
    }
    return layout;
}

void draw_world_facility_detail(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                                const Skin &skin, const WorldBuildingSelection &selection) {
    if (!view.detail_skin)
        throw std::invalid_argument("Ordinary facility detail has no measured plan");
    const Vector2 origin{layout.panel.x - 8, layout.panel.y - 45};
    std::vector<Rectangle> clips;
    const auto box = [&](const std::array<int, 4> &r) {
        return Rectangle{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
    };
    const auto active_clip = [&]() -> std::optional<Rectangle> {
        return clips.empty() ? std::nullopt : std::optional{clips.back()};
    };
    const auto rectangle = [&](const sim::StartupSkinRect &r) {
        if (r.outline)
            DrawRectangleLinesEx(box(r.rect), 1, color(r.rgb));
        else
            DrawRectangleRec(box(r.rect), color(r.rgb));
    };
    const auto image = [&](const sim::SteamFacilityImage &part) {
        const auto resource = sim::steam_facility_resource(part.asset);
        if (!resource)
            throw std::invalid_argument("Unknown detail artwork request");
        const auto binding = std::string_view(resource->published_image).rfind("original/", 0) == 0
                                 ? Sprites::Binding::common
                                 : Sprites::Binding::steam_common;
        const Vector2 anchor{origin.x + part.position[0], origin.y + part.position[1]};
        if (part.crop) {
            const auto &r = *part.crop;
            if (r[2] <= 0 || r[3] <= 0)
                return;
            SpriteBlit blit{{float(r[0]), float(r[1]), float(r[2]), float(r[3])},
                            {anchor.x, anchor.y, float(r[2]), float(r[3])}};
            if (const auto clip = active_clip()) {
                const auto cut = clip_sprite_blit(blit, *clip);
                if (!cut)
                    return;
                blit = *cut;
            }
            skin.sprites.image(std::filesystem::path(resource->published_image).filename().string(),
                               blit.source, blit.destination, binding);
        } else if (part.frame >= 0 && resource->published_sprite) {
            skin.sprites.draw(std::filesystem::path(resource->published_sprite).filename().string(),
                              part.frame, anchor, WHITE, binding, 1, -1, active_clip());
        }
    };
    const auto text = [&](const sim::SteamFacilityDetailText &part) {
        const auto value = label(part, view);
        // Approved Chinese desktop typography: retain source columns and fit complete
        // text using actual font widths, with the existing twelve-pixel UI as the base.
        const bool special = view.detail_type != app::WorldFacilityTemplate::ordinary;
        const float base = special && part.font_size > 0 ? float(part.font_size) : 12.F;
        float width = part.rectangle[2] > 0 ? float(part.rectangle[2]) : 220.F;
        if (part.role == Role::bonus_name) {
            const auto row = std::find_if(
                view.bonus_rows.begin() +
                    std::min<std::size_t>(selection.first_row, view.bonus_rows.size()),
                view.bonus_rows.end(), [&](const auto &r) { return r.source.name == value; });
            width =
                (row != view.bonus_rows.end() && row->source.values.size() == 2 ? 130.F : 151.F) -
                44.F - 2;
        } else if (part.role == Role::remaining)
            width = 137;
        else if (part.role == Role::bonus_value && part.rectangle[2] == 0)
            width = 219.F - part.rectangle[0];
        const auto suffix = part.role == Role::bonus_value ? part.value : std::string{};
        const float size =
            base * std::min(1.F, width / std::max(1.F, skin.text.width(value + suffix, base)));
        const float measured = skin.text.width(value + suffix, size);
        const int anchor = part.anchor.value_or(1);
        const bool layout_text = part.method == sim::SteamFacilityTextMethod::text_layout;
        float x = origin.x + part.rectangle[0];
        if (layout_text) {
            if (anchor & 2)
                x += std::max(0.F, (width - measured) / 2);
            else if (anchor & 4)
                x += width - measured;
        } else if (anchor & 2)
            x -= measured / 2;
        else if (anchor & 4)
            x -= measured;
        // Special templates retain their published10/11px labels and layout midpoint.
        // Ordinary two-page typography keeps the approved12px desktop adaptation.
        const float y =
            origin.y + part.rectangle[1] +
            (special && layout_text && (anchor & 0x20) ? (part.rectangle[3] - size) / 2 : 0);
        const auto draw = [&](const std::string &s, float at, Color tint) {
            if (const auto clip = active_clip())
                skin.text.clipped(s, at, y, *clip, tint, size);
            else
                skin.text.draw(s, at, y, tint, size);
        };
        draw(value, x, part.role == Role::bonus_value ? Color{0, 101, 255, 255} : color(part.rgb));
        if (!suffix.empty())
            draw(suffix, x + skin.text.width(value, size), color(part.rgb));
    };
    // Source touch requests remain ordered metadata. PC mouse/keyboard actions are
    // translated by world_building_input; this loop never sends a simulation command.
    for (const auto &operation : view.detail_skin->operations) {
        if (const auto *part = std::get_if<sim::SteamFacilityDraw>(&operation)) {
            if (const auto *r = std::get_if<sim::StartupSkinRect>(part))
                rectangle(*r);
            else if (const auto *i = std::get_if<sim::SteamFacilityImage>(part))
                image(*i);
            else if (const auto *c = std::get_if<sim::SteamFacilityClip>(part)) {
                if (c->kind == sim::SteamFacilityClipKind::pop) {
                    if (clips.empty())
                        throw std::invalid_argument("Detail clip underflow");
                    clips.pop_back();
                } else {
                    auto next = box(c->rectangle);
                    if (!clips.empty())
                        next = GetCollisionRec(next, clips.back());
                    clips.push_back(next);
                }
            } else if (const auto *m = std::get_if<sim::SteamFacilityMapchip2>(part)) {
                const auto pieces = sim::steam_facility_mapchip2_draws(*m);
                if (!pieces)
                    throw std::invalid_argument("Invalid detail mapchip request");
                for (const auto &piece : *pieces)
                    skin.sprites.draw(piece.sprite, piece.frame,
                                      {origin.x + piece.position[0], origin.y + piece.position[1]},
                                      WHITE, Sprites::Binding::map, 1, -1, active_clip());
            } else if (const auto *n = std::get_if<sim::SteamFacilityNumber>(part)) {
                const auto resource = sim::steam_facility_resource(n->asset);
                if (!resource || !resource->published_sprite)
                    throw std::invalid_argument("Invalid detail number asset");
                const auto digits = sim::steam_facility_number_draws(
                    *n, skin.sprites.common_digit_width(
                            std::filesystem::path(resource->published_sprite).filename().string()));
                if (!digits)
                    throw std::invalid_argument("Invalid detail number request");
                for (const auto &digit : *digits)
                    image(digit);
            } else
                throw std::invalid_argument("Unexpected upgrade text in detail plan");
        } else if (const auto *part = std::get_if<sim::SteamFacilityDetailText>(&operation))
            text(*part);
        else if (const auto *part = std::get_if<sim::SteamFacilityArrowTouch>(&operation)) {
            const auto resource =
                sim::steam_facility_resource(sim::SteamFacilityAsset::title_arrows);
            const auto bounds = skin.sprites.frame_bounds(
                std::filesystem::path(resource->published_sprite).filename().string(), part->frame,
                Sprites::Binding::common);
            const auto arrow = sim::steam_manual_arrow_skin(
                {part->position, part->frame, false},
                {int(bounds.x), int(bounds.y), int(bounds.width), int(bounds.height)}, 128);
            if (!arrow)
                throw std::invalid_argument("Invalid detail title arrow helper");
            const auto &c = arrow->crop;
            const auto &d = arrow->destination;
            skin.sprites.image(
                "buttoneffect.png", {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                {origin.x + d[0], origin.y + d[1], d[2], d[3]}, Sprites::Binding::steam_common);
        } else if (const auto *part = std::get_if<sim::SteamFacilityScrollRequest>(&operation)) {
            const auto scroll = sim::steam_facility_scroll_skin(*part, selection.first_row);
            if (!scroll)
                throw std::invalid_argument("Invalid detail scroll request");
            for (const auto &r : scroll->rectangles)
                rectangle(r);
        }
    }
    if (!clips.empty())
        throw std::invalid_argument("Detail clip leak");
}
} // namespace ark::desktop::ui
