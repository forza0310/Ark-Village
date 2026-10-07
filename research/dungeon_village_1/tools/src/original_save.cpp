// 原格式来源和限制见../ORIGINAL_SAVE.md。独立只读实现，不执行原加载修复。
#include "dungeon_village_tools/original_save.hpp"
#include "dungeon_village_tools/archive.hpp"

#include <array>
#include <fstream>
#include <map>
#include <stdexcept>

namespace dungeon_village_tools {
namespace {
constexpr std::size_t max_bytes = 16U * 1024U * 1024U;
constexpr std::size_t max_fields = 131072;
constexpr unsigned max_depth = 32;

std::vector<std::uint8_t> decode_base64(const std::vector<std::uint8_t> &input) {
    std::string text;
    for (const auto c : input) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue;
        text.push_back(static_cast<char>(c));
    }
    if (text.size() % 4 != 0)
        throw std::runtime_error("Base64长度不是4的倍数");
    const auto digit = [](char c) -> unsigned {
        if (c >= 'A' && c <= 'Z')
            return static_cast<unsigned>(c - 'A');
        if (c >= 'a' && c <= 'z')
            return static_cast<unsigned>(c - 'a' + 26);
        if (c >= '0' && c <= '9')
            return static_cast<unsigned>(c - '0' + 52);
        if (c == '+')
            return 62;
        if (c == '/')
            return 63;
        throw std::runtime_error("非法Base64字符");
    };
    std::vector<std::uint8_t> result;
    result.reserve(text.size() / 4 * 3);
    for (std::size_t i = 0; i < text.size(); i += 4) {
        const auto a = digit(text[i]), b = digit(text[i + 1]);
        const bool pad2 = text[i + 2] == '=', pad3 = text[i + 3] == '=';
        if ((pad2 && !pad3) || ((pad2 || pad3) && i + 4 != text.size()))
            throw std::runtime_error("非法Base64填充位置");
        const auto c = pad2 ? 0U : digit(text[i + 2]);
        const auto d = pad3 ? 0U : digit(text[i + 3]);
        if ((pad2 && (b & 15U)) || (pad3 && !pad2 && (c & 3U)))
            throw std::runtime_error("Base64未使用位不为零");
        result.push_back(static_cast<std::uint8_t>((a << 2U) | (b >> 4U)));
        if (!pad2)
            result.push_back(static_cast<std::uint8_t>((b << 4U) | (c >> 2U)));
        if (!pad3)
            result.push_back(static_cast<std::uint8_t>((c << 6U) | d));
    }
    if (result.size() >= 2 && result[0] == 0x1f && result[1] == 0x8b)
        throw std::runtime_error("原Base64读器支持的GZIP分支尚未接入本工具");
    return result;
}

class ContainerReader {
  public:
    ContainerReader(const std::vector<std::uint8_t> &bytes, std::vector<OriginalSaveField> &fields,
                    bool steam)
        : bytes_(bytes), fields_(fields), steam_(steam) {}

    void parse(std::size_t begin, std::size_t end, const std::string &path, unsigned depth) {
        if (depth > max_depth)
            fail("嵌套深度超限", begin);
        std::size_t at = begin;
        const auto groups = count(at, end);
        if (groups > 5)
            fail("标签组数量超限", begin);
        std::array<bool, 5> seen{};
        for (std::size_t group = 0; group < groups; ++group) {
            const auto tag_offset = at;
            const auto tag = count(at, end);
            if (tag > 4)
                fail("未知标签没有整组长度，不能跳过", tag_offset);
            if (seen[tag])
                fail("重复标签", tag_offset);
            seen[tag] = true;
            const auto n = count(at, end);
            const auto group_path = path + "/" + std::to_string(tag);
            add({group_path, "group", tag_offset, 8, std::to_string(n)});
            if (n > max_fields - fields_.size())
                fail("全局字段数量超限", at);
            const auto minimum = tag == 2 ? 8U : 4U;
            if (n > (end - at) / minimum)
                fail("字段数量超过剩余载荷", at);
            for (std::size_t index = 0; index < n; ++index) {
                const auto field_path = group_path + "/" + std::to_string(index);
                if (tag == 1 || tag == 2) {
                    const auto offset = at;
                    const auto width = tag == 1 ? 4U : 8U;
                    const auto raw = integer(at, end, width);
                    const auto value =
                        width == 4
                            ? (raw <= 0x7fffffffULL
                                   ? static_cast<std::int64_t>(raw)
                                   : static_cast<std::int64_t>(raw) - 0x100000000LL)
                            : (raw <= 0x7fffffffffffffffULL ? static_cast<std::int64_t>(raw)
                                                            : -1 - static_cast<std::int64_t>(~raw));
                    add({field_path, tag == 1 ? "int32" : "int64", offset, width,
                         std::to_string(value)});
                } else {
                    const auto raw_length = integer(at, end, 4);
                    // Steam的byte[]允许-1表示null；APK没有此协议，不能把null当空数组。
                    if (steam_ && tag == 4 && raw_length == 0xffffffffULL) {
                        add({field_path, "null-bytes", at, 0, "null"});
                        continue;
                    }
                    if (raw_length > 0x7fffffffULL)
                        fail("负载荷长度", at - 4);
                    const auto length = static_cast<std::size_t>(raw_length);
                    if (length > end - at)
                        fail("载荷截断", at);
                    if (tag == 0) {
                        // 子容器在同一缓冲区内递归，预算共享；不复制子树载荷。
                        add({field_path, "container", at, length, ""});
                        parse(at, at + length, field_path, depth + 1);
                    } else if (tag == 3) {
                        static constexpr char hex[] = "0123456789abcdef";
                        std::string value;
                        value.reserve(length * 2);
                        for (std::size_t j = 0; j < length; ++j) {
                            value += hex[bytes_[at + j] >> 4U];
                            value += hex[bytes_[at + j] & 15U];
                        }
                        add({field_path, "string-bytes", at, length, std::move(value)});
                    } else {
                        add({field_path, "opaque-bytes", at, length,
                             sha256_hex(bytes_.data() + at, length)});
                    }
                    at += length;
                }
            }
        }
        if (at != end)
            fail("容器存在尾字节", at);
    }

