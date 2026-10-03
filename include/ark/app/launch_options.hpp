#pragma once

#include <optional>
#include <string>
#include <vector>

namespace ark::app {

enum class LaunchMode { window, check, help };

struct LaunchOptions {
    LaunchMode mode = LaunchMode::window;
    int width = 480;
    int height = 660;
    int frames = 0;
    std::string screenshot;
    bool paused = false;
    std::string font;
    std::string inspect_page; // Bounded rendering inspection; never a normal new-game trajectory.
};

struct LaunchResult {
    std::optional<LaunchOptions> options;
    std::string error;
};

LaunchResult parse_arguments(const std::vector<std::string> &arguments);

} // namespace ark::app
