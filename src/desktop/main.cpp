// Process entry: arguments, CPU asset/model validation, then the raylib adapter.
#include "ark/app/game.hpp"
#include "ark/app/initial_ai_check.hpp"
#include "ark/app/launch_options.hpp"
#include "ark/simulation/startup_world_runtime.hpp"
#include "game_view.hpp"
#include "resources.hpp"
#include "world_view.hpp"
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
            std::cout << "ark_village [--check|--check-ai] [--world|--legacy-slice|--ai-preview] "
                         "[--paused] "
                         "[--font TTF] "
                         "[--size W H] "
                         "[--frames N] [--verify-play] [--tick-rate 1..240] "
                         "[--zoom-percent 50..200] [--screenshot PNG] [--inspect-page "
                         "menu|shops|plants|food|placement|detail|bonuses|equipment|booster|"
                         "arrival|visitor|motion|ai|world-active|world-month|world-rank]\n"
                         "Default: continuous world. --world is an explicit alias.\n"
                         "--legacy-slice opens the former construction slice. --check-ai, "
                         "--ai-preview, --verify-play and legacy inspection pages select their "
                         "diagnostic slice explicitly. --tick-rate is a legacy-only experiment.\n";
            return 0;
        }
        SetTraceLogLevel(LOG_WARNING);
        const auto assets = std::filesystem::path(GetApplicationDirectory()) / "assets";
        ark::desktop::check_assets(assets);
        if (options.world) {
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
        }
        if (options.mode == ark::app::LaunchMode::check_ai) {
            for (const auto &c : ark::app::check_initial_ai())
                std::cout << "PASS initial AI birth=" << c.birth.x << ',' << c.birth.y
                          << " first_facility=" << c.first_facility << " rounds=" << c.rounds
                          << " funds=" << c.funds << '\n';
            return 0;
        }
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
