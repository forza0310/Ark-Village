#pragma once
#include "dungeon_village_prototype/startup_system_records.hpp"

namespace dungeon_village_prototype {
// 磁盘观察值不是可写Owner；missing仅表示尚未发布system的默认存储根。
struct StartupApplicationStorageSnapshot {
    StartupSystemRecords records;
    bool missing{};
    std::string digest; // 已有文件hash；missing时为规范默认system字节hash，仍严格区分missing。
};
struct StartupApplicationStorageResult {
    std::optional<StartupApplicationStorageSnapshot> snapshot;
    std::string error;
    // snapshot存在即已经提交；清理债务不能被调用者当作事务失败重试业务。
    bool cleanup_pending{};
};
struct StartupApplicationStorageBlob {
    StartupSaveReference reference;
    std::vector<std::uint8_t> bytes;
};
struct StartupApplicationStorageView {
    StartupApplicationStorageSnapshot snapshot;
    std::vector<std::uint8_t> system_bytes;
    std::vector<StartupApplicationStorageBlob> blobs; // 按32字节摘要字典升序，去重且无额外载荷。
};
struct StartupApplicationStorageCapture {
    std::optional<StartupApplicationStorageView> view;
    std::string error;
};
// root必须显式存在且为安全普通目录。Windows之外返回不支持，不降级为无锁写入。
StartupApplicationStorageResult load_startup_application_storage(const std::filesystem::path &root);
// candidate不得擅改expected的revision或四目录；三个写入口在独占租约中核磁盘修订和摘要。
StartupApplicationStorageResult commit_startup_application_records(const std::filesystem::path &root,
    const StartupApplicationStorageSnapshot &expected, const StartupSystemRecords &candidate);
StartupApplicationStorageResult save_startup_application_slot(const std::filesystem::path &root,
    const StartupApplicationStorageSnapshot &expected, const StartupSystemRecords &candidate,
    int slot, StartupSaveKind kind, const StartupWorldRuntimeSession &world);
StartupApplicationStorageResult hide_startup_application_slot(const std::filesystem::path &root,
    const StartupApplicationStorageSnapshot &expected, const StartupSystemRecords &candidate,
    int slot, StartupSaveKind kind);
StartupWorldLoadResult load_startup_application_slot(const std::filesystem::path &root,
    const StartupApplicationStorageSnapshot &expected, int slot, StartupSaveKind kind);
// 同一租约捕获隐藏/可见四条引用的去重字节；总128MiB，目录读取不重建世界或原档。
StartupApplicationStorageCapture capture_startup_application_storage(const std::filesystem::path &root,
    const StartupApplicationStorageSnapshot &expected);
} // namespace dungeon_village_prototype
