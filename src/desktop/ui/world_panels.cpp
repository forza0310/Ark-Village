// S004/S005/S009 supply visual composition; STARTUP supplies page0 and its secretary binding.
// Responsive panel positioning is a desktop adaptation, not a fixed-APK touch-coordinate claim.
#include "world_panels.hpp"
#include "script_text.hpp"
#include "skin.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
namespace {
using Page = simulation::rules::WorldScriptPage;
using Kind = simulation::rules::WorldScriptPageKind;
bool dialogue(const Page &page) { return page.kind == Kind::dialogue && page.legacy_page == 0; }
bool secretary(const Page &page) {
    return dialogue(page) && page.speaker_kind == 0 && page.speaker_definition == -1;
}
} // namespace

WorldPageLayout world_page_layout(const Page &page, Extent extent) {
    if (extent.width < 240 || extent.height < 256)
        throw std::invalid_argument("World page requires the supported logical viewport");
    const bool talk = dialogue(page);
    const bool news = page.kind == Kind::newspaper;
    const float width = talk ? 202.F : std::min(310.F, extent.width - 16.F);
    const float height = talk ? 122.F : std::min(240.F, extent.height - 68.F);
    WorldPageLayout result;
    result.panel = {(extent.width - width) / 2, (extent.height - height) / 2, width, height};
    const auto &box = result.panel;
    const float inset = secretary(page) ? 37.F : 15.F;
    // Task99/100 reserve the upper paper area for their actual monster display.
    const bool task_monster = page.legacy_page == 99 || page.legacy_page == 100;
    const float top = news ? 64.F : task_monster ? 95.F : 29.F;
    result.body = {box.x + inset, box.y + top, box.width - inset - 15, box.height - top - 39};
    result.confirm = {box.x + box.width - 68, box.y + box.height - 28, 58, 20};
    return result;
}

void draw_world_page_chrome(const Page &page, const WorldPageLayout &layout, const Skin &skin,
                            int paragraph_index) {
    const bool news = page.kind == Kind::newspaper;
    std::string title = decode_script_text(page.title).text;
    if (news) {
        // The frozen runtime page owns its actual paragraphs; the screenshot's 1/3 is not data.
        const auto total = std::max<std::size_t>(1, page.paragraphs.size());
        const auto current =
            std::min(static_cast<std::size_t>(std::max(0, paragraph_index)), total - 1);
        title = "大家的冒险通信 " + std::to_string(current + 1) + "/" + std::to_string(total);
    } else if (page.legacy_page == 49) {
        title = "村庄升级条件";
    } else if (title.empty() && page.legacy_page == 94) {
        title = "入手!";
    }
    skin.window(layout.panel, title);
    const auto &body = layout.body;
    skin.content({body.x - 6, body.y - 6, body.width + 12, body.height + 12});
    if (secretary(page)) {
        // SEB87 frame0 uses image126, with (-8,-27) sprite offsets. Keep the sprite beside the
        // body rather than painting the white content rectangle over its face.
        skin.sprites.draw("chara_hishoko01.seb", 0, {layout.panel.x + 18, layout.panel.y + 61},
                          WHITE, Sprites::Binding::secretary);
    }
    if (news) {
        const Rectangle banner{layout.panel.x + (layout.panel.width - 180) / 2, layout.panel.y + 24,
                               180, 26};
        skin.sprites.image("wnd_newsLogo.png", {0, 0, 180, 26}, banner);
        skin.centered(decode_script_text(page.title).text,
                      {banner.x + 27, banner.y + 3, banner.width - 54, banner.height - 6}, blue,
                      11);
    }
}

void draw_world_popularity(int popularity, const Layout &layout, const Skin &skin) {
    // btmbar_popular01 is an atlas: label x0..24, pink fill y0..3, dark track y5..9,
    // yellow fill y10..13. Rendering all 77x14 previously stacked every variant at once.
    const float x = layout.right_button.x - 77, bottom = layout.extent.height;
    skin.sprites.image("btmbar_popular00.png", {0, 0, 78, 41}, {x, bottom - 41, 78, 41});
    skin.sprites.image("btmbar_popular01.png", {0, 0, 25, 14}, {x + 10, bottom - 34, 25, 14});
    skin.sprites.image("btmbar_popular01.png", {25, 5, 52, 5}, {x + 12, bottom - 6, 52, 5});
    skin.number(popularity, {x + 74, bottom - 20});
}
} // namespace ark::desktop::ui
