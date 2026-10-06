#pragma once

// Commerce keeps buy/sell direction, source scroll and the two payment/receipt moments separate.
#include "../world_build_placement.hpp"
#include "ark/simulation/startup_world_commerce.hpp"
#include "layout.hpp"
#include <optional>
#include <string>
#include <vector>

namespace ark::desktop::ui {
class Skin;
struct WorldCommerceRow {
    int identity{}, price{}, remaining{}, owned{};
    std::string name;
    bool fresh{};
    WorldBuildGraphic graphic;
};
struct WorldCommerceView {
    std::uint64_t page{};
    int raw{}, mode{}, tab{}, selection{}, first_visible{}, counter{}, feedback_counter{}, points{};
    std::int64_t funds{};
    // Ready to display: raw86 may project a committed receipt before its automatic initialization.
    bool initialized{}, can_confirm{}, can_cancel{}, can_inspect{};
    std::string title;
    std::vector<WorldCommerceRow> rows;
    std::optional<WorldCommerceRow> choice;
};
struct WorldCommerceLayout {
    Rectangle panel, heading, rows, status, feedback, cancel, confirm, inspect;
    std::array<Rectangle, 2> tabs;
    float row_height{};
};
struct WorldCommerceInput {
    std::optional<Vector2> click;
    bool enter{}, escape{}, up{}, down{}, left{}, right{}, inspect{};
    int wheel_rows{};
};
struct WorldCommerceIntent {
    simulation::StartupCommerceAction action;
    std::uint64_t page{};
    int selection{}; // Source list index; never a definition identity.
};
bool world_commerce_page(const simulation::rules::WorldScriptPage &page);
WorldCommerceView world_commerce_view(const simulation::StartupWorldRuntimeState &state,
                                      const simulation::rules::WorldScriptPage &page);
WorldCommerceLayout world_commerce_layout(Extent extent);
std::optional<WorldCommerceIntent> world_commerce_input(const WorldCommerceView &view,
                                                        const WorldCommerceLayout &layout,
                                                        const WorldCommerceInput &input,
                                                        bool blocked);
void draw_world_commerce(const WorldCommerceView &view, const WorldCommerceLayout &layout,
                         const Skin &skin, bool enabled, const std::string &feedback = {},
                         std::optional<std::int64_t> last_amount = {});
} // namespace ark::desktop::ui
