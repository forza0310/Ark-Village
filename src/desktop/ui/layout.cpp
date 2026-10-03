// S001 menu rows are 28 units. S002 catalog uses four picture/name/quote rows.
#include "layout.hpp"
#include <algorithm>
namespace ark::desktop::ui {
Layout::Layout(Extent size) : extent(size) {
    const float w = static_cast<float>(size.width), h = static_cast<float>(size.height);
    scene = {0, 24, w, h - 53};
    left_button = {0, h - 29, 60, 29};
    right_button = {w - 60, h - 29, 60, 29};
    catalog = {(w - 164) / 2, 29, 164, 198};
    for (int i = 0; i < 3; ++i)
        tabs[i] = {catalog.x + 3 + i * 53, catalog.y + 2, 53, 17};
    for (int i = 0; i < 4; ++i)
        rows[i] = {catalog.x + 3, catalog.y + 21 + i * 36, 158, 36};
    for (int i = 0; i < 5; ++i)
        menu_rows[i] = {2, 28.0F + i * 28, 89, 28};
    detail = {(w - 220) / 2, 37, 220, 182};
    detail_previous = {detail.x + 8, detail.y + 2, 12, 17};
    detail_next = {detail.x + detail.width - 20, detail.y + 2, 12, 17};
    // STARTUP page0 has a 202x122 dialogue, independently centered in the available viewport.
    dialogue = {(w - 202) / 2, (h - 122) / 2, 202, 122};
    message = {65, h - 27, std::min(170.0F, w - 130), 25};
}
std::array<Rectangle, 4> Layout::arrows(Vector2 p) const {
    return {{{p.x + 39, p.y - 40, 28, 16},
             {p.x + 39, p.y + 8, 28, 16},
             {p.x - 67, p.y + 8, 28, 16},
             {p.x - 67, p.y - 40, 28, 16}}};
}
} // namespace ark::desktop::ui
