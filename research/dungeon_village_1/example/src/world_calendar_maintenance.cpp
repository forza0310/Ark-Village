#include "dungeon_village_reference/world_calendar_maintenance.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_reference {
namespace {
bool fits(std::int64_t value) {
    return value >= std::numeric_limits<std::int32_t>::min() &&
           value <= std::numeric_limits<std::int32_t>::max();
}
bool valid_owner(const WorldCalendarMaintenanceState &state) {
    std::set<int> humans;
    for (const auto &human : state.humans)
        if (human.definition < 0 || !humans.insert(human.definition).second)
            return false;
    for (const auto definition : state.active_human_definitions)
        if (!humans.count(definition))
            return false;
    std::set<int> definitions;
    for (const auto &definition : state.facility_definitions)
        if (definition.definition < 0 || !definitions.insert(definition.definition).second)
            return false;
    std::set<std::uint64_t> facilities;
    for (const auto &facility : state.facilities)
        if (!facility.identity || !facilities.insert(facility.identity).second ||
            !definitions.count(facility.definition))
            return false;
    std::set<int> items;
    for (const auto &item : state.shop_items)
        if (item.definition < 0 || !items.insert(item.definition).second)
            return false;
    return true;
}
} // namespace
CalendarMaintenanceResult
prepare_world_calendar_maintenance(const WorldCalendarMaintenanceState &state,
                                   const WorldCalendarState &date, WorldCalendarStage stage) {
    if (!valid_owner(state) || !valid_world_calendar_state(date))
        return {CalendarMaintenanceError::invalid_owner, {}};
    CalendarMaintenanceCandidate candidate{state, {}};
    auto &next = candidate.state;
    switch (stage) {
    case WorldCalendarStage::year_statistics: {
        if (static_cast<std::size_t>(date.year) >= next.yearly_statistics.size())
            break; // 原D先检查C.length，不能延长历史数组。
        std::array<std::int32_t, 10> values{next.rank,     next.popularity, 0, 0, next.legacy_v,
                                            next.legacy_D, next.legacy_n0,  0, 0, 0};
        const auto humans = std::count_if(next.humans.begin(), next.humans.end(),
                                          [](const auto &human) { return human.presence != 0; });
        std::int64_t facilities{};
        for (const auto &facility : next.facilities) {
            const auto definition =
                std::find_if(next.facility_definitions.begin(), next.facility_definitions.end(),
                             [&](const auto &d) { return d.definition == facility.definition; });
            if (definition->category == 2 || definition->category == 3)
                ++facilities;
        }
        if (!fits(humans) || !fits(facilities))
            return {CalendarMaintenanceError::overflow, {}};
        values[2] = static_cast<std::int32_t>(humans);
        values[3] = static_cast<std::int32_t>(facilities);
        for (int category = 0; category < 3; ++category) {
            // 源码先分别按月累加收入/支出，再减；保持每个int中间值校验。
            std::int64_t income{}, expense{};
            for (const auto &month : next.monthly_cash) {
                income += month[category][0];
                expense += month[category][1];
                if (!fits(income) || !fits(expense))
                    return {CalendarMaintenanceError::overflow, {}};
            }
            if (!fits(income - expense))
                return {CalendarMaintenanceError::overflow, {}};
            values[category + 7] = static_cast<std::int32_t>(income - expense);
        }
        auto &row = next.yearly_statistics[static_cast<std::size_t>(date.year)];
        for (std::size_t i = 0; i < row.size(); ++i) {
            const auto value = static_cast<std::int64_t>(row[i]) + values[i];
            if (!fits(value))
                return {CalendarMaintenanceError::overflow, {}};
            row[i] = static_cast<std::int32_t>(value);
        }
        break;
    }
    case WorldCalendarStage::year_characters:
        for (auto &human : next.humans)
            std::fill(human.yearly_totals.begin(), human.yearly_totals.end(), 0);
        break;
    case WorldCalendarStage::year_facilities:
        for (auto &facility : next.facilities)
            for (auto &month : facility.yearly_cash)
                month = {0, 0};
        break;
    case WorldCalendarStage::year_refresh:
        next.monthly_cash = {};
        break;
    case WorldCalendarStage::month_kill_reset:
        std::fill(next.monster_month_kills.begin(), next.monster_month_kills.end(), 0);
        break;
    case WorldCalendarStage::month_resident_countdown:
        for (auto &human : next.humans)
            if (human.presence != 0 && human.leave_months > 0)
                --human.leave_months;
        break;
    case WorldCalendarStage::month_resident_presence:
        for (auto &human : next.humans) {
            if (human.presence == 0)
                continue;
            if (std::find(next.active_human_definitions.begin(),
                          next.active_human_definitions.end(),
                          human.definition) != next.active_human_definitions.end()) {
                human.absent_months = 0;
            } else {
                if (human.absent_months == std::numeric_limits<std::int32_t>::max())
                    return {CalendarMaintenanceError::overflow, {}};
                ++human.absent_months;
            }
        }
        break;
    case WorldCalendarStage::month_shop_restock:
        // 原H是by商店道具补货，不是bq任务生成。
        if ((date.month + 1) % 3 == 2 && next.shop_refresh_enabled) {
            bool unlocked{}, changed{};
            for (auto &item : next.shop_items) {
                if (item.minimum_rank == -1 || item.minimum_rank > next.rank)
                    continue;
                const auto missing =
                    static_cast<std::int64_t>(item.maximum_quantity) - item.quantity;
                if (!fits(missing))
                    return {CalendarMaintenanceError::overflow, {}};
                if (missing <= 0)
                    continue;
                if (!fits(missing * 4))
                    return {CalendarMaintenanceError::overflow, {}};
                const auto draw = next.random.draw(3);
                if (draw.error != WorldRandomError::none)
                    return {CalendarMaintenanceError::random_failed, {}};
                const auto raw = ((missing * 4) / 10 + draw.ticket) - 1;
                const auto added = std::clamp<std::int64_t>(raw, 0, missing);
                if (added == 0)
                    continue;
                const auto quantity = static_cast<std::int64_t>(item.quantity) + added;
                if (!fits(quantity))
                    return {CalendarMaintenanceError::overflow, {}};
                item.quantity = static_cast<std::int32_t>(quantity);
                if (item.presence == 0) {
                    item.newly_available = true;
                    item.presence = 1;
                    item.legacy_q = 0;
                    unlocked = true;
                }
                changed = true;
            }
            if (unlocked || changed)
                candidate.pending_requests.push_back(
                    {CalendarMaintenanceRequestKind::hint, unlocked ? 17 : 16, {}, 1});
        }
        break;
    case WorldCalendarStage::month_quarter_items:
        if (date.month == 2 || date.month == 5 || date.month == 8 || date.month == 11) {
            const auto value = static_cast<std::int64_t>(next.quarter_base) + 3;
            if (!fits(value))
                return {CalendarMaintenanceError::overflow, {}};
            next.quarter_counter = static_cast<std::int32_t>(value);
            for (auto &flags : next.item_flags)
                flags &= ~std::uint32_t{4};
        }
        break;
    case WorldCalendarStage::month_equipment:
        for (auto &human : next.humans)
            human.equipment = prepare_world_month_equipment(human.presence, human.equipment);
        break;
    case WorldCalendarStage::month_facility_definitions:
        for (auto &definition : next.facility_definitions)
            definition.month_counter = definition.category == 2 ? 30 : 20;
        break;
    case WorldCalendarStage::month_facility_age:
        for (auto &facility : next.facilities) {
            if (facility.month_age == std::numeric_limits<std::int32_t>::max())
                return {CalendarMaintenanceError::overflow, {}};
            ++facility.month_age;
        }
        break;
    case WorldCalendarStage::month_housing_tax:
        if (date.month == 3 && date.year > 0) {
            std::int64_t total{};
            bool any{};
            for (auto &human : next.humans) {
                if (human.presence == 0 || human.residence_status != 1)
                    continue;
                const auto product = static_cast<std::int64_t>(human.legacy_F) *
                                     (human.profession_type == 7 ? 60 : 30);
                if (!fits(product))
                    return {CalendarMaintenanceError::overflow, {}};
                auto value = product / 100;
                value -= value % 10;
                human.legacy_G = static_cast<std::int32_t>(value);
                total += value;
                if (!fits(total))
                    return {CalendarMaintenanceError::overflow, {}};
                any = true;
            }
            if (any) {
                candidate.pending_requests.push_back(
                    {CalendarMaintenanceRequestKind::script, 122, {}});
                candidate.pending_requests.push_back(
                    {CalendarMaintenanceRequestKind::page, 90, {}});
                candidate.pending_requests.push_back({CalendarMaintenanceRequestKind::script, 123,
                                                      static_cast<std::int32_t>(total)});
            }
        }
        break;
    case WorldCalendarStage::month_all_residents_script:
        if (!next.event203_seen &&
            std::all_of(next.humans.begin(), next.humans.end(),
                        [](const auto &human) { return human.residence_status == 1; }))
            candidate.pending_requests.push_back({CalendarMaintenanceRequestKind::script, 203, {}});
        break;
    case WorldCalendarStage::month_kill_display_clear:
        next.kill_display.clear();
        break;
    default:
        return {CalendarMaintenanceError::unsupported_stage, {}};
    }
    return {CalendarMaintenanceError::none, std::move(candidate)};
}
} // namespace dungeon_village_reference
