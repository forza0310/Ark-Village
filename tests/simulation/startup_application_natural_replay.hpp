#pragma once
#include <filesystem>
namespace ark::simulation { class StartupApplication; }

// 真实应用新局的被动经营Driver；复用既有测试可执行，不是玩家存档入口。
int run_startup_application_natural_replay_cli(int argc, const char **argv);
int run_startup_application_natural_driver_checks(const std::filesystem::path &root);
// 复用现有真实动作夹具的未领取声音，不重复模拟自然前缀。
int run_startup_application_natural_pending_sound_check(
    const ark::simulation::StartupApplication &app);
