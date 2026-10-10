// Process entry: arguments, CPU asset/model validation, then the raylib adapter.
#include "ark/app/launch_options.hpp"
#include "ark/simulation/startup_world_runtime.hpp"
#include "resources.hpp"
#include "world_view.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#endif

namespace {
void report_error(const std::string &message) {
    const auto line = "Ark-Village: " + message + '\n';
#ifdef _WIN32
    // A Windows console may still use CP936. Write Unicode directly without
    // changing the caller's code page; redirected diagnostics remain UTF-8.
    const auto handle = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode{};
    if (GetConsoleMode(handle, &mode)) {
        const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line.data(),
                                               static_cast<int>(line.size()), nullptr, 0);
        if (length > 0) {
            std::wstring wide(static_cast<std::size_t>(length), L'\0');
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, line.data(),
                                static_cast<int>(line.size()), wide.data(), length);
            DWORD written{};
            if (WriteConsoleW(handle, wide.data(), static_cast<DWORD>(wide.size()), &written,
                              nullptr))
                return;
        }
    }
#endif
    std::cerr << line;
}
} // namespace

int main(int argc, char **argv) {
    try {
        const auto parsed = ark::app::parse_arguments({argv + 1, argv + argc});
        if (!parsed.options)
            throw std::runtime_error(parsed.error);
        const auto &options = *parsed.options;
        if (options.mode == ark::app::LaunchMode::help) {
            std::cout << "ark_village [--check|--help] [--world] [--paused] [--font TTF] "
                         "[--save-dir PATH] [--size W H] [--frames N] "
                         "[--zoom-percent 25..200] [--screenshot PNG] "
                         "[--inspect-page world-PAGE]\n"
                         "Default: title -> new or saved continuous world. --world is an alias.\n"
                         "--check validates packaged resources and the complete world startup.\n"
                         "--inspect-page world-title|world-title-slots|world-title-actions "
                         "captures the title flow without updating the world.\n"
                         "--inspect-page world-commerce-suite --frames N --screenshot prefix.png "
                         "captures six commerce pages after one natural preparation.\n";
            return 0;
        }
        SetTraceLogLevel(LOG_WARNING);
        const auto assets = std::filesystem::path(GetApplicationDirectory()) / "assets";
        ark::desktop::check_assets(assets);
        if (options.mode == ark::app::LaunchMode::check) {
            using namespace ark::simulation;
            // The bootstrap is discarded on return; the runtime is the only live owner.
            auto session = [] {
                StartupSession bootstrap;
                return StartupWorldRuntimeSession(bootstrap.state(),
                                                  rules::WorldRandomStream::from_java_seed(1));
            }();
            const auto result = session.update();
            if (result.error != StartupWorldRuntimeError::none)
                throw std::runtime_error("Complete world startup check failed");
            std::cout << "PASS packaged complete world, funds="
                      << session.state().scene.world.world.ai.accounting.funds() << '\n';
        } else {
            ark::desktop::run_world_game(options, assets);
        }
        return 0;
    } catch (const std::exception &error) {
        report_error(error.what());
        return 1;
    }
}
