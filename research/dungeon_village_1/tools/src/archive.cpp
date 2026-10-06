// Strict archive parsing and visual extraction; malformed or unsupported input raises exceptions.
// Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_tools/archive.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>

namespace dungeon_village_tools {
namespace {

constexpr std::size_t kMaxEntries = 100000;
constexpr std::size_t kMaxNameLength = 4096;

class Reader {
  public:
    explicit Reader(const std::vector<std::uint8_t> &bytes) : bytes_(bytes) {}

    std::uint8_t read_u8() {
        require(1);
        return bytes_[position_++];
    }

    std::uint32_t read_u32() {
        require(4);
        const std::uint32_t result = (static_cast<std::uint32_t>(bytes_[position_]) << 24U) |
                                     (static_cast<std::uint32_t>(bytes_[position_ + 1]) << 16U) |
                                     (static_cast<std::uint32_t>(bytes_[position_ + 2]) << 8U) |
                                     static_cast<std::uint32_t>(bytes_[position_ + 3]);
        position_ += 4;
        return result;
    }

    std::vector<std::uint8_t> read_bytes(std::size_t size) {
        require(size);
        const auto begin = bytes_.begin() + static_cast<std::ptrdiff_t>(position_);
        position_ += size;
        return {begin, begin + static_cast<std::ptrdiff_t>(size)};
    }

    std::size_t remaining() const { return bytes_.size() - position_; }

  private:
    void require(std::size_t size) const {
        if (size > remaining()) {
            throw std::runtime_error("归档被截断");
        }
    }

    const std::vector<std::uint8_t> &bytes_;
    std::size_t position_{};
};

std::uint32_t read_u32_at(const std::vector<std::uint8_t> &bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("归档条目长度字段越界");
    }
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U) |
           static_cast<std::uint32_t>(bytes[offset + 3]);
}

std::string lowercase(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    return text;
}

bool has_suffix(const std::string &text, const std::string &suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const std::array<std::pair<std::uint32_t, std::uint32_t>, 15> kKnownChecksums = {{
    {3967613572U, 2503629547U},
    {580282563U, 3679220737U},
    {2127962609U, 3503018438U},
    {1416911673U, 2659212157U},
    {3277703196U, 725607681U},
    {491867833U, 1879026245U},
    {2928711961U, 1224065020U},
    {1348013988U, 1057868729U},
    {199728330U, 3317227465U},
    {2008104103U, 378607304U},
    {1766122277U, 581703137U},
    {1971932774U, 505776908U},
    {2612373199U, 3400654849U},
    {112310729U, 3820794699U},
    {3309258413U, 3541973628U},
}};

void write_atomically(const std::filesystem::path &path, const std::vector<std::uint8_t> &data) {
    if (std::filesystem::exists(path)) {
        if (read_binary_file(path) == data) {
            return;
        }
        throw std::runtime_error("目标文件已存在且内容不同: " + path.string());
    }

    std::filesystem::create_directories(path.parent_path());
    const auto temporary = path.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            throw std::runtime_error("无法创建临时文件: " + temporary);
        }
        stream.write(reinterpret_cast<const char *>(data.data()),
                     static_cast<std::streamsize>(data.size()));
        if (!stream) {
            throw std::runtime_error("写入临时文件失败: " + temporary);
        }
    }

    try {
        std::filesystem::rename(temporary, path);
    } catch (...) {
        std::filesystem::remove(temporary);
        throw;
    }
}

} // namespace

const ArchiveEntry *Archive::find(const std::string &name) const {
    const auto wanted = lowercase(name);
    for (const auto &entry : entries) {
        if (lowercase(entry.name) == wanted) {
            return &entry;
        }
    }
    return nullptr;
}

std::vector<std::uint8_t> read_binary_file(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw std::runtime_error("无法读取文件: " + path.string());
    }
    const auto end = stream.tellg();
    if (end < 0) {
        throw std::runtime_error("无法取得文件大小: " + path.string());
    }
    const auto size = static_cast<std::uintmax_t>(end);
    if (size > static_cast<std::uintmax_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::runtime_error("文件过大: " + path.string());
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!stream && !bytes.empty()) {
        throw std::runtime_error("读取文件失败: " + path.string());
    }
    return bytes;
}

