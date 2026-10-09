#pragma once

#include "dungeon_village_prototype/startup_application.hpp"
#include <functional>

namespace dungeon_village_prototype {
// 研究轮末控制器的规范字节是唯一Driver状态；具体策略由调用者严格校验，不能留空。
struct StartupApplicationReplayMetadata {
    std::string controller_id;
    std::string producer_revision;
    std::uint64_t next_frame{}, next_command{};
    std::vector<std::uint8_t> controller_state;
    std::vector<StartupWorldOpaqueSection> extensions;
};
// 验证器必须是纯检查；全部分配/关系检查在文件发布前完成。
using StartupApplicationReplayValidator = std::function<std::string(
    const StartupApplication &, const StartupApplicationReplayMetadata &)>;

// 复用已经过实际边界测试的内部研究路径检查和无覆盖发布；返回空字符串才成功。
// 路径限定编译绑定research/work内，拒绝覆盖输入、已有候选或应用两栏/系统。
std::string save_startup_application_replay(
    const std::filesystem::path &research_directory, const std::filesystem::path &output,
    const StartupApplication &, const StartupApplicationReplayMetadata &,
    const StartupApplicationReplayValidator &,
    const std::vector<std::filesystem::path> &protected_paths = {});
// 目标目录须已存在，system.avr/world0.avr/world1.avr须全不存在。
// 私有解码及Driver校验完成后，只创建system.avr，再noexcept联合安装app与Driver字节。
// 不调用普通应用构造/载入，不重写镜像、不初始化页面、不抽随机、不写两栏世界档。
std::string restore_startup_application_replay(
    const std::filesystem::path &source, const std::filesystem::path &isolated_directory,
    const std::string &expected_controller, StartupApplication &,
    StartupApplicationReplayMetadata &, const StartupApplicationReplayValidator &,
    const std::vector<std::filesystem::path> &protected_paths = {});
// 完整规范状态摘要，含系统、世界/历史、应用控制字段及Driver；不含环境路径。
// 它不是容器文件字节hash；逐轮比较复用完整Session摘要，文件自检仅在实际存取时执行。
// 非法输入抛具名异常，不返回一个看似成功的摘要。
std::string startup_application_replay_digest(
    const StartupApplication &, const StartupApplicationReplayMetadata &,
    const StartupApplicationReplayValidator &);
const char *startup_application_replay_schema();
} // namespace dungeon_village_prototype
