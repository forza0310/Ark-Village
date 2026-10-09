#include "ark/simulation/startup_system_records.hpp"
#include "ark/assets/sha256.hpp"
#include "../../src/simulation/startup_persistence_bytes.hpp"
#include "../../src/simulation/startup_world_file_io.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <set>
#include <atomic>
#include <thread>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace ark::simulation;
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
    const auto digest = ark::assets::sha256_hex(bytes);
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
std::set<std::filesystem::path> file_names(const std::filesystem::path &path) {
    std::set<std::filesystem::path> names;
    for (const auto &entry : std::filesystem::directory_iterator(path))
        names.insert(entry.path().filename());
    return names;
}
// 无覆盖发布的主责验证放在既有系统文件套件；不为内部平台函数新增target。
void create_only_files(const std::filesystem::path &directory, const Bytes &original) {
    const auto created = directory / "create-only.avr";
    persistence_detail::create_save_file(created, original);
    check(read(created) == original, "无覆盖出口首次发布完整字节");
    const auto reject_create = [&](const std::filesystem::path &target, const Bytes &bytes) {
        const auto before = file_names(directory);
        bool refused = false;
        try {
            persistence_detail::create_save_file(target, bytes);
        } catch (const std::exception &e) {
            refused = *e.what() != '\0';
        }
        check(refused, "无覆盖出口对已有目标或不可创建路径具名拒绝");
        check(file_names(directory) == before, "无覆盖发布失败不遗留临时文件或新增目录");
    };
    reject_create(created, Bytes{1, 2, 3});
    check(read(created) == original, "无覆盖拒绝保留已有文件原字节");
    reject_create(created, original);
    check(read(created) == original, "即便字节相同也拒绝覆盖已有文件");
    const auto occupied = directory / "occupied-directory";
    check(std::filesystem::create_directory(occupied), "准备已有目标目录");
    const auto marker = occupied / "keep.bin";
    write(marker, Bytes{9, 8, 7});
    reject_create(occupied, original);
    check(std::filesystem::is_directory(occupied) && read(marker) == Bytes({9, 8, 7}) &&
              file_names(occupied) == std::set<std::filesystem::path>{"keep.bin"},
          "目标目录拒绝后原内容不变");
    reject_create(directory / "missing-create-parent" / "system.avr", original);
    check(!std::filesystem::exists(directory / "missing-create-parent"),
          "无覆盖出口不偷偷建立缺失父目录");
    // 已有硬链接目标不能经别名覆盖。这里只验证平台出口，不冒称上层路径隔离已完成。
    const auto alias = directory / "existing-alias.avr";
    std::filesystem::create_hard_link(created, alias);
    reject_create(alias, Bytes{4, 5, 6});
    check(std::filesystem::equivalent(alias, created) && read(alias) == original &&
              read(created) == original,
          "无覆盖拒绝保留已有硬链接及两端原字节");
    const auto raced = directory / "concurrent-create.avr";
    const auto before_race = file_names(directory);
    const Bytes offers[2] = {{1, 3, 5, 7}, {2, 4, 6}};
    bool accepted[2]{};
    bool rejected[2]{};
    {
        // 即使第二个线程创建失败，析构也先放开首线程再收齐，避免留下等待者。
        struct Writers {
            std::atomic<bool> start{false};
            std::vector<std::thread> threads;
            ~Writers() {
                start.store(true);
                for (auto &thread : threads)
                    if (thread.joinable())
                        thread.join();
            }
        } writers;
        writers.threads.reserve(2);
        for (int i = 0; i < 2; ++i)
            writers.threads.emplace_back([&, i] {
                while (!writers.start.load())
                    std::this_thread::yield();
                try {
                    persistence_detail::create_save_file(raced, offers[i]);
                    accepted[i] = true;
                } catch (const std::exception &) {
                    rejected[i] = true;
                }
            });
        writers.start.store(true);
    }
    check(accepted[0] != accepted[1] && rejected[0] != rejected[1] &&
              accepted[0] == rejected[1],
          "同名并发无覆盖发布恰一成功一拒绝");
    check(read(raced) == offers[accepted[0] ? 0 : 1], "竞争失败者不覆盖成功者字节");
    auto expected_files = before_race;
    expected_files.insert(raced.filename());
    check(file_names(directory) == expected_files, "两写者退出后只留下完整目标且无临时文件");
}
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
    const auto in_memory = persistence_detail::encode_system_records_bytes(r);
    check(in_memory == original, "内部系统字节桥与既有格式完全相同");
    const auto from_bytes = persistence_detail::decode_system_records_bytes(in_memory);
    check(persistence_detail::encode_system_records_bytes(from_bytes) == original,
          "纯字节桥往返保留全部已知字段及未知可选段");
    create_only_files(work.path, original);
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
