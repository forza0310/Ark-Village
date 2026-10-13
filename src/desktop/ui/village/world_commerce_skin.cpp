// Facility blueprints and committed single-item receipts share the source commerce terminal.
#include "../common/skin.hpp"
#include "world_commerce_top.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
Vector2 origin(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Commerce source needs a supported viewport");
    return {float((extent.width - 240) / 2), float((extent.height - 256) / 2)};
}
Rectangle translated(std::array<int, 4> r, Vector2 offset) {
    return {offset.x + r[0], offset.y + r[1], float(r[2]), float(r[3])};
}
} // namespace

void prepare_world_commerce_source(WorldCommerceView &view,
                                   const sim::StartupWorldRuntimeState &state, const Skin &skin,
                                   bool scrolling) {
    view.source_skin.reset();
    if (!view.initialized)
        return;
    // A paid86 can be published before its real Init. Wait for the source payload;
    // never manufacture page caches or extend this automatically retiring page.
    if (view.raw == 86 && !state.commerce_pages_initialized.count(view.page)) {
        view.initialized = false;
        return;
    }
    if (view.raw == 85) {
        sim::SteamCommerceFacilityOptions options;
        options.view_y = 23;
        options.scroll_first_touch = scrolling;
        options.text.title = view.title;
        options.text.facility_header = "设施";
        options.text.price_header = "价格";
        options.text.explanation = "购买后即可建设该设施";
        options.text.title_widths =
            std::array<int, 2>{int(skin.text.width(view.title)), int(skin.text.width(view.title))};
        options.text.price_width = int(skin.text.width(options.text.price_header));
        view.source_skin = sim::steam_commerce_facility_skin(state, view.page, options);
    } else if (view.raw == 86 && view.choice) {
        sim::SteamCommerceResultOptions options;
        options.view_y = 23;
        options.finger_frame = 0; // Same explicit PC static-hand policy as83.
        options.text.title = "南瓜商会";
        options.text.cash_label = "所持金";
        options.text.acknowledgement = "了解";
        options.text.messages = {"已购买", "已出售"};
        options.text.title_widths = std::array<int, 2>{int(skin.text.width(options.text.title)),
                                                       int(skin.text.width(options.text.title))};
        options.text.name_width = int(skin.text.width(view.choice->name));
        options.text.acknowledgement_width = int(skin.text.width(options.text.acknowledgement));
        view.source_skin = sim::steam_commerce_result_skin(state, view.page, options);
    } else
        throw std::invalid_argument("Source commerce preparation requires85/86");
    if (!view.source_skin)
        throw std::invalid_argument("Commerce source plan rejected its real page payload");
}

WorldFacilityPaintedControls world_commerce_source_controls(const WorldCommerceView &view,
                                                            Extent extent) {
    if (!view.source_skin || view.raw != 85 || !view.choice)
        throw std::invalid_argument("Commerce facility controls require a prepared85");
    const auto offset = origin(extent);
    WorldFacilityPaintedControls controls;
    controls.page = view.page;
    controls.raw = view.raw;
    controls.selection = view.selection;
    controls.first = view.first_visible;
    controls.count = int(view.rows.size());
    controls.choice = view.choice->identity;
    controls.extent = extent;
    controls.cancel = {float(extent.width - 58), float(extent.height - 25), 54, 21};
    controls.inspect = {4, float(extent.height - 25), 54, 21};
    controls.scroll_visible = 3;
    bool frame = false;
    for (const auto &operation : view.source_skin->operations) {
        if (const auto *part = std::get_if<sim::SteamStartupFrameDraw>(&operation)) {
            if (!frame && std::holds_alternative<sim::StartupSkinRect>(*part))
                frame = true;
        } else if (const auto *row = std::get_if<sim::SteamHumanEquipmentRow>(&operation)) {
            const int index = row->touch.value & 0xffff;
            if (!row->touch.rectangle || index >= controls.count || row->touch.option != 16)
                throw std::invalid_argument("Commerce facility row lost its source marker");
            auto rectangle = *row->touch.rectangle;
            rectangle[0] -= row->margin[0];
            rectangle[1] -= row->margin[2];
            rectangle[2] += row->margin[0] + row->margin[1];
            rectangle[3] += row->margin[2] + row->margin[3];
            controls.rows.push_back(
                {index, view.rows.at(index).identity, translated(rectangle, offset)});
        } else if (const auto *scroll = std::get_if<sim::SteamHumanCatalogueScroll>(&operation)) {
            if (!scroll->touch.rectangle)
                throw std::invalid_argument("Commerce facility scrollbar lost its source track");
            controls.scroll = translated(*scroll->touch.rectangle, offset);
        }
    }
    if (!frame || int(controls.rows.size()) != std::min(3, controls.count - controls.first) ||
        !controls.scroll)
        throw std::invalid_argument("Commerce facility plan lost its painted geometry");
    return controls;
}

