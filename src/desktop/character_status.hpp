#pragma once

// Read-only HP presentation from research/ui/COMBAT_RENDER. Logical HP animation remains in
// the actor update; these source-pixel rectangles are scaled by the scene exactly once.
#include "ark/simulation/rules/character_hp.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace ark::desktop {
struct CharacterStatusInput {
    simulation::rules::CharacterHpState hp;
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
// Empty when inactive/hidden/action7. A visible bar requires positive capacity; invalid
// capacity is rejected rather than manufacturing full HP or dividing by zero.
std::vector<CharacterStatusRectangle> character_hp_bar(const CharacterStatusInput &input);
} // namespace ark::desktop
