#include "dungeon_village_reference/weapon_choice.hpp"

#include <limits>
#include <set>

namespace dungeon_village_reference {
WeaponChoiceResult prepare_weapon_choice(const std::vector<WeaponChoiceDefinition> &catalogue,
                                         std::int32_t current_weapon_id,
                                         std::int32_t reselect_counter, std::optional<int> ticket,
                                         const std::function<std::optional<int>(int)> &draw) {
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
    if (!ticket && draw) {
        try {
            ticket = draw(static_cast<int>(result.eligible_ids.size()));
        } catch (...) {
            return {WeaponChoiceError::invalid_input, std::nullopt};
        }
    }
    if (!ticket)
        return {WeaponChoiceError::missing_ticket, std::nullopt};
    if (*ticket < 0 || static_cast<std::size_t>(*ticket) >= result.eligible_ids.size())
        return {WeaponChoiceError::invalid_ticket, std::nullopt};
    result.weapon_id = result.eligible_ids[static_cast<std::size_t>(*ticket)];
    result.consumes_ticket = true;
    return {WeaponChoiceError::none, result};
}
EquipmentChoiceResult prepare_equipment_choice(const EquipmentChoiceInput &i) {
    if ((i.kind != EquipmentChoiceKind::armor && i.kind != EquipmentChoiceKind::accessory) ||
        (i.kind == EquipmentChoiceKind::armor ? (i.slot != 1 && i.slot != 2) : i.slot != 3) ||
        i.reselect_counter < 0)
        return {WeaponChoiceError::invalid_input, std::nullopt};
    const EquipmentChoiceDefinition *current = nullptr;
    std::set<int> ids;
    for (const auto &d : i.catalogue) {
        if (d.id < 0 || d.rank < 0 || d.type < 0 || !ids.insert(d.id).second)
            return {WeaponChoiceError::invalid_input, std::nullopt};
        if (i.current == d.id)
            current = &d;
    }
    if (i.current && !current)
        return {WeaponChoiceError::invalid_input, std::nullopt};
    const auto actual_slot = [&](const EquipmentChoiceDefinition &d) {
        return i.kind == EquipmentChoiceKind::accessory ? 3 : d.type == 2 ? 1 : 2;
    };
    EquipmentChoiceCandidate c;
    c.selected_slot = i.slot;
    if (current) {
        c.equipment_id = current->id;
        c.selected_slot = actual_slot(*current);
        if (i.reselect_counter > 0)
            return {WeaponChoiceError::none, c};
    }
    const std::int64_t rank = current ? current->rank : 0;
    for (const auto &d : i.catalogue)
        if (d.unlocked && (i.kind == EquipmentChoiceKind::accessory || actual_slot(d) == i.slot) &&
            d.rank >= rank - 1 && d.rank <= rank + 2)
            c.eligible_ids.push_back(d.id);
    if (c.eligible_ids.empty()) {
        if (!current) {
            if (i.kind == EquipmentChoiceKind::accessory)
                return {WeaponChoiceError::invalid_input, std::nullopt};
            c.literal_zero_fallback = true;
            c.selected_slot.reset(); // Literal0 need not resolve to a catalogue definition.
            for (const auto &d : i.catalogue)
                if (d.id == 0)
                    c.selected_slot = actual_slot(d);
        }
        return {WeaponChoiceError::none, c};
    }
    if (c.eligible_ids.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {WeaponChoiceError::invalid_input, std::nullopt};
    auto ticket = i.ticket;
    if (!ticket && i.draw) {
        try {
            ticket = i.draw(static_cast<int>(c.eligible_ids.size()));
        } catch (...) {
            return {WeaponChoiceError::invalid_input, std::nullopt};
        }
    }
    if (!ticket)
        return {WeaponChoiceError::missing_ticket, std::nullopt};
    if (*ticket < 0 || static_cast<std::size_t>(*ticket) >= c.eligible_ids.size())
        return {WeaponChoiceError::invalid_ticket, std::nullopt};
    c.equipment_id = c.eligible_ids[static_cast<std::size_t>(*ticket)];
    c.consumes_ticket = true;
    for (const auto &d : i.catalogue)
        if (d.id == c.equipment_id)
            c.selected_slot = actual_slot(d);
    return {WeaponChoiceError::none, c};
}
} // namespace dungeon_village_reference
