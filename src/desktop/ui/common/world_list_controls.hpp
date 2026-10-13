#pragma once
#include "layout.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace ark::desktop::ui {
// Last-painted geometry and source tags only. Selection and page clocks remain in the Owner.
struct WorldFacilityPaintedControls {
    std::uint64_t page{};
    int raw{}, phase{}, selection{}, first{}, count{}, choice{-1};
    Extent extent;
    struct Row {
        int index{}, identity{};
        Rectangle rectangle;
    };
    std::vector<Row> rows;
    Rectangle cancel{}, confirm{}, inspect{}, previous{}, next{};
    std::optional<Rectangle> scroll;
    int scroll_visible{};
};
inline Vector2 world_list_skin_origin(Rectangle panel) {
    return {panel.x - (120 - int(panel.width) / 2), panel.y - (131 - int(panel.height) / 2)};
}
} // namespace ark::desktop::ui
