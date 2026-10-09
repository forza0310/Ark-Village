#include "startup_world_file_io.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace dungeon_village_prototype::persistence_detail {
std::vector<std::uint8_t> read_save_file(const std::filesystem::path &path, std::size_t limit) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
        throw std::runtime_error("无法打开存档");
    const auto length = input.tellg();
    if (length < 0 || static_cast<std::uint64_t>(length) > limit)
        throw std::runtime_error("存档长度超过预算");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bytes.data()),
                    static_cast<std::streamsize>(bytes.size())) ||
        input.peek() != std::char_traits<char>::eof())
        throw std::runtime_error("存档读取失败或读取期间长度改变");
    return bytes;
}

namespace {
void publish_save_file(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes,
                       bool replace_existing) {
    if (path.filename().empty() || std::filesystem::is_symlink(path))
        throw std::runtime_error("无效存档目标");
    static std::atomic<unsigned long long> sequence{};
    std::filesystem::path temporary;
#ifdef _WIN32
    HANDLE handle = INVALID_HANDLE_VALUE;
#else
    int handle = -1;
#endif
    // 独占创建防并行写者覆盖临时文件；不删除或先改名旧有效目标。
    for (int attempt = 0; attempt < 32; ++attempt) {
        temporary = path;
        temporary += ".tmp." +
                     std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                     "." + std::to_string(sequence++);
#ifdef _WIN32
        handle = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
        if (handle != INVALID_HANDLE_VALUE)
            break;
        if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS)
            throw std::runtime_error("无法创建临时存档");
#else
        handle = ::open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (handle >= 0)
            break;
        if (errno != EEXIST)
            throw std::runtime_error("无法创建临时存档");
#endif
    }
#ifdef _WIN32
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("临时存档名称冲突");
#else
    if (handle < 0)
        throw std::runtime_error("临时存档名称冲突");
#endif
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    } cleanup{temporary};
#ifdef _WIN32
    struct Close {
        HANDLE h;
        ~Close() {
            if (h != INVALID_HANDLE_VALUE)
                CloseHandle(h);
        }
    } close{handle};
    DWORD written{};
    if (bytes.size() > MAXDWORD ||
        !WriteFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) ||
        written != bytes.size() || !FlushFileBuffers(handle))
        throw std::runtime_error("写入或刷新存档失败");
    const bool closed = CloseHandle(handle) != 0;
    close.h = INVALID_HANDLE_VALUE;
    if (!closed)
        throw std::runtime_error("关闭存档失败");
    if (read_save_file(temporary, bytes.size()) != bytes)
        throw std::runtime_error("临时存档回读校验失败");
    // 不以预先exists检查代替原子无覆盖语义，竞争写者创建的目标也必须保留。
    const DWORD flags = MOVEFILE_WRITE_THROUGH |
                        (replace_existing ? MOVEFILE_REPLACE_EXISTING : 0);
    if (!MoveFileExW(temporary.c_str(), path.c_str(), flags))
        throw std::runtime_error(replace_existing ? "替换存档失败，原档保留"
                                                 : "新存档发布失败，已有目标保留");
#else
    struct Close {
        int h;
        ~Close() {
            if (h >= 0)
                ::close(h);
        }
    } close{handle};
    std::size_t offset{};
    while (offset < bytes.size()) {
        const auto count = ::write(handle, bytes.data() + offset, bytes.size() - offset);
        if (count < 0 && errno == EINTR)
            continue;
        if (count <= 0)
            throw std::runtime_error("写入存档失败");
        offset += static_cast<std::size_t>(count);
    }
    if (::fsync(handle) != 0)
        throw std::runtime_error("刷新存档失败");
    const int closed = ::close(handle);
    close.h = -1;
    if (closed != 0)
        throw std::runtime_error("关闭存档失败");
    if (read_save_file(temporary, bytes.size()) != bytes)
        throw std::runtime_error("临时存档回读校验失败");
    if (replace_existing)
        std::filesystem::rename(temporary, path);
    else {
        // 同目录硬链接发布具备EEXIST无覆盖保证；成功后仅由RAII移除临时名字。
        // 不在目标已发布后再执行可能抛错的动作，不把清理或掉电耐久性称为跨文件事务。
        if (::link(temporary.c_str(), path.c_str()) != 0)
            throw std::runtime_error("新存档发布失败，已有目标保留");
    }
#endif
}
} // namespace
void replace_save_file(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    publish_save_file(path, bytes, true);
}
void create_save_file(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    publish_save_file(path, bytes, false);
}
} // namespace dungeon_village_prototype::persistence_detail
