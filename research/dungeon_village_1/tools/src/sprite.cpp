// Legacy big-endian SEB structure only; unsupported compressed streams and trailing bytes are
// rejected. Package responsibilities and evidence boundaries: ../README.md.

#include "dungeon_village_tools/sprite.hpp"

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace dungeon_village_tools {
namespace {

constexpr std::size_t kMaxLayers = 4096;
constexpr std::size_t kMaxParts = 100000;

class Reader {
  public:
    explicit Reader(const std::vector<std::uint8_t> &bytes) : bytes_(bytes) {}

    std::uint8_t read_u8() {
        require(1);
        return bytes_[position_++];
    }

    std::uint16_t read_u16() {
        const auto high = read_u8();
        const auto low = read_u8();
        return static_cast<std::uint16_t>((static_cast<std::uint16_t>(high) << 8U) | low);
    }

    std::int16_t read_i16() { return static_cast<std::int16_t>(read_u16()); }

    std::size_t remaining() const { return bytes_.size() - position_; }

  private:
    void require(std::size_t size) const {
        if (size > remaining()) {
            throw std::runtime_error("SEB 文件被截断");
        }
    }

    const std::vector<std::uint8_t> &bytes_;
    std::size_t position_{};
};

std::size_t checked_count(std::int16_t value, std::size_t maximum, const char *field) {
    if (value < 0 || static_cast<std::size_t>(value) > maximum) {
        throw std::runtime_error(std::string("SEB ") + field + "非法");
    }
    return static_cast<std::size_t>(value);
}

} // namespace

SpriteDefinition parse_legacy_seb(const std::vector<std::uint8_t> &bytes) {
    Reader reader(bytes);
    const auto first = reader.read_u8();
    if ((first & 0x80U) != 0U) {
        throw std::runtime_error("当前提取器不支持压缩 SEB 格式");
    }

    const auto layer_count_value =
        static_cast<std::int16_t>((static_cast<std::uint16_t>(first) << 8U) | reader.read_u8());
    const auto layer_count = checked_count(layer_count_value, kMaxLayers, "图层数量");
    const auto frame_count_value = reader.read_i16();
    const auto frame_count =
        checked_count(frame_count_value, std::numeric_limits<std::uint16_t>::max(), "帧数量");

    SpriteDefinition sprite;
    sprite.frame_count = static_cast<std::uint16_t>(frame_count);
    sprite.layers.reserve(layer_count);
    std::size_t total_parts = 0;
    for (std::size_t layer_index = 0; layer_index < layer_count; ++layer_index) {
        const auto part_count = checked_count(reader.read_i16(), kMaxParts, "图层记录数量");
        // The Java reader discards this field; retain its bits without inferring a record count.
        const auto legacy_tag = reader.read_u16();
        if (part_count > kMaxParts - total_parts) {
            throw std::runtime_error("SEB 总记录数量非法");
        }
        total_parts += part_count;

        SpriteLayer layer;
        layer.legacy_tag = legacy_tag;
        layer.parts.reserve(part_count);
        for (std::size_t part_index = 0; part_index < part_count; ++part_index) {
            SpritePart part;
            part.frame = reader.read_i16();
            part.image_index = reader.read_i16();
            part.source_x = reader.read_i16();
            part.source_y = reader.read_i16();
            part.width = reader.read_i16();
            part.height = reader.read_i16();
            part.offset_x = reader.read_i16();
            part.offset_y = reader.read_i16();
            part.flip_x = reader.read_i16();
            part.flip_y = reader.read_i16();
            layer.parts.push_back(part);
        }
        sprite.layers.push_back(std::move(layer));
    }

    if (reader.remaining() != 0) {
        throw std::runtime_error("SEB 文件存在未解析尾随字节");
    }
    return sprite;
}

} // namespace dungeon_village_tools
