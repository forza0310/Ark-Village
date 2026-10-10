#include "crash_report.hpp"
#include "ark_library_contract.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace ark::desktop {
namespace {
std::atomic<CrashReporter *> current_reporter{};
constexpr std::size_t text_limit = 64 * 1024;
constexpr char marker[] = "Ark-Village text diagnostics v1\n";
#ifdef _WIN32
void write(HANDLE file, std::string_view text) noexcept {
    if (file == INVALID_HANDLE_VALUE)
        return;
    LARGE_INTEGER zero{}, position{};
    if (!SetFilePointerEx(file, zero, &position, FILE_CURRENT) || position.QuadPart < 0 ||
        static_cast<std::uint64_t>(position.QuadPart) >= text_limit)
        return;
    const auto remaining = text_limit - static_cast<std::size_t>(position.QuadPart);
    DWORD written{};
    WriteFile(file, text.data(), static_cast<DWORD>(std::min(text.size(), remaining)), &written,
              nullptr);
}
void reset(HANDLE file) noexcept {
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    SetEndOfFile(file);
}
void close(HANDLE &file) noexcept {
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
    file = INVALID_HANDLE_VALUE;
}
HANDLE open(const std::filesystem::path &file, DWORD sharing = FILE_SHARE_READ) {
    return CreateFileW(file.c_str(), GENERIC_WRITE, sharing, nullptr, CREATE_NEW,
                       FILE_ATTRIBUTE_NORMAL, nullptr);
}
// Remove only our marked, finished report directories. Never follow directory links
// or touch a live process's exclusively held lock file.
std::filesystem::path prune(const std::filesystem::path &root, const std::filesystem::path &self) {
    std::vector<std::filesystem::directory_entry> reports;
    for (const auto &entry : std::filesystem::directory_iterator(root)) {
        if (entry.path() == self || entry.is_symlink() || !entry.is_directory() ||
            entry.path().filename().string().rfind("ark-", 0) != 0)
            continue;
        std::ifstream input(entry.path() / "owner.txt", std::ios::binary);
        std::string tag(sizeof(marker) - 1, '\0');
        input.read(tag.data(), static_cast<std::streamsize>(tag.size()));
        if (!input || tag != marker)
            continue;
        HANDLE lock = CreateFileW((entry.path() / "active.lock").c_str(), GENERIC_READ, 0, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (lock == INVALID_HANDLE_VALUE)
            continue;
        CloseHandle(lock);
        bool known = true;
        for (const auto &file : std::filesystem::directory_iterator(entry.path())) {
            const auto name = file.path().filename().string();
            if (file.is_symlink() || !file.is_regular_file() ||
                (name != "owner.txt" && name != "active.lock" && name != "recent-0.log" &&
                 name != "recent-1.log" && name != "context-0.txt" && name != "context-1.txt" &&
                 name != "report.txt")) {
                known = false;
                break;
            }
        }
        if (!known)
            continue;
        reports.push_back(entry);
    }
    std::sort(reports.begin(), reports.end(), [](const auto &a, const auto &b) {
        return a.last_write_time() > b.last_write_time();
    });
    // Each directory is limited to seven small text/lock files (<384KiB), so five
    // retained groups are also strictly below the advertised 5MiB aggregate limit.
    for (std::size_t i = 5; i < reports.size(); ++i) {
        for (const auto *name : {"owner.txt", "active.lock", "recent-0.log", "recent-1.log",
                                 "context-0.txt", "context-1.txt", "report.txt"})
            std::filesystem::remove(reports[i].path() / name);
        std::filesystem::remove(reports[i].path()); // Refuse nonempty directories, never recurse.
    }
    // Reserve eviction until an actual report exists; ordinary launches must not erase history.
    return reports.size() >= 5 ? reports[4].path() : std::filesystem::path{};
}
// x64 unwind metadata is already in the loaded images. No symbol-server access,
// extra DLL, heap allocation or game mutex is required on the exception path.
void stack(HANDLE output, CONTEXT context) noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    for (unsigned frame = 0; frame < 64 && context.Rip; ++frame) {
        MEMORY_BASIC_INFORMATION memory{};
        wchar_t full_name[1024]{};
        char module[1024]{}, line[1280]{};
        DWORD64 base{};
        if (VirtualQuery(reinterpret_cast<void *>(context.Rip), &memory, sizeof(memory)) &&
            memory.Type == MEM_IMAGE) {
            base = reinterpret_cast<DWORD64>(memory.AllocationBase);
            GetModuleFileNameW(reinterpret_cast<HMODULE>(memory.AllocationBase), full_name, 1024);
            const wchar_t *name = full_name;
            for (const wchar_t *p = full_name; *p; ++p)
                if (*p == L'\\' || *p == L'/')
                    name = p + 1;
            WideCharToMultiByte(CP_UTF8, 0, name, -1, module, sizeof(module), nullptr, nullptr);
        }
        const auto count = std::snprintf(line, sizeof(line), "#%u %s +0x%llx pc=0x%llx\n", frame,
                                         *module ? module : "<unknown>",
                                         static_cast<unsigned long long>(context.Rip - base),
                                         static_cast<unsigned long long>(context.Rip));
        if (count > 0)
            write(output, {line, std::min<std::size_t>(count, sizeof(line) - 1)});
        const auto old_pc = context.Rip, old_sp = context.Rsp;
        DWORD64 image{};
        auto *function = RtlLookupFunctionEntry(context.Rip, &image, nullptr);
        if (function) {
            void *handler_data{};
            DWORD64 establisher{};
            RtlVirtualUnwind(UNW_FLAG_NHANDLER, image, context.Rip, function, &context,
                             &handler_data, &establisher, nullptr);
        } else {
            SIZE_T bytes{};
            if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void *>(context.Rsp),
                                   &context.Rip, sizeof(context.Rip), &bytes) ||
                bytes != sizeof(context.Rip))
                break;
            context.Rsp += sizeof(context.Rip);
        }
        if (context.Rip == old_pc || context.Rsp <= old_sp ||
            context.Rsp - old_sp > 16 * 1024 * 1024)
            break;
    }
