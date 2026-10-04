#include "dungeon_village_prototype/facility_projection.hpp"
#include "dungeon_village_reference/world_facilities.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

using namespace dungeon_village_prototype;
using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool value, const char *message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
void map_targets() {
    const std::array<Position, 4> offsets{{{58, 65}, {65, 58}, {41, 35}, {35, 41}}};
    for (int y = 0; y < 24; ++y)
        for (int x = 0; x < 24; ++x) {
            const auto special = project_facility_use_target({x, y}, 6, 3);
            check(special && special->world_target == Position{x * 100 + 33, y * 100 + 40} &&
                      special->projected_target == Position{(x + y) * 30 + 22, (y - x) * 15 + 1},
                  "special entry old-s projection and world truncation");
            for (int d = 0; d < 4; ++d) {
                const auto rest = project_facility_use_target({x, y}, 8, 2, d);
                check(rest && rest->world_target ==
                                  Position{x * 100 + offsets[d].x, y * 100 + offsets[d].y},
                      "all four rest offsets on all576 source-map cells");
                const auto plan =
                    prepare_facility_use_plan({{}, 8, 2, 0, 0, 0, rest->world_target, d});
                check(plan.candidate &&
                          plan.candidate->control.queue.front() ==
                              LegacyActorControl{0, rest->world_target.x, rest->world_target.y},
                      "presentation target enters domain plan, never a copied pixel coordinate");
            }
        }
    check(!project_facility_use_target({-1, 1}, 8, 2, 0) &&
              !project_facility_use_target({1, 1}, 8, 2) &&
              !project_facility_use_target({1, 1}, 8, 2, 4) &&
              !project_facility_use_target({1, 1}, 6, 3, 0) &&
              !project_facility_use_target({1, 1}, 2, 0),
          "do not guess directions or apply this adapter to other categories");
}
} // namespace
int main() {
    try {
        map_targets();
        std::cout << checks << " facility projection checks passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
