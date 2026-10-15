#include "world_steam_budget.hpp"
#include "world_steam_strategy.hpp"
#include <algorithm>

namespace ark::test {
// Housing uses the same operation/admission state as the original blueprint route.
// Extra plots remain real world instances, so construction and cold loads need no sidecar flag.
std::optional<app::WorldCommand> SteamVillageStrategy::housing(const app::WorldState &state,
                                                               bool may_build) {
    const auto &layout = steam_village_layout();
    const bool reference_destination =
        std::any_of(layout.begin(), layout.end(), [&](const auto &t) {
            return t.role == SteamLayoutRole::residence && steam_layout_inside_fence(state, t) &&
                   !steam_layout_match(state, t) && steam_layout_blockers(state, t).empty();
        });
    const bool extend = steam_reference_layout_complete(state);
    if (!may_build) {
        for (const auto &[id, plot] : state.scene.world.world.facilities) {
            if (plot.placement.definition_id != 24 || plot.status != 1)
                continue;
            const auto anchor = plot.placement.anchor;
            const bool reference_plot =
                std::any_of(layout.begin(), layout.end(), [&](const auto &t) {
                    return t.role == SteamLayoutRole::recruitment && t.anchor == anchor;
                });
            const bool extra_plot = extend && !steam_layout_slot(state, id);
            if (!(reference_destination && reference_plot) && !extra_plot)
                continue;
            const auto human = SteamPeopleStrategy::eligible_homeless(
                state, steam_residence_reserve(state, extra_plot ? std::optional<std::uint64_t>{id}
                                                                 : std::nullopt));
            if (!human)
                continue;
            recruitment_ = id;
            resident_ = *human;
            app::WorldCommand command;
            command.kind = app::WorldCommandKind::open_facility;
            command.facility = id;
            return command;
        }
        return {};
    }
    if (!extend ||
        !SteamPeopleStrategy::eligible_homeless(state, steam_residence_plot_reserve(state)))
        return {};
    // An unfinished plot must finish before another is purchased; an empty completed
    // plot waits for its real candidate instead of silently consuming another footprint.
    if (steam_extra_recruitment_plot(state))
        return {};
    const auto destination = steam_connected_extension(state, 24);
    if (!destination)
        return {};
    operation_ = Operation::build;
    target_ = *destination;
    target_.role = SteamLayoutRole::recruitment;
    operation_committed_ = false;
    app::WorldCommand command;
    command.kind = app::WorldCommandKind::open_build_menu;
    return command;
}
} // namespace ark::test
