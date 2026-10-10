#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"

#include <algorithm>
#include <limits>
#include <utility>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Page = ref::WorldScriptPage;
constexpr std::array<StartupInformationEntry, 5> entries{{
    {15, "冒险者", 35, true}, {14, "村情报", 34, true},
    {16, "收支情报", 36, true}, {17, "持有物品", 37, true},
    {18, "装备一览", 38, true}}};
bool information(const Page &page) {
    return page.kind == ref::WorldScriptPageKind::raw_page &&
           (page.legacy_page == 9 || (page.legacy_page >= 34 && page.legacy_page <= 40));
}
bool directory(const Page &page) {
    return page.legacy_page == 35 || page.legacy_page == 37 || page.legacy_page == 38 || page.legacy_page == 39 || page.legacy_page == 40;
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
std::optional<std::vector<std::vector<int>>> lists(const State &s, int raw) {
    std::vector<std::vector<int>> result;
    if (raw == 35 || raw == 40) {
        if (!s.rules) return {};
        result.emplace_back();
        for (const auto &human : s.rules->humans) {
            const auto presence = s.human_presence.find(human.identity);
            if (presence == s.human_presence.end()) return {};
            if (presence->second != 0) result.back().push_back(human.identity);
        }
        // 35贡献重算及35/40循环选行都要求非空群体；维护拒绝，不生成假人物。
        if (result.front().empty()) return {};
    } else if (raw == 37) {
        const auto rows = startup_item_information(s);
        if (!rows) return {};
        result.emplace_back();
        for (const auto &row : *rows) result.back().push_back(row.definition);
    } else if (raw == 38) {
        for (int slot = 0; slot < 4; ++slot) {
            const auto rows = startup_equipment_information(s, slot, StartupInformationEdition::steam_2_56);
            if (!rows) return {};
            result.emplace_back();
            for (const auto &row : rows->rows) result.back().push_back(row.definition);
        }
    } else return {};
    for (const auto &list : result)
        if (list.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) return {};
    return result;
}
bool payload(const State &s, const Page &page) {
    const auto phase = s.page_phases.find(page.id);
    const auto counter = s.page_counters.find(page.id);
    const int last_phase = page.legacy_page == 9 ? 4 :
                          (page.legacy_page == 35 || page.legacy_page == 38) ? 3 :
                          page.legacy_page == 40 ? 2 :
                          page.legacy_page == 36 ? 1 : 0;
    if (phase == s.page_phases.end() || counter == s.page_counters.end() ||
        phase->second < 0 || phase->second > last_phase || counter->second < 0 ||
        counter->second > (page.legacy_page == 9 ? 3 : std::numeric_limits<int>::max() - 1))
        return false;
    const auto data = s.information_page_data.find(page.id);
    if (!directory(page))
        return data == s.information_page_data.end() &&
               (page.legacy_page != 34 || startup_town_information(s).has_value());
    if (data == s.information_page_data.end()) return false;
    const auto &v = data->second;
    std::size_t list_size{};
    if (page.legacy_page == 39) {
        const auto rows = startup_facility_information(s);
        if (!rows || !v.lists.empty() || rows->size() != v.facilities.size()) return false;
        for (std::size_t i = 0; i < rows->size(); ++i)
            if ((*rows)[i].instance != v.facilities[i]) return false;
        list_size = v.facilities.size();
    } else {
        const auto expected = lists(s, page.legacy_page);
        // 当前维护页模态；35/40子页可改属性/职业但不改presence，冻结目录须完整一致。
        // 这是本消费者的恢复约束，不推断原程序所有异步路径都不会改共享定义。
        if (!expected || v.lists != *expected || !v.facilities.empty()) return false;
        list_size = v.lists[(page.legacy_page == 35 || page.legacy_page == 40) ? 0U : static_cast<std::size_t>(phase->second)].size();
    }
    if (v.selection < 0 || v.first_visible < 0) return false;
    if (list_size == 0)
        return (page.legacy_page == 38 || page.lifecycle == 4) && v.selection == 0 && v.first_visible == 0;
    if (list_size > static_cast<std::size_t>(std::numeric_limits<int>::max())) return false;
    const int count = static_cast<int>(list_size), rows = page.legacy_page == 38 ? 4 : 5;
    return v.selection < count && v.first_visible <= v.selection &&
           v.selection - v.first_visible < rows && v.first_visible <= std::max(0, count - rows);
}
// 仅投影贡献所需字段；原B1/B2分别由战斗累计与消费账本持有，不能读陈旧日历镜像。
bool refresh_contributions(State &s) {
    if (!s.rules) return false;
    std::vector<ref::WorldAwardHuman> humans;
    for (const auto &human : s.rules->humans) {
        const int id = human.identity;
        const auto calendar = s.human_calendar.find(id);
        const auto presence = s.human_presence.find(id);
        const auto battle = s.scene.world.world.ai.battle.humans.find(id);
        const auto spending = s.scene.world.world.human_spending.find(id);
        if (calendar == s.human_calendar.end() || presence == s.human_presence.end() ||
            battle == s.scene.world.world.ai.battle.humans.end() ||
            spending == s.scene.world.world.human_spending.end()) return false;
        auto totals = calendar->second.yearly_totals;
        totals[1] = battle->second.killed_stat1;
        totals[2] = spending->second;
        humans.push_back({id, presence->second, totals, calendar->second.contribution});
    }
    auto result = ref::prepare_world_human_contributions(std::move(humans));
    if (!result.candidate) return false;
    for (const auto &human : *result.candidate)
        s.human_calendar.find(human.definition)->second.contribution = human.contribution;
    return true;
}
bool clear_human_notices(State &s, const std::vector<int> &ids) {
    for (const int id : ids) if (!s.scripts.humans.count(id)) return false;
    for (const int id : ids) s.scripts.humans.find(id)->second.pending_notice = false;
    return true;
}
bool close(State &s, std::uint64_t id) {
    const auto result = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
    return result.candidate && write_startup_world_runtime_scripts(s, result.candidate->state);
}
bool event(State &s, int id) {
    const auto result = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                  startup_world_runtime_scripts(s), {id, {}, {}});
    // 空37/39分别消费固定事件15/17；页锁吞掉输出不算完成空目录提示。
    return result.candidate && !result.candidate->inserted_pages.empty() &&
           write_startup_world_runtime_scripts(s, result.candidate->state);
}
bool push(State &s, int raw, std::uint64_t *created = nullptr) {
    Page child;
    child.kind = ref::WorldScriptPageKind::raw_page;
    child.legacy_page = raw;
    child.title = raw == 9 ? "情报" : raw == 34 ? "村情报" : raw == 35 ? "冒险者" :
                  raw == 36 ? "收支情报" : raw == 37 ? "持有物品" : raw == 38 ? "装备一览" :
                  raw == 40 ? "赠送礼物" : raw == 64 ? "装备与道具" : "设施收支一览";
    const auto result = ref::prepare_world_script_page(startup_world_runtime_scripts(s), child);
    if (!result.candidate || result.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, result.candidate->state)) return false;
    if (created) *created = result.candidate->inserted_pages.front().id;
    return true;
}
void scroll(StartupInformationPageData &data, int rows) {
    if (data.first_visible > data.selection) data.first_visible = data.selection;
    if (data.selection - data.first_visible >= rows) data.first_visible = data.selection - rows + 1;
}
} // namespace

