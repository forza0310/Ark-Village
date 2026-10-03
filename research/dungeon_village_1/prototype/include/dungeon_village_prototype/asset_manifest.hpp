#pragma once

// Logical-key asset contract for replacements; this is not the full APK image/SEB index loader.

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

// Read active logical rows, reject unsafe/missing paths and duplicate keys; pixel bounds are
// checked on texture load.
AssetManifest load_asset_manifest(const std::filesystem::path &asset_root);

} // namespace dungeon_village_prototype
