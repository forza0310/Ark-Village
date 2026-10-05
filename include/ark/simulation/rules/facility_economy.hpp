#pragma once

// Pure integer economy derivation with shared definition and instance modifiers kept separate.

#include <array>
#include <cstdint>
#include <optional>

namespace ark::simulation::rules {

struct LevelEndpoints {
    std::int32_t first{};
    std::int32_t fifth{};
};

struct FacilityEconomyDefinition {
    std::array<LevelEndpoints, 4> attributes;
    LevelEndpoints upgrade_uses;
    std::int32_t construction_cost{};
    std::int32_t construction_ticks{};
    std::uint32_t legacy_flags{};
    bool decoration{};
};

struct FacilityEconomyInput {
    int level{1};
    std::array<std::int32_t, 4> definition_improvements{};
    std::array<std::int32_t, 4> instance_modifiers{};
    std::array<std::int32_t, 10> legacy_job_counts{};
    std::uint64_t completed_definition_uses{};
};

struct FacilityEconomyValues {
    std::array<std::int64_t, 4> definition_attributes{};
    std::array<std::int64_t, 4> instance_attributes{};
    std::int64_t construction_cost{};
    std::int64_t construction_ticks{};
    std::int64_t upgrade_uses{};
    bool upgrade_ready{};
};

enum class FacilityEconomyError { none, invalid_input, numeric_overflow };
struct FacilityEconomyResult {
    FacilityEconomyError error{FacilityEconomyError::none};
    std::optional<FacilityEconomyValues> values;
};

// Derive all four slots and upgrade readiness without mutation; readiness does not increment the
// level.
FacilityEconomyResult derive_facility_economy(const FacilityEconomyDefinition &definition,
                                              const FacilityEconomyInput &input);
struct FacilityUpgradeCandidate {
    int level{};
    std::uint64_t remaining_uses{};
    FacilityEconomyValues values;
    std::array<std::array<std::int64_t, 3>, 3> display{}; // 原o.ap：旧、新、差额；含当前实例邻接。
};
// a/o.a(o,m)：先减本级门槛再升共享等级；提示清除由raw81关闭消费者单独提交。
std::optional<FacilityUpgradeCandidate>
prepare_facility_upgrade(const FacilityEconomyDefinition &definition,
                         const FacilityEconomyInput &input);

} // namespace ark::simulation::rules
