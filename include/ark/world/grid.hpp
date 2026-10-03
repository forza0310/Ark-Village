#pragma once

// Logical grid primitives. Screen projection belongs exclusively to the desktop adapter.
#include <cstddef>
#include <tuple>
#include <vector>

namespace ark::world {
// Original simulation coordinates: one grid cell spans 100 units on x/z. No screen pixels.
struct WorldPosition {
    float x{}, z{};
};
struct Cell {
    int x{};
    int y{};
    bool operator==(Cell other) const { return x == other.x && y == other.y; }
    bool operator!=(Cell other) const { return !(*this == other); }
    bool operator<(Cell other) const { return std::tie(y, x) < std::tie(other.y, other.x); }
};
struct Bounds {
    int min_x{}, max_x{}, min_y{}, max_y{};
    bool contains(Cell cell) const;
};
struct SourceCell {
    int display_id{}, variant{};
};
struct SourceMap {
    int width{}, height{};
    std::vector<SourceCell> cells;
    bool contains(Cell cell) const;
    // Reject invalid coordinates before converting signed coordinates to an array index.
    std::size_t index(Cell cell) const;
};
} // namespace ark::world
