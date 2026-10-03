#include "dungeon_village_prototype/asset_manifest.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

namespace dungeon_village_prototype {
namespace {

std::vector<std::string> split(const std::string &text, char delimiter) {
    std::vector<std::string> values;
    std::size_t begin = 0;
    while (begin <= text.size()) {
        const auto end = text.find(delimiter, begin);
        values.push_back(text.substr(begin, end == std::string::npos ? end : end - begin));
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }
    return values;
}

std::vector<int> parse_numbers(const std::string &text, std::size_t expected) {
    const auto fields = split(text, ',');
    if (fields.size() != expected) {
        throw std::runtime_error("素材清单中的坐标字段数量错误");
    }
    std::vector<int> result;
    result.reserve(fields.size());
    for (const auto &field : fields) {
        std::size_t consumed = 0;
        const auto value = std::stoi(field, &consumed);
        if (consumed != field.size()) {
            throw std::runtime_error("素材清单中的坐标不是整数");
        }
        result.push_back(value);
    }
    return result;
}

} // namespace

AssetManifest load_asset_manifest(const std::filesystem::path &asset_root) {
    std::ifstream stream(asset_root / "MANIFEST.tsv");
    if (!stream) {
        throw std::runtime_error("无法读取素材清单");
    }
    std::string line;
    if (!std::getline(stream, line) || line.find("logical_key\t") != 0) {
        throw std::runtime_error("素材清单表头无效");
    }

    AssetManifest result;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto fields = split(line, '\t');
        if (fields.size() != 10) {
            throw std::runtime_error("素材清单列数无效");
        }
        if (fields[0].empty()) {
            continue;
        }
        const auto frame = parse_numbers(fields[7], 4);
        const auto anchor = parse_numbers(fields[8], 2);
        const std::filesystem::path relative = fields[4];
        if (relative.empty() || relative.is_absolute()) {
            throw std::runtime_error("素材路径必须是根目录内的相对路径");
        }
        for (const auto &part : relative) {
            if (part == "..") {
                throw std::runtime_error("素材路径不能包含父目录");
            }
        }
        if (frame[0] < 0 || frame[1] < 0 || frame[2] <= 0 || frame[3] <= 0) {
            throw std::runtime_error("素材帧矩形必须非负且尺寸为正");
        }
        const auto path = asset_root / fields[4];
        if (!std::filesystem::is_regular_file(path)) {
            throw std::runtime_error("规范化素材不存在: " + path.string());
        }
        const AssetDefinition definition{
            path, {frame[0], frame[1], frame[2], frame[3]}, {anchor[0], anchor[1]}};
        if (!result.emplace(fields[0], definition).second) {
            throw std::runtime_error("素材逻辑键重复: " + fields[0]);
        }
    }
    return result;
}

} // namespace dungeon_village_prototype
