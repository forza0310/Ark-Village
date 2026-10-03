#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace dungeon_village_tools {

struct ArchiveEntry {
    std::string name;
    std::uint32_t declared_size{};
    std::uint8_t flags{};
    std::vector<std::uint8_t> data;
};

struct Archive {
    std::uint32_t format_tag{};
    std::vector<ArchiveEntry> entries;

    const ArchiveEntry *find(const std::string &name) const;
};

struct PngSize {
    std::uint32_t width{};
    std::uint32_t height{};
};

std::vector<std::uint8_t> read_binary_file(const std::filesystem::path &path);
void xor_transform(std::vector<std::uint8_t> &bytes, const std::vector<std::uint8_t> &key);
const std::vector<std::uint8_t> &game_asset_key();

std::uint32_t crc32(const std::uint8_t *data, std::size_t size);
std::uint32_t crc32(const std::vector<std::uint8_t> &data);
std::uint32_t crc32(const std::string &text);
std::uint32_t game_crc32(const std::uint8_t *data, std::size_t size);
std::uint32_t game_crc32(const std::vector<std::uint8_t> &data);
std::uint32_t game_crc32(const std::string &text);
std::string sha256_hex(const std::uint8_t *data, std::size_t size);
std::string sha256_hex(const std::vector<std::uint8_t> &data);
std::string sha256_hex(const std::string &text);
std::optional<std::uint32_t> expected_encrypted_crc(const std::string &asset_name);

Archive parse_archive(const std::vector<std::uint8_t> &decoded);
Archive decode_game_archive(const std::string &asset_name,
                            const std::vector<std::uint8_t> &encrypted);

bool is_safe_relative_path(const std::string &name);
bool is_visual_archive(const Archive &archive);
bool is_visual_entry(const ArchiveEntry &entry);
std::size_t extract_visual_entries(const Archive &archive,
                                   const std::filesystem::path &output_directory);

std::optional<PngSize> read_png_size(const std::vector<std::uint8_t> &data);

} // namespace dungeon_village_tools
