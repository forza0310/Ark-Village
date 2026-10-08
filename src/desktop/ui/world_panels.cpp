// S004/S005/S009 supply visual composition; STARTUP supplies page0 and its secretary binding.
// Responsive panel positioning is a desktop adaptation, not a fixed-APK touch-coordinate claim.
#include "world_panels.hpp"
#include "../world_rank.hpp"
#include "ark/simulation/rules/world_notices.hpp"
#include "ark/simulation/startup_world_visuals.hpp"
#include "script_text.hpp"
#include "skin.hpp"
#include "world_menu.hpp"
#include "world_reports.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop::ui {
bool world_hud_buttons_visible(const simulation::rules::WorldScriptPage *page) {
    return !page || (page->legacy_page != 21 && page->legacy_page != 74);
}
std::string world_page_body(const simulation::StartupWorldRuntimeState &s,
                            const simulation::rules::WorldScriptPage &page, int paragraph) {
    std::string body;
    if (!page.paragraphs.empty())
        body =
            page.paragraphs.at(std::min(paragraph, static_cast<int>(page.paragraphs.size()) - 1));
    if (page.task_definition &&
        (page.legacy_page == 30 || page.legacy_page == 31 || page.legacy_page == 32))
        body += s.rules->tasks.at(*page.task_definition).name + "完成!";
    if (page.legacy_page == 49)
        body = s.page_counters.count(page.id) ? world_rank_conditions(s) : "";
    if (page.legacy_page == 89 && page.monster_definition && body.empty()) {
        // The source binds a definition identity, not an actor or source-record index.
        const auto found = std::find_if(
            s.rules->monsters.begin(), s.rules->monsters.end(),
            [&](const auto &monster) { return monster.identity == *page.monster_definition; });
        if (found != s.rules->monsters.end())
            body = found->name;
    }
    return body;
}
void draw_world_hud(const simulation::StartupWorldRuntimeState &s, const Layout &layout,
                    const Skin &skin, bool failed, bool menu_open, bool menu_pending,
                    const simulation::rules::WorldScriptPage *page) {
    const float w = layout.extent.width, h = layout.extent.height;
    skin.tile("top_bar.png", {22, 0, 90, 24}, {22, 0, w - 150, 24});
    skin.sprites.image("top_bar.png", {0, 0, 22, 24}, {0, 0, 22, 24});
    skin.sprites.image("top_bar.png", {112, 0, 128, 24}, {w - 128, 0, 128, 24});
    const auto cash = std::to_string(s.scene.world.world.ai.accounting.funds()) + "G";
    draw_world_date(world_date_view(s.scene.calendar), skin, w - 15 - skin.text.width(cash));
    skin.right(cash, w - 7, 6);
    skin.sprites.image("townPointbar.png", {0, 0, 55, 15}, {w - 55, 24, 55, 15});
    skin.number(s.village_points, {w - 3, 27});
    skin.tile("btmbar.png", {116, 1, 4, 20}, {0, h - 21, w, 20});
    if (!page || page->legacy_page != 74)
        draw_world_popularity(s.popularity, layout, skin);
    // Catalogue/details own the footer; the same predicate rejects their hidden mouse buttons.
    if (!world_hud_buttons_visible(page))
        return;
    skin.button(layout.left_button, s.scene.framework_paused ? "继续" : "暂停", !failed);
    skin.button(world_menu_button(layout.extent), "菜单",
                !failed && !menu_pending && (menu_open || (s.scene.scene_state == 0 && !page)));
    if (failed)
        skin.centered("当前活动尚未接入", {8, 46, w - 16, 20}, MAROON);
    if (s.report_state && !page)
        draw_world_month(world_month_view(s), skin);
}
namespace {
using Page = simulation::rules::WorldScriptPage;
using Kind = simulation::rules::WorldScriptPageKind;
bool dialogue(const Page &page) { return page.kind == Kind::dialogue && page.legacy_page == 0; }
bool secretary(const Page &page) {
    return dialogue(page) && page.speaker_kind == 0 && page.speaker_definition == -1;
}
} // namespace

