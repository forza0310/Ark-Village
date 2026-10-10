// Common scene chrome and five-item menu rows; individual management pages own their layouts.
#include "layout.hpp"
#include <algorithm>
namespace ark::desktop::ui {
Layout::Layout(Extent size) : extent(size) {
    const float w = static_cast<float>(size.width), h = static_cast<float>(size.height);
    scene = {0, 24, w, h - 53};
    // Paint reaches the 21-unit footer; input stops above the taller 29-unit corner buttons.
    scene_clip = {0, 24, w, h - 45};
    left_button = {0, h - 29, 60, 29};
    right_button = {w - 60, h - 29, 60, 29};
    for (int i = 0; i < 5; ++i)
        menu_rows[i] = {2, 28.0F + i * 28, 89, 28};
}
} // namespace ark::desktop::ui
