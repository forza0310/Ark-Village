#pragma once
#include "ark/simulation/application/startup_application.hpp"
#include <vector>

namespace ark::simulation::persistence_detail {
// 内部研究文件边界；编译时必须给PRIVATE DUNGEON_VILLAGE_RESEARCH_WORK_ROOT。
// 只检查/规范化，不创建、删除、重命名目录或文件；失败抛runtime_error。
struct ApplicationReplayPaths {
    std::filesystem::path directory;
    std::filesystem::path container;
    StartupApplicationPaths application;
};
// root须已存在且严格位于编译绑定的research/work内；output父目录须已存在。
// 捕获只接受新目标，额外protected_paths用于trace/证书等不能重叠的输入输出。
ApplicationReplayPaths prepare_replay_capture_paths(const std::filesystem::path &research_directory,
    const std::filesystem::path &output, const StartupApplicationPaths &current_application_paths,
    const std::vector<std::filesystem::path> &protected_paths = {});
// source须是research/work内无硬链接的普通文件；隔离root须不存在而父目录已存在。
// 保护当前应用整根、源文件和额外输出；只返回安全新根，不创建文件。
ApplicationReplayPaths prepare_replay_restore_paths(const std::filesystem::path &research_directory,
    const std::filesystem::path &source, const StartupApplicationPaths &current_application_paths,
    const std::vector<std::filesystem::path> &protected_paths = {});
} // namespace ark::simulation::persistence_detail
