#pragma once

// Read-only views of rank/award/residence presentations. Simulation counters and rewards are
// owned by the runtime; input sends one command and drawing never advances the source pages.
#include "../common/layout.hpp"
#include "../common/world_list_controls.hpp"
#include "ark/simulation/presentation/steam_human_skin.hpp"
#include "ark/simulation/presentation/steam_town_progress_skin.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldProgressionView {
    std::uint64_t page{};
    int raw{}, rank{}, counter{}, phase{}, selection{};
    bool initialized{}, confirm_enabled{};
    std::string title, human;
    std::vector<std::string> rows;
    std::optional<simulation::SteamTownProgressPlan> skin;
    std::optional<simulation::SteamHumanOverviewPlan> ceremony;
};
struct WorldProgressionLayout {
    Extent extent;
    Rectangle panel, body, cancel, confirm;
};
struct WorldProgressionInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{};
};
struct WorldProgressionIntent {
    bool rank_action{}, cancel{};
    int selection{}; // raw48: zero requests promotion, 1..4 requests a criterion explanation.
    std::optional<simulation::StartupTownRankInput> input{};
};
bool world_progression_page(const simulation::rules::WorldScriptPage &page);
WorldProgressionView world_progression_view(const simulation::StartupWorldRuntimeState &state,
                                            const simulation::rules::WorldScriptPage &page);
WorldProgressionLayout world_progression_layout(Extent extent, int raw = 48);
void prepare_world_progression(WorldProgressionView &view,
                               const simulation::StartupWorldRuntimeState &state, const Skin &skin);
std::optional<WorldProgressionIntent>
world_progression_input(const WorldProgressionView &view, const WorldProgressionLayout &layout,
                        const WorldProgressionInput &input, bool blocked,
                        const std::optional<WorldFacilityPaintedControls> &painted = {});
std::optional<WorldFacilityPaintedControls>
draw_world_progression(const WorldProgressionView &view, const WorldProgressionLayout &layout,
                       const Skin &skin, bool enabled);
} // namespace ark::desktop::ui
