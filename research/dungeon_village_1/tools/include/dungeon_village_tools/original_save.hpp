#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dungeon_village_tools {
// 原版记录的只读结构检查，不导入维护Owner、不提供写回原档接口。
enum class OriginalSaveFormat { container, apk_record, apk_base64, steam_record };
struct OriginalSaveField {
    std::string path;
    std::string type;
    std::size_t offset{}; // 相对解壳后的容器；不含校验头。
    std::size_t size{};
    std::string value; // 整数十进制；字符串原字节十六进制；opaque仅摘要。
};
struct OriginalSaveInspection {
    bool empty_record{};
    std::string input_sha256;
    std::vector<std::uint8_t> container_bytes;
    std::vector<OriginalSaveField> fields;
};
// 限制是维护工具策略：输入16MiB、共131072行、嵌套32层。未知标签不能跳过。
OriginalSaveInspection inspect_original_save(const std::vector<std::uint8_t> &input,
                                             OriginalSaveFormat format,
                                             std::optional<std::uint64_t> steam_id = std::nullopt);
OriginalSaveInspection
inspect_original_save_file(const std::filesystem::path &file, OriginalSaveFormat format,
                           std::optional<std::uint64_t> steam_id = std::nullopt);
// 按原索引路径比较值和类型；仅布局偏移变化不当作字段值变化。
std::vector<std::string> diff_original_saves(const OriginalSaveInspection &before,
                                             const OriginalSaveInspection &after);
} // namespace dungeon_village_tools
