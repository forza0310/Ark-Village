#include "startup_application_storage_replay.hpp"
#include "startup_application_storage_paths.hpp"
#include "../persistence/startup_persistence_bytes.hpp"
#include "../persistence/startup_world_file_io.hpp"
#include "ark/assets/sha256.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <limits>
#include <map>
#include <stdexcept>
#include <type_traits>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

namespace ark::simulation::persistence_detail {
namespace {
namespace fs = std::filesystem;
constexpr std::size_t file_limit = 128U * 1024U * 1024U;
constexpr std::size_t system_limit = 4U * 1024U * 1024U;
void need(bool value, const char *reason) {
    if (!value) throw std::runtime_error(std::string("应用文件视图拒绝：") + reason);
}
// 这里只核线字节和引用闭包；世界语义只在带共享预算的入口解码一次。
void validate_structure(const StartupApplicationStorageView &view) {
    need(view.system_bytes.size() <= system_limit, "系统载荷超过预算");
    const auto records = decode_system_records_bytes(view.system_bytes);
    need(encode_system_records_bytes(records) == encode_system_records_bytes(view.snapshot.records),
         "系统字节与目录观察值不一致");
    if (view.snapshot.missing) {
        need(view.snapshot.digest == ark::assets::sha256_hex(view.system_bytes) &&
             encode_system_records_bytes(records) == encode_system_records_bytes(StartupSystemRecords{}),
             "缺失系统只能表示原始默认目录");
    } else {
        need(view.snapshot.digest == ark::assets::sha256_hex(view.system_bytes),
             "系统观察摘要不符");
    }
    std::map<std::array<std::uint8_t,32>, StartupSaveReference> references;
    for (const auto &slot : records.save_directory)
        for (const auto &entry : slot) if (entry.reference) {
            const auto &ref = *entry.reference;
            const auto inserted = references.emplace(ref.sha256, ref);
            need(inserted.second || inserted.first->second == ref, "同一内容摘要的引用元数据冲突");
        }
    need(view.blobs.size() == references.size() && view.blobs.size() <= 4,
         "槽位blob集合不等于包含隐藏项的引用闭包");
    std::size_t total = view.system_bytes.size();
    auto expected = references.begin();
    for (const auto &blob : view.blobs) {
        need(expected != references.end() && blob.reference == expected->second,
             "blob缺失、重复、额外或非规范摘要顺序");
        need(blob.reference.purpose == StartupWorldSavePurpose::normal &&
             blob.reference.bytes == blob.bytes.size(), "blob用途或字节数不符");
        need(blob.bytes.size() <= file_limit - total, "所有槽位字节累计超预算");
        total += blob.bytes.size();
        need(storage_hash_hex(blob.reference.sha256) == ark::assets::sha256_hex(blob.bytes),
             "blob内容摘要不符");
        ++expected;
    }
}
#ifdef _WIN32
[[noreturn]] void windows_failure(const char *operation) {
    const auto error = GetLastError();
    throw std::runtime_error(std::string("应用新根发布：") + operation + " (Windows " +
                             std::to_string(error) + ")");
}
struct Handle {
    HANDLE value{INVALID_HANDLE_VALUE};
    Handle() = default;
    explicit Handle(HANDLE h) : value(h) {}
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
    Handle(Handle &&other) noexcept : value(other.value) { other.value = INVALID_HANDLE_VALUE; }
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
Handle directory_handle(const fs::path &path, DWORD access) {
    Handle handle(CreateFileW(path.c_str(), access, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                              nullptr));
    if (handle.value == INVALID_HANDLE_VALUE) windows_failure("不能固定目录句柄");
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(handle.value, &info)) windows_failure("不能读取目录身份");
    need((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
         (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0, "目录句柄是重解析点或非目录");
    return handle;
}
// 所有父组件保持无DELETE共享的句柄；发布期间其它正常写者仍可创建自己的子项。
std::vector<Handle> pin_parents(const fs::path &parent) {
    std::vector<Handle> handles;
    fs::path cursor = parent.root_path();
    handles.push_back(directory_handle(cursor, FILE_READ_ATTRIBUTES));
    for (const auto &component : parent.relative_path()) {
        cursor /= component;
        handles.push_back(directory_handle(cursor, FILE_READ_ATTRIBUTES));
    }
    return handles;
}
struct Staging {
    fs::path root;
    Handle handle;
    std::vector<fs::path> files;
    bool has_worlds{}, published{}, cleaned{};
    explicit Staging(const fs::path &parent) {
        static std::atomic<unsigned long long> serial{};
        for (int attempt = 0; attempt < 32; ++attempt) {
            root = parent / (".avrapp-stage." +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                "." + std::to_string(serial++));
            if (CreateDirectoryW(root.c_str(), nullptr)) {
                try {
                    auto created = directory_handle(root, DELETE | FILE_READ_ATTRIBUTES);
                    handle.value = created.value;
                    created.value = INVALID_HANDLE_VALUE;
                } catch (...) {
                    // 新建失败后只移除这个仍为空的专属目录，不递归碰其它内容。
                    RemoveDirectoryW(root.c_str());
                    throw;
                }
                return;
            }
            const auto error = GetLastError();
            if (error != ERROR_ALREADY_EXISTS && error != ERROR_FILE_EXISTS)
                windows_failure("不能独占创建临时目录");
        }
        throw std::runtime_error("应用新根发布：临时目录名称竞争");
    }
    // 不递归删目录，只回收本次确实登记的文件。残留未知临时项时报告，不能掩盖。
    std::string cleanup() noexcept {
        if (published || cleaned) return {};
        try {
            for (const auto &file : files) {
                validate_storage_file(file, true);
                std::error_code ec;
                fs::remove(file, ec);
                if (ec) return "临时文件清理失败：" + file.string();
            }
            if (has_worlds && !RemoveDirectoryW((root / "worlds").c_str()) &&
                GetLastError() != ERROR_PATH_NOT_FOUND && GetLastError() != ERROR_FILE_NOT_FOUND)
                return "临时worlds目录仍有残留：" + root.string();
            FILE_DISPOSITION_INFO disposition{TRUE};
            if (!SetFileInformationByHandle(handle.value, FileDispositionInfo,
                                            &disposition, sizeof(disposition)))
                return "临时根清理失败：" + root.string();
            cleaned = true;
            return {};
        } catch (...) { return "临时目录校验／清理失败"; }
    }
    ~Staging() { (void)cleanup(); }
};
#endif
} // namespace

void validate_application_replay_storage_view(const StartupApplicationStorageView &view,
                                              CodecDecodeBudget &budget) {
    validate_structure(view);
    for (const auto &blob : view.blobs)
        (void)decode_world_session_bytes(blob.bytes, startup_world_rules(),
                                         StartupWorldSavePurpose::normal, "", budget);
}
StartupApplicationStorageSnapshot publish_application_replay_storage(const fs::path &new_root,
    const StartupApplicationStorageView &view, const std::function<void()> &before_publish) {
    static_assert(std::is_nothrow_move_constructible_v<StartupApplicationStorageSnapshot>);
    validate_structure(view);
    auto installed = view.snapshot;
    installed.missing = false;
    installed.digest = ark::assets::sha256_hex(view.system_bytes);
#ifdef _WIN32
    const auto target = validate_storage_root(new_root, false);
    const auto parents = pin_parents(target.parent_path());
    (void)parents;
    need(validate_storage_root(target, false) == target, "固定父目录后规范根变化");
    Staging staging(target.parent_path());
    try {
        staging.files.reserve(view.blobs.size() + 1);
        const auto worlds = staging.root / "worlds";
        if (!CreateDirectoryW(worlds.c_str(), nullptr)) windows_failure("不能创建临时worlds目录");
        staging.has_worlds = true;
        for (const auto &blob : view.blobs) {
            const auto file = worlds / ("world-" + storage_hash_hex(blob.reference.sha256) + ".avrs");
            staging.files.push_back(file);
            create_save_file(file, blob.bytes);
        }
        const auto system = staging.root / "system.avr";
        staging.files.push_back(system);
        create_save_file(system, view.system_bytes);
        // 文件写闭、刷新和逐字节回读沿create_save_file；名称只由受验摘要产生。
        if (before_publish) before_publish();
        need(validate_storage_root(target, false) == target, "发布前根路径变化");
        const auto native = target.native();
        const auto name_bytes = native.size() * sizeof(wchar_t);
        need(name_bytes <= std::numeric_limits<DWORD>::max() - sizeof(FILE_RENAME_INFO),
             "Windows目录名称超长");
        // FileNameLength不含终止符，变长缓冲仍保留WCHAR[1]及零终止空间。
        // 本机真实恢复曾把相邻内存的“on/ld”拼到目标名；不能只因API返回成功就认定落点正确。
        std::vector<std::uint8_t> rename_buffer(sizeof(FILE_RENAME_INFO) + name_bytes, 0);
        auto *rename = reinterpret_cast<FILE_RENAME_INFO *>(rename_buffer.data());
        rename->ReplaceIfExists = FALSE;
        rename->RootDirectory = nullptr;
        rename->FileNameLength = static_cast<DWORD>(name_bytes);
        std::copy(native.begin(), native.end(), rename->FileName);
        // 同卷目录句柄原子改名，ReplaceIfExists=false；竞争创建目标也不能被覆盖。
        if (!SetFileInformationByHandle(staging.handle.value, FileRenameInfo, rename,
                                       static_cast<DWORD>(rename_buffer.size())))
            windows_failure("完整新根无覆盖发布失败");
        staging.published = true;
        return installed; // 此后只有noexcept移动及句柄/容器析构，不再读取或分配。
    } catch (const std::exception &error) {
        const auto cleanup = staging.cleanup();
        if (!cleanup.empty()) throw std::runtime_error(std::string(error.what()) + "；" + cleanup);
        throw;
    }
#else
    (void)new_root;
    (void)before_publish;
    throw std::runtime_error("应用新根发布：当前平台尚未提供经验证的原子无覆盖目录发布");
#endif
}
} // namespace ark::simulation::persistence_detail
