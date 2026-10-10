#pragma once
#include "ark/simulation/startup_application_storage.hpp"
#include "startup_world_codec.hpp"
#include <functional>

namespace ark::simulation::persistence_detail {
// 同一应用容器的活动Session、历史和各唯一blob共享budget；临时世界销毁不返还预算。
void validate_application_replay_storage_view(const StartupApplicationStorageView &, CodecDecodeBudget &);
// 完整文件视图先在独占同卷目录准备，最后无覆盖发布新根；成功后只移动返回已准备的观察值。
// before_publish用于主回放层复核源／protected_paths，必须在原子发布前完成所有可能失败动作。
StartupApplicationStorageSnapshot publish_application_replay_storage(
    const std::filesystem::path &new_root, const StartupApplicationStorageView &,
    const std::function<void()> &before_publish = {});
} // namespace ark::simulation::persistence_detail
