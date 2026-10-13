#include "world_startup.hpp"
#include "ark/simulation/presentation/startup_skin.hpp"
#include <algorithm>

namespace ark::desktop::ui {
namespace {
void image(const simulation::StartupSkinDraw &part, Vector2 origin, const Skin &skin) {
    const auto binding =
        part.package == simulation::StartupSkinPackage::event   ? Sprites::Binding::event
        : part.package == simulation::StartupSkinPackage::title ? Sprites::Binding::title
                                                                : Sprites::Binding::common;
    const Vector2 anchor{origin.x + part.offset[0], origin.y + part.offset[1]};
    if (part.sprite >= 0)
        skin.sprites.indexed_sprite(binding, part.sprite, part.frame, part.layer, part.image,
                                    anchor);
    else {
        const auto &c = part.crop;
        skin.sprites.indexed_image(binding, part.image,
                                   {float(c[0]), float(c[1]), float(c[2]), float(c[3])},
                                   {anchor.x, anchor.y, float(c[2]), float(c[3])});
    }
}
void fit(const Skin &skin, const std::string &value, Rectangle box, Color color = ink,
         float size = 11) {
    const float scale =
        std::min(1.F, (box.width - 4) / std::max(1.F, skin.text.width(value, size)));
    skin.centered(value, box, color, size * scale);
}
} // namespace
void draw_world_records(const simulation::StartupSystemRecords &records, int page, const Skin &skin,
                        Rectangle panel) {
    skin.window(panel, "排行榜 " + std::to_string(page + 1) + "/2");
    const Vector2 origin{panel.x - 9, panel.y - 34};
    image(*simulation::startup_skin_image(simulation::StartupSkinImage::record_background), origin,
          skin);
    fit(skin, page == 0 ? "最高通关点数" : "16年间的最高资金",
        {panel.x + 24, panel.y + 28, panel.width - 48, 20}, blue);
    fit(skin, page == 0 ? records.score_village : records.cash_village,
        {panel.x + 10, panel.y + 132, panel.width - 20, 20});
    const auto value = page == 0 ? records.high_score : records.cash_peak;
    fit(skin, std::to_string(value) + (page == 0 ? (value ? "P" : "") : "G"),
        {panel.x + 10, panel.y + 153, panel.width - 20, 20}, {200, 100, 60, 255});
    skin.sprites.draw("arrow02.seb", 3, {panel.x + 18, panel.y + 34}, WHITE,
                      Sprites::Binding::common);
    skin.sprites.draw("arrow02.seb", 0, {panel.x + 200, panel.y + 34}, WHITE,
                      Sprites::Binding::common);
    // Decorative human selection/animation has no complete published desktop mapping yet.
}
} // namespace ark::desktop::ui
