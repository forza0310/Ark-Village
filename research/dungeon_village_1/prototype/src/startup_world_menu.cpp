// 真实导航菜单只修改唯一Owner候选；冻结目录、原位置和退休身份分别保存。
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Page = ref::WorldScriptPage;
bool navigation(int raw) { return raw == 3 || raw == 4 || raw == 7 || raw == 10; }
bool menu_class(const Page &p) {
    if (p.kind != ref::WorldScriptPageKind::raw_page) return false;
    switch (p.legacy_page) {
    case 3: case 4: case 5: case 7: case 10: case 20: case 9: case 8: return true;
    default: return false;
    }
}
const Page *find(const State &s, std::uint64_t id) {
    const Page *result{};
    for (const auto &p : s.scripts.pages)
        if (p.id == id) {
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
const Page *parent(const State &s, std::uint64_t id) {
    const Page *result{};
    for (const auto &p : s.scripts.pages) {
        if (p.id == id) return result;
        if (p.lifecycle != 4) result = &p;
    }
    return nullptr;
}
std::optional<std::vector<int>> tags(const State &s, int raw) {
    if (raw == 3) {
        std::vector<int> result{0, 1, 2};
        if (s.scripts.user_flags & 1U) result.push_back(3);
        result.insert(result.end(), {5, 6});
        return result;
    }
    if (raw == 4) {
        std::vector<int> result;
        if (s.scripts.user_flags & 4U) result.push_back(7);
        if (s.active_task) {
            const auto task = s.tasks.find(*s.active_task);
            if (task == s.tasks.end() || task->second.identity != *s.active_task) return {};
            result.push_back(8);
        }
        result.push_back(9);
        return result;
    }
    if (raw == 7) {
        std::vector<int> result;
        if (s.scripts.user_flags & 8U) result.push_back(10);
        if (s.scripts.user_flags & 16U) result.push_back(11);
        result.push_back(12);
        return result;
    }
    if (raw == 10) return std::vector<int>{20, 21, 22, 23, 24};
    return {};
}
// 目录在Init冻结，不用后来改变的flag/任务重建已初始化或恢复的页。
bool valid_tags(int raw, const std::vector<int> &v) {
    if (raw == 3) return v == std::vector<int>({0,1,2,5,6}) ||
                         v == std::vector<int>({0,1,2,3,5,6});
    if (raw == 10) return v == std::vector<int>({20,21,22,23,24});
    const std::array<int,3> allowed = raw == 4 ? std::array<int,3>{7,8,9}
                                             : std::array<int,3>{10,11,12};
    if (v.empty() || v.back() != allowed.back()) return false;
    std::size_t next{};
    for (const int tag : v) {
        while (next < allowed.size() && allowed[next] != tag) ++next;
        if (next == allowed.size()) return false;
        ++next;
    }
    return true;
}
std::optional<std::array<int,2>> child_position(const State &s, std::uint64_t id,
                                              bool english) {
    const auto data = s.menu_page_data.find(id);
    const auto pos = s.menu_page_positions.find(id);
    if (data == s.menu_page_data.end() || pos == s.menu_page_positions.end()) return {};
    const auto x = static_cast<std::int64_t>(pos->second[0]) + 68 + (english ? 28 : 0);
    const auto y = static_cast<std::int64_t>(pos->second[1]) + 28LL * data->second.selection;
    if (x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
        y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max()) return {};
    return std::array<int,2>{static_cast<int>(x), static_cast<int>(y)};
}
bool close(State &s, std::uint64_t id) {
    const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
bool push(State &s, int raw, std::optional<std::uint64_t> *id = nullptr, int mode = 0) {
    Page p;
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = raw;
    p.legacy_f = mode;
    p.title = raw == 3 ? "菜单" : raw == 4 ? "冒险" : raw == 7 ? "村办" :
              raw == 10 ? "系统" : raw == 48 ? "升级" : "";
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(s), p);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, r.candidate->state)) return false;
    if (id) *id = r.candidate->inserted_pages.front().id;
    return true;
}
bool invoke(State &s, int event) {
    const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                                             startup_world_runtime_scripts(s), {event, {}, {}});
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
void suspend(State &s, std::uint64_t id) {
    for (auto &p : s.scripts.pages) if (p.id == id && p.lifecycle != 4) p.lifecycle = 3;
    s.scene.top_is_main = false;
}
Error dispatch(State &s, std::uint64_t id, int raw, int tag, bool english) {
    if (raw == 3) {
        if (tag == 1 || tag == 2 || tag == 6)
            return open_startup_world_navigation_submenu(s, tag == 1 ? 4 : tag == 2 ? 7 : 10,
                                                         english);
        if (tag == 5) return open_startup_world_information_menu(s, english);
        if (tag == 3) {
            const auto e = open_startup_world_magic_pot(s, StartupMagicPotEntry::main_menu);
            if (e == Error::none) suspend(s, id); // 原壶入口保留raw3。
            return e;
        }
        if (tag == 0) {
            if (!ref::world_script_seen(s.scripts, 114) && !invoke(s, 114))
                return Error::script_failed;
            const auto e = open_startup_world_build_menu(s);
            if (e != Error::none) return e;
            return retire_startup_world_menu_pages(s) ? Error::none : Error::script_failed;
        }
    } else if (raw == 4) {
        if (tag == 7) return open_startup_world_runtime_task_tracking(s);
        if (tag == 8)
            return act_startup_world_runtime_task_page(s, id,
                                                       StartupWorldTaskAction::request_abort).error;
        if (tag == 9) {
            const auto e = open_startup_world_present_directory(s);
            if (e != Error::none) return e;
            return retire_startup_world_menu_pages(s) ? Error::none : Error::script_failed;
        }
    } else if (raw == 7) {
        Error e{Error::invalid_page};
        if (tag == 10) e = push(s, 48, nullptr, 1) ? Error::none : Error::script_failed;
        if (tag == 11) e = open_startup_world_commerce(s);
        if (tag == 12) e = open_startup_world_village_activities(s);
        if (e != Error::none) return e;
        return retire_startup_world_menu_pages(s) ? Error::none : Error::script_failed;
    } else if (raw == 10) {
        // 系统保存、设置、游戏说明、平台高分及结束须由应用层接管。
        // 在缺真实消费者时保持原Owner，不能造一个可通用关闭的raw页冒称已完成。
        return Error::missing_source;
    }
    return Error::invalid_page;
}
} // namespace

