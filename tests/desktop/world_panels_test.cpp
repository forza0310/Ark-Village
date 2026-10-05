// Layout contracts keep active text and confirmation inside the viewport at every supported size.
#include "support/checks.hpp"
#include "ui/world_panels.hpp"
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
        const float footer = extent.height - 26.F;
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
    check(hidden.size() == 1 && hidden[0].index == 0 && hidden[0].box.y == 313 &&
              hidden[0].box.height == 21,
          "Unshipped special notice1 stays hidden without promoting a third queue record");
    notices[0].counter = notices[1].counter = 0;
    check(ui::world_notice_view(notices, {540, 360}).empty(),
          "Unstarted first-two notices do not expose a later queued record");
    std::cout << "PASS world page layout contracts\n";
}
} // namespace ark::test
