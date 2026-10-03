// Process entry: arguments, CPU asset/model validation, then the raylib adapter.
#include "ark/app/game.hpp"
#include "ark/app/launch_options.hpp"
#include "game_view.hpp"
#include "resources.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    try {
        const auto parsed = ark::app::parse_arguments({argv + 1, argv + argc});
        if (!parsed.options)
            throw std::runtime_error(parsed.error);
        const auto &options = *parsed.options;
        if (options.mode == ark::app::LaunchMode::help) {
            std::cout << "ark_village [--check] [--paused] [--font TTF] [--size W H] [--frames N] "
                         "[--screenshot PNG] [--inspect-page shops|plants|food|arrival|visitor]\n";
            return 0;
        }
        SetTraceLogLevel(LOG_WARNING);
        const auto assets = std::filesystem::path(GetApplicationDirectory()) / "assets";
        ark::desktop::check_assets(assets);
        if (options.mode == ark::app::LaunchMode::check) {
            ark::app::Game game;
            for (int i = 0; i < 420; ++i)
                game.update();
            if (!game.state().adventurer || game.state().adventurer->definition_id != 1 ||
                game.state().event89_count != 1 || game.state().money != 5000)
                throw std::runtime_error("First-arrival model check failed");
            std::cout << "PASS packaged source assets, 24x24 map, money=5000, free first arrival "
                         "definition=1\n";
            return 0;
        }
        ark::desktop::run_game(options, assets);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Ark-Village: " << error.what() << '\n';
        return 1;
    }
}
