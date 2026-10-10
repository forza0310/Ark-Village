#pragma once

#include "ark/app/session/world_session.hpp"
#include <istream>
#include <map>
#include <ostream>
#include <stdexcept>

namespace ark::test {
// Sidecar observations, never world state. Only revenue wholly inside a calendar
// month counts: a sale on the boundary cannot certify the preceding full month.
struct ActiveTradeEvidence {
    std::map<int, std::int64_t> revenue;
    void observe(const app::WorldState &before, const app::WorldState &after) {
        const int a = before.scene.calendar.year * 12 + before.scene.calendar.month;
        const int b = after.scene.calendar.year * 12 + after.scene.calendar.month;
        if (a != b)
            return;
        for (const auto &[id, facility] : after.scene.world.world.facilities) {
            const auto old = before.scene.world.world.facilities.find(id);
            if (old != before.scene.world.world.facilities.end() &&
                facility.sales > old->second.sales)
                revenue[a] += facility.sales - old->second.sales;
        }
    }
    bool full_month_after(int milestone, int current) const {
        if (milestone < 0)
            return false;
        for (const auto &[month, sales] : revenue)
            if (month > milestone && month < current && sales > 0)
                return true;
        return false;
    }
    void encode(std::ostream &out) const {
        out << revenue.size();
        for (const auto &[month, sales] : revenue)
            out << ' ' << month << ' ' << sales;
        out << '\n';
    }
    void decode(std::istream &in) {
        std::size_t size{};
        in >> size;
        if (!in || size > 10000)
            throw std::runtime_error("Invalid monthly trade evidence size");
        for (std::size_t n = 0; n < size; ++n) {
            int month{};
            std::int64_t sales{};
            in >> month >> sales;
            if (!in || month < 0 || sales <= 0 || !revenue.emplace(month, sales).second)
                throw std::runtime_error("Invalid monthly trade evidence row");
        }
    }
};
} // namespace ark::test
