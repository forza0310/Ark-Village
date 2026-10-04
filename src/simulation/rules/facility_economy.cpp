// Pure integer economy derivation with shared definition and instance modifiers kept separate.
// Package responsibilities and evidence boundaries: ../README.md.

#include "ark/simulation/rules/facility_economy.hpp"

#include <algorithm>
#include <limits>

namespace ark::simulation::rules {
namespace {

bool valid_endpoints(LevelEndpoints endpoints) {
    return endpoints.first >= 0 && endpoints.fifth >= 0;
}

std::int64_t interpolate(LevelEndpoints endpoints, int numerator, int denominator) {
    return endpoints.first + static_cast<std::int64_t>(numerator) *
                                 (static_cast<std::int64_t>(endpoints.fifth) - endpoints.first) /
                                 denominator;
}

bool multiply_percent(std::int64_t &value, std::int64_t percent) {
    if (value > std::numeric_limits<std::int64_t>::max() / percent ||
        value < std::numeric_limits<std::int64_t>::min() / percent) {
        return false;
    }
    value = value * percent / 100;
    return true;
}

bool flag(std::uint32_t bits, std::uint32_t bit) {
    return (bits & bit) != 0U;
}

} // namespace

FacilityEconomyResult derive_facility_economy(const FacilityEconomyDefinition &definition,
                                              const FacilityEconomyInput &input) {
    if (input.level < 1 || input.level > 5 || definition.construction_cost < 0 ||
        definition.construction_ticks < 0 || !valid_endpoints(definition.upgrade_uses) ||
        !std::all_of(definition.attributes.begin(), definition.attributes.end(), valid_endpoints) ||
        std::any_of(
            input.legacy_job_counts.begin(), input.legacy_job_counts.end(),
            [](std::int32_t count) { return count < 0; })) {
        return {FacilityEconomyError::invalid_input, std::nullopt};
    }
    FacilityEconomyValues values;
    for (std::size_t slot = 0; slot < values.definition_attributes.size(); ++slot) {
        auto value = interpolate(definition.attributes[slot], input.level - 1, 4) +
                     input.definition_improvements[slot];
        if (slot == 0) {
            int group = -1;
            if (flag(definition.legacy_flags, 4096)) {
                group = 2;
            } else if (flag(definition.legacy_flags, 8192)) {
                group = 3;
            } else if (flag(definition.legacy_flags, 512)) {
                group = 4;
            } else if (flag(definition.legacy_flags, 2048)) {
                group = 6;
            } else if (flag(definition.legacy_flags, 65536)) {
                group = 5;
            }
            if (group >= 0 &&
                !multiply_percent(
                    value, 100 + 20LL * input.legacy_job_counts[static_cast<std::size_t>(group)])) {
                return {FacilityEconomyError::numeric_overflow, std::nullopt};
            }
            const int second_group = flag(definition.legacy_flags, 131072)   ? 0
                                     : flag(definition.legacy_flags, 262144) ? 1
                                                                             : -1;
            if (second_group >= 0 &&
                !multiply_percent(
                    value,
                    100 + 10LL * input.legacy_job_counts[static_cast<std::size_t>(second_group)])) {
                return {FacilityEconomyError::numeric_overflow, std::nullopt};
            }
        }
        if (slot == 3) {
            value -= value % 10;
        }
        const auto cap = static_cast<std::int64_t>(definition.attributes[slot].fifth) * 2;
        values.definition_attributes[slot] = std::min(value, cap);
        values.instance_attributes[slot] = std::min(
            values.definition_attributes[slot] + (slot == 3 ? 0 : input.instance_modifiers[slot]),
            cap);
    }
    values.construction_cost = definition.construction_cost;
    if (definition.decoration) {
        const auto discount = std::min(input.legacy_job_counts[8], 3);
        values.construction_cost = interpolate(
            {definition.construction_cost, definition.construction_cost / 4}, discount, 3);
        values.construction_cost -= values.construction_cost % 10;
    }
    const auto acceleration = std::min(input.legacy_job_counts[9], 3);
    values.construction_ticks = interpolate(
        {definition.construction_ticks, definition.construction_ticks / 10}, acceleration, 3);
    values.upgrade_uses = interpolate(definition.upgrade_uses, input.level - 1, 4);
    values.upgrade_ready =
        definition.upgrade_uses.fifth > 0 && input.level < 5 &&
        input.completed_definition_uses >= static_cast<std::uint64_t>(values.upgrade_uses);
    return {FacilityEconomyError::none, values};
}

} // namespace ark::simulation::rules
