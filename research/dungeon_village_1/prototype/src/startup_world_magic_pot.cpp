// 魔法壶业务只在已证菜单/页面调用点消费；库存、元素、共同随机及整栈由私有Owner候选提交。
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_reference/world_magic_pot.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Page = ref::WorldScriptPage;
using Error = StartupWorldRuntimeError;
using Action = StartupMagicPotAction;
bool supported(int raw) { return raw >= 41 && raw <= 47; }
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
ref::WorldMagicPotDate date(const State &s) {
    return {s.scene.calendar.year, s.scene.calendar.month, s.scene.calendar.subperiod};
}
const ref::WorldMagicPotRecipeDefinition *recipe(const State &s, int id) {
    if (!s.rules) return nullptr;
    const auto p = std::find_if(s.rules->magic_pot_recipes.begin(), s.rules->magic_pot_recipes.end(),
                              [id](const auto &v) { return v.identity == id; });
    return p == s.rules->magic_pot_recipes.end() ? nullptr : &*p;
}
const StartupWorldItem *item(const State &s, int id) {
    if (!s.rules) return nullptr;
    const auto p = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                              [id](const auto &v) { return v.identity == id; });
    return p == s.rules->items.end() ? nullptr : &*p;
}
bool item_state(const State &s, int id) {
    const auto owned = s.items.find(id);
    const auto catalog = s.catalog.find({0, id});
    const auto stock = s.shop_item_stock.find(id);
    return item(s, id) && owned != s.items.end() && catalog != s.catalog.end() &&
           stock != s.shop_item_stock.end() && owned->second.inventory >= 0 &&
           owned->second.inventory <= 999 && owned->second.inventory == catalog->second.inventory &&
           owned->second.status == catalog->second.status &&
           owned->second.unlock_counter == catalog->second.unlock_counter &&
           owned->second.newly_unlocked == catalog->second.newly_unlocked;
}
std::optional<int> reward_status(const State &s, const ref::WorldMagicPotRecipeDefinition &r) {
    if (r.reward_kind >= 0 && r.reward_kind <= 3) {
        const auto c = s.catalog.find({r.reward_kind, r.reward_definition});
        if (c == s.catalog.end() || c->second.status < 0 || c->second.status > 2) return {};
        if (r.reward_kind == 0) {
            if (!item_state(s, r.reward_definition)) return {};
        } else if (!s.rules || !std::any_of(s.rules->equipment.begin(), s.rules->equipment.end(),
                   [&](const auto &d) { return d.shop.kind == r.reward_kind && d.shop.id == r.reward_definition; }))
            return {};
        return c->second.status;
    }
    if (r.reward_kind != 4 || !s.rules) return {};
    const auto c = s.facility_presence.find(r.reward_definition);
    if (c == s.facility_presence.end() || c->second < 0 || c->second > 2 ||
        !std::any_of(s.rules->facilities.begin(), s.rules->facilities.end(),
                     [&](const auto &d) { return d.id == r.reward_definition; })) return {};
    return c->second;
}
std::optional<std::string> reward_name(const State &s, const ref::WorldMagicPotRecipeDefinition &r) {
    if (!s.rules) return {};
    if (r.reward_kind == 0) {
        const auto d = item(s, r.reward_definition);
        return d ? std::optional<std::string>(d->name) : std::nullopt;
    }
    if (r.reward_kind >= 1 && r.reward_kind <= 3) {
        const auto d = std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
            [&](const auto &v) { return v.shop.kind == r.reward_kind && v.shop.id == r.reward_definition; });
        return d == s.rules->equipment.end() ? std::nullopt : std::optional<std::string>(d->name);
    }
    if (r.reward_kind == 4) {
        const auto d = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
            [&](const auto &v) { return v.id == r.reward_definition; });
        return d == s.rules->facilities.end() ? std::nullopt : std::optional<std::string>(d->name);
    }
    return {};
}
bool definitions(const State &s) {
    if (!s.rules || s.magic_pot_recipes.size() != s.rules->magic_pot_recipes.size()) return false;
    std::vector<ref::WorldMagicPotRecipeProgress> progress;
    for (const auto &d : s.rules->magic_pot_recipes) {
        const auto p = s.magic_pot_recipes.find(d.identity);
        if (p == s.magic_pot_recipes.end() || p->second.identity != d.identity || !reward_status(s, d))
            return false;
        progress.push_back(p->second);
    }
    // 同时核13槽、身份唯一性、配方成本/进度，不产生页、更新或随机。
    return ref::discoverable_world_magic_pot_recipes(s.legacy_n, s.rules->magic_pot_recipes, progress).identities.has_value() &&
           ref::valid_world_magic_pot_state(s.legacy_n, date(s));
}
std::optional<std::vector<int>> item_list(const State &s) {
    if (!s.rules) return {};
    std::vector<int> list;
    std::set<int> ids;
    for (const auto &d : s.rules->items) {
        if (!ids.insert(d.identity).second || !item_state(s, d.identity)) return {};
        if (s.items.find(d.identity)->second.inventory > 0) list.push_back(d.identity);
    }
    return list;
}
bool event(State &s, int id, std::optional<std::string> replacement = {}) {
    const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                    startup_world_runtime_scripts(s), {id, std::move(replacement), {}});
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
bool seen(const State &s, int id) { return ref::world_script_seen(s.scripts, id); }
bool close(State &s, std::uint64_t id) {
    const auto source = startup_world_runtime_scripts(s);
    if (ref::validate_world_script_state(startup_world_runtime_catalog(), source) != ref::WorldScriptError::none)
        return false;
    const auto r = ref::prepare_world_script_close_page(source, id);
    return r.candidate &&
           ref::validate_world_script_state(startup_world_runtime_catalog(), r.candidate->state) == ref::WorldScriptError::none &&
           write_startup_world_runtime_scripts(s, r.candidate->state);
}
std::optional<std::uint64_t> open(State &s, int raw, int binding = -1,
                                  std::optional<std::uint64_t> parent = {}) {
    Page p;
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = raw;
    p.legacy_g = binding;
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(s), p);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, r.candidate->state)) return {};
    const auto id = r.candidate->inserted_pages.front().id;
    if (parent) s.magic_pot_page_parents.emplace(id, *parent);
    return id;
}
bool payload(const State &s, const Page &p);
bool parent_valid(const State &s, const Page &p) {
    int raw = p.legacy_page == 42 || p.legacy_page == 43 ? 41
              : p.legacy_page == 44 ? 42 : p.legacy_page == 47 ? 43 : -1;
    const auto parent = s.magic_pot_page_parents.find(p.id);
    if (raw == -1) return parent == s.magic_pot_page_parents.end();
    if (parent == s.magic_pot_page_parents.end() || parent->second == p.id) return false;
    const auto father = page(s, parent->second);
    if (!father || father->legacy_page != raw || father->kind != ref::WorldScriptPageKind::raw_page ||
        !payload(s, *father))
        return false;
    if (p.legacy_page == 47) {
        const auto d = s.magic_pot_page_data.find(father->id);
        const auto list = s.magic_pot_page_lists.find(father->id);
        if (d == s.magic_pot_page_data.end() || list == s.magic_pot_page_lists.end() ||
            d->second[0] < 0 || static_cast<std::size_t>(d->second[0]) >= list->second.size() ||
            list->second[static_cast<std::size_t>(d->second[0])] != p.legacy_g) return false;
    }
    // 锚点原父页必须是紧邻下层的存活页，不能任意同类页替代。
    const Page *previous = nullptr;
    for (const auto &entry : s.scripts.pages) {
        if (entry.lifecycle == 4) continue;
        if (entry.id == p.id) return previous && previous->id == father->id;
        previous = &entry;
    }
    return false;
}
void scroll(std::array<int, 3> &v, int count) {
    if (count == 0) { v[0] = v[1] = 0; return; }
    if (v[0] >= count) v[0] = count - 1;
    if (v[1] > v[0]) v[1] = v[0];
    if (v[1] + 5 <= v[0]) v[1] = v[0] - 4;
}
bool payload(const State &s, const Page &p) {
    const auto d = s.magic_pot_page_data.find(p.id);
    const auto list = s.magic_pot_page_lists.find(p.id);
    const auto count = s.page_counters.find(p.id), phase = s.page_phases.find(p.id);
    if (!definitions(s) || !s.magic_pot_pages_initialized.count(p.id) ||
        d == s.magic_pot_page_data.end() || list == s.magic_pot_page_lists.end() ||
        count == s.page_counters.end() || phase == s.page_phases.end() ||
        count->second < 0 || phase->second < 0 || !parent_valid(s, p)) return false;
    const auto &v = d->second;
    if (v[0] < 0 || v[1] < 0 || list->second.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    if (p.legacy_page == 41)
        return v[0] < 2 && v[1] == 0 && v[2] == -1 && phase->second == 0 && list->second.empty();
    if (p.legacy_page == 42 || p.legacy_page == 43) {
        const auto expected = p.legacy_page == 42 ? item_list(s)
            : ref::catalogue_world_magic_pot_recipes(s.rules->magic_pot_recipes).identities;
        if (!expected || *expected != list->second || v[2] != -1 || phase->second > (p.legacy_page == 43 ? 1 : 0))
            return false;
        const int size = static_cast<int>(list->second.size());
        return size == 0 ? v[0] == 0 && v[1] == 0
                         : v[0] < size && v[1] <= v[0] && v[0] - v[1] < 5;
    }
    if (!list->second.empty() || v[0] != 0 || v[1] != 0 || phase->second != 0) return false;
    if (p.legacy_page == 45) return v[2] == -1;
    if (v[2] != p.legacy_g) return false;
    if (p.legacy_page == 44) return item_state(s, v[2]);
    const auto r = s.magic_pot_recipes.find(v[2]);
    return recipe(s, v[2]) != nullptr && r != s.magic_pot_recipes.end() &&
           (p.legacy_page != 47 || r->second.status != 0);
}
bool initialize(State &s, const Page &p) {
    if (s.magic_pot_pages_initialized.count(p.id)) return payload(s, p);
    if (!definitions(s) || !parent_valid(s, p) ||
        s.magic_pot_page_data.count(p.id) || s.magic_pot_page_lists.count(p.id)) return false;
    std::vector<int> list;
    if (p.legacy_page == 42) {
        const auto entries = item_list(s); if (!entries) return false; list = *entries;
    } else if (p.legacy_page == 43) {
        const auto entries = ref::catalogue_world_magic_pot_recipes(s.rules->magic_pot_recipes);
        if (!entries.identities || entries.identities->empty()) return false;
        list = *entries.identities;
    }
    const int binding = p.legacy_page == 44 || p.legacy_page >= 46 ? p.legacy_g : -1;
    s.magic_pot_page_data.emplace(p.id, std::array<int, 3>{0, 0, binding});
    s.magic_pot_page_lists.emplace(p.id, std::move(list));
    s.page_counters.try_emplace(p.id, 0);
    s.page_phases.try_emplace(p.id, 0);
    s.magic_pot_pages_initialized.insert(p.id);
    if (!payload(s, p)) return false;
    if (p.legacy_page == 41 && !seen(s, 101) && !event(s, 101)) return false;
    if (p.legacy_page == 42 && s.magic_pot_page_lists.find(p.id)->second.empty())
        return event(s, 15) && close(s, p.id); // 原初始化空池先15，不等下一42更新。
    return true;
}
bool process(State &s) {
    const auto r = ref::prepare_world_magic_pot_processing(s.legacy_n, date(s));
    const auto time = ref::world_magic_pot_date(date(s));
    if (!r.candidate || !time.value) return false;
    if (*time.value != s.legacy_n[12]) s.magic_pot_output = r.candidate->produced;
    if (!r.candidate->changed) return true;
    s.legacy_n = r.candidate->state;
    s.magic_pot_display = r.candidate->display;
    if (!open(s, 45)) return false;
    std::vector<ref::WorldMagicPotRecipeProgress> progress;
    for (const auto &d : s.rules->magic_pot_recipes) progress.push_back(s.magic_pot_recipes.find(d.identity)->second);
    const auto found = ref::discoverable_world_magic_pot_recipes(s.legacy_n, s.rules->magic_pot_recipes, progress);
    if (!found.identities) return false;
    for (int id : *found.identities) if (!open(s, 46, id)) return false;
    if (s.legacy_n[0] >= 100) {
        if (!seen(s, 103) && !event(s, 103)) return false;
    } else if (s.legacy_n[0] >= 30 && !seen(s, 102) && !event(s, 102)) return false;
    return true;
}
bool exit_deposit(State &s, std::uint64_t id) {
    return (s.legacy_n[1] <= 0 || seen(s, 105) || event(s, 105)) && close(s, id);
}
bool grant(State &s, int kind, int id) {
    // raw47只调用定义低层c()/b()/a()；c.e.a的全局通知、110及E/151不属于本路径。
    const auto entry = s.catalog.find({kind, id});
    if (kind < 0 || kind > 3 || entry == s.catalog.end() ||
        entry->second.status < 0 || entry->second.status > 2 ||
        entry->second.unlock_counter < 0 || entry->second.free_purchases < 0) return false;
    auto &d = entry->second;
    if (kind == 0) {
        if (!item_state(s, id)) return false;
        d.inventory = std::min(d.inventory + 1, 999);
        if (d.status == 0) {
            d.newly_unlocked = true;
            d.status = 1;
            d.unlock_counter = 0;
        }
        s.items.find(id)->second = d;
        auto &stock = s.shop_item_stock.find(id)->second;
        stock.presence = d.status;
        stock.legacy_q = d.unlock_counter;
        stock.newly_available = d.newly_unlocked;
        return true;
    }
    if (!s.rules || !std::any_of(s.rules->equipment.begin(), s.rules->equipment.end(),
        [=](const auto &v) { return v.shop.kind == kind && v.shop.id == id; })) return false;
    if (d.status == 0) {
        d.newly_unlocked = true;
        d.free_purchases = 1;
        if (kind == 1 && (d.flags & 32U) && !seen(s, 216) && !event(s, 216)) return false;
        // a.o.g(0/1/2)按g实例顺序给category1/4/5加入{7,0}，c.m.c按种类去重。
        if (s.shop_order.size() != s.shops.size()) return false;
        const int category = kind == 1 ? 1 : kind == 2 ? 4 : 5;
        std::set<std::uint64_t> visited;
        for (const auto shop_id : s.shop_order) {
            const auto shop = s.shops.find(shop_id);
            const auto detail = s.facility_details.find(shop_id);
            if (!visited.insert(shop_id).second || shop == s.shops.end() ||
                detail == s.facility_details.end()) return false;
            if (shop->second.category != category) continue;
            auto &notices = detail->second.notices;
            if (!std::any_of(notices.begin(), notices.end(), [](const auto &n) { return n[0] == 7; }))
                notices.push_back({7, 0});
        }
    }
    d.status = 1;
    return true;
}
} // namespace

