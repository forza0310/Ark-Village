// 原79确认只清整类NEW，72只读；82结束先排I，真正人气由共同世界后续消费。
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Page = ref::WorldScriptPage;
using Error = StartupWorldRuntimeError;
using Action = StartupFacilityCatalogAction;
bool supported(int raw) { return raw == 79 || raw == 72 || raw == 82; }
const Page *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                              [](const auto &v) { return v.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
const Page *page(const State &s, std::uint64_t id) {
    const auto p = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                              [id](const auto &v) { return v.id == id && v.lifecycle != 4; });
    return p == s.scripts.pages.end() ? nullptr : &*p;
}
const Page *previous_live(const State &s, std::uint64_t id) {
    auto p = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                         [id](const auto &v) { return v.id == id; });
    if (p == s.scripts.pages.end())
        return nullptr;
    while (p != s.scripts.pages.begin()) {
        --p;
        if (p->lifecycle != 4)
            return &*p;
    }
    return nullptr;
}
int kind(int raw, int mode) {
    if (raw == 79)
        return mode == 1 ? 1 : mode == 4 ? 2 : mode == 5 ? 3 : -1;
    if (raw == 72)
        return mode == 0 ? 1 : mode == 1 || mode == 2 ? 2 : mode == 3 ? 3 : -1;
    return -1;
}
int information_mode(int mode) { return mode == 1 ? 0 : mode == 4 ? 1 : mode == 5 ? 3 : -1; }
const StartupWorldEquipment *equipment(const State &s, int k, int id) {
    if (!s.rules)
        return nullptr;
    const auto d = std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                              [=](const auto &v) { return v.shop.kind == k && v.shop.id == id; });
    return d == s.rules->equipment.end() ? nullptr : &*d;
}
const StartupDefinition *facility(const State &s, int id) {
    if (!s.rules)
        return nullptr;
    const auto d = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                              [id](const auto &v) { return v.id == id; });
    return d == s.rules->facilities.end() ? nullptr : &*d;
}
std::optional<std::vector<int>> catalogue(const State &s, int k) {
    if (!s.rules || k < 1 || k > 3)
        return {};
    std::vector<int> result;
    std::set<int> seen;
    for (const auto &d : s.rules->equipment) {
        if (d.shop.kind != k)
            continue;
        const auto c = s.catalog.find({k, d.shop.id});
        if (d.shop.id < 0 || !seen.insert(d.shop.id).second || c == s.catalog.end() ||
            c->second.status < 0 || c->second.status > 2)
            return {};
        if (c->second.status != 0)
            result.push_back(d.shop.id);
    }
    if (result.empty() || result.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {};
    // 原交换遍历保留等值可能被第三条更小项跨越的行为，不能换成stable_sort。
    for (std::size_t i = 0; i + 1 < result.size(); ++i)
        for (std::size_t j = result.size() - 1; j > i; --j)
            if (equipment(s, k, result[j])->gift_order < equipment(s, k, result[i])->gift_order)
                std::swap(result[i], result[j]);
    return result;
}
bool human_definition(const State &s, int id, bool require_present) {
    if (!s.rules)
        return false;
    const auto d = std::find_if(s.rules->humans.begin(), s.rules->humans.end(),
                              [id](const auto &v) { return v.identity == id; });
    const auto presence = s.human_presence.find(id);
    const auto growth = s.scene.world.world.ai.growth.find(id);
    if (d == s.rules->humans.end() || presence == s.human_presence.end() ||
        growth == s.scene.world.world.ai.growth.end() || presence->second < 0 ||
        presence->second > 2 || (require_present && presence->second == 0))
        return false;
    const int profession = growth->second.definition.current_profession;
    return profession >= 0 && static_cast<std::size_t>(profession) < s.rules->jobs.size();
}
std::optional<std::vector<int>> participants(const State &s, int mode) {
    if (!s.rules || mode < 0 || mode > 1)
        return {};
    std::vector<int> result;
    std::set<int> seen;
    for (const auto &h : s.rules->humans) {
        if (!seen.insert(h.identity).second || !human_definition(s, h.identity, false))
            return {};
        if (s.human_presence.find(h.identity)->second != 0) {
            const int profession = s.scene.world.world.ai.growth.find(h.identity)->second.definition.current_profession;
            if (s.rules->jobs[static_cast<std::size_t>(profession)].type == (mode == 0 ? 1 : 0))
                result.push_back(h.identity);
            if (result.size() == 3)
                break;
        }
    }
    if (result.empty()) {
        if (!human_definition(s, 0, false))
            return {};
        result.push_back(0);
    }
    return result;
}
bool payload(const State &s, const Page &p) {
    const auto data = s.facility_catalog_page_data.find(p.id);
    const auto list = s.facility_catalog_page_lists.find(p.id);
    const auto phase = s.page_phases.find(p.id);
    const auto counter = s.page_counters.find(p.id);
    if (!s.rules || !supported(p.legacy_page) || !s.facility_catalog_pages_initialized.count(p.id) ||
        data == s.facility_catalog_page_data.end() || list == s.facility_catalog_page_lists.end() ||
        phase == s.page_phases.end() || counter == s.page_counters.end() || counter->second < 0 ||
        list->second.empty() || list->second.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    const auto &v = data->second;
    if (v[0] != p.legacy_f || v[1] < 0 || v[2] < 0 || phase->second < 0)
        return false;
    if (p.legacy_page == 82) {
        if (!p.facility_definition || *p.facility_definition != v[3] || v[0] < 0 || v[0] > 1 ||
            v[1] != 0 || v[2] != 0 || phase->second > 1 || list->second.size() > 3 ||
            s.facility_catalog_page_parents.count(p.id))
            return false;
        const auto d = facility(s, v[3]);
        if (!d || d->legacy_icon != (v[0] == 0 ? 2 : 3))
            return false;
        const auto expected = participants(s, v[0]);
        return expected && *expected == list->second;
    }
    const int k = kind(p.legacy_page, v[0]);
    const auto expected = catalogue(s, k);
    if (!expected || *expected != list->second || p.facility_definition)
        return false;
    const int count = static_cast<int>(list->second.size());
    if (p.legacy_page == 79)
        return v[3] == -1 && phase->second <= 1 && v[1] < count && v[2] <= v[1] &&
               v[1] - v[2] < 4 && v[2] < count &&
               !s.facility_catalog_page_parents.count(p.id);
    const auto parent = s.facility_catalog_page_parents.find(p.id);
    if (parent == s.facility_catalog_page_parents.end() || parent->second == p.id ||
        v[1] != 0 || v[2] != 0 || v[3] != p.legacy_g || phase->second >= count)
        return false;
    const auto father = page(s, parent->second);
    if (!father || previous_live(s, p.id) != father ||
        father->kind != ref::WorldScriptPageKind::raw_page || father->legacy_page != 79 ||
        information_mode(father->legacy_f) != v[0] ||
        !payload(s, *father))
        return false;
    const auto &fv = s.facility_catalog_page_data.find(father->id)->second;
    const auto &fl = s.facility_catalog_page_lists.find(father->id)->second;
    return fl[static_cast<std::size_t>(fv[1])] == v[3];
}
bool initialize(State &s, const Page &p) {
    if (s.facility_catalog_pages_initialized.count(p.id))
        return payload(s, p);
    // 未初始化也拒绝残留半载荷，计数/phase可由共同框架先建立。
    if (!s.rules || !supported(p.legacy_page) || s.facility_catalog_page_data.count(p.id) ||
        s.facility_catalog_page_lists.count(p.id))
        return false;
    std::vector<int> list;
    int phase = 0;
    int binding = -1;
    if (p.legacy_page == 82) {
        if (!p.facility_definition || p.legacy_f < 0 || p.legacy_f > 1)
            return false;
        binding = *p.facility_definition;
        const auto d = facility(s, binding);
        if (!d || d->legacy_icon != (p.legacy_f == 0 ? 2 : 3))
            return false;
        const auto candidates = participants(s, p.legacy_f);
        if (!candidates)
            return false;
        list = *candidates;
    } else {
        const auto entries = catalogue(s, kind(p.legacy_page, p.legacy_f));
        if (!entries)
            return false;
        list = *entries;
        if (p.legacy_page == 72) {
            binding = p.legacy_g;
            const auto selected = std::find(list.begin(), list.end(), binding);
            if (selected == list.end())
                return false; // 合法79信息载荷必定在同类目录中，拒绝伪造缺项。
            phase = static_cast<int>(selected - list.begin());
        }
    }
    const auto counter = s.page_counters.find(p.id);
    const auto prior_phase = s.page_phases.find(p.id);
    if ((counter != s.page_counters.end() && counter->second != 0) ||
        (prior_phase != s.page_phases.end() && prior_phase->second != 0))
        return false;
    s.facility_catalog_page_data.emplace(p.id, std::array<int, 4>{p.legacy_f, 0, 0, binding});
    s.facility_catalog_page_lists.emplace(p.id, std::move(list));
    s.page_phases[p.id] = phase;
    s.page_counters.try_emplace(p.id, 0);
    s.facility_catalog_pages_initialized.insert(p.id);
    return payload(s, p);
}
bool close(State &s, std::uint64_t id) {
    const auto scripts = startup_world_runtime_scripts(s);
    // 共用close只处理生命周期，不校验整个页栈；Owner在候选清提示/排I之后仍须拒绝坏身份。
    if (ref::validate_world_script_state(startup_world_runtime_catalog(), scripts) != ref::WorldScriptError::none)
        return false;
    const auto r = ref::prepare_world_script_close_page(scripts, id);
    if (!r.candidate)
        return false;
    const auto p = std::find_if(r.candidate->state.pages.begin(), r.candidate->state.pages.end(),
                              [id](const auto &v) { return v.id == id; });
    return p != r.candidate->state.pages.end() && p->lifecycle == 4 &&
           write_startup_world_runtime_scripts(s, r.candidate->state);
}
void scroll(std::array<int, 4> &v) {
    // 原窗口允许末页不足四行，不把滚动起点夹到count-4。
    if (v[2] > v[1]) v[2] = v[1];
    if (v[1] - v[2] >= 4) v[2] = v[1] - 3;
}
} // namespace

bool valid_startup_world_facility_catalog_page(const State &s, const Page &p) {
    if (!s.rules || p.kind != ref::WorldScriptPageKind::raw_page || !supported(p.legacy_page) ||
        p.lifecycle < 0 || p.lifecycle == 4)
        return false;
    if (s.facility_catalog_pages_initialized.count(p.id))
        return payload(s, p);
    if (p.lifecycle != 0 || s.facility_catalog_page_data.count(p.id) ||
        s.facility_catalog_page_lists.count(p.id))
        return false;
    const auto counter = s.page_counters.find(p.id);
    const auto phase = s.page_phases.find(p.id);
    if ((counter != s.page_counters.end() && counter->second != 0) ||
        (phase != s.page_phases.end() && phase->second != 0))
        return false;
    if (p.legacy_page == 82) {
        if (!p.facility_definition || p.legacy_f < 0 || p.legacy_f > 1 ||
            s.facility_catalog_page_parents.count(p.id))
            return false;
        const auto d = facility(s, *p.facility_definition);
        return d && d->legacy_icon == (p.legacy_f == 0 ? 2 : 3) && participants(s, p.legacy_f).has_value();
    }
    if (p.facility_definition)
        return false;
    const auto list = catalogue(s, kind(p.legacy_page, p.legacy_f));
    if (!list)
        return false;
    if (p.legacy_page == 79)
        return !s.facility_catalog_page_parents.count(p.id);
    const auto parent = s.facility_catalog_page_parents.find(p.id);
    if (parent == s.facility_catalog_page_parents.end() || parent->second == p.id)
        return false;
    const auto father = page(s, parent->second);
    if (!father || previous_live(s, p.id) != father ||
        father->kind != ref::WorldScriptPageKind::raw_page || father->legacy_page != 79 ||
        information_mode(father->legacy_f) != p.legacy_f || !payload(s, *father))
        return false;
    const auto &fv = s.facility_catalog_page_data.find(father->id)->second;
    const auto &fl = s.facility_catalog_page_lists.find(father->id)->second;
    return fl[static_cast<std::size_t>(fv[1])] == p.legacy_g;
}
bool initialize_startup_world_facility_catalog_pages(State &s) {
    if (s.scene.framework_paused)
        return true;
    std::vector<Page> pending;
    for (const auto &p : s.scripts.pages)
        if (p.lifecycle != 4 && p.kind == ref::WorldScriptPageKind::raw_page && supported(p.legacy_page))
            pending.push_back(p);
    if (pending.empty())
        return true;
    auto next = s;
    for (const auto &p : pending)
        if (!valid_startup_world_facility_catalog_page(next, p) || !initialize(next, p))
            return false;
    s = std::move(next);
    return true;
}
std::optional<StartupFacilityCatalogView> inspect_startup_world_facility_catalog_page(const State &s, std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || !s.facility_catalog_pages_initialized.count(id) ||
        !valid_startup_world_facility_catalog_page(s, *p))
        return {};
    const auto &v = s.facility_catalog_page_data.find(id)->second;
    const auto &entries = s.facility_catalog_page_lists.find(id)->second;
    const int phase = s.page_phases.find(id)->second;
    const int binding = p->legacy_page == 72 ? entries[static_cast<std::size_t>(phase)] : v[3];
    return StartupFacilityCatalogView{p->legacy_page, v[0], phase, s.page_counters.find(id)->second,
                                     v[1], v[2], binding, entries};
}
Error act_startup_world_facility_catalog_page(State &s, std::uint64_t id, Action action, int selection) {
    const auto p = top(s);
    if (static_cast<int>(action) < static_cast<int>(Action::confirm) ||
        static_cast<int>(action) > static_cast<int>(Action::inspect) || s.scene.framework_paused ||
        !p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page || !supported(p->legacy_page))
        return Error::invalid_page;
    if (!s.facility_catalog_pages_initialized.count(id) || !valid_startup_world_facility_catalog_page(s, *p))
        return Error::missing_source;
    const Page original = *p;
    auto next = s;
    next.scripts.executing_page = id;
    auto &v = next.facility_catalog_page_data.find(id)->second;
    auto &phase = next.page_phases.find(id)->second;
    auto &counter = next.page_counters.find(id)->second;
    const auto &list = next.facility_catalog_page_lists.find(id)->second;
    const int count = static_cast<int>(list.size());
    if (action == Action::previous || action == Action::next || action == Action::select) {
        if (original.legacy_page != 79)
            return Error::invalid_page;
        const int chosen = action == Action::select ? selection
            : static_cast<int>((static_cast<std::int64_t>(v[1]) +
                                (action == Action::previous ? count - 1 : 1)) % count);
        if (chosen < 0 || chosen >= count)
            return Error::invalid_page;
        v[1] = chosen;
        scroll(v);
    } else if (action == Action::previous_tab || action == Action::next_tab) {
        if (original.legacy_page == 79)
            phase = 1 - phase;
        else if (original.legacy_page == 72)
            phase = static_cast<int>((static_cast<std::int64_t>(phase) +
                                      (action == Action::previous_tab ? count - 1 : 1)) % count);
        else
            return Error::invalid_page;
    } else if (action == Action::inspect) {
        if (original.legacy_page != 79)
            return Error::invalid_page;
        Page child;
        child.kind = ref::WorldScriptPageKind::raw_page;
        child.legacy_page = 72;
        child.legacy_f = information_mode(v[0]);
        child.legacy_g = list[static_cast<std::size_t>(v[1])];
        const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(next), child);
        if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
            !write_startup_world_runtime_scripts(next, r.candidate->state))
            return Error::script_failed;
        next.facility_catalog_page_parents.emplace(r.candidate->inserted_pages.front().id, id);
    } else if (action == Action::confirm || action == Action::cancel) {
        if (original.legacy_page == 82) {
            if (counter < 40)
                counter = 40;
            else if (phase == 0) {
                phase = 1;
                counter = 0;
            } else {
                next.scene.world.popularity_queue.insert(next.scene.world.popularity_queue.begin(), {10, 20, 1});
                if (!close(next, id))
                    return Error::script_failed;
            }
        } else {
            if (original.legacy_page == 79) {
                const int k = kind(79, v[0]);
                for (auto &entry : next.catalog)
                    if (entry.first.first == k)
                        entry.second.newly_unlocked = false;
            }
            if (!close(next, id))
                return Error::script_failed;
        }
    } else
        return Error::invalid_page;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
std::optional<State> update_startup_world_facility_catalog_page(const State &s, std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page || !supported(p->legacy_page))
        return {};
    if (s.scene.framework_paused)
        return s;
    auto next = s;
    const Page original = *p;
    if (!valid_startup_world_facility_catalog_page(next, original) || !initialize(next, original))
        return {};
    auto &counter = next.page_counters.find(id)->second;
    if (counter == std::numeric_limits<int>::max())
        return {};
    ++counter;
    if (original.legacy_page == 82 && next.page_phases.find(id)->second == 0 && counter == 1)
        next.sound_requests.push_back({StartupAudioOperation::jingle, 4});
    return next;
}
} // namespace dungeon_village_prototype
