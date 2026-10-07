#include "ark/simulation/rules/world_magic_pot.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace ark::simulation::rules {
namespace {
bool fits(std::int64_t value) {
    return value >= 0 && value <= std::numeric_limits<std::int32_t>::max();
}
bool valid(const WorldMagicPotState &s) {
    const auto capacity = world_magic_pot_capacity(s[11]);
    if (!capacity || std::any_of(s.begin(), s.end(), [](auto n) { return n < 0; }) ||
        s[0] > 999 || s[1] > *capacity || s[2] > *capacity)
        return false;
    for (int i = 3; i < 7; ++i)
        if (s[i] > 999)
            return false;
    // 原只增壶级，最近处理件数不超过当前容量；待元素保留整数残量，不强行清零。
    // 负数/坏级别/超容量拒绝为维护契约，不冒称原APK存在相同异常分支。
    return true;
}
bool valid(const WorldMagicPotItem &item) {
    return item.identity >= 0 &&
           std::all_of(item.elements.begin(), item.elements.end(), [](auto n) { return n >= 0; });
}
bool valid(const WorldMagicPotRecipeDefinition &r) {
    return r.identity >= 0 && r.reward_kind >= 0 && r.reward_kind <= 4 &&
           r.reward_definition >= 0 && r.experience_required >= 0 &&
           std::all_of(r.costs.begin(), r.costs.end(), [](auto n) { return n >= 0; });
}
bool valid(const WorldMagicPotRecipeProgress &p) {
    return p.identity >= 0 && (p.status == 0 || p.status == 1);
}
// c/d.a整数映射的壶有效域：0..待件数映射0..100，零宽返回下端0。
// 向零截断保留原百分比与随后元素整数除100的两次舍入，不能合并成浮点比例。
std::int32_t percentage(std::int32_t processed, std::int32_t pending) {
    return pending == 0 ? 0 : processed * 100 / pending;
}
WorldMagicPotRecipeListResult invalid_list() {
    return {WorldMagicPotError::invalid_input, {}};
}
} // namespace

