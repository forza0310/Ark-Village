#pragma once

#include "ark/app/bootstrap/launch_options.hpp"
#include <filesystem>

namespace ark::desktop {
// Default canonical-world window; no persistent Game/startup preview is scheduled beside it.
void run_world_game(const app::LaunchOptions &options, const std::filesystem::path &assets);
} // namespace ark::desktop
