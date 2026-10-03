#pragma once

// Pure FACILITIES arithmetic adapted from research/example/facility_economy. Slot order is
// price, quality, charm, maintenance; shared progress is distinct from instance neighbourhood.
#include <array>
#include <cstdint>
namespace ark::facilities {
struct Endpoints {
    std::int32_t first{}, fifth{};
};
struct EconomyDefinition {
    std::array<Endpoints, 4> attributes{};
    Endpoints upgrade_uses;
    std::int32_t construction_cost{}, construction_ticks{};
    std::uint32_t flags{};
    bool decoration{};
};
struct EconomyInput {
    int level{1};
    std::array<std::int32_t, 4> improvements{};
    std::array<std::int32_t, 3> modifiers{};
    std::array<std::int32_t, 10> job_counts{};
    std::uint64_t completed_uses{};
};
struct EconomyValues {
    std::array<std::int64_t, 4> definition{}, instance{};
    std::int64_t construction_cost{}, construction_ticks{}, upgrade_uses{};
    bool upgrade_ready{}; // Derived readiness never replaces the stored upgrade prompt flag.
};
// Invalid inputs/overflow throw before returning a result. No clamping to zero or mutation.
EconomyValues derive_economy(const EconomyDefinition &definition, const EconomyInput &input);
} // namespace ark::facilities
