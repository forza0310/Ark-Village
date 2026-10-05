#pragma once

// e8f66d9 building pages read the canonical Owner. Selection and responsive geometry are
// desktop state; only intents cross to the simulation thread, carrying stable page/definition IDs.
#include "../world_build_placement.hpp"
#include "ark/simulation/startup_world_building.hpp"
#include "layout.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldBuildingRow {
    int identity{}; // Definition for raw21; human definition for raw80, never a row index.
    std::string name;
    std::int64_t cost{};
    WorldBuildGraphic graphic{}; // Empty for human residence rows.
};
struct WorldBuildingView {
    int raw{};
    std::uint64_t page{};
    bool initialized{}, can_confirm{};
    std::string title;
    std::optional<std::uint64_t> facility;
    std::array<std::vector<WorldBuildingRow>, 3> catalogs;
    std::vector<WorldBuildingRow> residents;
    int phase{}, page_count{};
    std::array<std::int64_t, 4> attributes{};
    std::int64_t income{};
    std::size_t neighbours{};
    std::array<std::array<std::int64_t, 3>, 3> upgrade{};
};
struct WorldBuildingLayout {
    Rectangle panel, body, rows, cancel, confirm, previous, next;
    std::array<Rectangle, 3> tabs;
    float row_height{38};
};
struct WorldBuildingSelection {
    int tab{}, selected{}, first_row{};
};
struct WorldBuildingInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{};
    int wheel_rows{};
};
enum class WorldBuildingAction {
    select_build,
    cancel_build,
    facility_previous,
    facility_next,
    facility_confirm,
    facility_cancel,
    residence_select,
    residence_cancel,
    confirm_upgrade
};
struct WorldBuildingIntent {
    WorldBuildingAction action;
    std::uint64_t page{};
    int selection{};
};
bool world_building_page(const simulation::rules::WorldScriptPage &page);
WorldBuildingView world_building_view(const simulation::StartupWorldRuntimeState &state,
                                      const simulation::rules::WorldScriptPage &page);
WorldBuildingLayout world_building_layout(Extent extent);
int world_building_visible_rows(const WorldBuildingLayout &layout);
std::optional<WorldBuildingIntent> world_building_input(const WorldBuildingView &view,
                                                        const WorldBuildingLayout &layout,
                                                        WorldBuildingSelection &selection,
                                                        const WorldBuildingInput &input,
                                                        bool blocked);
void draw_world_building(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                         const Skin &skin, const WorldBuildingSelection &selection, bool enabled,
                         const std::string &feedback = {});
} // namespace ark::desktop::ui
