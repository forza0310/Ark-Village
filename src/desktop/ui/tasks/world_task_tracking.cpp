#include "world_task_tracking.hpp"
#include "../common/skin.hpp"
#include <algorithm>
#include <stdexcept>
namespace ark::desktop::ui {
std::optional<WorldTaskTrackingView>
world_task_tracking_view(const simulation::StartupWorldRuntimeState &s) {
    if (s.scene.scene_state != 4 && s.scene.scene_state != 5)
        return {};
    if (!simulation::valid_startup_world_runtime_task_tracking_scene(s) || !s.rules)
        throw std::invalid_argument("Invalid task tracking scene payload");
    const auto p = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(), [](const auto &v) {
        return v.kind == simulation::rules::WorldScriptPageKind::scene;
    });
    WorldTaskTrackingView v;
    v.scene = p->id;
    v.selected = s.page_phases.at(p->id);
    const auto &list = s.task_page_lists.at(p->id);
    v.tasks = list;
    v.available_count = int(s.task_order.size());
    v.count = int(list.size());
    const auto &t = s.tasks.at(list.at(v.selected));
    const auto d = std::find_if(s.rules->tasks.begin(), s.rules->tasks.end(),
                                [&](const auto &a) { return a.factory.identity == t.definition; });
    if (d == s.rules->tasks.end())
        throw std::invalid_argument("Tracking lost task definition");
    v.name = d->name;
    v.fee = d->recruitment_fee;
    v.affordable = s.scene.world.world.ai.accounting.funds() >= v.fee;
    v.ready = s.scene.scene_state == 4 && !s.scene.framework_paused;
    v.initialized = s.scene.scene_state == 4;
    return v;
}
WorldTaskTrackingView world_task_information_view(const simulation::StartupWorldRuntimeState &s,
                                                  const simulation::rules::WorldScriptPage &p) {
    if (!s.rules || p.kind != simulation::rules::WorldScriptPageKind::raw_page ||
        p.legacy_page != 29 || !p.task_identity || !p.task_definition)
        throw std::invalid_argument("Information requires the source raw29 task binding");
    WorldTaskTrackingView v;
    v.scene = p.id;
    v.raw = 29;
    v.tasks = {*p.task_identity};
    v.ready = p.lifecycle == 2 && !s.scene.framework_paused;
    v.initialized = p.lifecycle == 2;
    return v;
}
void prepare_world_task_tracking(WorldTaskTrackingView &v,
                                 const simulation::StartupWorldRuntimeState &s, const Skin &skin) {
    v.skin.reset();
    if (!v.initialized)
        return;
    simulation::SteamTaskInfoOptions o;
    o.view_y = 23;
    o.bottom_width = int(skin.text.width(v.raw == 29 ? "了解" : "攻略"));
    // Source layout remains 240 logical units; the desktop translates it once when drawing.
    o.view_layout = std::array<int, 4>{0, 23, 240, 217};
    v.skin = v.raw == 29 ? simulation::steam_task_information_skin(s, v.scene, o)
                         : simulation::steam_task_information_scene_skin(s, o);
    if (!v.skin)
        throw std::invalid_argument("Invalid bound task information plan");
}
WorldTaskTrackingLayout world_task_tracking_layout(Extent e) {
    if (e.width < 240 || e.height < 256)
        throw std::invalid_argument("Unsupported task tracking viewport");
    return {e,
            {(e.width - 240.F) / 2, (e.height - 256.F) / 2},
            {e.width - 64.F, e.height - 30.F, 55, 18}};
}
std::optional<simulation::StartupTaskPageInput>
world_task_tracking_input(const WorldTaskTrackingView &v, const WorldTaskTrackingLayout &l,
                          const WorldTaskInput &i, bool blocked,
                          const std::optional<WorldTaskTrackingPaintedControls> &painted) {
    if (blocked || !v.ready)
        return {};
    simulation::StartupTaskPageInput f;
    const auto *p = painted ? &painted->geometry : nullptr;
    const bool fresh = p && p->page == v.scene && p->raw == v.raw && p->selection == v.selected &&
                       painted->tasks == v.tasks && painted->available_count == v.available_count &&
                       p->extent.width == l.viewport.width && p->extent.height == l.viewport.height;
    f.left = v.raw != 29 && i.left;
    f.right = v.raw != 29 && i.right;
    f.confirm = i.enter;
    f.cancel = i.escape;
    f.track = v.raw != 29 && i.track;
    if (fresh) {
        f.left |= v.raw != 29 && world_facility_hit(i.click, p->previous);
        f.right |= v.raw != 29 && world_facility_hit(i.click, p->next);
        f.confirm |= world_facility_hit(i.click, p->confirm);
        f.cancel |= world_facility_hit(i.click, p->cancel);
    }
    if (!f.left && !f.right && !f.confirm && !f.cancel && !f.track)
        return {};
    return f;
}
std::optional<WorldTaskTrackingPaintedControls>
draw_world_task_tracking(const WorldTaskTrackingView &v, const WorldTaskTrackingLayout &l,
                         const Skin &skin, bool enabled) {
    if (!v.initialized)
        return {}; // Scene5 camera movement has no dungeon-information helper.
    if (!v.skin)
        throw std::invalid_argument("Task information rendering requires measured source plan");
    WorldTaskTrackingPaintedControls out;
    out.geometry = draw_world_task_plan(*v.skin, l.origin, skin);
    auto &c = out.geometry;
    c.page = v.scene;
    c.raw = v.raw;
    c.selection = v.selected;
    c.extent = l.viewport;
    c.cancel = l.cancel;
    out.tasks = v.tasks;
    out.available_count = v.available_count;
    skin.choice(l.cancel, "返回", enabled);
    return out;
}
} // namespace ark::desktop::ui
