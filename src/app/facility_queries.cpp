// Read-only aggregation over the current finite village. Source roads are a snapshot, not the
// pending reconstructed post-init map. Recomputing avoids stale or duplicated modifier caches.
#include "ark/app/game.hpp"
#include <stdexcept>
namespace ark::app {
facilities::Neighbourhood Game::neighbourhood(facilities::InstanceId instance) const {
    const auto &data = startup_data();
    if (!state_.facilities.count(instance))
        throw std::out_of_range("Unknown neighbourhood instance");
    std::vector<world::Cell> roads;
    for (int y = 0; y < data.map.height; ++y)
        for (int x = 0; x < data.map.width; ++x) {
            const world::Cell cell{x, y};
            if (definition(display(data.map.cells[data.map.index(cell)].display_id).definition_id)
                        .kind == 6 &&
                !facility_at(cell))
                roads.push_back(cell);
        }
    return facilities::derive_neighbourhood(data.definitions, state_.facilities, roads, data.map)
        .at(instance);
}
facilities::EconomyValues
Game::facility_values(int definition_id, std::optional<facilities::InstanceId> instance) const {
    const auto &d = definition(definition_id);
    const auto &progress = state_.definition_progress.at(definition_id);
    facilities::EconomyInput input;
    input.level = progress.level;
    input.improvements = progress.improvements;
    input.completed_uses = progress.completed_uses;
    input.job_counts = startup_data().initial_job_counts;
    if (instance) {
        if (state_.facilities.at(*instance).definition_id != definition_id)
            throw std::invalid_argument("Facility query definition/instance mismatch");
        input = facilities::with_neighbours(input, neighbourhood(*instance));
    }
    return facilities::derive_economy(d.economy, input);
}
} // namespace ark::app
