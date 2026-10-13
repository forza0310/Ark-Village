// Selection/recruitment consumes ordered research plans. The GPU path owns no task or clock.
#include "../common/steam_plan_draw.hpp"
#include "world_tasks.hpp"
#include <type_traits>

namespace ark::desktop::ui {
void prepare_world_task(WorldTaskView &v, const simulation::StartupWorldRuntimeState &s,
                        const Skin &skin, bool scrolling) {
    v.skin.reset();
    if (v.raw < 22 || v.raw > 24 || !v.initialized)
        return;
    simulation::SteamTaskSelectionOptions o;
    o.view_y = 23;
    o.scroll_first_touch = scrolling;
    const std::string title = v.raw == 22 ? "任务选择" : "寻求！冒险者";
    o.title_widths = std::array<int, 2>{int(skin.text.width(title)), int(skin.text.width(title))};
    o.bottom_width = int(skin.text.width(v.raw == 22 ? "请选择任务" : "募集冒险者"));
    constexpr const char *phrases[]{"交给我!",     "多指教",    "来了!",
                                    "我会加油的!", "我要参加!", "好期待"};
    for (int i = 0; i < 6; ++i)
        o.phrase_widths[i] = int(skin.text.width(phrases[i]));
    v.skin = simulation::steam_task_selection_skin(s, v.page, o);
    if (!v.skin)
        throw std::invalid_argument("Invalid bound task selection/recruitment plan");
}
std::optional<WorldTaskIntent>
world_task_selection_input(const WorldTaskView &v, const WorldTaskLayout &l,
                           WorldTaskSelection &selection, const WorldTaskInput &i, bool blocked,
                           const std::optional<WorldTaskPaintedControls> &painted) {
    auto &g = selection.gesture;
    if (g.page != v.page) {
        g = {};
        g.page = v.page;
    }
    const auto *p = painted ? &painted->geometry : nullptr;
    std::vector<std::uint64_t> ids;
    for (const auto &row : v.rows)
        if (row.task)
            ids.push_back(*row.task);
    const bool fresh = p && p->page == v.page && p->raw == v.raw && p->selection == v.selected &&
                       p->first == v.first && p->count == int(v.rows.size()) &&
                       painted->tasks == ids && p->extent.width == l.viewport.width &&
                       p->extent.height == l.viewport.height;
    const auto drag = world_facility_scroll_input(
        g, fresh ? p : nullptr, fresh && !blocked && v.initialized && v.interactive && v.raw == 22,
        i.pointer, i.pointer_pressed, i.pointer_down, i.pointer_released);
    if (blocked || !v.initialized || !v.interactive || v.raw == 24)
        return {};
    simulation::StartupTaskPageInput f;
    f.up = v.raw == 22 && i.up;
    f.down = v.raw == 22 && i.down;
    f.left = v.raw == 23 && i.left;
    f.right = v.raw == 23 && i.right;
    f.confirm = i.enter;
    f.cancel = i.escape;
    f.track = v.raw == 22 && i.track;
    if (v.raw == 22 && i.wheel_rows)
        f.scroll = simulation::StartupTaskPageInput::Scroll{false, i.wheel_rows};
    if (drag.absolute)
        f.scroll = simulation::StartupTaskPageInput::Scroll{true, *drag.absolute};
    if (fresh && !drag.consume_click) {
        f.confirm |= world_facility_hit(i.click, p->confirm);
        f.cancel |= world_facility_hit(i.click, p->cancel);
        f.track |= v.raw == 22 && world_facility_hit(i.click, p->inspect);
        for (const auto &row : p->rows)
            if (v.raw == 22 && row.index >= 0 && row.index < int(v.rows.size()) &&
                world_facility_hit(i.click, row.rectangle)) {
                f.selection = row.index;
                f.confirm = true;
                break;
            }
    }
    if (!f.up && !f.down && !f.left && !f.right && !f.confirm && !f.cancel && !f.track &&
        !f.selection && !f.scroll)
        return {};
    return WorldTaskIntent{f.cancel && !f.confirm ? simulation::StartupWorldTaskAction::cancel
                                                  : simulation::StartupWorldTaskAction::confirm,
                           f.selection.value_or(v.selected), f};
}
WorldFacilityPaintedControls draw_world_task_plan(const simulation::SteamTaskSelectionPlan &plan,
                                                  Vector2 origin, const Skin &skin) {
    SteamPlanDraw draw(origin, skin, plan.title);
    WorldFacilityPaintedControls c;
    for (const auto &op : plan.operations)
        std::visit(
            [&](const auto &p) {
                using T = std::decay_t<decltype(p)>;
                if constexpr (std::is_same_v<T, simulation::SteamStartupFrameDraw>)
                    draw.frame(p);
                else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionImage>) {
                    const auto r = simulation::steam_task_selection_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing task selection resource");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.published(r->published_image, r->published_sprite, p.position, p.frame,
                                       p.crop);
                } else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionText>)
                    draw.literal(p.value, p.rectangle, p.anchor, p.font_size, p.layout, p.rgb,
                                 p.rectangle[0] == 29   ? 102.F
                                 : p.rectangle[0] == 69 ? 150.F
                                                        : 180.F);
                else if constexpr (std::is_same_v<T, simulation::SteamTaskSelectionClip>)
                    draw.clip({p.push ? simulation::SteamFacilityClipKind::push_intersect
                                      : simulation::SteamFacilityClipKind::pop,
                               p.rectangle});
                else if constexpr (std::is_same_v<T, simulation::SteamHumanImage>) {
                    const auto r = simulation::steam_human_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing task decoration");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.human_image(p);
                } else if constexpr (std::is_same_v<T, simulation::SteamHumanPortrait>)
                    draw.portrait(p);
                else if constexpr (std::is_same_v<T, simulation::SteamFacilityImage>) {
                    const auto r = simulation::steam_facility_resource(p.asset);
                    if (!r)
                        throw std::invalid_argument("Missing recruitment label");
                    if (!r->published_sprite && !p.crop)
                        draw.whole_image(r->published_image, p.position);
                    else
                        draw.facility_image(p);
                } else if constexpr (std::is_same_v<T, simulation::SteamFacilityNumber>)
                    draw.number(p);
                else if constexpr (std::is_same_v<T, simulation::SteamHumanCatalogueScroll>) {
                    for (const auto &rect : p.rectangles)
                        draw.rectangle(rect);
                    const auto &r = *p.touch.rectangle;
                    c.scroll = Rectangle{origin.x + r[0] - 3, origin.y + r[1], float(r[2] + 3),
                                         float(r[3])};
                    c.scroll_visible = 5;
                } else if constexpr (std::is_same_v<T, simulation::SteamFacilityArrowTouch>) {
                    const auto arrow = steam_plan_arrow(p, skin);
                    const auto &crop = arrow.crop;
                    const auto &destination = arrow.destination;
                    skin.sprites.image(
                        "buttoneffect.png",
                        {float(crop[0]), float(crop[1]), float(crop[2]), float(crop[3])},
                        {origin.x + destination[0], origin.y + destination[1], destination[2],
                         destination[3]},
                        Sprites::Binding::steam_common);
                    const auto &r = arrow.touch_rectangle;
                    const Rectangle hit{origin.x + r[0], origin.y + r[1], float(r[2]), float(r[3])};
                    if (p.value == 16)
                        c.previous = hit;
                    else if (p.value == 18)
                        c.next = hit;
                    else
                        throw std::invalid_argument("Unsupported task arrow intent");
                } else if constexpr (std::is_same_v<T, simulation::SteamStartupTouch>) {
                    if (p.rectangle) {
                        const auto &r = *p.rectangle;
                        const Rectangle b{origin.x + r[0], origin.y + r[1], float(r[2]),
                                          float(r[3])};
                        if (p.component == 11) {
                            const int index = p.value & 0xffff;
                            c.rows.push_back({index, 0, b});
                        } else if (p.component == 4)
                            c.confirm = b;
                    }
                }
            },
            op);
    draw.finish();
    return c;
}
std::optional<WorldTaskPaintedControls> draw_world_task_selection(const WorldTaskView &v,
                                                                  const WorldTaskLayout &l,
                                                                  const Skin &skin, bool enabled,
                                                                  const std::string &feedback) {
    if (!v.initialized) {
        skin.window(l.panel, v.title);
        return {};
    }
    if (!v.skin)
        throw std::invalid_argument("Task rendering requires measured source plan");
    WorldTaskPaintedControls out;
    out.geometry = draw_world_task_plan(*v.skin, world_list_skin_origin(l.panel), skin);
    auto &c = out.geometry;
    c.page = v.page;
    c.raw = v.raw;
    c.selection = v.selected;
    c.first = v.first;
    c.count = int(v.rows.size());
    c.extent = l.viewport;
    for (const auto &row : v.rows)
        if (row.task)
            out.tasks.push_back(*row.task);
    if (v.raw != 24) {
        c.cancel = l.cancel;
        skin.choice(l.cancel, "返回", enabled);
    }
    if (v.raw == 22) {
        c.confirm = l.confirm;
        c.inspect = l.inspect;
        skin.choice(l.confirm, "选择", enabled);
        skin.choice(l.inspect, "追踪", enabled);
    } else if (v.raw == 24) {
        c.confirm = l.confirm;
        skin.choice(l.confirm, "加速", enabled);
    }
    if (!feedback.empty())
        skin.text.draw(feedback, l.cancel.x - 170, l.cancel.y + 22, MAROON, 9);
    return out;
}
} // namespace ark::desktop::ui
