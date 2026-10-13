// 商会购买即时提交；设施兑换先付村点，93满40确认再领取，不混同金币建设费。
#include "ark/simulation/facilities/startup_world_commerce.hpp"
#include "ark/simulation/world/startup_world_menu.hpp"
#include "ark/simulation/facilities/startup_world_building.hpp"

#include <algorithm>
#include <limits>
#include <set>

namespace ark::simulation {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
using Action = StartupCommerceAction;
using Page = ref::WorldScriptPage;
bool supported(int raw) { return raw == 83 || raw == 84 || raw == 85 || raw == 86 || raw == 93; }
const Page *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &v) { return v.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
const StartupWorldItem *item(const State &s, int id) {
    if (!s.rules)
        return nullptr;
    const auto p = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                [&](const auto &v) { return v.identity == id; });
    return p == s.rules->items.end() ? nullptr : &*p;
}
const StartupDefinition *facility(const State &s, int id) {
    if (!s.rules)
        return nullptr;
    const auto p = std::find_if(s.rules->facilities.begin(), s.rules->facilities.end(),
                                [&](const auto &v) { return v.id == id; });
    return p == s.rules->facilities.end() ? nullptr : &*p;
}
bool item_state(const State &s, int id) {
    const auto d = item(s, id);
    const auto p = s.items.find(id);
    const auto c = s.catalog.find({0, id});
    const auto stock = s.shop_item_stock.find(id);
    return d && d->commerce_price >= 0 && p != s.items.end() && c != s.catalog.end() &&
           stock != s.shop_item_stock.end() && s.item_commerce_read.count(id) &&
           p->second.inventory >= 0 && p->second.inventory <= 999 && stock->second.quantity >= 0 &&
           c->second.inventory == p->second.inventory && c->second.status == p->second.status &&
           c->second.unlock_counter == p->second.unlock_counter &&
           c->second.newly_unlocked == p->second.newly_unlocked;
}
bool facility_state(const State &s, int id) {
    return facility(s, id) && id >= 0 &&
           static_cast<std::size_t>(id) < s.rules->facility_initial.size() &&
           s.rules->facility_initial[id].capacity >= 0 && s.facility_presence.count(id) &&
           s.facility_commerce_read.count(id) && s.facility_free_builds.count(id) &&
           s.facility_free_builds.at(id) >= 0 && s.facility_free_builds.at(id) <= 99 &&
           s.facility_unlock_counters.count(id) && s.facility_unlock_notices.count(id);
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
std::optional<std::uint64_t> open(State &s, int raw, int mode = 0, int binding = -1) {
    Page p;
    p.kind = ref::WorldScriptPageKind::raw_page;
    p.legacy_page = raw;
    p.legacy_f = mode;
    p.legacy_s = binding;
    if (raw == 93)
        p.legacy_r = 3;
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(s), p);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, r.candidate->state))
        return {};
    return r.candidate->inserted_pages.front().id;
}
bool payload(const State &s, const Page &p) {
    const auto d = s.commerce_page_data.find(p.id);
    const auto counter = s.page_counters.find(p.id);
    if (!s.rules || !supported(p.legacy_page) || !s.commerce_pages_initialized.count(p.id) ||
        d == s.commerce_page_data.end() || counter == s.page_counters.end() || counter->second < 0)
        return false;
    const auto &v = d->second;
    if (v[0] < 0 || v[0] > 1 || v[1] < 0 || v[1] > 1 || v[2] < 0 || v[3] < 0 || v[5] < 0 ||
        v[5] > 20)
        return false;
    const int raw = p.legacy_page;
    if ((raw != 84 && raw != 86 && v[0] != 0) || ((raw != 84 || v[0] == 1) && v[1] != 0) ||
        (raw != 84 && v[5] != 0))
        return false;
    if (raw == 83)
        return v[2] < 3 && v[3] == 0 && v[4] == -1;
    if (raw == 86)
        return v[2] == 0 && v[3] == 0 && v[4] == p.legacy_s && v[0] == p.legacy_f &&
               item_state(s, v[4]);
    if (raw == 93)
        return v[2] == 0 && v[3] == 0 && v[4] == p.legacy_s && p.legacy_r == 3 &&
               facility_state(s, v[4]) &&
               (s.facility_presence.at(v[4]) == 0 ||
                (p.lifecycle == 4 && s.facility_presence.at(v[4]) == 2));
    const auto list = s.commerce_page_lists.find(p.id);
    if (list == s.commerce_page_lists.end() || list->second.empty() || v[4] != -1 ||
        list->second.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    const int rows = raw == 85 ? 3 : 5;
    const int count = static_cast<int>(list->second.size());
    if (v[2] >= count || v[3] > v[2] || v[2] - v[3] >= rows || v[3] > std::max(0, count - rows) ||
        (raw == 84 && v[0] != p.legacy_f))
        return false;
    std::set<int> seen;
    for (int id : list->second) {
        if (!seen.insert(id).second)
            return false;
        if (raw == 84) {
            if (!item_state(s, id) ||
                (v[0] == 0 ? s.shop_item_stock.at(id).quantity : s.items.at(id).inventory) <= 0)
                return false;
        } else if (!facility_state(s, id) || s.facility_presence.at(id) != 0 ||
                   facility(s, id)->unlock_rank < 0 || facility(s, id)->unlock_rank > s.rank)
            return false;
    }
    return true;
}
bool initialize(State &s, const Page &p) {
    if(p.legacy_page==93&&!valid_startup_world_commerce_claim_page(s,p))return false;
    if (s.commerce_pages_initialized.count(p.id))
        return payload(s, p);
    if (!s.rules || !supported(p.legacy_page) ||
        ((p.legacy_page == 84 || p.legacy_page == 86) && (p.legacy_f < 0 || p.legacy_f > 1)) ||
        (p.legacy_page == 93 && p.legacy_r != 3))
        return false;
    auto &v = s.commerce_page_data[p.id];
    v = {p.legacy_page == 84 || p.legacy_page == 86 ? p.legacy_f : 0,  0, 0, 0,
         p.legacy_page == 86 || p.legacy_page == 93 ? p.legacy_s : -1, 0};
    s.page_counters.try_emplace(p.id, 0);
    s.commerce_pages_initialized.insert(p.id);
    if (p.legacy_page == 84) {
        auto &list = s.commerce_page_lists[p.id];
        for (const auto &d : s.rules->items) {
            if (!item_state(s, d.identity))
                return false;
            if ((v[0] == 0 ? s.shop_item_stock.at(d.identity).quantity
                           : s.items.at(d.identity).inventory) > 0)
                list.push_back(d.identity);
        }
        if (list.empty())
            return event(s, v[0] == 0 ? 13 : 15) && close(s, p.id);
    } else if (p.legacy_page == 85) {
        auto &list = s.commerce_page_lists[p.id];
        for (const auto &d : s.rules->facilities) {
            if (!facility_state(s, d.id))
                return false;
            // 用户确认的未开放图纸策略；原APK/Steam85为p!=2，新局原初值已另修为p2。
            if (s.facility_presence.at(d.id) == 0 && d.unlock_rank != -1 && d.unlock_rank <= s.rank)
                list.push_back(d.id);
        }
        if (list.empty())
            return event(s, 13) && close(s, p.id);
    }
    if(p.legacy_page==93)for(auto &page:s.scripts.pages)if(page.id==p.id&&page.lifecycle==0)page.lifecycle=1;
    return payload(s, p);
}
void scroll(std::array<int, 6> &v, int count, int rows) {
    if (v[2] >= count)
        v[2] = 0;
    v[3] = std::min(v[3], std::max(0, count - rows));
    if (v[3] > v[2])
        v[3] = v[2];
    if (v[3] + rows <= v[2])
        v[3] = v[2] - rows + 1;
}
bool cash(State &s, int amount, bool sale) {
    const int raw_month = s.scene.calendar.month;
    if (amount < 0 || raw_month < 0 || raw_month >= 12)
        return false;
    auto &ai = s.scene.world.world.ai;
    auto &month = s.monthly_cash[raw_month][3][sale ? 0 : 1];
    if (month < 0 || amount > std::numeric_limits<int>::max() - month ||
        (!sale && ai.accounting.funds() < amount) ||
        ai.next_cash_id == std::numeric_limits<std::uint64_t>::max() ||
        s.simulation_steps == std::numeric_limits<std::uint64_t>::max())
        return false;
    if (ai.accounting.post_cash({ai.next_cash_id++, s.simulation_steps + 1, ref::CashCategory::shop,
                                 sale ? ref::CashDirection::income : ref::CashDirection::expense,
                                 amount}) != ref::AccountingError::none)
        return false;
    month += amount;
    if (sale && s.completion_mode == 0 && ai.accounting.funds() > s.cash_peak) {
        s.cash_peak = ai.accounting.funds();
        s.cash_peak_village = s.scripts.village_name;
    }
    return true;
}
// 仅由已验证85载荷的Owner候选调用；旧单动作和整轮输入共享付款、已读及插页顺序。
Error facility_action(State &s, std::uint64_t id, Action action) {
    const auto &list = s.commerce_page_lists.at(id);
    if (action == Action::inspect)
        return open_startup_world_facility_definition(s, list[s.commerce_page_data.at(id)[2]]);
    if (action == Action::cancel) {
        for (int entry : list)
            s.facility_commerce_read.at(entry) = true;
        return close(s, id) ? Error::none : Error::script_failed;
    }
    if (action != Action::confirm)
        return Error::invalid_page;
    const int entry = list[s.commerce_page_data.at(id)[2]];
    const int price = s.rules->facility_initial[entry].capacity;
    if (s.village_points < price)
        return event(s, 12) ? Error::none : Error::script_failed;
    s.village_points = std::clamp(s.village_points - price, 0, 999);
    if (!open(s, 93, 0, entry) || !close(s, id))
        return Error::script_failed;
    for (int known : list)
        s.facility_commerce_read.at(known) = true;
    return Error::none;
}
} // namespace

