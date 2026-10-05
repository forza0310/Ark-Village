// Layout contracts keep active text and confirmation inside the viewport at every supported size.
#include "ui/world_panels.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
bool contains(Rectangle outer, Rectangle inner) {
    return inner.width > 0 && inner.height > 0 && inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
} // namespace
int main() {
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
    std::cout << "PASS world page layout contracts\n";
}
