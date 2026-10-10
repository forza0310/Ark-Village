#include "dungeon_village_prototype/startup_world_information.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Page = ref::WorldScriptPage;
constexpr std::array<StartupInformationEntry, 5> entries{{
    {15, "冒险者", 35, false}, {14, "村情报", 34, false},
    {16, "收支情报", 36, true}, {17, "持有物品", 37, false},
    {18, "装备一览", 38, false}}};
bool information(const Page &page) {
    return page.kind == ref::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 9 || page.legacy_page == 36);
}
const Page *find_page(const State &s, std::uint64_t id) {
    const Page *found = nullptr;
    for (const auto &page : s.scripts.pages)
        if (page.id == id) {
            if (found || id == 0)
                return nullptr;
            found = &page;
        }
    return found;
}
const Page *top(const State &s) {
    const auto page = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    return page == s.scripts.pages.rend() ? nullptr : &*page;
}
bool payload(const State &s, const Page &page) {
    const auto phase = s.page_phases.find(page.id);
    const auto counter = s.page_counters.find(page.id);
    return phase != s.page_phases.end() && counter != s.page_counters.end() &&
           phase->second >= 0 && phase->second <= (page.legacy_page == 9 ? 4 : 1) &&
           counter->second >= 0 &&
           counter->second <= (page.legacy_page == 9 ? 3 : std::numeric_limits<int>::max() - 1);
}
bool close(State &s, std::uint64_t id) {
    const auto result = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
    return result.candidate && write_startup_world_runtime_scripts(s, result.candidate->state);
}
bool push(State &s, int raw) {
    Page child;
    child.kind = ref::WorldScriptPageKind::raw_page;
    child.legacy_page = raw;
    child.title = raw == 9 ? "情报" : "收支情报";
    const auto result = ref::prepare_world_script_page(startup_world_runtime_scripts(s), child);
    return result.candidate && result.candidate->inserted_pages.size() == 1 &&
           write_startup_world_runtime_scripts(s, result.candidate->state);
}
bool menu_class(const Page &page) {
    if (page.kind != ref::WorldScriptPageKind::raw_page)
        return false;
    switch (page.legacy_page) {
    case 3: case 4: case 5: case 7: case 8: case 9: case 10: case 20:
        return true; // 原b/g.e()；36不在菜单类内。
    default:
        return false;
    }
}
} // namespace

bool valid_startup_world_information_page(const State &s, std::uint64_t id) {
    const auto *page = find_page(s, id);
    if (!page || !information(*page) || page->lifecycle < 0 || page->lifecycle > 4)
        return false;
    const bool no_payload = !s.page_phases.count(id) && !s.page_counters.count(id);
    if (page->lifecycle == 0)
        return no_payload;
    if (page->lifecycle == 4 && no_payload)
        return true; // 允许框架清载荷后的退休对象，不让其再次初始化。
    return payload(s, *page);
}

bool initialize_startup_world_information_pages(State &s) {
    if (s.scene.framework_paused)
        return true;
    // 先检查所有页，避免一个坏父页导致前面的新页留下半份初始化。
    for (const auto &page : s.scripts.pages)
        if (information(page) && !valid_startup_world_information_page(s, page.id))
            return false;
    for (auto &page : s.scripts.pages)
        if (information(page) && page.lifecycle == 0) {
            s.page_phases.emplace(page.id, 0);
            s.page_counters.emplace(page.id, 0);
            page.lifecycle = 1;
        }
    return true;
}

Error open_startup_world_information_menu(State &s) {
    const auto *parent = top(s);
    if (!s.rules || s.scene.framework_paused || !parent || parent->lifecycle != 2 ||
        !((parent->kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0) ||
          (parent->kind == ref::WorldScriptPageKind::raw_page && parent->legacy_page == 3)))
        return Error::invalid_page;
    const auto id = parent->id;
    auto next = s;
    next.scripts.executing_page = id;
    for (auto &page : next.scripts.pages)
        if (page.id == id)
            page.lifecycle = 3;
    if (!push(next, 9))
        return Error::script_failed;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}

