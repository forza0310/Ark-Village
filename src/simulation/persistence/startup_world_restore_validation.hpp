#pragma once

#include "ark/simulation/world/startup_world_runtime.hpp"

#include <string>

namespace ark::simulation::persistence_detail {
// 只读校验完整候选；审计检查点允许轮内相位，但不能作为可继续执行的主状态。
bool validate_restored_state(const StartupWorldRuntimeState &state, std::string &reason,
                             bool audit_checkpoint = false);
} // namespace ark::simulation::persistence_detail
