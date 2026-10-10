#include "ark/simulation/startup_system_records.hpp"
#include "ark/simulation/startup_application_storage.hpp"
#include "ark/assets/sha256.hpp"
#include "../../src/simulation/startup_persistence_bytes.hpp"
#include "../../src/simulation/startup_world_file_io.hpp"
#include "../../src/simulation/startup_application_storage_paths.hpp"
#include <chrono>
#include <algorithm>
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
// 四条目录是逻辑记录；本夹具的interrupt载荷为合法normal条件档，不冒充原日历轮内自动档。
void application_storage(const std::filesystem::path &parent) {
#ifdef _WIN32
    const auto root=parent/"application-storage";
    check(std::filesystem::create_directory(root),"创建独立应用存储夹具根");
    auto loaded=load_startup_application_storage(root);
    check(loaded.snapshot && loaded.snapshot->missing && loaded.snapshot->digest==
              ark::assets::sha256_hex(persistence_detail::encode_system_records_bytes(StartupSystemRecords{})) &&
              file_names(root).empty(),"读取缺失系统只返回默认观察值，不写文件");
    const auto empty=*loaded.snapshot;
    auto records=empty.records;
    records.high_score=77;
    records.opaque_sections={{1042,3,Bytes{7,0,255}}};
    auto committed=commit_startup_application_records(root,empty,records);
    check(committed.snapshot && !committed.snapshot->missing && !committed.cleanup_pending &&
              committed.error.empty() && committed.snapshot->records.revision==1 &&
              committed.snapshot->records.high_score==77,"单系统提交同步纪录与修订");
    auto current=*committed.snapshot;
    const auto system=root/"system.avr";
    auto before=read(system);
    check(!commit_startup_application_records(root,empty,records).snapshot && read(system)==before,
          "旧缺失观察值不能覆盖已发布系统");
    StartupSession initial;
    StartupWorldRuntimeSession world(initial.state(),ref::WorldRandomStream::from_java_seed(1));
    const auto world_digest=startup_world_session_digest(world);
    for(const bool existing_directory:{false,true}) {
        if(existing_directory)check(std::filesystem::create_directory(root/"worlds"),"准备预存空worlds目录");
        const auto held=CreateFileW(system.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        check(held!=INVALID_HANDLE_VALUE,"锁住首个blob保存的系统提交点");
        const auto rejected=save_startup_application_slot(root,current,current.records,0,StartupSaveKind::manual,world);
        CloseHandle(held);
        check(!rejected.snapshot && !rejected.cleanup_pending &&
                  rejected.error.find("替换存档失败")!=std::string::npos && read(system)==before &&
                  std::filesystem::exists(root/"worlds")==existing_directory,
              "首次blob在真实系统替换失败后只回收本次新目录，预存空目录不误删");
        if(existing_directory)check(file_names(root/"worlds").empty(),"失败后预存目录保留且无新blob");
    }
    auto candidate=current.records;
    candidate.last_slot=0;
    committed=save_startup_application_slot(root,current,candidate,0,StartupSaveKind::manual,world);
    check(committed.snapshot.has_value(),"真实新局稳定normal载荷保存："+committed.error);
    current=*committed.snapshot;
    const auto &manual=current.records.save_directory[0][1];
    check(manual.packed_date==300 && manual.village==world.state().scripts.village_name &&
              manual.cash==world.state().scene.world.world.ai.accounting.funds() && manual.reference &&
              current.records.opaque_sections.front().bytes==Bytes({7,0,255}),
          "保存目录取同一世界日期/村名/现金，未知系统段保留");
    const auto old_blob=root/"worlds"/("world-"+persistence_detail::storage_hash_hex(manual.reference->sha256)+".avrs");
    const auto old_bytes=read(old_blob);
    before=read(system);
    const auto reuse_lock=CreateFileW(system.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(reuse_lock!=INVALID_HANDLE_VALUE,"锁住复用既有blob的系统提交点");
    const auto reuse_failed=save_startup_application_slot(root,current,current.records,1,StartupSaveKind::manual,world);
    CloseHandle(reuse_lock);
    check(!reuse_failed.snapshot && !reuse_failed.cleanup_pending &&
              reuse_failed.error.find("替换存档失败")!=std::string::npos && read(system)==before &&
              read(old_blob)==old_bytes && file_names(root/"worlds").size()==1,
          "同hash复用但系统失败时不误删预存blob");
    auto restored=load_startup_application_slot(root,current,0,StartupSaveKind::manual);
    check(restored.snapshot && startup_world_state_digest(restored.snapshot->session.state())==
              startup_world_state_digest(world.state()),"目录加载完整校验正常世界，不推进世界");
    const auto prior=current;
    committed=save_startup_application_slot(root,current,current.records,0,StartupSaveKind::interrupt,world);
    check(committed.snapshot && file_names(root/"worlds").size()==1,
          "中断条件目录可复用相同字节blob，不复制第二份或改用途");
    current=*committed.snapshot;
    check(current.records.save_directory[0][0].reference==current.records.save_directory[0][1].reference,
          "同一字节摘要的两目录引用一致");
    auto view=capture_startup_application_storage(root,current);
    check(view.view && view.view->blobs.size()==1 && view.view->blobs.front().bytes==old_bytes &&
              view.view->system_bytes==read(system),"租约内捕获完整系统与去重四目录文件视图");
    before=read(system);
    check(!commit_startup_application_records(root,prior,prior.records).snapshot && read(system)==before,
          "旧修订/摘要观察值明确拒绝，不覆盖较新目录");
    auto tampered=current.records;
    tampered.save_directory[0][1].packed_date=-1;
    check(!commit_startup_application_records(root,current,tampered).snapshot && read(system)==before,
          "普通纪录出口禁止绕过目录隐藏事务");
    for(const auto slot:{-1,2})
        check(!hide_startup_application_slot(root,current,current.records,slot,StartupSaveKind::manual).snapshot,
              "越界槽号显式拒绝");
    check(!hide_startup_application_slot(root,current,current.records,0,static_cast<StartupSaveKind>(7)).snapshot,
          "未知目录种类显式拒绝");
    committed=hide_startup_application_slot(root,current,current.records,0,StartupSaveKind::manual);
    check(committed.snapshot && committed.error.empty(),"隐藏目录成功");
    current=*committed.snapshot;
    check(current.records.save_directory[0][1].packed_date==-1 &&
              current.records.save_directory[0][1].reference==prior.records.save_directory[0][1].reference &&
              current.records.save_directory[0][0].packed_date==300 && read(old_blob)==old_bytes &&
              !load_startup_application_slot(root,current,0,StartupSaveKind::manual).snapshot,
          "隐藏只清选中日期，其他行与原字节仍在且隐藏行不能加载");
    view=capture_startup_application_storage(root,current);
    check(view.view && view.view->blobs.size()==1,"隐藏目录引用仍进入文件视图，不被当垃圾删除");
    before=read(system);
    check(!hide_startup_application_slot(root,current,current.records,0,StartupSaveKind::manual).snapshot &&
              read(system)==before,"重复隐藏明确拒绝且系统不重写");
    auto changed_world=world;
    changed_world.set_speed(1); // 明确维护条件差异，确保保存为不同字节，不注入业务奖励。
    const auto files_before=file_names(root/"worlds");
    const auto locked=CreateFileW(system.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(locked!=INVALID_HANDLE_VALUE,"锁住系统替换点制造真实晚期写失败");
    const auto failed=save_startup_application_slot(root,current,current.records,1,StartupSaveKind::manual,changed_world);
    CloseHandle(locked);
    check(!failed.snapshot && !failed.error.empty() && !failed.cleanup_pending && read(system)==before &&
              file_names(root/"worlds")==files_before && read(old_blob)==old_bytes,
          "新blob已准备但系统发布失败时持租约回收新文件，保留旧目录与全部旧档");
    const auto lease=CreateFileW((root/"write.lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(lease!=INVALID_HANDLE_VALUE,"持有真实写租约夹具");
    const auto busy=commit_startup_application_records(root,current,current.records);
    CloseHandle(lease);
    check(!busy.snapshot && read(system)==before,"另一个存储写者持租约时拒绝，不偷偷无锁提交");
    committed=save_startup_application_slot(root,current,current.records,0,StartupSaveKind::interrupt,changed_world);
    check(committed.snapshot && file_names(root/"worlds").size()==2,"另一行换新blob仍保留隐藏行旧引用");
    current=*committed.snapshot;
    const auto held_blob=CreateFileW(old_blob.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    check(held_blob!=INVALID_HANDLE_VALUE,"阻止已退休blob清理夹具");
    committed=save_startup_application_slot(root,current,current.records,0,StartupSaveKind::manual,changed_world);
    CloseHandle(held_blob);
    check(committed.snapshot && committed.cleanup_pending && committed.error.empty() && read(old_blob)==old_bytes,
          "提交后清理失败仍明确成功并报告清理债务，不诱使重复业务提交");
    current=*committed.snapshot;
    check(load_startup_application_slot(root,current,0,StartupSaveKind::manual).snapshot.has_value(),
          "清理债务不影响新目录世界读取");
    const auto fresh_blob=root/"worlds"/("world-"+persistence_detail::storage_hash_hex(current.records.save_directory[0][1].reference->sha256)+".avrs");
    const auto alias=root/"alias.avrs";
    std::filesystem::create_hard_link(fresh_blob,alias);
    check(!load_startup_application_slot(root,current,0,StartupSaveKind::manual).snapshot,
          "载荷硬链接别名拒绝，不能借存储根改外部同实体文件");
    std::filesystem::remove(alias);
    const auto fresh_bytes=read(fresh_blob);
    write(fresh_blob,Bytes{1,2,3});
    check(!load_startup_application_slot(root,current,0,StartupSaveKind::manual).snapshot &&
              !capture_startup_application_storage(root,current).view,
          "可见目录不掩盖损坏载荷，加载/文件视图都拒绝");
    write(fresh_blob,fresh_bytes);
    check(startup_world_session_digest(world)==world_digest,"存储/读取/失败均不修改源Session与随机历史");
    check(!load_startup_application_storage(root/".."/"application-storage").snapshot,
          "上溯路径不能在规范化后蒙混过关");
    auto exhausted=current.records;
    exhausted.revision=UINT64_MAX;
    check(save_startup_system_file(system,exhausted).empty(),"构造修订耗尽的系统条件档");
    loaded=load_startup_application_storage(root);
    before=read(system);
    check(loaded.snapshot && !commit_startup_application_records(root,*loaded.snapshot,loaded.snapshot->records).snapshot &&
              read(system)==before,"修订耗尽拒绝，不回卷旧身份");
#else
    check(!load_startup_application_storage(parent).snapshot,"未实现平台明确拒绝目录存储");
#endif
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
    r.revision=17;
    StartupSaveReference directory_ref;
    directory_ref.sha256.fill(0x42);
    directory_ref.bytes=1234;
    r.save_directory[0][1]={20303,"目录村",-678,directory_ref};
    r.save_directory[1][0]={-1,"隐藏村",INT64_MIN,directory_ref};
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
              copy.facility_levels == r.facility_levels && copy.profession_status == r.profession_status &&
              copy.revision==17 && copy.save_directory==r.save_directory,
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
    for (const auto mutation : {0, 1, 2, 3, 4, 5, 6}) {
        changed = original;
        switch (mutation) {
        case 0: u32(changed, 8, 3); break;
        case 1: changed.at(16) ^= 1; break; // 数据身份字符串，不改变长度。
        case 2: u32(changed, offsets[3].header + 8, 1); break;
        case 3: u32(changed, offsets[3].header, 9); break;
        case 4: u32(changed, offsets[3].header, 1); break;
        case 5: u32(changed, offsets[0].header + 4, 2); break;
        case 6: u32(changed, 8, 1); break;
        }
        resign(changed);
        reject(broken, changed, "版本／来源／未知必需／保留／重复段拒绝");
    }
    for (const auto mutation : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14}) {
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
        case 8: bad.save_directory[0][1].packed_date=20304; break;
        case 9: bad.save_directory[0][1].packed_date=21200; break;
        case 10: bad.save_directory[0][1].reference.reset(); break;
        case 11: bad.save_directory[0][1].reference->purpose=StartupWorldSavePurpose::replay; break;
        case 12: bad.save_directory[0][1].reference->bytes=128U*1024U*1024U+1; break;
        case 13: bad.save_directory[0][1].village="坏\n名"; break;
        case 14: ++bad.save_directory[1][0].reference->bytes; break;
        }
        check(!validate_startup_system_records(bad).empty(), "非法业务或预算数据显式拒绝");
        check(!save_startup_system_file(file, bad).empty() && read(file) == original,
              "编码拒绝不覆盖旧有效文件");
    }
    changed=original;
    // 分区3的revision8 + 首条空目录(date4/textlen4/cash8)后为严格u32引用标记。
    u32(changed,offsets[2].payload+24,2);
    const Bytes directory_payload(changed.begin()+static_cast<std::ptrdiff_t>(offsets[2].payload),
        changed.begin()+static_cast<std::ptrdiff_t>(offsets[2].payload+offsets[2].length));
    const auto directory_hash=ark::assets::sha256_hex(directory_payload);
    std::copy(directory_hash.begin(),directory_hash.end(),changed.begin()+static_cast<std::ptrdiff_t>(offsets[2].digest));
    resign(changed);
    reject(broken,changed,"分区和整体摘要重签后仍拒绝非法目录引用布尔");
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
    application_storage(work.path);
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
