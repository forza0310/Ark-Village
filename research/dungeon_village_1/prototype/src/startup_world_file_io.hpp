#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>
namespace dungeon_village_prototype::persistence_detail {
// 单独的平台接入层：唯一临时文件、刷新、原子替换；不修改世界。
void replace_save_file(const std::filesystem::path &, const std::vector<std::uint8_t> &);
std::vector<std::uint8_t> read_save_file(const std::filesystem::path &, std::size_t max_bytes);
} // namespace dungeon_village_prototype::persistence_detail
