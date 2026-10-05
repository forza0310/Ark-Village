#include "world_rest_visuals.hpp"
#include "ark/simulation/startup_world_visuals.hpp"

#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
namespace {
// Published B170 phase boundary, widths and image slices come from ui/PAGES.md. Coordinates
// inside the panel and its pink/green palette are explicit desktop layout choices, not a
// recovered c/m.L transform. e8f66d9 proves the actual portrait crop and current shared HP.
OverlayPlan rest_plan(const simulation::rules::BattleActorRecord &actor, int capacity,
                      const simulation::StartupPortrait &portrait, float y) {
    if (actor.state_counter < 0)
        throw std::invalid_argument("Inn rest display requires a nonnegative actor counter");
    if (capacity <= 0)
        throw std::invalid_argument("Inn HP display requires positive shared capacity");
    OverlayPlan plan;
    // walk01 frame0 source(0,24), offset(-9,-24), crop anchor(8,21): PNG(1,27).
    const OverlayPortrait face{portrait.image, {1, 27, 15, 14}, {1, y + 1, 15, 14}};
    if (actor.state_counter < 170) {
        plan.push_back(OverlayImage{"restBar00.png", {0, 0, 48, 17}, {0, y, 48, 17}});
        const int width = actor.state_counter * 30 / 170;
        if (width > 0)
            plan.push_back(
                OverlayRectangle{17, y + 14, static_cast<float>(width), 2, {255, 128, 192}});
        plan.push_back(face);
        return plan;
    }
    plan.push_back(OverlayImage{"restBar00.png", {0, 0, 17, 17}, {0, y, 17, 17}});
    // The number is h()/capacity, never current HP or the amount recovered this visit.
    const auto digits = std::to_string(capacity);
    for (std::size_t i = 0; i < digits.size(); ++i)
        plan.push_back(
            OverlaySprite{"number11.seb", digits[i] - '0', 18 + static_cast<float>(i * 7), y});
    plan.push_back(OverlayRectangle{18, y + 12, 29, 5, {246, 246, 246}});
    plan.push_back(OverlayRectangle{19, y + 13, 27, 3, {39, 53, 74}});
    // Presentation clamps the mapping only; lethal negative HP and overflow-capacity state
    // remain untouched. int64 protects the multiply while preserving integer truncation.
    const auto width = static_cast<int>(std::clamp<std::int64_t>(
        static_cast<std::int64_t>(actor.hp.displayed) * 26 / capacity, 0, 26));
    if (width > 0)
        plan.push_back(OverlayRectangle{20, y + 14, static_cast<float>(width), 2, {83, 255, 0}});
    if (width < 26)
        plan.push_back(OverlayRectangle{20 + static_cast<float>(width),
                                        y + 14,
                                        static_cast<float>(26 - width),
                                        2,
                                        {68, 100, 104}});
    plan.push_back(face);
    return plan;
}
} // namespace

std::vector<WorldRestRow> world_rest_rows(const simulation::StartupWorldRuntimeState &state,
                                          std::uint64_t facility) {
    const auto &world = state.scene.world.world;
    const auto &inn = world.facilities.at(facility);
    std::vector<WorldRestRow> rows;
    if (inn.category != 2)
        return rows;
    const auto count = std::min<std::size_t>(inn.occupants.size(), 4);
    for (std::size_t i = 0; i < count; ++i) {
        const auto id = inn.occupants[i];
        // Facilities can retain a source object after it leaves the live roster. The owner
        // explicitly preserves that record; retirement alone does not clear its rest flags.
        const auto live = world.ai.battle.actors.find(id);
        const auto &actor =
            live != world.ai.battle.actors.end() ? live->second : world.ai.retired_actors.at(id);
        if (!(actor.control.flags & 32U))
            continue;
        const auto portrait = simulation::startup_world_portrait(state, actor.definition);
        if (!portrait)
            throw std::invalid_argument("Inn portrait requires a current profession and sex");
        const int capacity = world.ai.growth.at(actor.definition).derived.combat[0];
        rows.push_back(
            {id, actor.state_counter, rest_plan(actor, capacity, *portrait, -20.F * rows.size())});
    }
    return rows;
}
} // namespace ark::desktop
