#pragma once

// 2b479f6 building pages read the canonical Owner. Selection and responsive geometry are
// desktop state; only intents cross to the simulation thread, carrying stable page/definition IDs.
#include "../world_build_placement.hpp"
#include "ark/app/world_facility_queries.hpp"
#include "ark/simulation/startup_world_building.hpp"
#include "ark/simulation/startup_world_visuals.hpp"
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
    WorldBuildGraphic graphic{};                   // Empty for human residence rows.
    std::optional<int> residence_qualifications{}; // Published H, independent of the gold quote.
    std::string common_image{}; // Special -1/-2 rows bind PNG indices, never guessed SEB frames.
    Rectangle image_source{};
    Vector2 image_offset{};
};
struct WorldFacilityBonusRow {
    simulation::StartupFacilityBonusRow source;
    simulation::StartupVisualDraw icon;
};
struct WorldBuildingView {
    int raw{};
    std::uint64_t page{};
    bool initialized{}, can_confirm{}, definition_preview{}, can_use_items{}, can_view_products{};
    std::string title;
    std::optional<std::uint64_t> facility;
    WorldBuildGraphic graphic;
    app::WorldFacilityTemplate detail_type{app::WorldFacilityTemplate::ordinary};
    int level{};
    std::optional<std::int64_t> remaining_uses;    // Source d()-K; absent at shared MAX.
    std::optional<std::int64_t> cumulative_profit; // Instance kind3/9, source months0..current.
    std::vector<WorldFacilityBonusRow> bonus_rows; // Page Y order, including repeated sources.
    std::optional<simulation::StartupVisualDraw> category_icon;
    std::vector<simulation::StartupFacilityExitEffectDraw> exit_effects;
    std::optional<std::size_t> product_count;
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
    Rectangle footer_name{}, footer_profit{};
    std::array<Rectangle, 3> tabs;
    float row_height{38};
};
// Responsive desktop positions for the published raw74 static subset. Picture dimensions
// follow the 97x74 source region; the source image group is fitted inside this desktop region.
struct WorldBuildingDetailLayout {
    Rectangle name, price, picture, level, values, effects, remaining, maintenance, sources,
        source_heading, source_footer, source_scroll;
    float source_row_height{19};
};
WorldBuildingDetailLayout world_building_detail_layout(const WorldBuildingLayout &layout,
                                                       app::WorldFacilityTemplate type);
struct WorldBuildingSelection {
    int tab{}, selected{}, first_row{};
};
struct WorldBuildingIcon {
    Rectangle clip;
    Vector2 anchor;
    Color background{190, 242, 230, 255};
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
WorldBuildingLayout world_building_layout(Extent extent, int raw = 21);
int world_building_visible_rows(const WorldBuildingLayout &layout);
WorldBuildingIcon world_building_icon(const WorldBuildingLayout &layout, int visible_row);
std::optional<WorldBuildingIntent> world_building_input(const WorldBuildingView &view,
                                                        const WorldBuildingLayout &layout,
                                                        WorldBuildingSelection &selection,
                                                        const WorldBuildingInput &input,
                                                        bool blocked);
void draw_world_building(const WorldBuildingView &view, const WorldBuildingLayout &layout,
                         const Skin &skin, const WorldBuildingSelection &selection, bool enabled,
                         const std::string &feedback = {});
} // namespace ark::desktop::ui
