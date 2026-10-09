#pragma once
#include <filesystem>

// 二星策略首批：有限首星交接及真实下一任务；不宣称已经完成二星或住宅。
int run_startup_application_second_star_replay_cli(int argc, const char **argv);
int run_startup_application_second_star_driver_checks(const std::filesystem::path &parent);
