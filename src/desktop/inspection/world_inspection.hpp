#pragma once

#include "ark/app/bootstrap/launch_options.hpp"
#include "ark/app/session/world_session.hpp"
#include "world_management_inspection.hpp"
#include <filesystem>
#include <memory>
#include <optional>

namespace ark::desktop {
// These synchronous branches are diagnostics only. Normal gameplay commits through WorldSession.
void prepare_world_inspection(simulation::StartupWorldRuntimeState &state,
                              const app::LaunchOptions &options, bool transient,
                              WorldManagementInspection &management,
                              std::optional<simulation::StartupWorldRuntimeState> *checkpoint,
                              WorldManagementInspection *inspection_checkpoint);

// Exercises real asynchronous save/load commands against isolated files; never opens player slots
// implicitly. Pending command ownership stays with the normal window controller.
class WorldSaveInspection {
  public:
    explicit WorldSaveInspection(const app::LaunchOptions &options);
    const std::filesystem::path &directory() const { return directory_; }
    bool ready() const { return ready_; }
    void observe(const app::WorldFrame &publication, app::WorldSession &session,
                 std::uint64_t &pending_menu, double elapsed);

  private:
    std::string mode_;
    std::filesystem::path directory_;
    int stage_{};
    bool enabled_{}, ready_{};
    std::shared_ptr<const simulation::StartupWorldRuntimeState> frozen_;
    std::optional<app::WorldSaveMetadata> load_metadata_;
};

// Requires the requested bounded run to finish and rejects blank captures.
void capture_world_screenshot(const app::LaunchOptions &options, int frames);
} // namespace ark::desktop
