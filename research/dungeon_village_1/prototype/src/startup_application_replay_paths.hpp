#pragma once
#include "dungeon_village_prototype/startup_application.hpp"
#include <vector>

namespace dungeon_village_prototype::persistence_detail {
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
// source须是research/work内的普通文件；隔离root须现有，允许保留无关文件。
// 返回的system.avr/world0.avr/world1.avr都尚不存在，不写两栏或当前应用文件。
ApplicationReplayPaths prepare_replay_restore_paths(const std::filesystem::path &research_directory,
    const std::filesystem::path &source, const StartupApplicationPaths &current_application_paths,
    const std::vector<std::filesystem::path> &protected_paths = {});
} // namespace dungeon_village_prototype::persistence_detail
