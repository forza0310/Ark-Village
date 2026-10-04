#include "character_status.hpp"
#include "ark/app/game.hpp"
#include "character_visibility.hpp"
#include <algorithm>
#include <stdexcept>

namespace ark::desktop {
std::optional<CharacterStatusInput> character_status_input(const app::Game &game, bool selected) {
    const app::LifeActorState *actor = game.life_state();
    if (!actor)
        actor = game.ai_state();
    if (!actor || !game.state().adventurer)
        return std::nullopt;
    return CharacterStatusInput{actor->hp, actor->stats.combat[0], actor->control.action, true,
                                selected,  character_visible(game)};
}

std::vector<CharacterStatusRectangle> character_hp_bar(const CharacterStatusInput &input) {
    if (!input.visible || input.action == 7 || (!input.hp.animating && !input.selected))
        return {};
    if (input.capacity <= 0)
        throw std::invalid_argument("Visible HP bar requires positive capacity");
    // Source integer mapping is bounded after division; negative target HP is valid after a
    // lethal hit. int64 also makes extreme valid HP inputs safe without changing the owner.
    const auto width = [&](int hp) {
        return static_cast<int>(
            std::clamp<std::int64_t>(static_cast<std::int64_t>(hp) * 18 / input.capacity, 0, 18));
    };
    const auto displayed = width(input.hp.displayed);
    const auto target = width(input.hp.target);
    const std::array<std::uint8_t, 3> fill = input.human
                                                 ? std::array<std::uint8_t, 3>{83, 255, 0}
                                                 : std::array<std::uint8_t, 3>{35, 203, 255};
    const std::array<std::uint8_t, 3> damage = input.human
                                                   ? std::array<std::uint8_t, 3>{255, 52, 35}
                                                   : std::array<std::uint8_t, 3>{252, 255, 5};
    std::vector<CharacterStatusRectangle> rectangles{{-11, -26, 21, 5, {246, 246, 246}},
                                                     {-10, -25, 19, 3, {39, 53, 74}}};
    const auto append = [&](int start, int length, const std::array<std::uint8_t, 3> &rgb) {
        if (length > 0)
            rectangles.push_back({-9 + start, -24, length, 2, rgb});
    };
    // Positive requests are recovery: only the display segment is visible. Damage preserves
    // the old display above the new target, while a nonpositive difference draws no transition.
    if (input.hp.requested_delta > 0) {
        append(0, displayed, fill);
    } else {
        append(0, target, fill);
        append(target, displayed - target, damage);
    }
    append(displayed, 18 - displayed, {68, 100, 104});
    return rectangles;
}
} // namespace ark::desktop
