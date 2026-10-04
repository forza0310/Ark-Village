#pragma once

// Generated only from pinned research tables. Runtime does not load research or JSON.
#include "ark/facilities/exit.hpp"
#include "ark/people/human_growth.hpp"
#include "ark/people/weapon_choice.hpp"
#include <map>
namespace ark::app {
struct InitialWeaponRule {
    people::WeaponChoiceDefinition selection;
    int price{};
    std::array<int, 4> combat{};
};
struct InitialServiceRule {
    int wait{};
    std::vector<facilities::FacilityAttributeEffect> effects;
};
struct InitialAiRules {
    people::HumanDefinitionStatsInput first_definition;
    std::vector<people::HumanProfessionRule> professions;
    std::vector<InitialWeaponRule> weapons;
    std::array<int, 2> satisfaction_thresholds{};
    int weapon_reselect_counter{};
    std::map<int, InitialServiceRule> services; // Only the evidenced initial28/30/33 interval.
};
const InitialAiRules &initial_ai_rules();
} // namespace ark::app
