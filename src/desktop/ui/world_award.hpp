#pragma once

// Read-only annual-page projection and hit testing. Award rules remain in the sole world owner.
#include "ark/simulation/startup_world_runtime.hpp"
#include "layout.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
bool world_page_automatic(const simulation::rules::WorldScriptPage &page);
bool world_page_regular_confirmation(const simulation::rules::WorldScriptPage &page);
struct WorldAwardRow {
    int definition{};
    std::string name;
    int contribution{};
};
struct WorldAwardView {
    bool initialized{};
    bool termination_pending{};
    int medals{};
    std::vector<WorldAwardRow> rows; // Preserve the source exchange-sort order, including ties.
};
struct WorldAwardLayout {
    Rectangle panel, rows, terminate, prompt, yes, no;
};
WorldAwardView world_award_view(const simulation::StartupWorldRuntimeState &state,
                                std::uint64_t page);
WorldAwardLayout world_award_layout(Extent extent, bool termination_pending);
// Return only explicit termination actions; there is no fallback to ordinary page confirmation
// and no invented raw88 award action. blocked combines pause, failure and a pending input serial.
std::optional<simulation::rules::WorldAwardAction>
world_award_input(const WorldAwardView &view, const WorldAwardLayout &layout,
                  std::optional<Vector2> click, bool enter, bool escape, bool blocked);
void draw_world_award(const WorldAwardView &view, const WorldAwardLayout &layout, const Skin &skin,
                      int first_row, bool enabled);
} // namespace ark::desktop::ui
