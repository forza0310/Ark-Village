// Steam raw12：仅维护非日文四行的世界载荷与候选；跨局系统提交由应用协调。
#include "dungeon_village_prototype/startup_world_config.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Page = ref::WorldScriptPage;
using Error = StartupWorldRuntimeError;
const Page *find(const State &s, std::uint64_t id) {
    const Page *result{};
    for (const auto &p : s.scripts.pages) if (p.id == id) {
        if (!id || result) return nullptr;
        result = &p;
    }
    return result;
}
const Page *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                               [](const auto &v) { return v.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
bool metadata(const Page &p) {
    return p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == 12 &&
        p.source_record == 10 && p.legacy_tag == 21 && p.replacement.empty() &&
        p.speaker_kind == 0 && p.speaker_definition == -1 && p.legacy_r == 0 &&
        p.legacy_s == 0 && p.legacy_t == 0 && p.legacy_g == 0 && p.legacy_l == 0 &&
        p.message_commands.empty() && !p.task_identity && !p.task_definition &&
        !p.monster_definition && !p.facility_definition && p.title == "设置变更" &&
        p.paragraphs.empty();
}
bool context(const State &s, const Page &p) {
    if (p.lifecycle == 4) return true;
    const Page *scene{};
    bool reached{};
    for (const auto &v : s.scripts.pages) {
        if (v.id == p.id) { reached = true; continue; }
        if (v.lifecycle == 4) continue;
        if (reached || scene || v.kind != ref::WorldScriptPageKind::scene || v.lifecycle != 3)
            return false;
        scene = &v;
    }
    return reached && scene && !s.scene.top_is_main;
}
bool callable(const State &s, std::uint64_t id) {
    const auto *p = top(s);
    return p && p->id == id && p->lifecycle == 2 &&
        valid_startup_world_config_pages(s) &&
        (!s.scripts.executing_page || *s.scripts.executing_page == id);
}
bool system_values(const StartupConfigSystemValues &v) {
    return v.bgm >= 0 && v.bgm <= 5 && v.se >= 0 && v.se <= 5;
}
void advance(State &s, std::uint64_t id) {
    // 10321B63先公共+1模INT_MAX，1032692E再专属+1模20；两步不合成一钟。
    auto &n = s.page_counters.find(id)->second;
    n = n == std::numeric_limits<int>::max() - 1 ? 0 : n + 1;
    n = (n + 1) % 20;
}
bool setting(State &s, StartupWorldConfigResult &result, int row, int value) {
    const std::array<int, 4> before{{s.quality_setting, s.scene.speed_setting,
                                   result.system_volumes[0], result.system_volumes[1]}};
    const auto plan = steam_config_setting_effects({row, row >= 0 && row < 4 ? before[row] : 0, value});
    if (!plan) return false;
    for (const auto &operation : plan->operations) {
        if (const auto *write = std::get_if<SteamConfigPropertyWrite>(&operation)) {
            if (write->property == SteamConfigProperty::world) {
                if (write->index == 12) s.quality_setting = write->value;
                else if (write->index == 13) s.scene.speed_setting = write->value;
                else return false;
            } else if (write->index == 3) result.system_volumes[0] = write->value;
            else if (write->index == 4) result.system_volumes[1] = write->value;
            else return false;
        }
        // 保留属性写在表现后继之前；应用不得据此再次调用原setter或重复试听。
        result.effects.push_back(operation);
    }
    return true;
}
StartupWorldConfigResult rejected(Error error) {
    StartupWorldConfigResult result;
    result.error = error;
    return result;
}
}

