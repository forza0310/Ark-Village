#pragma once

// Page chrome reads the canonical page identity; it never confirms pages or advances a clock.
#include "ark/simulation/rules/world_scripts.hpp"
#include "layout.hpp"

namespace ark::desktop::ui {
class Skin;
struct WorldPageLayout {
    Rectangle panel;
    Rectangle body; // Text inset, shared by wrapping, scrolling and drawing.
    Rectangle confirm;
};
WorldPageLayout world_page_layout(const simulation::rules::WorldScriptPage &page, Extent extent);
void draw_world_page_chrome(const simulation::rules::WorldScriptPage &page,
                            const WorldPageLayout &layout, const Skin &skin, int paragraph_index);
// The maintained source has not published the meter's value-to-fill mapping. Draw only the
// certified artwork and actual number; do not turn popularity into an invented percentage.
void draw_world_popularity(int popularity, const Layout &layout, const Skin &skin);
struct WorldNoticeLine {
    std::size_t index{};
    Rectangle box;
    std::string text;
};
// Source queue order/height/lifetime, with the baseline moved to the current footer. These
// records never advance the queue; message1 and unshipped special artwork follow the prototype.
std::vector<WorldNoticeLine>
world_notice_view(const std::vector<simulation::rules::WorldScriptNotice> &notices, Extent extent);
void draw_world_notices(const std::vector<WorldNoticeLine> &lines, const Skin &skin);
} // namespace ark::desktop::ui
