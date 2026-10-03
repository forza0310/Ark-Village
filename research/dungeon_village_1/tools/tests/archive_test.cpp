#include "dungeon_village_tools/archive.hpp"
#include "dungeon_village_tools/sprite.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace dungeon_village_tools;

namespace {

int checks = 0;

void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition) {
        std::cerr << "失败: " << message << '\n';
        std::exit(1);
    }
}

template <typename Function> void check_throws(Function function, const std::string &message) {
    ++checks;
    try {
        function();
    } catch (const std::exception &) {
        return;
    }
    std::cerr << "失败: " << message << '\n';
    std::exit(1);
}

void append_u32(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
}

void append_i16(std::vector<std::uint8_t> &bytes, std::int16_t value) {
    const auto converted = static_cast<std::uint16_t>(value);
    bytes.push_back(static_cast<std::uint8_t>((converted >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>(converted & 0xffU));
}

struct FixtureEntry {
    std::string name;
    std::vector<std::uint8_t> data;
    std::uint8_t flags{};
};

std::vector<std::uint8_t> make_archive(const std::vector<FixtureEntry> &entries) {
    std::vector<std::uint8_t> payload;
    std::vector<std::uint32_t> offsets;
    for (const auto &entry : entries) {
        offsets.push_back(static_cast<std::uint32_t>(payload.size()));
        append_u32(payload, static_cast<std::uint32_t>(entry.data.size()));
        payload.insert(payload.end(), entry.data.begin(), entry.data.end());
    }

    std::vector<std::uint8_t> archive;
    append_u32(archive, 0x01020304U);
    append_u32(archive, static_cast<std::uint32_t>(payload.size()));
    append_u32(archive, static_cast<std::uint32_t>(entries.size()));
    for (const auto &entry : entries) {
        append_u32(archive, static_cast<std::uint32_t>(entry.name.size()));
        archive.insert(archive.end(), entry.name.begin(), entry.name.end());
    }
    for (const auto offset : offsets) {
        append_u32(archive, offset);
    }
    for (const auto &entry : entries) {
        append_u32(archive, static_cast<std::uint32_t>(entry.data.size()));
    }
    for (const auto &entry : entries) {
        archive.push_back(entry.flags);
    }
    archive.insert(archive.end(), payload.begin(), payload.end());
    return archive;
}

std::vector<std::uint8_t> tiny_png_header(std::uint32_t width, std::uint32_t height) {
    std::vector<std::uint8_t> bytes = {137, 80, 78, 71, 13,  10,  26,  10,
                                       0,   0,  0,  13, 'I', 'H', 'D', 'R'};
    append_u32(bytes, width);
    append_u32(bytes, height);
    return bytes;
}

void valid_archive_round_trip() {
    auto decoded = make_archive({{"img.inf", {'a', '\n'}},
                                 {"tiles.png", tiny_png_header(240, 120)},
                                 {"sound.ogg", {1, 2, 3}}});
    auto encrypted = decoded;
    xor_transform(encrypted, game_asset_key());
    xor_transform(encrypted, game_asset_key());
    check(encrypted == decoded, "XOR 两次恢复原文");

    xor_transform(encrypted, game_asset_key());
    auto restored = encrypted;
    xor_transform(restored, game_asset_key());
    const auto archive = parse_archive(restored);
    check(archive.entries.size() == 3, "解析全部条目");
    check(archive.find("IMG.INF") != nullptr, "名称查找不区分大小写");
    check(is_visual_archive(archive), "img.inf 标识视觉归档");
    check(is_visual_entry(*archive.find("tiles.png")), "PNG 是视觉条目");
    check(!is_visual_entry(*archive.find("sound.ogg")), "音频不进入视觉输出");
    const auto size = read_png_size(archive.find("tiles.png")->data);
    check(size.has_value() && size->width == 240 && size->height == 120, "读取 PNG 尺寸");
}

void malformed_inputs_are_rejected() {
    check_throws([] { parse_archive({0, 1, 2}); }, "拒绝截断头");

    auto duplicate = make_archive({{"same.png", {1}}, {"SAME.PNG", {2}}});
    check_throws([&] { parse_archive(duplicate); }, "拒绝重复条目");

    auto compressed = make_archive({{"img.inf", {1}, 1}});
    check_throws([&] { parse_archive(compressed); }, "拒绝压缩条目");

    auto wrong_payload_size = make_archive({{"img.inf", {1}}});
    wrong_payload_size[7]++;
    check_throws([&] { parse_archive(wrong_payload_size); }, "拒绝载荷大小不符");

    auto bad_offset = make_archive({{"img.inf", {1}}});
    const std::size_t offset_position = 12 + 4 + std::string("img.inf").size();
    bad_offset[offset_position] = 0x7f;
    check_throws([&] { parse_archive(bad_offset); }, "拒绝越界条目偏移");
    auto excessive_count = make_archive({{"img.inf", {1}}});
    excessive_count[8] = 0x7f;
    check_throws([&] { parse_archive(excessive_count); }, "拒绝超限条目数量");
    check_throws([] { decode_game_archive("image.dat", {1, 2, 3}); }, "拒绝固定资源校验不符");
}

void paths_and_crc_are_validated() {
    check(is_safe_relative_path("image/tiles.png"), "接受安全相对路径");
    check(!is_safe_relative_path("../tiles.png"), "拒绝父目录路径");
    check(!is_safe_relative_path("/tmp/tiles.png"), "拒绝绝对路径");
    check(crc32(std::string("123456789")) == 0xcbf43926U, "CRC32 标准向量");
    check(game_crc32(std::string("image.dat")) == 491867833U, "游戏资源名称校验向量");
    check(sha256_hex(std::string()) ==
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
          "SHA-256 空字符串标准向量");
    check(sha256_hex(std::string("abc")) ==
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
          "SHA-256 abc 标准向量");
    check(sha256_hex(std::string(1000, 'a')) ==
              "41edece42d63e8d9bf515a9ba6932e1c20cbc9f5a5d134645adb5db1b9737ea3",
          "SHA-256 多块标准向量");
    check(sha256_hex(std::vector<std::uint8_t>{}) == sha256_hex(std::string()),
          "SHA-256 空字节数组");
    check(expected_encrypted_crc("image.dat").has_value(), "固定资源存在校验值");
    check(!expected_encrypted_crc("unknown.dat").has_value(), "未知资源没有伪造校验值");
}

void seb_is_parsed_and_validated() {
    std::vector<std::uint8_t> bytes;
    append_i16(bytes, 1);
    append_i16(bytes, 2);
    append_i16(bytes, 2);
    append_i16(bytes, 6);
    for (std::int16_t frame = 0; frame < 2; ++frame) {
        append_i16(bytes, frame);
        append_i16(bytes, 3);
        append_i16(bytes, static_cast<std::int16_t>(frame * 30));
        append_i16(bytes, 4);
        append_i16(bytes, 30);
        append_i16(bytes, 29);
        append_i16(bytes, -15);
        append_i16(bytes, -20);
        append_i16(bytes, frame);
        append_i16(bytes, 0);
    }

    const auto sprite = parse_legacy_seb(bytes);
    check(sprite.frame_count == 2, "解析 SEB 帧数量");
    check(sprite.layers.size() == 1 && sprite.layers[0].parts.size() == 2, "解析 SEB 图层和记录");
    check(sprite.layers[0].legacy_tag == 6, "保留 SEB 未解释图层标签");
    const auto &part = sprite.layers[0].parts[1];
    check(part.frame == 1 && part.image_index == 3 && part.source_x == 30, "解析 SEB 图片和源矩形");
    check(part.offset_x == -15 && part.offset_y == -20 && part.flip_x == 1, "解析 SEB 偏移和翻转");

    auto truncated = bytes;
    truncated.pop_back();
    check_throws([&] { parse_legacy_seb(truncated); }, "拒绝截断 SEB");

    auto trailing = bytes;
    trailing.push_back(0);
    check_throws([&] { parse_legacy_seb(trailing); }, "拒绝 SEB 尾随字节");

    check_throws([] { parse_legacy_seb({0xff, 0, 0, 0}); }, "拒绝未实现的压缩 SEB");

    for (std::size_t size = 0; size < bytes.size(); ++size) {
        const std::vector<std::uint8_t> prefix(bytes.begin(), bytes.begin() + size);
        check_throws([&] { parse_legacy_seb(prefix); }, "拒绝任意位置截断 SEB");
    }
    auto negative_frames = bytes;
    negative_frames[2] = 0xff;
    check_throws([&] { parse_legacy_seb(negative_frames); }, "拒绝负帧数量");
    auto negative_parts = bytes;
    negative_parts[4] = 0xff;
    check_throws([&] { parse_legacy_seb(negative_parts); }, "拒绝负记录数量");
    auto unknown_tag = bytes;
    unknown_tag[6] = 0xff;
    unknown_tag[7] = 0xff;
    check(parse_legacy_seb(unknown_tag).layers[0].legacy_tag == 0xffffU,
          "未知 legacy_tag 按原始 16 位保留，不当作数量");
}

} // namespace

int main() {
    valid_archive_round_trip();
    malformed_inputs_are_rejected();
    paths_and_crc_are_validated();
    seb_is_parsed_and_validated();
    std::cout << checks << " 项检查通过\n";
    return 0;
}
