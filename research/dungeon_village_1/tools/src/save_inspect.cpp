#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/original_save.hpp"
#include "dungeon_village_tools/original_save_audit.hpp"
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace dungeon_village_tools;

namespace {
// 审计模式只输出数值和有界引用诊断，不输出容器中的名字/字符串原字节。
void print_audit(std::ostream &out, const OriginalSaveAudit &audit, const char *label) {
    out << "# audit=" << label << " profile=" << audit.profile << '\n';
    out << "partition\tbytes\tconsumed\trecords\tnoncanonical_booleans\n";
    for (const auto &part : audit.partitions)
        out << part.partition << '\t' << part.bytes << '\t' << part.consumed << '\t' << part.records
            << '\t' << part.noncanonical_booleans << '\n';
    out << "partition\treference\tchecked\tmissing\tfirst_missing_values\tfirst_missing_pairs\n";
    for (const auto &ref : audit.references) {
        out << ref.partition << '\t' << ref.field << '\t' << ref.checked << '\t' << ref.missing
            << '\t';
        for (std::size_t i = 0; i < ref.missing_values.size(); ++i)
            out << (i ? "," : "") << ref.missing_values[i];
        out << '\t';
        for (std::size_t i = 0; i < ref.missing_pairs.size(); ++i)
            out << (i ? "," : "") << ref.missing_pairs[i].first << ':'
                << ref.missing_pairs[i].second;
        out << '\n';
    }
    out << "fact\tvalue\n";
    for (const auto &fact : audit.facts)
        out << fact.first << '\t' << fact.second << '\n';
}
} // namespace

// 输出至stdout，输入始终只读；不提供修改、写回或运行原代码的路径。
int main(int argc, char **argv) {
    try {
        const std::string usage =
            "用法: kairo_save_inspect --format container|apk-record|apk-base64|steam-record "
            "[--steam-id 十进制ID] [--audit-profile apk108|steam-9cf4bb10] 文件 [--compare 文件]";
        if (argc < 4 || std::string(argv[1]) != "--format")
            throw std::runtime_error(usage);
        const std::string format_name = argv[2];
        OriginalSaveFormat format;
        if (format_name == "container")
            format = OriginalSaveFormat::container;
        else if (format_name == "apk-record")
            format = OriginalSaveFormat::apk_record;
        else if (format_name == "apk-base64")
            format = OriginalSaveFormat::apk_base64;
        else if (format_name == "steam-record")
            format = OriginalSaveFormat::steam_record;
        else
            throw std::runtime_error("不支持的输入格式");
        int at = 3;
        std::optional<std::uint64_t> steam_id;
        std::optional<OriginalSaveAuditProfile> audit_profile;
        while (at < argc && std::string(argv[at]).rfind("--", 0) == 0) {
            const std::string option = argv[at];
            if (++at == argc)
                throw std::runtime_error(usage);
            if (option == "--audit-profile") {
                if (audit_profile)
                    throw std::runtime_error("审计profile不能重复");
                const std::string value = argv[at++];
                if (value == "apk108")
                    audit_profile = OriginalSaveAuditProfile::apk108;
                else if (value == "steam-9cf4bb10")
                    audit_profile = OriginalSaveAuditProfile::steam_9cf4bb10;
                else
                    throw std::runtime_error("不支持的审计profile");
                continue;
            }
            if (option != "--steam-id" || steam_id)
                throw std::runtime_error(usage);
            const std::string id = argv[at++];
            std::uint64_t value{};
            if (id.empty())
                throw std::runtime_error("SteamID必须是十进制整数");
            for (const char c : id) {
                if (c < '0' || c > '9' ||
                    value > (std::numeric_limits<std::uint64_t>::max() -
                             static_cast<unsigned>(c - '0')) /
                                10)
                    throw std::runtime_error("SteamID不合法或越界");
                value = value * 10 + static_cast<unsigned>(c - '0');
            }
            steam_id = value;
        }
        if (audit_profile && format != OriginalSaveFormat::container &&
            ((format == OriginalSaveFormat::steam_record) !=
             (*audit_profile == OriginalSaveAuditProfile::steam_9cf4bb10)))
            throw std::runtime_error("输入格式与审计profile不匹配");
        if (at == argc || (argc - at != 1 && argc - at != 3))
            throw std::runtime_error(usage);
        const auto file = argv[at++];
        const bool compare = at < argc;
        if (compare && std::string(argv[at++]) != "--compare")
            throw std::runtime_error(usage);
        const auto inspection = inspect_original_save_file(file, format, steam_id);
        std::ostringstream report;
        if (audit_profile) {
            const auto audit = audit_original_save(inspection, *audit_profile);
            report << "# input_sha256=" << inspection.input_sha256 << '\n';
            print_audit(report, audit, compare ? "before" : "input");
            if (compare) {
                const auto other = inspect_original_save_file(argv[at], format, steam_id);
                const auto other_audit = audit_original_save(other, *audit_profile);
                report << "# after_sha256=" << other.input_sha256 << '\n';
                print_audit(report, other_audit, "after");
            }
            std::cout << report.str(); // 两份均通过后才发布；失败没有半份成功报告。
            return 0;
        }
        if (compare) {
            const auto other = inspect_original_save_file(argv[at], format, steam_id);
            const auto changes = diff_original_saves(inspection, other);
            std::cout << "# before_sha256=" << inspection.input_sha256
                      << " after_sha256=" << other.input_sha256 << '\n';
            for (const auto &change : changes)
                std::cout << change << '\n';
        } else {
            std::cout << "# input_sha256=" << inspection.input_sha256
                      << " empty_record=" << inspection.empty_record
                      << " container_bytes=" << inspection.container_bytes.size() << '\n';
            std::cout << "# container_sha256=" << sha256_hex(inspection.container_bytes) << '\n';
            std::cout << "path\ttype\toffset\tbytes\tvalue\n";
            for (const auto &field : inspection.fields)
                std::cout << field.path << '\t' << field.type << '\t' << field.offset << '\t'
                          << field.size << '\t' << field.value << '\n';
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
