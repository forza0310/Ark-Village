// Published road refresh subset from research/rules/STARTUP.md; no renderer or inferred AI rules.
#include "ark/world/terrain.hpp"
#include <array>
#include <stdexcept>
namespace ark::world {
SourceMap connect_roads(SourceMap terrain, int road_display_id) {
    if (terrain.width <= 0 || terrain.height <= 0 || road_display_id < 0 ||
        terrain.cells.size() != static_cast<std::size_t>(terrain.width) * terrain.height)
        throw std::invalid_argument("Invalid road terrain");
    constexpr std::array<Cell, 4> directions{{{0, 1}, {1, 0}, {0, -1}, {-1, 0}}};
    constexpr std::array<int, 16> frames{{0, 13, 15, 3, 14, 11, 5, 7, 12, 9, 1, 10, 4, 8, 6, 2}};
    for (int y = 0; y < terrain.height; ++y)
        for (int x = 0; x < terrain.width; ++x) {
            auto &tile = terrain.cells[terrain.index({x, y})];
            if (tile.display_id != road_display_id)
                continue;
            unsigned mask{};
            for (std::size_t i = 0; i < directions.size(); ++i) {
                const Cell neighbour{x + directions[i].x, y + directions[i].y};
                // The original refresh treats the outside of the map as connected.
                if (!terrain.contains(neighbour) ||
                    terrain.cells[terrain.index(neighbour)].display_id == road_display_id)
                    mask |= 1U << i;
            }
            tile.variant = frames[mask];
        }
    return terrain;
}
} // namespace ark::world
