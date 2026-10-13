#pragma once

// Source-owned equipment catalogue and facility publicity. The view is read-only;
// all selection, phase gates and delayed popularity remain in the canonical Owner.
#include "../common/layout.hpp"
#include "ark/simulation/facilities/startup_world_facility_catalog.hpp"
#include "ark/simulation/presentation/steam_facility_catalog_skin.hpp"
#include "ark/simulation/presentation/steam_facility_reputation_skin.hpp"
#include "world_facility_input.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldFacilityCatalogRow {
    int identity{}, price{};
    std::string name;
    std::array<int, 4> combat{};
    bool fresh{};
    int icon_image{}, icon{};
};
struct WorldFacilityCatalogView {
    std::uint64_t page{};
    int raw{}, phase{}, counter{}, selection{}, first_visible{};
    bool initialized{};
    std::string title;
    std::vector<WorldFacilityCatalogRow> rows;
    std::optional<WorldFacilityCatalogRow> choice;
    std::vector<std::string> participants;
    std::optional<simulation::SteamHumanOverviewPlan> skin;
    std::optional<simulation::SteamFacilityReputationPlan> reputation_skin;
};
struct WorldFacilityCatalogLayout {
    Extent extent;
    Rectangle panel, rows, status, cancel, confirm, inspect, previous, next;
    float row_height{};
};
struct WorldFacilityCatalogInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{}, inspect{};
    int wheel_rows{};
    std::optional<Vector2> pointer{};
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
};
struct WorldFacilityCatalogIntent {
    simulation::StartupFacilityCatalogAction action;
    int selection{};
    std::optional<simulation::StartupFacilityCatalogInput> input{};
};
bool world_facility_catalog_page(const simulation::rules::WorldScriptPage &page);
WorldFacilityCatalogView
world_facility_catalog_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page);
WorldFacilityCatalogLayout world_facility_catalog_layout(Extent extent, int raw = 79);
std::optional<WorldFacilityCatalogIntent>
world_facility_catalog_input(const WorldFacilityCatalogView &view,
                             const WorldFacilityCatalogLayout &layout,
                             const WorldFacilityCatalogInput &input, bool blocked,
                             const std::optional<WorldFacilityPaintedControls> &painted = {},
                             Extent extent = {}, WorldFacilityGesture *gesture = nullptr);
void prepare_world_facility_catalog(WorldFacilityCatalogView &view,
                                    const simulation::StartupWorldRuntimeState &state,
                                    const Skin &skin, bool scrolling = false);
std::optional<WorldFacilityPaintedControls>
draw_world_facility_catalog(const WorldFacilityCatalogView &view,
                            const WorldFacilityCatalogLayout &layout, const Skin &skin,
                            bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
