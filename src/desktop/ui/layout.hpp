#pragma once

// Shared logical rectangles for painting and input; never infer hit areas from physical pixels.
#include "../projection.hpp"
#include <array>
namespace ark::desktop::ui {
struct Layout {
    explicit Layout(Extent extent);
    Extent extent;
    Rectangle scene, scene_clip, left_button, right_button, catalog, tabs[3], rows[4], menu_rows[5];
    Rectangle detail, dialogue, message, pause_button;
    Rectangle detail_previous, detail_next;
    std::array<Rectangle, 4> arrows(Vector2 anchor) const;
};
} // namespace ark::desktop::ui
