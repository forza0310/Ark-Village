#pragma once

// Raw83 is source-owned navigation; this projection never buys, initializes or clears NEW.
#include "ark/simulation/presentation/steam_commerce_skin.hpp"
#include "world_commerce.hpp"

namespace ark::desktop::ui {
struct WorldCommerceTopView {
    std::uint64_t page{};
    bool initialized{}, interactive{};
    std::optional<simulation::SteamCommerceSkinInput> source;
    std::optional<simulation::SteamCommerceSkinPlan> skin;
};
// One displayed page/selection/viewport binds the actual three source Touch regions.
struct WorldCommerceTopControls {
    std::uint64_t page{};
    int selection{};
    Extent extent{};
    Vector2 origin{};
    Rectangle panel{}, cancel{};
    std::array<Rectangle, 3> rows{};
};
WorldCommerceTopView world_commerce_top_view(const simulation::StartupWorldRuntimeState &state,
                                             const simulation::rules::WorldScriptPage &page);
void prepare_world_commerce_top(WorldCommerceTopView &view, const Skin &skin);
WorldCommerceTopControls world_commerce_top_controls(const WorldCommerceTopView &view,
                                                     Extent extent);
std::optional<simulation::StartupCommerceTopInput>
world_commerce_top_input(const WorldCommerceTopView &view, Extent extent,
                         const std::optional<WorldCommerceTopControls> &painted,
                         const WorldCommerceInput &input, bool blocked);
std::optional<WorldCommerceTopControls> draw_world_commerce_top(const WorldCommerceTopView &view,
                                                                Extent extent, const Skin &skin,
                                                                bool enabled,
                                                                const std::string &feedback = {});
} // namespace ark::desktop::ui
