#include "ark/simulation/world/startup_world_runtime.hpp"
#include "support/audio_requests.hpp"
#include "ark/simulation/actors/rules/world_detached_actor.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace ark::simulation;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
void display_and_input() {
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> adapter;
    configure_startup_world_runtime_scene_adapter(adapter);
    StartupWorldRuntimeState s;
    s.visual_effects = {{0, 11, 0, 0, 0}, {0, 3, 0, 0, 0}, {20, 8, 0, 0, 0}};
    s.delayed_effects = {{4, 0, 10, 20}, {5, 1, 30, 40}};
    auto next = adapter.scene_other(s, {ref::WorldSceneStage::global_display, -1, {}});
    check(next.has_value(), "source display updates");
    check(next->state.visual_effects.size() == 2 && next->state.visual_effects[0][1] == 3,
          "forward removal skips shifted effect; actual cz9 expires");
    check(next->state.delayed_effects.size() == 1 && next->state.delayed_effects[0][1] == 0 &&
              test_support::audio_ids(next->state.sound_requests) == std::vector<int>{17} &&
              next->state.sound_requests.front().operation == StartupAudioOperation::ordinary_play,
          "reverse delayed source zero dispatch, positive only decrement");
    check(s.visual_effects[0][1] == 11 && s.delayed_effects.size() == 2,
          "candidate does not mutate input");
    s.scripts.notices = {{0, 0, 5, {}, {}}, {0, 22, 5, {}, {}}};
    next = adapter.scene_other(s, {ref::WorldSceneStage::common_display_tail, -1, {}});
    check(next && next->state.scripts.notices.size() == 1 &&
              test_support::audio_ids(next->state.sound_requests) == std::vector<int>{11} &&
              next->state.sound_requests.front().operation == StartupAudioOperation::ordinary_play,
          "q reverse expiry and first tick sound");
    s.scripts.notices = {{2, 0, 80, {}, "first"},
                         {3, std::numeric_limits<int>::max(), 80, {}, "invalid"}};
    check(!adapter.scene_other(s, {ref::WorldSceneStage::common_display_tail, -1, {}}) &&
              s.scripts.notices.front().counter == 0 && s.sound_requests.empty(),
          "late notice counter failure leaves sole queue and sound output unchanged");
    s.global_effects = {{0, 49}, {1, 49}};
    next = adapter.scene_other(s, {ref::WorldSceneStage::common_global_flag, -1, {}});
    check(next && next->state.global_effects == std::vector<std::array<int, 2>>{{1, 49}},
          "n.k only type0 ages and expires50");
    s.scene.scene_state = 3;
    s.scene.scene_counter = 9;
    s.confirm_input = true;
    next = adapter.scene_other(s, {ref::WorldSceneStage::wait_input, -1, {}});
    check(next && next->state.scene.scene_state == 3, "wait9 cannot confirm");
    s.scene.scene_counter = 10;
    next = adapter.scene_other(s, {ref::WorldSceneStage::wait_input, -1, {}});
    check(next && next->state.scene.scene_state == 0 && next->state.scene.scene_counter == 0,
          "wait10 a0 clears scene counter");
    check(!adapter.scene_other(s, {ref::WorldSceneStage::normal_input, -1, {}}),
          "nonempty normal command cannot silently succeed");
}
void camera() {
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> adapter;
    configure_startup_world_runtime_scene_adapter(adapter);
    StartupWorldRuntimeState s;
    s.scene.scene_state = 6;
    s.scripts.selected_actor = 1;
    // The maintained tracking consumer requires an actual human in the roster, not
    // metadata alone. Keep the original strict-distance and update-count oracle below.
    ref::BattleActorRecord tracked;
    tracked.id = ref::CharacterId{1};
    tracked.kind = ref::ActorKind::human;
    s.scene.world.world.ai.battle.actors.emplace(tracked.id, tracked);
    s.scene.world.world.ai.human_order.push_back(tracked.id);
    s.actor_metadata.emplace(ref::CharacterId{1}, StartupWorldActorMetadata{0, 0, 0, {0, 0}});
    s.camera = {5, 0};
    auto next = adapter.scene_other(s, {ref::WorldSceneStage::actor_camera_input, -1, {}});
    check(next && next->state.scene.scene_state == 6 && next->state.camera[0] == 0 &&
              next->state.global_updates == 1,
          "strict equal5 moves but remains6; global aK advances");
    next = adapter.scene_other(next->state, {ref::WorldSceneStage::actor_camera_input, -1, {}});
    check(next && next->state.scene.scene_state == 0 && next->state.global_updates == 2,
          "next zero distance aligns and switches0");
    s.scene.scene_state = 7;
    next = adapter.scene_other(s, {ref::WorldSceneStage::facility_camera_input, -1, {}});
    check(next && next->disposition == ref::WorldSceneDisposition::skip_round &&
              next->state.scene.scene_state == 0,
          "missing selected facility L159 not tail");
}
void build_mode_updates() {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream{});
    auto s = session.state();
    const auto adapter = startup_world_runtime_adapter();
    s.scene.scene_state = 1;
    s.scene.speed_setting = 1;
    s.build_mode = 7;
    s.build_feedback_counter = 2;
    s.build_feedback_message = "施工反馈夹具";
    const auto first = s.scene.world.facility_order.front();
    s.facility_details.at(first).notices = {{0, 43}, {7, 0}};
    // 显式未完成施工夹具；state1不能因处于建设模式就继续Tenant.c。
    s.scene.world.world.facilities.at(first).status = 0;
    s.dungeon_facilities.at(first).updates = 20;
    const auto result = ref::prepare_owned_world_runtime(s, {27, true}, adapter);
    check(result.state && result.scene && result.scene->scheduled_rounds == 1 &&
              result.scene->begun_rounds == 1 && result.worlds.empty(),
          "source build mode runs exactly one scene round even with speed setting1, no world AI");
    const auto &next = *result.state;
    check(next.build_feedback_counter == 1 && next.build_feedback_message == "施工反馈夹具" &&
              next.facility_details.at(first).notices == std::vector<std::array<int, 2>>{{7, 0}},
          "build ao.c decrements feedback and Tenant.d ages/removes only current notice front");
    check(next.dungeon_facilities.at(first).updates == 20 &&
              next.scene.world.world.facilities.at(first).status == 0 &&
              next.scene.calendar.units == s.scene.calendar.units &&
              next.scene.calendar.month_ticks == s.scene.calendar.month_ticks &&
              next.scene.world.updates == s.scene.world.updates &&
              next.scene.random.draws() == s.scene.random.draws(),
          "build mode freezes actual construction/actors/calendar/shared RNG, not just visual "
          "motion");
    const auto expired = adapter.scene_other(next, {ref::WorldSceneStage::build_update, 0, {}});
    check(
        expired && expired->state.build_feedback_counter == 0 &&
            expired->state.build_feedback_message == "移动去哪里" &&
            expired->state.facility_details.at(first).notices.front()[1] == 1,
        "feedback expiry restores original af7; shifted notice first advances on later call only");
    for (int kind = 0; kind < 8; ++kind) {
        auto edge = s;
        const int duration = kind == 0 ? 44 : kind == 7 ? 60 : 43;
        edge.facility_details.at(first).notices = {{kind, duration - 1}, {7, 0}};
        const auto aged = adapter.scene_other(edge, {ref::WorldSceneStage::build_update, 0, {}});
        check(aged && aged->state.facility_details.at(first).notices ==
                          std::vector<std::array<int, 2>>{{7, 0}},
              "all eight original notice kinds expire44/43/60 and leave shifted front untouched");
    }
    auto invalid = s;
    const auto last = invalid.scene.world.facility_order.back();
    invalid.facility_details.at(last).notices = {{8, 0}};
    check(!adapter.scene_other(invalid, {ref::WorldSceneStage::build_update, 0, {}}) &&
              invalid.build_feedback_counter == 2 &&
              invalid.facility_details.at(first).notices.front()[1] == 43,
          "late invalid tenant notice rejects entire build candidate including prior "
          "feedback/front changes");
    invalid = s;
    invalid.facility_details.at(first).notices = {{7, std::numeric_limits<int>::max()}};
    check(!adapter.scene_other(invalid, {ref::WorldSceneStage::build_update, 0, {}}),
          "build notice integer overflow rejects, no relaxed limits");
    invalid = s;
    invalid.confirm_input = true;
    check(!ref::prepare_owned_world_runtime(invalid, {27, true}, adapter).state &&
              invalid.facility_details.at(first).notices.front()[1] == 43,
          "unimplemented nonempty placement command rolls back already prepared build display "
          "update");
}
void facility_cameras() {
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> adapter;
    configure_startup_world_runtime_scene_adapter(adapter);
    for (int shape = 0; shape < 3; ++shape)
        for (int direction = 0; direction < 2; ++direction) {
            StartupWorldRuntimeState s;
            s.scene.scene_state = 7;
            s.scene.world.world.map = {8, 8, std::vector<ref::LegacyMapCell>(64)};
            ref::RescueFacility f;
            f.placement = {{1},
                           3,
                           static_cast<ref::FacilityShape>(shape),
                           static_cast<ref::FacilityOrientation>(direction),
                           {4, 4}};
            s.scene.world.world.facilities.emplace(1, f);
            s.scene.world.world.map =
                *ref::bind_facility_map(s.scene.world.world.map, {{f.placement, 3}}).map;
            s.scripts.selected_facility = 1;
            const auto occupied = ref::facility_footprint(
                f.placement.shape, f.placement.orientation, f.placement.anchor, 8, 8);
            const auto cell = occupied.cells.front().position;
            const float x = static_cast<float>((cell.x + cell.y) * 30 +
                                               (shape == 1 ? direction == 0 ? 15 : 45 : 30));
            const float y = static_cast<float>((cell.y - cell.x) * 15 + 15 -
                                               (shape == 0   ? 15
                                                : shape == 1 ? 22
                                                             : 30));
            s.camera = {x + 5, y};
            s.previous_camera = s.camera;
            auto next =
                adapter.scene_other(s, {ref::WorldSceneStage::facility_camera_input, 0, {}});
            check(next && next->state.camera == std::array<float, 2>{x, y} &&
                      next->state.previous_camera == std::array<float, 2>{x, y} &&
                      next->state.scene.scene_state == 7 && next->state.global_updates == 0,
                  "Tenant.f projects actual first occupied cell/shape/q; equal5 moves but stays7, "
                  "no aK");
            next = adapter.scene_other(next->state,
                                       {ref::WorldSceneStage::facility_camera_input, 0, {}});
            check(next && next->state.scene.scene_state == 0 &&
                      next->state.scene.scene_counter == 0,
                  "next strict zero-distance facility focus switches0 and resets scene counter");
            auto invalid = s;
            const auto index = cell.y * 8 + cell.x;
            invalid.scene.world.world.map.cells.at(index).facility.reset();
            check(!adapter.scene_other(invalid,
                                       {ref::WorldSceneStage::facility_camera_input, 0, {}}) &&
                      invalid.camera == s.camera,
                  "stale actual z0 binding rejects facility camera instead of guessing anchor");
        }
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream{});
    auto s = session.state();
    s.scene.scene_state = 7;
    const auto id = s.scene.world.facility_order.front();
    const auto &placement = s.scene.world.world.facilities.at(id).placement;
    const auto occupied =
        ref::facility_footprint(placement.shape, placement.orientation, placement.anchor, 24, 24);
    const auto cell = occupied.cells.front().position;
    check(placement.shape == ref::FacilityShape::single,
          "actual first original instance is single for exact focus-to-calendar integration");
    s.scripts.selected_facility = id;
    s.camera = {static_cast<float>((cell.x + cell.y) * 30 + 30),
                static_cast<float>((cell.y - cell.x) * 15)};
    const auto result =
        ref::prepare_owned_world_runtime(s, {27, true}, startup_world_runtime_adapter());
    check(result.state && result.scene && result.scene->scheduled_rounds == 1 &&
              result.worlds.empty() && result.state->scene.scene_state == 0 &&
              result.state->scene.calendar.units == s.scene.calendar.units + 27 &&
              result.state->scene.world.updates == s.scene.world.updates &&
              result.state->scene.random.draws() == s.scene.random.draws(),
          "real facility camera align7-to0 advances actual calendar tail without retroactive AI "
          "pass");
}
void detached_focus() {
    StartupSession initial;
    StartupWorldRuntimeSession session(initial.state(), ref::WorldRandomStream{});
    auto s = session.state();
    const auto adapter = startup_world_runtime_adapter();
    const auto id = s.focus_actor.actor.id;
    check(s.focus_actor.actor.legacy_id == -1 && s.focus_actor.actor.definition == 0 &&
              s.focus_actor.actor.control.flags == 2U && s.focus_actor.metadata.profession == 3 &&
              !s.scene.world.world.ai.battle.actors.count(id),
          "source W initialization stays detached, definition0 UID-1 ad3 flags2");
    s.camera = {120.9F, 30.9F};
    check(enter_startup_world_focus(s) && s.scene.scene_state == 2 &&
              s.focus_actor.actor.position.x == 100.F && s.focus_actor.actor.position.z == 300.F &&
              s.focus_actor.perception.cell == ref::Position{} &&
              s.focus_actor.metadata.cached_view == ref::Position{},
          "entry inverse truncates camera first and only writes W.n, not old s/u");
    s.focus_actor.metadata.cached_view = {120, 30};
    const auto roster = s.scene.world.world.ai.human_order;
    const auto metadata_count = s.actor_metadata.size();
    constexpr std::uint32_t held[]{131072U,          524288U,          65536U,
                                   262144U,          131072U | 65536U, 131072U | 262144U,
                                   524288U | 65536U, 524288U | 262144U};
    constexpr float dx[]{0.F, 0.F, -12.F, 12.F, -8.486563F, 8.486563F, -8.486563F, 8.486563F};
    constexpr float dz[]{12.F, -12.F, 0.F, 0.F, 8.486563F, 8.486563F, -8.486563F, -8.486563F};
    constexpr int facing[]{0, 2, 3, 1, 0, 0, 2, 2};
    for (int direction = 0; direction < 8; ++direction) {
        auto candidate = s;
        candidate.focus_held_input = held[direction];
        const auto moved = advance_startup_world_focus(candidate, adapter);
        check(moved &&
                  std::abs(moved->focus_actor.actor.position.x - (100.F + dx[direction])) < .001F &&
                  std::abs(moved->focus_actor.actor.position.z - (300.F + dz[direction])) < .001F &&
                  moved->focus_actor.actor.control.facing == facing[direction],
              "all eight source held directions use exact12/8.486563 and facing precedence");
    }
    auto moved = advance_startup_world_focus(s, adapter);
    check(
        moved && moved->focus_actor.actor.state_counter == 1 &&
            moved->focus_actor.actor.control.alternate_counter == 1 &&
            moved->focus_actor.actor.control.action_counter == 1 &&
            moved->scene.world.world.ai.human_order == roster &&
            moved->actor_metadata.size() == metadata_count &&
            !moved->scene.world.world.ai.battle.actors.count(id) &&
            !moved->scene.world.world.actors.count(id) &&
            !moved->scene.world.world.ai.contexts.count(id),
        "extra W.d advances prefix exactly once and removes temporary projection, no roster leak");
    auto growth = s;
    growth.scene.world.world.ai.growth.at(0).pending = {1000, 0};
    const auto old_level = growth.scene.world.world.ai.growth.at(0).definition.profession_levels;
    const auto grown = advance_startup_world_focus(growth, adapter);
    check(grown &&
              grown->scene.world.world.ai.growth.at(0).definition.profession_levels != old_level &&
              grown->scene.world.world.ai.growth.at(0).notice_pending &&
              !grown->scripts.notices.empty() &&
              grown->focus_actor.actor.capacity ==
                  grown->scene.world.world.ai.growth.at(0).derived.combat[0] &&
              grown->scene.world.world.ai.growth.at(0).pending.amount == 0,
          "W actual n().a advances shared definition0 growth and actual script/notice output once");
    auto returned_true = s;
    returned_true.focus_actor.actor.control.flags |= 32768U;
    returned_true.focus_actor.actor.control.queue = {{8, 5}};
    returned_true.focus_actor.perception.cell = {-1, -1};
    // 无效位置不是原delete_true；普通消费者必须拒绝，真实归属不受影响。
    check(!advance_startup_world_focus(returned_true, adapter) &&
              returned_true.focus_actor.actor.control.queue.size() == 1,
          "detached departure validation retains out-of-map rejection");
    returned_true = s;
    returned_true.focus_actor.actor.control.flags |= 32768U | 1024U;
    returned_true.focus_actor.actor.control.queue = {{8, 1}};
    for (auto &facility : returned_true.scene.world.world.facilities)
        facility.second.status = 0; // 显式无可用普通设施夹具；目录类型仍与实例严格一致。
    const auto retained = advance_startup_world_focus(returned_true, adapter);
    check(retained.has_value(), "source deleted W.d yields retained detached candidate");
    check(
        retained && retained->focus_actor.actor.control.queue.empty() &&
            retained->focus_actor.actor.state_counter == 1 &&
            retained->focus_actor.routes.bad_area_updates ==
                returned_true.focus_actor.routes.bad_area_updates &&
            !retained->scene.world.world.ai.battle.actors.count(id),
        "failed activity with old1024/32768 ignores source W.d true but skips tail and retains W");
    const auto frame = adapter.scene_other(s, {ref::WorldSceneStage::frame_view_sync, -1, {}});
    check(frame && frame->state.camera == std::array<float, 2>{120.F, 30.F},
          "frame view sync uses W.n before its source movement");
    auto unknown = s;
    unknown.focus_held_input = 1U;
    check(!advance_startup_world_focus(unknown, adapter) &&
              unknown.focus_actor.actor.state_counter == 0 &&
              unknown.scene.random.draws() == s.scene.random.draws(),
          "unknown held input rejects before prefix and preserves owner/random");
    unknown = s;
    unknown.focus_actor.actor.control.queue = {{14, 0}};
    check(!advance_startup_world_focus(unknown, adapter) &&
              unknown.focus_actor.actor.state_counter == 0,
          "unimplemented detached attack domain fails whole candidate after private prefix");
    auto late_failure = s;
    late_failure.focus_actor.actor.control.queue = {{8, 5}};
    late_failure.focus_actor.routes.bad_area_updates = std::numeric_limits<int>::max();
    late_failure.scene.random = ref::WorldRandomStream::from_raw({0, 0, 0, 0});
    check(!advance_startup_world_focus(late_failure, adapter) &&
              late_failure.scene.random.draws() == 0 &&
              late_failure.focus_actor.actor.control.queue ==
                  std::vector<ref::LegacyActorControl>{{8, 5}} &&
              late_failure.focus_actor.actor.state_counter == 0,
          "late tail rejection rolls back consumed activity5 random and whole W prefix/FIFO");
    auto projection = s.scene.world.world;
    projection.ai.battle.actors.emplace(id, s.focus_actor.actor);
    projection.ai.contexts.emplace(id, s.focus_actor.perception);
    projection.actors.emplace(id, s.focus_actor.routes);
    check(ref::valid_detached_human(projection, id) &&
              !ref::prepare_world_actor_cleanup(projection, id).candidate &&
              !ref::prepare_world_actor_tail(projection,
                                             {id, ref::world_schedule_facts(s.scene.world), {}})
                   .candidate &&
              ref::prepare_world_detached_actor_cleanup(projection, id).candidate.has_value(),
          "explicit detached cleanup accepted; ordinary cleanup/tail retain roster guard");
    projection.ai.human_order.push_back(id);
    check(!ref::valid_detached_human(projection, id) &&
              !ref::prepare_world_detached_actor_cleanup(projection, id).candidate,
          "detached guard refuses any ordinary roster alias");
    // 长运行保留实际aB0=false；30次坏区r排活动5，随后真实8消费者请求出口路线。
    auto running = s;
    for (int n = 0; n < 220; ++n) {
        const auto next = advance_startup_world_focus(running, adapter);
        check(next.has_value(),
              "detached W source220-call continuity through bad-area r and FIFO8");
        running = *next;
    }
    check(running.focus_actor.actor.state_counter > 0 &&
              !running.scene.world.world.ai.battle.actors.count(id) &&
              running.scene.world.world.ai.human_order == roster &&
              running.scene.random.draws() > s.scene.random.draws(),
          "W bad-area cleanup and real activity5 use one owner/random without joining bl");
    auto scene_frame = s;
    scene_frame.scene.top_is_main = true;
    scene_frame.scene.framework_paused = false;
    scene_frame.scene.speed_setting = 1;
    const auto focused = ref::prepare_owned_world_runtime(scene_frame, {27, true}, adapter);
    check(
        focused.state && focused.scene && focused.scene->scheduled_rounds == 1 &&
            focused.worlds.size() == 1 && focused.state->focus_actor.actor.state_counter == 1 &&
            focused.state->scene.world.updates == scene_frame.scene.world.updates + 1 &&
            focused.state->scene.calendar.units == scene_frame.scene.calendar.units &&
            focused.state->scene.calendar.month_ticks == scene_frame.scene.calendar.month_ticks &&
            focused.state->scene.world.world.ai.human_order == roster &&
            !focused.state->scene.world.world.ai.battle.actors.count(id),
        "source state2 integrates one common world plus one W.d even at speed1, no calendar tick");
    auto rejected_input = scene_frame;
    rejected_input.confirm_input = true;
    const auto rejected_frame =
        ref::prepare_owned_world_runtime(rejected_input, {27, true}, adapter);
    check(!rejected_frame.state && rejected_input.focus_actor.actor.state_counter == 0 &&
              rejected_input.scene.world.updates == scene_frame.scene.world.updates &&
              rejected_input.scene.random.draws() == scene_frame.scene.random.draws(),
          "late unimplemented focus confirmation rolls back common world, extra W.d and RNG");
    auto cancelled = s;
    cancelled.cancel_input = true;
    const auto cancel = adapter.scene_other(cancelled, {ref::WorldSceneStage::focus_input, 0, {}});
    check(cancel && cancel->state.scene.scene_state == 0 && !cancel->state.cancel_input,
          "focus cancel source a0 clears counter and one-shot input");
    cancelled.confirm_input = true;
    check(!adapter.scene_other(cancelled, {ref::WorldSceneStage::focus_input, 0, {}}),
          "focus attack confirm is explicit missing UI consumer, not silent success");
}
} // namespace
int main() {
    try {
        display_and_input();
        camera();
        build_mode_updates();
        facility_cameras();
        detached_focus();
        std::cout << "startup world scene checks: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