std::optional<sim::StartupCommerceFacilityInput>
world_commerce_facility_input(const WorldCommerceView &view, Extent extent,
                              const std::optional<WorldFacilityPaintedControls> &painted,
                              WorldFacilityGesture &gesture, const WorldCommerceInput &input,
                              bool blocked) {
    if (gesture.page != view.page || input.focus_lost) {
        gesture = {};
        gesture.page = view.page;
    }
    if (input.keyboard_event || input.up || input.down || input.enter || input.escape ||
        input.inspect || input.wheel_rows)
        gesture.marked_row.reset();
    const bool fresh =
        painted && painted->page == view.page && painted->raw == 85 &&
        painted->selection == view.selection && painted->first == view.first_visible &&
        painted->count == int(view.rows.size()) &&
        painted->choice == (view.choice ? view.choice->identity : -1) &&
        painted->extent.width == extent.width && painted->extent.height == extent.height;
    if (!fresh)
        gesture.marked_row.reset();
    const auto drag = world_facility_scroll_input(
        gesture, fresh ? &*painted : nullptr,
        fresh && !blocked && !input.focus_lost && view.initialized && view.raw == 85, input.pointer,
        input.pointer_pressed, input.pointer_down, input.pointer_released);
    if (blocked || input.focus_lost || !view.initialized || view.raw != 85)
        return {};
    sim::StartupCommerceFacilityInput out{input.up,      input.down, input.enter, input.escape,
                                          input.inspect, {},         {}};
    if (input.wheel_rows)
        out.scroll = sim::StartupCommerceFacilityInput::Scroll{false, input.wheel_rows};
    if (drag.absolute)
        out.scroll = sim::StartupCommerceFacilityInput::Scroll{true, *drag.absolute};
    if (fresh && !drag.consume_click) {
        out.cancel |= world_facility_hit(input.click, painted->cancel);
        out.inspect |= world_facility_hit(input.click, painted->inspect);
        for (const auto &row : painted->rows)
            if (row.index >= 0 && row.index < int(view.rows.size()) &&
                row.identity == view.rows[row.index].identity &&
                world_facility_hit(input.click, row.rectangle)) {
                out.selection = row.index;
                out.confirm = gesture.marked_row == row.index && view.selection == row.index;
                gesture.marked_row = row.index;
            }
    }
    if (out.up || out.down || out.confirm || out.cancel || out.inspect || out.selection ||
        out.scroll)
        return out;
    return {};
}

std::optional<WorldFacilityPaintedControls>
draw_world_commerce_source(const WorldCommerceView &view, Extent extent, const Skin &skin,
                           bool enabled, const std::string &feedback) {
    if (!view.initialized)
        return {};
    if (!view.source_skin)
        throw std::invalid_argument("Commerce drawing requires its measured source plan");
    draw_world_commerce_plan(*view.source_skin, origin(extent), skin);
    if (view.raw == 86)
        return {}; // The painted acknowledgement has no PC input; Owner closes it automatically.
    const auto controls = world_commerce_source_controls(view, extent);
    skin.button(controls.cancel, "返回", enabled);
    skin.button(controls.inspect, "情报", enabled);
    if (!feedback.empty())
        skin.text.draw(feedback, origin(extent).x + 10, origin(extent).y + 215, MAROON, 9);
    return controls;
}
} // namespace ark::desktop::ui