bool valid_startup_world_information_page(const State &s, std::uint64_t id) {
    const auto *page = find_page(s, id);
    if (!page || !information(*page) || page->lifecycle < 0 || page->lifecycle > 4)
        return false;
    if (page->legacy_page == 9 && page->lifecycle != 4) {
        const Page *parent = nullptr;
        for (const auto &p : s.scripts.pages) {
            if (p.id == id) break;
            if (p.lifecycle != 4) parent = &p;
        }
        if (!parent || parent->lifecycle != 3) return false;
        const auto position = s.menu_page_positions.find(id);
        if (parent->kind == ref::WorldScriptPageKind::scene) {
            if (position != s.menu_page_positions.end()) return false;
        } else {
            if (parent->kind != ref::WorldScriptPageKind::raw_page || parent->legacy_page != 3 ||
                position == s.menu_page_positions.end()) return false;
            const auto menu = inspect_startup_world_menu_page(s, parent->id);
            if (!menu) return false;
            const auto x = static_cast<std::int64_t>(menu->stored_position[0]) + 68;
            const auto y = static_cast<std::int64_t>(menu->stored_position[1]) + 28LL * menu->selection;
            // 未持久化语言；只接受原非英语/英语两种实际偏移，不用当前flag重建父目录。
            if (position->second[1] != y ||
                (position->second[0] != x && position->second[0] != x + 28)) return false;
        }
    }
    if (page->legacy_page == 39 && page->lifecycle != 4) {
        const Page *parent = nullptr;
        for (const auto &p : s.scripts.pages) {
            if (p.id == id) break;
            if (p.lifecycle != 4) parent = &p;
        }
        if (!parent || parent->kind != ref::WorldScriptPageKind::raw_page ||
            parent->legacy_page != 34 || parent->lifecycle != 3 ||
            !valid_startup_world_information_page(s, parent->id)) return false;
    }
    const bool no_payload = !s.page_phases.count(id) && !s.page_counters.count(id) &&
                            !s.information_page_data.count(id);
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
    std::vector<std::uint64_t> pending;
    for (const auto &page : s.scripts.pages) {
        if (information(page) && !valid_startup_world_information_page(s, page.id))
            return false;
        if (information(page) && page.lifecycle == 0) pending.push_back(page.id);
    }
    for (const auto &entry : s.information_page_data) {
        const auto *page = find_page(s, entry.first);
        if (!page || !information(*page) || !directory(*page)) return false;
    }
    if (pending.empty()) return true;
    auto next = s;
    const auto executing = next.scripts.executing_page;
    for (const auto id : pending) {
        const auto *page = find_page(next, id);
        if (!page || page->lifecycle != 0) return false;
        const int raw = page->legacy_page;
        int empty_event{};
        if (raw == 35 && !refresh_contributions(next)) return false;
        if (raw == 34 && !startup_town_information(next)) return false;
        if (raw == 39) {
            const auto rows = startup_facility_information(next);
            if (!rows) return false;
            StartupInformationPageData data;
            for (const auto &row : *rows) data.facilities.push_back(row.instance);
            if (data.facilities.empty()) empty_event = 17;
            next.information_page_data.emplace(id, std::move(data));
        } else if (directory(*page)) {
            auto frozen = lists(next, raw);
            if (!frozen) return false;
            if (raw == 37 && frozen->front().empty()) empty_event = 15;
            next.information_page_data.emplace(id, StartupInformationPageData{std::move(*frozen), 0, 0, {}});
        }
        next.page_phases.emplace(id, 0);
        next.page_counters.emplace(id, 0);
        for (auto &p : next.scripts.pages) if (p.id == id) p.lifecycle = 1;
        next.scripts.executing_page = id;
        // 事件可压新页并使vector重分配；后续只用ID，不能再引用旧page。
        if (empty_event && (!event(next, empty_event) || !close(next, id))) return false;
        // APK b/g:10809：40与35共用原序人物目录，但仅35重算贡献；98仅40首次执行。
        if (raw == 40 && !ref::world_script_seen(next.scripts, 98) && !event(next, 98)) return false;
        if (!valid_startup_world_information_page(next, id)) return false;
    }
    next.scripts.executing_page = executing;
    s = std::move(next);
    return true;
}

