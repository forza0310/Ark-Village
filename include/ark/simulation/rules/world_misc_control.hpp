#pragma once

#include "ark/simulation/rules/rescue_commit.hpp"
#include "ark/simulation/rules/world_control.hpp"

namespace ark::simulation::rules {
// 仅增加现有 AI 聚合尚未拥有的定义 m。人物 HP/control/速度仍以 world 为唯一权威。
// m 影响后续到访，不能把26只实现成实例删除或把它冒充实例状态 A。
struct WorldMiscControlState {
    RescueWorldState world;
    std::map<int, int> human_definition_state; // a.e.m，共享定义ID，不是人物实例ID。
};
struct WorldMiscSoundRequest {
    CharacterId actor;
    int sound{8};
    Position position; // 本次 a.a(旧u,bm) 的实际结果，不猜测格坐标或新n投影。
};
using MiscSoundProjection = std::function<std::optional<Position>(Position)>;
struct WorldMiscControlInput {
    CharacterId actor;
    std::optional<Position> cached_view;  // 当前缓存 u；25和33必需，由表现适配显式输入。
    MiscSoundProjection sound_projection; // 只在33调用，用当前镜头把旧u映射到bm。
};
enum class WorldMiscControlError {
    none,
    invalid_input,
    stale_actor,
    missing_definition,
    missing_projection,
    preparation_failed
};
struct WorldMiscControlCandidate {
    WorldMiscControlState state;
    WorldControlAction action{WorldControlAction::continue_same_call};
    std::vector<WorldMiscSoundRequest> sounds{};
};
struct WorldMiscControlResult {
    WorldMiscControlError error{WorldMiscControlError::none};
    std::optional<WorldMiscControlCandidate> candidate;
};
// 只执行当前25/26/32/33。25真实HP及两个cd记录，26共享m1后true删除请求，
// 32只写aN=28/6竖速，33只发投影音效8。没有共同计数/HP动画/物理/实例删除。
// 所有准备在私有副本内完成；投影回调必须纯读取，不能自行播放音效或写外部状态。
WorldMiscControlResult prepare_world_misc_control(const WorldMiscControlState &state,
                                                  const WorldMiscControlInput &input);

struct WorldStateCommandInput {
    CharacterId actor;
    std::optional<int> boost_ticket; // c18真实需要时才消费一次[0,100)，不替外层抽号。
};
struct WorldStateTransitionInput {
    CharacterId actor;
    int next_state{};
    std::optional<int> boost_ticket;
};
struct WorldStateCommandCandidate {
    RescueWorldState state;
    bool consumed_boost_ticket{};
    bool cleaned_up{};
    std::vector<int> event_requests{}; // 新触发116；实际脚本/窗口留给有序消费者。
};
struct WorldStateCommandResult {
    WorldMiscControlError error{WorldMiscControlError::none};
    std::optional<WorldStateCommandCandidate> candidate;
};
// 真实c(state)由状态分支或控制2共用；不伪造队首2，不运行共同c/d前段。
WorldStateCommandResult prepare_world_state_transition(const RescueWorldState &state,
                                                       const WorldStateTransitionInput &input);
// 通用队首2：真实c0..20，不是动作n或8成功的A直写。整体替换队列后同次续行；
// q仍读旧s/O，不加status守卫。10旅店在c10公共重置之后执行真实r；18读当前共享u。
WorldStateCommandResult prepare_world_state_command(const RescueWorldState &state,
                                                    const WorldStateCommandInput &input);
} // namespace ark::simulation::rules
