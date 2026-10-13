// Execute maintained human page streams. Font/GPU adaptation stays outside the Owner.
#include "../common/steam_plan_draw.hpp"
#include "ark/simulation/presentation/steam_manual_skin.hpp"
#include "world_human_detail.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
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
    std::string title = v.raw == 73   ? "装备信息"
                        : v.raw == 61 ? "职业一览 " + std::to_string(v.phase + 1) + "/2"
                        : v.raw == 64 ? "赠送礼物 " + std::to_string(v.phase + 1) + "/5"
                        : v.raw == 62 ? "职业一览"
                        : v.raw == 63 ? "转职"
                        : v.raw == 68 ? "装备变更"
                        : v.raw == 69 ? "能力上升"
                        : v.raw == 70 ? "职业大师"
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
    const auto character_frame = sim::steam_human_shared_character_frame(state);
    if (!character_frame)
        throw std::invalid_argument("Invalid shared human character clock");
    o.character_frame = *character_frame; // Read entry_updates; drawing never advances this clock.
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
    if (v.raw == 70) {
        // Match the exact source labels before measuring; the plan handles attribute/magic
        // branches, signed clocks and the final choice without advancing either Owner clock.
        constexpr std::array<const char *, 6> attributes{"体力", "力",   "器用",
                                                         "丈夫", "魔力", "运"};
        constexpr std::array<const char *, 4> spells{"火魔法", "冰魔法", "雷魔法", "回复魔法"};
        const auto name = v.mastery_attribute >= 10 ? spells.at(v.mastery_attribute - 10)
                                                    : attributes.at(v.mastery_attribute);
        o.mastery_widths = {int(skin.text.width(name)),
                            int(skin.text.width(std::to_string(v.mastery_value))),
                            int(skin.text.width("上升了!"))};
        o.mastery_button_widths = {int(skin.text.width("转职")), int(skin.text.width("保持现状"))};
        v.detail_page_skin = sim::steam_human_mastery_skin(state, v.page, o);
    } else if (v.raw == 61)
        v.detail_page_skin = sim::steam_human_profession_catalogue_skin(state, v.page, o);
    else if (v.raw == 64) {
        if (v.phase == 4 && v.choice) {
            const auto item =
                std::find_if(state.rules->items.begin(), state.rules->items.end(),
                             [&](const auto &d) { return d.identity == v.choice->identity; });
            if (item == state.rules->items.end())
                throw std::invalid_argument("Gift catalogue item is missing");
            if (item->effect < 6) {
                o.item_explain_widths[0] = int(skin.text.width(
                    std::array<const char *, 6>{"体力", "力量", "灵巧", "耐久", "魔力", "运气"}.at(
                        item->effect)));
                o.item_explain_widths[1] =
                    int(skin.text.width(item->effect == 0 ? "最大值上升" : "上升"));
            } else if (item->effect == 9)
                o.item_explain_widths[2] = int(skin.text.width(
                    std::string(
                        std::array<const char *, 4>{"火魔法", "冰魔法", "雷魔法", "回复魔法"}.at(
                            item->spell)) +
                    "习得"));
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
    SteamPlanDraw draw(origin, skin, plan.title.empty() ? "冒险者信息" : plan.title,
                       world_human_down_text);
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
            draw.text(*p, label(*p, plan.recovery_gift));
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
        else if (const auto *p = std::get_if<sim::SteamHumanClip>(&op))
            draw.clip({p->kind == sim::SteamHumanClipKind::push_intersect
                           ? sim::SteamFacilityClipKind::push_intersect
                           : sim::SteamFacilityClipKind::pop,
                       p->rectangle});
        else if (const auto *p = std::get_if<sim::SteamFacilityMapchip2>(&op))
            draw.mapchip(*p);
        else if (const auto *p = std::get_if<sim::SteamFacilityArrowTouch>(&op)) {
            const auto expanded = steam_plan_arrow(*p, skin);
            const auto &c = expanded.crop;
            const auto &d = expanded.destination;
            skin.sprites.image(
                "buttoneffect.png", {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                {origin.x + d[0], origin.y + d[1], d[2], d[3]}, Sprites::Binding::steam_common);
        }
        // Source touch components are metadata. PC intents stay in world_human_input.
    }
    draw.finish();
}
std::optional<WorldHumanPaintedControls> world_human_painted_controls(const WorldHumanView &v,
                                                                      const WorldHumanLayout &base,
                                                                      Extent extent,
                                                                      const Skin &skin) {
    if (!v.initialized || (v.raw != 60 && !world_human_source_child(v)) || !v.detail_page_skin)
        return {};
    const auto layout =
        v.raw == 60 ? world_human_detail_layout(base) : world_human_subwindow_layout(base, v.raw);
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
    if (v.raw == 61 || v.raw == 64 || v.raw == 63 || v.raw == 66 || v.raw == 68 || v.raw == 69 ||
        (v.raw == 70 && v.phase < 2))
        out.primary = layout.confirm;
    if (v.raw == 64 && v.phase != 4)
        out.inspect = layout.inspect;
    for (const auto &op : v.detail_page_skin->operations) {
        if (const auto *p = std::get_if<sim::SteamStartupTouch>(&op);
            p && v.raw == 70 && v.phase == 2 && p->component == 3 && p->rectangle) {
            const int choice = p->value & 0xFFFF;
            if ((p->value & 0xFFFF0000) != 0x20000 || choice > 1)
                throw std::invalid_argument("Invalid mastery choice touch");
            const auto &r = *p->rectangle;
            const float left = std::max(layout.panel.x, origin.x + r[0]);
            const float top = std::max(layout.panel.y, origin.y + r[1]);
            const float right =
                std::min(layout.panel.x + layout.panel.width, origin.x + r[0] + r[2]);
            const float bottom =
                std::min(layout.panel.y + layout.panel.height, origin.y + r[1] + r[3]);
            out.mastery_buttons[choice] =
                Rectangle{left, top, std::max(0.F, right - left), std::max(0.F, bottom - top)};
        } else if (const auto *p = std::get_if<sim::SteamHumanOverviewText>(&op);
                   p && (p->role == sim::SteamHumanOverviewTextRole::gift_accept ||
                         p->role == sim::SteamHumanOverviewTextRole::profession_change)) {
            const auto &r = p->rectangle;
            out.primary = Rectangle{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
        } else if (const auto *p = std::get_if<sim::SteamFacilityArrowTouch>(&op)) {
            const auto expanded = steam_plan_arrow(*p, skin);
            const auto &r = expanded.touch_rectangle;
            const Rectangle hit{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
            if (p->value == 16)
                out.previous = hit;
            else if (p->value == 18)
                out.next = hit;
            else
                throw std::invalid_argument("Unsupported human arrow intent");
        } else if (const auto *p = std::get_if<sim::SteamHumanEquipmentRow>(&op);
                   p && p->touch.rectangle) {
            const auto &r = *p->touch.rectangle;
            // The PC row is the painted source row clipped to its window. Original platform
            // margin/focus processing remains separate from this explicit desktop mapping.
            const float left = std::max(layout.panel.x, origin.x + r[0]);
            const float right =
                std::min(layout.panel.x + layout.panel.width, origin.x + r[0] + r[2]);
            const int index = p->touch.value & 0xFFFF;
            const int identity = v.raw == 60 ? index : v.rows.at(index).identity;
            out.rows.push_back(
                {index, identity, {left, origin.y + r[1], right - left, float(r[3])}});
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
