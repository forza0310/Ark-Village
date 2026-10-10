// raw13：原Init抽N次Random(N)后尾部反取4；翻页不重抽，确认脉冲只消费一次。
// APK b/g:10593–10634,11468–11484；Steam Init10314BF0、Update103267E2。
#include "dungeon_village_prototype/startup_world_manual.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Page = ref::WorldScriptPage;
using Error = StartupWorldRuntimeError;
const Page *find(const State &s, std::uint64_t id) {
    const Page *result{};
    for (const auto &page : s.scripts.pages) if (page.id == id) {
        if (!id || result) return nullptr;
        result = &page;
    }
    return result;
}
const Page *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
        [](const auto &page) { return page.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
bool rules(const State &s) {
    return s.rules && !s.rules->manual_pages.empty() &&
        s.rules->manual_pages.size() < static_cast<std::size_t>(std::numeric_limits<int>::max());
}
bool context(const State &s, const Page &page) {
    if (page.lifecycle == 4) return true;
    const Page *scene{};
    bool reached{};
    for (const auto &p : s.scripts.pages) {
        if (p.id == page.id) { reached = true; continue; }
        if (p.lifecycle == 4) continue;
        if (reached || scene || p.kind != ref::WorldScriptPageKind::scene || p.lifecycle != 3)
            return false;
        scene = &p;
    }
    return reached && scene && !s.scene.top_is_main;
}
bool metadata(const Page &p) {
    return p.kind == ref::WorldScriptPageKind::raw_page && p.legacy_page == 13 &&
        p.source_record == 10 && p.legacy_tag == 22 && p.replacement.empty() &&
        p.speaker_kind == 0 && p.speaker_definition == -1 && p.legacy_r == 0 &&
        p.legacy_s == 0 && p.legacy_t == 0 && p.legacy_f == 0 && p.legacy_g == 0 &&
        p.legacy_l == 0 && p.message_commands.empty() && !p.task_identity &&
        !p.task_definition && !p.monster_definition && !p.facility_definition &&
        p.title == "游戏方法" && p.paragraphs.empty();
}
bool callable(const State &s, std::uint64_t id) {
    const auto *page = top(s);
    return page && page->id == id && page->lifecycle == 2 &&
        valid_startup_world_manual_page(s, id) &&
        (!s.scripts.executing_page || *s.scripts.executing_page == id);
}
}

bool valid_startup_world_manual_page(const State &s, std::uint64_t id) {
    const auto *p = find(s, id);
    if (!rules(s) || !p || !metadata(*p) || p->lifecycle < 0 || p->lifecycle > 4 ||
        !context(s, *p) || s.information_page_data.count(id) || s.menu_page_data.count(id) ||
        s.menu_page_positions.count(id) || s.page_secondary_counters.count(id) ||
        s.page_human_bindings.count(id) || s.page_job_bindings.count(id)) return false;
    const auto phase = s.page_phases.find(id), counter = s.page_counters.find(id);
    const auto data = s.manual_page_data.find(id);
    const bool empty = phase == s.page_phases.end() && counter == s.page_counters.end() &&
        data == s.manual_page_data.end();
    if (empty) return p->lifecycle == 0 || p->lifecycle == 4;
    if (p->lifecycle == 0 || phase == s.page_phases.end() || counter == s.page_counters.end() ||
        data == s.manual_page_data.end() || phase->second < 0 ||
        static_cast<std::size_t>(phase->second) > s.rules->manual_pages.size() ||
        counter->second < 0 || counter->second >= std::numeric_limits<int>::max() ||
        data->second.decorations.size() > 4 || data->second.text_page < 0 ||
        static_cast<std::size_t>(data->second.text_page) >= s.rules->manual_pages.size() ||
        (static_cast<std::size_t>(phase->second) < s.rules->manual_pages.size() &&
         data->second.text_page != phase->second) ||
        (static_cast<std::size_t>(phase->second) == s.rules->manual_pages.size() &&
         data->second.text_page != 0 &&
         static_cast<std::size_t>(data->second.text_page) != s.rules->manual_pages.size() - 1) ||
        (data->second.localize_text && data->second.text_page != 0)) return false;
    std::set<int> used;
    for (const int human : data->second.decorations) {
        if (!used.insert(human).second || std::count_if(s.rules->humans.begin(), s.rules->humans.end(),
            [&](const auto &entry) { return entry.identity == human; }) != 1) return false;
    }
    // X是Init冻结结果；后续presence变化不重新洗牌，也不要求与当前池相等。
    return true;
}

