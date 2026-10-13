#pragma once
// Desktop geometry and device state are separate from the single pot Owner and its clocks.
#include "../common/world_list_input.hpp"
#include "ark/simulation/facilities/startup_world_magic_pot.hpp"
#include "ark/simulation/presentation/steam_magic_pot_effect_skin.hpp"

namespace ark::desktop::ui {
class Skin;
struct WorldMagicPotRow {
    int identity{};
    std::string name;
    bool known{true};
};
struct WorldMagicPotView {
    std::uint64_t page{};
    int raw{}, phase{}, selection{}, first_visible{}, counter{}, binding{-1};
    bool initialized{}, interactive{}, can_confirm{}, can_cancel{}, affordable{};
    std::string title, comment, reward_name;
    std::vector<WorldMagicPotRow> rows;
    std::optional<simulation::SteamMagicPotPlan> skin;
};
struct WorldMagicPotLayout {
    Extent extent;
    Rectangle panel, rows, cancel, confirm, feedback;
};
struct WorldMagicPotInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{};
    int wheel_rows{};
    bool keyboard_event{};
    std::optional<Vector2> pointer{};
    bool pointer_pressed{}, pointer_down{}, pointer_released{};
};
bool world_magic_pot_page(const simulation::rules::WorldScriptPage &page);
WorldMagicPotView world_magic_pot_view(const simulation::StartupWorldRuntimeState &state,
                                       const simulation::rules::WorldScriptPage &page);
WorldMagicPotLayout world_magic_pot_layout(Extent extent, int raw = 41);
void prepare_world_magic_pot(WorldMagicPotView &view,
                             const simulation::StartupWorldRuntimeState &state, const Skin &skin,
                             bool scrolling = false);
// All simultaneous logical inputs enter one transaction; only42 keeps a device row marker.
std::optional<simulation::StartupMagicPotInput>
world_magic_pot_input(const WorldMagicPotView &view, const WorldMagicPotLayout &layout,
                      const WorldMagicPotInput &input, bool blocked,
                      const std::optional<WorldFacilityPaintedControls> &painted = {},
                      WorldFacilityGesture *gesture = nullptr);
std::optional<WorldFacilityPaintedControls> draw_world_magic_pot(const WorldMagicPotView &view,
                                                                 const WorldMagicPotLayout &layout,
                                                                 const Skin &skin, bool enabled,
                                                                 const std::string &feedback = {});
} // namespace ark::desktop::ui
