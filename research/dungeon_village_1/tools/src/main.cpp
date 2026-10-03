// Archive inspection/extraction CLI; list-only mode never writes decoded entries.
// Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_tools/archive.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct Options {
    std::filesystem::path input;
    std::filesystem::path output;
    bool list_only{};
};

Options parse_options(int argc, char **argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--input" && index + 1 < argc) {
            options.input = argv[++index];
        } else if (argument == "--output" && index + 1 < argc) {
            options.output = argv[++index];
        } else if (argument == "--list-only") {
            options.list_only = true;
        } else {
            throw std::invalid_argument("未知或不完整参数: " + argument);
        }
    }
    if (options.input.empty()) {
        throw std::invalid_argument("必须提供 --input");
    }
    if (!options.list_only && options.output.empty()) {
        throw std::invalid_argument("提取时必须提供 --output");
    }
    return options;
}

} // namespace

int main(int argc, char **argv) {
    try {
        const auto options = parse_options(argc, argv);
        const auto encrypted = dungeon_village_tools::read_binary_file(options.input);
        const auto archive = dungeon_village_tools::decode_game_archive(
            options.input.filename().string(), encrypted);

        std::cout << options.input.filename().string() << "\t条目=" << archive.entries.size()
                  << "\t视觉归档="
                  << (dungeon_village_tools::is_visual_archive(archive) ? "是" : "否") << '\n';
        for (const auto &entry : archive.entries) {
            std::cout << entry.name << '\t' << entry.data.size() << '\t'
                      << static_cast<unsigned int>(entry.flags);
            if (const auto size = dungeon_village_tools::read_png_size(entry.data);
                size.has_value()) {
                std::cout << "\tPNG=" << size->width << 'x' << size->height;
            }
            std::cout << '\n';
        }

        if (!options.list_only) {
            const auto count =
                dungeon_village_tools::extract_visual_entries(archive, options.output);
            std::cout << "已提取视觉条目=" << count << '\n';
        }
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "错误: " << error.what() << '\n';
        return 1;
    }
}
