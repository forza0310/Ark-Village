// Independent adjacency fixtures cover every original road frame and boundary behaviour.
#include "ark/world/terrain.hpp"
#include <array>
#include <stdexcept>
namespace {
void require(bool ok) {
    if (!ok)
        throw std::runtime_error("Road connection contract failed");
}
} // namespace
int main() {
    using namespace ark::world;
    const std::array<int, 16> expected{{0, 13, 15, 3, 14, 11, 5, 7, 12, 9, 1, 10, 4, 8, 6, 2}};
    const std::array<Cell, 4> neighbours{{{1, 2}, {2, 1}, {1, 0}, {0, 1}}};
    for (unsigned mask = 0; mask < 16; ++mask) {
        SourceMap map{3, 3, std::vector<SourceCell>(9, {27, 0})};
        map.cells[map.index({1, 1})] = {37, 0};
        for (unsigned bit = 0; bit < 4; ++bit)
            if (mask & (1U << bit))
                map.cells[map.index(neighbours[bit])] = {37, 0};
        const auto connected = connect_roads(map, 37);
        require(connected.cells[connected.index({1, 1})].variant == expected[mask]);
        require(map.cells[map.index({1, 1})].variant == 0); // Derived copy leaves source intact.
        require(connected.cells[0].display_id == 27 && connected.cells[0].variant == 0);
    }
    SourceMap strip{3, 1, std::vector<SourceCell>(3, {37, 0})};
    const auto all = connect_roads(strip, 37);
    for (const auto &cell : all.cells)
        require(cell.variant == 2); // All four directions, including out-of-map connections.
    strip.cells[1] = {27, 0};       // A building replaces the central road in the visual input.
    const auto broken = connect_roads(strip, 37);
    require(broken.cells[0].variant == expected[13] && broken.cells[2].variant == expected[7]);
    require(connect_roads(broken, 37).cells[0].variant == broken.cells[0].variant);
    bool rejected{};
    try {
        connect_roads({3, 1, {}}, 37);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    require(rejected);
}