void xor_transform(std::vector<std::uint8_t> &bytes, const std::vector<std::uint8_t> &key) {
    if (key.empty()) {
        throw std::invalid_argument("XOR 密钥不能为空");
    }
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] ^= key[index % key.size()];
    }
}

const std::vector<std::uint8_t> &game_asset_key() {
    static const std::vector<std::uint8_t> key = [] {
        constexpr std::array<std::int32_t, 11> words = {
            -1387743643, 321849466,   -380916995,  1114766278, 1209944503, 138008561,
            -893766998,  -1242421477, -1230126924, 626883230,  1684377624,
        };
        std::vector<std::uint8_t> result;
        result.reserve(words.size() * 4);
        for (const auto signed_word : words) {
            const auto word = static_cast<std::uint32_t>(signed_word);
            result.push_back(static_cast<std::uint8_t>(word & 0xffU));
            result.push_back(static_cast<std::uint8_t>((word >> 8U) & 0xffU));
            result.push_back(static_cast<std::uint8_t>((word >> 16U) & 0xffU));
            result.push_back(static_cast<std::uint8_t>((word >> 24U) & 0xffU));
        }
        return result;
    }();
    return key;
}

std::uint32_t crc32(const std::uint8_t *data, std::size_t size) {
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1U) ^ ((crc & 1U) != 0U ? 0xedb88320U : 0U);
        }
    }
    return crc ^ 0xffffffffU;
}

std::uint32_t crc32(const std::vector<std::uint8_t> &data) {
    return crc32(data.data(), data.size());
}

std::uint32_t crc32(const std::string &text) {
    return crc32(reinterpret_cast<const std::uint8_t *>(text.data()), text.size());
}

std::uint32_t game_crc32(const std::uint8_t *data, std::size_t size) {
    std::uint32_t crc = 0xffffffffU;
    for (std::size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1U) ^ ((crc & 1U) == 0U ? 0xedb88320U : 0U);
        }
    }
    return crc ^ 0xffffffffU;
}

std::uint32_t game_crc32(const std::vector<std::uint8_t> &data) {
    return game_crc32(data.data(), data.size());
}

std::uint32_t game_crc32(const std::string &text) {
    return game_crc32(reinterpret_cast<const std::uint8_t *>(text.data()), text.size());
}

std::optional<std::uint32_t> expected_encrypted_crc(const std::string &asset_name) {
    const auto name_crc = game_crc32(asset_name);
    for (const auto &[known_name_crc, known_content_crc] : kKnownChecksums) {
        if (known_name_crc == name_crc) {
            return known_content_crc;
        }
    }
    return std::nullopt;
}

