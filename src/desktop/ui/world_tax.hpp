#pragma once

#include "ark/simulation/startup_world_tax.hpp"
#include "world_human.hpp"

namespace ark::desktop::ui {
struct WorldTaxRow {
    int identity{}, amount{}, portrait_image{};
    std::string name;
};
struct WorldTaxView {
    int raw{}, total{}, selection{}, first_visible{};
    std::uint64_t page{};
    bool initialized{};
    std::vector<WorldTaxRow> rows;
};
struct WorldTaxLayout {
    Rectangle panel, rows, total, confirm;
    float row_height{};
};
struct WorldTaxIntent {
    simulation::StartupWorldTaxAction action;
    std::uint64_t page{};
    int selection{};
};
bool world_tax_page(const simulation::rules::WorldScriptPage &page);
WorldTaxView world_tax_view(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page);
WorldTaxLayout world_tax_layout(Extent extent);
std::optional<WorldTaxIntent> world_tax_input(const WorldTaxView &view,
                                              const WorldTaxLayout &layout,
                                              const WorldHumanInput &input, bool blocked);
void draw_world_tax(const WorldTaxView &view, const WorldTaxLayout &layout, const Skin &skin,
                    bool enabled);
} // namespace ark::desktop::ui
