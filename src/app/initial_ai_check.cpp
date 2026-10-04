#include "ark/app/initial_ai_check.hpp"
#include "ark/app/initial_ai.hpp"
#include <stdexcept>
namespace ark::app {
std::vector<InitialAiCheck> check_initial_ai() {
    Game game;
    for (int n = 0; n < 420; ++n)
        game.update();
    std::vector<InitialAiCheck> result;
    for (const auto birth : startup_data().spawn_points)
        for (const auto branch : {std::array<int, 3>{0, 5, 33}, {0, 0, 30}, {40, 0, 28}}) {
            InitialAiSession ai(game, birth);
            if (ai.round({branch[0], branch[1], 0, 0, {}}) != InitialAiError::none ||
                !ai.state().journey || ai.state().journey->binding.definition_id != branch[2])
                throw std::runtime_error("Initial AI choice did not match maintained source case");
            bool closed{};
            for (int n = 0; n < 999; ++n) {
                if (ai.round({0, 0, 0, 0, {}}) != InitialAiError::none)
                    throw std::runtime_error("Initial AI admitted round failed");
                const auto &s = ai.state();
                closed = s.completions == 1 && s.departures >= 2 &&
                         (branch[2] == 30   ? s.equipment_commits == 1
                          : branch[2] == 33 ? s.attribute_commits == 1
                                            : !s.presentation_requests.empty());
                if (closed) {
                    result.push_back({birth, branch[2], s.rounds, s.accounting.funds()});
                    break;
                }
            }
            if (!closed)
                throw std::runtime_error("Initial AI interval did not close within 1000 rounds");
        }
    return result;
}
} // namespace ark::app
