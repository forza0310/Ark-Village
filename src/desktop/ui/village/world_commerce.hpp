#pragma once

// Commerce84–86 keeps buy/sell direction and source scroll; raw83 has its own ordered top skin.
#include "../../scene/world_build_placement.hpp"
#include "../common/layout.hpp"
#include "../common/world_list_input.hpp"
#include "ark/simulation/facilities/startup_world_commerce.hpp"
#include "ark/simulation/presentation/startup_world_visuals.hpp"
#include "ark/simulation/presentation/steam_commerce_skin.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldCommerceRow {
    int identity{}, price{}, remaining{}, owned{};
    std::string name;
    bool fresh{};
    WorldBuildGraphic graphic;
    std::vector<simulation::StartupVisualDraw> icon;
};
struct WorldCommerceView {
    std::uint64_t page{};
    int raw{}, mode{}, tab{}, selection{}, first_visible{}, counter{}, feedback_counter{}, points{};
    std::int64_t funds{};
    // Ready to display: raw86 may project a committed receipt before its automatic initialization.
    bool initialized{}, can_confirm{}, can_cancel{}, can_inspect{};
    std::string title;
    std::vector<WorldCommerceRow> rows;
    std::optional<WorldCommerceRow> choice;
    std::optional<simulation::SteamCommerceSkinPlan> source_skin;
};
struct WorldCommerceLayout {
    Rectangle panel, heading, rows, status, feedback, cancel, confirm, inspect;
    std::array<Rectangle, 2> tabs;
    float row_height{};
};
struct WorldCommerceInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{}, inspect{};
    int wheel_rows{};
    bool focus_lost{};
    bool keyboard_event{};
    std::optional<Vector2> pointer{};
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
};
struct WorldCommerceIntent {
    simulation::StartupCommerceAction action;
    std::uint64_t page{};
    int selection{}; // Source list index; never a definition identity.
};
bool world_commerce_page(const simulation::rules::WorldScriptPage &page);
WorldCommerceView world_commerce_view(const simulation::StartupWorldRuntimeState &state,
                                      const simulation::rules::WorldScriptPage &page);
WorldCommerceLayout world_commerce_layout(Extent extent);
// Raw85/86 use the source plan; preparation, drawing and gestures never transact.
void prepare_world_commerce_source(WorldCommerceView &view,
                                   const simulation::StartupWorldRuntimeState &state,
                                   const Skin &skin, bool scrolling = false);
WorldFacilityPaintedControls world_commerce_source_controls(const WorldCommerceView &view,
                                                            Extent extent);
std::optional<simulation::StartupCommerceFacilityInput>
world_commerce_facility_input(const WorldCommerceView &view, Extent extent,
                              const std::optional<WorldFacilityPaintedControls> &painted,
                              WorldFacilityGesture &gesture, const WorldCommerceInput &input,
                              bool blocked);
std::optional<WorldFacilityPaintedControls>
draw_world_commerce_source(const WorldCommerceView &view, Extent extent, const Skin &skin,
                           bool enabled, const std::string &feedback = {});
std::optional<WorldCommerceIntent> world_commerce_input(const WorldCommerceView &view,
                                                        const WorldCommerceLayout &layout,
                                                        const WorldCommerceInput &input,
                                                        bool blocked);
void draw_world_commerce(const WorldCommerceView &view, const WorldCommerceLayout &layout,
                         const Skin &skin, bool enabled, const std::string &feedback = {},
                         std::optional<std::int64_t> last_amount = {});
} // namespace ark::desktop::ui
