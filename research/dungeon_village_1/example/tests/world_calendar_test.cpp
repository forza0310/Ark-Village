#include "dungeon_village_reference/world_calendar.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace dungeon_village_reference;
namespace {
int checks{};
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
// 此无副作用消费者只验证调度夹具；不是生产月界或真实任务/归档实现。
std::optional<WorldCalendarState> fixture_consumer(const WorldCalendarState &state,
                                                   WorldCalendarStage) {
    return state;
}
void normal_and_guards() {
    WorldCalendarState state{0, 0, 0, 42, 0, 77};
    const auto result = prepare_world_calendar(state, 27, {});
    check(result.candidate && result.candidate->state.units == 69 &&
              result.candidate->state.previous_units == 42 &&
              result.candidate->state.month_ticks == 78 && result.candidate->calls.empty(),
          "ordinary date tick does not need or invent rollover consumers");
    const auto zero = prepare_world_calendar(state, 0, {});
    check(zero.candidate && zero.candidate->state.units == 42 &&
              zero.candidate->state.month_ticks == 78,
          "zero advance still increments controller tick as source c(int)");
    for (int s = -1; s <= 8; ++s)
        for (int speed = -1; speed <= 3; ++speed)
            check(reference_world_frame_rounds(s, speed) == (s == 0 && speed == 1 ? 2 : 1),
                  "two rounds only at initial scene0 and setting exactly1");
    const auto missing = prepare_world_calendar({0, 0, 0, 10799, 0, 0}, 1, {});
    check(missing.error == WorldCalendarError::missing_consumer && !missing.candidate,
          "rollover cannot silently skip checkpoint or task consumer");
    check(prepare_world_calendar(state, -1, {}).error == WorldCalendarError::invalid_state,
          "negative advance is outside researched normalized contract");
    for (int field = 0; field < 6; ++field) {
        auto invalid = state;
        switch (field) {
        case 0:
            invalid.year = -1;
            break;
        case 1:
            invalid.month = 12;
            break;
        case 2:
            invalid.subperiod = 4;
            break;
        case 3:
            invalid.units = 10800;
            break;
        case 4:
            invalid.previous_units = -1;
            break;
        case 5:
            invalid.month_ticks = -1;
            break;
        }
        check(prepare_world_calendar(invalid, 27, fixture_consumer).error ==
                  WorldCalendarError::invalid_state,
              "all calendar scalar contracts are validated before callbacks");
    }
    auto overflow = state;
    overflow.month_ticks = std::numeric_limits<std::int32_t>::max();
    check(prepare_world_calendar(overflow, 0, fixture_consumer).error ==
              WorldCalendarError::overflow,
          "tick overflow rejected even if a later month stage would reset it");
    overflow = {std::numeric_limits<std::int32_t>::max(), 11, 3, 10799, 0, 0};
    check(prepare_world_calendar(overflow, 1, fixture_consumer).error ==
              WorldCalendarError::overflow,
          "year overflow does not leak a checkpoint");
    check(prepare_world_calendar(state, std::numeric_limits<std::int32_t>::max(), fixture_consumer)
                  .error == WorldCalendarError::overflow,
          "source signed-int addition boundary rejected, not silently widened into a new rule");
}
void boundaries_and_order() {
    const WorldCalendarState original{0, 11, 3, 10773, 10746, 1599};
    std::vector<WorldCalendarState> observed;
    const auto result = prepare_world_calendar(
        original, 27, [&](const WorldCalendarState &state, WorldCalendarStage) {
            observed.push_back(state);
            return std::optional<WorldCalendarState>{state};
        });
    check(result.candidate && result.candidate->state.year == 1 &&
              result.candidate->state.month == 0 && result.candidate->state.subperiod == 0 &&
              result.candidate->state.units == 0 && result.candidate->state.month_ticks == 0,
          "year/month/subperiod normalization and month tick reset");
    check(observed.front().year == 0 && observed.front().month == 11 &&
              observed.front().units == 10773 && observed.front().previous_units == 10773 &&
              observed.front().month_ticks == 1599,
          "checkpoint sees old date/units and tick before normalization");
    const std::vector<WorldCalendarStage> expected{WorldCalendarStage::checkpoint_before_normalize,
                                                   WorldCalendarStage::year_statistics,
                                                   WorldCalendarStage::year_characters,
                                                   WorldCalendarStage::year_facilities,
                                                   WorldCalendarStage::year_refresh,
                                                   WorldCalendarStage::month_special_scripts,
                                                   WorldCalendarStage::month_kill_reset,
                                                   WorldCalendarStage::month_rank_check,
                                                   WorldCalendarStage::month_resident_countdown,
                                                   WorldCalendarStage::month_resident_presence,
                                                   WorldCalendarStage::month_shop_restock,
                                                   WorldCalendarStage::month_quarter_items,
                                                   WorldCalendarStage::month_equipment,
                                                   WorldCalendarStage::month_facility_definitions,
                                                   WorldCalendarStage::month_facility_age,
                                                   WorldCalendarStage::month_housing_tax,
                                                   WorldCalendarStage::month_all_residents_script,
                                                   WorldCalendarStage::month_kill_display_clear,
                                                   WorldCalendarStage::subperiod_refresh,
                                                   WorldCalendarStage::subperiod_task_deadline,
                                                   WorldCalendarStage::subperiod_task_midpoint,
                                                   WorldCalendarStage::subperiod_task_generation,
                                                   WorldCalendarStage::subperiod_capacity_hint};
    check(result.candidate->calls == expected,
          "source year then month then subperiod exact stages");
    check(observed[1].month_ticks == 1600 && observed[5].month_ticks == 1600 &&
              observed[6].month_ticks == 0 && observed.back().month_ticks == 0,
          "month special scripts precede t reset, later domains observe new month t0");
    check(original.year == 0 && original.month_ticks == 1599, "input calendar remains unchanged");
    auto same_month = prepare_world_calendar({0, 0, 0, 0, 0, 5}, 518400, fixture_consumer);
    check(same_month.candidate && same_month.candidate->state.year == 1 &&
              same_month.candidate->state.month_ticks == 6 &&
              same_month.candidate->calls.size() == 5,
          "whole-year jump does not repeat month/subperiod operations when final values equal");
    auto same_period = prepare_world_calendar({0, 0, 0, 0, 0, 5}, 43200, fixture_consumer);
    check(same_period.candidate && same_period.candidate->state.month == 1 &&
              same_period.candidate->calls.size() == 14,
          "whole-month jump calls one month block but no unchanged subperiod block");
}
void equipment() {
    const std::vector<std::int32_t> slots{-9, 0, 1, 2, std::numeric_limits<std::int32_t>::max()};
    check(prepare_world_month_equipment(0, slots) == slots,
          "absent definition p0 leaves every equipment timer unchanged");
    const std::vector<std::int32_t> expected{-9, 0, 0, 1,
                                             std::numeric_limits<std::int32_t>::max() - 1};
    check(prepare_world_month_equipment(1, slots) == expected &&
              prepare_world_month_equipment(-1, slots) == expected && slots[2] == 1,
          "nonzero p decrements positive slots only, regardless of active actor membership");
    check(prepare_world_month_equipment(7, {}).empty(), "empty source array stays empty");
}
struct FixtureOwner {
    WorldCalendarState calendar;
    int effects{};
    std::vector<std::int32_t> equipment;
};
void owned_rollback() {
    const FixtureOwner original{{0, 0, 3, 10799, 10700, 1599}, 0, {2, 0, -3}};
    OwnedWorldCalendarAdapter<FixtureOwner> adapter;
    adapter.read = [](const FixtureOwner &owner) -> const WorldCalendarState & {
        return owner.calendar;
    };
    adapter.write = [](FixtureOwner &owner) -> WorldCalendarState & { return owner.calendar; };
    adapter.consume = [](const FixtureOwner &owner,
                         WorldCalendarStage stage) -> std::optional<FixtureOwner> {
        auto next = owner;
        ++next.effects;
        if (stage == WorldCalendarStage::month_equipment)
            next.equipment = prepare_world_month_equipment(1, next.equipment);
        if (stage == WorldCalendarStage::subperiod_capacity_hint)
            return {};
        return next;
    };
    const auto failed = prepare_owned_world_calendar(original, 1, adapter);
    check(failed.error == WorldCalendarError::consumer_failed && !failed.state && !failed.audit &&
              original.effects == 0 && original.equipment[0] == 2 && original.calendar.month == 0,
          "late task consumer failure rolls back date and earlier equipment/effects as one owner");
    adapter.consume = [](const FixtureOwner &owner, WorldCalendarStage stage) {
        auto next = owner;
        ++next.effects;
        if (stage == WorldCalendarStage::month_equipment)
            next.equipment = prepare_world_month_equipment(1, next.equipment);
        return std::optional<FixtureOwner>{next};
    };
    const auto success = prepare_owned_world_calendar(original, 1, adapter);
    check(success.state && success.audit && success.state->equipment[0] == 1 &&
              success.state->calendar.month == 1 && success.state->effects == 19,
          "successful fixture returns shared calendar and domain mutation together");
    adapter.consume = [](const FixtureOwner &owner, WorldCalendarStage) {
        auto next = owner;
        ++next.calendar.month_ticks;
        return std::optional<FixtureOwner>{next};
    };
    const auto mutation = prepare_owned_world_calendar(original, 1, adapter);
    check(mutation.error == WorldCalendarError::invalid_calendar_mutation && !mutation.state,
          "consumer cannot take ownership of date/t scalar mutation");
}
void normalized_property() {
    for (int month = 0; month < 12; ++month)
        for (int period = 0; period < 4; ++period)
            for (int units : {0, 10773, 10799})
                for (int advance : {0, 1, 27, 10800, 43200, 518400}) {
                    WorldCalendarState original{2, month, period, units, 0, 7};
                    const auto result = prepare_world_calendar(original, advance, fixture_consumer);
                    check(result.candidate && valid_world_calendar_state(result.candidate->state),
                          "all admitted calendar candidates remain normalized");
                    const auto absolute = [](const WorldCalendarState &s) {
                        return (
                            ((static_cast<std::int64_t>(s.year) * 12 + s.month) * 4 + s.subperiod) *
                                10800 +
                            s.units);
                    };
                    check(absolute(result.candidate->state) == absolute(original) + advance,
                          "normalization preserves the exact raw period arithmetic");
                }
}
} // namespace
int main() {
    try {
        normal_and_guards();
        boundaries_and_order();
        equipment();
        owned_rollback();
        normalized_property();
        std::cout << "world_calendar: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "world_calendar: " << e.what() << '\n';
        return 1;
    }
}
