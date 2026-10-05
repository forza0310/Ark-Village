// Product geometry must agree with source visibility; animation reads the canonical actor clock.
#include "support/world_fixture.hpp"
#include "world_rank.hpp"
#include "world_scene.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
namespace sim = ark::simulation;
namespace rules = sim::rules;
using namespace ark::desktop;
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
bool close(Vector2 a, Vector2 b) {
    return std::abs(a.x - b.x) < .001F && std::abs(a.y - b.y) < .001F;
}
void geometry() {
    auto state = ark::test::initial_world();
    state.camera = {{426, -72}};
    state.reference_viewport = {{0, 23, 240, 297}};
    for (const auto point : {rules::CombatPoint{0, 0, 0}, rules::CombatPoint{1050, 6, 750}}) {
        const auto source = sim::startup_world_view_projection(state, point);
        check(close(world_anchor(state, point, 1),
                    {static_cast<float>(source.x), static_cast<float>(source.y)}),
              "Reference canvas projection differs from source visibility");
    }
    const auto extent = canvas_extent(1080, 720);
    check(extent.width == 384 && extent.height == 256,
          "1080x720 logical canvas does not retain desktop aspect ratio");
    check(world_viewport(extent, 1) == std::array<int, 4>{{0, 24, 384, 211}},
          "Desktop canonical viewport does not match scene clipping");
    for (float zoom : {.5F, 1.F, 1.5F, 2.F}) {
        state.reference_viewport = world_viewport(extent, zoom);
        const rules::CombatPoint point{1050, 6, 750};
        const auto source = sim::startup_world_view_projection(state, point);
        const auto anchor = world_anchor(state, point, zoom);
        check(close(anchor, {source.x * zoom, source.y * zoom}),
              "Wide canvas drawing and source visibility use different transforms");
    }
    state.reference_viewport = world_viewport(extent, 1);
    float zoom = 1;
    const rules::CombatPoint point{1050, 6, 750};
    const auto anchor = world_anchor(state, point, zoom);
    const auto original_camera = state.camera;
    world_zoom_at(state, extent, anchor, 1, zoom);
    check(std::abs(zoom - 1.05F) < .001F && close(world_anchor(state, point, zoom), anchor),
          "Small-step zoom moved the actor underneath the pointer");
    world_zoom_at(state, extent, anchor, -1, zoom);
    check(std::abs(zoom - 1.F) < .001F &&
              close({state.camera[0], state.camera[1]}, {original_camera[0], original_camera[1]}),
          "Canonical camera drifted after zoom round trip");
    world_zoom_at(state, extent, anchor, 10000, zoom);
    check(zoom == 2 && close(world_anchor(state, point, zoom), anchor),
          "Upper zoom clamp moved pointer anchor");
    world_zoom_at(state, extent, anchor, -10000, zoom);
    check(zoom == .5F && close(world_anchor(state, point, zoom), anchor),
          "Lower zoom clamp moved pointer anchor");
}
void actor_animation() {
    auto state = ark::test::initial_world();
    state.reference_viewport = world_viewport(canvas_extent(1080, 720), 1);
    check(state.scripts.pages.size() == 1 &&
              state.scripts.pages.front().kind == rules::WorldScriptPageKind::scene,
          "New world does not begin with its canonical main scene");
    // Explicit test-user input closes source pages; it never creates or schedules an actor.
    // Each world step still passes through the complete source round and render-cache boundary.
    bool observed_introduction{};
    for (int frame = 0; frame < 1000 && state.scene.world.world.ai.human_order.empty(); ++frame) {
        auto result = sim::prepare_startup_world_runtime(state);
        check(result.candidate.has_value(), "Natural visitor round was rejected");
        check(sim::update_startup_world_render_cache(*result.candidate),
              "Natural visitor render-cache boundary was rejected");
        state = std::move(*result.candidate);
        const auto page = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                       [](const auto &entry) { return entry.lifecycle != 4; });
        check(page != state.scripts.pages.rend(), "Visitor test lost the active page");
        if (!observed_introduction && page->kind != rules::WorldScriptPageKind::scene) {
            check(page->kind == rules::WorldScriptPageKind::dialogue && page->source_record == 4 &&
                      rules::world_script_seen(state.scripts, 7),
                  "First world page no longer presents the actual event7 introduction");
            observed_introduction = true;
        }
        if (page->kind != rules::WorldScriptPageKind::scene && page->legacy_page != 16 &&
            page->legacy_page != 56 && page->legacy_page != 57) {
            const auto page_id = page->id;
            check(sim::acknowledge_startup_world_runtime_page(state, page_id) ==
                      sim::StartupWorldRuntimeError::none,
                  "Explicit visitor test-user confirmation was rejected");
        }
    }
    check(observed_introduction, "Visitor flow skipped its source introduction");
    check(!state.scene.world.world.ai.human_order.empty(), "Natural first visitor did not arrive");
    const auto id = state.scene.world.world.ai.human_order.front();
    auto &actor = state.scene.world.world.ai.battle.actors.at(id);
    const auto &meta = state.actor_metadata.at(id);
    actor.control.action = 0;
    actor.control.facing = 2;
    actor.control.flags = 2U;
    for (int tick : {0, 4, 8, 12, 16}) {
        actor.control.action_counter = tick;
        const auto pose = world_actor_pose(state, id);
        check(!pose.monster && pose.sprite == 2 && pose.frame == (tick / 4) % 4 &&
                  pose.image == state.rules->jobs.at(meta.profession).sprites.at(meta.sex),
              "Human walking no longer uses source four-phase clock and profession image");
    }
    actor.control.flags = 0;
    check(world_actor_pose(state, id).frame == 0,
          "Disabled source animation still advances from action counter");
    actor.control.flags = 2U;
    actor.control.action = 5;
    actor.control.action_counter = 7;
    check(world_actor_pose(state, id).sprite == 7 && world_actor_pose(state, id).frame == 1,
          "Human special action no longer replaces facing with counter phase");
    // Drawing interpolates two immutable samples without moving the simulation actor.
    actor.control.action = 0;
    actor.control.state = 0;
    actor.control.flags = 2;
    state.actor_metadata.at(id).render_position = {100, 0, 100};
    const auto previous = state;
    state.actor_metadata.at(id).render_position = {113.4F, 0, 100};
    const auto halfway = world_actor_render_position(state, id, &previous, .5F);
    check(std::abs(halfway.x - 106.7F) < .001F &&
              state.actor_metadata.at(id).render_position.x == 113.4F &&
              previous.actor_metadata.at(id).render_position.x == 100,
          "Raster interpolation mutated authoritative walking position");
    state.scene.framework_paused = true;
    check(world_actor_render_position(state, id, &previous, .2F).x == 113.4F,
          "Paused actor still interpolates");
    state.scene.framework_paused = false;
    actor.control.flags |= 1;
    check(world_actor_render_position(state, id, &previous, .2F).x == 113.4F,
          "Hidden actor interpolates across facility entry");
    actor.control.flags = 2;
    state.actor_metadata.at(id).render_position.x = 500;
    check(world_actor_render_position(state, id, &previous, .2F).x == 500,
          "Map teleport was turned into a walking path");
    // Explicit presentation fixture: reuse the single actor slot to cover monster body families.
    actor.kind = rules::ActorKind::monster;
    actor.definition = state.scene.world.world.ai.monster_growth.begin()->first;
    auto &definition = state.scene.world.world.ai.monster_growth.at(actor.definition);
    definition.body = 2;
    definition.sprite_variant = 3;
    actor.control.action = 3;
    actor.control.action_counter = 19;
    auto pose = world_actor_pose(state, id);
    check(pose.monster && pose.sprite == 10 && pose.image == 63 && pose.frame == 3,
          "Monster attack frame/image does not match source body family");
    actor.control.action_counter = 29;
    check(world_actor_pose(state, id).frame == 0, "Monster attack recovery lost idle frame");
    actor.control.action = 9;
    actor.control.action_counter = 19;
    check(world_actor_pose(state, id).frame == 0, "Monster action9 is no longer static");
}
void rank_conditions() {
    auto state = ark::test::initial_world();
    const auto original = state;
    state.rank_values = {-11, 22, 33, 44};
    state.rank_met = {true, false, true, false};
    // Independent source-table expectations cover every rank, including definition-ID terms.
    const std::array<std::array<std::string, 4>, 5> expected{{
        {{"人气 -11 / 300 [满足]", "最高月收入 22 / 5000 [未满足]", "活动举办次数 33 / 2 [满足]",
          ""}},
        {{"人气 -11 / 800 [满足]", "设施数量 22 / 10 [未满足]", "住宅数量 33 / 4 [满足]",
          "任务成功次数 44 / 12 [未满足]"}},
        {{"人气 -11 / 1500 [满足]", "最高月收入 22 / 35000 [未满足]", "活动举办次数 33 / 15 [满足]",
          ""}},
        {{"人气 -11 / 2500 [满足]", "设施数量 22 / 25 [未满足]", "住宅数量 33 / 10 [满足]",
          "任务成功次数 44 / 30 [未满足]"}},
        {{"人气 -11 / 3500 [满足]", "最高月收入 22 / 70000 [未满足]", "活动举办次数 33 / 30 [满足]",
          ""}},
    }};
    for (int rank = 0; rank < 5; ++rank) {
        state.rank = rank;
        auto rows = expected.at(rank);
        const int definition_id = rank == 0 ? 35 : rank == 2 ? 40 : 64;
        if (rank == 0 || rank == 2 || rank == 4) {
            const auto &definitions = state.rules->facilities;
            const auto definition = std::find_if(
                definitions.begin(), definitions.end(),
                [definition_id](const auto &entry) { return entry.id == definition_id; });
            check(definition != definitions.end(), "Source rank facility catalogue is incomplete");
            rows[3] = "建造" + definition->name + " [未满足]";
        }
        check(world_rank_conditions(state) ==
                  rows[0] + '\n' + rows[1] + '\n' + rows[2] + '\n' + rows[3],
              "Rank display recomputed cached values/status or changed original thresholds");
        check(state.rank == rank && state.rank_values == std::array<int, 4>{-11, 22, 33, 44} &&
                  state.rank_met == std::array<bool, 4>{true, false, true, false} &&
                  state.popularity == original.popularity &&
                  state.scene.world.world.ai.accounting.funds() ==
                      original.scene.world.world.ai.accounting.funds() &&
                  state.scene.random.draws() == original.scene.random.draws() &&
                  ark::test::same_world_clock(state, original) &&
                  state.scripts.pages.size() == original.scripts.pages.size() &&
                  state.scripts.pages.front().lifecycle ==
                      original.scripts.pages.front().lifecycle &&
                  state.scripts.user_flags == original.scripts.user_flags,
              "Read-only rank text mutated world, rank, page, cash, date or random state");
    }
    state.rank = 0;
    state.rank_values[3] = 0;
    const auto unmet = world_rank_conditions(state);
    state.rank_met[3] = true;
    check(world_rank_conditions(state) == unmet.substr(0, unmet.find_last_of('[')) + "[满足]",
          "Named facility condition did not follow cached boolean independently of numeric zero");
}
} // namespace
int main() {
    geometry();
    actor_animation();
    rank_conditions();
}