Error open_startup_world_information_menu(State &s, bool english) {
    const bool callback = startup_world_menu_callback(s, 3);
    const auto *parent = callback ? find_page(s, *s.scripts.executing_page) : top(s);
    if (!s.rules || s.scene.framework_paused || !parent || (!callback && parent->lifecycle != 2) ||
        !((parent->kind == ref::WorldScriptPageKind::scene && s.scene.scene_state == 0) ||
          callback))
        return Error::invalid_page;
    const auto id = parent->id;
    std::optional<std::array<int, 2>> position;
    if (callback) {
        const auto menu = inspect_startup_world_menu_page(s, id);
        if (!menu) return Error::missing_source;
        const auto x = static_cast<std::int64_t>(menu->stored_position[0]) + 68 + (english ? 28 : 0);
        const auto y = static_cast<std::int64_t>(menu->stored_position[1]) + 28LL * menu->selection;
        if (x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
            y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max())
            return Error::missing_source;
        position = std::array<int, 2>{static_cast<int>(x), static_cast<int>(y)};
    }
    auto next = s;
    next.scripts.executing_page = id;
    for (auto &page : next.scripts.pages)
        if (page.id == id)
            page.lifecycle = 3;
    std::uint64_t child{};
    if (!push(next, 9, &child))
        return Error::script_failed;
    if (position) next.menu_page_positions.emplace(child, *position);
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}