WorldMagicPotDateResult world_magic_pot_date(const WorldMagicPotDate &date) {
    if (date.year < 0 || date.month < 0 || date.month >= 12 || date.subperiod < 0 ||
        date.subperiod >= 4)
        return {WorldMagicPotError::invalid_input, {}};
    const auto ordinal = std::int64_t(date.year) * 48 + date.month * 4 + date.subperiod;
    if (!fits(ordinal))
        return {WorldMagicPotError::numeric_overflow, {}};
    return {WorldMagicPotError::none, static_cast<std::int32_t>(ordinal)};
}
std::optional<std::int32_t> world_magic_pot_capacity(std::int32_t level) {
    constexpr std::array<std::int32_t, 4> capacities{10, 10, 20, 30};
    if (level < 1 || level > 3)
        return {};
    return capacities[level];
}
bool valid_world_magic_pot_state(const WorldMagicPotState &state, const WorldMagicPotDate &date) {
    const auto time = world_magic_pot_date(date);
    return time.value && valid(state) && state[12] <= *time.value;
}
WorldMagicPotCommentResult world_magic_pot_comment(const WorldMagicPotItem &item) {
    if (!valid(item))
        return {WorldMagicPotError::invalid_input, {}};
    std::int64_t total{};
    for (const auto value : item.elements)
        total += value;
    if (!fits(total))
        return {WorldMagicPotError::numeric_overflow, {}};
    const std::int32_t group = total >= 10 ? 0 : total >= 5 ? 1 : 2;
    constexpr std::array<std::int32_t, 3> bounds{4, 3, 2};
    return {WorldMagicPotError::none, WorldMagicPotComment{group, bounds[group]}};
}
WorldMagicPotDepositResult prepare_world_magic_pot_deposit(
    const WorldMagicPotState &state, const WorldMagicPotItem &item, std::int32_t inventory,
    const WorldMagicPotDate &date, std::int32_t comment_ticket) {
    const auto time = world_magic_pot_date(date);
    if (time.error != WorldMagicPotError::none)
        return {time.error, WorldMagicPotDenial::none, {}};
    if (!valid(state) || !valid(item) || inventory < 0 || inventory > 999 || state[12] > *time.value)
        return {WorldMagicPotError::invalid_input, WorldMagicPotDenial::none, {}};
    if (state[1] == *world_magic_pot_capacity(state[11]))
        return {WorldMagicPotError::none, WorldMagicPotDenial::full19, {}};
    if (inventory == 0)
        return {WorldMagicPotError::none, WorldMagicPotDenial::no_inventory, {}};
    const auto comment = world_magic_pot_comment(item);
    if (comment.error != WorldMagicPotError::none)
        return {comment.error, WorldMagicPotDenial::none, {}};
    if (comment_ticket < 0 || comment_ticket >= comment.candidate->bound)
        return {WorldMagicPotError::invalid_input, WorldMagicPotDenial::none, {}};
    WorldMagicPotDeposit next;
    next.state = state;
    next.inventory = inventory - 1;
    next.comment_group = comment.candidate->group;
    next.comment_ticket = comment_ticket;
    for (int i = 0; i < 4; ++i) {
        const auto sum = std::int64_t(state[i + 3]) + item.elements[i];
        const auto pending = std::int64_t(state[i + 7]) + item.elements[i] / 2;
        if (!fits(sum) || !fits(pending))
            return {WorldMagicPotError::numeric_overflow, WorldMagicPotDenial::none, {}};
        const auto after = static_cast<std::int32_t>(std::min<std::int64_t>(sum, 999));
        next.state[i + 3] = after;
        next.state[i + 7] = static_cast<std::int32_t>(pending);
        next.display[0][i] = state[i + 3];
        next.display[1][i] = after;
        next.display[2][i] = item.elements[i]; // 原显示增量不是封顶后的实际差。
    }
    if (state[1] == 0)
        next.state[12] = *time.value;
    ++next.state[1];
    return {WorldMagicPotError::none, WorldMagicPotDenial::none, std::move(next)};
}
WorldMagicPotProcessingResult prepare_world_magic_pot_processing(
    const WorldMagicPotState &state, const WorldMagicPotDate &date) {
    const auto time = world_magic_pot_date(date);
    if (time.error != WorldMagicPotError::none)
        return {time.error, {}};
    if (!valid(state) || state[12] > *time.value)
        return {WorldMagicPotError::invalid_input, {}};
    WorldMagicPotProcessing next;
    next.state = state;
    const auto elapsed = *time.value - state[12];
    if (elapsed == 0)
        return {WorldMagicPotError::none, std::move(next)};
    const auto processed = std::min(elapsed, state[1]);
    const auto ratio = percentage(processed, state[1]);
    bool has_output{};
    for (int i = 0; i < 4; ++i) {
        const auto product = std::int64_t(state[i + 7]) * ratio;
        if (!fits(product))
            return {WorldMagicPotError::numeric_overflow, {}};
        next.produced[i] = static_cast<std::int32_t>(product / 100);
        has_output = has_output || next.produced[i] > 0;
    }
    if (!has_output && processed >= *world_magic_pot_capacity(state[11])) {
        next.produced[0] = next.produced[1] = next.produced[2] = 1;
        has_output = true;
    }
    if (!has_output)
        return {WorldMagicPotError::none, std::move(next)};
    for (int i = 0; i < 4; ++i) {
        const auto sum = std::int64_t(state[i + 3]) + next.produced[i];
        if (!fits(sum))
            return {WorldMagicPotError::numeric_overflow, {}};
        next.state[i + 3] = static_cast<std::int32_t>(std::min<std::int64_t>(sum, 999));
        next.state[i + 7] = std::max(0, state[i + 7] - next.produced[i]);
        next.display[0][i] = state[i + 3];
        next.display[1][i] = next.state[i + 3];
        next.display[2][i] = next.produced[i];
    }
    next.state[0] = std::min(999, state[0] + processed);
    next.state[1] -= processed;
    next.state[2] = processed;
    next.state[12] = *time.value;
    next.processed = processed;
    next.percentage = ratio;
    next.changed = true;
    next.display[0][4] = state[0];
    next.display[1][4] = next.state[0];
    next.display[2][4] = processed;
    return {WorldMagicPotError::none, std::move(next)};
}
WorldMagicPotRecipeListResult catalogue_world_magic_pot_recipes(
    const std::vector<WorldMagicPotRecipeDefinition> &definitions) {
    std::set<std::int32_t> seen;
    std::vector<std::int32_t> ids;
    for (const auto &recipe : definitions) {
        if (!valid(recipe) || !seen.insert(recipe.identity).second)
            return invalid_list();
        if ((recipe.flags & 2U) != 0)
            ids.push_back(recipe.identity);
    }
    return {WorldMagicPotError::none, std::move(ids)};
}
WorldMagicPotRecipeListResult discoverable_world_magic_pot_recipes(
    const WorldMagicPotState &state,
    const std::vector<WorldMagicPotRecipeDefinition> &definitions,
    const std::vector<WorldMagicPotRecipeProgress> &progress) {
    if (!valid(state) || definitions.size() != progress.size())
        return invalid_list();
    std::map<std::int32_t, WorldMagicPotRecipeProgress> statuses;
    for (const auto &p : progress)
        if (!valid(p) || !statuses.emplace(p.identity, p).second)
            return invalid_list();
    const auto catalogue = catalogue_world_magic_pot_recipes(definitions);
    if (catalogue.error != WorldMagicPotError::none)
        return catalogue;
    std::vector<std::int32_t> ids;
    for (const auto &recipe : definitions) {
        const auto p = statuses.find(recipe.identity);
        if (p == statuses.end())
            return invalid_list();
        if ((recipe.flags & 2U) != 0 && p->second.status != 1 &&
            state[0] >= recipe.experience_required)
            ids.push_back(recipe.identity);
    }
    return {WorldMagicPotError::none, std::move(ids)};
}
WorldMagicPotRecipeGate check_world_magic_pot_recipe_selection(
    const WorldMagicPotRecipeDefinition &recipe, const WorldMagicPotRecipeProgress &progress,
    std::int32_t reward_status) {
    if (!valid(recipe) || !valid(progress) || recipe.identity != progress.identity ||
        reward_status < 0 || reward_status > 2)
        return {WorldMagicPotError::invalid_input, WorldMagicPotDenial::none};
    if (progress.status == 0)
        return {WorldMagicPotError::none, WorldMagicPotDenial::undiscovered};
    if ((recipe.reward_kind >= 1 && recipe.reward_kind <= 3 && reward_status == 1) ||
        (recipe.reward_kind == 4 && reward_status == 2))
        return {WorldMagicPotError::none, WorldMagicPotDenial::already_owned106};
    return {};
}
WorldMagicPotRecipeCostResult prepare_world_magic_pot_recipe_cost(
    const WorldMagicPotState &state, const WorldMagicPotRecipeDefinition &recipe) {
    if (!valid(state) || !valid(recipe))
        return {WorldMagicPotError::invalid_input, WorldMagicPotDenial::none, {}};
    for (int i = 0; i < 4; ++i)
        if (state[i + 3] < recipe.costs[i])
            return {WorldMagicPotError::none, WorldMagicPotDenial::insufficient_elements, {}};
    WorldMagicPotRecipeCost next{state, recipe.identity, recipe.reward_kind, recipe.reward_definition};
    for (int i = 0; i < 4; ++i)
        next.state[i + 3] -= recipe.costs[i];
    return {WorldMagicPotError::none, WorldMagicPotDenial::none, std::move(next)};
}
WorldMagicPotDiscoveryResult prepare_world_magic_pot_discovery(
    const WorldMagicPotRecipeProgress &progress) {
    if (!valid(progress))
        return {WorldMagicPotError::invalid_input, {}};
    auto next = progress;
    if (next.status == 0)
        next.pending_notice = true;
    next.status = 1;
    return {WorldMagicPotError::none, std::move(next)};
}
} // namespace ark::simulation::rules
