#pragma once

#include <filesystem>
#include <map>
#include <string>

namespace dungeon_village_prototype {

struct Rectangle {
    int x{};
    int y{};
    int width{};
    int height{};
};

struct Point {
    int x{};
    int y{};
};

struct AssetDefinition {
    std::filesystem::path path;
    Rectangle frame;
    Point anchor;
};

using AssetManifest = std::map<std::string, AssetDefinition>;

AssetManifest load_asset_manifest(const std::filesystem::path &asset_root);

} // namespace dungeon_village_prototype
