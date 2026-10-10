#include "ark/simulation/village/startup_world_tax.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;

const ref::WorldScriptPage *top_tax_page(const State &state, std::uint64_t id) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->id != id ||
        top->kind != ref::WorldScriptPageKind::raw_page ||
        (top->legacy_page != 90 && top->legacy_page != 98))
        return nullptr;
    return &*top;
}
bool valid_humans(const State &state) {
    if (!state.rules ||
        state.rules->humans.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    std::set<int> definitions;
    for (const auto &human : state.rules->humans) {
        const int id = human.identity;
        if (!definitions.insert(id).second || !state.human_presence.count(id) ||
            !state.human_homes.count(id) || !state.human_calendar.count(id) ||
            !state.scene.world.world.ai.battle.humans.count(id))
            return false;
    }
    return true;
}
std::vector<int> current_residents(const State &state) {
    std::vector<int> residents;
    for (const auto &human : state.rules->humans)
        if (state.human_presence.at(human.identity) != 0 &&
            state.human_homes.at(human.identity)[2] == 1)
            residents.push_back(human.identity);
    return residents;
}
bool valid_list(const State &state, std::uint64_t page) {
    const auto residents = state.tax_page_residents.find(page);
    const auto selection = state.tax_page_selection.find(page);
    const auto first = state.tax_page_scroll.find(page);
    if (residents == state.tax_page_residents.end() ||
        selection == state.tax_page_selection.end() || first == state.tax_page_scroll.end())
        return false;
    std::set<int> definitions;
    for (const int id : residents->second)
        if (!state.human_calendar.count(id) || !definitions.insert(id).second ||
            std::none_of(state.rules->humans.begin(), state.rules->humans.end(),
                         [&](const auto &human) { return human.identity == id; }))
            return false;
    if (residents->second.empty())
        return selection->second == 0 && first->second == 0;
    return selection->second >= 0 &&
           selection->second < static_cast<int>(residents->second.size()) && first->second >= 0 &&
           first->second <= selection->second &&
           static_cast<std::int64_t>(first->second) + 5 > selection->second;
}
bool initialize(State &state, std::uint64_t page, int raw) {
    if (!valid_humans(state))
        return false;
    const auto counter = state.page_counters.find(page);
    if (counter != state.page_counters.end() &&
        (counter->second < 0 || counter->second >= std::numeric_limits<int>::max()))
        return false;
    if (raw == 90) {
        if (state.tax_page_residents.count(page)) {
            if (!valid_list(state, page))
                return false;
        } else {
            if (state.tax_page_selection.count(page) || state.tax_page_scroll.count(page))
                return false;
            state.tax_page_residents[page] = current_residents(state);
            state.tax_page_selection[page] = state.tax_page_scroll[page] = 0;
        }
    }
    if (counter == state.page_counters.end())
        state.page_counters[page] = 0;
    return true;
}
bool close_page(State &state, std::uint64_t id) {
    const auto closed =
        ref::prepare_world_script_close_page(startup_world_runtime_scripts(state), id);
    return closed.candidate && write_startup_world_runtime_scripts(state, closed.candidate->state);
}
std::string grouped_gold(int amount) {
    auto value = std::to_string(amount);
    const std::size_t first = amount < 0 ? 1 : 0;
    for (auto offset = value.size(); offset > first + 3;) {
        offset -= 3;
        value.insert(offset, ",");
    }
    return value + "G"; // b.d.a("G",long)的明确分支，保留半角G。
}
bool pay_tax(State &state, std::uint64_t page) {
    // raw98调用n.q()重新按当时资格合计；不读raw90的冻结名单或事件123的文本替换值。
    std::int64_t total{};
    for (const int id : current_residents(state)) {
        total += state.human_calendar.at(id).legacy_G;
        if (total < std::numeric_limits<int>::min() || total > std::numeric_limits<int>::max())
            return false;
    }
    auto scripts = startup_world_runtime_scripts(state);
    if (!scripts.finance || scripts.finance->month < 0 || scripts.finance->month >= 12)
        return false;
    auto &finance = *scripts.finance;
    const auto month = static_cast<std::size_t>(finance.month);
    const auto income = static_cast<std::int64_t>(finance.monthly_totals[month][4][0]) + total;
    if (income < std::numeric_limits<int>::min() || income > std::numeric_limits<int>::max() ||
        (total > 0 && finance.cash > std::numeric_limits<std::int64_t>::max() - total) ||
        (total < 0 && finance.cash < std::numeric_limits<std::int64_t>::min() - total))
        return false;
    finance.cash += total;
    finance.monthly_totals[month][4][0] = static_cast<int>(income);
    if (finance.legacy_flags14 == 0 && finance.cash > finance.cash_peak) {
        finance.cash_peak = finance.cash;
        finance.cash_peak_village = scripts.village_name;
    }
    const auto replacement = grouped_gold(static_cast<int>(total));
    scripts.notices.push_back({28, -1, 80, replacement, "<co=0064FF>" + replacement + "</co>入手"});
    // 原循环是全部bv，不限居民/已开放定义；F唯一事实位在battle.humans。
    for (const auto &human : state.rules->humans) {
        const int id = human.identity;
        state.scene.world.world.ai.battle.humans.at(id).battle_reward_stat = 0;
        state.human_calendar.at(id).legacy_F = state.human_calendar.at(id).legacy_G = 0;
    }
    const auto closed = ref::prepare_world_script_close_page(scripts, page);
    return closed.candidate && write_startup_world_runtime_scripts(state, closed.candidate->state);
}
} // namespace

