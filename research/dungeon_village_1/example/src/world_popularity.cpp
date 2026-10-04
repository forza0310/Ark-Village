#include "dungeon_village_reference/world_popularity.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <set>
#include <sstream>

namespace dungeon_village_reference {
namespace {
bool integer(const std::string &text, int &out) {
    const auto r = std::from_chars(text.data(), text.data() + text.size(), out);
    return !text.empty() && r.ec == std::errc{} && r.ptr == text.data() + text.size();
}
bool valid(const WorldPopularityState &s) {
    // c/n.J真实初值为current50/maximum0；只有首次a(delta)才提升历史最高值。
    if (s.popularity < -50 || s.maximum < 0 || s.rewards.size() > 1000)
        return false;
    std::set<int> ids;
    for (const auto &r : s.rewards)
        if (r.definition < 0 || r.threshold < 0 || r.status < 0 || !ids.insert(r.definition).second)
            return false;
    return true;
}
} // namespace
WorldPopularityTableResult parse_world_popularity_rewards(const std::string &table) {
    std::istringstream rows(table);
    std::string row;
    std::vector<WorldPopularityReward> rewards;
    std::set<int> ids;
    while (std::getline(rows, row)) {
        if (row.empty())
            return {WorldPopularityError::invalid_table, {}};
        std::istringstream fields(row);
        std::string text;
        std::vector<std::string> values;
        while (std::getline(fields, text, '\t'))
            values.push_back(text);
        if (values.size() != 7 || row.back() == '\t')
            return {WorldPopularityError::invalid_table, {}};
        WorldPopularityReward r;
        int flags{};
        if (!integer(values[0], r.definition) || !integer(values[1], r.legacy_tag) ||
            !integer(values[2], r.threshold) || !integer(values[3], r.human) ||
            !integer(values[4], r.facility) || !integer(values[6], flags) || flags < 0 ||
            r.definition < 0 || r.threshold < 0 || !ids.insert(r.definition).second)
            return {WorldPopularityError::invalid_table, {}};
        const auto p = parse_world_script_program(values[5]);
        if (!p || rewards.size() == 1000)
            return {WorldPopularityError::invalid_table, {}};
        r.program = *p;
        r.flags = static_cast<std::uint32_t>(flags);
        rewards.push_back(std::move(r));
    }
    if (rewards.empty())
        return {WorldPopularityError::invalid_table, {}};
    return {WorldPopularityError::none, std::move(rewards)};
}
std::optional<WorldScriptCatalog>
world_popularity_script_catalog(const WorldScriptCatalog &source,
                                const std::vector<WorldPopularityReward> &rewards) {
    if (rewards.size() > 1000)
        return {};
    auto catalog = source;
    for (std::size_t index = 0; index < rewards.size(); ++index) {
        const int identity = 1000 + static_cast<int>(index);
        if (catalog.events.count(identity))
            return {};
        const auto old = catalog.programs.find(identity);
        if (old != catalog.programs.end() && old->second != rewards[index].program)
            return {};
        catalog.programs[identity] = rewards[index].program;
    }
    return catalog;
}
WorldPopularityResult prepare_world_popularity(const WorldScriptCatalog &source,
                                               const WorldPopularityState &s, int delta,
                                               bool show_notice) {
    if (!valid(s))
        return {WorldPopularityError::invalid_input, WorldScriptError::none, {}};
    auto c = WorldPopularityCandidate{s, {}, {}, {}};
    const auto fail = [](WorldPopularityError error,
                         WorldScriptError script = WorldScriptError::none) {
        return WorldPopularityResult{error, script, {}};
    };
    const auto amount = static_cast<std::int64_t>(s.popularity) + delta;
    if (amount < std::numeric_limits<int>::min() || amount > std::numeric_limits<int>::max())
        return fail(WorldPopularityError::overflow);
    c.state.popularity = std::max(static_cast<int>(amount), -50);
    c.state.maximum = std::max(s.maximum, c.state.popularity);
    if (delta != 0 && show_notice) {
        std::int64_t total = delta;
        for (const auto &pulse : s.pulses)
            total += pulse[2];
        if (total < std::numeric_limits<int>::min() || total > std::numeric_limits<int>::max())
            return fail(WorldPopularityError::overflow);
        c.state.pulses = {{0, s.pulses.empty() ? 0 : 3, static_cast<int>(total)}};
    }
    const auto registered = world_popularity_script_catalog(source, s.rewards);
    if (!registered)
        return fail(WorldPopularityError::invalid_table);
    WorldScriptError script_error{WorldScriptError::none};
    const auto run = [&](int id, bool program) {
        const WorldScriptInput input{id, {}, {}};
        const auto r = program ? prepare_world_script_program(*registered, c.state.scripts, input)
                               : prepare_world_script(*registered, c.state.scripts, input);
        if (!r.candidate) {
            script_error = r.error;
            return false;
        }
        c.state.scripts = r.candidate->state;
        c.trace.insert(c.trace.end(), r.candidate->executed.begin(), r.candidate->executed.end());
        if (!program)
            c.invoked_events.push_back(id);
        return true;
    };
    std::optional<std::size_t> reward;
    for (std::size_t index = 0; index < c.state.rewards.size(); ++index)
        if (c.state.rewards[index].status == 0 &&
            c.state.popularity >= c.state.rewards[index].threshold) {
            reward = index;
            c.state.rewards[index].status = 1;
            c.state.rewards[index].pending_notice = true;
            c.awarded_definition = c.state.rewards[index].definition;
            break;
        }
    if (!reward && c.state.popularity > 10000 && s.maximum / 100 != c.state.maximum / 100) {
        if (c.state.rewards.empty())
            return fail(WorldPopularityError::invalid_input);
        reward = c.state.rewards.size() - 1;
    }
    if (reward) {
        c.state.reward_display = true;
        if (!run(124, false) || !run(1000 + static_cast<int>(*reward), true) || !run(125, false))
            return fail(WorldPopularityError::script_failed, script_error);
    }
    constexpr std::array<int, 5> thresholds{1000, 3000, 5000, 10000, 20000};
    for (std::size_t n = 0; n < thresholds.size(); ++n) {
        const int id = 220 + static_cast<int>(n);
        if (c.state.popularity >= thresholds[n] && !world_script_seen(c.state.scripts, id) &&
            !run(id, false))
            return fail(WorldPopularityError::script_failed, script_error);
    }
    return {WorldPopularityError::none, WorldScriptError::none, std::move(c)};
}
WorldPopularityResult prepare_world_popularity_unlock_page(const WorldPopularityState &s,
                                                           std::uint64_t id) {
    if (!valid(s))
        return {WorldPopularityError::invalid_input, WorldScriptError::none, {}};
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [id](const auto &p) { return p.id == id; });
    if (page == s.scripts.pages.end() || page->legacy_page != 97 || page->lifecycle != 2)
        return {WorldPopularityError::invalid_input, WorldScriptError::none, {}};
    const auto closed = prepare_world_script_close_page(s.scripts, id);
    if (!closed.candidate)
        return {WorldPopularityError::script_failed, closed.error, {}};
    WorldPopularityCandidate c{s, {}, {}, {}};
    c.state.scripts = closed.candidate->state;
    c.state.reward_display = false;
    return {WorldPopularityError::none, WorldScriptError::none, std::move(c)};
}
} // namespace dungeon_village_reference
