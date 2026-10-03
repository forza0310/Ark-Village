#include "dungeon_village_reference/weapon_choice.hpp"

#include <limits>
#include <set>

namespace dungeon_village_reference {
WeaponChoiceResult prepare_weapon_choice(const std::vector<WeaponChoiceDefinition> &catalogue,
                                         std::int32_t current_weapon_id,
                                         std::int32_t reselect_counter, std::optional<int> ticket) {
    if (current_weapon_id < 0 || reselect_counter < 0)
        return {WeaponChoiceError::invalid_input, std::nullopt};
    const WeaponChoiceDefinition *current = nullptr;
    std::set<std::int32_t> ids;
    for (const auto &weapon : catalogue) {
        if (weapon.id < 0 || weapon.rank < 0 || !ids.insert(weapon.id).second)
            return {WeaponChoiceError::invalid_input, std::nullopt};
        if (weapon.id == current_weapon_id)
            current = &weapon;
    }
    if (!current)
        return {WeaponChoiceError::invalid_input, std::nullopt};
    WeaponChoiceCandidate result{current_weapon_id, {}, false};
    if (reselect_counter > 0)
        return {WeaponChoiceError::none, result};
    const auto upper = static_cast<std::int64_t>(current->rank) + 2;
    for (const auto &weapon : catalogue)
        if (weapon.unlocked && weapon.rank >= current->rank && weapon.rank <= upper)
            result.eligible_ids.push_back(weapon.id);
    if (result.eligible_ids.empty())
        return {WeaponChoiceError::none, result};
    if (result.eligible_ids.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {WeaponChoiceError::invalid_input, std::nullopt};
    if (!ticket)
        return {WeaponChoiceError::missing_ticket, std::nullopt};
    if (*ticket < 0 || static_cast<std::size_t>(*ticket) >= result.eligible_ids.size())
        return {WeaponChoiceError::invalid_ticket, std::nullopt};
    result.weapon_id = result.eligible_ids[static_cast<std::size_t>(*ticket)];
    result.consumes_ticket = true;
    return {WeaponChoiceError::none, result};
}
} // namespace dungeon_village_reference
