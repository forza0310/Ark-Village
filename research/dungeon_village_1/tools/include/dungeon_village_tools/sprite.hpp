#pragma once

#include <cstdint>
#include <vector>

namespace dungeon_village_tools {

struct SpritePart {
    std::int16_t frame{};
    std::int16_t image_index{};
    std::int16_t source_x{};
    std::int16_t source_y{};
    std::int16_t width{};
    std::int16_t height{};
    std::int16_t offset_x{};
    std::int16_t offset_y{};
    std::int16_t flip_x{};
    std::int16_t flip_y{};
};

struct SpriteLayer {
    std::uint16_t legacy_tag{};
    std::vector<SpritePart> parts;
};

struct SpriteDefinition {
    std::uint16_t frame_count{};
    std::vector<SpriteLayer> layers;
};

SpriteDefinition parse_legacy_seb(const std::vector<std::uint8_t> &bytes);

} // namespace dungeon_village_tools
