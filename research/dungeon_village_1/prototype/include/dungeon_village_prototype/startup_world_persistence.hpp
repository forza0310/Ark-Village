#pragma once

#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include <filesystem>

namespace dungeon_village_prototype {
// 维护格式用途显式分开；不是APK或Steam原档兼容接口。
enum class StartupWorldSavePurpose : std::uint32_t { normal = 1, replay = 2 };
struct StartupWorldOpaqueSection {
    std::uint32_t id{}; // 扩展ID必须>=1024；仅保留不影响世界引用的可选附属数据。
    std::uint32_t version{1};
    std::vector<std::uint8_t> bytes;
};
struct StartupWorldSaveMetadata {
    StartupWorldSavePurpose purpose{StartupWorldSavePurpose::normal};
    std::string producer_revision; // 仅追溯；不因Git提交不同自动失效。
    std::string controller_id;     // 回放策略语义版本；正常档为空。
    std::uint64_t next_frame{};    // 完成外层轮后下一帧，不是轮内检查点。
    std::vector<std::uint8_t> controller_state;
    std::vector<StartupWorldOpaqueSection> extensions;
};
struct StartupWorldSaveResult {
    bool ok{};
    std::string error;
};
struct StartupWorldSavedSession {
    StartupWorldRuntimeSession session;
    StartupWorldSaveMetadata metadata;
};
struct StartupWorldLoadResult {
    std::optional<StartupWorldSavedSession> snapshot;
    std::string error;
};
// 捕获不推进世界或随机。回放要求外层轮完成且声音已领取；正常档要求稳定主场景。
// 写失败保留旧有效目标；读取先形成私有候选，不修改调用者或外部控制器。
StartupWorldSaveResult save_startup_world_file(const std::filesystem::path &path,
                                               const StartupWorldRuntimeSession &session,
                                               const StartupWorldSaveMetadata &metadata);
StartupWorldLoadResult load_startup_world_file(const std::filesystem::path &path,
                                               const StartupWorldRules &rules,
                                               StartupWorldSavePurpose expected_purpose,
                                               const std::string &expected_controller = {});
// 字段规范编码的SHA-256，包含全部动态字段；Session版本同时覆盖有序审计历史。
std::string startup_world_state_digest(const StartupWorldRuntimeState &state);
std::string startup_world_session_digest(const StartupWorldRuntimeSession &session);
const char *startup_world_persistence_dataset();
} // namespace dungeon_village_prototype
