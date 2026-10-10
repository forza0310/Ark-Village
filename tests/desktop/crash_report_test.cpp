// Real OS exceptions must run in child processes: the test runner and player saves survive.
#include "ark/app/session/world_session.hpp"
#include "platform/crash_report.hpp"
#include "support/world_fixture.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
namespace fs = std::filesystem;
using ark::desktop::CrashReporter;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
std::string read(const fs::path &path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::vector<fs::path> groups(const fs::path &root) {
    std::vector<fs::path> result;
    for (const auto &entry : fs::directory_iterator(root))
        if (entry.is_directory() && entry.path().filename().string().rfind("ark-", 0) == 0)
            result.push_back(entry.path());
    return result;
}
// Only the handle of the child created here is ever terminated, and only after timeout.
DWORD run(const fs::path &exe, const fs::path &root, const wchar_t *mode) {
    auto command = L"\"" + exe.wstring() + L"\" --child " + mode + L" \"" + root.wstring() + L"\"";
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child{};
    require(CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                           nullptr, nullptr, &startup, &child),
            "CreateProcess failed");
    CloseHandle(child.hThread);
    const auto wait = WaitForSingleObject(child.hProcess, 10000);
    if (wait != WAIT_OBJECT_0) {
        TerminateProcess(child.hProcess, 99);
        WaitForSingleObject(child.hProcess, 5000);
    }
    DWORD status{};
    GetExitCodeProcess(child.hProcess, &status);
    CloseHandle(child.hProcess);
    require(wait == WAIT_OBJECT_0, "Diagnostic child timed out");
    return status;
}
int child(const std::string &mode, const fs::path &root) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    CrashReporter reporter(root);
    if (mode == "unwritable") {
        require(!reporter.ready(), "Invalid root should disable diagnostics");
        return 0;
    }
    require(reporter.ready(), "Reporter failed to initialize");
    std::cout << "nearby-stdout\n";
    std::cerr << "nearby-stderr\n";
    reporter.context("nearby-world-context\n");
    if (mode == "normal")
        return 0;
    if (mode == "access") {
        std::thread worker([&] {
            reporter.context("fault-thread=" + std::to_string(GetCurrentThreadId()) + "\n");
            auto *address = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_NOACCESS);
            if (!address)
                std::terminate();
            *static_cast<volatile int *>(address) = 42;
        });
        worker.join();
        throw std::runtime_error("Access violation unexpectedly returned");
    }
    if (mode == "terminate")
        std::terminate();
    if (mode == "runtime") {
        ark::desktop::WorldDiagnostics observer;
        ark::app::WorldFrame frame;
        frame.state = std::make_shared<const ark::app::WorldState>(ark::test::initial_world());
        frame.failed = true;
        frame.error = "stage=projectile id=71 detail=6";
        for (unsigned n = 1; n <= 64; ++n) {
            ark::app::WorldCommandResult receipt;
            receipt.serial = n;
            frame.command_results.push_back(receipt);
        }
        observer.observe(frame); // Revision zero must also report a failure.
        const auto before = read(reporter.directory() / "report.txt");
        observer.observe(frame);
        require(read(reporter.directory() / "report.txt") == before,
                "Repeated snapshot rewrote report");
        return 0;
    }
    if (mode == "large") {
        for (int n = 0; n < 6; ++n)
            reporter.log(std::string(64000, char('a' + n)));
        reporter.log("LATEST-LOG\n");
    }
    reporter.report("explicit-test", "reporting boundary, not a throw site");
    return 0;
}
void check_report(const fs::path &root, std::string_view kind) {
    const auto dirs = groups(root);
    require(dirs.size() == 1, "Expected one diagnostic group");
    const auto report = read(dirs[0] / "report.txt");
    require(report.find(kind) != std::string::npos, "Missing expected report kind");
    require(report.find("#0 ") != std::string::npos && report.find(".exe +0x") != std::string::npos,
            "Missing module/RVA stack");
    require(report.find("library=") != std::string::npos &&
                report.find("executable-build=") != std::string::npos,
            "Missing build identity");
    const auto logs = read(dirs[0] / "recent-0.log") + read(dirs[0] / "recent-1.log");
    require(logs.find("nearby-stdout") != std::string::npos &&
                logs.find("nearby-stderr") != std::string::npos,
            "Missing recent streams");
}
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc == 4 && std::string(argv[1]) == "--child")
            return child(argv[2], fs::u8path(argv[3]));
        wchar_t executable[32768]{};
        require(GetModuleFileNameW(nullptr, executable, 32768) != 0, "Executable path missing");
        const fs::path exe(executable);
        const auto root = exe.parent_path().parent_path() / "validation" /
                          ("crash-report-" + std::to_string(GetCurrentProcessId()) + "-" +
                           std::to_string(GetTickCount64()));
        require(fs::create_directories(root), "Test root already exists");
        require(run(exe, root / "normal", L"normal") == 0, "Normal child failed");
        require(groups(root / "normal").empty(), "Normal exit kept a report");
        std::ofstream(root / "file") << "not a directory";
        require(run(exe, root / "file", L"unwritable") == 0,
                "Unavailable directory blocked startup");
        require(run(exe, root / "access", L"access") == EXCEPTION_ACCESS_VIOLATION,
                "Wrong native exception exit");
        check_report(root / "access", "windows-unhandled");
        const auto access = groups(root / "access").at(0);
        const auto contexts = read(access / "context-0.txt") + read(access / "context-1.txt");
        const auto position = contexts.find("fault-thread=");
        require(position != std::string::npos, "Worker identity missing");
        const auto thread =
            contexts.substr(position + 13, contexts.find('\n', position) - position - 13);
        require(read(access / "report.txt").find("thread=" + thread + " ") != std::string::npos,
                "Report used observer thread instead of fault thread");
        require(run(exe, root / "terminate", L"terminate") == 0xE0000001U, "Wrong terminate exit");
        check_report(root / "terminate", "std-terminate");
        require(run(exe, root / "runtime", L"runtime") == 0, "Runtime rejection child failed");
        check_report(root / "runtime", "runtime-rejection");
        const auto runtime = groups(root / "runtime").at(0);
        const auto snapshot = read(runtime / "context-0.txt") + read(runtime / "context-1.txt");
        require(snapshot.find("command serial=1 ") != std::string::npos &&
                    snapshot.find("command serial=64 ") != std::string::npos &&
                    snapshot.find("stage=projectile id=71") != std::string::npos,
                "Runtime context/receipts missing");
        const auto retained = root / "retention";
        fs::create_directories(retained / "foreign");
        std::ofstream(retained / "foreign" / "keep.txt") << "foreign data";
        {
            CrashReporter live(retained);
            require(live.ready(), "Concurrent report setup failed");
            for (int n = 0; n < 8; ++n)
                require(run(exe, retained, L"large") == 0, "Retention child failed");
            require(fs::exists(live.directory() / "active.lock"),
                    "Retention deleted a live process group");
        }
        require(groups(retained).size() == 5, "Retention must keep five groups");
        require(run(exe, retained, L"normal") == 0 && groups(retained).size() == 5,
                "Normal launches must preserve previous reports");
        std::uintmax_t total{};
        for (const auto &directory : groups(retained)) {
            const auto logs = read(directory / "recent-0.log") + read(directory / "recent-1.log");
            require(logs.find("LATEST-LOG") != std::string::npos, "Rotation lost latest logs");
            for (const auto &file : fs::directory_iterator(directory)) {
                require(file.file_size() <= 65536, "Text file exceeded bound");
                total += file.file_size();
            }
        }
        require(total <= 5 * 1024 * 1024, "Aggregate retention exceeded 5MiB");
        require(read(retained / "foreign" / "keep.txt") == "foreign data",
                "Retention touched foreign data");
        fs::remove_all(root); // Only this newly created, independently named test tree.
        std::cout << "PASS Windows text crash reports, worker stack, bounded logs, context and "
                     "retention\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