bool valid_startup_world_commerce_claim_page(const State &s,const Page &p) {
    if(!s.rules||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=93||p.lifecycle<0||p.lifecycle>4||
       p.legacy_r!=3||!facility_state(s,p.legacy_s)||s.page_phases.count(p.id)||
       s.page_secondary_counters.count(p.id)||s.commerce_page_lists.count(p.id))return false;
    if(!s.commerce_pages_initialized.count(p.id))return p.lifecycle==0&&
        !s.commerce_page_data.count(p.id)&&!s.page_counters.count(p.id)&&s.facility_presence.at(p.legacy_s)==0;
    const auto count=s.page_counters.find(p.id);
    return p.lifecycle!=0&&count!=s.page_counters.end()&&count->second<std::numeric_limits<int>::max()&&payload(s,p);
}

Error open_startup_world_commerce(State &s) {
    const auto p = top(s);
    const bool menu = startup_world_menu_callback(s, 7);
    if (!s.rules || !p || (!menu && p->kind != ref::WorldScriptPageKind::scene) || s.scene.scene_state != 0 ||
        s.scene.framework_paused || (s.scripts.user_flags & 16U) == 0)
        return Error::invalid_page;
    auto next = s;
    next.scripts.executing_page = menu ? s.scripts.executing_page : std::optional<std::uint64_t>{p->id};
    if (!ref::world_script_seen(next.scripts, 97) && !event(next, 97))
        return Error::script_failed;
    if (!open(next, 83))
        return Error::script_failed;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
bool initialize_startup_world_commerce_pages(State &s) {
    if (s.scene.framework_paused)
        return true;
    std::vector<Page> pending;
    for (const auto &p : s.scripts.pages)
        if (p.lifecycle != 4 && p.kind == ref::WorldScriptPageKind::raw_page &&
            supported(p.legacy_page))
            pending.push_back(p);
    if (pending.empty())
        return true;
    auto next = s;
    const auto executing = next.scripts.executing_page;
    for (const auto &p : pending) {
        next.scripts.executing_page = p.id;
        if (!initialize(next, p))
            return false;
    }
    next.scripts.executing_page = executing;
    s = std::move(next);
    return true;
}
std::optional<StartupCommerceView> inspect_startup_world_commerce_page(const State &s,
                                                                       std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page || !payload(s, *p))
        return {};
    if(p->legacy_page==93&&!valid_startup_world_commerce_claim_page(s,*p))return {};
    const auto &v = s.commerce_page_data.at(id);
    StartupCommerceView r{p->legacy_page,         v[0], v[1], v[2], v[3], v[4], v[5],
                          s.page_counters.at(id), {}};
    if (s.commerce_page_lists.count(id))
        r.entries = s.commerce_page_lists.at(id);
    return r;
}
Error act_startup_world_commerce_page(State &s, std::uint64_t id, Action action, int selection) {
    const auto p = top(s);
    if (static_cast<int>(action) < static_cast<int>(Action::confirm) ||
        static_cast<int>(action) > static_cast<int>(Action::inspect) || s.scene.framework_paused ||
        !p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page ||
        !supported(p->legacy_page))
        return Error::invalid_page;
    // 原83取消只关闭已有页；尚未初始化的脚本页也无需购买目录或新页创建资格。
    // 已初始化页仍走完整载荷验证，不能用取消掩盖缺字段。
    if (p->legacy_page == 83 && action == Action::cancel &&
        !s.commerce_pages_initialized.count(id)) {
        if (s.commerce_page_data.count(id) || s.commerce_page_lists.count(id))
            return Error::missing_source;
        auto next = s;
        next.scripts.executing_page = id;
        if (!close(next, id))
            return Error::script_failed;
        next.scripts.executing_page.reset();
        s = std::move(next);
        return Error::none;
    }
    if (!payload(s, *p)||(p->legacy_page==93&&!valid_startup_world_commerce_claim_page(s,*p)))
        return Error::missing_source;
    const int raw = p->legacy_page;
    auto next = s;
    next.scripts.executing_page = id;
    auto &v = next.commerce_page_data.at(id);
    if (action == Action::previous || action == Action::next || action == Action::select) {
        if (raw != 83 && raw != 84 && raw != 85)
            return Error::invalid_page;
        const int count = raw == 83 ? 3 : static_cast<int>(next.commerce_page_lists.at(id).size());
        const int chosen = action == Action::select
                               ? selection
                               : (v[2] + (action == Action::previous ? count - 1 : 1)) % count;
        if (chosen < 0 || chosen >= count)
            return Error::invalid_page;
        v[2] = chosen;
        if (raw != 83)
            scroll(v, count, raw == 85 ? 3 : 5);
    } else if (action == Action::previous_tab || action == Action::next_tab) {
        if (raw != 84 || v[0] != 0)
            return Error::invalid_page;
        v[1] = 1 - v[1];
    } else if (action == Action::inspect) {
        if (raw != 85)
            return Error::invalid_page;
        const auto result = facility_action(next, id, action);
        if (result != Error::none)
            return result;
    } else if (action == Action::cancel) {
        if (raw != 83 && raw != 84 && raw != 85)
            return Error::invalid_page;
        if (raw == 85) {
            const auto result = facility_action(next, id, action);
            if (result != Error::none)
                return result;
        } else {
            if (raw == 84)
                for (int entry : next.commerce_page_lists.at(id))
                    next.item_commerce_read.at(entry) = true;
            if (!close(next, id))
                return Error::script_failed;
        }
    } else if (action != Action::confirm)
        return Error::invalid_page;
    else if (raw == 83) {
        if (!open(next, v[2] == 2 ? 85 : 84, v[2] == 1 ? 1 : 0))
            return Error::script_failed;
    } else if (raw == 84) {
        if (v[5] > 0)
            v[5] = 0;
        else {
            auto &list = next.commerce_page_lists.at(id);
            const int entry = list[v[2]];
            const bool sale = v[0] == 1;
            const int price = item(next, entry)->commerce_price / (sale ? 2 : 1);
            if (!sale && next.scene.world.world.ai.accounting.funds() < price) {
                if (!event(next, 11))
                    return Error::script_failed;
            } else {
                if (!cash(next, price, sale))
                    return Error::missing_source;
                auto &owned = next.items.at(entry);
                auto &stock = next.shop_item_stock.at(entry);
                if (sale)
                    --owned.inventory;
                else {
                    --stock.quantity;
                    owned.inventory = std::min(999, owned.inventory + 1);
                    if (owned.status == 0) {
                        owned.status = 1;
                        owned.unlock_counter = 0;
                        owned.newly_unlocked = true;
                    }
                }
                next.catalog.at({0, entry}) = owned;
                stock.presence = owned.status;
                stock.legacy_q = owned.unlock_counter;
                stock.newly_available = owned.newly_unlocked;
                if ((sale ? owned.inventory : stock.quantity) == 0)
                    list.erase(list.begin() + v[2]);
                scroll(v, static_cast<int>(list.size()), 5);
                if (!open(next, 86, v[0], entry))
                    return Error::script_failed;
                next.sound_requests.push_back({StartupAudioOperation::ordinary_play, 25}); // 固定APK d/a.B初值0，A[0]。
                if (list.empty()) {
                    if (!event(next, sale ? 14 : 13) || !close(next, id))
                        return Error::script_failed;
                } else
                    v[5] = 20;
            }
        }
    } else if (raw == 85) {
        const auto result = facility_action(next, id, action);
        if (result != Error::none)
            return result;
    } else if (raw == 93) {
        auto &counter = next.page_counters.at(id);
        if (counter < 40)
            counter = 40;
        else {
            const int entry = v[4];
            auto &free = next.facility_free_builds.at(entry);
            free = std::min(99, free + 1);
            if (next.facility_presence.at(entry) == 0) {
                next.facility_presence.at(entry) = 2;
                next.facility_unlock_counters.at(entry) = 0;
                next.facility_unlock_notices.at(entry) = true;
            }
            if (!close(next, id))
                return Error::script_failed;
        }
    } else if (!close(next, id))
        return Error::script_failed;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
Error input_startup_world_commerce_top(State &s,std::uint64_t id,const StartupCommerceTopInput &input) {
    const auto p=top(s);
    if(s.scene.framework_paused||!p||p->id!=id||
       p->kind!=ref::WorldScriptPageKind::raw_page||p->legacy_page!=83)
        return Error::invalid_page;
    if(!payload(s,*p)||s.commerce_page_lists.count(id))return Error::missing_source;
    if(input.selection&&(*input.selection<0||*input.selection>=3))return Error::invalid_page;
    auto next=s;
    auto &selected=next.commerce_page_data.find(id)->second[2];
    if(input.selection)selected=*input.selection;
    if(input.up)selected=(selected+2)%3;
    if(input.down)selected=(selected+1)%3;
    // 子页创建也在同一候选；插页失败不能留下已改变的父选择。
    if(input.confirm||input.cancel) {
        next.scripts.executing_page=id;
        if(input.confirm) {
            if(!open(next,selected==2?85:84,selected==1?1:0))return Error::script_failed;
        } else if(!close(next,id))return Error::script_failed;
        next.scripts.executing_page.reset();
    }
    s=std::move(next);
    return Error::none;
}
Error input_startup_world_commerce_facility(State &s,std::uint64_t id,const StartupCommerceFacilityInput &input) {
    const auto p=top(s);
    if(s.scene.framework_paused||!p||p->id!=id||
       p->kind!=ref::WorldScriptPageKind::raw_page||p->legacy_page!=85)
        return Error::invalid_page;
    if(!payload(s,*p))return Error::missing_source;
    const int count=static_cast<int>(s.commerce_page_lists.at(id).size());
    if(input.selection&&(*input.selection<0||*input.selection>=count))return Error::invalid_page;
    if(input.scroll&&input.scroll->value==std::numeric_limits<int>::max())return Error::invalid_page;
    auto next=s;
    auto &v=next.commerce_page_data.at(id);
    if(input.scroll) {
        const auto wanted=input.scroll->absolute?std::int64_t(input.scroll->value):std::int64_t(v[3])+input.scroll->value;
        const int first=static_cast<int>(std::clamp(wanted,std::int64_t(0),std::int64_t(std::max(0,count-3))));
        v[2]+=first-v[3];v[3]=first;
    }
    if(input.selection)v[2]=*input.selection;
    if(input.up)v[2]=v[2]==0?count-1:v[2]-1;
    if(input.down)v[2]=v[2]==count-1?0:v[2]+1;
    scroll(v,count,3);
    if(input.confirm||input.cancel||input.inspect) {
        next.scripts.executing_page=id;
        const auto result=facility_action(next,id,input.confirm?Action::confirm:input.cancel?Action::cancel:Action::inspect);
        if(result!=Error::none)return result;
        next.scripts.executing_page.reset();
    }
    s=std::move(next);
    return Error::none;
}
std::optional<State> update_startup_world_commerce_page(const State &s, std::uint64_t id) {
    const auto p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page ||
        !supported(p->legacy_page))
        return {};
    if (s.scene.framework_paused)
        return s;
    auto next = s;
    next.scripts.executing_page = id;
    const Page original = *p;
    if (!initialize(next, original))
        return {};
    const auto current = std::find_if(next.scripts.pages.begin(), next.scripts.pages.end(),
                                      [&](const auto &v) { return v.id == id; });
    if (current == next.scripts.pages.end())
        return {};
    if (current->lifecycle != 4) {
        auto &counter = next.page_counters.at(id);
        if (counter == std::numeric_limits<int>::max())
            return {};
        ++counter;
        if(original.legacy_page==93)counter%=std::numeric_limits<int>::max();
        if (original.legacy_page == 93 && counter == 1)
            next.sound_requests.push_back({StartupAudioOperation::jingle, 5});
        auto &feedback = next.commerce_page_data.at(id)[5];
        if (original.legacy_page == 84 && feedback > 0)
            --feedback;
        if (original.legacy_page == 86 && !close(next, id))
            return {};
    }
    next.scripts.executing_page.reset();
    return next;
}
} // namespace ark::simulation