Error open_startup_world_present_directory(State &s) {
    if (!s.rules || s.scene.framework_paused || !startup_world_menu_callback(s, 4))
        return Error::invalid_page;
    auto next = s;
    if (!push(next, 40)) return Error::script_failed;
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
    if (input.select_row) {
        if (keys || *input.select_row < 0 || raw == 34 || raw == 36) return Error::invalid_page;
        if (raw == 9) {
            if (*input.select_row > 4) return Error::invalid_page;
        } else {
            const auto &data = s.information_page_data.find(id)->second;
            const auto phase = s.page_phases.find(id)->second;
            const auto count = raw == 39 ? data.facilities.size() :
                data.lists[(raw == 35 || raw == 40) ? 0U : static_cast<std::size_t>(phase)].size();
            if (static_cast<std::size_t>(*input.select_row) >= count)
                return Error::invalid_page;
        }
    }
    if (raw == 36 && (input.up || input.down))
        return Error::invalid_page;
    if ((raw == 37 || raw == 39) && (input.left || input.right))
        return Error::invalid_page;
    if (raw == 34 && (input.up || input.down || input.left || input.right))
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
            // 先压真实目标，后逆序退休菜单；37空目录由后续Init处理。
            if (!push(next, entries[static_cast<std::size_t>(phase->second)].target_raw))
                return Error::script_failed;
            if (!retire_startup_world_menu_pages(next)) return Error::script_failed;
        } else if (input.cancel || input.left) {
            if (!close(next, id))
                return Error::script_failed;
        }
    } else if (raw == 34) {
        // 原确认优先返回，39保留实际34为父页；没有方向键业务消费者。
        if (input.confirm) {
            for (auto &p : next.scripts.pages) if (p.id == id) p.lifecycle = 3;
            if (!push(next, 39)) return Error::script_failed;
        } else if (input.cancel && !close(next, id)) return Error::script_failed;
    } else if (raw == 39) {
        auto &data = next.information_page_data.find(id)->second;
        const int count = static_cast<int>(data.facilities.size());
        if (input.select_row) data.selection = *input.select_row;
        else {
            if (input.up) data.selection = data.selection == 0 ? count - 1 : data.selection - 1;
            if (input.down) data.selection = data.selection == count - 1 ? 0 : data.selection + 1;
        }
        scroll(data, 5);
        if (input.confirm) {
            const auto facility = data.facilities[static_cast<std::size_t>(data.selection)];
            // 源直接拿对象引用；维护重验稳定ID/占地后才退栈，缺实体不能半切镜头。
            if (!startup_world_runtime_facility_target(next, facility)) return Error::missing_source;
            if (!activate_startup_world_main_page(next)) return Error::script_failed;
            next.scene.scene_state = 7;
            next.scene.scene_counter = 0;
            next.scripts.selected_facility = facility;
            next.scripts.selection_mode = 0;
            // 原39不清subPlayer/subMonster，保留两者选择。
        } else if (input.cancel && !close(next, id)) return Error::script_failed;
    } else if (raw == 36) {
        // 原36左右独立：同轮双向回原页，随后仍可确认/返回关闭。
        if (input.left)
            phase->second = (phase->second + 1) % 2;
        if (input.right)
            phase->second = (phase->second + 1) % 2;
        if ((input.confirm || input.cancel) && !close(next, id))
            return Error::script_failed;
    } else if (raw == 35 || raw == 40) {
        auto &data = next.information_page_data.find(id)->second;
        const auto &humans = data.lists.front();
        const int count = static_cast<int>(humans.size());
        if (input.select_row) data.selection = *input.select_row;
        else {
            if (input.up) data.selection = data.selection == 0 ? count - 1 : data.selection - 1;
            if (input.down) data.selection = data.selection == count - 1 ? 0 : data.selection + 1;
        }
        scroll(data, 5);
        // 35/40先上下/滚动、再独立左右；翻页不会重置同一人物目录的选择。
        const int pages = raw == 35 ? 4 : 3;
        if (input.left) phase->second = (phase->second + pages - 1) % pages;
        if (input.right) phase->second = (phase->second + 1) % pages;
        if (input.confirm || input.cancel) {
            if (!clear_human_notices(next, humans)) return Error::missing_source;
            if (input.confirm) {
                if (raw == 35) {
                    if (!append_startup_world_human_detail_page(next, humans[data.selection], 1))
                        return Error::script_failed;
                } else {
                    const int human = humans[data.selection];
                    if (!startup_world_human_details(next, human)) return Error::missing_source;
                    for (auto &p : next.scripts.pages) if (p.id == id) p.lifecycle = 3;
                    std::uint64_t child{};
                    if (!push(next, 64, &child)) return Error::script_failed;
                    next.page_human_bindings.emplace(child, human);
                    next.page_phases.emplace(child, 0);
                    next.page_counters.emplace(child, 0);
                    next.human_page_selections.emplace(child, 0);
                }
            } else if (!close(next, id)) return Error::script_failed;
        }
    } else {
        auto &data = next.information_page_data.find(id)->second;
        if (raw == 38) {
            const int before = phase->second;
            if (input.left) phase->second = (phase->second + 3) % 4;
            if (input.right) phase->second = (phase->second + 1) % 4;
            if (before != phase->second) data.selection = data.first_visible = 0;
        }
        const int count = static_cast<int>(data.lists[static_cast<std::size_t>(phase->second)].size());
        if (input.select_row) data.selection = *input.select_row;
        else if (count > 0) {
            // 上下独立，不能套raw9的互斥优先级；合法空38跳过取模。
            if (input.up) data.selection = data.selection == 0 ? count - 1 : data.selection - 1;
            if (input.down) data.selection = data.selection == count - 1 ? 0 : data.selection + 1;
        }
        scroll(data, raw == 37 ? 5 : 4);
        if (input.confirm || input.cancel) {
            if (raw == 37 && !clear_startup_world_item_notices(next)) return Error::missing_source;
            if (!close(next, id)) return Error::script_failed;
        }
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
                                    entries, {}, 0, 0, {}, {}, {}, {}, {}, {}};
    if (page->legacy_page == 34) {
        view.town = startup_town_information(s);
        view.village_name = s.scripts.village_name;
        if (!view.town) return {};
    } else if (page->legacy_page == 36) {
        view.income = startup_income_information(s.monthly_cash, s.scene.calendar.month,
                                                 phase->second);
        if (!view.income)
            return {};
    } else if (directory(*page)) {
        const auto &data = s.information_page_data.find(id)->second;
        view.selection = data.selection;
        view.first_visible = data.first_visible;
        if (page->legacy_page == 39) {
            view.facilities = startup_facility_information(s);
            if (!view.facilities) return {};
        } else if (page->legacy_page == 35 || page->legacy_page == 40) {
            view.humans.emplace();
            for (const int human : data.lists.front()) {
                const auto details = startup_world_human_details(s, human);
                const auto presence = s.human_presence.find(human);
                const auto calendar = s.human_calendar.find(human);
                const auto script = s.scripts.humans.find(human);
                const auto battle = s.scene.world.world.ai.battle.humans.find(human);
                const auto spending = s.scene.world.world.human_spending.find(human);
                if (!details || presence == s.human_presence.end() || calendar == s.human_calendar.end() ||
                    script == s.scripts.humans.end() || battle == s.scene.world.world.ai.battle.humans.end() ||
                    spending == s.scene.world.world.human_spending.end()) return {};
                view.humans->push_back({*details, presence->second, calendar->second.contribution,
                                       battle->second.killed_stat1, spending->second,
                                       script->second.pending_notice});
            }
        } else if (page->legacy_page == 37) {
            view.items = startup_item_information(s);
            if (!view.items) return {};
        } else {
            view.equipment = startup_equipment_information(s, phase->second,
                                                            StartupInformationEdition::steam_2_56);
            if (!view.equipment) return {};
        }
        // payload已核冻结列表与当前资格/原序完整相等；此处仅取当前数值和文字。
    }
    return view;
}
} // namespace dungeon_village_prototype
