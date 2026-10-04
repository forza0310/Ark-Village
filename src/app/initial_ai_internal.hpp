#pragma once
// Shared read-only table lookup for the split private AI owner implementation.
#include "ark/app/initial_ai.hpp"
#include <algorithm>
#include <stdexcept>
namespace ark::app {
inline const facilities::Definition &initial_definition(int id) {
    const auto &all = startup_data().definitions;
    const auto found =
        std::find_if(all.begin(), all.end(), [=](const auto &d) { return d.id == id; });
    if (found == all.end())
        throw std::invalid_argument("Unknown initial AI facility definition");
    return *found;
}
} // namespace ark::app
