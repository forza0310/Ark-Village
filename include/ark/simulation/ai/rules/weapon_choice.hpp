#pragma once

// Weapon-only selection. Equipment cooldown is not stock, money or facility use duration.
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace ark::simulation::rules {
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
                                         std::optional<int> ticket = std::nullopt,
                                         const std::function<std::optional<int>(int)> &draw = {});

enum class EquipmentChoiceKind { armor, accessory };
struct EquipmentChoiceDefinition {
    int id{};
    int rank{};
    int type{}; // Armor d==2 belongs to slot1, all other types slot2. Accessory ignores type.
    bool unlocked{};
};
struct EquipmentChoiceInput {
    EquipmentChoiceKind kind{EquipmentChoiceKind::armor};
    int slot{1}; // Armor1/2; accessory3. Caller already consumed armor-arrival draw2 +1.
    std::vector<EquipmentChoiceDefinition> catalogue;
    std::optional<int> current; // Null equipment uses rank0 and does NOT block on A[slot]>0.
    int reselect_counter{};
    std::optional<int> ticket;
    // 仅真实候选非空时调用，已有装备且冷却为正不抽；空槽不受冷却阻止。
    std::function<std::optional<int>(int)> draw{};
};
struct EquipmentChoiceCandidate {
    int equipment_id{};
    std::optional<int>
        selected_slot; // Armor type determines actual commit slot, not original draw.
    std::vector<int> eligible_ids;
    bool consumes_ticket{};
    bool literal_zero_fallback{}; // Armor null/empty returns source literal0; not invented gear.
};
struct EquipmentChoiceResult {
    WeaponChoiceError error{WeaponChoiceError::none};
    std::optional<EquipmentChoiceCandidate> candidate;
};
// Armor/accessory rank[current-1,current+2], inclusive, unlike weapon lower bound current.
// Empty+missing accessory would dereference null in the source: maintenance returns invalid_input.
EquipmentChoiceResult prepare_equipment_choice(const EquipmentChoiceInput &input);
} // namespace ark::simulation::rules
