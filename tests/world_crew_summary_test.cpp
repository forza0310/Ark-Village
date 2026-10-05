// Page31 must preserve initialized X and read actual H/I without replaying its initializer.
#include "ui/world_crew_summary.hpp"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
bool contains(Rectangle outer, Rectangle inner) {
    return inner.width > 0 && inner.height > 0 && inner.x >= outer.x && inner.y >= outer.y &&
           inner.x + inner.width <= outer.x + outer.width &&
           inner.y + inner.height <= outer.y + outer.height;
}
} // namespace
int main() {
    namespace ui = ark::desktop::ui;
    namespace sim = ark::simulation;
    sim::StartupWorldRules catalogue;
    sim::StartupWorldRuntimeState state;
    state.rules = &catalogue;
    for (const int id : {42, 7}) {
        sim::StartupWorldHuman human;
        human.identity = id;
        human.name = "human-" + std::to_string(id);
        catalogue.humans.push_back(human);
        state.scene.world.world.ai.battle.humans[id].task_kills = id == 42 ? 1 : 0;
        state.scene.world.world.ai.battle.humans[id].participant_downs = id == 42 ? 3 : 8;
    }
    state.participants = {7}; // Current participants are deliberately different from initialized X.
    check(!ui::world_crew_summary_view(state, 9).initialized && state.crew_summaries.empty(),
          "Reading an uninitialized result must not initialize it or copy current participants");
    state.crew_summaries[9] = {42, 7, 42};
    const auto draws = state.scene.random.draws();
    const auto funds = state.scene.world.world.ai.accounting.funds();
    const auto result = ui::world_crew_summary_view(state, 9);
    check(result.initialized && result.rows.size() == 3 && result.rows[0].definition == 42 &&
              result.rows[0].name == "human-42" && result.rows[1].definition == 7 &&
              result.rows[2].definition == 42,
          "Summary must resolve identities and preserve source order and duplicate participants");
    check(result.rows[0].kills == 1 && result.rows[1].kills == 0 && result.rows[2].kills == 1 &&
              result.rows[0].downs == 3 && result.rows[1].downs == 8,
          "Summary must retain special-task normalized kills and independent down counts");
    check(state.scene.random.draws() == draws &&
              state.scene.world.world.ai.accounting.funds() == funds &&
              state.scene.world.world.ai.battle.humans.at(42).task_kills == 1,
          "Repeated rows must not normalize kills again, draw random values or issue rewards");
    state.crew_summaries[10] = {};
    check(ui::world_crew_summary_view(state, 10).initialized,
          "An initialized empty roster differs from a page that has not initialized");
    state.crew_summaries[11] = {99};
    bool rejected{};
    try {
        (void)ui::world_crew_summary_view(state, 11);
    } catch (const std::invalid_argument &) {
        rejected = true;
    }
    check(rejected, "Unknown definitions must not be replaced with invented names or rows");
    for (const auto extent : {ark::desktop::Extent{240, 256}, ark::desktop::Extent{540, 360}}) {
        const auto layout = ui::world_crew_summary_layout(extent);
        check(contains({0, 24, static_cast<float>(extent.width), extent.height - 53.F},
                       layout.panel) &&
                  contains(layout.panel, layout.rows) && contains(layout.panel, layout.confirm),
              "Crew panel and commands must fit the supported viewport");
        check(ui::world_crew_summary_visible_rows(layout) == 5 &&
                  layout.rows.y + layout.rows.height + 5 < layout.confirm.y,
              "Five source rows must fit without covering confirmation");
    }
    std::cout << "PASS crew-summary identities, data and layout\n";
}