Error initialize_startup_world_tax_page(State &state, std::uint64_t page) {
    const auto top = top_tax_page(state, page);
    if (state.scene.framework_paused || !top)
        return Error::invalid_page;
    auto next = state;
    if (!initialize(next, page, top->legacy_page))
        return Error::missing_source;
    state = std::move(next);
    return Error::none;
}
std::optional<State> update_startup_world_tax_page(const State &state, std::uint64_t page) {
    const auto top = top_tax_page(state, page);
    if (!top)
        return {};
    if (state.scene.framework_paused)
        return state;
    auto next = state;
    next.scripts.executing_page = page;
    if (!initialize(next, page, top->legacy_page))
        return {};
    next.page_counters[page] = (next.page_counters[page] + 1) % std::numeric_limits<int>::max();
    if (top->legacy_page == 98 && !pay_tax(next, page))
        return {};
    next.scripts.executing_page.reset();
    return next;
}
Error act_startup_world_tax_page(State &state, std::uint64_t page, StartupWorldTaxAction action,
                                 int selection) {
    const auto top = top_tax_page(state, page);
    if (state.scene.framework_paused || !top || top->legacy_page != 90)
        return Error::invalid_page;
    auto next = state;
    next.scripts.executing_page = page;
    if (!initialize(next, page, 90))
        return Error::missing_source;
    if (action == StartupWorldTaxAction::confirm) {
        if (!close_page(next, page))
            return Error::script_failed;
    } else {
        const int size = static_cast<int>(next.tax_page_residents.at(page).size());
        if (size == 0)
            return Error::invalid_page; // 自然入口有至少一名居民；拒绝原空列表的模零输入。
        auto &chosen = next.tax_page_selection.at(page);
        if (action == StartupWorldTaxAction::previous)
            chosen = chosen == 0 ? size - 1 : chosen - 1;
        else if (action == StartupWorldTaxAction::next)
            chosen = chosen == size - 1 ? 0 : chosen + 1;
        else if (action == StartupWorldTaxAction::select) {
            if (selection < 0 || selection >= size)
                return Error::invalid_page;
            chosen = selection;
        } else {
            return Error::invalid_page;
        }
        auto &first = next.tax_page_scroll.at(page);
        if (first > chosen)
            first = chosen;
        if (static_cast<std::int64_t>(first) + 5 <= chosen)
            first = chosen - 5 + 1;
    }
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}
std::optional<StartupWorldTaxView> inspect_startup_world_tax_page(const State &state,
                                                                  std::uint64_t page) {
    const auto top = top_tax_page(state, page);
    if (!top || top->legacy_page != 90 || !valid_humans(state))
        return {};
    const auto frozen = state.tax_page_residents.find(page);
    if (frozen != state.tax_page_residents.end() && !valid_list(state, page))
        return {};
    const auto residents =
        frozen == state.tax_page_residents.end() ? current_residents(state) : frozen->second;
    StartupWorldTaxView view;
    if (frozen != state.tax_page_residents.end()) {
        view.selection = state.tax_page_selection.at(page);
        view.first_visible = state.tax_page_scroll.at(page);
    }
    std::int64_t total{};
    for (const int definition : residents) {
        const int amount = state.human_calendar.at(definition).legacy_G;
        view.rows.push_back({definition, amount});
        total += amount;
        if (total < std::numeric_limits<int>::min() || total > std::numeric_limits<int>::max())
            return {};
    }
    view.total = static_cast<int>(total);
    return view;
}
} // namespace ark::simulation
