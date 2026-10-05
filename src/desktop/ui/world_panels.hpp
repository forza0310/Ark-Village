#pragma once

// Page chrome reads the canonical page identity; it never confirms pages or advances a clock.
#include "ark/simulation/rules/world_scripts.hpp"
#include "ark/simulation/startup_world_runtime.hpp"
#include "layout.hpp"
#include "script_text.hpp"

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
    bool standard_skin{true};
    std::vector<ScriptTextRun> runs;
    std::optional<int> portrait_image;
    std::vector<std::array<int, 2>> attributes;
};
struct WorldNoticeAttribute {
    int kind{}, value{};
    float x{};
};
struct WorldNoticeLayout {
    float end{}; // Start of the 16px right cap, relative to the row origin.
    std::vector<WorldNoticeAttribute> attributes;
};
WorldNoticeLayout world_notice_layout(const WorldNoticeLine &line, float text_width);
// Source queue order/height/lifetime, with the baseline moved to the current footer. These
// records never advance the queue; message1 and unshipped special artwork follow the prototype.
std::vector<WorldNoticeLine>
world_notice_view(const std::vector<simulation::rules::WorldScriptNotice> &notices, Extent extent);
std::vector<WorldNoticeLine> world_notice_view(const simulation::StartupWorldRuntimeState &state,
                                               Extent extent);
void draw_world_notices(const std::vector<WorldNoticeLine> &lines, const Skin &skin);
} // namespace ark::desktop::ui
