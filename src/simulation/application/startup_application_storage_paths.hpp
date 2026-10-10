#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
namespace ark::simulation::persistence_detail {
// 只读校验。false仅允许末级根缺失、父目录已存在；绑定本研究work，拒绝别名与链接。
std::filesystem::path validate_storage_root(const std::filesystem::path &, bool must_exist);
void validate_storage_file(const std::filesystem::path &, bool allow_missing = true);
std::string storage_hash_hex(const std::array<std::uint8_t,32> &);
} // namespace ark::simulation::persistence_detail
