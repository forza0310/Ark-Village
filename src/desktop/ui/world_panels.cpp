// S004/S005/S009 supply visual composition; STARTUP supplies page0 and its secretary binding.
// Responsive panel positioning is a desktop adaptation, not a fixed-APK touch-coordinate claim.
#include "world_panels.hpp"
#include "ark/simulation/rules/world_notices.hpp"
#include "ark/simulation/startup_world_visuals.hpp"
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

std::vector<WorldNoticeLine>
world_notice_view(const std::vector<simulation::rules::WorldScriptNotice> &notices, Extent extent) {
    const auto positions = simulation::rules::world_notice_placements(notices);
    if (!positions)
        throw std::invalid_argument("Invalid source notice queue");
    std::vector<WorldNoticeLine> lines;
    for (const auto &position : *positions) {
        const auto &notice = notices.at(position.index);
        if (notice.message == 1)
            continue; // Same unrendered special record as the published prototype.
        WorldNoticeLine line;
        line.index = position.index;
        // Clear the desktop HUD's 41px popularity fan without changing source row
        // offsets/lifetimes.
        line.box = {0, extent.height - 42.F + position.offset, static_cast<float>(extent.width),
                    static_cast<float>(position.height)};
        const auto decoded = decode_script_text(notice.text);
        line.text = decoded.text;
        line.standard_skin = notice.message != 32;
        line.runs = decoded.runs;
        lines.push_back(std::move(line));
    }
    return lines;
}
std::vector<WorldNoticeLine> world_notice_view(const simulation::StartupWorldRuntimeState &s,
                                               Extent extent) {
    auto lines = world_notice_view(s.scripts.notices, extent);
    for (auto &line : lines) {
        const auto &notice = s.scripts.notices.at(line.index);
        if (notice.message != 0)
            continue;
        if (!notice.human_definition || !notice.attribute_changes)
            throw std::invalid_argument("Growth notice has no source human/ap binding");
        const auto portrait = simulation::startup_world_portrait(s, *notice.human_definition);
        if (!portrait)
            throw std::invalid_argument("Growth notice has no current portrait");
        line.portrait_image = portrait->image;
        // ap is already ordered by the source's exchange sort. Do not sort ties or total levels.
        for (std::size_t i = 0; i < notice.attribute_changes->size() / 2; ++i) {
            const auto &attribute = notice.attribute_changes->at(i);
            if (attribute[1] <= 0)
                continue;
            if (attribute[0] < 0 || attribute[0] > 3)
                throw std::invalid_argument("Growth notice has an invalid attribute");
            line.attributes.push_back(attribute);
        }
    }
    return lines;
}
WorldNoticeLayout world_notice_layout(const WorldNoticeLine &line, float text_width) {
    WorldNoticeLayout result;
    result.end = std::min(line.box.width - 16, 32 + text_width + 5);
    constexpr int widths[]{17, 25, 24, 25};
    for (const auto &attribute : line.attributes) {
        const float width =
            widths[attribute[0]] + 8.F * (1 + std::to_string(attribute[1]).size()) + 4;
        if (result.end + width > line.box.width - 16)
            break;
        result.attributes.push_back({attribute[0], attribute[1], result.end});
        result.end += width;
    }
    return result;
}
void draw_world_notices(const std::vector<WorldNoticeLine> &lines, const Skin &skin) {
    for (const auto &line : lines) {
        if (!line.standard_skin) {
            // Special item-summary32 has a different, still unimplemented composition.
            DrawRectangleRec(line.box, {250, 254, 248, 255});
            skin.text.clipped(line.text, line.box.x + 4, line.box.y + 3, line.box, ink, 10);
            continue;
        }
        const float x = line.box.x, y = line.box.y;
        const auto layout = world_notice_layout(line, skin.text.width(line.text, 10));
        const float end = layout.end;
        // TASK_REPORT_RENDER: image77 left32, middle x32 up to108, final16. Logical row
        // heights remain 21/19; the 22px skin overlaps its neighbour as in the source atlas.
        skin.sprites.image("hisho_talk.png", {0, 0, 32, 22}, {x, y, 32, 22});
        skin.tile("hisho_talk.png", {32, 0, 108, 22}, {x + 32, y, end - 32, 22});
        skin.sprites.image("hisho_talk.png", {160, 0, 16, 22}, {x + end, y, 16, 22});
        if (line.portrait_image)
            skin.sprites.human_image(*line.portrait_image, {1, 27, 15, 14}, {x + 3, y + 3, 15, 14});
        else
            skin.sprites.image("chara_hishoko01.png", {0, 0, 16, 19}, {x + 2, y + 3, 16, 19});
        float cursor = x + 32;
        const Rectangle clip{cursor, y + 3, end - 32, 16};
        for (const auto &run : line.runs) {
            const auto rgb = run.rgb.value_or(0x403020);
            const Color color{static_cast<unsigned char>(rgb >> 16),
                              static_cast<unsigned char>(rgb >> 8), static_cast<unsigned char>(rgb),
                              255};
            skin.text.clipped(run.text, cursor, y + 6, clip, color, 10);
            cursor += skin.text.width(run.text, 10);
        }
        constexpr int widths[]{17, 25, 24, 25};
        for (const auto &attribute : layout.attributes) {
            cursor = x + attribute.x;
            skin.sprites.draw("icon_param00.seb", attribute.kind + 4, {cursor, y + 3}, WHITE,
                              Sprites::Binding::common);
            cursor += widths[attribute.kind];
            skin.sprites.draw("number08.seb", 14, {cursor, y + 6}, WHITE, Sprites::Binding::common);
            cursor += 8;
            const float width = 8.F * std::to_string(attribute.value).size();
            skin.number(attribute.value, {cursor + width, y + 6});
            cursor += width + 4;
        }
    }
}

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
