#include "resource_metadata.hpp"
#include "ark/assets/table.hpp"
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace ark::desktop::resource_detail {
std::vector<std::uint8_t> read_bytes(const std::filesystem::path &path) {
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("Asset metadata too large");
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot read asset metadata: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::map<int, std::filesystem::path> image_index(const std::filesystem::path &root,
                                                 const char *group) {
    std::map<int, std::filesystem::path> result;
    for (const auto &row : assets::parse_tsv(read_bytes(root / group / "img.inf"))) {
        const bool ordinal = std::string(group) == "title";
        if (row.size() != (ordinal ? 1U : 2U))
            throw std::runtime_error("Invalid image index row");
        auto name = std::filesystem::path(row.back());
        if (name.has_parent_path() || name.is_absolute())
            throw std::runtime_error("Unsafe image path");
        name.replace_extension(".png");
        const int id =
            ordinal ? static_cast<int>(result.size()) : assets::parse_table_integer(row[0]);
        if (!result.emplace(id, name).second)
            throw std::runtime_error("Duplicate image index");
    }
    return result;
}
void validate(const assets::SpritePart &p, int width, int height) {
    if (p.image_index < 0 || p.source_x < 0 || p.source_y < 0 || p.width <= 0 || p.height <= 0 ||
        p.source_x + p.width > width || p.source_y + p.height > height || p.flip_x < 0 ||
        p.flip_x > 1 || p.flip_y < 0 || p.flip_y > 1)
        throw std::runtime_error("Unsupported sprite command or invalid image rectangle/flip");
}
int map_frame_index(const std::string &sprite, const assets::SpriteDefinition &data, int variant) {
    // 2b479f6 PAGES: map layers outside their source frame range return null and do not draw.
    // A logical second orientation must not become the single resource's frame0.
    if (variant < 0)
        throw std::runtime_error("Map sprite frame outside source: " + sprite +
                                 " variant=" + std::to_string(variant) +
                                 " frames=" + std::to_string(data.frame_count));
    return variant;
}
} // namespace ark::desktop::resource_detail
