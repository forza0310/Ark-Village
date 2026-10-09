#include "dungeon_village_prototype/startup_system_records.hpp"
#include "dungeon_village_tools/archive.hpp"
#include "startup_world_file_io.hpp"
#include "startup_persistence_bytes.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace dungeon_village_prototype {
namespace {
using Bytes = std::vector<std::uint8_t>;
constexpr std::size_t file_budget = 4U * 1024U * 1024U;
constexpr std::size_t opaque_budget = 1024U * 1024U, text_budget = 4096, vector_budget = 4096;
constexpr char magic[] = "AVRSYS01";
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void valid(const StartupSystemRecords &r) {
    require(r.last_slot >= 0 && r.last_slot <= 1, "系统记录栏位非法");
    require(r.high_score >= 0 && r.cash_peak >= 0 && r.trophy >= 0 && r.trophy <= 4,
            "系统记录数值或奖杯档位非法");
    require(r.score_village.size() <= text_budget && r.cash_village.size() <= text_budget,
            "系统记录村名超预算");
    require(valid_startup_world_human_profile({r.score_village, 0, false}) &&
                valid_startup_world_human_profile({r.cash_village, 0, false}),
            "系统记录村名编码或控制字符非法");
    require(r.facility_levels.size() <= vector_budget && r.profession_status.size() <= vector_budget,
            "系统记录继承列表超预算");
    require(r.facility_levels.size() % 2 == 0 && r.profession_status.size() % 2 == 0,
            "系统记录继承short字节串长度为奇数");
    require(r.opaque_sections.size() <= 60, "系统记录可选分区过多");
    std::set<std::uint32_t> ids;
    // 固定头／两必需段／字段长度／整体摘要的实际编码开销，不将payload预算冒充文件预算。
    std::size_t total = 292 + std::string(startup_world_persistence_dataset()).size() +
                        r.score_village.size() + r.cash_village.size() +
                        r.facility_levels.size() + r.profession_status.size();
    for (const auto &s : r.opaque_sections) {
        require(s.id >= 1024 && s.version > 0 && ids.insert(s.id).second,
                "系统记录可选分区身份非法或重复");
        require(s.bytes.size() <= opaque_budget && total <= file_budget &&
                    s.bytes.size() + 84 <= file_budget - total,
                "系统记录可选载荷超预算");
        total += s.bytes.size() + 84;
    }
}
struct Writer {
    Bytes bytes;
    void raw(const void *p, std::size_t n) {
        require(bytes.size() <= file_budget && n <= file_budget - bytes.size(), "系统文件超预算");
        if (n) {
            const auto *first = static_cast<const std::uint8_t *>(p);
            bytes.insert(bytes.end(), first, first + n);
        }
    }
    void u32(std::uint32_t n) {
        for (int i = 0; i < 4; ++i) {
            const auto b = static_cast<std::uint8_t>(n >> (8 * i));
            raw(&b, 1);
        }
    }
    void u64(std::uint64_t n) {
        for (int i = 0; i < 8; ++i) {
            const auto b = static_cast<std::uint8_t>(n >> (8 * i));
            raw(&b, 1);
        }
    }
    void text(const std::string &s) {
        require(s.size() <= text_budget, "系统文件文本超预算");
        u32(static_cast<std::uint32_t>(s.size()));
        raw(s.data(), s.size());
    }
    void blob(const Bytes &b) {
        u32(static_cast<std::uint32_t>(b.size()));
        raw(b.data(), b.size());
    }
};
struct Reader {
    const Bytes &bytes;
    std::size_t at{};
    void need(std::size_t n) const {
        require(at <= bytes.size() && n <= bytes.size() - at, "系统文件截断");
    }
    std::uint32_t u32() {
        need(4);
        std::uint32_t value{};
        for (int i = 0; i < 4; ++i)
            value |= static_cast<std::uint32_t>(bytes[at++]) << (8 * i);
        return value;
    }
    std::uint64_t u64() {
        need(8);
        std::uint64_t value{};
        for (int i = 0; i < 8; ++i)
            value |= static_cast<std::uint64_t>(bytes[at++]) << (8 * i);
        return value;
    }
    Bytes raw(std::uint64_t n) {
        require(n <= file_budget, "系统分区长度超预算");
        need(static_cast<std::size_t>(n));
        const auto first = bytes.begin() + static_cast<std::ptrdiff_t>(at);
        at += static_cast<std::size_t>(n);
        return {first, bytes.begin() + static_cast<std::ptrdiff_t>(at)};
    }
    std::string text() {
        const auto n = u32();
        require(n <= text_budget, "系统文件文本超预算");
        const auto b = raw(n);
        return {b.begin(), b.end()};
    }
    Bytes blob() {
        const auto n = u32();
        require(n <= vector_budget, "系统继承列表超预算");
        return raw(n);
    }
    void end() const { require(at == bytes.size(), "系统分区有尾字节"); }
};
std::int64_t signed64(Reader &r) {
    const auto value = r.u64();
    require(value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
            "系统记录整数越界");
    return static_cast<std::int64_t>(value);
}
int signed32(Reader &r) {
    const auto value = r.u32();
    require(value <= static_cast<std::uint32_t>(std::numeric_limits<int>::max()),
            "系统记录整数越界");
    return static_cast<int>(value);
}
Bytes encode(const StartupSystemRecords &r) {
    valid(r);
    Writer record, inheritance;
    record.u32(static_cast<std::uint32_t>(r.last_slot));
    record.u64(static_cast<std::uint64_t>(r.high_score));
    record.u64(static_cast<std::uint64_t>(r.cash_peak));
    record.text(r.score_village);
    record.text(r.cash_village);
    record.u32(static_cast<std::uint32_t>(r.trophy));
    inheritance.blob(r.facility_levels);
    inheritance.blob(r.profession_status);
    Writer file;
    file.raw(magic, 8);
    file.u32(1);
    file.text(startup_world_persistence_dataset());
    file.u32(static_cast<std::uint32_t>(2 + r.opaque_sections.size()));
    const auto section = [&](std::uint32_t id, std::uint32_t version, bool required,
                             const Bytes &payload) {
        file.u32(id);
        file.u32(version);
        file.u32(required ? 1 : 0);
        file.u64(payload.size());
        const auto hash = dungeon_village_tools::sha256_hex(payload);
        file.raw(hash.data(), 64);
        file.raw(payload.data(), payload.size());
    };
    section(1, 1, true, record.bytes);
    section(2, 1, true, inheritance.bytes);
    for (const auto &s : r.opaque_sections)
        section(s.id, s.version, false, s.bytes);
    const auto digest = dungeon_village_tools::sha256_hex(file.bytes);
    file.raw(digest.data(), 64);
    return std::move(file.bytes);
}
StartupSystemRecords decode(Bytes file) {
    require(file.size() >= 80 && file.size() <= file_budget, "系统文件长度非法");
    const std::string checksum(file.end() - 64, file.end());
    file.resize(file.size() - 64);
    require(checksum == dungeon_village_tools::sha256_hex(file), "系统文件整体摘要不符");
    Reader reader{file};
    const auto signature = reader.raw(8);
    require(std::equal(signature.begin(), signature.end(), magic), "系统文件标识不符");
    require(reader.u32() == 1, "不支持的系统文件版本");
    require(reader.text() == startup_world_persistence_dataset(), "系统记录固定数据来源不匹配");
    const auto count = reader.u32();
    require(count >= 2 && count <= 62, "系统文件分区数非法");
    StartupSystemRecords r;
    bool record_seen = false, inheritance_seen = false;
    std::set<std::uint32_t> ids;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto id = reader.u32(), version = reader.u32(), required = reader.u32();
        require(ids.insert(id).second && version > 0 && required <= 1, "系统分区身份非法或重复");
        require((id == 1 || id == 2) ? version == 1 && required == 1 : id >= 1024 && required == 0,
                "系统必需分区版本不符或未知必需／保留分区");
        const auto n = reader.u64();
        require(n <= (id >= 1024 ? opaque_budget : file_budget), "系统分区载荷超预算");
        const auto digest = reader.raw(64), payload = reader.raw(n);
        require(std::string(digest.begin(), digest.end()) == dungeon_village_tools::sha256_hex(payload),
                "系统分区摘要不符");
        Reader section{payload};
        if (id == 1) {
            r.last_slot = signed32(section);
            r.high_score = signed64(section);
            r.cash_peak = signed64(section);
            r.score_village = section.text();
            r.cash_village = section.text();
            r.trophy = signed32(section);
            section.end();
            record_seen = true;
        } else if (id == 2) {
            r.facility_levels = section.blob();
            r.profession_status = section.blob();
            section.end();
            inheritance_seen = true;
        } else
            r.opaque_sections.push_back({id, version, payload});
    }
    reader.end();
    require(record_seen && inheritance_seen, "系统必需分区缺失");
    valid(r);
    return r;
}
} // namespace
namespace persistence_detail {
Bytes encode_system_records_bytes(const StartupSystemRecords &records) {
    auto bytes = encode(records);
    (void)decode(bytes); // 沿用文件写入前自检，不经过临时文件往返。
    return bytes;
}
StartupSystemRecords decode_system_records_bytes(Bytes bytes) {
    return decode(std::move(bytes));
}
} // namespace persistence_detail

