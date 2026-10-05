#pragma once

#include "ark/simulation/startup_world_runtime.hpp"
#include "ark/simulation/rules/world_exploration.hpp"

namespace ark::simulation {
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
} // namespace ark::simulation
