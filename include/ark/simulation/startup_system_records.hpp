#pragma once

#include "ark/simulation/startup_world_persistence.hpp"
#include <array>

namespace ark::simulation {
enum class StartupSaveKind { interrupt = 0, manual = 1 };
// 四条目录只保存受校验内容身份，不接受外部文件名或路径。
struct StartupSaveReference {
    std::array<std::uint8_t, 32> sha256{};
    std::uint64_t bytes{};
    StartupWorldSavePurpose purpose{StartupWorldSavePurpose::normal};
};
struct StartupSaveDirectoryEntry {
    int packed_date{-1}; // 原零基year*10000+month*100+week；-1仅隐藏目录。
    std::string village;
    std::int64_t cash{};
    std::optional<StartupSaveReference> reference;
};
bool operator==(const StartupSaveReference &, const StartupSaveReference &);
bool operator==(const StartupSaveDirectoryEntry &, const StartupSaveDirectoryEntry &);
// 应用跨局纪录独立于单世界；继承列表保留原序，未知可选段原样往返。
struct StartupSystemRecords {
    int last_slot{};
    std::int64_t high_score{}, cash_peak{};
    std::string score_village{"没有记录"}, cash_village{"没有记录"};
    int trophy{};
    // 原继承大端short字节串；单项语义与当前rules长度由应用安装候选时验证。
    std::vector<std::uint8_t> facility_levels, profession_status;
    std::vector<StartupWorldOpaqueSection> opaque_sections;
    std::uint64_t revision{};
    // 第一索引slot0/1，第二索引中断0/手动1；隐藏行仍可保留引用及原目录文本。
    std::array<std::array<StartupSaveDirectoryEntry, 2>, 2> save_directory{};
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
