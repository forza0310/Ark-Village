#include "dungeon_village_prototype/startup_world_clear_score.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"

#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
static_assert(std::numeric_limits<float>::is_iec559 && std::numeric_limits<float>::digits == 24,
              "APK score projection requires IEEE binary32 float");
constexpr std::array<float, 6> coefficients{{5, 300, 500, 50, 1000, 10}};
constexpr std::array<int, 8> thresholds{{75, 75, 45, 45, 65, 45, 65, 9999}};
using Error = StartupClearScoreError;
bool add_count(std::int64_t &sum, int value) {
    if (value < 0 || sum > std::numeric_limits<std::int32_t>::max() - value)
        return false;
    sum += value;
    return true;
}
std::int64_t score(std::int64_t count, std::size_t row) {
    // APK先long→binary32再binary32乘法；保留两次舍入，不能改成整数／double乘。
    const float quantity = static_cast<float>(count);
    const float product = quantity * coefficients[row];
    return static_cast<std::int64_t>(product);
}
bool valid_rows(const StartupClearScoreRows &rows) {
    for (std::size_t i = 0; i < rows.size(); ++i)
        if (rows[i].count < 0 || rows[i].count > std::numeric_limits<std::int32_t>::max() ||
            rows[i].score != score(rows[i].count, i))
            return false;
    return true;
}
} // namespace

StartupClearScoreResult startup_world_clear_score(const StartupWorldRuntimeState &s) {
    auto fail = [](Error e) { return StartupClearScoreResult{e, {}}; };
    if (!s.rules || s.popularity < 0 || s.task_progress.successes < 0)
        return fail(Error::invalid_input);
    StartupClearScoreRows rows;
    rows[0].count = s.popularity;
    rows[1].count = s.task_progress.successes;
    std::map<int, int> kinds;
    for (const auto &d : s.rules->facilities) {
        if (!kinds.emplace(d.id, d.kind).second)
            return fail(Error::invalid_input);
        const auto p = s.facility_presence.find(d.id);
        if (p == s.facility_presence.end())
            return fail(Error::missing_binding);
        if (p->second < 0)
            return fail(Error::invalid_input);
        if (p->second != 0 && (d.kind == 2 || d.kind == 3) && !add_count(rows[2].count, 1))
            return fail(Error::numeric_overflow);
    }
    std::set<int> humans;
    for (const auto &d : s.rules->humans) {
        if (!humans.insert(d.identity).second)
            return fail(Error::invalid_input);
        const auto p = s.human_presence.find(d.identity);
        if (p == s.human_presence.end())
            return fail(Error::missing_binding);
        if (p->second < 0)
            return fail(Error::invalid_input);
        if (p->second == 0)
            continue;
        const auto g = s.scene.world.world.ai.growth.find(d.identity);
        if (g == s.scene.world.world.ai.growth.end())
            return fail(Error::missing_binding);
        if (g->second.definition.profession_levels.size() != s.rules->jobs.size())
            return fail(Error::invalid_input);
        for (const int level : g->second.definition.profession_levels) {
            if (level < 0)
                return fail(Error::invalid_input);
            if (!add_count(rows[3].count, level))
                return fail(Error::numeric_overflow);
        }
        if (g->second.definition.legacy_u < 0)
            return fail(Error::invalid_input);
        if (!add_count(rows[5].count, g->second.definition.legacy_u))
            return fail(Error::numeric_overflow);
    }
    std::set<std::uint64_t> instances;
    for (const auto id : s.scene.world.facility_order) {
        if (!instances.insert(id).second)
            return fail(Error::invalid_input);
        const auto instance = s.scene.world.world.facilities.find(id);
        if (instance == s.scene.world.world.facilities.end())
            return fail(Error::missing_binding);
        const auto kind = kinds.find(instance->second.placement.definition_id);
        if (kind == kinds.end())
            return fail(Error::missing_binding);
        if (kind->second == 12 && !add_count(rows[4].count, 1))
            return fail(Error::numeric_overflow);
    }
    for (std::size_t i = 0; i < rows.size(); ++i)
        rows[i].score = score(rows[i].count, i);
    return {Error::none, rows};
}

StartupClearScorePageResult prepare_startup_clear_score_page(
    const StartupClearScoreRows &rows, const StartupClearScorePageState &s, bool confirm) {
    auto fail = [](Error e) { return StartupClearScorePageResult{e, {}, false, {}}; };
    if (s.finished)
        return fail(Error::already_finished);
    if (!valid_rows(rows) || s.stage < 0 || s.stage > 6 || s.counter < 0 ||
        s.counter > thresholds[s.stage] || s.row < 0 || s.row > 6 ||
        s.captured_high_score < 0 || s.new_record || s.trophy != 0)
        return fail(Error::invalid_input);
    const bool end_stage = s.stage == 4 || s.stage == 6;
    if ((s.stage == 0 && s.row != 0) || (end_stage && s.row != 6) ||
        (!end_stage && s.row >= 6) || (s.stage == 5 && s.row == 0))
        return fail(Error::invalid_input);
    std::int64_t expected{};
    for (int i = 0; i < s.row; ++i)
        expected += rows[static_cast<std::size_t>(i)].score;
    if (s.stage == 3 && s.counter == thresholds[3])
        expected += rows[static_cast<std::size_t>(s.row)].score;
    if (s.sum != expected)
        return fail(Error::invalid_input);
    if (s.stage == 4 && s.sum <= s.captured_high_score)
        return fail(Error::invalid_input);
    auto next = s;
    if (next.counter < thresholds[next.stage]) {
        ++next.counter;
        if (next.stage == 3 && next.counter == thresholds[3])
            next.sum += rows[static_cast<std::size_t>(next.row)].score;
    }
    bool done{};
    if (next.counter >= thresholds[next.stage]) {
        switch (next.stage) {
        case 0:
            if (confirm) { next.stage = 1; next.counter = 0; }
            break;
        case 1: next.stage = 2; next.counter = 0; break;
        case 2: next.stage = 3; next.counter = 0; break;
        case 3:
            if (confirm) {
                ++next.row;
                next.counter = 0;
                next.stage = next.row < 6 ? 5 : next.sum > next.captured_high_score ? 4 : 6;
            }
            break;
        case 4: next.stage = 6; next.counter = 0; break;
        case 5: next.stage = 1; next.counter = 0; break;
        case 6:
            next.finished = done = true;
            next.new_record = next.sum > next.captured_high_score;
            if (next.new_record)
                next.trophy = next.sum >= 100000 ? 4 : next.sum >= 75000 ? 3 :
                              next.sum >= 50000 ? 2 : 1;
            next.stage = 7;
            next.counter = 0;
            break;
        }
    }
    return {Error::none, next, done, {}};
}
} // namespace dungeon_village_prototype
