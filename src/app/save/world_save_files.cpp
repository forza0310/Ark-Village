#include "ark/app/save/world_save_files.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <mutex>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace ark::app {
const char *world_save_error_text(WorldSaveError error) {
    switch (error) {
    case WorldSaveError::none:
        return "操作完成";
    case WorldSaveError::ineligible:
        return "请先返回村庄并等待当前报告结束";
    case WorldSaveError::too_large:
        return "存档超过大小限制";
    case WorldSaveError::malformed:
        return "存档损坏或内容不完整";
    case WorldSaveError::unsupported_version:
        return "存档版本不受支持";
    case WorldSaveError::dataset_mismatch:
        return "存档与当前游戏数据不匹配";
    case WorldSaveError::invalid_world:
        return "存档中的世界数据无效";
    case WorldSaveError::io_error:
        return "无法读取或写入存档文件";
    }
    return "存档操作失败";
}
std::filesystem::path default_world_save_directory() {
#ifdef _WIN32
    const auto required = GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
    if (!required)
        throw std::runtime_error("LOCALAPPDATA is unavailable");
    std::wstring value(required, L'\0');
    const auto length = GetEnvironmentVariableW(L"LOCALAPPDATA", value.data(), required);
    if (!length || length >= required)
        throw std::runtime_error("LOCALAPPDATA could not be read");
    value.resize(length);
    return std::filesystem::path(value) / L"Ark-Village" / L"saves";
#else
    const auto *base = std::getenv("XDG_DATA_HOME");
    if (base && *base)
        return std::filesystem::path(base) / "Ark-Village" / "saves";
    const auto *home = std::getenv("HOME");
    if (!home || !*home)
        throw std::runtime_error("User data directory is unavailable");
    return std::filesystem::path(home) / ".local" / "share" / "Ark-Village" / "saves";
#endif
}
std::filesystem::path world_save_slot_path(const std::filesystem::path &directory, int slot) {
    if (slot < 0 || slot >= 2 || directory.empty())
        throw std::invalid_argument("Invalid save slot or directory");
    return directory / ("manual-" + std::to_string(slot + 1) + ".ark");
}
WorldSaveCandidate read_world_save_slot(const std::filesystem::path &directory, int slot) {
    try {
        const auto path = world_save_slot_path(directory, slot);
        const auto size = std::filesystem::file_size(path);
        if (size > world_save_max_bytes)
            return {WorldSaveError::too_large, "存档超过大小限制", {}, {}};
        std::ifstream file(path, std::ios::binary);
        if (!file)
            return {WorldSaveError::io_error, "无法打开存档", {}, {}};
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        if (!file.read(reinterpret_cast<char *>(bytes.data()),
                       static_cast<std::streamsize>(size)) ||
            file.peek() != std::char_traits<char>::eof())
            return {WorldSaveError::io_error, "存档读取不完整", {}, {}};
        return decode_world_save(bytes);
    } catch (const std::exception &) {
        return {WorldSaveError::io_error, "无法读取存档文件", {}, {}};
    }
}
std::array<WorldSaveSlotInfo, 2> inspect_world_save_slots(const std::filesystem::path &directory) {
    std::array<WorldSaveSlotInfo, 2> slots;
    for (int i = 0; i < 2; ++i) {
        std::error_code error;
        slots[i].exists = std::filesystem::exists(world_save_slot_path(directory, i), error);
        if (error) {
            slots[i].error = WorldSaveError::io_error;
            slots[i].message = "无法检查存档";
        } else if (slots[i].exists) {
            auto candidate = read_world_save_slot(directory, i);
            slots[i].metadata = std::move(candidate.metadata);
            slots[i].error = candidate.error;
            slots[i].message = candidate.error == WorldSaveError::none
                                   ? std::string{}
                                   : world_save_error_text(candidate.error);
        }
    }
    return slots;
}
WorldSaveFileResult write_world_save_slot(const std::filesystem::path &directory, int slot,
                                          const WorldSaveImage &image) {
    // Serialize local writers; each uses an exclusively created same-directory temporary file.
    static std::mutex writers;
    std::lock_guard<std::mutex> lock(writers);
    if (image.bytes.size() > world_save_max_bytes)
        return {WorldSaveError::too_large, "存档超过大小限制"};
    const auto checked = decode_world_save(image.bytes);
    if (!checked.state)
        return {checked.error, checked.message};
    std::filesystem::path temporary;
    bool owns_temporary{};
    try {
        const auto target = world_save_slot_path(directory, slot);
        std::filesystem::create_directories(directory);
#ifdef _WIN32
        HANDLE handle = INVALID_HANDLE_VALUE;
        for (unsigned attempt = 0; attempt < 100 && handle == INVALID_HANDLE_VALUE; ++attempt) {
            temporary = target;
            temporary +=
                L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(attempt);
            handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                 FILE_ATTRIBUTE_NORMAL, nullptr);
            if (handle == INVALID_HANDLE_VALUE && GetLastError() != ERROR_FILE_EXISTS)
                break;
        }
        if (handle == INVALID_HANDLE_VALUE)
            return {WorldSaveError::io_error, "无法创建临时存档"};
        owns_temporary = true;
        DWORD written{};
        const bool flushed = WriteFile(handle, image.bytes.data(),
                                       static_cast<DWORD>(image.bytes.size()), &written, nullptr) &&
                             written == image.bytes.size() && FlushFileBuffers(handle);
        const bool closed = CloseHandle(handle) != 0;
        if (!flushed || !closed ||
            !MoveFileExW(temporary.c_str(), target.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return {WorldSaveError::io_error, "写入失败，原存档已保留"};
        }
#else
        std::string pattern = target.string() + ".tmp-XXXXXX";
        std::vector<char> name(pattern.begin(), pattern.end());
        name.push_back('\0');
        const int descriptor = mkstemp(name.data());
        if (descriptor < 0)
            return {WorldSaveError::io_error, "无法创建临时存档"};
        temporary = name.data();
        owns_temporary = true;
        std::size_t offset{};
        while (offset < image.bytes.size()) {
            const auto written =
                ::write(descriptor, image.bytes.data() + offset, image.bytes.size() - offset);
            if (written <= 0)
                break;
            offset += static_cast<std::size_t>(written);
        }
        const bool flushed = offset == image.bytes.size() && fsync(descriptor) == 0;
        const bool closed = ::close(descriptor) == 0;
        if (!flushed || !closed || std::rename(temporary.c_str(), target.c_str()) != 0) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            return {WorldSaveError::io_error, "写入失败，原存档已保留"};
        }
#endif
        return {};
    } catch (const std::exception &) {
        if (owns_temporary) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
        }
        return {WorldSaveError::io_error, "写入失败，原存档已保留"};
    }
}
} // namespace ark::app
