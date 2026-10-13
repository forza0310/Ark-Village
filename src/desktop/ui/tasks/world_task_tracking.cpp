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
    return v;
}
WorldTaskTrackingLayout world_task_tracking_layout(Extent e) {
    if (e.width < 240 || e.height < 256)
        throw std::invalid_argument("Unsupported task tracking viewport");
    return {{8, e.height - 95.F, e.width - 16.F, 60},
            {8, e.height - 56.F, 25, 18},
            {38, e.height - 56.F, 25, 18},
            {e.width - 124.F, e.height - 56.F, 55, 18},
            {e.width - 64.F, e.height - 56.F, 55, 18}};
}
std::optional<simulation::StartupTaskPageInput>
world_task_tracking_input(const WorldTaskTrackingView &v, const WorldTaskTrackingLayout &l,
                          const WorldTaskInput &i, bool blocked) {
    if (blocked || !v.ready)
        return {};
    simulation::StartupTaskPageInput f;
    f.left = i.left || world_facility_hit(i.click, l.previous);
    f.right = i.right || world_facility_hit(i.click, l.next);
    f.confirm = i.enter || world_facility_hit(i.click, l.confirm);
    f.cancel = i.escape || world_facility_hit(i.click, l.cancel);
    f.track = i.track;
    if (!f.left && !f.right && !f.confirm && !f.cancel && !f.track)
        return {};
    return f;
}
void draw_world_task_tracking(const WorldTaskTrackingView &v, const WorldTaskTrackingLayout &l,
                              const Skin &skin, bool enabled) {
    skin.content(l.strip);
    const float size =
        12.F * std::min(1.F, (l.strip.width - 10) / std::max(1.F, skin.text.width(v.name)));
    skin.text.draw(v.name, l.strip.x + 5, l.strip.y + 4, ink, size);
    skin.right(std::to_string(v.fee) + "G", l.strip.x + l.strip.width - 5, l.strip.y + 20,
               v.affordable ? blue : MAROON, 10);
    const bool active = enabled && v.ready;
    skin.choice(l.previous, "←", active);
    skin.choice(l.next, "→", active);
    skin.choice(l.confirm, "募集", active);
    skin.choice(l.cancel, "返回", active);
}
} // namespace ark::desktop::ui
