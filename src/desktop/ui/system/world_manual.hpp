#pragma once

// Read-only manual projection. Selection, cached text and decoration identity belong to Owner.
#include "../common/layout.hpp"
#include "ark/simulation/presentation/steam_manual_skin.hpp"
#include "ark/simulation/world/startup_world_manual.hpp"
#include <optional>

namespace ark::desktop::ui {
class Skin;
struct WorldManualView {
    std::optional<simulation::StartupManualPageView> source;
    bool interactive{};
};
struct WorldManualInput {
    std::optional<Vector2> click;
    bool confirm{}, right{}, left{}, cancel{};
};
// Last painted bounds belong to one page/index/viewport. Counter records the displayed frame;
// the controller clears this cache on world generation changes rather than owning an input clock.
struct WorldManualLayout {
    std::uint64_t page{};
    int index{}, counter{};
    Extent extent{};
    std::array<Rectangle, 2> arrows{};
    Rectangle back{};
};
bool world_manual_page(const simulation::rules::WorldScriptPage &page);
WorldManualView world_manual_view(const simulation::StartupWorldRuntimeState &state,
                                  const simulation::rules::WorldScriptPage &page);
std::optional<simulation::StartupManualInput>
world_manual_input(const WorldManualView &view, Extent extent,
                   const std::optional<WorldManualLayout> &painted, const WorldManualInput &input,
                   bool blocked);
WorldManualLayout draw_world_manual(const simulation::StartupWorldRuntimeState &state,
                                    const WorldManualView &view, Extent extent, const Skin &skin,
                                    bool enabled);
} // namespace ark::desktop::ui
