#pragma once

// 75 consumes owned items; 76 applies shared improvements; 77 only displays the result.
#include "../world_build_placement.hpp"
#include "ark/simulation/startup_world_facility_items.hpp"
#include "layout.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldFacilityItemRow {
    int identity{}, owned{};
    std::string name;
    std::array<int, 3> hint{}; // Source k bins, not the final multiplied/clamped attribute deltas.
};
struct WorldFacilityItemsView {
    std::uint64_t page{}, facility{};
    int raw{}, counter{}, selection{}, first_visible{}, response{};
    bool initialized{}, can_confirm{}, can_cancel{};
    std::string title, name;
    WorldBuildGraphic graphic;
    std::vector<WorldFacilityItemRow> rows;
    std::optional<WorldFacilityItemRow> choice;
    std::array<std::array<std::int64_t, 3>, 3> attributes{};
};
struct WorldFacilityItemsLayout {
    Rectangle panel, heading, rows, hints, feedback, cancel, confirm;
    float row_height{};
};
struct WorldFacilityItemsInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{};
    int wheel_rows{};
};
struct WorldFacilityItemsIntent {
    simulation::StartupFacilityItemAction action;
    std::uint64_t page{};
    int selection{-1};
};
bool world_facility_items_page(const simulation::rules::WorldScriptPage &page);
WorldFacilityItemsView world_facility_items_view(const simulation::StartupWorldRuntimeState &state,
                                                 const simulation::rules::WorldScriptPage &page);
WorldFacilityItemsLayout world_facility_items_layout(Extent extent);
std::optional<WorldFacilityItemsIntent>
world_facility_items_input(const WorldFacilityItemsView &view,
                           const WorldFacilityItemsLayout &layout,
                           const WorldFacilityItemsInput &input, bool blocked);
void draw_world_facility_items(const WorldFacilityItemsView &view,
                               const WorldFacilityItemsLayout &layout, const Skin &skin,
                               bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
