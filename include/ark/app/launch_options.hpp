#pragma once

#include <optional>
#include <string>
#include <vector>

namespace ark::app {

enum class LaunchMode { window, check, check_ai, help };

struct LaunchOptions {
    LaunchMode mode = LaunchMode::window;
    int width = 1080;
    int height = 720;
    int frames = 0;
    int zoom_percent = 100;
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
