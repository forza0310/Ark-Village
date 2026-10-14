#pragma once

#include "../common/world_list_input.hpp"
#include "ark/simulation/presentation/steam_information_skin.hpp"
#include "ark/simulation/village/startup_world_tax.hpp"

namespace ark::desktop::ui {
class Skin;
struct WorldTaxRow {
    int identity{}, amount{};
    std::string name;
};
struct WorldTaxView {
    int raw{}, total{}, selection{}, first_visible{};
    std::uint64_t page{};
    bool initialized{}, interactive{};
    std::vector<WorldTaxRow> rows;
    std::optional<simulation::SteamInformationSkinPlan> skin;
};
struct WorldTaxLayout {
    Extent extent;
    Vector2 origin;
    Rectangle panel, rows, total;
    float row_height{};
};
struct WorldTaxInput {
    std::optional<Vector2> click, pointer;
    bool enter{}, escape{}, up{}, down{}, focus_lost{};
    int wheel_rows{};
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
};
bool world_tax_page(const simulation::rules::WorldScriptPage &page);
WorldTaxView world_tax_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page);
WorldTaxLayout world_tax_layout(Extent extent);
void prepare_world_tax(WorldTaxView &view, const simulation::StartupWorldRuntimeState &state,
                       const Skin &skin, bool scrolling = false);
std::optional<WorldFacilityPaintedControls>
world_tax_painted_controls(const WorldTaxView &view, const WorldTaxLayout &layout);
// One device frame reaches the source consumer; row release selects and confirms without
// a marker. Dragging uses source scroll requests and never acknowledges the report.
std::optional<simulation::StartupWorldTaxInput>
world_tax_input(const WorldTaxView &view, const WorldTaxLayout &layout,
                WorldFacilityGesture &gesture, const WorldTaxInput &input, bool blocked,
                const std::optional<WorldFacilityPaintedControls> &painted = {});
std::optional<WorldFacilityPaintedControls>
draw_world_tax(const WorldTaxView &view, const WorldTaxLayout &layout, const Skin &skin);
} // namespace ark::desktop::ui
