#pragma once

// Page chrome reads the canonical page identity; it never confirms pages or advances a clock.
#include "ark/simulation/rules/world_scripts.hpp"
#include "ark/simulation/startup_world_runtime.hpp"
#include "layout.hpp"
#include "script_text.hpp"

namespace ark::desktop::ui {
class Skin;
// One visibility rule governs both footer artwork and pointer admission. Keyboard pause is
// separate.
bool world_hud_buttons_visible(const simulation::rules::WorldScriptPage *page);
// Shared by input pagination and drawing; reading text never advances the source page.
std::string world_page_body(const simulation::StartupWorldRuntimeState &state,
                            const simulation::rules::WorldScriptPage &page, int paragraph);
// The caller supplies current modal visibility; this function owns artwork only.
void draw_world_hud(const simulation::StartupWorldRuntimeState &state, const Layout &layout,
                    const Skin &skin, bool failed, bool menu_open, bool menu_pending,
                    const simulation::rules::WorldScriptPage *page);
struct WorldScriptRewardView {
    std::uint64_t page{};
    int kind{}, counter{};
    bool initialized{}, ready_to_claim{};
    std::string label;
};
bool world_script_reward_page(const simulation::rules::WorldScriptPage &page);
WorldScriptRewardView world_script_reward_view(const simulation::StartupWorldRuntimeState &state,
                                               const simulation::rules::WorldScriptPage &page);
// STARTUP and the archived HUD identify the calendar subperiod as the displayed week.
// Read the canonical date; rendering never advances or normalizes its counters.
struct WorldDateView {
    std::string text;
    std::array<std::string, 3> numbers;
    float week_progress{}; // Canonical units / 10800, not monthly report ticks.
};
WorldDateView world_date_view(const simulation::rules::WorldCalendarState &calendar);
void draw_world_date(const WorldDateView &view, const Skin &skin, float right);
struct WorldPageLayout {
    Rectangle panel;
    Rectangle body; // Text inset, shared by wrapping, scrolling and drawing.
    Rectangle confirm;
};
void draw_world_script_reward(const WorldScriptRewardView &view, const WorldPageLayout &layout,
                              const Skin &skin, bool enabled);
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
