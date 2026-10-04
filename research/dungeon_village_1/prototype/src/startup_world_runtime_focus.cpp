#include "dungeon_village_prototype/startup_world_runtime.hpp"

#include "dungeon_village_reference/world_detached_actor.hpp"
#include "dungeon_village_reference/world_random_consumers.hpp"

#include <cmath>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
constexpr ref::CharacterId focus_id{std::numeric_limits<std::uint64_t>::max()};
bool install(State &s) {
    auto &world = s.scene.world.world;
    if (!(s.focus_actor.actor.id == focus_id) || s.focus_actor.actor.legacy_id != -1 ||
        s.focus_actor.actor.definition != 0 ||
        !world.ai.battle.actors.emplace(focus_id, s.focus_actor.actor).second ||
        !world.ai.contexts.emplace(focus_id, s.focus_actor.perception).second ||
        !world.actors.emplace(focus_id, s.focus_actor.routes).second ||
        !s.actor_metadata.emplace(focus_id, s.focus_actor.metadata).second)
        return false;
    return ref::valid_detached_human(world, focus_id);
}
bool extract(State &s) {
    auto &world = s.scene.world.world;
    if (!ref::valid_detached_human(world, focus_id) || !s.actor_metadata.count(focus_id))
        return false;
    s.focus_actor = {world.ai.battle.actors.at(focus_id), world.ai.contexts.at(focus_id),
                     world.actors.at(focus_id), s.actor_metadata.at(focus_id)};
    world.ai.battle.actors.erase(focus_id);
    world.ai.contexts.erase(focus_id);
    world.actors.erase(focus_id);
    s.actor_metadata.erase(focus_id);
    return true;
}
bool move(State &s) {
    constexpr std::uint32_t directions = 131072U | 524288U | 65536U | 262144U;
    if (s.focus_held_input & ~directions)
        return false;
    auto &a = s.scene.world.world.ai.battle.actors.at(focus_id);
    const bool down = s.focus_held_input & 131072U, up = s.focus_held_input & 524288U,
               left = s.focus_held_input & 65536U, right = s.focus_held_input & 262144U;
    ref::WorldPosition velocity;
    if (down && left)
        velocity = {-8.486563F, 8.486563F};
    else if (down && right)
        velocity = {8.486563F, 8.486563F};
    else if (up && left)
        velocity = {-8.486563F, -8.486563F};
    else if (up && right)
        velocity = {8.486563F, -8.486563F};
    else if (down)
        velocity.z = 12.F;
    else if (up)
        velocity.z = -12.F;
    else if (left)
        velocity.x = -12.F;
    else if (right)
        velocity.x = 12.F;
    a.position.x += static_cast<float>(velocity.x);
    a.position.z += static_cast<float>(velocity.z);
    a.vertical_velocity = 0.F;
    s.scene.world.world.actors.at(focus_id).horizontal_velocity = velocity;
    if (down)
        a.control.facing = 0;
    else if (up)
        a.control.facing = 2;
    else if (left)
        a.control.facing = 3;
    else if (right)
        a.control.facing = 1;
    return std::isfinite(a.position.x) && std::isfinite(a.position.z);
}
} // namespace

bool enter_startup_world_focus(State &s) {
    if (!std::isfinite(s.camera[0]) || !std::isfinite(s.camera[1]) ||
        s.camera[0] < static_cast<float>(std::numeric_limits<int>::min()) ||
        s.camera[0] >= static_cast<float>(std::numeric_limits<int>::max()) ||
        s.camera[1] < static_cast<float>(std::numeric_limits<int>::min()) ||
        s.camera[1] >= static_cast<float>(std::numeric_limits<int>::max()))
        return false;
    // c/a:116先截镜头为int，随后a(int[],vector)逆投影；只写n，不写s/t/u/au。
    const float x = static_cast<float>(static_cast<int>(s.camera[0]));
    const float y = static_cast<float>(static_cast<int>(s.camera[1]));
    const ref::CombatPoint position{(x - 2.F * y) * 100.F / 60.F, 0.F,
                                    (y + x * .5F) * 100.F / 30.F};
    if (!std::isfinite(position.x) || !std::isfinite(position.z))
        return false;
    s.focus_actor.actor.position = position;
    s.scene.scene_state = 2;
    s.scene.scene_counter = 0;
    return true;
}

