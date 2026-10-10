#include "ark/simulation/village/rules/world_calendar.hpp"

#include <limits>

namespace ark::simulation::rules {
namespace {
bool same_calendar(const WorldCalendarState &a, const WorldCalendarState &b) {
    return a.year == b.year && a.month == b.month && a.subperiod == b.subperiod &&
           a.units == b.units && a.previous_units == b.previous_units &&
           a.month_ticks == b.month_ticks;
}
} // namespace
int reference_world_frame_rounds(int scene_state, int speed_setting) {
    return scene_state == 0 && speed_setting == 1 ? 2 : 1;
}
bool valid_world_calendar_state(const WorldCalendarState &state) {
    return state.year >= 0 && state.month >= 0 && state.month < 12 && state.subperiod >= 0 &&
           state.subperiod < 4 && state.units >= 0 && state.units < 10800 &&
           state.previous_units >= 0 && state.previous_units < 10800 && state.month_ticks >= 0;
}
WorldCalendarResult prepare_world_calendar(const WorldCalendarState &state, std::int32_t advance,
                                           const WorldCalendarConsumer &consumer) {
    if (!valid_world_calendar_state(state) || advance < 0)
        return {WorldCalendarError::invalid_state, {}};
    const auto total = static_cast<std::int64_t>(state.units) + advance;
    const auto periods = static_cast<std::int64_t>(state.subperiod) + total / 10800;
    const auto months = static_cast<std::int64_t>(state.month) + periods / 4;
    const auto years = static_cast<std::int64_t>(state.year) + months / 12;
    if (total > std::numeric_limits<std::int32_t>::max() ||
        years > std::numeric_limits<std::int32_t>::max() ||
        state.month_ticks == std::numeric_limits<std::int32_t>::max())
        return {WorldCalendarError::overflow, {}};
    WorldCalendarCandidate candidate{state, {}};
    candidate.state.previous_units = state.units;
    const auto call = [&](WorldCalendarStage stage) -> WorldCalendarError {
        if (!consumer)
            return WorldCalendarError::missing_consumer;
        const auto next = consumer(candidate.state, stage);
        if (!next)
            return WorldCalendarError::consumer_failed;
        // 外部消费者不能改写本方法控制的日期或t；各域变更由owned适配器原子返回。
        if (!same_calendar(*next, candidate.state))
            return WorldCalendarError::invalid_calendar_mutation;
        candidate.state = *next;
        candidate.calls.push_back(stage);
        return WorldCalendarError::none;
    };
    if (total >= 10800) {
        const auto error = call(WorldCalendarStage::checkpoint_before_normalize);
        if (error != WorldCalendarError::none)
            return {error, {}};
    }
    candidate.state.year = static_cast<std::int32_t>(years);
    candidate.state.month = static_cast<std::int32_t>(months % 12);
    candidate.state.subperiod = static_cast<std::int32_t>(periods % 4);
    candidate.state.units = static_cast<std::int32_t>(total % 10800);
    ++candidate.state.month_ticks;
    if (state.year != candidate.state.year) {
        for (auto stage : {WorldCalendarStage::year_statistics, WorldCalendarStage::year_characters,
                           WorldCalendarStage::year_facilities, WorldCalendarStage::year_refresh}) {
            const auto error = call(stage);
            if (error != WorldCalendarError::none)
                return {error, {}};
        }
    }
    if (state.month != candidate.state.month) {
        const auto special = call(WorldCalendarStage::month_special_scripts);
        if (special != WorldCalendarError::none)
            return {special, {}};
        candidate.state.month_ticks = 0;
        for (auto stage :
             {WorldCalendarStage::month_kill_reset, WorldCalendarStage::month_rank_check,
              WorldCalendarStage::month_resident_countdown,
              WorldCalendarStage::month_resident_presence, WorldCalendarStage::month_shop_restock,
              WorldCalendarStage::month_quarter_items, WorldCalendarStage::month_equipment,
              WorldCalendarStage::month_facility_definitions,
              WorldCalendarStage::month_facility_age, WorldCalendarStage::month_housing_tax,
              WorldCalendarStage::month_all_residents_script,
              WorldCalendarStage::month_kill_display_clear}) {
            const auto error = call(stage);
            if (error != WorldCalendarError::none)
                return {error, {}};
        }
    }
    if (state.subperiod != candidate.state.subperiod) {
        for (auto stage :
             {WorldCalendarStage::subperiod_refresh, WorldCalendarStage::subperiod_task_deadline,
              WorldCalendarStage::subperiod_task_midpoint,
              WorldCalendarStage::subperiod_task_generation,
              WorldCalendarStage::subperiod_capacity_hint}) {
            const auto error = call(stage);
            if (error != WorldCalendarError::none)
                return {error, {}};
        }
    }
    return {WorldCalendarError::none, std::move(candidate)};
}
std::vector<std::int32_t> prepare_world_month_equipment(std::int32_t definition_presence,
                                                        const std::vector<std::int32_t> &slots) {
    auto result = slots;
    if (definition_presence != 0)
        for (auto &slot : result)
            if (slot > 0)
                --slot;
    return result;
}
} // namespace ark::simulation::rules
