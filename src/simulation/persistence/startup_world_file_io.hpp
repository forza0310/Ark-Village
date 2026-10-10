#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
namespace ark::simulation::persistence_detail {
// 单独的平台接入层：唯一临时文件、刷新、原子替换；不修改世界。
void replace_save_file(const std::filesystem::path &, const std::vector<std::uint8_t> &);
// 无覆盖发布：目标已存在（包括预检后出现）即拒绝，保留已有目标并退休本次临时文件。
// 只负责单文件发布；研究隔离目录／别名／应用联合安装由上层另行校验。
void create_save_file(const std::filesystem::path &, const std::vector<std::uint8_t> &);
std::vector<std::uint8_t> read_save_file(const std::filesystem::path &, std::size_t max_bytes);
} // namespace ark::simulation::persistence_detail
