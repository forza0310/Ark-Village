#pragma once

// Pot pages share the existing five-row management geometry, never its buy/sell consumer.
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "world_commerce.hpp"

namespace ark::desktop::ui {
struct WorldMagicPotRow {
    int identity{};
    std::string name, value;
    bool known{true}, fresh{};
};
struct WorldMagicPotView {
    std::uint64_t page{};
    int raw{}, phase{}, selection{}, first_visible{}, counter{};
    bool initialized{}, can_confirm{}, can_cancel{};
    std::string title, heading, status, comment;
    std::vector<WorldMagicPotRow> rows;
    std::array<std::array<std::int32_t, 5>, 3> display{};
    std::array<std::int32_t, 4> costs{};
};
struct WorldMagicPotIntent {
    simulation::StartupMagicPotAction action;
    int selection{};
};
bool world_magic_pot_page(const simulation::rules::WorldScriptPage &page);
WorldMagicPotView world_magic_pot_view(const simulation::StartupWorldRuntimeState &state,
                                       const simulation::rules::WorldScriptPage &page);
std::optional<WorldMagicPotIntent> world_magic_pot_input(const WorldMagicPotView &view,
                                                         const WorldCommerceLayout &layout,
                                                         const WorldCommerceInput &input,
                                                         bool blocked);
void draw_world_magic_pot(const WorldMagicPotView &view, const WorldCommerceLayout &layout,
                          const Skin &skin, bool enabled, const std::string &feedback = {});
} // namespace ark::desktop::ui
