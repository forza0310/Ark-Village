// Explicit display fixtures exercise source g/h and draw ordering, never advance a task.
#include "world_dungeon_visuals.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace ark::desktop;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
float fill_width(const std::vector<WorldDungeonLayer> &layers) {
    for (const auto &layer : layers)
        for (const auto &command : layer.plan)
            if (const auto *image = std::get_if<OverlayImage>(&command);
                image && image->name == "dungeon_bar.png" && image->source[1] == 10)
                return image->source[2];
    return 0;
}
const OverlaySprite *find_sprite(const std::vector<WorldDungeonLayer> &layers,
                                 const std::string &name, int frame) {
    for (const auto &layer : layers)
        for (const auto &command : layer.plan)
            if (const auto *sprite = std::get_if<OverlaySprite>(&command);
                sprite && sprite->name == name && sprite->frame == frame)
                return sprite;
    return nullptr;
}
} // namespace
int main() {
    namespace sim = ark::simulation;
    namespace rules = sim::rules;
    WorldDungeonView view;
    check(world_dungeon_visuals(view, 0).empty(), "No active task must produce no overlay");
    view.active = true;
    auto layers = world_dungeon_visuals(view, 0);
    check(layers.size() == 1 && layers[0].depth_offset == 0 &&
              find_sprite(layers, "cursor_rect02.seb", 1) && fill_width(layers) == 0,
          "An unbound task gets its source label, not a fabricated progress bar or diamond");
    view.has_facility = true;
    view.kind = 0;
    view.phase = 1;
    view.extent = 100;
    view.progress = 49;
    layers = world_dungeon_visuals(view, 60);
    check(layers.size() == 2 && layers[0].depth_offset == 0 && layers[1].depth_offset == 20 &&
              fill_width(layers) == 24,
          "Progress and marker must sort independently and retain integer truncation");
    const auto *marker = find_sprite(layers, "cursor_rect02.seb", 0);
    check(marker && marker->x == -2 && marker->y == 0,
          "Normal diamond's SEB offset is applied exactly once by the renderer");
    const auto *digit4 = find_sprite(layers, "number05.seb", 4);
    const auto *digit9 = find_sprite(layers, "number05.seb", 9);
    check(digit4 && digit9 && digit4->x < digit9->x,
          "Percent is mapped independently to 49, not reconstructed as 48 from the 24-pixel fill");
    const auto alternate = world_dungeon_visuals(view, 60, true);
    check(find_sprite(alternate, "cursor_rect02.seb", 1)->x ==
                  find_sprite(layers, "cursor_rect02.seb", 1)->x - 4 &&
              find_sprite(alternate, "menuRT01.seb", 0)->x ==
                  find_sprite(layers, "menuRT01.seb", 0)->x + 4,
          "Published language offsets must be explicit and opposite for label and percent");
    const auto tall = world_dungeon_visuals(view, 85);
    check(find_sprite(tall, "menuRT01.seb", 0)->y == find_sprite(layers, "menuRT01.seb", 0)->y - 25,
          "Progress geometry must use actual tenant PNG height rather than a diamond constant");
    for (const int g : {0, 50, 100, 150}) {
        view.progress = g;
        check(fill_width(world_dungeon_visuals(view, 60)) == std::min(g / 2, 50),
              "Progress fills must bound zero, half, completion and overflow");
    }
    view.extent = 0;
    check(fill_width(world_dungeon_visuals(view, 60)) == 0,
          "Zero extent must map to zero without dividing by zero");
    view.extent = 100;
    view.special_marker = true;
    view.progress = -1;
    layers = world_dungeon_visuals(view, 0);
    check(layers.size() == 1 && layers.front().depth_offset == 20 &&
              find_sprite(layers, "cursor_rect03.seb", 0),
          "Negative progress hides the bar but preserves the independent special target outline");
    for (const int phase : {0, 2}) {
        view.phase = phase;
        view.progress = 10;
        check(world_dungeon_visuals(view, 0).size() == 1,
              "Construction and completed phases must not display an active progress bar");
    }
    view.kind = 1;
    check(world_dungeon_visuals(view, 0).empty(),
          "Non-exploration tasks must not acquire an exploration target diamond");

    sim::StartupWorldRules catalogue;
    sim::StartupWorldRuntimeState state;
    state.rules = &catalogue;
    check(!world_dungeon_view(state).active, "Owner without active task must return an empty view");
    sim::StartupWorldTask task_source;
    task_source.factory.identity = 91;
    task_source.factory.kind = 0;
    catalogue.tasks.push_back(task_source);
    state.active_task = 501;
    state.tasks[501] = {501, 91, 0, 0, {}, rules::Position{1, 1}};
    const auto unbound = world_dungeon_view(state);
    check(unbound.active && !unbound.has_facility && unbound.site.x == 1 && unbound.site.y == 1,
          "Task identity, definition identity and its map site are distinct namespaces");
    auto &world = state.scene.world.world;
    world.map.width = world.map.height = 2;
    world.map.cells.resize(4);
    rules::RescueFacility facility;
    facility.placement.instance_id = {42};
    facility.placement.definition_id = 8;
    facility.placement.anchor = {1, 1};
    facility.status = 1;
    world.facilities[42] = facility;
    sim::StartupDefinition tenant;
    tenant.id = 8;
    tenant.flags = 524288;
    catalogue.facilities.push_back(tenant);
    state.tasks[501].facility = 42;
    state.dungeon_facilities[42].progress = 49;
    state.dungeon_facilities[42].extent = 100;
    state.surface.resize(4);
    const auto &display = sim::startup_evidence().displays.front();
    state.surface[3].display_definition = display.id;
    state.surface[3].variant = 1;
    const auto draws = state.scene.random.draws();
    const auto bound = world_dungeon_view(state);
    check(
        bound.has_facility && bound.phase == 1 && bound.progress == 49 && bound.extent == 100 &&
            bound.special_marker && bound.tenant_sprite == display.sprite &&
            bound.tenant_frame == 1,
        "Bound view must read canonical instance progress and first-cell current display binding");
    (void)world_dungeon_visuals(bound, 60);
    check(state.scene.random.draws() == draws && state.active_task == 501 &&
              state.dungeon_facilities.at(42).progress == 49,
          "Planning must not advance random, complete a task or change progress");
    std::cout << "PASS dungeon display owner and source geometry contracts\n";
}