std::optional<State> advance_startup_world_focus(const State &current,
                                                 const ref::WorldRuntimeAdapter<State> &adapter) {
    if (current.scene.scene_state != 2 || !current.rules)
        return {};
    auto s = current;
    if (!install(s) || !move(s))
        return {};
    const auto prefix = ref::prepare_world_execution_prefix(s.scene.world.world.ai, focus_id);
    if (!prefix.candidate)
        return {};
    s.scene.world.world.ai = prefix.candidate->state;
    if (!prefix.candidate->sounds.empty() || !prefix.candidate->growth_requests.empty()) {
        if (!adapter.prefix_effects)
            return {};
        auto next = adapter.prefix_effects(
            s, {focus_id, prefix.candidate->sounds, prefix.candidate->growth_requests});
        if (!next)
            return {};
        s = std::move(*next);
    }
    // W使用同一v FIFO，r新增8必须同次续行；未知领域命令显式拒绝。
    ref::WorldControlAdapter<State> control;
    control.read = [](const State &value, ref::CharacterId id) {
        const auto a = value.scene.world.world.ai.battle.actors.find(id);
        return a == value.scene.world.world.ai.battle.actors.end() ? nullptr : &a->second.control;
    };
    control.write = [](State &value, ref::CharacterId id, const ref::ActorControlState &c) {
        value.scene.world.world.ai.battle.actors.at(id).control = c;
        return true;
    };
    control.domain =
        [&adapter](const State &value,
                   ref::CharacterId id) -> std::optional<ref::WorldControlStep<State>> {
        const auto &queue = value.scene.world.world.ai.battle.actors.at(id).control.queue;
        if (queue.empty() || queue.front()[0] != 8 || !adapter.actors.owned_command)
            return {};
        auto next = value;
        const auto routes = startup_world_runtime_routes(next);
        auto input = adapter.actors.owned_command(next, routes, id, queue.front());
        if (!input || !input->departure)
            return {};
        input->departure->departure.draw = [&](int bound) -> std::optional<std::int64_t> {
            const auto draw = next.scene.random.draw(bound);
            return draw.error == ref::WorldRandomError::none
                       ? std::optional<std::int64_t>{draw.ticket}
                       : std::nullopt;
        };
        input->departure->expression_draw = [&](const ref::ActorEffectState &effects, int kind) {
            return ref::prepare_world_random_expression(next.scene.random, effects, kind, 0, true)
                .ticket;
        };
        const auto departure = ref::prepare_world_detached_departure_control(next.scene.world.world,
                                                                             *input->departure);
        if (!departure.candidate)
            return {};
        const bool event116 = !value.scene.world.world.ai.battle.events.count(116) &&
                              departure.candidate->state.ai.battle.events.count(116);
        next.scene.world.world = departure.candidate->state;
        if (event116) {
            if (!adapter.actors.event)
                return {};
            auto scripted = adapter.actors.event(next, 116);
            if (!scripted)
                return {};
            next = std::move(*scripted);
        }
        const auto action = departure.candidate->delete_instance
                                ? ref::WorldControlAction::delete_true
                            : departure.candidate->departure_succeeded
                                ? ref::WorldControlAction::hold_false
                                : ref::WorldControlAction::continue_same_call;
        return ref::WorldControlStep<State>{std::move(next), action};
    };
    const auto interpreted = ref::prepare_world_control(s, focus_id, control);
    if (!interpreted.candidate)
        return {};
    s = interpreted.candidate->state;
    // 原MainScene忽略W.d的true，仅该调用点跳尾；绝不删除W或实际名单成员。
    if (interpreted.candidate->flow != ref::WorldControlFlow::delete_requested) {
        if (!adapter.actors.projected_facing)
            return {};
        const auto facing = [&s, &adapter](const ref::BattleActorRecord &actor) {
            return adapter.actors.projected_facing(s, focus_id, actor);
        };
        const auto tail = ref::prepare_world_detached_actor_tail(
            s.scene.world.world, {focus_id, ref::world_schedule_facts(s.scene.world),
                                  startup_evidence().spawn_points, facing});
        if (!tail.candidate)
            return {};
        s.scene.world.world = tail.candidate->state;
        if (!tail.candidate->projected_actor)
            return {};
        s.actor_metadata.at(focus_id).cached_view =
            startup_world_raw_projection(tail.candidate->projected_actor->position);
    }
    if (!extract(s))
        return {};
    return s;
}
} // namespace dungeon_village_prototype
