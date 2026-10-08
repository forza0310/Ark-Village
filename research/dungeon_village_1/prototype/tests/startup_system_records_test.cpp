#include "dungeon_village_prototype/startup_system_records.hpp"
#include "dungeon_village_tools/archive.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace dungeon_village_prototype;
namespace {
using Bytes = std::vector<std::uint8_t>;
int checks{};
void check(bool value, const std::string &message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
Bytes read(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    check(static_cast<bool>(input), "测试文件读取失败");
    return {std::istreambuf_iterator<char>(input), {}};
}
void write(const std::filesystem::path &path, const Bytes &bytes) {
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char *>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    check(static_cast<bool>(output), "测试文件写入失败");
}
void u32(Bytes &bytes, std::size_t at, std::uint32_t value) {
    for (int i = 0; i < 4; ++i)
        bytes.at(at + i) = static_cast<std::uint8_t>(value >> (8 * i));
}
std::uint64_t number(const Bytes &bytes, std::size_t &at, int size) {
    std::uint64_t value{};
    for (int i = 0; i < size; ++i)
        value |= static_cast<std::uint64_t>(bytes.at(at++)) << (8 * i);
    return value;
}
void resign(Bytes &bytes) {
    check(bytes.size() >= 64, "测试整体摘要边界");
    bytes.resize(bytes.size() - 64);
    const auto digest = dungeon_village_tools::sha256_hex(bytes);
    bytes.insert(bytes.end(), digest.begin(), digest.end());
}
// 按公开字节布局定位段，故意损坏摘要以外的契约，确保拒绝不是只碰巧校验和失败。
struct SectionOffset { std::size_t header, digest, payload, length; };
std::vector<SectionOffset> sections(const Bytes &bytes) {
    std::size_t at = 12;
    const auto dataset_length = number(bytes, at, 4);
    at += static_cast<std::size_t>(dataset_length);
    const auto count = number(bytes, at, 4);
    std::vector<SectionOffset> out;
    for (std::uint64_t i = 0; i < count; ++i) {
        const auto header = at;
        at += 12;
        const auto length = number(bytes, at, 8);
        const auto digest = at;
        at += 64;
        const auto payload = at;
        at += static_cast<std::size_t>(length);
        check(at <= bytes.size() - 64, "测试分区边界");
        out.push_back({header, digest, payload, static_cast<std::size_t>(length)});
    }
    return out;
}
void reject(const std::filesystem::path &path, const Bytes &bytes, const char *scenario) {
    write(path, bytes);
    const auto result = load_startup_system_file(path);
    check(!result.records && !result.missing && !result.error.empty(), scenario);
}
struct Workspace {
    std::filesystem::path path;
    ~Workspace() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};
void run(const std::filesystem::path &directory) {
    check(!directory.empty(), "测试目录参数为空");
    std::filesystem::create_directories(directory);
    Workspace work{directory /
                   ("system-records-fixture-" + std::to_string(
                       std::chrono::steady_clock::now().time_since_epoch().count()))};
    check(std::filesystem::create_directory(work.path), "创建项目内测试目录");
    const auto file = work.path / "system.avr", broken = work.path / "broken.avr";
    const auto missing = load_startup_system_file(file);
    check(missing.records && missing.missing && missing.error.empty(), "真正缺失才提供默认候选");
    check(missing.records->last_slot == 0 && missing.records->high_score == 0 &&
              missing.records->cash_peak == 0 && missing.records->score_village == "没有记录" &&
              missing.records->cash_village == "没有记录" && missing.records->trophy == 0 &&
              missing.records->facility_levels.empty() && missing.records->profession_status.empty(),
          "默认系统记录不凭空补继承数据");
    StartupSystemRecords r;
    r.last_slot = 1;
    r.high_score = 9876543210LL;
    r.cash_peak = 12345678901LL;
    r.score_village = "分数村";
    r.cash_village = "资金村";
    r.trophy = 4;
    r.facility_levels = {0, 1, 0, 5};
    r.profession_status = {0, 0, 0, 2};
    r.opaque_sections = {{1024, 9, {0, 255, 7}}, {2048, 1, {}}};
    check(validate_startup_system_records(r).empty(), "有效系统记录校验");
    check(save_startup_system_file(file, r).empty(), "系统文件正常保存");
    const auto original = read(file);
    const auto offsets = sections(original);
    check(std::string(original.begin(), original.begin() + 8) == "AVRSYS01",
          "独立系统magic不冒充世界文件");
    const auto loaded = load_startup_system_file(file);
    check(loaded.records && !loaded.missing && loaded.error.empty(), "系统文件正常恢复");
    const auto &copy = *loaded.records;
    check(copy.last_slot == 1 && copy.high_score == 9876543210LL && copy.cash_peak == 12345678901LL &&
              copy.score_village == "分数村" && copy.cash_village == "资金村" && copy.trophy == 4 &&
              copy.facility_levels == r.facility_levels && copy.profession_status == r.profession_status,
          "全部已知字段往返");
    check(copy.opaque_sections.size() == 2 && copy.opaque_sections[0].id == 1024 &&
              copy.opaque_sections[0].version == 9 && copy.opaque_sections[0].bytes == Bytes({0, 255, 7}) &&
              copy.opaque_sections[1].id == 2048 && copy.opaque_sections[1].bytes.empty(),
          "未知可选段身份版本原序与字节保留");
    check(save_startup_system_file(broken, copy).empty() && read(broken) == original,
          "系统记录重编码字节稳定");
    for (const auto length : {std::size_t{0}, std::size_t{7}, original.size() - 1})
        reject(broken, Bytes(original.begin(), original.begin() + static_cast<std::ptrdiff_t>(length)),
               "截断不得当缺失");
    auto changed = original;
    changed.at(offsets[0].payload) ^= 1;
    reject(broken, changed, "整体摘要损坏拒绝");
    resign(changed);
    reject(broken, changed, "整体摘要有效仍拒绝坏分区摘要");
    for (const auto mutation : {0, 1, 2, 3, 4, 5}) {
        changed = original;
        switch (mutation) {
        case 0: u32(changed, 8, 2); break;
        case 1: changed.at(16) ^= 1; break; // 数据身份字符串，不改变长度。
        case 2: u32(changed, offsets[2].header + 8, 1); break;
        case 3: u32(changed, offsets[2].header, 9); break;
        case 4: u32(changed, offsets[2].header, 1); break;
        case 5: u32(changed, offsets[0].header + 4, 2); break;
        }
        resign(changed);
        reject(broken, changed, "版本／来源／未知必需／保留／重复段拒绝");
    }
    for (const auto mutation : {0, 1, 2, 3, 4, 5, 6, 7}) {
        auto bad = r;
        switch (mutation) {
        case 0: bad.last_slot = 2; break;
        case 1: bad.high_score = -1; break;
        case 2: bad.trophy = 5; break;
        case 3: bad.score_village.assign(4097, 'x'); break;
        case 4: bad.facility_levels = {0}; break;
        case 5: bad.profession_status.assign(4098, 0); break;
        case 6: bad.opaque_sections[0].bytes.assign(1024U * 1024U + 1, 0); break;
        case 7:
            bad.opaque_sections.clear();
            for (std::uint32_t i = 0; i < 4; ++i)
                bad.opaque_sections.push_back({1024 + i, 1, Bytes(1024U * 1024U, 0)});
            break;
        }
        check(!validate_startup_system_records(bad).empty(), "非法业务或预算数据显式拒绝");
        check(!save_startup_system_file(file, bad).empty() && read(file) == original,
              "编码拒绝不覆盖旧有效文件");
    }
    reject(broken, Bytes(4U * 1024U * 1024U + 1, 0), "读取总预算拒绝");
    const auto directory_result = load_startup_system_file(work.path);
    check(!directory_result.records && !directory_result.missing && !directory_result.error.empty(), "目录不得当缺失");
    check(!save_startup_system_file(work.path, r).empty(), "目录写目标拒绝");
    check(!save_startup_system_file(work.path / "missing-parent" / "system.avr", r).empty(),
          "不可创建临时文件写失败");
#ifdef _WIN32
    // 无共享删除的文件句柄迫使原子替换失败，断言旧档与临时文件清理。
    const auto handle = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    check(handle != INVALID_HANDLE_VALUE, "锁定旧有效文件夹具");
    const auto error = save_startup_system_file(file, r);
    check(CloseHandle(handle) != 0, "释放锁定句柄");
    check(!error.empty() && read(file) == original, "替换失败保留旧有效系统文件");
#endif
    for (const auto &entry : std::filesystem::directory_iterator(work.path))
        check(entry.path().filename().string().find(".tmp.") == std::string::npos,
              "失败路径不遗留临时系统文件");
}
} // namespace
int run_startup_system_records_tests(const std::filesystem::path &directory) {
    try {
        run(directory);
        std::cout << "系统记录文件检查通过：" << checks << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "系统记录文件检查失败：" << e.what() << '\n';
        return 1;
    }
}
