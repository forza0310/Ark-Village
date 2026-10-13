#pragma once

// 75 consumes owned items; 76 applies shared improvements; 77 only displays the result.
#include "../../scene/world_build_placement.hpp"
#include "../common/layout.hpp"
#include "ark/simulation/facilities/startup_world_facility_items.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "world_facility_input.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldFacilityItemRow {
    int identity{}, owned{};
    std::string name;
    std::array<int, 3> hint{}; // Source k bins, not the final multiplied/clamped attribute deltas.
    std::vector<simulation::StartupVisualDraw> icon;
};
struct WorldFacilityItemsView {
    std::uint64_t page{}, facility{};
    int raw{}, counter{}, selection{}, first_visible{}, response{};
    bool initialized{}, can_confirm{}, can_cancel{};
    std::string title, name;
    std::optional<std::int64_t> profit; // Cumulative month income minus costs, facility kinds3/9.
    WorldBuildGraphic graphic;
    std::vector<WorldFacilityItemRow> rows;
    std::optional<WorldFacilityItemRow> choice;
    std::array<std::array<std::int64_t, 3>, 3> attributes{};
    std::optional<simulation::SteamFacilityEffectPlan> skin;
};
struct WorldFacilityItemsLayout {
    Extent extent;
    Rectangle panel, heading, rows, hints, feedback, cancel, confirm;
    float row_height{};
};
struct WorldFacilityItemsInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{};
    int wheel_rows{};
    bool keyboard_event{}; // Any key-down clears the source mouse marker.
    std::optional<Vector2> pointer{};
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
};
// User-approved ADR-0058 maps the researched component/absolute-row marker to PC clicks.
using WorldFacilityItemsSelection = WorldFacilityGesture;
struct WorldFacilityItemsIntent {
    simulation::StartupFacilityItemAction action;
    std::uint64_t page{};
    int selection{-1};
    std::optional<simulation::StartupFacilityItemInput> input{};
};
bool world_facility_items_page(const simulation::rules::WorldScriptPage &page);
WorldFacilityItemsView world_facility_items_view(const simulation::StartupWorldRuntimeState &state,
                                                 const simulation::rules::WorldScriptPage &page);
WorldFacilityItemsLayout world_facility_items_layout(Extent extent, int raw = 75);
Rectangle world_facility_item_highlight(const WorldFacilityItemsLayout &layout, int visible_row);
std::optional<WorldFacilityItemsIntent> world_facility_items_input(
    const WorldFacilityItemsView &view, const WorldFacilityItemsLayout &layout,
    WorldFacilityItemsSelection &selection, const WorldFacilityItemsInput &input, bool blocked,
    const std::optional<WorldFacilityPaintedControls> &painted = {}, Extent extent = {});
void prepare_world_facility_items(WorldFacilityItemsView &view,
                                  const simulation::StartupWorldRuntimeState &state,
                                  const Skin &skin, bool scrolling = false);
std::optional<WorldFacilityPaintedControls>
draw_world_facility_items(const WorldFacilityItemsView &view,
                          const WorldFacilityItemsLayout &layout, const Skin &skin, bool enabled,
                          const std::string &feedback = {});
} // namespace ark::desktop::ui
