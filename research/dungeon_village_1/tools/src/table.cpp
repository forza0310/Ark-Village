// Strict UTF-8/TSV and typed row readers retain all original fields, including unresolved columns.
// Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_tools/table.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace dungeon_village_tools {
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

std::vector<std::int32_t> parse_integer_list(const std::string &field) {
    if (field.empty()) {
        return {};
    }
    std::vector<std::int32_t> result;
    std::size_t begin = 0;
    while (true) {
        const auto end = field.find('&', begin);
        result.push_back(
            parse_table_integer(field.substr(begin, end == std::string::npos ? end : end - begin)));
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }
    return result;
}

EventProgram parse_event_program(const std::string &field) {
    if (field.size() > 1024U * 1024U) {
        throw std::runtime_error("事件文本超过研究读取预算");
    }
    if (field.empty()) {
        return {};
    }
    EventProgram program;
    std::size_t command_begin = 0;
    while (true) {
        if (program.size() == 4096) {
            throw std::runtime_error("事件指令数超过研究读取预算");
        }
        const auto command_end = field.find('&', command_begin);
        const auto command = std::string_view(field).substr(
            command_begin,
            command_end == std::string::npos ? command_end : command_end - command_begin);
        std::vector<std::int32_t> instruction;
        std::size_t value_begin = 0;
        while (true) {
            if (instruction.size() == 64) {
                throw std::runtime_error("单条事件参数数超过研究读取预算");
            }
            const auto value_end = command.find(',', value_begin);
            auto value = command.substr(value_begin, value_end == std::string_view::npos
                                                         ? value_end
                                                         : value_end - value_begin);
            while (!value.empty() && static_cast<unsigned char>(value.front()) <= 0x20U) {
                value.remove_prefix(1);
            }
            while (!value.empty() && static_cast<unsigned char>(value.back()) <= 0x20U) {
                value.remove_suffix(1);
            }
            // from_chars intentionally excludes '+', unlike Integer.parseInt.
            if (!value.empty() && value.front() == '+') {
                value.remove_prefix(1);
                if (value.empty() || value.front() == '-' || value.front() == '+') {
                    throw std::runtime_error("事件整数正号格式无效");
                }
            }
            instruction.push_back(parse_table_integer(std::string(value)));
            if (value_end == std::string_view::npos) {
                break;
            }
            value_begin = value_end + 1;
        }
        program.push_back(std::move(instruction));
        if (command_end == std::string::npos) {
            return program;
        }
        command_begin = command_end + 1;
    }
}

std::vector<FacilityTableRow> parse_facility_table(const std::vector<std::uint8_t> &bytes) {
    const auto rows = parse_tsv(bytes);
    if (rows.empty()) {
        throw std::runtime_error("设施表不能为空");
    }
    std::vector<FacilityTableRow> result;
    std::set<std::int32_t> ids;
    for (const auto &row : rows) {
        if (row.size() != 36) {
            throw std::runtime_error("设施表必须保留完整 36 列");
        }
        FacilityTableRow definition;
        definition.definition_id = parse_table_integer(row[0]);
        if (definition.definition_id < 0 || !ids.insert(definition.definition_id).second ||
            row[1].empty()) {
            throw std::runtime_error("设施 ID/名称无效或 ID 重复");
        }
        definition.name = row[1];
        for (std::size_t column = 2; column <= 25; ++column) {
            (void)parse_table_integer(row[column]);
        }
        for (const auto column : {26, 27, 28, 29, 31, 34}) {
            (void)parse_integer_list(row[static_cast<std::size_t>(column)]);
        }
        (void)parse_table_integer(row[32]);
        (void)parse_table_integer(row[35]);
        definition.event_program = parse_event_program(row[33]);
        definition.kind = parse_table_integer(row[3]);
        definition.activity_category = parse_table_integer(row[4]);
        definition.footprint_kind = parse_table_integer(row[10]);
        if (definition.footprint_kind < 0 || definition.footprint_kind > 2) {
            throw std::runtime_error("设施形状不属于已证实的 0/1/2 索引");
        }
        std::copy(row.begin(), row.end(), definition.fields.begin());
        result.push_back(std::move(definition));
    }
    return result;
}

std::vector<ItemTableRow> parse_item_table(const std::vector<std::uint8_t> &bytes) {
    const auto rows = parse_tsv(bytes);
    if (rows.empty()) {
        throw std::runtime_error("道具表不能为空");
    }
    std::vector<ItemTableRow> result;
    std::set<std::int32_t> ids;
    for (const auto &row : rows) {
        if (row.size() != 25) {
            throw std::runtime_error("道具表必须保留完整 25 列");
        }
        ItemTableRow item;
        item.definition_id = parse_table_integer(row[0]);
        if (item.definition_id < 0 || row[1].empty() || !ids.insert(item.definition_id).second) {
            throw std::runtime_error("道具 ID/名称无效或 ID 重复");
        }
        item.name = row[1];
        for (std::size_t column = 2; column < row.size(); ++column) {
            if (column != 23) {
                (void)parse_table_integer(row[column]);
            }
        }
        item.legacy_category = parse_table_integer(row[3]);
        if (item.legacy_category < 0) {
            throw std::runtime_error("道具类别不能为负");
        }
        for (std::size_t slot = 0; slot < item.facility_improvements.size(); ++slot) {
            item.facility_improvements[slot] = parse_table_integer(row[9 + slot]);
        }
        std::copy(row.begin(), row.end(), item.fields.begin());
        result.push_back(std::move(item));
    }
    return result;
}

} // namespace dungeon_village_tools
