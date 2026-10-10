#pragma once
#include <filesystem>

// 独立住宅策略：继承募集任务，真实赠礼／入住／建设；不扩为完整二星完成。
int run_startup_application_residence_replay_cli(int argc, const char **argv);
int run_startup_application_residence_driver_checks(const std::filesystem::path &parent);