bool world_script_reward_page(const Page &page) {
    return page.kind == Kind::raw_page && page.legacy_page == 95;
}
WorldScriptRewardView world_script_reward_view(const simulation::StartupWorldRuntimeState &state,
                                               const Page &page) {
    if (!state.rules || !world_script_reward_page(page))
        throw std::invalid_argument("Script reward requires a bound source raw95 page");
    WorldScriptRewardView out;
    out.page = page.id;
    out.kind = page.legacy_r;
    const auto counter = state.page_counters.find(page.id);
    if (page.lifecycle == 4 || counter == state.page_counters.end())
        return out;
    out.counter = counter->second;
    out.ready_to_claim = out.counter >= 40;
    switch (page.legacy_r) {
    case 0:
        out.label = std::to_string(page.legacy_s) + "G";
        break;
    case 1:
        out.label = "村子点 " + std::to_string(page.legacy_s);
        break;
    case 3:
        out.label = state.rules->facilities.at(page.legacy_s).name;
        break;
    case 4:
        out.label = state.rules->jobs.at(page.legacy_s).name;
        break;
    case 9:
        out.label = "配置更替";
        break;
    case 10:
        out.label = "勋章";
        break;
    case 11:
        out.label = state.rules->activities.at(page.legacy_s).name;
        break;
    default:
        throw std::invalid_argument("Script reward kind is outside published raw95 payloads");
    }
    out.initialized = true;
    return out;
}
void draw_world_script_reward(const WorldScriptRewardView &view, const WorldPageLayout &layout,
                              const Skin &skin, bool enabled) {
    skin.window(layout.panel, "获得奖励");
    skin.content(layout.body);
    if (view.initialized) {
        const auto lines = wrap_plain_text(view.label, layout.body.width - 12,
                                           [&](const auto &text) { return skin.text.width(text); });
        for (std::size_t n = 0; n < lines.size() && (n + 1) * 16 <= layout.body.height; ++n)
            skin.text.draw(lines[n], layout.body.x + 6, layout.body.y + 6 + n * 16, blue);
    }
    // Early input only requests the source's jump to40; only a later qualifying input claims.
    skin.button(layout.confirm, view.ready_to_claim ? "领取" : "继续", enabled && view.initialized);
}

WorldDateView world_date_view(const simulation::rules::WorldCalendarState &calendar) {
    if (calendar.units < 0 || calendar.units >= 10800)
        throw std::invalid_argument("HUD requires the normalized source within-week clock");
    WorldDateView view;
    view.numbers = {std::to_string(static_cast<std::int64_t>(calendar.year) + 1),
                    std::to_string(calendar.month + 1), std::to_string(calendar.subperiod + 1)};
    view.text = view.numbers[0] + "年" + view.numbers[1] + "月" + view.numbers[2] + "周";
    view.week_progress = calendar.units / 10800.F;
    return view;
}
void draw_world_date(const WorldDateView &view, const Skin &skin, float right) {
    // Published common SEB8/number01 contains digits and frames10/11/12 for year/month/week.
    // The desktop shrinks the whole date only when necessary to preserve actual cash width.
    float width = 39;
    for (const auto &value : view.numbers)
        width += static_cast<float>(value.size()) * 8;
    const float scale = std::clamp((right - 29) / (width + 10), 0.F, 1.F);
    if (scale <= 0)
        return;
    float x = 29;
    for (int unit = 0; unit < 3; ++unit) {
        for (char digit : view.numbers[unit]) {
            skin.sprites.draw("number01.seb", digit - '0', {x, 8}, WHITE, Sprites::Binding::common,
                              scale);
            x += 8 * scale;
        }
        skin.sprites.draw("number01.seb", 10 + unit, {x, 8}, WHITE, Sprites::Binding::common,
                          scale);
        x += 13 * scale;
    }
    // Desktop progress geometry. Calendar units and denominator are source facts;
    // exact original bar crop, dimensions and fill direction have not been published.
    // The source top_bar.png remains unchanged, and drawing never advances the calendar.
    x += 3 * scale;
    DrawRectangleRec({x, 6, 5 * scale, 14}, ink);
    DrawRectangleRec({x + scale, 7, 3 * scale, 12}, {52, 72, 64, 255});
    const float height = view.week_progress * 12;
    if (height > 0)
        DrawRectangleRec({x + scale, 19 - height, 3 * scale, height}, {217, 242, 139, 255});
}

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
