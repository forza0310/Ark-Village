#pragma once

// Source-owned equipment catalogue and facility publicity. The view is read-only;
// all selection, phase gates and delayed popularity remain in the canonical Owner.
#include "ark/simulation/startup_world_facility_catalog.hpp"
#include "layout.hpp"
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
};
struct WorldFacilityCatalogView {
    std::uint64_t page{};
    int raw{}, phase{}, counter{}, selection{}, first_visible{};
    bool initialized{};
    std::string title;
    std::vector<WorldFacilityCatalogRow> rows;
    std::optional<WorldFacilityCatalogRow> choice;
    std::vector<std::string> participants;
};
struct WorldFacilityCatalogLayout {
    Rectangle panel, rows, status, cancel, confirm, inspect, previous, next;
    float row_height{};
};
struct WorldFacilityCatalogInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{}, inspect{};
    int wheel_rows{};
};
struct WorldFacilityCatalogIntent {
    simulation::StartupFacilityCatalogAction action;
    int selection{};
};
bool world_facility_catalog_page(const simulation::rules::WorldScriptPage &page);
WorldFacilityCatalogView
world_facility_catalog_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page);
WorldFacilityCatalogLayout world_facility_catalog_layout(Extent extent);
std::optional<WorldFacilityCatalogIntent>
world_facility_catalog_input(const WorldFacilityCatalogView &view,
                             const WorldFacilityCatalogLayout &layout,
                             const WorldFacilityCatalogInput &input, bool blocked);
void draw_world_facility_catalog(const WorldFacilityCatalogView &view,
                                 const WorldFacilityCatalogLayout &layout, const Skin &skin,
                                 bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
