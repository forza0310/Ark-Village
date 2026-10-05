#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_reference/world_exploration.hpp"

namespace dungeon_village_prototype {
// 玩家显式打开任务列表/操作栈顶页；不自动接任务，不绕过征集、费用或出发动画。
StartupWorldRuntimeError open_startup_world_runtime_task_menu(StartupWorldRuntimeState &state);
// 原冒险页4的已证任务中止入口；普通任务目录入口不替玩家选择中止。
StartupWorldRuntimeError
open_startup_world_runtime_task_control_menu(StartupWorldRuntimeState &state);
std::optional<StartupWorldRuntimeState>
update_startup_world_runtime_task_control_page(const StartupWorldRuntimeState &state,
                                               std::uint64_t page);
// 共用n.o实体恢复，但没有期限页/费用/aI消费；调用者仅能在私有候选上执行。
bool abort_startup_world_runtime_task_entities(StartupWorldRuntimeState &state);
StartupWorldTaskPageResult act_startup_world_runtime_task_page(StartupWorldRuntimeState &state,
                                                               std::uint64_t page,
                                                               StartupWorldTaskAction action,
                                                               int selection = 0);
std::optional<StartupWorldRuntimeState>
update_startup_world_runtime_task_page(const StartupWorldRuntimeState &state, std::uint64_t page,
                                       bool held = false);
StartupWorldTaskPageResult act_startup_world_runtime_deadline_page(StartupWorldRuntimeState &state,
                                                                   std::uint64_t page,
                                                                   int selection);
std::optional<StartupWorldRuntimeState>
update_startup_world_runtime_deadline_page(const StartupWorldRuntimeState &state,
                                           std::uint64_t page);
// raw33实际关闭后，首次获准main b()入口才合计费用/续费或完整中止。
std::optional<StartupWorldRuntimeState>
prepare_startup_world_runtime_deadline_result(const StartupWorldRuntimeState &state);
ref::WorldTaskCreationState startup_world_runtime_factory(const StartupWorldRuntimeState &state);
bool write_startup_world_runtime_factory(StartupWorldRuntimeState &state,
                                         const ref::WorldTaskCreationState &factory);
std::optional<StartupWorldRuntimeState>
prepare_startup_world_runtime_dungeon_finish(const StartupWorldRuntimeState &state,
                                             std::uint64_t facility);
std::optional<StartupWorldRuntimeState>
prepare_startup_world_runtime_dungeon_crew(const StartupWorldRuntimeState &state,
                                           std::uint64_t facility);
std::optional<StartupWorldRuntimeState>
prepare_startup_world_runtime_encounter(const StartupWorldRuntimeState &state,
                                        ref::EncounterCreationInput input);
// 全局入口需要真实created/denial，不能从新名单长度猜测创建结果。
std::optional<ref::OwnedWorldRuntimeCreation<StartupWorldRuntimeState>>
prepare_startup_world_runtime_encounter_creation(const StartupWorldRuntimeState &state,
                                                 ref::EncounterCreationInput input);
std::optional<StartupWorldRuntimeState>
consume_startup_world_runtime_encounter_request(const StartupWorldRuntimeState &state,
                                                const ref::EncounterCreationRequest &request);
std::optional<ref::WorldEventEntryInput>
startup_world_runtime_task_entry(const StartupWorldRuntimeState &state, ref::CharacterId actor);
std::optional<ref::EncounterCommitInput>
startup_world_runtime_encounter_input(const StartupWorldRuntimeState &state,
                                      std::uint64_t encounter);
bool consume_startup_world_runtime_task_encounter_request(StartupWorldRuntimeState &state,
                                                          const ref::EncounterRequest &request);
bool initialize_startup_world_runtime_task_result_page(StartupWorldRuntimeState &state,
                                                       std::uint64_t page);
void configure_startup_world_runtime_task_adapter(
    ref::WorldRuntimeAdapter<StartupWorldRuntimeState> &adapter);
// 局部L/P可在人物c期间真实生成怪物；同轮怪物d前同步其构造时的渲染元数据。
void synchronize_startup_world_runtime_monsters(StartupWorldRuntimeState &state);
} // namespace dungeon_village_prototype
