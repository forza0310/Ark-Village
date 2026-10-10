#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace ark::app {
struct WorldFrame;
}
namespace ark::desktop {
// Process-scoped Windows text diagnostics. No world ownership, save writing or network.
// Platform failures disable diagnostics without changing the game's error handling.
class CrashReporter {
  public:
    explicit CrashReporter(std::filesystem::path root = {}) noexcept;
    ~CrashReporter();
    CrashReporter(const CrashReporter &) = delete;
    CrashReporter &operator=(const CrashReporter &) = delete;
    bool ready() const noexcept;
    std::filesystem::path directory() const;
    void log(std::string_view text) noexcept;
    void context(std::string_view text) noexcept;
    void report(std::string_view kind, std::string_view message) noexcept;
    static CrashReporter *current() noexcept;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
void diagnostic_log(std::string_view text) noexcept;
// Called from the desktop reader; periodic context is bounded independently of draw FPS.
class WorldDiagnostics {
  public:
    void observe(const app::WorldFrame &frame) noexcept;

  private:
    std::chrono::steady_clock::time_point next_{};
    std::uint64_t generation_{};
    bool reported_failure_{};
};
} // namespace ark::desktop
