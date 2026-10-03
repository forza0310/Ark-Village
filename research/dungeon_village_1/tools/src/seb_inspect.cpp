#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/sprite.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<std::filesystem::path> collect_files(const std::filesystem::path &input) {
    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_regular_file(input)) {
        files.push_back(input);
    } else if (std::filesystem::is_directory(input)) {
        for (const auto &entry : std::filesystem::recursive_directory_iterator(input)) {
            if (entry.is_regular_file() && entry.path().extension() == ".seb") {
                files.push_back(entry.path());
            }
        }
    } else {
        throw std::runtime_error("输入路径不是文件或目录: " + input.string());
    }
    std::sort(files.begin(), files.end());
    return files;
}

} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 2 && !(argc == 3 && std::string(argv[2]) == "--parts")) {
            throw std::invalid_argument("用法: kairo_seb_inspect <SEB 文件或目录> [--parts]");
        }
        const bool show_parts = argc == 3;
        if (show_parts && !std::filesystem::is_regular_file(argv[1])) {
            throw std::invalid_argument("逐记录输出必须指定单个 SEB 文件");
        }
        const auto files = collect_files(argv[1]);
        if (files.empty()) {
            throw std::runtime_error("输入中没有 SEB 文件");
        }

        std::size_t total_layers = 0;
        std::size_t total_parts = 0;
        std::uint16_t largest_frame_count = 0;
        for (const auto &file : files) {
            const auto sprite = dungeon_village_tools::parse_legacy_seb(
                dungeon_village_tools::read_binary_file(file));
            total_layers += sprite.layers.size();
            largest_frame_count = std::max(largest_frame_count, sprite.frame_count);
            for (const auto &layer : sprite.layers) {
                total_parts += layer.parts.size();
                if (show_parts) {
                    std::cout << "图层 legacy_tag=" << layer.legacy_tag << '\n';
                    for (const auto &part : layer.parts) {
                        std::cout << "frame=" << part.frame << " image=" << part.image_index
                                  << " source=" << part.source_x << ',' << part.source_y << ','
                                  << part.width << ',' << part.height << " offset=" << part.offset_x
                                  << ',' << part.offset_y << " flip=" << part.flip_x << ','
                                  << part.flip_y << '\n';
                    }
                }
            }
        }

        std::cout << "SEB 文件=" << files.size() << "\t图层=" << total_layers
                  << "\t记录=" << total_parts << "\t最大帧数=" << largest_frame_count << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "错误: " << error.what() << '\n';
        return 1;
    }
}
