// 设施道具由同一Owner分两次原调用点提交：75扣库存/实例计数，76首次改良定义。
#include "ark/simulation/facilities/startup_world_facility_items.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"
#include "ark/simulation/facilities/rules/facility_items.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Action = StartupFacilityItemAction;
const ref::WorldScriptPage *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
const StartupDefinition *definition(const State &s, int id) {
    if (!s.rules)
        return nullptr;
    const auto d = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                [id](const auto &v) { return v.id == id; });
    return d == s.rules->facilities.end() ? nullptr : &*d;
}
const StartupWorldItem *item(const State &s, int id) {
    if (!s.rules)
        return nullptr;
    const auto d = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                [id](const auto &v) { return v.identity == id; });
    return d == s.rules->items.end() ? nullptr : &*d;
}
bool close(State &s, std::uint64_t id) {
    const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
bool event(State &s, int id) {
    const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                                             startup_world_runtime_scripts(s), {id, {}, {}});
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
bool facility_program(State &s, int definition_id) {
    const auto r = ref::prepare_world_script_program(startup_world_runtime_catalog(),
                                                     startup_world_runtime_scripts(s),
                                                     {2000 + definition_id, {}, {}});
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
std::optional<std::uint64_t> open(State &s, int raw, std::uint64_t facility, int definition_id,
                                  std::optional<int> selected = {}) {
    ref::WorldScriptPage p;
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = raw;
    p.legacy_f = definition_id;
    p.title = raw == 75 ? "使用道具" : "设施强化";
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(s), p);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, r.candidate->state))
        return {};
    const auto id = r.candidate->inserted_pages.front().id;
    s.facility_page_bindings[id] = facility;
    s.page_counters[id] = s.page_phases[id] = 0;
    if (selected)
        s.facility_item_page_items[id] = *selected;
    return id;
}
bool eligible(const StartupDefinition &d) {
    return d.kind != 12 && d.kind != 2 && d.detail != 1 && d.detail != 4 && d.detail != 5 &&
           d.detail != 6;
}
std::optional<ref::FacilityEconomyInput> economy_input(const State &s, int definition_id,
                                                       std::uint64_t facility) {
    const auto use = s.scene.world.world.facility_uses.find(definition_id);
    const auto shared = s.scripts.facilities.find(definition_id);
    const auto n = s.neighbourhood.find(facility);
    if (!s.rules || use == s.scene.world.world.facility_uses.end() ||
        shared == s.scripts.facilities.end() || n == s.neighbourhood.end())
        return {};
    ref::FacilityEconomyInput in;
    in.level = use->second.level;
    in.completed_definition_uses = use->second.completed_uses;
    in.definition_improvements = shared->second.improvements;
    std::copy(n->second.begin(), n->second.end(), in.instance_modifiers.begin());
    for (const auto &h : s.rules->humans) {
        const auto p = s.human_presence.find(h.identity);
        const auto g = s.scene.world.world.ai.growth.find(h.identity);
        if (p == s.human_presence.end() || g == s.scene.world.world.ai.growth.end())
            return {};
        if (!p->second)
            continue;
        const int job = g->second.definition.current_profession;
        if (job < 0 || static_cast<std::size_t>(job) >= s.rules->jobs.size())
            return {};
        const int type = s.rules->jobs[job].type;
        if (type < 0 || static_cast<std::size_t>(type) >= in.legacy_job_counts.size() ||
            in.legacy_job_counts[type] == std::numeric_limits<int>::max())
            return {};
        ++in.legacy_job_counts[type];
    }
    return in;
}
bool initialized_payload(const State &s, const ref::WorldScriptPage &p) {
    const auto phase = s.page_phases.find(p.id), count = s.page_counters.find(p.id);
    if (phase == s.page_phases.end() || phase->second != 0 || count == s.page_counters.end() ||
        count->second < 0)
        return false;
    if (p.legacy_page != 75)
        return s.facility_item_page_items.count(p.id) &&
               item(s, s.facility_item_page_items.at(p.id));
    const auto list = s.facility_item_page_lists.find(p.id);
    const auto selected = s.facility_item_page_selections.find(p.id);
    if (list == s.facility_item_page_lists.end() || list->second.empty() ||
        selected == s.facility_item_page_selections.end() || selected->second < 0 ||
        static_cast<std::size_t>(selected->second) >= list->second.size())
        return false;
    std::set<int> seen;
    for (const int id : list->second) {
        const auto owned = s.items.find(id);
        if (!item(s, id) || !seen.insert(id).second || owned == s.items.end() ||
            owned->second.inventory <= 0 || owned->second.inventory > 999)
            return false;
    }
    return true;
}
bool improve(State &s, const ref::WorldScriptPage &p) {
    const auto *d = definition(s, p.legacy_f);
    const auto selected = s.facility_item_page_items.find(p.id);
    const auto binding = s.facility_page_bindings.find(p.id);
    if (!d || selected == s.facility_item_page_items.end() ||
        binding == s.facility_page_bindings.end() || d->id < 0 ||
        static_cast<std::size_t>(d->id) >= s.rules->facility_initial.size())
        return false;
    const auto *i = item(s, selected->second);
    const auto in = economy_input(s, d->id, binding->second);
    if (!i || !in)
        return false;
    const auto r = ref::prepare_facility_improvement(
        {d->id, d->legacy_icon, s.rules->facility_initial[d->id].item_affinities, d->economy},
        {i->identity, i->category, i->facility_improvements}, *in);
    if (!r.candidate)
        return false;
    const auto &c = *r.candidate;
    auto &shared = s.scripts.facilities.at(d->id);
    shared.improvements = c.definition_improvements;
    for (std::size_t n = 0; n < shared.attributes.size(); ++n) {
        const auto value = c.after.definition_attributes[n];
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
            return false;
        shared.attributes[n] = static_cast<int>(value);
    }
    // 定义共享J立即影响所有同类设施，但实例邻接各自推导，绝不复制选中实例价格。
    for (auto &f : s.scene.world.world.facilities) {
        if (f.second.placement.definition_id != d->id)
            continue;
        const auto current = economy_input(s, d->id, f.first);
        const auto v = current ? ref::derive_facility_economy(d->economy, *current)
                               : ref::FacilityEconomyResult{};
        if (!v.values || v.values->instance_attributes[0] < std::numeric_limits<int>::min() ||
            v.values->instance_attributes[0] > std::numeric_limits<int>::max())
            return false;
        f.second.price = static_cast<int>(v.values->instance_attributes[0]);
    }
    s.scripts.job_counts = in->legacy_job_counts;
    s.facility_item_response = c.legacy_response;
    for (std::size_t n = 0; n < 3; ++n) {
        s.facility_upgrade_display[0][n] = c.before.instance_attributes[n];
        s.facility_upgrade_display[1][n] = c.after.instance_attributes[n];
        s.facility_upgrade_display[2][n] = c.visible_deltas[n];
    }
    return true;
}
bool initialize(State &s, const ref::WorldScriptPage &p) {
    if (!valid_startup_world_facility_item_page(s, p))
        return false;
    if (s.facility_item_pages_initialized.count(p.id))
        return initialized_payload(s, p);
    if (p.legacy_page == 75) {
        std::vector<int> list;
        for (const auto &d : s.rules->items) {
            const auto owned = s.items.find(d.identity);
            if (owned == s.items.end() || owned->second.inventory < 0 ||
                owned->second.inventory > 999)
                return false;
            if (owned->second.inventory > 0)
                list.push_back(d.identity);
        }
        s.facility_item_page_lists[p.id] = std::move(list);
        s.facility_item_page_selections[p.id] = 0;
        if (s.facility_item_page_lists.at(p.id).empty())
            return event(s, 15) && close(s, p.id);
    } else if (p.legacy_page == 76 && !improve(s, p))
        return false;
    s.facility_item_pages_initialized.insert(p.id);
    return initialized_payload(s, p);
}
} // namespace
bool valid_startup_world_facility_item_page(const State &s, const ref::WorldScriptPage &p) {
    if (!s.rules || p.kind != ref::WorldScriptPageKind::raw_page || p.legacy_page < 75 ||
        p.legacy_page > 77)
        return false;
    const auto binding = s.facility_page_bindings.find(p.id);
    if (binding == s.facility_page_bindings.end())
        return false;
    const auto f = s.scene.world.world.facilities.find(binding->second);
    const auto *d = definition(s, p.legacy_f);
    if (!d || !eligible(*d) || f == s.scene.world.world.facilities.end() ||
        f->second.placement.definition_id != d->id || f->second.status == 0 ||
        !s.facility_item_confirmations.count(binding->second) ||
        !s.facility_month_age.count(binding->second) || !s.page_counters.count(p.id) ||
        !s.page_phases.count(p.id))
        return false;
    if (p.legacy_page != 75) {
        const auto i = s.facility_item_page_items.find(p.id);
        if (i == s.facility_item_page_items.end() || !item(s, i->second))
            return false;
    } else {
        const ref::WorldScriptPage *parent{};
        for (const auto &q : s.scripts.pages) {
            if (q.id == p.id)
                break;
            if (q.lifecycle != 4)
                parent = &q;
        }
        if (!parent || parent->legacy_page != 74 ||
            !valid_startup_world_facility_page(s, *parent) ||
            !s.facility_page_bindings.count(parent->id) ||
            s.facility_page_bindings.at(parent->id) != binding->second)
            return false;
    }
    return !s.facility_item_pages_initialized.count(p.id) || initialized_payload(s, p);
}
Error open_startup_world_facility_items(State &s, std::uint64_t page) {
    const auto *p = top(s);
    if (s.scene.framework_paused || !p || p->id != page ||
        !valid_startup_world_facility_page(s, *p) || !s.facility_page_bindings.count(page))
        return Error::invalid_page;
    const auto *d = definition(s, p->legacy_f);
    const auto phase = s.page_phases.find(page);
    if (!d || !eligible(*d) || phase == s.page_phases.end() || phase->second != 0)
        return Error::invalid_page;
    auto next = s;
    next.scripts.executing_page = page;
    if (!open(next, 75, s.facility_page_bindings.at(page), d->id))
        return Error::script_failed;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
bool initialize_startup_world_facility_item_pages(State &s) {
    if (s.scene.framework_paused)
        return true;
    std::vector<ref::WorldScriptPage> pending;
    for (const auto &p : s.scripts.pages)
        if (p.lifecycle != 4 && p.kind == ref::WorldScriptPageKind::raw_page &&
            p.legacy_page >= 75 && p.legacy_page <= 77)
            pending.push_back(p);
    const auto executing = s.scripts.executing_page;
    for (const auto &p : pending) {
        s.scripts.executing_page = p.id;
        if (!initialize(s, p))
            return false;
    }
    s.scripts.executing_page = executing;
    return true;
}
Error act_startup_world_facility_item_page(State &s, std::uint64_t id, Action action,
                                           int selection) {
    if (action != Action::confirm && action != Action::cancel && action != Action::previous &&
        action != Action::next && action != Action::select)
        return Error::invalid_page;
    const auto *p = top(s);
    if (s.scene.framework_paused || !p || p->id != id ||
        !valid_startup_world_facility_item_page(s, *p))
        return Error::invalid_page;
    auto next = s;
    const auto page = *p;
    next.scripts.executing_page = id;
    if (!next.facility_item_pages_initialized.count(id) || !initialized_payload(next, page))
        return Error::missing_source;
    if (page.legacy_page == 75) {
        auto &list = next.facility_item_page_lists.at(id);
        auto &selected = next.facility_item_page_selections.at(id);
        if (action == Action::cancel) {
            if (!close(next, id))
                return Error::script_failed;
        } else if (action == Action::select) {
            if (selection < 0 || static_cast<std::size_t>(selection) >= list.size())
                return Error::invalid_page;
            selected = selection;
        } else if (action == Action::previous || action == Action::next) {
            const int count = static_cast<int>(list.size());
            selected = (selected + (action == Action::next ? 1 : count - 1)) % count;
        } else {
            if (selection == -1)
                selection = selected;
            if (selection < 0 || static_cast<std::size_t>(selection) >= list.size())
                return Error::invalid_page;
            const int chosen = list[selection];
            const auto facility = next.facility_page_bindings.at(id);
            const auto *d = definition(next, page.legacy_f);
            const auto counters =
                ref::confirm_facility_item({next.facility_item_confirmations.at(facility),
                                            next.facility_month_age.at(facility)},
                                           d->legacy_icon);
            if (!counters.transition ||
                next.facility_item_confirmations.at(facility) == std::numeric_limits<int>::max() ||
                counters.transition->counters.item_confirmations > std::numeric_limits<int>::max())
                return Error::missing_source;
            --next.items.at(chosen).inventory;
            const auto shared = next.catalog.find({0, chosen});
            if (shared == next.catalog.end() ||
                shared->second.inventory != next.items.at(chosen).inventory + 1)
                return Error::missing_source;
            shared->second.inventory = next.items.at(chosen).inventory;
            selected = selection;
            if (next.items.at(chosen).inventory == 0) {
                list.erase(list.begin() + selection);
                if (static_cast<std::size_t>(selected) >= list.size())
                    selected = 0;
            }
            if (!open(next, 76, facility, d->id, chosen))
                return Error::script_failed;
            ++next.facility_item_confirmations.at(facility);
            if (counters.transition->script_triggered && !facility_program(next, d->id))
                return Error::script_failed;
            next.facility_item_confirmations.at(facility) =
                static_cast<int>(counters.transition->counters.item_confirmations);
            next.facility_month_age.at(facility) =
                static_cast<int>(counters.transition->counters.months_since_trigger);
            if (list.empty() && !close(next, id))
                return Error::script_failed;
        }
    } else if (page.legacy_page == 77 && action == Action::confirm) {
        auto &counter = next.page_counters.at(id);
        if (counter < 49)
            counter = 49;
        else if (counter >= 55 && !close(next, id))
            return Error::script_failed;
    } else
        return Error::invalid_page;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
std::optional<State> prepare_startup_world_facility_item_page(const State &s) {
    const auto *p = top(s);
    if (s.scene.framework_paused || !p || !valid_startup_world_facility_item_page(s, *p))
        return {};
    auto next = s;
    const auto page = *p;
    next.scripts.executing_page = page.id;
    if (!initialize(next, page))
        return {};
    const auto *current = top(next);
    if (!current || current->id != page.id) {
        next.scripts.executing_page.reset();
        return next;
    }
    auto &counter = next.page_counters.at(page.id);
    if (counter == std::numeric_limits<int>::max())
        return {};
    ++counter;
    if (page.legacy_page == 76 && counter >= 49) {
        if (!open(next, 77, next.facility_page_bindings.at(page.id), page.legacy_f,
                  next.facility_item_page_items.at(page.id)) ||
            !close(next, page.id))
            return {};
    }
    next.scripts.executing_page.reset();
    return next;
}
} // namespace ark::simulation
