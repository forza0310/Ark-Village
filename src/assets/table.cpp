// Adapted research/tools UTF-8/TSV readers; only the asset metadata subset is retained.
// Package responsibilities and evidence boundaries: ../README.md.

#include "ark/assets/table.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace ark::assets {
namespace {

void validate_utf8(const std::vector<std::uint8_t> &bytes) {
    for (std::size_t index = 0; index < bytes.size();) {
        const auto byte = bytes[index++];
        if (byte < 0x80U) {
            if ((byte < 0x20U && byte != '\t' && byte != '\r' && byte != '\n') || byte == 0x7FU) {
                throw std::runtime_error("表中存在非法控制字符");
            }
            continue;
        }
        const int continuation = byte >= 0xC2U && byte <= 0xDFU   ? 1
                                 : byte >= 0xE0U && byte <= 0xEFU ? 2
                                 : byte >= 0xF0U && byte <= 0xF4U ? 3
                                                                  : -1;
        if (continuation < 0 || static_cast<std::size_t>(continuation) > bytes.size() - index) {
            throw std::runtime_error("表不是完整 UTF-8");
        }
        std::uint32_t codepoint = byte & (continuation == 1   ? 0x1FU
                                          : continuation == 2 ? 0x0FU
                                                              : 0x07U);
        for (int part = 0; part < continuation; ++part) {
            const auto next = bytes[index++];
            if ((next & 0xC0U) != 0x80U) {
                throw std::runtime_error("UTF-8 续字节无效");
            }
            codepoint = (codepoint << 6U) | (next & 0x3FU);
        }
        const auto minimum = continuation == 1 ? 0x80U : continuation == 2 ? 0x800U : 0x10000U;
        if (codepoint < minimum || codepoint > 0x10FFFFU ||
            (codepoint >= 0xD800U && codepoint <= 0xDFFFU)) {
            throw std::runtime_error("UTF-8 码点无效");
        }
    }
}

} // namespace

TableRows parse_tsv(const std::vector<std::uint8_t> &bytes) {
    if (bytes.size() > 16U * 1024U * 1024U) {
        throw std::runtime_error("表超过研究读取器的大小上限");
    }
    validate_utf8(bytes);
    const std::size_t begin =
        bytes.size() >= 3 && bytes[0] == 0xEFU && bytes[1] == 0xBBU && bytes[2] == 0xBFU ? 3 : 0;
    TableRows rows;
    if (begin == bytes.size()) {
        return rows;
    }
    std::vector<std::string> row;
    std::string field;
    bool line_ended = false;
    for (auto index = begin; index < bytes.size(); ++index) {
        const auto byte = bytes[index];
        if (byte == '\r') {
            if (index + 1 >= bytes.size() || bytes[index + 1] != '\n') {
                throw std::runtime_error("孤立 CR 不属于已支持的表换行格式");
            }
            continue;
        }
        if (byte == '\t' || byte == '\n') {
            row.push_back(std::move(field));
            field.clear();
            if (row.size() > 1024) {
                throw std::runtime_error("表列数超过研究上限");
            }
            if (byte == '\n') {
                rows.push_back(std::move(row));
                row.clear();
                if (rows.size() > 100000) {
                    throw std::runtime_error("表行数超过研究上限");
                }
            }
            line_ended = byte == '\n';
        } else {
            field.push_back(static_cast<char>(byte));
            line_ended = false;
        }
    }
    if (!line_ended) {
        row.push_back(std::move(field));
        if (row.size() > 1024 || rows.size() == 100000) {
            throw std::runtime_error("表行列数超过研究上限");
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

std::int32_t parse_table_integer(const std::string &field) {
    if (field.empty()) {
        throw std::runtime_error("表整数为空");
    }
    std::int32_t value{};
    const auto result = std::from_chars(field.data(), field.data() + field.size(), value);
    if (result.ec != std::errc{} || result.ptr != field.data() + field.size()) {
        throw std::runtime_error("表整数格式无效或溢出: " + field);
    }
    return value;
}

} // namespace ark::assets