bool valid_startup_world_menu_page(const State &s, std::uint64_t id) {
    const auto *p = find(s, id);
    if (!p || p->kind != ref::WorldScriptPageKind::raw_page || !navigation(p->legacy_page) ||
        p->lifecycle < 0 || p->lifecycle > 4 || s.main_menu_selection < 0 ||
        s.main_menu_selection > 5) return false;
    const auto data = s.menu_page_data.find(id);
    const auto pos = s.menu_page_positions.find(id);
    const auto counter = s.page_counters.find(id);
    if (s.page_phases.count(id)) return false;
    const bool empty = data == s.menu_page_data.end() && counter == s.page_counters.end();
    if (p->lifecycle == 4 && empty && pos == s.menu_page_positions.end()) return true;
    const auto *above = parent(s, id);
    if (empty && p->lifecycle == 0) {
        if (!above || above->lifecycle != 3) return false;
        if (p->legacy_page == 3) {
            const auto rows = tags(s, 3);
            return above->kind == ref::WorldScriptPageKind::scene &&
                   pos == s.menu_page_positions.end() && rows &&
                   static_cast<std::size_t>(s.main_menu_selection) < rows->size();
        }
        if (above->kind == ref::WorldScriptPageKind::scene)
            return pos == s.menu_page_positions.end(); // 明确维护直达子菜单入口。
        if (above->kind != ref::WorldScriptPageKind::raw_page || above->legacy_page != 3 ||
            !valid_startup_world_menu_page(s, above->id) || pos == s.menu_page_positions.end())
            return false;
        return pos->second == child_position(s, above->id, false) ||
               pos->second == child_position(s, above->id, true);
    }
    if (data == s.menu_page_data.end() || pos == s.menu_page_positions.end() ||
        counter == s.page_counters.end() || counter->second < 0 ||
        counter->second > (p->legacy_page == 4 && p->lifecycle == 4 ? 4 : 3) ||
        !valid_tags(p->legacy_page, data->second.tags) || data->second.selection < 0 ||
        static_cast<std::size_t>(data->second.selection) >= data->second.tags.size()) return false;
    if (p->legacy_page == 3) {
        if (data->second.parent) return false;
        if (p->lifecycle != 4 && (!above || above->kind != ref::WorldScriptPageKind::scene ||
            above->lifecycle != 3 || data->second.selection != s.main_menu_selection ||
            pos->second != s.main_menu_position)) return false;
    } else if (data->second.parent) {
        const auto *owner = find(s, *data->second.parent);
        if (!owner || owner->kind != ref::WorldScriptPageKind::raw_page ||
            owner->legacy_page != 3 || owner->id == id) return false;
        if (p->lifecycle != 4 && (!above || above->id != owner->id || owner->lifecycle != 3 ||
            !valid_startup_world_menu_page(s, owner->id) ||
            (pos->second != child_position(s, owner->id, false) &&
             pos->second != child_position(s, owner->id, true)))) return false;
    } else if (p->lifecycle != 4 &&
               (!above || above->kind != ref::WorldScriptPageKind::scene ||
                above->lifecycle != 3)) return false;
    return true;
}
bool initialize_startup_world_menu_pages(State &s) {
    if (s.scene.framework_paused) return true;
    std::vector<std::pair<std::uint64_t, int>> pending;
    for (const auto &p : s.scripts.pages) {
        if (p.kind != ref::WorldScriptPageKind::raw_page || !navigation(p.legacy_page) ||
            p.lifecycle == 4) continue;
        if (!valid_startup_world_menu_page(s, p.id)) return false;
        if (!s.menu_page_data.count(p.id)) pending.emplace_back(p.id, p.legacy_page);
    }
    // 普通世界或已初始化菜单不制造一份仅供空Init使用的完整Owner副本。
    if (pending.empty()) return true;
    auto next = s;
    for (const auto &[id, raw] : pending) {
        const auto rows = tags(next, raw);
        if (!rows) return false;
        StartupWorldMenuPageData data;
        data.tags = *rows;
        if (raw == 3) {
            data.selection = next.main_menu_selection;
            if (static_cast<std::size_t>(data.selection) >= rows->size() ||
                !refresh_startup_world_build_notices(next)) return false;
            next.menu_page_positions.emplace(id, next.main_menu_position);
        } else {
            const auto *owner = parent(next, id);
            if (!owner) return false;
            if (owner->kind == ref::WorldScriptPageKind::raw_page) data.parent = owner->id;
            else next.menu_page_positions.emplace(id, std::array<int,2>{0,25});
        }
        next.menu_page_data.emplace(id, std::move(data));
        next.page_counters.emplace(id, 0);
        if (!valid_startup_world_menu_page(next, id)) return false;
    }
    s = std::move(next);
    return true;
}
std::optional<StartupWorldMenuView> inspect_startup_world_menu_page(const State &s,
                                                                  std::uint64_t id) {
    const auto *p = find(s, id);
    if (!p || !valid_startup_world_menu_page(s, id) || !s.menu_page_data.count(id)) return {};
    const auto &data = s.menu_page_data.find(id)->second;
    return StartupWorldMenuView{id, p->legacy_page, s.page_counters.find(id)->second,
                               data.selection, s.menu_page_positions.find(id)->second, data.tags};
}
bool startup_world_menu_callback(const State &s, int raw) {
    if (!s.rules || s.scene.framework_paused || !s.scripts.executing_page) return false;
    const auto *p = find(s, *s.scripts.executing_page);
    return p && p->legacy_page == raw && p->lifecycle == 2 &&
           valid_startup_world_menu_page(s, p->id) && s.menu_page_data.count(p->id);
}
Error open_startup_world_main_menu(State &s) {
    const auto *p = top(s);
    if (!s.rules || s.scene.framework_paused || !p || p->kind != ref::WorldScriptPageKind::scene ||
        s.scene.scene_state != 0) return Error::invalid_page;
    const auto rows = tags(s, 3);
    if (!rows || s.main_menu_selection < 0 ||
        static_cast<std::size_t>(s.main_menu_selection) >= rows->size()) return Error::missing_source;
    for (const auto &page : s.scripts.pages)
        if (page.lifecycle != 4 && menu_class(page)) return Error::invalid_page;
    auto next = s;
    next.scripts.executing_page = p->id;
    if (!push(next, 3)) return Error::script_failed;
    suspend(next, p->id);
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
Error open_startup_world_navigation_submenu(State &s, int raw, bool english) {
    if (raw != 4 && raw != 7 && raw != 10) return Error::invalid_page;
    const auto *p = top(s);
    const bool callback = startup_world_menu_callback(s, 3);
    if (!s.rules || s.scene.framework_paused || !p ||
        (!callback && (p->kind != ref::WorldScriptPageKind::scene || s.scene.scene_state != 0)))
        return Error::invalid_page;
    const auto anchor = callback ? *s.scripts.executing_page : p->id;
    const auto position = callback ? child_position(s, anchor, english) : std::nullopt;
    if (callback && !position) return Error::missing_source;
    auto next = s;
    next.scripts.executing_page = anchor;
    std::optional<std::uint64_t> id;
    if (!push(next, raw, &id)) return Error::script_failed;
    if (position) next.menu_page_positions.emplace(*id, *position);
    suspend(next, anchor);
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
bool retire_startup_world_menu_pages(State &s) {
    auto next = s;
    std::vector<std::uint64_t> ids;
    for (auto p = next.scripts.pages.rbegin(); p != next.scripts.pages.rend(); ++p)
        if (p->lifecycle != 4 && menu_class(*p)) ids.push_back(p->id);
    for (const auto id : ids) if (!close(next, id)) return false;
    s = std::move(next);
    return true;
}
Error input_startup_world_menu_page(State &s, std::uint64_t id, const StartupWorldMenuInput &input) {
    const auto *p = top(s);
    if (!p || p->id != id || p->lifecycle != 2 || s.scene.framework_paused ||
        !navigation(p->legacy_page)) return Error::invalid_page;
    if (!valid_startup_world_menu_page(s, id)) return Error::missing_source;
    const auto &data = s.menu_page_data.find(id)->second;
    const bool keys = input.up || input.down || input.left || input.right || input.confirm ||
                      input.cancel || input.save_shortcut || input.browser_shortcut;
    if (input.select_row && (keys || *input.select_row < 0 ||
        static_cast<std::size_t>(*input.select_row) >= data.tags.size())) return Error::invalid_page;
    if (input.save_shortcut || input.browser_shortcut) return Error::missing_source;
    auto next = s;
    next.scripts.executing_page = id;
    int row = data.selection;
    if (input.select_row) row = *input.select_row;
    else if (input.up) row = (row + static_cast<int>(data.tags.size()) - 1) %
                            static_cast<int>(data.tags.size());
    else if (input.down) row = (row + 1) % static_cast<int>(data.tags.size());
    next.menu_page_data.find(id)->second.selection = row;
    if (!input.select_row && !input.up && !input.down && (input.confirm || input.right)) {
        next.page_counters.find(id)->second = 3;
        const auto e = dispatch(next, id, p->legacy_page, data.tags[static_cast<std::size_t>(row)],
                                input.english);
        if (e != Error::none) return e;
    } else if (!input.select_row && !input.up && !input.down &&
               (input.cancel || input.left) && !close(next, id)) return Error::script_failed;
    if (p->legacy_page == 3) next.main_menu_selection = row;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
std::optional<State> update_startup_world_menu_page(const State &s, std::uint64_t id) {
    const auto *p = top(s);
    if (!p || p->id != id || p->lifecycle != 2 || s.scene.framework_paused ||
        !valid_startup_world_menu_page(s, id)) return {};
    auto next = s;
    // 公共Update和FrameMenu各增一次，后者封顶3。导航动作另经显式已解析输入提交。
    const auto answer = s.task_abort_answers.find(id);
    const bool aborting = p->legacy_page == 4 && answer != s.task_abort_answers.end() &&
                          answer->second == 0;
    // 是答先经公共Update，再直接中止/退休；不执行后面的FrameMenu加1/封顶。
    next.page_counters.find(id)->second = aborting ? next.page_counters.find(id)->second + 1
        : std::min(3, next.page_counters.find(id)->second + 2);
    if (p->legacy_page == 4) {
        auto consumed = update_startup_world_runtime_task_control_page(next, id);
        if (!consumed) return {};
        next = std::move(*consumed);
    }
    return next;
}
} // namespace dungeon_village_prototype
