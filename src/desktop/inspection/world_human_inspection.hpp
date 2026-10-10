#pragma once

// Bounded, explicit window diagnostics. All choices go through ordinary source consumers;
// this policy never injects a person, money, attributes, dates or page payloads.
#include "ark/simulation/actors/startup_world_human.hpp"
#include "world_task_inspection.hpp"
#include <string>

namespace ark::desktop {
struct WorldHumanInspection {
    std::optional<int> human;
    std::optional<int> equipment;
    bool gift_confirmed{};
    bool cancelled{};
    bool profession_confirmed{};
    bool profession_completed{};
    std::optional<int> target_profession;
    std::optional<simulation::StartupHumanDetails> before_choice;
    std::int64_t cash_before_choice{};
    std::uint64_t draws_before_choice{};
    int stock_before_choice{};
    int points_before_choice{};
    std::optional<std::uint64_t> recruitment;
    std::optional<std::uint64_t> home;
    std::optional<int> resident;
    bool housing_gift{};
    bool tax_confirmed{};
    bool tax_pending{};
    bool tax_collected{};
    std::int64_t cash_before_tax{};
    std::int64_t expected_tax{};
    int income_before_tax{};
    int tax_month{-1};
    // Separate natural-tools policy; existing human/housing preparations keep their choices.
    std::optional<std::uint64_t> item_facility;
    bool facility_item_confirmed{};
    int facility_improvement_before{};
    WorldTaskInspection commerce_tasks;
    std::optional<int> commerce_item;
    bool commerce_bought{};
    std::optional<int> commerce_facility;
    bool commerce_facility_paid{};
};
bool human_inspection_mode(const std::string &mode);
void begin_human_inspection(simulation::StartupWorldRuntimeState &state, const std::string &mode,
                            WorldHumanInspection &inspection);
bool human_inspection_ready(const simulation::StartupWorldRuntimeState &state,
                            const std::string &mode, const WorldHumanInspection &inspection);
// Automatic pages can be opened by a preceding input, so observe the exact update boundary.
void before_human_inspection_update(const simulation::StartupWorldRuntimeState &state,
                                    const std::string &mode, WorldHumanInspection &inspection);
void after_human_inspection_update(const simulation::StartupWorldRuntimeState &state,
                                   WorldHumanInspection &inspection);
// True means this policy handled the page, including waiting for a real initialization,
// animation or automatic consumer. Generic confirmation must not run in that case.
bool apply_human_inspection_input(simulation::StartupWorldRuntimeState &state,
                                  const std::string &mode, WorldHumanInspection &inspection);
} // namespace ark::desktop