  private:
    [[noreturn]] static void fail(const std::string &why, std::size_t offset) {
        throw std::runtime_error(why + "；容器偏移=" + std::to_string(offset));
    }
    std::uint64_t integer(std::size_t &at, std::size_t end, unsigned width) const {
        if (width > end - at)
            fail("整数截断", at);
        std::uint64_t value{};
        for (unsigned i = 0; i < width; ++i)
            value = (value << 8U) | bytes_[at++];
        return value;
    }
    std::size_t count(std::size_t &at, std::size_t end) const {
        const auto value = integer(at, end, 4);
        if (value > 0x7fffffffULL)
            fail("负数量或长度", at - 4);
        return static_cast<std::size_t>(value);
    }
    void add(OriginalSaveField field) {
        if (fields_.size() == max_fields)
            fail("全局字段数量超限", field.offset);
        fields_.push_back(std::move(field));
    }
    const std::vector<std::uint8_t> &bytes_;
    std::vector<OriginalSaveField> &fields_;
    bool steam_{};
};
} // namespace

OriginalSaveInspection inspect_original_save(const std::vector<std::uint8_t> &input,
                                             OriginalSaveFormat format,
                                             std::optional<std::uint64_t> steam_id) {
    if (input.size() > max_bytes)
        throw std::runtime_error("输入超过16MiB预算");
    if (format != OriginalSaveFormat::container && format != OriginalSaveFormat::apk_record &&
        format != OriginalSaveFormat::apk_base64 && format != OriginalSaveFormat::steam_record)
        throw std::runtime_error("未知输入格式");
    if ((format == OriginalSaveFormat::steam_record) != steam_id.has_value())
        throw std::runtime_error("Steam记录必须且仅能配合显式SteamID解析");
    OriginalSaveInspection result;
    result.input_sha256 = sha256_hex(input);
    auto decoded = format == OriginalSaveFormat::apk_base64 ? decode_base64(input) : input;
    if (format != OriginalSaveFormat::container) {
        if (decoded.empty()) {
            result.empty_record = true;
            return result; // 空记录不同于有效新局，不能构造替代世界。
        }
        if (decoded.size() < 8)
            throw std::runtime_error("记录校验头截断");
        if (steam_id) {
            std::vector<std::uint8_t> key;
            for (unsigned i = 0; i < 8; ++i)
                key.push_back(static_cast<std::uint8_t>(*steam_id >> (i * 8U)));
            xor_transform(decoded, key);
        } else
            xor_transform(decoded, game_asset_key());
        std::uint64_t expected{};
        for (unsigned i = 0; i < 8; ++i)
            expected = (expected << 8U) | decoded[i];
        if (expected != game_crc32(decoded.data() + 8, decoded.size() - 8))
            throw std::runtime_error("原记录游戏校验不匹配");
        result.container_bytes.assign(decoded.begin() + 8, decoded.end());
    } else
        result.container_bytes = std::move(decoded);
    ContainerReader reader(result.container_bytes, result.fields,
                           format == OriginalSaveFormat::steam_record);
    reader.parse(0, result.container_bytes.size(), "$", 0);
    return result;
}

OriginalSaveInspection inspect_original_save_file(const std::filesystem::path &file,
                                                  OriginalSaveFormat format,
                                                  std::optional<std::uint64_t> steam_id) {
    std::ifstream stream(file, std::ios::binary | std::ios::ate);
    if (!stream)
        throw std::runtime_error("无法只读打开存档");
    const auto length = stream.tellg();
    if (length < 0 || static_cast<std::uintmax_t>(length) > max_bytes)
        throw std::runtime_error("存档文件大小超出预算");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    stream.seekg(0);
    if (!bytes.empty())
        stream.read(reinterpret_cast<char *>(bytes.data()), length);
    if (!stream || stream.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("文件截断或读取期间长度发生变化");
    return inspect_original_save(bytes, format, steam_id);
}

std::vector<std::string> diff_original_saves(const OriginalSaveInspection &before,
                                             const OriginalSaveInspection &after) {
    std::map<std::string, const OriginalSaveField *> left, right;
    for (const auto &field : before.fields)
        left.emplace(field.path, &field);
    for (const auto &field : after.fields)
        right.emplace(field.path, &field);
    std::vector<std::string> result;
    if (before.empty_record != after.empty_record)
        result.push_back("$\tempty-record-changed");
    for (const auto &[path, field] : left) {
        const auto found = right.find(path);
        if (found == right.end())
            result.push_back(path + "\tremoved");
        else if (field->type != found->second->type || field->value != found->second->value ||
                 (field->type != "container" && field->size != found->second->size))
            result.push_back(path + "\tchanged\t" + field->value + "\t" + found->second->value);
    }
    for (const auto &[path, field] : right) {
        (void)field;
        if (!left.count(path))
            result.push_back(path + "\tadded");
    }
    return result;
}
} // namespace dungeon_village_tools
