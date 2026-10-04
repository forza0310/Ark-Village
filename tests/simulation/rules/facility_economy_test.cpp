#include "ark/simulation/rules/facility_economy.hpp"

#include <climits>
#include <cstdlib>
#include <iostream>

using namespace ark::simulation::rules;

namespace {

int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
FacilityEconomyDefinition inn() {
    FacilityEconomyDefinition result;
    result.attributes = {LevelEndpoints{300, 450}, LevelEndpoints{5, 50}, LevelEndpoints{5, 50},
                         LevelEndpoints{240, 480}};
    result.upgrade_uses = {50, 500};
    result.construction_cost = 1000;
    result.construction_ticks = 400;
    result.legacy_flags = 101;
    return result;
}
FacilityEconomyValues derive(const FacilityEconomyDefinition &definition,
                             const FacilityEconomyInput &input = {}) {
    const auto result = derive_facility_economy(definition, input);
    check(result.error == FacilityEconomyError::none && result.values.has_value(),
          "valid derivation succeeds");
    return *result.values;
}

void fixed_record_baselines() {
    const std::array<std::int64_t, 5> prices = {300, 337, 375, 412, 450};
    const std::array<std::int64_t, 5> maintenance = {240, 300, 360, 420, 480};
    const std::array<std::int64_t, 5> uses = {50, 162, 275, 387, 500};
    for (int level = 1; level <= 5; ++level) {
        FacilityEconomyInput input;
        input.level = level;
        const auto values = derive(inn(), input);
        const auto index = static_cast<std::size_t>(level - 1);
        check(values.definition_attributes[0] == prices[index] &&
                  values.instance_attributes[0] == prices[index],
              "inn integer interpolation");
        check(values.definition_attributes[3] == maintenance[index], "inn maintenance levels");
        check(values.upgrade_uses == uses[index] && values.construction_cost == 1000 &&
                  values.construction_ticks == 400,
              "inn construction and upgrade thresholds");
    }
    auto cafe = inn();
    cafe.attributes = {LevelEndpoints{420, 630}, LevelEndpoints{6, 60}, LevelEndpoints{5, 50},
                       LevelEndpoints{192, 384}};
    cafe.upgrade_uses = {30, 300};
    cafe.construction_cost = 700;
    cafe.legacy_flags = 262244;
    const std::array<std::int64_t, 5> cafe_maintenance = {190, 240, 280, 330, 380};
    for (int level = 1; level <= 5; ++level) {
        FacilityEconomyInput input;
        input.level = level;
        check(derive(cafe, input).definition_attributes[3] ==
                  cafe_maintenance[static_cast<std::size_t>(level - 1)],
              "cafe rounds maintenance after interpolation");
    }
}

void improvement_order_and_caps() {
    auto definition = inn();
    definition.legacy_flags = 4096 | 131072;
    FacilityEconomyInput input;
    input.definition_improvements = {4, 3, -9, 9};
    input.instance_modifiers = {17, 4, 5, 999};
    input.legacy_job_counts[2] = 1;
    input.legacy_job_counts[0] = 1;
    const auto values = derive(definition, input);
    check(values.definition_attributes[0] == 400 && values.instance_attributes[0] == 417,
          "improvement then two separate percentage truncations then instance addition");
    check(values.definition_attributes[1] == 8 && values.instance_attributes[1] == 12,
          "quality instance modifiers");
    check(values.definition_attributes[2] == -4 && values.instance_attributes[2] == 1,
          "signed improvement has no invented zero floor");
    check(values.definition_attributes[3] == 240 && values.instance_attributes[3] == 240,
          "maintenance rounds before cap and ignores instance modifiers");
    input.definition_improvements = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
    input.instance_modifiers = {INT_MAX, INT_MAX, INT_MAX, INT_MAX};
    const auto capped = derive(definition, input);
    check(capped.definition_attributes == std::array<std::int64_t, 4>{900, 100, 100, 960} &&
              capped.instance_attributes == capped.definition_attributes,
          "all attributes capped by twice fifth endpoint");
    definition.attributes[3] = {19, 19};
    input.definition_improvements = {0, 0, 0, 100};
    check(derive(definition, input).definition_attributes[3] == 38,
          "cap can follow rounding and need not be multiple of ten");
}

void flag_priority() {
    auto definition = inn();
    FacilityEconomyInput input;
    input.legacy_job_counts = {1, 2, 1, 2, 3, 4, 5, 0, 0, 0};
    const std::array<std::uint32_t, 5> flags = {4096, 8192, 512, 2048, 65536};
    const std::array<std::int64_t, 5> expected = {360, 420, 480, 600, 540};
    for (std::size_t index = 0; index < flags.size(); ++index) {
        definition.legacy_flags = flags[index];
        check(derive(definition, input).definition_attributes[0] == expected[index],
              "each primary price group");
    }
    definition.legacy_flags = 4096 | 8192 | 512 | 2048 | 65536 | 131072 | 262144;
    check(derive(definition, input).definition_attributes[0] == 396,
          "if-else priorities in both price groups");
    definition.legacy_flags = 1;
    check(derive(definition, input).definition_attributes[0] == 300,
          "unrelated flag does not multiply price");
}

void construction_and_upgrade() {
    auto definition = inn();
    definition.decoration = true;
    FacilityEconomyInput input;
    const std::array<std::int64_t, 5> costs = {1000, 750, 500, 250, 250};
    const std::array<std::int64_t, 5> durations = {400, 280, 160, 40, 40};
    for (int count = 0; count < 5; ++count) {
        input.legacy_job_counts[8] = count;
        input.legacy_job_counts[9] = count;
        const auto values = derive(definition, input);
        check(values.construction_cost == costs[static_cast<std::size_t>(count)] &&
                  values.construction_ticks == durations[static_cast<std::size_t>(count)],
              "construction discount and speed stop changing after three");
    }
    definition.decoration = false;
    check(derive(definition, input).construction_cost == 1000,
          "non-decoration construction ignores discount group");
    input.completed_definition_uses = 49;
    check(!derive(definition, input).upgrade_ready, "before shared definition threshold");
    input.completed_definition_uses = 50;
    check(derive(definition, input).upgrade_ready, "shared definition threshold inclusive");
    input.level = 5;
    input.completed_definition_uses = 500;
    check(!derive(definition, input).upgrade_ready, "fifth level cannot request sixth level");
    input.level = 1;
    definition.upgrade_uses = {0, 0};
    check(!derive(definition, input).upgrade_ready, "zero final endpoint disables upgrade");
}

void invalid_and_overflow() {
    FacilityEconomyInput input;
    for (const int level : {0, 6}) {
        input.level = level;
        const auto result = derive_facility_economy(inn(), input);
        check(result.error == FacilityEconomyError::invalid_input && !result.values.has_value(),
              "invalid level has no partial result");
    }
    input.level = 1;
    input.legacy_job_counts[0] = -1;
    check(derive_facility_economy(inn(), input).error == FacilityEconomyError::invalid_input,
          "negative resident job count rejected");
    input = {};
    auto definition = inn();
    definition.attributes[1].first = -1;
    check(derive_facility_economy(definition, input).error == FacilityEconomyError::invalid_input,
          "negative baseline rejected");
    definition = inn();
    definition.attributes[0] = {INT_MAX, INT_MAX};
    definition.legacy_flags = 4096;
    input.definition_improvements[0] = INT_MAX;
    input.legacy_job_counts[2] = INT_MAX;
    const auto overflow = derive_facility_economy(definition, input);
    check(overflow.error == FacilityEconomyError::numeric_overflow && !overflow.values.has_value(),
          "intermediate overflow rejected before clamp");
    input = {};
    definition = inn();
    definition.attributes[0] = {INT_MAX, INT_MAX};
    input.definition_improvements[0] = INT_MAX;
    const auto wide = derive(definition, input);
    check(wide.definition_attributes[0] == static_cast<std::int64_t>(INT_MAX) * 2,
          "wide type does not reproduce Java integer wrap");
}

} // namespace

int main() {
    fixed_record_baselines();
    improvement_order_and_caps();
    flag_priority();
    construction_and_upgrade();
    invalid_and_overflow();
    std::cout << checks << " checks passed\n";
}
