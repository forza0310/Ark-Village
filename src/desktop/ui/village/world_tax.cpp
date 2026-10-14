// The Owner freezes resident references and owns taxes, selection and clocks. This adapter
// reads that payload, executes the Steam plan and submits one complete device-input frame.
#include "world_tax.hpp"
#include "../common/skin.hpp"
#include "../information/world_information.hpp"
#include "ark/simulation/actors/startup_world_profile.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
namespace sim = simulation;
constexpr const char *tax_title = "住宅税收";
Rectangle translated(std::array<int, 4> r, Vector2 origin) {
    return {origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
}
} // namespace
bool world_tax_page(const sim::rules::WorldScriptPage &page) {
    return page.kind == sim::rules::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 90 || page.legacy_page == 98);
}
WorldTaxView world_tax_view(const sim::StartupWorldRuntimeState &state,
                            const sim::rules::WorldScriptPage &page) {
    if (!state.rules || !world_tax_page(page))
        throw std::invalid_argument("Tax view requires a supported page and catalogue");
    WorldTaxView view;
    view.raw = page.legacy_page;
    view.page = page.id;
    // raw98 automatically posts taxes. A waiting90 never rebuilds its missing Init payload.
    if (view.raw == 98 || page.lifecycle != 2 || !state.tax_page_residents.count(page.id))
        return view;
    const auto tax = sim::inspect_startup_world_tax_page(state, page.id);
    if (!tax)
        throw std::invalid_argument("Initialized tax page lost its source payload");
    view.total = tax->total;
    view.selection = tax->selection;
    view.first_visible = tax->first_visible;
    for (const auto &row : tax->rows) {
        const auto human = sim::startup_world_human_profile(state, row.definition);
        if (!human)
            throw std::invalid_argument("Tax page references a missing resident profile");
        view.rows.push_back({row.definition, row.amount, human->name});
    }
    view.initialized = true;
    view.interactive = !state.scene.framework_paused;
    return view;
}
WorldTaxLayout world_tax_layout(Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("Tax page requires the supported logical viewport");
    WorldTaxLayout out;
    out.extent = extent;
    out.panel = {(extent.width - 220.F) / 2, (extent.height - 168.F) / 2, 220, 168};
    out.origin = world_list_skin_origin(out.panel);
    out.rows = translated({3, 94, 231, 95}, out.origin);
    out.total = translated({30, 200, 182, 12}, out.origin);
    out.row_height = 19;
    return out;
}
void prepare_world_tax(WorldTaxView &view, const sim::StartupWorldRuntimeState &state,
                       const Skin &skin, bool scrolling) {
    view.skin.reset();
    if (!view.initialized)
        return;
    sim::SteamInformationSkinOptions options;
    options.view_y = 23;
    options.scroll_first_touch = scrolling;
    options.title_widths =
        std::array<int, 2>{int(skin.text.width(tax_title)), int(skin.text.width(tax_title))};
    view.skin = sim::steam_tax_information_skin(state, view.page, options);
    if (!view.skin)
        throw std::invalid_argument("Tax skin rejected its initialized Owner payload");
    // Translations stay in this page adapter; the shared information GPU executor is reused.
    for (auto &request : view.skin->draws)
        if (auto *text = std::get_if<sim::SteamInformationText>(&request)) {
            using Role = sim::SteamInformationTextRole;
            switch (text->role) {
            case Role::tax_title:
                text->value = tax_title;
                break;
            case Role::tax_owner_header:
                text->value = "房主";
                break;
            case Role::tax_income_header:
                text->value = "收入";
                break;
            case Role::tax_total_label:
                text->value = "合计";
                break;
            default:
                break;
            }
        }
}
std::optional<WorldFacilityPaintedControls>
world_tax_painted_controls(const WorldTaxView &view, const WorldTaxLayout &layout) {
    if (!view.initialized || view.raw != 90 || !view.skin)
        return {};
    WorldFacilityPaintedControls controls;
    controls.page = view.page;
    controls.raw = view.raw;
    controls.selection = view.selection;
    controls.first = view.first_visible;
    controls.count = int(view.rows.size());
    controls.extent = layout.extent;
    for (const auto &touch : view.skin->touches) {
        if (!touch.rectangle)
            continue; // Touch2 without a rectangle cannot create a full-screen confirmation.
        auto r = *touch.rectangle;
        r[0] -= touch.margin[0];
        r[1] -= touch.margin[2];
        r[2] += touch.margin[0] + touch.margin[1];
        r[3] += touch.margin[2] + touch.margin[3];
        if (touch.component == 11 && (touch.value & 0xffff0000) == 0x20000) {
            const int index = touch.value & 0xffff;
            if (index >= controls.count || touch.option != 0)
                throw std::invalid_argument("Tax row lost its source index or no-marker policy");
            controls.rows.push_back(
                {index, view.rows.at(index).identity, translated(r, layout.origin)});
        } else if (touch.component == 12 && touch.value == 0x40000 && touch.scroll_arguments) {
            controls.scroll = translated(r, layout.origin);
            controls.scroll_visible = (*touch.scroll_arguments)[1];
        }
    }
    return controls;
}
std::optional<sim::StartupWorldTaxInput>
world_tax_input(const WorldTaxView &view, const WorldTaxLayout &layout,
                WorldFacilityGesture &gesture, const WorldTaxInput &input, bool blocked,
                const std::optional<WorldFacilityPaintedControls> &painted) {
    if (gesture.page != view.page) {
        gesture = {};
        gesture.page = view.page;
    }
    const bool fresh =
        painted && painted->page == view.page && painted->raw == view.raw &&
        painted->selection == view.selection && painted->first == view.first_visible &&
        painted->count == int(view.rows.size()) && painted->extent.width == layout.extent.width &&
        painted->extent.height == layout.extent.height;
    const bool usable =
        !blocked && !input.focus_lost && view.initialized && view.interactive && view.raw == 90;
    const auto drag = world_facility_scroll_input(
        gesture, fresh ? &*painted : nullptr, usable && fresh, input.pointer, input.pointer_pressed,
        input.pointer_down, input.pointer_released);
    if (!usable)
        return {};
    sim::StartupWorldTaxInput frame;
    frame.up = input.up;
    frame.down = input.down;
    frame.confirm = input.enter;
    if (input.wheel_rows)
        frame.scroll = sim::StartupWorldTaxInput::Scroll{false, input.wheel_rows};
    if (drag.absolute)
        frame.scroll = sim::StartupWorldTaxInput::Scroll{true, *drag.absolute};
    if (fresh && !drag.consume_click)
        for (const auto &row : painted->rows)
            if (row.index >= 0 && row.index < int(view.rows.size()) &&
                row.identity == view.rows[row.index].identity &&
                world_facility_hit(input.click, row.rectangle)) {
                frame.selection = row.index;
                frame.confirm = true; // Component11 has no marker: one release acknowledges.
                break;
            }
    if (frame.up || frame.down || frame.confirm || frame.selection || frame.scroll)
        return frame;
    return {}; // Escape has no tax consumer and cannot acknowledge the report.
}
std::optional<WorldFacilityPaintedControls>
draw_world_tax(const WorldTaxView &view, const WorldTaxLayout &layout, const Skin &skin) {
    if (!view.initialized || view.raw == 98)
        return {};
    if (!view.skin)
        throw std::invalid_argument("Tax rendering requires its measured source plan");
    sim::StartupInformationPageView source;
    source.raw = view.raw;
    draw_information_plan(*view.skin, source, layout.origin, skin);
    return world_tax_painted_controls(view, layout);
}
} // namespace ark::desktop::ui
