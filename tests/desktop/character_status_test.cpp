// Published HP display contract, real HP animation, and immutable normal/preview read models.
#include "character_status.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
int checks{};
void check(bool ok, const char *why) {
    ++checks;
    if (!ok)
        throw std::runtime_error(why);
}
using ark::desktop::character_hp_bar;
using ark::desktop::CharacterStatusInput;
using ark::desktop::CharacterStatusRectangle;
using Color = std::array<std::uint8_t, 3>;
int pixels(const std::vector<CharacterStatusRectangle> &bar, Color rgb) {
    int total{};
    for (const auto &rectangle : bar)
        if (rectangle.rgb == rgb)
            total += rectangle.width;
    return total;
}
void contract() {
    CharacterStatusInput input{{-50, 100, 100, 50, true, 0}, 100};
    auto bar = character_hp_bar(input);
    check(bar.size() == 4 && bar[0].x == -11 && bar[0].y == -26 && bar[0].width == 21 &&
              bar[0].height == 5 && bar[0].rgb == Color{246, 246, 246} && bar[1].x == -10 &&
              bar[1].y == -25 && bar[1].width == 19 && bar[1].height == 3 &&
              bar[1].rgb == Color{39, 53, 74},
          "published frame and inset dimensions/colors at the actor anchor");
    check(bar[2].x == -9 && bar[2].y == -24 && bar[2].height == 2 && bar[3].x == 0 &&
              bar[3].y == -24 && bar[3].height == 2 && pixels(bar, {83, 255, 0}) == 9 &&
              pixels(bar, {255, 52, 35}) == 9,
          "human damage preserves target fill and old display difference, not current HP alone");
    input.human = false;
    bar = character_hp_bar(input);
    check(pixels(bar, {35, 203, 255}) == 9 && pixels(bar, {252, 255, 5}) == 9 &&
              pixels(bar, {255, 52, 35}) == 0,
          "monster target and damage transition use independent blue/yellow palette");
    input.hp.requested_delta = 50;
    input.hp.displayed = 50;
    input.hp.target = 100;
    bar = character_hp_bar(input);
    check(pixels(bar, {35, 203, 255}) == 9 && pixels(bar, {68, 100, 104}) == 9 &&
              pixels(bar, {252, 255, 5}) == 0,
          "healing uses displayed value and remaining slot, never future target or damage color");
    input.hp.animating = false;
    check(character_hp_bar(input).empty(), "inactive unselected HP is hidden");
    input.selected = true;
    check(!character_hp_bar(input).empty(), "selected actor can display inactive HP");
    input.action = 7;
    check(character_hp_bar(input).empty(), "action7 suppresses even selected HP");
    input.action = 0;
    input.visible = false;
    check(character_hp_bar(input).empty(), "hidden actor has no floating status");
    input.visible = true;
    for (const int capacity : {0, -1}) {
        input.capacity = capacity;
        bool rejected{};
        try {
            character_hp_bar(input);
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        check(rejected, "invalid visible capacity explicitly rejected");
    }
    input.capacity = std::numeric_limits<int>::max();
    input.hp = {0, std::numeric_limits<int>::max(), 0, std::numeric_limits<int>::max(), true, 0};
    check(pixels(character_hp_bar(input), {35, 203, 255}) == 18,
          "extreme integer HP mapping has no multiplication overflow");
    input.capacity = 100;
    input.hp = {-150, 100, 100, -50, true, 0};
    bar = character_hp_bar(input);
    check(pixels(bar, {35, 203, 255}) == 0 && pixels(bar, {252, 255, 5}) == 18,
          "negative lethal target clamps to zero while preserving old display");
    input.hp = {0, -50, -50, -50, true, 0};
    check(pixels(character_hp_bar(input), {68, 100, 104}) == 18,
          "negative display maps to the full remaining slot");
    input.hp = {0, 200, 200, 200, true, 0};
    check(pixels(character_hp_bar(input), {35, 203, 255}) == 18,
          "over-capacity display saturates without changing logical HP");
}
void logical_animation() {
    using namespace ark::simulation::rules;
    CharacterStatusInput input;
    input.capacity = 100;
    const auto hit = prepare_hp_change({0, 100, 100, 100, false, 0}, -50, 100);
    check(hit.candidate.has_value(), "real HP damage accepted");
    input.hp = *hit.candidate;
    check(pixels(character_hp_bar(input), {255, 52, 35}) == 9, "initial damage lag is visible");
    for (int tick = 1; tick <= 30; ++tick) {
        const auto next = advance_hp_animation(input.hp);
        check(next.candidate.has_value(), "real logical HP count advanced");
        input.hp = *next.candidate;
        const auto bar = character_hp_bar(input);
        if (tick == 10)
            check(pixels(bar, {255, 52, 35}) == 9, "pre-interpolation ten-count delay retained");
        if (tick == 19)
            check(pixels(bar, {255, 52, 35}) == 1, "late damage interpolation shrinks transition");
        if (tick == 20)
            check(pixels(bar, {255, 52, 35}) == 0 && pixels(bar, {83, 255, 0}) == 9,
                  "completed damage interpolation reads target fill");
        if (tick == 30)
            check(bar.empty(), "count30 clears active display, independent of render FPS");
    }
    input.hp = *prepare_hp_change({0, 50, 50, 50, false, 0}, 50, 100).candidate;
    input.hp = *advance_hp_animation(input.hp, 20).candidate;
    check(pixels(character_hp_bar(input), {83, 255, 0}) == 17 &&
              pixels(character_hp_bar(input), {68, 100, 104}) == 1,
          "recovery display retains source interpolation result rather than snapping to full");
}
} // namespace
int main() {
    try {
        contract();
        logical_animation();
        std::cout << checks << " checks passed\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