Error open_startup_world_manual_page(State &s) {
    const auto *source = top(s);
    if (!rules(s)) return Error::missing_source;
    if (!source || s.scene.framework_paused || !s.scripts.executing_page ||
        *s.scripts.executing_page != source->id || !startup_world_menu_callback(s, 10))
        return Error::invalid_page;
    const auto view = inspect_startup_world_menu_page(s, source->id);
    if (!view || view->selection < 0 || static_cast<std::size_t>(view->selection) >= view->tags.size() ||
        view->tags[static_cast<std::size_t>(view->selection)] != 22) return Error::invalid_page;
    auto next = s;
    Page page;
    page.kind = ref::WorldScriptPageKind::raw_page; page.legacy_page = 13;
    page.source_record = 10; page.legacy_tag = 22; page.title = "游戏方法";
    const auto opened = ref::prepare_world_script_page(startup_world_runtime_scripts(next), page);
    if (!opened.candidate || opened.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(next, opened.candidate->state) ||
        !retire_startup_world_menu_pages(next)) return Error::script_failed;
    next.scene.top_is_main = false;
    if (!valid_startup_world_manual_page(next, opened.candidate->inserted_pages.front().id))
        return Error::missing_source;
    s = std::move(next);
    return Error::none;
}

bool initialize_startup_world_manual_pages(State &s) {
    std::vector<std::uint64_t> pending;
    for (const auto &page : s.scripts.pages)
        if (page.kind == ref::WorldScriptPageKind::raw_page && page.legacy_page == 13) {
            if (!valid_startup_world_manual_page(s, page.id)) return false;
            if (page.lifecycle == 0) pending.push_back(page.id);
        }
    if (s.scene.framework_paused || pending.empty()) return true;
    auto next = s;
    for (const auto id : pending) {
        std::vector<int> pool;
        std::set<int> definitions;
        for (const auto &human : next.rules->humans) {
            const auto status = next.human_presence.find(human.identity);
            if (!definitions.insert(human.identity).second || status == next.human_presence.end()) return false;
            if (status->second != 0) pool.push_back(human.identity);
        }
        if (pool.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) return false;
        for (std::size_t i = 0; i < pool.size(); ++i) {
            const auto draw = next.scene.random.draw(static_cast<int>(pool.size()));
            if (draw.error != ref::WorldRandomError::none) return false;
            std::swap(pool[i], pool[static_cast<std::size_t>(draw.ticket)]);
        }
        std::vector<int> decorations;
        for (auto i = pool.rbegin(); i != pool.rend() && decorations.size() < 4; ++i)
            decorations.push_back(*i);
        next.manual_page_data.emplace(id, StartupManualPageData{std::move(decorations), 0, true});
        next.page_phases.emplace(id, 0); next.page_counters.emplace(id, 0);
        for (auto &page : next.scripts.pages) if (page.id == id) page.lifecycle = 1;
        if (!valid_startup_world_manual_page(next, id)) return false;
    }
    s = std::move(next);
    return true;
}

std::optional<StartupManualPageView> inspect_startup_world_manual_page(const State &s, std::uint64_t id) {
    if (!valid_startup_world_manual_page(s, id)) return {};
    const auto *page = find(s, id);
    if (page->lifecycle == 0 || page->lifecycle == 4) return {};
    const int index = s.page_phases.find(id)->second;
    const bool about = static_cast<std::size_t>(index) == s.rules->manual_pages.size();
    const auto &data = s.manual_page_data.find(id)->second;
    return StartupManualPageView{id, index, s.page_counters.find(id)->second, about,
        s.rules->manual_pages[static_cast<std::size_t>(data.text_page)],
        data.decorations, data.text_page, data.localize_text};
}

Error input_startup_world_manual_page(State &s, std::uint64_t id, const StartupManualInput &input) {
    if (s.scene.framework_paused || !callable(s, id)) return Error::invalid_page;
    auto next = s;
    auto &index = next.page_phases.find(id)->second;
    const auto old_index = index;
    const int pages = static_cast<int>(next.rules->manual_pages.size()) + 1;
    if (input.confirm || input.right) index = index == pages - 1 ? 0 : index + 1;
    if (input.left) index = index == 0 ? pages - 1 : index - 1;
    if (index != old_index && index != pages - 1) {
        auto &data = next.manual_page_data.find(id)->second;
        data.text_page = index;
        data.localize_text = false;
    }
    if (input.cancel) {
        const auto closed = ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
        if (!closed.candidate || !write_startup_world_runtime_scripts(next, closed.candidate->state))
            return Error::script_failed;
    }
    s = std::move(next);
    return Error::none;
}

std::optional<State> update_startup_world_manual_page(const State &s, std::uint64_t id) {
    if (!callable(s, id)) return {};
    if (s.scene.framework_paused) return s;
    auto next = s;
    auto &counter = next.page_counters.find(id)->second;
    counter = counter == std::numeric_limits<int>::max() - 1 ? 0 : counter + 1;
    return next;
}
} // namespace dungeon_village_prototype