bool valid_startup_world_magic_pot_page(const State &s, const Page &p) {
    if (!supported(p.legacy_page) || p.kind != ref::WorldScriptPageKind::raw_page ||
        p.lifecycle < 0 || p.lifecycle >= 4) return false;
    if (s.magic_pot_pages_initialized.count(p.id)) return payload(s, p);
    if (p.lifecycle != 0 || !definitions(s) || !parent_valid(s, p) ||
        s.magic_pot_page_data.count(p.id) || s.magic_pot_page_lists.count(p.id)) return false;
    const auto count = s.page_counters.find(p.id), phase = s.page_phases.find(p.id);
    if ((count != s.page_counters.end() && count->second != 0) ||
        (phase != s.page_phases.end() && phase->second != 0)) return false;
    if (p.legacy_page == 44) return item_state(s, p.legacy_g);
    if (p.legacy_page == 46) return recipe(s, p.legacy_g) != nullptr;
    if (p.legacy_page == 47) {
        const auto r = s.magic_pot_recipes.find(p.legacy_g);
        return recipe(s, p.legacy_g) != nullptr && r != s.magic_pot_recipes.end() && r->second.status == 1;
    }
    return true;
}
bool initialize_startup_world_magic_pot_pages(State &s) {
    if (s.scene.framework_paused) return true;
    const auto active = top(s);
    if (!active || active->kind != ref::WorldScriptPageKind::raw_page || !supported(active->legacy_page)) return true;
    if (s.magic_pot_pages_initialized.count(active->id)) return payload(s, *active);
    const Page p = *active;
    auto next = s;
    next.scripts.executing_page = p.id;
    if (!initialize(next, p)) return false;
    next.scripts.executing_page = s.scripts.executing_page;
    s = std::move(next); return true;
}
std::optional<StartupMagicPotView> inspect_startup_world_magic_pot_page(const State &s, std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || !supported(p->legacy_page) || !payload(s, *p)) return {};
    const auto &v = s.magic_pot_page_data.find(id)->second;
    return StartupMagicPotView{p->legacy_page, s.page_phases.find(id)->second,
        s.page_counters.find(id)->second, v[0], v[1], v[2], s.magic_pot_page_lists.find(id)->second};
}
std::optional<StartupMagicPotMenuInformation> startup_magic_pot_menu_information(const State &s) {
    const auto time=ref::world_magic_pot_date(date(s));
    if(!time.value || !ref::valid_world_magic_pot_state(s.legacy_n,date(s)))return {};
    const auto &n=s.legacy_n;
    StartupMagicPotMenuInformation result{0,n[1]};
    const int elapsed=*time.value-n[12];
    if(elapsed==0)return result;
    const int processed=std::min(elapsed,n[1]);
    // 已校验待件数不超过原容量30；保留RateConvert与元素除100的两次整数截断。
    const int percentage=n[1]==0?0:processed*100/n[1];
    for(int index=7;index<11;++index) {
        const auto product=static_cast<std::int64_t>(n[index])*percentage;
        if(product>std::numeric_limits<std::int32_t>::max())return {};
        if(product/100>0) {
            result.processed=processed;
            return result; // 原显示查询找到首个正产出即返回，不提前检查未读元素乘积。
        }
    }
    return result;
}
Error open_startup_world_magic_pot(State &s, StartupMagicPotEntry entry) {
    const auto p = top(s);
    if (!p || p->kind != ref::WorldScriptPageKind::scene || s.scene.scene_state != 0 ||
        s.scene.framework_paused || (s.scripts.user_flags & 1U) == 0 ||
        (entry != StartupMagicPotEntry::main_menu && entry != StartupMagicPotEntry::development_menu)) return Error::invalid_page;
    if (!definitions(s)) return Error::missing_source;
    auto next = s; next.scripts.executing_page = p->id;
    if (entry == StartupMagicPotEntry::main_menu) next.scripts.user_flags &= ~2U;
    if (!process(next) || !open(next, 41)) return Error::script_failed;
    next.scripts.executing_page.reset(); s = std::move(next); return Error::none;
}
Error act_startup_world_magic_pot_page(State &s, std::uint64_t id, Action action, int selection) {
    const auto p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page || !supported(p->legacy_page) ||
        s.scene.framework_paused || static_cast<int>(action) < 0 || action > Action::next_tab) return Error::invalid_page;
    if (!payload(s, *p)) return Error::missing_source;
    const Page original = *p;
    auto next = s; next.scripts.executing_page = id;
    auto &v = next.magic_pot_page_data.find(id)->second;
    auto &counter = next.page_counters.find(id)->second;
    auto &list = next.magic_pot_page_lists.find(id)->second;
    const int raw = original.legacy_page;
    if (action == Action::previous || action == Action::next || action == Action::select) {
        const int size = raw == 41 ? 2 : static_cast<int>(list.size());
        if ((raw != 41 && raw != 42 && raw != 43) || size == 0) return Error::invalid_page;
        const int chosen = action == Action::select ? selection : (v[0] + (action == Action::previous ? size - 1 : 1)) % size;
        if (chosen < 0 || chosen >= size) return Error::invalid_page;
        v[0] = chosen; if (raw != 41) scroll(v, size);
    } else if (action == Action::previous_tab || action == Action::next_tab) {
        if (raw != 43) return Error::invalid_page;
        auto &phase = next.page_phases.find(id)->second; phase = 1 - phase;
    } else if (action == Action::cancel) {
        if (raw == 45) return Error::invalid_page;
        if (!(raw == 42 ? exit_deposit(next, id) : close(next, id))) return Error::script_failed;
    } else if (action == Action::confirm) {
        if (raw == 41) {
            if (v[0] == 0) {
                const auto capacity = ref::world_magic_pot_capacity(next.legacy_n[11]);
                if (!capacity) return Error::missing_source;
                if (next.legacy_n[1] >= *capacity) { if (!event(next, 19)) return Error::script_failed; }
                else if (!open(next, 42, -1, id)) return Error::script_failed;
            } else if (!open(next, 43, -1, id)) return Error::script_failed;
        } else if (raw == 42) {
            if (list.empty() || next.legacy_n[1] >= *ref::world_magic_pot_capacity(next.legacy_n[11])) {
                if (!exit_deposit(next, id)) return Error::script_failed;
            } else {
                const int chosen = list[static_cast<std::size_t>(v[0])];
                const auto d = item(next, chosen);
                if (!d) return Error::missing_source;
                const ref::WorldMagicPotItem input{chosen, d->magic_pot_elements};
                const auto comment = ref::world_magic_pot_comment(input);
                if (!comment.candidate) return Error::missing_source;
                const auto ticket = next.scene.random.draw(comment.candidate->bound);
                if (ticket.error != ref::WorldRandomError::none) return Error::runtime_failed;
                const auto result = ref::prepare_world_magic_pot_deposit(next.legacy_n, input,
                    next.items.find(chosen)->second.inventory, date(next), ticket.ticket);
                if (!result.candidate) return Error::missing_source;
                next.legacy_n = result.candidate->state;
                next.items.find(chosen)->second.inventory = result.candidate->inventory;
                next.catalog.find({0, chosen})->second = next.items.find(chosen)->second;
                for (int row = 0; row < 3; ++row) for (int col = 0; col < 4; ++col)
                    next.magic_pot_display[row][col] = result.candidate->display[row][col];
                static const std::vector<std::vector<std::string>> comments{
                    {"相性似乎不错", "上了", "成功了", "感觉不错"},
                    {"上了", "感觉不错", "就那样吧"}, {"嗯", "就那样吧"}};
                next.magic_pot_comment = comments[static_cast<std::size_t>(result.candidate->comment_group)][static_cast<std::size_t>(result.candidate->comment_ticket)];
                const auto fresh = item_list(next); if (!fresh) return Error::missing_source;
                list = *fresh; scroll(v, static_cast<int>(list.size()));
                if (!open(next, 44, chosen, id)) return Error::script_failed;
            }
        } else if (raw == 43) {
            if (list.empty()) return Error::missing_source;
            const int chosen = list[static_cast<std::size_t>(v[0])];
            const auto d = recipe(next, chosen); if (!d) return Error::missing_source;
            const auto status = reward_status(next, *d); if (!status) return Error::missing_source;
            const auto gate = ref::check_world_magic_pot_recipe_selection(*d, next.magic_pot_recipes.find(chosen)->second, *status);
            if (gate.error != ref::WorldMagicPotError::none) return Error::missing_source;
            if (gate.denial == ref::WorldMagicPotDenial::already_owned106) {
                const auto name = reward_name(next, *d);
                if (!name) return Error::missing_source;
                if (!event(next, 106, *name)) return Error::script_failed;
            } else if (gate.denial == ref::WorldMagicPotDenial::none && !open(next, 47, chosen, id)) return Error::script_failed;
        } else if (raw == 44) {
            if (counter >= 99) { if (!close(next, id)) return Error::script_failed; }
            else if (counter >= 36) counter = 36 + (counter - 36 < 49 ? 49 : std::max(counter - 36, 55));
        } else if (raw == 45) {
            if (counter < 77) counter = 77;
            else if (counter >= 83 && !close(next, id)) return Error::script_failed;
        } else if (counter > 6) {
            const auto d = recipe(next, v[2]); if (!d) return Error::missing_source;
            if (raw == 46) {
                const auto r = ref::prepare_world_magic_pot_discovery(next.magic_pot_recipes.find(v[2])->second);
                if (!r.candidate) return Error::missing_source;
                next.magic_pot_recipes.find(v[2])->second = *r.candidate;
            } else {
                const auto r = ref::prepare_world_magic_pot_recipe_cost(next.legacy_n, *d);
                if (r.error != ref::WorldMagicPotError::none) return Error::missing_source;
                if (r.candidate) {
                    next.legacy_n = r.candidate->state;
                    if (!event(next, 107, d->name)) return Error::script_failed;
                    if (d->reward_kind == 4) {
                        Page gift; gift.kind = ref::WorldScriptPageKind::raw_page;
                        gift.legacy_page = 93; gift.legacy_r = 3; gift.legacy_s = d->reward_definition;
                        const auto given = ref::prepare_world_script_page(startup_world_runtime_scripts(next), gift);
                        if (!given.candidate || given.candidate->inserted_pages.size() != 1 ||
                            !write_startup_world_runtime_scripts(next, given.candidate->state)) return Error::script_failed;
                    } else if (!grant(next, d->reward_kind, d->reward_definition)) return Error::script_failed;
                }
            }
            if (!close(next, id)) return Error::script_failed;
        }
    } else return Error::invalid_page;
    next.scripts.executing_page.reset(); s = std::move(next); return Error::none;
}
std::optional<State> update_startup_world_magic_pot_page(const State &s, std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || !supported(p->legacy_page)) return {};
    if (s.scene.framework_paused) return s;
    const Page original = *p;
    auto next = s; next.scripts.executing_page = id;
    if (!initialize(next, original)) return {};
    const auto current = page(next, id);
    if (current) {
        auto &count = next.page_counters.find(id)->second;
        if (count == std::numeric_limits<int>::max()) return {};
        ++count;
        if (original.legacy_page == 42) {
            const auto &list = next.magic_pot_page_lists.find(id)->second;
            if ((list.empty() || next.legacy_n[1] >= *ref::world_magic_pot_capacity(next.legacy_n[11])) &&
                !exit_deposit(next, id)) return {};
        } else if (original.legacy_page == 44 || original.legacy_page == 45) {
            const int start = original.legacy_page == 44 ? 38 : 12;
            const int end = original.legacy_page == 44 ? 68 : 52;
            const int columns = original.legacy_page == 44 ? 4 : 5;
            if (count >= start && count < end) {
                const int age = count - start;
                bool active{};
                for (int col = 0; col < columns; ++col)
                    if (age >= col * 6 && age < col * 6 + 12 && next.magic_pot_display[2][col] != 0) active = true;
                if (!active) count += 6;
            }
        }
    }
    next.scripts.executing_page.reset(); return next;
}
bool apply_startup_world_magic_pot_activity(State &s, int kind) {
    if (!definitions(s)) return false;
    if (kind == 5) { s.legacy_n[11] = std::min(3, s.legacy_n[11] + 1); return true; }
    if (kind != 6) return false;
    s.scripts.user_flags |= 3U;
    return event(s, 104) && (seen(s, 219) || event(s, 219));
}
} // namespace dungeon_village_prototype
