#pragma once

#include "dungeon_village_reference/actor_control.hpp"

#include <functional>
#include <utility>

namespace dungeon_village_reference {

// v() 的三种真实出口：继续本次 FIFO、返回 false 跑 d 尾部、返回 true 跳过尾部。
enum class WorldControlAction { continue_same_call, hold_false, delete_true };
enum class WorldControlFlow { finished, held, delete_requested };
enum class WorldControlError {
    none,
    invalid_adapter,
    stale_actor,
    malformed_control,
    missing_consumer,
    consumer_failed,
    no_progress,
    budget_exhausted
};

template <class Owner> struct WorldControlStep {
    Owner state;
    WorldControlAction action{WorldControlAction::continue_same_call};
};

// 只操作聚合所有者的私有候选。领域回调不能闭包写入真实世界/随机源/显示队列。
// read/write 访问同一人物的唯一 control；不是独立 ActorControlState 的双向同步。
template <class Owner> struct WorldControlAdapter {
    std::function<const ActorControlState *(const Owner &, CharacterId)> read;
    std::function<bool(Owner &, CharacterId, const ActorControlState &)> write;
    std::function<std::optional<WorldControlStep<Owner>>(const Owner &, CharacterId)> domain;
};

template <class Owner> struct WorldControlCandidate {
    Owner state;
    WorldControlFlow flow{WorldControlFlow::finished};
    std::size_t local_commands{};
    std::size_t domain_segments{};
};
template <class Owner> struct WorldControlResult {
    WorldControlError error{WorldControlError::none};
    std::optional<WorldControlCandidate<Owner>> candidate;
};

namespace world_control_detail {
inline bool same_control(const ActorControlState &a, const ActorControlState &b) {
    return a.flags == b.flags && a.state == b.state && a.action == b.action &&
           a.action_counter == b.action_counter && a.alternate_counter == b.alternate_counter &&
           a.facing == b.facing && a.queue == b.queue;
}
inline bool valid_control(const ActorControlState &s) {
    if (s.state < 0 || s.state > 20 || s.action < 0 || s.action > 11 || s.action_counter < 0 ||
        s.alternate_counter < 0 || s.facing < 0 || s.facing > 3)
        return false;
    for (const auto &command : s.queue)
        if (!valid_actor_control(command))
            return false;
    return true;
}
} // namespace world_control_detail

// 仅组合 v：共同 d 前缀、携物表情和尾部由唯一世界调度者各调用一次。
// 0/2/8/10/12..19/21..30/32/33 留给当前领域消费者；不复用旧运动/出发结果。
// 正等待返回 held 后绝不再解释；设施退出/r 的分段返回应交 continue，而非 held。
// budget 是维护防循环保护，不是原作控制条数限制；耗尽拒绝整段，不延迟至下帧。
template <class Owner>
WorldControlResult<Owner> prepare_world_control(const Owner &state, CharacterId actor,
                                                const WorldControlAdapter<Owner> &adapter,
                                                std::size_t budget = 4096) {
    const auto failed = [](WorldControlError e) -> WorldControlResult<Owner> { return {e, {}}; };
    if (!adapter.read || !adapter.write)
        return failed(WorldControlError::invalid_adapter);
    WorldControlCandidate<Owner> c{state};
    std::size_t iterations{};
    for (;;) {
        const auto *current = adapter.read(c.state, actor);
        if (!current)
            return failed(WorldControlError::stale_actor);
        if (!world_control_detail::valid_control(*current))
            return failed(WorldControlError::malformed_control);
        if (current->queue.empty())
            return {WorldControlError::none, std::move(c)};
        if (iterations++ >= budget)
            return failed(WorldControlError::budget_exhausted);

        const auto local = prepare_local_control_prefix(*current);
        if (!local.candidate)
            return failed(WorldControlError::malformed_control);
        if (!adapter.write(c.state, actor, local.candidate->state))
            return failed(WorldControlError::stale_actor);
        c.local_commands += local.candidate->removed_commands;
        if (local.candidate->flow == ActorControlFlow::waiting ||
            local.candidate->flow == ActorControlFlow::moving ||
            local.candidate->flow == ActorControlFlow::departure_started) {
            c.flow = WorldControlFlow::held;
            return {WorldControlError::none, std::move(c)};
        }
        if (local.candidate->flow == ActorControlFlow::empty)
            return {WorldControlError::none, std::move(c)};
        if (!adapter.domain)
            return failed(WorldControlError::missing_consumer);

        // 捕获值仅用于进度校验，不作为人物控制权威，更不能恢复清队列前的旧尾部。
        const ActorControlState before = local.candidate->state;
        auto step = adapter.domain(c.state, actor);
        if (!step)
            return failed(WorldControlError::consumer_failed);
        c.state = std::move(step->state);
        ++c.domain_segments;
        if (step->action == WorldControlAction::delete_true) {
            c.flow = WorldControlFlow::delete_requested;
            return {WorldControlError::none, std::move(c)};
        }
        const auto *after = adapter.read(c.state, actor);
        if (!after)
            return failed(WorldControlError::stale_actor);
        if (!world_control_detail::valid_control(*after))
            return failed(WorldControlError::malformed_control);
        if (step->action == WorldControlAction::hold_false) {
            c.flow = WorldControlFlow::held;
            return {WorldControlError::none, std::move(c)};
        }
        if (world_control_detail::same_control(before, *after))
            return failed(WorldControlError::no_progress);
        // 同次续行重新读取当前队列，可见 c/r/退出替换的新队列，不使用进入时快照。
    }
}
} // namespace dungeon_village_reference
