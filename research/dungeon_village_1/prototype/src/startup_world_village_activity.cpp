// 村办51—54的唯一Owner消费者：页面、资源、人物效果与共同随机整轮提交。
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_prototype/startup_world_expansion.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_reference/world_village_activity.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Action = StartupVillageActivityAction;
const ref::WorldScriptPage *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &page) { return page.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
bool supported(int raw) { return raw >= 51 && raw <= 54; }
bool event(State &s, int id) {
    const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                                             startup_world_runtime_scripts(s), {id, {}, {}});
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
bool close(State &s, std::uint64_t id) {
    const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
std::optional<std::uint64_t> open(State &s, int raw, std::optional<int> activity = {}) {
    ref::WorldScriptPage page;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(s), page);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, r.candidate->state))
        return {};
    const auto id = r.candidate->inserted_pages.front().id;
    if (activity)
        s.activity_page_bindings[id] = *activity;
    return id;
}
std::optional<ref::WorldVillageActivityDefinition> definition(const State &s, int id) {
    if (!s.rules)
        return {};
    const auto d = std::find_if(s.rules->activities.begin(), s.rules->activities.end(),
                                [&](const auto &a) { return a.identity == id; });
    const auto status = s.scripts.activities.find(id);
    const auto flags = s.activity_flags.find(id);
    const auto held = s.activity_counts.find(id);
    if (d == s.rules->activities.end() || status == s.scripts.activities.end() ||
        flags == s.activity_flags.end() || held == s.activity_counts.end())
        return {};
    return ref::WorldVillageActivityDefinition{id,
                                               status->second.status,
                                               flags->second,
                                               held->second,
                                               d->parameters[2],
                                               d->parameters[3],
                                               d->parameters[4],
                                               d->parameters[5]};
}
std::optional<std::vector<int>> available(const State &s) {
    if (!s.rules)
        return {};
    std::vector<ref::WorldVillageActivityDefinition> definitions;
    for (const auto &a : s.rules->activities) {
        const auto d = definition(s, a.identity);
        if (!d)
            return {};
        definitions.push_back(*d);
    }
    return ref::catalogue_world_village_activities(definitions);
}
std::optional<std::vector<int>> humans(const State &s) {
    if (!s.rules)
        return {};
    std::vector<int> result;
    for (const auto &h : s.rules->humans) {
        const auto status = s.human_presence.find(h.identity);
        if (status == s.human_presence.end())
            return {};
        if (status->second != 0)
            result.push_back(h.identity);
    }
    return result;
}
bool draw_pair(State &s, const std::vector<int> &list, std::uint64_t page) {
    if (list.empty() || list.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    for (auto &id : s.activity_page_display_humans[page]) {
        const auto draw = s.scene.random.draw(static_cast<int>(list.size()));
        if (draw.error != ref::WorldRandomError::none)
            return false;
        id = list[draw.ticket]; // 有放回，不能去重或预抽下一页。
    }
    return true;
}
bool rebuild(State &s, std::uint64_t id) {
    const auto list = available(s);
    if (!list)
        return false;
    s.activity_page_lists[id] = *list;
    auto &chosen = s.activity_page_selections[id];
    auto &first = s.activity_page_scroll[id];
    if (chosen < 0 || first < 0)
        return false;
    if (chosen >= static_cast<int>(list->size()))
        chosen = 0;
    first = std::clamp(first, 0, std::max(0, static_cast<int>(list->size()) - 5));
    if (first > chosen)
        first = chosen;
    if (first + 5 <= chosen)
        first = chosen - 4;
    return true;
}
std::optional<std::uint64_t> parent(const State &s, std::uint64_t child);
bool human_reference(const State &s, int id) {
    return s.rules &&
           std::any_of(s.rules->humans.begin(), s.rules->humans.end(),
                       [&](const auto &human) { return human.identity == id; }) &&
           s.human_presence.count(id) && s.scene.world.world.ai.growth.count(id);
}
// 已初始化载荷不能靠operator[]补缺值；只验证，不重建目录或消费随机。
bool payload(const State &s, std::uint64_t id, int raw) {
    const auto chosen = s.activity_page_selections.find(id);
    const auto scroll = s.activity_page_scroll.find(id);
    const auto counter = s.page_counters.find(id);
    if (!s.rules || !supported(raw) || !s.activity_pages_initialized.count(id) ||
        chosen == s.activity_page_selections.end() || scroll == s.activity_page_scroll.end() ||
        counter == s.page_counters.end() || chosen->second < 0 || scroll->second < 0 ||
        counter->second < 0)
        return false;
    if (raw != 51) {
        const auto binding = s.activity_page_bindings.find(id);
        if (binding == s.activity_page_bindings.end())
            return false;
        const auto a = definition(s, binding->second);
        if (!a || a->kind < 0 || a->kind > 3 || (raw == 54 && a->kind > 1))
            return false;
    }
    if (raw == 52)
        return chosen->second < 2 && scroll->second == 0 && parent(s, id).has_value();
    if (raw == 53)
        return chosen->second == 0 && scroll->second == 0;
    const auto list = s.activity_page_lists.find(id);
    const auto display = s.activity_page_display_humans.find(id);
    if (list == s.activity_page_lists.end() || display == s.activity_page_display_humans.end() ||
        list->second.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    const int count = static_cast<int>(list->second.size());
    if ((count == 0 && (chosen->second != 0 || scroll->second != 0)) ||
        (count > 0 &&
         (chosen->second >= count || scroll->second > chosen->second ||
          chosen->second - scroll->second >= 5 || scroll->second > std::max(0, count - 5))))
        return false;
    std::set<int> seen;
    for (const int entry : list->second) {
        if (!seen.insert(entry).second)
            return false;
        if (raw == 51) {
            if (!definition(s, entry))
                return false;
        } else if (!human_reference(s, entry) || !s.human_activity_previous.count(entry) ||
                   !s.shop_humans.count(entry))
            return false;
    }
    // 抽取／54列表在初始化时冻结；后续p变化不使引用失效，也不重新抽签。
    // 空池沿初始化的{0,0}占位，但仍要求它们是完整的真实定义引用。
    for (const int human : display->second)
        if (!human_reference(s, human))
            return false;
    const auto answer = s.activity_page_answers.find(id);
    return answer == s.activity_page_answers.end() ||
           (raw == 51 && (answer->second == 0 || answer->second == 1));
}
bool initialize(State &s, std::uint64_t id, int raw) {
    if (s.activity_pages_initialized.count(id))
        return payload(s, id, raw);
    if (raw == 51) {
        if (!rebuild(s, id))
            return false;
        if (s.activity_page_lists.at(id).empty()) {
            if (!event(s, 49) || !close(s, id))
                return false;
        } else {
            if (!ref::world_script_seen(s.scripts, 100) && !event(s, 100))
                return false;
            const auto pool = humans(s);
            if (!pool)
                return false;
            s.activity_page_display_humans[id] = {0, 0};
            if (!pool->empty() && !draw_pair(s, *pool, id))
                return false;
        }
    } else {
        const auto binding = s.activity_page_bindings.find(id);
        if (binding == s.activity_page_bindings.end() || !definition(s, binding->second))
            return false;
        if (raw == 54) {
            const auto pool = humans(s);
            if (!pool)
                return false;
            s.activity_page_lists[id] = *pool;
        }
    }
    s.activity_page_selections.try_emplace(id, 0);
    s.activity_page_scroll.try_emplace(id, 0);
    s.page_counters.try_emplace(id, 0);
    s.activity_pages_initialized.insert(id);
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [&](const auto &p) { return p.id == id; });
    return page != s.scripts.pages.end() && (page->lifecycle == 4 || payload(s, id, raw));
}
std::optional<std::uint64_t> parent(const State &s, std::uint64_t child) {
    const auto binding = s.activity_page_parents.find(child);
    const auto current = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                      [&](const auto &p) { return p.id == child; });
    if (binding == s.activity_page_parents.end() || current == s.scripts.pages.end() ||
        s.activity_page_answers.count(binding->second))
        return {};
    const auto p = std::find_if(s.scripts.pages.begin(), current,
                                [&](const auto &p) { return p.id == binding->second; });
    if (p == current || p->lifecycle == 4 || p->kind != ref::WorldScriptPageKind::raw_page ||
        p->legacy_page != 51 || !payload(s, p->id, 51))
        return {};
    const auto activity = s.activity_page_bindings.find(child);
    const auto &list = s.activity_page_lists.at(p->id);
    const auto selected = s.activity_page_selections.find(p->id);
    if (activity == s.activity_page_bindings.end() ||
        selected == s.activity_page_selections.end() || selected->second < 0 ||
        selected->second >= static_cast<int>(list.size()) ||
        list[selected->second] != activity->second)
        return {};
    return p->id;
}
bool finish(State &s, std::uint64_t page, const ref::WorldVillageActivityDefinition &a) {
    if (a.kind < 0 || a.kind > 3 || s.quarter_counter == std::numeric_limits<int>::min())
        return false;
    --s.quarter_counter;
    if (a.kind == 0 || a.kind == 1) {
        const auto pool = humans(s);
        if (!pool || pool->empty())
            return false;
        for (const int human : *pool) {
            auto &ai = s.scene.world.world.ai;
            const auto growth = ai.growth.find(human);
            const auto shop = s.shop_humans.find(human);
            if (growth == ai.growth.end() || shop == s.shop_humans.end() ||
                !s.human_activity_previous.count(human))
                return false;
            const auto r = ref::prepare_world_village_human_effect(
                a, growth->second.definition, ai.professions, growth->second.derived,
                shop->second.satisfaction);
            if (!r.candidate)
                return false;
            growth->second.definition = r.candidate->definition;
            shop->second.satisfaction = r.candidate->satisfaction;
            s.human_activity_previous[human] = r.candidate->previous_value;
            if (r.candidate->stats) {
                growth->second.derived = *r.candidate->stats;
                if (!synchronize_startup_world_human_capacity(s, human))
                    return false;
            }
        }
        if (a.kind == 0)
            s.scene.world.popularity_queue.insert(s.scene.world.popularity_queue.begin(),
                                                  {10, a.magnitude, 1});
        const auto result = open(s, 54, a.identity);
        if (!result || !draw_pair(s, *pool, *result))
            return false;
    } else if (a.kind == 2) {
        s.scene.world.popularity_queue.insert(s.scene.world.popularity_queue.begin(),
                                              {10, a.magnitude, 1});
    } else if (!expand_startup_world_map(s)) {
        return false;
    }
    return close(s, page);
}
} // namespace

