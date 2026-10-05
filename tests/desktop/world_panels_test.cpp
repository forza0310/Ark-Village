// Layout contracts keep active text and confirmation inside the viewport at every supported size.
#include "support/checks.hpp"
#include "support/world_fixture.hpp"
#include "ui/world_panels.hpp"
#include "ui/world_reports.hpp"
#include <iostream>

namespace {
bool contains(Rectangle outer, Rectangle inner) {
    return inner.width > 0 && inner.height > 0 && inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
} // namespace
namespace ark::test {
void world_panels() {
    Checks check{"world_panels"};
    using namespace ark::desktop;
    using namespace ark::simulation::rules;
    for (const auto size :
         {Extent{240, 256}, Extent{360, 240 + 80}, Extent{540, 360}, Extent{1080, 720}}) {
        for (const auto kind :
             {WorldScriptPageKind::dialogue, WorldScriptPageKind::newspaper,
              WorldScriptPageKind::simple_message, WorldScriptPageKind::raw_page}) {
            WorldScriptPage page;
            page.kind = kind;
            page.legacy_page = kind == WorldScriptPageKind::dialogue ? 0 : 15;
            const auto layout = ui::world_page_layout(page, size);
            check(
                contains({0, 24, static_cast<float>(size.width), size.height - 53.F}, layout.panel),
                "Modal must clear the top HUD and lower playback buttons");
            check(contains(layout.panel, layout.body) && contains(layout.panel, layout.confirm),
                  "Body and confirmation must remain inside the modal");
            check(layout.body.height >= 34 && layout.body.width >= 100,
                  "Smallest viewport must retain readable scrollable text");
            check(layout.body.y + layout.body.height + 6 < layout.confirm.y,
                  "White text frame must not cover the confirmation button");
        }
    }
    for (const int raw : {99, 100}) {
        WorldScriptPage task;
        task.kind = WorldScriptPageKind::raw_page;
        task.legacy_page = raw;
        const auto layout = ui::world_page_layout(task, {240, 256});
        check(layout.body.y - layout.panel.y >= 95 && layout.body.height >= 34 &&
                  contains(layout.panel, layout.body),
              "Task monster display reserves its own area above readable source paragraphs");
    }
    WorldScriptPage page;
    page.kind = WorldScriptPageKind::dialogue;
    page.legacy_page = 0;
    const auto with_secretary = ui::world_page_layout(page, {540, 360});
    page.speaker_definition = 3;
    const auto other_speaker = ui::world_page_layout(page, {540, 360});
    check(other_speaker.body.width > with_secretary.body.width,
          "Unknown speakers must not reserve an invented secretary portrait");
    // UI owns only the footer transform and plain-text decoding. The source suite already
    // exhausts queue lifetime boundaries; these examples cover their product wiring.
    std::vector<WorldScriptNotice> notices(3);
    notices[0].message = 2;
    notices[0].counter = 8;
    notices[0].text = "<co=ff0000>先到</co><br>通知";
    notices[1].message = 3;
    notices[1].counter = 8;
    notices[1].text = "<po=cm>后到通知";
    notices[2].message = 4;
    notices[2].counter = 8;
    notices[2].text = "第三条仍在队列";
    const auto original = notices;
    for (const auto extent : {Extent{240, 256}, Extent{540, 360}}) {
        const auto lines = ui::world_notice_view(notices, extent);
        check(lines.size() == 2 && lines[0].index == 1 && lines[1].index == 0 &&
                  lines[0].text == "后到通知" && lines[1].text == "先到\n通知",
              "Notice wiring uses source reverse first-two order and decodes script tags");
        const float footer = extent.height - 42.F;
        check(lines[0].box.x == 0 && lines[1].box.x == 0 && lines[0].box.width == extent.width &&
                  lines[1].box.width == extent.width && lines[0].box.y == footer - 40 &&
                  lines[0].box.height == 19 && lines[1].box.y == footer - 21 &&
                  lines[1].box.height == 21 &&
                  lines[0].box.y + lines[0].box.height == lines[1].box.y &&
                  lines[1].box.y + lines[1].box.height == footer,
              "Notice heights and offsets translate to the desktop footer without rescaling source "
              "rows");
    }
    check(notices.size() == original.size() && notices[0].counter == original[0].counter &&
              notices[1].counter == original[1].counter &&
              notices[2].counter == original[2].counter && notices[0].text == original[0].text &&
              notices[1].text == original[1].text && notices[2].text == original[2].text,
          "Repeated notice projections neither advance counters nor mutate queued source text");
    notices[1].message = 1;
    const auto hidden = ui::world_notice_view(notices, {540, 360});
    check(hidden.size() == 1 && hidden[0].index == 0 && hidden[0].box.y == 297 &&
              hidden[0].box.height == 21,
          "Unshipped special notice1 stays hidden without promoting a third queue record");
    notices[0].counter = notices[1].counter = 0;
    check(ui::world_notice_view(notices, {540, 360}).empty(),
          "Unstarted first-two notices do not expose a later queued record");
    auto state = initial_world();
    const int human_id = state.rules->humans.front().identity;
    const int monster_id = state.rules->monsters.front().identity;
    state.report_state = 1;
    state.report_snapshot = {0, 18, 3165, 4000, -835, human_id};
    state.report_portraits = {monster_id, monster_id, monster_id, monster_id, monster_id};
    for (const int defeats : {0, 1, 4, 5}) {
        state.report_snapshot[0] = defeats;
        const auto view = ui::world_month_view(state);
        const auto expected = defeats > 4 ? 3U : static_cast<unsigned>(defeats);
        check(view.defeats == defeats && view.monsters.size() == expected &&
                  view.ellipsis == (defeats > 4) && view.human_image.has_value() == (defeats == 0),
              "Monthly 0/1/4/5 defeats preserve count, zero-human and overflow branches");
        for (const auto &portrait : view.monsters)
            check(portrait.definition == monster_id,
                  "Repeated monster portraits must not be deduplicated");
    }
    state.report_portraits[1] = -1;
    check(ui::world_month_view(state).monsters.size() == 1,
          "Monthly portrait -1 ends the sequence even when later slots contain definitions");
    state.report_state = 2;
    state.report_new_records[4] = true;
    const auto income = ui::world_month_view(state);
    check(income.income == 3165 && income.expenses == 4000 && income.balance == -835 &&
              income.record && income.monsters.empty(),
          "Income view consumes the prepared snapshot and signed total, independent of live funds");
    WorldScriptNotice growth;
    growth.message = 0;
    growth.counter = 8;
    growth.text = "人物 Lv.<co=0064ff>3</co>";
    growth.human_definition = human_id;
    growth.attribute_changes = std::array<std::array<int, 2>, 4>{{{3, 8}, {1, 8}, {0, 7}, {2, 6}}};
    state.scripts.notices = {growth};
    const auto before = state;
    const auto growth_view = ui::world_notice_view(state, {240, 256});
    check(growth_view.size() == 1 && growth_view[0].portrait_image &&
              growth_view[0].attributes == std::vector<std::array<int, 2>>{{3, 8}, {1, 8}},
          "Growth keeps source ap order and only its first half, including equal increments");
    for (const auto input : {std::pair<float, std::size_t>{40, 2}, {110, 1}, {180, 0}, {400, 0}}) {
        const auto layout = ui::world_notice_layout(growth_view[0], input.first);
        check(layout.attributes.size() == input.second && layout.end + 16 <= 240,
              "Narrow growth notices omit whole attribute groups when name/level consumes width");
    }
    const auto &person = state.rules->humans.front();
    const auto profession =
        state.scene.world.world.ai.growth.at(human_id).definition.current_profession;
    check(growth_view[0].portrait_image == state.rules->jobs.at(profession).sprites.at(person.sex),
          "Growth portrait reads current profession and sex");
    check(same_world_clock(state, before) && state.village_points == before.village_points &&
              state.scene.world.world.ai.accounting.funds() ==
                  before.scene.world.world.ai.accounting.funds() &&
              state.scripts.notices[0].counter == 8 &&
              state.scripts.notices[0].attribute_changes == growth.attribute_changes,
          "Report and growth projections do not consume points, fees, counters or ap");
    for (const Extent extent : {Extent{240, 256}, Extent{540, 360}}) {
        const auto layout = ui::world_victory_layout(extent);
        const Rectangle usable{0, 24, static_cast<float>(extent.width), extent.height - 51.F};
        check(contains(usable, layout.panel) && contains(usable, layout.confirm) &&
                  layout.confirm.y >= layout.panel.y + layout.panel.height &&
                  contains(usable, ui::world_month_confirm(extent)),
              "Report buttons stay clear of artwork and footer at narrow and wide sizes");
    }
    std::cout << "PASS world page layout contracts\n";
}
} // namespace ark::test
