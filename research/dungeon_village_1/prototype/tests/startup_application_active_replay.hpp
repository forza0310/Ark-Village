#pragma once
#include <filesystem>
#include "dungeon_village_prototype/startup_application_replay.hpp"
#include <array>
#include <memory>

// 首星主动管理的独立测试Driver；复用完整应用回放容器，不是玩家存档入口。
// 与被动日期通关Driver身份、策略和证书完全分离。
int run_startup_application_active_replay_cli(int argc, const char **argv);
int run_startup_application_active_driver_checks(const std::filesystem::path &parent);

// 仅测试层有限交接。内部私有恢复／推进成功才返回；不接受外部可写应用。
namespace active_replay_support {
using Application = dungeon_village_prototype::StartupApplication;
using Metadata = dungeon_village_prototype::StartupApplicationReplayMetadata;
struct Terminal {
    std::unique_ptr<Application> application;
    Metadata metadata;
    std::uint64_t next_task_month{}, sound_count{};
    std::string sound_hash;
};
Terminal prepare_v1_terminal(const std::filesystem::path &source,
                            const std::filesystem::path &live,
                            const std::filesystem::path &unused,
                            const std::vector<std::filesystem::path> &protected_paths);
std::string validate_v1(const Application &, const Metadata &);
std::string validate_v1_origin(const Metadata &); // 只核规范旧字节，不拿旧观测约束新世界。
int check_v1_origin_binding(const Metadata &); // 真实来源上的纯规范变异拒绝检查。
const dungeon_village_reference::WorldScriptPage *top(const Application &);
std::array<std::uint64_t, 4> date(const Application &);
std::array<std::uint64_t, 12> resources(const Application &);
std::string progress_json(const Application &);
bool references(const dungeon_village_prototype::StartupWorldRuntimeState &);
} // namespace active_replay_support
