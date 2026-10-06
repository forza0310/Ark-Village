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
    int tick_rate = 0; // Zero selects researched 47 ms pacing; explicit 1..240 is an experiment.
    std::string screenshot;
    bool paused = false;
    bool ai_preview = false;
    bool world = true; // Default canonical world; explicit legacy diagnostics select Game instead.
    bool verify_play = false; // Bounded normal-game controller probe; never an inspection fixture.
    std::string font;
    std::string save_directory; // Optional explicit player/test directory; default is user-local.
    std::string inspect_page;   // Bounded rendering inspection; never a normal new-game trajectory.
};

struct LaunchResult {
    std::optional<LaunchOptions> options;
    std::string error;
};

LaunchResult parse_arguments(const std::vector<std::string> &arguments);

} // namespace ark::app
