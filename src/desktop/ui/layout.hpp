#pragma once

// Shared logical rectangles for painting and input; never infer hit areas from physical pixels.
#include "../projection.hpp"
#include <array>
namespace ark::desktop::ui {
struct Layout {
    explicit Layout(Extent extent);
    Extent extent;
    Rectangle scene, scene_clip, left_button, right_button, menu_rows[5];
};
} // namespace ark::desktop::ui
