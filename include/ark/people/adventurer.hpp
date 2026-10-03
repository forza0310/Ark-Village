#pragma once

// First-visit fields from STARTUP.md. Definition ID is not the live character UID.
#include "ark/world/grid.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace ark::people {
struct Adventurer {
    int uid{}, definition_id{};
    std::string name;
    int job_id{}, sex{}, level{}, effort{}, satisfaction{};
    std::array<int, 6> attributes{};
    std::array<int, 4> equipment{}, combat{};
    std::array<int, 3> hp{};
    world::Cell cell;
    std::uint32_t flags{};
    world::WorldPosition position;
    std::optional<int>
        pending_activity{}; // Activity0 is evidenced; executing its priority is pending.
};
// Install the researched first visitor before event89. RNG chooses only an evidenced spawn point;
// local RNG consumption and unknown initial AI/facing are not original-game equivalence claims.
Adventurer first_visit(const Adventurer &definition, world::Cell spawn);
} // namespace ark::people