Archive parse_archive(const std::vector<std::uint8_t> &decoded) {
    Reader reader(decoded);
    Archive archive;
    archive.format_tag = reader.read_u32();
    const auto payload_size = reader.read_u32();
    const auto entry_count = reader.read_u32();
    if (entry_count > kMaxEntries) {
        throw std::runtime_error("归档条目数量非法");
    }

    std::vector<std::string> names;
    names.reserve(entry_count);
    std::set<std::string> normalized_names;
    for (std::uint32_t index = 0; index < entry_count; ++index) {
        const auto name_size = reader.read_u32();
        if (name_size == 0 || name_size > kMaxNameLength) {
            throw std::runtime_error("归档条目名称长度非法");
        }
        const auto name_bytes = reader.read_bytes(name_size);
        const std::string name(name_bytes.begin(), name_bytes.end());
        if (!normalized_names.insert(lowercase(name)).second) {
            throw std::runtime_error("归档包含重复条目: " + name);
        }
        names.push_back(name);
    }

    std::vector<std::uint32_t> offsets(entry_count);
    std::vector<std::uint32_t> declared_sizes(entry_count);
    std::vector<std::uint8_t> flags(entry_count);
    for (auto &offset : offsets) {
        offset = reader.read_u32();
    }
    for (auto &declared_size : declared_sizes) {
        declared_size = reader.read_u32();
    }
    for (auto &flag : flags) {
        flag = reader.read_u8();
    }

    if (reader.remaining() != payload_size) {
        throw std::runtime_error("归档载荷大小与头部不一致");
    }
    // Offsets address this payload, whose entries have their own length prefix, not the archive
    // start.
    const auto payload = reader.read_bytes(payload_size);

    archive.entries.reserve(entry_count);
    for (std::size_t index = 0; index < entry_count; ++index) {
        const auto offset = static_cast<std::size_t>(offsets[index]);
        const auto embedded_size = static_cast<std::size_t>(read_u32_at(payload, offset));
        if (embedded_size > payload.size() - offset - 4) {
            throw std::runtime_error("归档条目载荷越界: " + names[index]);
        }
        if ((flags[index] & 1U) != 0U && declared_sizes[index] > 0U) {
            throw std::runtime_error("归档条目使用未支持的压缩: " + names[index]);
        }
        const auto begin = payload.begin() + static_cast<std::ptrdiff_t>(offset + 4);
        archive.entries.push_back({names[index],
                                   declared_sizes[index],
                                   flags[index],
                                   {begin, begin + static_cast<std::ptrdiff_t>(embedded_size)}});
    }
    return archive;
}

Archive decode_game_archive(const std::string &asset_name,
                            const std::vector<std::uint8_t> &encrypted) {
    if (const auto expected = expected_encrypted_crc(asset_name);
        expected.has_value() && game_crc32(encrypted) != *expected) {
        throw std::runtime_error("资源校验值不匹配: " + asset_name);
    }
    auto decoded = encrypted;
    xor_transform(decoded, game_asset_key());
    return parse_archive(decoded);
}

bool is_safe_relative_path(const std::string &name) {
    if (name.empty()) {
        return false;
    }
    const std::filesystem::path path(name);
    // Windows的/tmp是当前盘根路径，is_absolute()为false，仍不能写入输出目录之外。
    if (path.has_root_path()) {
        return false;
    }
    for (const auto &part : path) {
        if (part == ".." || part == "." || part.empty()) {
            return false;
        }
    }
    return true;
}

bool is_visual_archive(const Archive &archive) { return archive.find("img.inf") != nullptr; }

bool is_visual_entry(const ArchiveEntry &entry) {
    const auto name = lowercase(entry.name);
    constexpr std::array<const char *, 8> audio_suffixes = {
        ".mld", ".mmf", ".spf", ".mid", ".wav", ".jet", ".ogg", "snd.inf",
    };
    for (const auto *suffix : audio_suffixes) {
        if (has_suffix(name, suffix)) {
            return false;
        }
    }
    return true;
}

std::size_t extract_visual_entries(const Archive &archive,
                                   const std::filesystem::path &output_directory) {
    if (!is_visual_archive(archive)) {
        return 0;
    }

    std::vector<const ArchiveEntry *> selected;
    for (const auto &entry : archive.entries) {
        if (!is_visual_entry(entry)) {
            continue;
        }
        if (!is_safe_relative_path(entry.name)) {
            throw std::runtime_error("拒绝不安全的归档路径: " + entry.name);
        }
        selected.push_back(&entry);
    }

    for (const auto *entry : selected) {
        write_atomically(output_directory / std::filesystem::path(entry->name), entry->data);
    }
    return selected.size();
}

std::optional<PngSize> read_png_size(const std::vector<std::uint8_t> &data) {
    constexpr std::array<std::uint8_t, 8> signature = {137, 80, 78, 71, 13, 10, 26, 10};
    if (data.size() < 24 || !std::equal(signature.begin(), signature.end(), data.begin()) ||
        data[12] != 'I' || data[13] != 'H' || data[14] != 'D' || data[15] != 'R') {
        return std::nullopt;
    }
    const auto width = read_u32_at(data, 16);
    const auto height = read_u32_at(data, 20);
    if (width == 0 || height == 0) {
        return std::nullopt;
    }
    return PngSize{width, height};
}

} // namespace dungeon_village_tools
