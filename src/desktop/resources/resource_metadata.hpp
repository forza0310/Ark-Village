#pragma once
// Private parsing/bounds helpers shared by runtime sprites and CPU asset validation.
#include "ark/assets/sprite.hpp"
#include <filesystem>
#include <map>
#include <vector>

namespace ark::desktop::resource_detail {
std::vector<std::uint8_t> read_bytes(const std::filesystem::path &path);
std::map<int, std::filesystem::path> image_index(const std::filesystem::path &root, const char *group = "image");
void validate(const assets::SpritePart &part, int width, int height);
int map_frame_index(const std::string &sprite, const assets::SpriteDefinition &data, int variant);
}
