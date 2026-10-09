#pragma once
#include <filesystem>

// 首星主动管理的独立测试Driver；复用完整应用回放容器，不是玩家存档入口。
// 与被动日期通关Driver身份、策略和证书完全分离。
int run_startup_application_active_replay_cli(int argc, const char **argv);
int run_startup_application_active_driver_checks(const std::filesystem::path &parent);