Error input_startup_world_information_page(State &s, std::uint64_t id,
                                          const StartupInformationInput &input) {
    const auto *page = top(s);
    if (!s.rules || s.scene.framework_paused || !page || page->id != id ||
        page->lifecycle != 2 || !information(*page))
        return Error::invalid_page;
    if (!valid_startup_world_information_page(s, id))
        return Error::missing_source;
    const int raw = page->legacy_page;
    const bool keys = input.up || input.down || input.left || input.right ||
                      input.confirm || input.cancel;
    if (input.select_row && (raw != 9 || keys || *input.select_row < 0 || *input.select_row > 4))
        return Error::invalid_page;
    if (raw == 36 && (input.up || input.down))
        return Error::invalid_page;
    if (!keys && !input.select_row)
        return Error::none;
    auto next = s;
    next.scripts.executing_page = id;
    auto phase = next.page_phases.find(id);
    auto counter = next.page_counters.find(id);
    if (raw == 9) {
        if (input.select_row)
            phase->second = *input.select_row;
        else if (input.up)
            phase->second = (phase->second + 4) % 5;
        else if (input.down)
            phase->second = (phase->second + 1) % 5;
        else if (input.confirm || input.right) {
            if (!entries[static_cast<std::size_t>(phase->second)].implemented)
                return Error::invalid_page;
            counter->second = 3;
            // 先压36，后逆序退休菜单；不能先关9再猜一个父页。
            if (!push(next, 36))
                return Error::script_failed;
            std::vector<std::uint64_t> retiring;
            for (auto p = next.scripts.pages.rbegin(); p != next.scripts.pages.rend(); ++p)
                if (p->lifecycle != 4 && menu_class(*p))
                    retiring.push_back(p->id);
            for (const auto closing : retiring)
                if (!close(next, closing))
                    return Error::script_failed;
        } else if (input.cancel || input.left) {
            if (!close(next, id))
                return Error::script_failed;
        }
    } else {
        // 原36左右独立：同轮双向回原页，随后仍可确认/返回关闭。
        if (input.left)
            phase->second = (phase->second + 1) % 2;
        if (input.right)
            phase->second = (phase->second + 1) % 2;
        if ((input.confirm || input.cancel) && !close(next, id))
            return Error::script_failed;
    }
    if (keys || input.select_row)
        next.scripts.redraw_requested = true;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}

std::optional<State> update_startup_world_information_page(const State &s, std::uint64_t id) {
    const auto *page = top(s);
    if (!page || page->id != id || page->lifecycle != 2 || !information(*page) ||
        !valid_startup_world_information_page(s, id))
        return {};
    if (s.scene.framework_paused)
        return s;
    auto next = s;
    auto counter = next.page_counters.find(id);
    if (page->legacy_page == 9) {
        counter->second = std::min(counter->second + 2, 3); // 公共update加1，h再加1封顶3。
        next.scripts.redraw_requested = true; // 原h()每轮调用表单管理器h()请求重画。
    } else
        counter->second = counter->second == std::numeric_limits<int>::max() - 1
                              ? 0 : counter->second + 1;
    return next;
}

std::optional<StartupInformationPageView>
inspect_startup_world_information_page(const State &s, std::uint64_t id) {
    const auto *page = find_page(s, id);
    if (!page || !information(*page) || page->lifecycle < 1 || page->lifecycle > 3 ||
        !valid_startup_world_information_page(s, id))
        return {};
    const auto phase = s.page_phases.find(id);
    const auto counter = s.page_counters.find(id);
    StartupInformationPageView view{id, page->legacy_page, phase->second, counter->second,
                                    entries, {}};
    if (page->legacy_page == 36) {
        view.income = startup_income_information(s.monthly_cash, s.scene.calendar.month,
                                                 phase->second);
        if (!view.income)
            return {};
    }
    return view;
}
} // namespace dungeon_village_prototype
