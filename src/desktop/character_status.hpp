#pragma once

// Read-only HP presentation from research/ui/COMBAT_RENDER. Logical HP animation remains in
// the actor update; these source-pixel rectangles are scaled by the scene exactly once.
#include "ark/people/hp.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace ark::app {
class Game;
}
namespace ark::desktop {
struct CharacterStatusInput {
    people::CharacterHpState hp;
    int capacity{};
    int action{};
    bool human{true};
    bool selected{};
    bool visible{true};
};
struct CharacterStatusRectangle {
    int x{}, y{}, width{}, height{};
    std::array<std::uint8_t, 3> rgb{};
};
// Normal and strict-preview modes expose the same immutable HP protocol. Selection is an
// explicit actor selection; the current UI's selected building/grid cell is not one.
std::optional<CharacterStatusInput> character_status_input(const app::Game &game,
                                                           bool selected = false);
// Empty when inactive/hidden/action7. A visible bar requires positive capacity; invalid
// capacity is rejected rather than manufacturing full HP or dividing by zero.
std::vector<CharacterStatusRectangle> character_hp_bar(const CharacterStatusInput &input);
} // namespace ark::desktop
