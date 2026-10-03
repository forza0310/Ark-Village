#pragma once

// Weapon-only selection. Equipment cooldown is not stock, money or facility use duration.
#include <cstdint>
#include <optional>
#include <vector>

namespace dungeon_village_reference {
struct WeaponChoiceDefinition {
    std::int32_t id{};
    std::int32_t rank{};
    bool unlocked{};
};
enum class WeaponChoiceError { none, invalid_input, missing_ticket, invalid_ticket };
struct WeaponChoiceCandidate {
    std::int32_t weapon_id{};
    std::vector<std::int32_t> eligible_ids; // Original catalogue order; no rank sorting.
    bool consumes_ticket{};
};
struct WeaponChoiceResult {
    WeaponChoiceError error{WeaponChoiceError::none};
    std::optional<WeaponChoiceCandidate> candidate;
};
// A positive legacy A[0] retains the current weapon without drawing. Otherwise use an injected
// ticket in [0, eligible_count); no eligible weapon also retains current. This does not equip,
// reset/decrement A[0], charge cash, or interpret armour/accessory branches.
WeaponChoiceResult prepare_weapon_choice(const std::vector<WeaponChoiceDefinition> &catalogue,
                                         std::int32_t current_weapon_id,
                                         std::int32_t reselect_counter,
                                         std::optional<int> ticket = std::nullopt);
} // namespace dungeon_village_reference
