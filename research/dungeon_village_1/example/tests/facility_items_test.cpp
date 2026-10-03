#include "dungeon_village_reference/facility_items.hpp"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

using namespace dungeon_village_reference;

namespace {

int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

FacilityItemDefinition cafe() {
    FacilityItemDefinition result;
    result.definition_id = 36;
    result.legacy_icon = 2;
    result.category_affinities = {0, 0, 0, 0, 0, 2, 0, 0, 1, 1, 1, 0};
    result.economy.attributes = {LevelEndpoints{420, 630}, LevelEndpoints{6, 60},
                                 LevelEndpoints{5, 50}, LevelEndpoints{192, 384}};
    result.economy.upgrade_uses = {30, 300};
    result.economy.construction_cost = 700;
    result.economy.construction_ticks = 400;
    result.economy.legacy_flags = 262244;
    return result;
}

ImprovementItemDefinition milk() {
    return {1, 5, {30, 4, 0}};
}

FacilityItemCandidate prepare(const FacilityItemDefinition &facility,
                              const ImprovementItemDefinition &item,
                              const FacilityEconomyInput &input = {},
                              const FacilityEventCounters &counters = {}, int inventory = 1) {
    const auto result = prepare_facility_item(facility, item, input, counters, inventory);
    check(result.error == FacilityItemError::none && result.candidate.has_value(),
          "complete item candidate produced");
    return *result.candidate;
}

void fixed_records_and_affinities() {
    const auto result = prepare(cafe(), milk(), {}, {}, 2);
    check(result.definition_id == 36 && result.item_id == 1 && result.remaining_inventory == 1,
          "identity and inventory carried in complete candidate");
    check(result.applied_improvements == std::array<std::int32_t, 3>{60, 8, 0} &&
              result.definition_improvements == std::array<std::int32_t, 4>{60, 8, 0, 0},
          "cafe category-five doubles milk improvements");
    check(result.before.instance_attributes == std::array<std::int64_t, 4>{420, 6, 5, 190} &&
              result.after.instance_attributes == std::array<std::int64_t, 4>{480, 14, 5, 190} &&
              result.visible_deltas == std::array<std::int64_t, 3>{60, 8, 0} &&
              result.legacy_response == 2,
          "actual old and new attributes include maintenance rounding");
    check(!result.instance_event.script_triggered &&
              result.instance_event.counters.item_confirmations == 1 &&
              result.instance_event.counters.months_since_trigger == 0,
          "item confirmation belongs to selected instance");
    const auto half = prepare(cafe(), {9, 8, {10, 1, 1}});
    check(half.applied_improvements == std::array<std::int32_t, 3>{5, 0, 0} &&
              half.legacy_response == 1,
          "half affinity truncates each odd increment individually");
    const auto neutral = prepare(cafe(), {0, 1, {0, 2, 0}});
    check(neutral.applied_improvements == std::array<std::int32_t, 3>{0, 2, 0} &&
              neutral.legacy_response == 0 && neutral.remaining_inventory == 0,
          "neutral affinity does not disable item use");
    const auto signed_half = prepare(cafe(), {100, 8, {-3, -5, 3}});
    check(signed_half.applied_improvements == std::array<std::int32_t, 3>{-1, -2, 1},
          "signed fixture halves toward zero, not down");
    const auto negative = prepare(cafe(), {100, 5, {-3, -5, -3}});
    check(negative.visible_deltas == std::array<std::int64_t, 3>{-6, -10, -6} &&
              negative.after.instance_attributes[1] == -4,
          "signed fixture does not invent a lower attribute cap");
}

void shared_definition_and_caps() {
    FacilityEconomyInput input;
    input.level = 2;
    input.definition_improvements = {0, 0, 0, 7};
    input.instance_modifiers = {30, 1, 2, 999};
    input.legacy_job_counts[1] = 1;
    input.completed_definition_uses = 123;
    const auto original = input;
    const auto result = prepare(cafe(), milk(), input, {4, 36}, 999);
    check(result.visible_deltas == std::array<std::int64_t, 3>{66, 8, 0},
          "price delta includes definition multiplier before instance modifier");
    check(result.instance_event.script_triggered &&
              result.instance_event.counters.item_confirmations == 0 &&
              result.instance_event.counters.months_since_trigger == 0,
          "confirmed item candidate includes script trigger and two-counter reset");
    check(result.definition_improvements[3] == 7 &&
              result.before.instance_attributes[3] == result.after.instance_attributes[3] &&
              result.after.upgrade_uses == result.before.upgrade_uses &&
              result.after.upgrade_ready == result.before.upgrade_ready &&
              result.after.construction_cost == result.before.construction_cost &&
              result.after.construction_ticks == result.before.construction_ticks,
          "item does not change maintenance improvement, level, uses or construction inputs");
    check(input.level == original.level &&
              input.definition_improvements == original.definition_improvements &&
              input.instance_modifiers == original.instance_modifiers &&
              input.legacy_job_counts == original.legacy_job_counts &&
              input.completed_definition_uses == original.completed_definition_uses,
          "preparing candidate does not mutate caller input");
    auto other_before_input = input;
    other_before_input.instance_modifiers = {100, 10, 20, 0};
    auto other_after_input = other_before_input;
    other_after_input.definition_improvements = result.definition_improvements;
    const auto other_before = derive_facility_economy(cafe().economy, other_before_input);
    const auto other_after = derive_facility_economy(cafe().economy, other_after_input);
    check(other_after.values->instance_attributes[0] -
                      other_before.values->instance_attributes[0] ==
                  66 &&
              other_after.values->instance_attributes[1] -
                      other_before.values->instance_attributes[1] ==
                  8,
          "same definition improvement affects another instance with its own neighbours");
    const FacilityEventCounters other_counters{4, 35};
    check(other_counters.item_confirmations == 4 && other_counters.months_since_trigger == 35,
          "other instance event counters are not shared definition data");
    input = {};
    input.definition_improvements = {10000, 1000, 1000, 0};
    const auto capped = prepare(cafe(), milk(), input, {4, 36});
    check(capped.visible_deltas == std::array<std::int64_t, 3>{0, 0, 0} &&
              capped.legacy_response == -1 && capped.remaining_inventory == 0,
          "all capped attributes still consume item with no-visible-change response");
    check(capped.definition_improvements == std::array<std::int32_t, 4>{10060, 1008, 1000, 0} &&
              capped.instance_event.script_triggered,
          "capped item keeps raw shared accumulation and still triggers instance script");
    const auto empty_delta = prepare(cafe(), {100, 5, {0, 0, 0}}, {}, {4, 36});
    check(empty_delta.legacy_response == -1 && empty_delta.remaining_inventory == 0 &&
              empty_delta.instance_event.script_triggered,
          "zero-effect fixture is a confirmed consumed item, not a cancelled action");
}

void failures_and_limits() {
    auto facility = cafe();
    auto item = milk();
    FacilityEconomyInput input;
    auto rejects = [&](FacilityItemError error, FacilityEventCounters counters = {},
                       int stock = 1) {
        const auto original_improvements = input.definition_improvements;
        const auto result = prepare_facility_item(facility, item, input, counters, stock);
        check(result.error == error && !result.candidate &&
                  input.definition_improvements == original_improvements,
              "failure does not expose partial stock, improvements or counters");
    };
    rejects(FacilityItemError::no_inventory, {}, 0);
    rejects(FacilityItemError::invalid_input, {}, -1);
    rejects(FacilityItemError::invalid_input, {}, 1000);
    facility.definition_id = -1;
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    facility.legacy_icon = -1;
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    facility.category_affinities[0] = 3;
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    facility.category_affinities.clear();
    rejects(FacilityItemError::invalid_input);
    facility = cafe();
    item.item_id = -1;
    rejects(FacilityItemError::invalid_input);
    item = milk();
    item.legacy_category = -1;
    rejects(FacilityItemError::invalid_input);
    item.legacy_category = 12;
    rejects(FacilityItemError::invalid_input);
    item = milk();
    input.level = 6;
    rejects(FacilityItemError::invalid_input);
    input = {};
    rejects(FacilityItemError::invalid_input, {-1, 0});
    rejects(FacilityItemError::invalid_input, {0, -1});
    rejects(FacilityItemError::numeric_overflow, {std::numeric_limits<std::int64_t>::max(), 36});
    const auto maximum = std::numeric_limits<std::int32_t>::max();
    const auto minimum = std::numeric_limits<std::int32_t>::min();
    item.improvements = {maximum, 0, 0};
    rejects(FacilityItemError::numeric_overflow);
    item.improvements = {minimum, 0, 0};
    rejects(FacilityItemError::numeric_overflow);
    item = milk();
    input.definition_improvements[0] = maximum - 59;
    rejects(FacilityItemError::numeric_overflow);
    input.definition_improvements[0] = maximum - 60;
    check(prepare(facility, item, input).definition_improvements[0] == maximum,
          "last representable raw improvement accepted despite cap");
    input = {};
    item.improvements = {-1, 0, 0};
    input.definition_improvements[0] = minimum;
    rejects(FacilityItemError::numeric_overflow);
    facility.economy.attributes[0] = {1000000000, 1000000000};
    facility.economy.legacy_flags = 4096;
    input = {};
    input.legacy_job_counts[2] = 400000000;
    item = {100, 1, {1000000000, 0, 0}};
    rejects(FacilityItemError::numeric_overflow);
    input.legacy_job_counts[2] = maximum;
    rejects(FacilityItemError::numeric_overflow);
}

void random_exact_scaling() {
    std::mt19937 random(0x17E17U);
    for (int trial = 0; trial < 2000; ++trial) {
        auto facility = cafe();
        facility.economy.legacy_flags = 0;
        facility.economy.attributes = {LevelEndpoints{1000, 1000}, LevelEndpoints{1000, 1000},
                                       LevelEndpoints{1000, 1000}, LevelEndpoints{100, 100}};
        const auto affinity = static_cast<std::int32_t>(random() % 3U);
        facility.category_affinities[5] = affinity;
        auto item = milk();
        FacilityEconomyInput input;
        input.definition_improvements[3] = 7;
        std::array<std::int32_t, 4> expected_improvements = input.definition_improvements;
        std::array<std::int32_t, 3> expected_scaled{};
        std::array<std::int64_t, 3> expected_visible{};
        bool any_change = false;
        for (std::size_t slot = 0; slot < 3; ++slot) {
            item.improvements[slot] = static_cast<std::int32_t>(random() % 201U) - 100;
            input.definition_improvements[slot] = static_cast<std::int32_t>(random() % 201U) - 100;
            input.instance_modifiers[slot] = static_cast<std::int32_t>(random() % 101U) - 50;
            auto amount = item.improvements[slot];
            if (affinity == 1) {
                amount = amount < 0 ? -((-amount) / 2) : amount / 2;
            } else if (affinity == 2) {
                amount += amount;
            }
            expected_scaled[slot] = amount;
            expected_improvements[slot] = input.definition_improvements[slot] + amount;
            expected_visible[slot] = amount;
            any_change = any_change || amount != 0;
        }
        const FacilityEventCounters counters{static_cast<std::int64_t>(random() % 7U),
                                             30 + static_cast<std::int64_t>(random() % 11U)};
        const auto inventory = static_cast<int>(random() % 999U) + 1;
        const auto result = prepare(facility, item, input, counters, inventory);
        check(result.applied_improvements == expected_scaled &&
                  result.definition_improvements == expected_improvements &&
                  result.visible_deltas == expected_visible,
              "random small signed values match independent integer scaling and addition");
        check(result.remaining_inventory == inventory - 1 &&
                  result.legacy_response == (any_change ? affinity : -1),
              "random complete consumption and no-change response");
        const bool triggered =
            counters.item_confirmations >= 4 && counters.months_since_trigger >= 36;
        check(result.instance_event.script_triggered == triggered &&
                  result.instance_event.counters.item_confirmations ==
                      (triggered ? 0 : counters.item_confirmations + 1) &&
                  result.instance_event.counters.months_since_trigger ==
                      (triggered ? 0 : counters.months_since_trigger),
              "random item and instance event coordination");
        for (std::size_t slot = 0; slot < 3; ++slot) {
            check(result.before.instance_attributes[slot] ==
                          1000 + input.definition_improvements[slot] +
                              input.instance_modifiers[slot] &&
                      result.after.instance_attributes[slot] ==
                          1000 + expected_improvements[slot] + input.instance_modifiers[slot],
                  "independent noncapped derived attributes");
        }
        check(result.after.instance_attributes[3] == 100 && result.definition_improvements[3] == 7,
              "maintenance unchanged and rounded independently of item");
    }
}

} // namespace

int main() {
    fixed_records_and_affinities();
    shared_definition_and_caps();
    failures_and_limits();
    random_exact_scaling();
    std::cout << checks << " checks passed\n";
}
