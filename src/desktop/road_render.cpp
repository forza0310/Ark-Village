// Adapted from research/prototype/road_render and STARTUP's road mark generation.
#include "road_render.hpp"
#include <stdexcept>
namespace ark::desktop {
std::optional<RoadPatch> road_patch(simulation::rules::Position cell, bool visible, bool quad,
                                    bool edge) {
    if (!visible)
        return {};
    if (quad)
        return RoadPatch{cell, "road4block00.png", 30, 20, 14, 21, -10};
    if (edge)
        return RoadPatch{cell, "road4block01.png", 27, 15, 20, 19, -10};
    return {};
}
} // namespace ark::desktop
