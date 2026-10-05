#pragma once

// Read-only views of rank/award/residence presentations. Simulation counters and rewards are
// owned by the runtime; input sends one command and drawing never advances the source pages.
#include "ark/simulation/startup_world_runtime.hpp"
#include "layout.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldProgressionView {
    int raw{}, rank{}, counter{}, phase{};
    bool initialized{}, confirm_enabled{};
    std::string title, human;
    std::vector<std::string> rows;
};
struct WorldProgressionLayout {
    Rectangle panel, body, cancel, confirm;
};
struct WorldProgressionInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{};
};
struct WorldProgressionIntent {
    bool rank_action{}, cancel{};
    int selection{}; // raw48: zero requests promotion, 1..4 requests a criterion explanation.
};
bool world_progression_page(const simulation::rules::WorldScriptPage &page);
WorldProgressionView world_progression_view(const simulation::StartupWorldRuntimeState &state,
                                            const simulation::rules::WorldScriptPage &page);
WorldProgressionLayout world_progression_layout(Extent extent);
std::optional<WorldProgressionIntent>
world_progression_input(const WorldProgressionView &view, const WorldProgressionLayout &layout,
                        int &selection, const WorldProgressionInput &input, bool blocked);
void draw_world_progression(const WorldProgressionView &view, const WorldProgressionLayout &layout,
                            const Skin &skin, int selection, bool enabled);
} // namespace ark::desktop::ui
