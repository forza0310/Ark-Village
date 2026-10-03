#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/sprite.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Mapping {
    std::string logical_key;
    std::filesystem::path source;
    std::filesystem::path normalized;
    std::string frame;
    std::string anchor;
    std::string transform;
};

const std::vector<Mapping> kMappings = {
    {"terrain.grass", "image/plain00.png", "normalized/terrain.grass.png", "0,0,60,29", "30,14",
     "原始 PNG；帧 0 来自 image/plain00.seb"},
    {"terrain.road", "image/road00.png", "normalized/terrain.road.png", "0,0,60,29", "30,14",
     "原始 PNG；帧 0 来自 image/road00.seb"},
    {"building.inn", "image/tenant10.png", "normalized/building.inn.single.png", "0,0,60,60",
     "30,45", "原始 PNG；帧 0 来自 image/tenant10.seb；定义 28，经 mapchip_main[48]"},
    {"building.cafe", "image/t_cafe.png", "normalized/building.cafe.png", "0,0,60,60", "30,45",
     "原始 PNG；帧 0 来自 image/t_cafe.seb"},
    {"building.inn.pair.front", "image/t_inn00.png", "normalized/building.inn.pair.front.png",
     "0,0,60,48", "30,33", "原始 PNG；t_inn00.seb 帧 0/1，定义 29 分片 0/1；奇数帧水平翻转"},
    {"building.inn.pair.back", "image/t_inn01.png", "normalized/building.inn.pair.back.png",
     "0,0,60,51", "30,36", "原始 PNG；t_inn00.seb 帧 2/3，定义 29 分片 2/3；奇数帧水平翻转"},
    {"character.night", "human/chara_night00.png", "normalized/character.night.png", "0,0,18,24",
     "9,24", "原始 PNG；帧 0 来自 human/walk00.seb"},
};

void reject_tsv_control_characters(const std::string &text) {
    if (text.find('\t') != std::string::npos || text.find('\n') != std::string::npos ||
        text.find('\r') != std::string::npos) {
        throw std::runtime_error("素材路径包含 TSV 控制字符");
    }
}

void write_identical_or_new(const std::filesystem::path &path,
                            const std::vector<std::uint8_t> &bytes) {
    if (std::filesystem::exists(path)) {
        if (dungeon_village_tools::read_binary_file(path) != bytes) {
            throw std::runtime_error("目标文件已存在且内容不同: " + path.string());
        }
        return;
    }
    std::filesystem::create_directories(path.parent_path());
    const auto temporary = path.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            throw std::runtime_error("无法创建文件: " + temporary);
        }
        stream.write(reinterpret_cast<const char *>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
        if (!stream) {
            throw std::runtime_error("无法写入文件: " + temporary);
        }
    }
    std::filesystem::rename(temporary, path);
}

void write_text_identical_or_new(const std::filesystem::path &path, const std::string &text) {
    write_identical_or_new(path, {text.begin(), text.end()});
}

} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 3) {
            throw std::invalid_argument("用法: kairo_asset_publish <提取目录> <素材目录>");
        }
        const std::filesystem::path extracted_root = argv[1];
        const std::filesystem::path assets_root = argv[2];
        if (!std::filesystem::is_directory(extracted_root)) {
            throw std::runtime_error("提取目录不存在");
        }

        std::vector<std::filesystem::path> files;
        for (const auto &entry : std::filesystem::recursive_directory_iterator(extracted_root)) {
            if (entry.is_regular_file()) {
                files.push_back(std::filesystem::relative(entry.path(), extracted_root));
            }
        }
        std::sort(files.begin(), files.end());
        std::map<std::filesystem::path, std::vector<std::uint8_t>> contents;
        for (const auto &relative : files) {
            reject_tsv_control_characters(relative.generic_string());
            const auto bytes = dungeon_village_tools::read_binary_file(extracted_root / relative);
            if (relative.extension() == ".png" &&
                !dungeon_village_tools::read_png_size(bytes).has_value()) {
                throw std::runtime_error("PNG 头非法: " + relative.generic_string());
            }
            if (relative.extension() == ".seb") {
                (void)dungeon_village_tools::parse_legacy_seb(bytes);
            }
            contents.emplace(relative, bytes);
        }
        for (const auto &mapping : kMappings) {
            if (contents.find(mapping.source) == contents.end()) {
                throw std::runtime_error("逻辑映射缺少源文件: " + mapping.source.generic_string());
            }
        }

        std::string manifest =
            "logical_key\tsource_archive\tsource_entry\tsource_sha256\tnormalized_path\twidth\t"
            "height\tframe\tanchor\ttransform\n";
        for (const auto &entry : contents) {
            const auto &relative = entry.first;
            const auto &bytes = entry.second;
            write_identical_or_new(assets_root / "original" / relative, bytes);
            const auto mapping = std::find_if(
                kMappings.begin(), kMappings.end(),
                [&relative](const auto &candidate) { return candidate.source == relative; });
            std::string logical_key;
            std::string normalized_path;
            std::string frame;
            std::string anchor;
            std::string transform = "原始提取文件，未转换";
            if (mapping != kMappings.end()) {
                logical_key = mapping->logical_key;
                normalized_path = mapping->normalized.generic_string();
                frame = mapping->frame;
                anchor = mapping->anchor;
                transform = mapping->transform;
                write_identical_or_new(assets_root / mapping->normalized, bytes);
            }
            std::string width;
            std::string height;
            if (const auto size = dungeon_village_tools::read_png_size(bytes); size.has_value()) {
                width = std::to_string(size->width);
                height = std::to_string(size->height);
            }
            manifest += logical_key + '\t' + relative.parent_path().filename().string() + ".dat\t" +
                        relative.filename().string() + '\t' +
                        dungeon_village_tools::sha256_hex(bytes) + '\t' + normalized_path + '\t' +
                        width + '\t' + height + '\t' + frame + '\t' + anchor + '\t' + transform +
                        '\n';
        }
        write_text_identical_or_new(assets_root / "MANIFEST.tsv", manifest);
        std::cout << "已发布原始文件=" << contents.size() << "\t逻辑映射=" << kMappings.size()
                  << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "错误: " << error.what() << '\n';
        return 1;
    }
}