bool valid_startup_world_config_page(const State &s, std::uint64_t id) {
    const auto *p = find(s, id);
    if (!s.rules || !p || !metadata(*p) || p->lifecycle < 0 || p->lifecycle > 4 ||
        !context(s, *p) || s.quality_setting < 0 || s.quality_setting > 1 ||
        s.page_phases.count(id) || s.page_secondary_counters.count(id) ||
        s.information_page_data.count(id) || s.manual_page_data.count(id) ||
        s.menu_page_data.count(id) || s.menu_page_positions.count(id) ||
        s.page_human_bindings.count(id) || s.page_job_bindings.count(id)) return false;
    const auto selection = s.config_page_selections.find(id);
    const auto counter = s.page_counters.find(id);
    const bool empty = selection == s.config_page_selections.end() && counter == s.page_counters.end();
    // 当前正常入口只能从已Init的页请求Pop；退休页仍须完整，Finish擦键后同轮删页。
    if (empty) return p->lifecycle == 0 && p->legacy_f == 0;
    return p->lifecycle != 0 && selection != s.config_page_selections.end() &&
        counter != s.page_counters.end() && selection->second >= 0 && selection->second < 4 &&
        counter->second >= 0 && counter->second < 20 && p->legacy_f >= 0 && p->legacy_f <= 1;
}
bool valid_startup_world_config_pages(const State &s) {
    if (s.quality_setting < 0 || s.quality_setting > 1) return false;
    for (const auto &[id, selection] : s.config_page_selections) {
        (void)selection;
        if (!valid_startup_world_config_page(s, id)) return false;
    }
    for (const auto &p : s.scripts.pages)
        if (p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == 12 &&
            !valid_startup_world_config_page(s, p.id)) return false;
    return true;
}
Error open_startup_world_config_page(State &s) {
    const auto *source = top(s);
    if (!source || s.scene.framework_paused || !s.scripts.executing_page ||
        *s.scripts.executing_page != source->id || !startup_world_menu_callback(s, 10))
        return Error::invalid_page;
    const auto view = inspect_startup_world_menu_page(s, source->id);
    if (!view || view->selection < 0 || static_cast<std::size_t>(view->selection) >= view->tags.size() ||
        view->tags[static_cast<std::size_t>(view->selection)] != 21) return Error::invalid_page;
    if (!valid_startup_world_config_pages(s)) return Error::missing_source;
    auto next = s;
    Page page;
    page.kind = ref::WorldScriptPageKind::raw_page; page.legacy_page = 12;
    page.source_record = 10; page.legacy_tag = 21; page.title = "设置变更";
    const auto opened = ref::prepare_world_script_page(startup_world_runtime_scripts(next), page);
    if (!opened.candidate || opened.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(next, opened.candidate->state) ||
        !retire_startup_world_menu_pages(next)) return Error::script_failed;
    next.scene.top_is_main = false;
    if (!valid_startup_world_config_page(next, opened.candidate->inserted_pages.front().id))
        return Error::missing_source;
    s = std::move(next);
    return Error::none;
}
bool initialize_startup_world_config_pages(State &s) {
    if (!valid_startup_world_config_pages(s)) return false;
    const auto *p = top(s);
    if (s.scene.framework_paused || !p || p->kind != ref::WorldScriptPageKind::raw_page ||
        p->legacy_page != 12 || p->lifecycle != 0) return true;
    const auto id = p->id;
    auto next = s;
    auto page = std::find_if(next.scripts.pages.begin(), next.scripts.pages.end(),
                             [=](const auto &v) { return v.id == id; });
    next.config_page_selections.emplace(id, 0);
    next.page_counters.emplace(id, 0);
    page->legacy_f = next.quality_setting;
    page->lifecycle = 1;
    if (!valid_startup_world_config_page(next, id)) return false;
    s = std::move(next);
    return true;
}
std::optional<StartupWorldConfigView> inspect_startup_world_config_page(const State &s,
                                                                      std::uint64_t id) {
    if (!valid_startup_world_config_page(s, id)) return {};
    const auto *p = find(s, id);
    if (p->lifecycle == 0 || p->lifecycle == 4) return {};
    return StartupWorldConfigView{id, s.config_page_selections.find(id)->second,
        s.page_counters.find(id)->second, p->legacy_f};
}
std::optional<State> update_startup_world_config_page(const State &s, std::uint64_t id) {
    if (!callable(s, id)) return {};
    if (s.scene.framework_paused) return s;
    auto next = s;
    advance(next, id);
    return next;
}
StartupWorldConfigResult prepare_startup_world_config_input(
    const State &s, std::uint64_t id, const StartupConfigSystemValues &system,
    const StartupConfigInput &input) {
    if (s.scene.framework_paused || !callable(s, id)) return rejected(Error::invalid_page);
    if (!system_values(system)) return rejected(Error::missing_source);
    const SteamConfigUpdateInput keys{{{{s.quality_setting, s.scene.speed_setting, system.bgm, system.se}},
        s.config_page_selections.find(id)->second, system.speed_unlocked}, input.up_repeat,
        input.down_repeat, input.left_repeat, input.right_repeat, input.back_soft_label};
    const auto plan = steam_config_english_update(keys);
    if (!plan) return rejected(Error::missing_source);
    StartupWorldConfigResult result;
    result.system_volumes = {{system.bgm, system.se}};
    result.checked_keys = plan->checked_keys;
    auto next = s;
    advance(next, id);
    next.config_page_selections.find(id)->second = plan->candidate.selection;
    for (const auto &operation : plan->operations) {
        if (const auto *write = std::get_if<SteamConfigSettingRequest>(&operation)) {
            if (!setting(next, result, write->row, write->after)) return rejected(Error::missing_source);
        } else switch (std::get<SteamConfigReturnRequest>(operation)) {
        case SteamConfigReturnRequest::save_system: result.save_system = true; break;
        case SteamConfigReturnRequest::reset_offscreen: result.reset_offscreen = true; break;
        case SteamConfigReturnRequest::pop: {
            const auto closed = ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
            if (!closed.candidate || !write_startup_world_runtime_scripts(next, closed.candidate->state))
                return rejected(Error::script_failed);
            result.popped = true;
            break;
        }
        }
    }
    if (!valid_startup_world_config_pages(next)) return rejected(Error::missing_source);
    result.candidate = std::move(next);
    return result;
}
StartupWorldConfigResult prepare_startup_world_config_setting(
    const State &s, std::uint64_t id, const StartupConfigSystemValues &system, int row, int value) {
    if (s.scene.framework_paused || !callable(s, id)) return rejected(Error::invalid_page);
    if (!system_values(system)) return rejected(Error::missing_source);
    StartupWorldConfigResult result;
    result.system_volumes = {{system.bgm, system.se}};
    auto next = s;
    if (!setting(next, result, row, value)) return rejected(Error::invalid_page);
    result.candidate = std::move(next);
    return result;
}
Error set_startup_world_config_selection(State &s, std::uint64_t id, int selection) {
    if (s.scene.framework_paused || !callable(s, id) || selection < 0 || selection >= 4)
        return Error::invalid_page;
    s.config_page_selections.find(id)->second = selection;
    return Error::none;
}
} // namespace dungeon_village_prototype
