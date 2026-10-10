#pragma once

#include "ark/simulation/map/startup_world_projection.hpp"

namespace ark::simulation {
// 只读调用点输入，不是第二个世界Owner。共享等级/装备开放取当前routes，扩展字段由Owner投影。
struct StartupWorldRouteFacts {
    const StartupWorldRules *rules{};
    std::vector<ref::DungeonFinishSurface> surface;
    std::vector<ref::Position> exits;          // 原Map.f顺序，不重建或排序。
    std::vector<ref::CandidateMapEvent> tasks; // 原bq全部，包括已结束记录。
    std::map<int, std::array<int, 4>> human_homes;
    std::map<int, std::array<int, 4>> facility_improvements;
    std::map<std::uint64_t, std::array<int, 3>> neighbourhood;
    std::array<int, 10> job_counts;
    std::map<ref::CharacterId, StartupWorldActorMetadata> actor_metadata;
    std::map<ref::CharacterId, int> facing;
    std::map<ref::CharacterId, bool> actor_visible; // 原渲染裁剪资格，不能由逻辑move_area替代。
    std::optional<ref::WorldEventEntryInput> task_entry;
    ref::WorldPathTaskAttempt task_attempt; // 实际F创建消费者；同Owner随机/任务提交。
    std::optional<ref::CollisionBox> actor_box;
    std::optional<ref::CollisionBox> rescue_box;
    std::optional<ref::CollisionBox> object_box;
    std::array<int, 4> calendar;
    bool primary_expression_table{true};
    ref::MiscSoundProjection sound_projection;
};
// 全目录真实字段，随机留给领域调用点懒抽；不执行c/d、收费、脚本或重选目标。
std::optional<ref::WorldActorDecisionInput>
prepare_startup_world_decision_input(const ref::WorldActorRoutesState &routes,
                                     ref::CharacterId actor, const StartupWorldRouteFacts &facts);
// Runtime-only shape: retains the full builder's validation/order but stores
// only inputs read by the current c branch. The public full builder stays an oracle.
std::optional<ref::WorldActorDecisionInput>
prepare_startup_world_decision_input_for_state(const ref::WorldActorRoutesState &routes,
                                               ref::CharacterId actor,
                                               const StartupWorldRouteFacts &facts);
std::optional<ref::WorldActorCommandInput>
prepare_startup_world_command_input(const ref::WorldActorRoutesState &routes,
                                    ref::CharacterId actor, const ref::LegacyActorControl &command,
                                    const StartupWorldRouteFacts &facts);
// Runtime-only command shape: validates all equipment keys in the original order,
// retaining equipment only for the actual shop command/exit consumer.
std::optional<ref::WorldActorCommandInput>
prepare_startup_world_command_input_for_command(const ref::WorldActorRoutesState &routes,
                                                ref::CharacterId actor,
                                                const ref::LegacyActorControl &command,
                                                const StartupWorldRouteFacts &facts);
} // namespace ark::simulation
