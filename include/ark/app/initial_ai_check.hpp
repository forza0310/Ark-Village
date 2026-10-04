#pragma once
#include "ark/world/grid.hpp"
#include <cstdint>
#include <vector>
namespace ark::app {
struct InitialAiCheck {
    world::Cell birth;
    int first_facility{};
    std::uint64_t rounds{};
    std::int64_t funds{};
};
// Run all six maintained initial intervals with explicit tickets; throw on failed closure.
// No window, calendar advancement after arrival, player construction, or APK RNG assertion.
std::vector<InitialAiCheck> check_initial_ai();
} // namespace ark::app