#else
    write(output, "Native stack walking is supported for the Windows x64 product.\n");
#endif
}
#endif
} // namespace

class CrashReporter::Impl {
  public:
    class Tee : public std::streambuf {
      public:
        Tee(Impl &owner, std::streambuf *original) : owner_(owner), original_(original) {}
        std::streambuf *original() const { return original_; }

      private:
        std::streamsize xsputn(const char *data, std::streamsize size) override {
            owner_.log({data, static_cast<std::size_t>(size)});
            return original_->sputn(data, size);
        }
        int_type overflow(int_type value) override {
            if (traits_type::eq_int_type(value, traits_type::eof()))
                return traits_type::not_eof(value);
            const char c = traits_type::to_char_type(value);
            return xsputn(&c, 1) == 1 ? value : traits_type::eof();
        }
        int sync() override { return original_->pubsync(); }
        Impl &owner_;
        std::streambuf *original_;
    };
    Impl() : out(*this, std::cout.rdbuf()), err(*this, std::cerr.rdbuf()) {}
    void initialize(std::filesystem::path root) {
#ifdef _WIN32
        if (root.empty())
            root = std::filesystem::temp_directory_path() / "Ark-Village" / "diagnostics";
        std::filesystem::create_directories(root);
        root = std::filesystem::absolute(root);
        char name[100]{};
        FILETIME now{};
        GetSystemTimeAsFileTime(&now);
        std::snprintf(name, sizeof(name), "ark-%08lx%08lx-%lu", now.dwHighDateTime,
                      now.dwLowDateTime, GetCurrentProcessId());
        directory = std::filesystem::absolute(root / name);
        if (!std::filesystem::create_directory(directory))
            return;
        owns_directory = true;
        std::ofstream(directory / "owner.txt", std::ios::binary) << marker;
        lock = open(directory / "active.lock", 0);
        report_file = open(directory / "report.txt");
        for (int n = 0; n < 2; ++n) {
            logs[n] = open(directory / ("recent-" + std::to_string(n) + ".log"));
            contexts[n] = open(directory / ("context-" + std::to_string(n) + ".txt"));
        }
        enabled = lock != INVALID_HANDLE_VALUE && report_file != INVALID_HANDLE_VALUE &&
                  logs[0] != INVALID_HANDLE_VALUE && logs[1] != INVALID_HANDLE_VALUE &&
                  contexts[0] != INVALID_HANDLE_VALUE && contexts[1] != INVALID_HANDLE_VALUE;
        if (!enabled)
            return;
        try {
            obsolete_directory = prune(root, directory);
            if (!obsolete_directory.empty()) {
                for (const auto *file : {"recent-0.log", "recent-1.log", "context-0.txt",
                                         "context-1.txt", "report.txt", "owner.txt", "active.lock"})
                    obsolete_files.push_back(obsolete_directory / file);
            }
        } catch (...) {
        } // Retention failure cannot block startup.
        DWORD guarantee = 64 * 1024;
        SetThreadStackGuarantee(&guarantee);
        previous_filter = SetUnhandledExceptionFilter(unhandled);
        previous_terminate = std::set_terminate(terminate);
        std::cout.rdbuf(&out);
        std::cerr.rdbuf(&err);
        log("Ark-Village diagnostics; DLL contract " ARK_LIBRARY_CONTRACT "\n");
#endif
    }
    ~Impl() {
#ifdef _WIN32
        if (enabled) {
            std::cout.rdbuf(out.original());
            std::cerr.rdbuf(err.original());
            SetUnhandledExceptionFilter(previous_filter);
            std::set_terminate(previous_terminate);
        }
        close(report_file);
        close(lock);
        for (auto &file : logs)
            close(file);
        for (auto &file : contexts)
            close(file);
        if (!retained && owns_directory) {
            std::error_code error;
            for (const auto *name : {"owner.txt", "active.lock", "recent-0.log", "recent-1.log",
                                     "context-0.txt", "context-1.txt", "report.txt"})
                std::filesystem::remove(directory / name, error);
            std::filesystem::remove(directory, error);
        }
#endif
    }
    void log(std::string_view text) noexcept {
#ifdef _WIN32
        if (!enabled)
            return;
        try {
            std::lock_guard<std::mutex> guard(writer);
            if (text.size() > text_limit)
                text.remove_prefix(text.size() - text_limit);
            if (log_bytes + text.size() > text_limit) {
                log_index ^= 1;
                reset(logs[log_index]);
                log_bytes = 0;
            }
            write(logs[log_index], text);
            log_bytes += text.size();
        } catch (...) {
        }
#endif
    }
    void context(std::string_view text) noexcept {
#ifdef _WIN32
        if (!enabled)
            return;
        try {
            std::lock_guard<std::mutex> guard(writer);
            context_index ^= 1;
            reset(contexts[context_index]);
            write(contexts[context_index], text);
        } catch (...) {
        }
#endif
    }
#ifdef _WIN32
    void report(std::string_view kind, std::string_view message, CONTEXT *context,
                DWORD code = 0) noexcept {
        if (!enabled || reporting.test_and_set())
            return;
        retained = true;
        // A dedicated preopened handle avoids the logging/world locks on a fatal path.
        reset(report_file);
        char header[256]{};
        SYSTEMTIME now{};
        GetSystemTime(&now);
        const int count = std::snprintf(header, sizeof(header),
                                        "Ark-Village diagnostic\nUTC=%04u-%02u-%02uT%02u:%02u:%02u "
                                        "thread=%lu exception=0x%08lx\n",
                                        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute,
                                        now.wSecond, GetCurrentThreadId(), code);
        if (count > 0)
            write(report_file, {header, static_cast<std::size_t>(count)});
        write(report_file, "kind=");
        write(report_file, kind.substr(0, 128));
        write(report_file, "\nmessage=");
        write(report_file, message.substr(0, 4096));
        write(report_file, "\nlibrary=" ARK_LIBRARY_CONTRACT "\n");
        write(report_file, "executable-build=" __DATE__ " " __TIME__ "\n");
        write(report_file, "Stacks for explicit reports are reporting-site stacks; only "
                           "windows-unhandled uses the fault context.\n");
        write(report_file,
              "Recent text: recent-0.log/recent-1.log; snapshots: context-0.txt/context-1.txt.\n");
        if (context)
            stack(report_file, *context);
        FlushFileBuffers(report_file);
        // Paths were prepared while healthy. The fatal path performs no filesystem allocation.
        // A live or externally changed directory is preserved; cleanup remains best effort.
        if (!obsolete_directory.empty() && obsolete_files.size() == 7) {
            bool safe = true;
            for (const auto &file : obsolete_files) {
                const auto attributes = GetFileAttributesW(file.c_str());
                if (attributes == INVALID_FILE_ATTRIBUTES ||
                    (attributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)))
                    safe = false;
            }
            const auto attributes = GetFileAttributesW(obsolete_directory.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES ||
                (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
                safe = false;
            if (safe) {
                HANDLE old_lock =
                    CreateFileW(obsolete_files.back().c_str(), GENERIC_READ, 0, nullptr,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (old_lock != INVALID_HANDLE_VALUE) {
                    CloseHandle(old_lock);
                    for (const auto &file : obsolete_files)
                        DeleteFileW(file.c_str());
                    RemoveDirectoryW(obsolete_directory.c_str());
                }
            }
        }
        reporting.clear();
    }
    static LONG WINAPI unhandled(EXCEPTION_POINTERS *exception) noexcept {
        const auto reporter = current_reporter.load();
        if (reporter && reporter->impl_ && exception)
            reporter->impl_->report("windows-unhandled", "fault-thread exception context",
                                    exception->ContextRecord,
                                    exception->ExceptionRecord->ExceptionCode);
        return EXCEPTION_EXECUTE_HANDLER;
    }
    [[noreturn]] static void terminate() noexcept {
        CONTEXT context{};
        RtlCaptureContext(&context);
        const auto reporter = current_reporter.load();
        if (reporter && reporter->impl_)
            reporter->impl_->report("std-terminate",
                                    "terminate-handler stack; not an original throw-site guarantee",
                                    &context);
        TerminateProcess(GetCurrentProcess(), 0xE0000001U);
        std::abort();
    }
    HANDLE lock{INVALID_HANDLE_VALUE}, report_file{INVALID_HANDLE_VALUE};
    HANDLE logs[2]{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE},
        contexts[2]{INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE};
    LPTOP_LEVEL_EXCEPTION_FILTER previous_filter{};
    std::terminate_handler previous_terminate{};
#endif
    std::filesystem::path directory;
    std::filesystem::path obsolete_directory;
    std::vector<std::filesystem::path> obsolete_files;
    bool enabled{}, owns_directory{};
    std::atomic<bool> retained{};
    std::atomic_flag reporting = ATOMIC_FLAG_INIT;
    std::mutex writer;
    int log_index{}, context_index{};
    std::size_t log_bytes{};
    Tee out, err;
};

CrashReporter::CrashReporter(std::filesystem::path root) noexcept {
    if (current_reporter.load())
        return;
    try {
        impl_ = std::make_unique<Impl>();
        impl_->initialize(std::move(root));
        if (impl_->enabled)
            current_reporter = this;
    } catch (...) {
        impl_.reset();
    }
}
CrashReporter::~CrashReporter() {
    if (current_reporter == this)
        current_reporter = nullptr;
}
bool CrashReporter::ready() const noexcept { return impl_ && impl_->enabled; }
std::filesystem::path CrashReporter::directory() const {
    return impl_ ? impl_->directory : std::filesystem::path{};
}
CrashReporter *CrashReporter::current() noexcept { return current_reporter.load(); }
void CrashReporter::log(std::string_view text) noexcept {
    if (impl_)
        impl_->log(text);
}
void CrashReporter::context(std::string_view text) noexcept {
    if (impl_)
        impl_->context(text);
}
void CrashReporter::report(std::string_view kind, std::string_view message) noexcept {
#ifdef _WIN32
    if (!impl_)
        return;
    CONTEXT context{};
    RtlCaptureContext(&context);
    impl_->report(kind, message, &context);
#endif
}
void diagnostic_log(std::string_view text) noexcept {
    if (auto *r = CrashReporter::current())
        r->log(text);
}
} // namespace ark::desktop
