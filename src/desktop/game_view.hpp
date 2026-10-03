#pragma once

#include "ark/app/launch_options.hpp"
#include <filesystem>
namespace ark::desktop {
// Runs the bounded first-play adapter. Resource/window ownership remains RAII on all exits.
void run_game(const app::LaunchOptions &options, const std::filesystem::path &assets);
} // namespace ark::desktop
