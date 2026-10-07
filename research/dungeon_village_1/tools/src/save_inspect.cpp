#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/original_save.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_tools;
// 输出至stdout，输入始终只读；不提供修改、写回或运行原代码的路径。
int main(int argc, char **argv) {
    try {
        const std::string usage =
            "用法: kairo_save_inspect --format container|apk-record|apk-base64|steam-record "
            "[--steam-id 十进制ID] 文件 [--compare 文件]";
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
        if (at < argc && std::string(argv[at]) == "--steam-id") {
            if (++at == argc)
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
        if (at == argc || (argc - at != 1 && argc - at != 3))
            throw std::runtime_error(usage);
        const auto file = argv[at++];
        const bool compare = at < argc;
        if (compare && std::string(argv[at++]) != "--compare")
            throw std::runtime_error(usage);
        const auto inspection = inspect_original_save_file(file, format, steam_id);
        if (compare) {
            const auto other = inspect_original_save_file(argv[at], format, steam_id);
            std::cout << "# before_sha256=" << inspection.input_sha256
                      << " after_sha256=" << other.input_sha256 << '\n';
            for (const auto &change : diff_original_saves(inspection, other))
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
