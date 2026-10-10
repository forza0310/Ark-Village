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
std::optional<simulation::StartupTitleActorSkin>
world_configuration_actor(const app::WorldNewGameDraft &draft) {
    const auto &humans = simulation::startup_world_rules().humans;
    const auto main = std::find_if(humans.begin(), humans.end(),
                                   [](const auto &human) { return human.identity == 0; });
    if (main == humans.end())
        return {};
    // The published raw91 a.h.B identity is not yet bound. Preview the actual new-game
    // definition instead; step0 is deliberately static until its page counter is owned.
    return simulation::startup_title_actor_skin(main->definition.current_profession,
                                                draft.human.sex, main->equipment[0], 0, 2, false);
}
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
void draw_world_configuration(const app::WorldNewGameDraft &draft, int selected,
                              const std::array<Rectangle, 4> &fields, Rectangle panel,
                              const Skin &skin) {
    skin.window(panel, "重新开始");
    const Vector2 origin{panel.x - 9, panel.y - 34};
    image(*simulation::startup_skin_image(simulation::StartupSkinImage::new_game_background),
          origin, skin);
    const Rectangle scene{origin.x + 30, origin.y + 65, 180, 63};
    DrawRectangleLinesEx(scene, 1, {52, 133, 255, 255});
    constexpr const char *names[]{"城镇名称", "冒险者姓名", "性别"};
    for (int n = 0; n < 3; ++n)
        skin.text.draw(names[n], panel.x + 12, fields[n].y + 3, ink, 9);
    const std::array<std::string, 4> values{draft.village, draft.human.name,
                                            draft.human.sex == 0 ? "男" : "女", "开始游戏"};
    for (int n = 0; n < 4; ++n) {
        if (selected == n && n != 2) {
            DrawRectangleRec(fields[n], {255, 180, 104, 255});
            skin.sprites.draw("finger_r.seb", 0, {fields[n].x - 4, fields[n].y + 9}, WHITE,
                              Sprites::Binding::common);
        }
        if (n == 2) {
            // The source gender row keeps its text to the left of the actor. The existing
            // desktop input rectangle stays unchanged; these are only drawing anchors.
            fit(skin, values[n], {panel.x + 50, fields[n].y, 28, fields[n].height},
                draft.human.sex == 0 ? ink : Color{255, 14, 1, 255});
            skin.sprites.draw("arrow02.seb", 3, {origin.x + 118, origin.y + 185}, WHITE,
                              Sprites::Binding::common);
            skin.sprites.draw("arrow02.seb", 0, {origin.x + 210, origin.y + 185}, WHITE,
                              Sprites::Binding::common);
            if (selected == n)
                skin.sprites.draw("finger_r.seb", 0, {origin.x + 136, origin.y + 186}, WHITE,
                                  Sprites::Binding::common);
        } else
            fit(skin, values[n], fields[n]);
    }
    if (const auto actor = world_configuration_actor(draft)) {
        const Vector2 anchor{origin.x + 163, origin.y + 195};
        // Consume the maintained basic layer plan in order; human SEB and PNG IDs are
        // separate from common/weapon. This is not the full temporary-W effects renderer.
        for (const auto *layer : {&actor->shadow, &actor->weapon})
            if (*layer) {
                const auto &part = **layer;
                const auto binding = part.resource == simulation::StartupVisualResource::weapon
                                         ? Sprites::Binding::weapon
                                         : Sprites::Binding::common;
                skin.sprites.indexed_sprite(binding, part.sprite, part.frame, part.layer,
                                            part.image,
                                            {anchor.x + part.offset[0], anchor.y + part.offset[1]});
            }
        skin.sprites.actor(false, actor->body.sprite, actor->body.image, actor->body.frame, anchor);
    }
}
void draw_world_clear(const app::WorldClearPage &clear, Extent extent, const Skin &skin) {
    const Vector2 origin{(extent.width - 240) / 2.F, (extent.height - 256) / 2.F};
    const auto plan = simulation::startup_clear_skin(clear.score.stage, clear.score.counter,
                                                     clear.animation_counter);
    if (!plan)
        return;
    skin.window({origin.x + 9, origin.y + 58, 222, 155}, "通关点数发表");
    skin.content({origin.x + 17, origin.y + 81, 207, 96});
    image(plan->background, origin, skin);
    skin.content({origin.x + 21, origin.y + 85, 197, 52});
    const auto centered = [&](const std::string &text, float y, Color color = ink) {
        fit(skin, text, {origin.x + 25, origin.y + y, 190, 18}, color);
    };
    const int stage = clear.score.stage, count = clear.score.counter;
    const auto &row = clear.rows[std::min(clear.score.row, 5)];
    constexpr const char *names[]{"街道人气",         "任务完成数", "设施发现数",
                                  "冒险者全员LV合计", "自宅数",     "努力度总和"};
    if (stage == 0 || stage == 6) {
        if (count >= 15)
            centered(stage == 0 ? "那么赶紧开始计算" : "通关点数的计算", 100);
        if (count >= (stage == 0 ? 45 : 40))
            centered(stage == 0 ? "通关点数吧。" : "就是这样。", 122);
    } else if (stage == 1) {
        if (count >= 15)
            centered(names[std::min(clear.score.row, 5)], 100, blue);
        if (count >= 45)
            centered(std::to_string(row.count), 122);
    } else if (stage == 2 && count >= 15)
        centered("通关点数…", 112);
    else if (stage == 3 && count >= 15)
        centered(std::to_string(row.score) + " 点追加!!", 112, blue);
    else if (stage == 4)
        centered("排行榜更新！", 112, blue);
    else if (stage == 5 && count >= 15)
        centered("接下来…", 112, {200, 100, 60, 255});
    skin.right(std::to_string(clear.score.sum) + "点", origin.x + 213, origin.y + 73, blue, 11);
    skin.text.draw("最高纪录", origin.x + 27, origin.y + 187, ink, 10);
    skin.right(std::to_string(clear.score.captured_high_score) + "点", origin.x + 213,
               origin.y + 187, blue, 11);
    for (const auto &actor : plan->actors)
        image(actor, origin, skin);
    if (plan->continue_marker)
        image(*plan->continue_marker, origin, skin);
}
} // namespace ark::desktop::ui