std::string validate_startup_system_records(const StartupSystemRecords &r) {
    try {
        valid(r);
        return {};
    } catch (const std::exception &e) {
        return e.what();
    }
}
StartupSystemLoadResult load_startup_system_file(const std::filesystem::path &path) {
    try {
        require(!path.empty() && !path.filename().empty(), "系统文件路径为空或无文件名");
        std::error_code error;
        const auto status = std::filesystem::symlink_status(path, error);
        if (status.type() == std::filesystem::file_type::not_found &&
            (!error || error == std::errc::no_such_file_or_directory))
            return {StartupSystemRecords{}, true, {}};
        require(!error, "无法查询系统文件状态");
        require(std::filesystem::is_regular_file(status), "系统文件目标不是普通文件");
        return {persistence_detail::decode_system_records_bytes(
                    persistence_detail::read_save_file(path, file_budget)), false, {}};
    } catch (const std::exception &e) {
        return {std::nullopt, false, e.what()};
    }
}
std::string save_startup_system_file(const std::filesystem::path &path,
                                   const StartupSystemRecords &r) {
    try {
        require(!path.empty() && !path.filename().empty(), "系统文件路径为空或无文件名");
        auto bytes = persistence_detail::encode_system_records_bytes(r);
        persistence_detail::replace_save_file(path, bytes);
        return {};
    } catch (const std::exception &e) {
        return e.what();
    }
}
} // namespace dungeon_village_prototype