Error open_startup_world_village_activities(State &s) {
    const auto p = top(s);
    if (!s.rules || !p || p->kind != ref::WorldScriptPageKind::scene || s.scene.scene_state != 0 ||
        s.scene.framework_paused)
        return Error::invalid_page;
    auto next = s;
    next.scripts.executing_page = p->id;
    if (!open(next, 51))
        return Error::script_failed;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
bool initialize_startup_world_village_activity_pages(State &s) {
    if (s.scene.framework_paused)
        return true;
    std::vector<std::pair<std::uint64_t, int>> pending;
    for (const auto &p : s.scripts.pages)
        if (p.lifecycle != 4 && p.kind == ref::WorldScriptPageKind::raw_page &&
            supported(p.legacy_page))
            pending.emplace_back(p.id, p.legacy_page);
    const auto executing = s.scripts.executing_page;
    for (const auto &[id, raw] : pending) {
        s.scripts.executing_page = id;
        if (!initialize(s, id, raw))
            return false;
    }
    s.scripts.executing_page = executing;
    return true;
}
std::optional<StartupVillageActivityView>
inspect_startup_world_village_activity_page(const State &s, std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page ||
        !payload(s, id, p->legacy_page))
        return {};
    StartupVillageActivityView r;
    r.raw = p->legacy_page;
    r.selection = s.activity_page_selections.at(id);
    r.first_visible = s.activity_page_scroll.at(id);
    r.counter = s.page_counters.at(id);
    if (s.activity_page_bindings.count(id))
        r.activity = s.activity_page_bindings.at(id);
    if (s.activity_page_lists.count(id))
        r.entries = s.activity_page_lists.at(id);
    if (s.activity_page_display_humans.count(id))
        r.display_humans = s.activity_page_display_humans.at(id);
    return r;
}
Error act_startup_world_village_activity_page(State &s, std::uint64_t id, Action action,
                                              int selection) {
    const auto p = top(s);
    if (!s.rules || s.scene.framework_paused || !p || p->id != id ||
        p->kind != ref::WorldScriptPageKind::raw_page || !supported(p->legacy_page))
        return Error::invalid_page;
    const int raw = p->legacy_page;
    auto next = s;
    next.scripts.executing_page = id;
    // 输入只消费框架已初始化的页，不能越过首次说明页隐式开展活动。
    if (!payload(next, id, raw))
        return Error::missing_source;
    if (raw == 51 && next.activity_page_answers.count(id))
        return Error::invalid_page; // 等父页实际恢复轮，不提前重建或再开展。
    auto &chosen = next.activity_page_selections.at(id);
    if (action == Action::previous || action == Action::next || action == Action::select) {
        if (raw == 53)
            return Error::invalid_page;
        const int size = raw == 52 ? 2 : static_cast<int>(next.activity_page_lists.at(id).size());
        if (size == 0)
            return Error::invalid_page;
        const int target = action == Action::select
                               ? selection
                               : (chosen + (action == Action::previous ? size - 1 : 1)) % size;
        if (target < 0 || target >= size)
            return Error::invalid_page;
        chosen = target;
        auto &first = next.activity_page_scroll.at(id);
        if (first > chosen)
            first = chosen;
        if (first + 5 <= chosen)
            first = chosen - 4;
    } else if (raw == 51) {
        if (action == Action::cancel) {
            if (!close(next, id))
                return Error::script_failed;
        } else if (action == Action::confirm) {
            const auto &list = next.activity_page_lists.at(id);
            if (chosen < 0 || chosen >= static_cast<int>(list.size()))
                return Error::invalid_page;
            const auto a = definition(next, list[chosen]);
            if (!a)
                return Error::missing_source;
            const auto gate =
                ref::check_world_village_activity(*a, next.quarter_counter, next.village_points);
            if (gate.error != ref::WorldVillageActivityError::none)
                return Error::missing_source;
            if (gate.denial != ref::WorldVillageActivityDenial::none) {
                if (!event(next, gate.denial == ref::WorldVillageActivityDenial::no_quarter_slots50
                                     ? 50
                                     : 12))
                    return Error::script_failed;
            } else {
                if (a->kind > 3)
                    return Error::missing_source;
                const auto child = open(next, 52, a->identity);
                if (!child)
                    return Error::script_failed;
                next.activity_page_parents[*child] = id;
                next.scripts.activities.at(a->identity).pending_notice = false;
            }
        } else
            return Error::invalid_page;
    } else if (raw == 52) {
        if (action != Action::confirm && action != Action::cancel)
            return Error::invalid_page;
        const auto owner = parent(next, id);
        const auto a = definition(next, next.activity_page_bindings.at(id));
        if (!owner || !a)
            return Error::missing_source;
        const bool held = action == Action::confirm && chosen == 0;
        if (held) {
            const auto start = ref::prepare_world_village_activity_start(*a, next.village_points,
                                                                         next.events_held);
            if (!start.candidate)
                return Error::missing_source;
            next.village_points = start.candidate->village_points;
            if (!open(next, 53, a->identity))
                return Error::script_failed;
            next.activity_counts.at(a->identity) = start.candidate->activity.held;
            next.activity_flags.at(a->identity) = start.candidate->activity.flags;
            next.events_held = start.candidate->events_held;
        }
        next.activity_page_answers[*owner] = held ? 0 : 1;
        if (!close(next, id))
            return Error::script_failed;
    } else if (raw == 53) {
        if (action != Action::confirm)
            return Error::invalid_page;
        const auto plan = ref::world_village_activity_animation(next.page_counters.at(id), true);
        const auto a = definition(next, next.activity_page_bindings.at(id));
        if (!plan || !a)
            return Error::missing_source;
        if (plan->complete && !finish(next, id, *a))
            return Error::script_failed;
    } else {
        if (action != Action::confirm && action != Action::cancel)
            return Error::invalid_page;
        if (!close(next, id))
            return Error::script_failed;
    }
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
std::optional<State> update_startup_world_village_activity_page(const State &s, std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page ||
        !supported(p->legacy_page))
        return {};
    if (s.scene.framework_paused)
        return s;
    auto next = s;
    next.scripts.executing_page = id;
    if (!initialize(next, id, p->legacy_page))
        return {};
    if (p->legacy_page == 51 && next.activity_page_answers.count(id)) {
        const int answer = next.activity_page_answers.at(id);
        if ((answer != 0 && answer != 1) || (answer == 0 && !rebuild(next, id)))
            return {};
        next.activity_page_answers.erase(id);
        if (next.activity_page_lists.at(id).empty() && !close(next, id))
            return {};
    }
    auto &counter = next.page_counters.at(id);
    if (counter < 0 || counter == std::numeric_limits<int>::max())
        return {};
    ++counter;
    if (p->legacy_page == 53 && counter == 70)
        next.sound_requests.push_back(5);
    next.scripts.executing_page.reset();
    return next;
}
} // namespace dungeon_village_prototype
