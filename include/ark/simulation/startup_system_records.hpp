#pragma once

#include "ark/simulation/startup_world_persistence.hpp"

namespace ark::simulation {
// 应用跨局纪录独立于单世界；继承列表保留原序，未知可选段原样往返。
struct StartupSystemRecords {
    int last_slot{};
    std::int64_t high_score{}, cash_peak{};
    std::string score_village{"没有记录"}, cash_village{"没有记录"};
    int trophy{};
    // 原继承大端short字节串；单项语义与当前rules长度由应用安装候选时验证。
    std::vector<std::uint8_t> facility_levels, profession_status;
    std::vector<StartupWorldOpaqueSection> opaque_sections;
};
struct StartupSystemLoadResult {
    std::optional<StartupSystemRecords> records;
    bool missing{}; // 仅真正文件缺失可返回默认候选；损坏／目录／权限不作缺失。
    std::string error;
};
// 空字符串表示成功；校验／读取均不修改调用者。保存复用原子单文件替换。
std::string validate_startup_system_records(const StartupSystemRecords &records);
StartupSystemLoadResult load_startup_system_file(const std::filesystem::path &path);
std::string save_startup_system_file(const std::filesystem::path &path,
                                   const StartupSystemRecords &records);
} // namespace ark::simulation
