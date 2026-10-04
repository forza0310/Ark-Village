// Current village -> temporary life-engine candidate. No durable map or cash mirror is kept.
#include "ark/app/game.hpp"
#include <limits>
#include <stdexcept>

namespace ark::app {
InitialAiSession InitialAiSession::from_village(const Game &game, const LifeActorState &actor) {
    InitialAiSession engine;
    engine.live_ = true;
    engine.layout_revision_ = game.state().layout_revision;
    static_cast<LifeActorState &>(engine.state_) = actor;
    engine.state_.accounting = game.state().accounting;
    engine.state_.next_cash_id = game.state().next_cash_id;
    engine.map_ = game.route_map();
    engine.instance_order_ = game.state().instance_order;
    const auto &data = startup_data();
    std::vector<world::Cell> roads;
    for (int y = 0; y < engine.map_.height; ++y)
        for (int x = 0; x < engine.map_.width; ++x) {
            const world::Cell cell{x, y};
            const auto &tile = engine.map_.cells[engine.map_.index(cell)];
            if (tile.category == world::RouteCategory::road && !tile.facility)
                roads.push_back(cell);
        }
    engine.neighbourhoods_ = facilities::derive_neighbourhood(
        data.definitions, game.state().facilities, roads, data.map);
    for (const auto &[id, instance] : game.state().facilities) {
        const auto &d = game.definition(instance.definition_id);
        const auto &runtime = game.state().facility_life.at(id);
        engine.state_.facilities.emplace(
            id, InitialAiFacility{{id, d.id, d.shape, instance.orientation, instance.anchor},
                                  runtime.sales,
                                  runtime.occupants,
                                  instance.remaining_ticks > 0 ? 0 : 1});
        const auto &shared = game.state().definition_progress.at(d.id);
        if (shared.completed_uses > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))
            throw std::overflow_error("Shared uses exceed life-engine counter range");
        engine.state_.uses[d.id] = {shared.level, static_cast<int>(shared.completed_uses),
                                    shared.upgrade_pending};
        facilities::EconomyInput input;
        input.level = shared.level;
        input.improvements = shared.improvements;
        input.completed_uses = shared.completed_uses;
        input.job_counts = data.initial_job_counts;
        engine.economy_inputs_[id] =
            facilities::with_neighbours(input, engine.neighbourhoods_.at(id));
    }
    return engine;
}
} // namespace ark::app
