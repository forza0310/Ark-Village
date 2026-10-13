#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_reference/human_management.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
const ref::WorldScriptPage *top(const State &s) {
    const auto p = std::find_if(s.scripts.pages.rbegin(), s.scripts.pages.rend(),
                                [](const auto &page) { return page.lifecycle != 4; });
    return p == s.scripts.pages.rend() ? nullptr : &*p;
}
bool close(State &s, std::uint64_t id) {
    const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
bool event(State &s, int id, std::optional<std::string> parameter = {}) {
    const auto r = ref::prepare_world_script(startup_world_runtime_catalog(),
                                             startup_world_runtime_scripts(s), {id, parameter, {}});
    return r.candidate && write_startup_world_runtime_scripts(s, r.candidate->state);
}
std::optional<std::uint64_t> open(State &s, int raw, int human) {
    ref::WorldScriptPage page;
    page.kind = ref::WorldScriptPageKind::raw_page;
    page.legacy_page = raw;
    const auto r = ref::prepare_world_script_page(startup_world_runtime_scripts(s), page);
    if (!r.candidate || r.candidate->inserted_pages.size() != 1 ||
        !write_startup_world_runtime_scripts(s, r.candidate->state))
        return {};
    const auto id = r.candidate->inserted_pages.front().id;
    s.page_human_bindings[id] = human;
    s.page_phases[id] = s.page_counters[id] = s.human_page_selections[id] = 0;
    return id;
}
std::vector<ref::HumanManagementProfession> professions(const State &s) {
    std::vector<ref::HumanManagementProfession> result;
    for (std::size_t n = 0; n < s.rules->jobs.size(); ++n) {
        const auto &j = s.rules->jobs[n];
        result.push_back({static_cast<int>(n), s.scripts.professions.at(static_cast<int>(n)).status,
                          j.sort_order, j.script_extra, j.change_points, j.required_medals});
    }
    return result;
}
std::optional<ref::HumanManagementEquipment> equipment(const State &s, int slot, int id) {
    const int kind = slot == 0 ? 1 : slot == 3 ? 3 : 2;
    const auto d =
        std::find_if(s.rules->equipment.begin(), s.rules->equipment.end(),
                     [&](const auto &e) { return e.shop.kind == kind && e.shop.id == id; });
    const auto current = s.catalog.find({kind, id});
    if (slot < 0 || slot > 3 || d == s.rules->equipment.end() || current == s.catalog.end() ||
        (kind == 2 && ((d->shop.type == 2) ? 1 : 2) != slot))
        return {};
    return ref::HumanManagementEquipment{id,
                                         slot,
                                         current->second.status,
                                         d->gift_order,
                                         d->reward_difficulty,
                                         d->shop.price,
                                         d->gift_rating,
                                         current->second.free_purchases,
                                         d->shop.combat};
}
bool reward(State &s, int human, const ref::HumanRewardCandidate &r) {
    auto &g = s.scene.world.world.ai.growth.at(human);
    g.definition = r.definition;
    if (r.derived) {
        g.derived = *r.derived;
        if (!synchronize_startup_world_human_capacity(s, human))
            return false;
    }
    s.shop_humans.at(human).satisfaction = r.satisfaction;
    s.human_calendar.at(human).celebrations = r.celebrations;
    s.scene.world.world.ai.pending_completion = r.pending_completion;
    s.reward_display = r.reward_display;
    if (r.effort_display)
        s.effort_display = *r.effort_display;
    return event(s, 58);
}
bool reward_page(State &s, int human, const ref::HumanRewardCandidate &r) {
    if (!r.effort_display)
        return true;
    const auto next_id=s.scripts.next_page_id;
    if(s.page_human_bindings.count(next_id)||s.page_counters.count(next_id)||s.page_phases.count(next_id)||
       s.page_secondary_counters.count(next_id))return false;
    const auto id = open(s, 67, human);
    if (!id)
        return false;
    auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                             [&](const auto &p) { return p.id == *id; });
    page->legacy_f = r.reward_display[0][1];
    page->legacy_g = r.reward_display[1][1];
    return valid_startup_world_runtime_effort_result_page(s,*page);
}
bool initialize(State &s, std::uint64_t id, int raw) {
    if(raw==70) {
        const auto p=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &v){return v.id==id;});
        if(p==s.scripts.pages.end()||!valid_startup_world_human_mastery_page(s,*p))return false;
    }
    const auto h = s.page_human_bindings.find(id);
    if (h == s.page_human_bindings.end() || !startup_world_human_details(s, h->second) ||
        (raw == 60 && !valid_startup_world_human_detail_context(s, id)))
        return false;
    const int human = h->second;
    if (raw == 65 || raw == 66 || raw == 68 || raw == 69) {
        const auto choice = s.human_equipment_choices.find(id);
        if (choice == s.human_equipment_choices.end() || choice->second[0] < 0 ||
            choice->second[0] > ((raw == 65 || raw == 68) ? 3 : 4))
            return false;
        if (choice->second[0] == 4) {
            const auto item =
                std::find_if(s.rules->items.begin(), s.rules->items.end(),
                             [&](const auto &d) { return d.identity == choice->second[1]; });
            if (item == s.rules->items.end() ||
                (raw == 69 && (item->effect < 0 || item->effect >= 6)))
                return false;
        } else if (raw == 69 || !equipment(s, choice->second[0], choice->second[1]))
            return false;
        if (raw == 66 && (!s.human_gift_scores.count(id) || !s.human_gift_messages.count(id)))
            return false;
    }
    if (s.human_pages_initialized.count(id)) {
        if (!s.page_phases.count(id) || !s.page_counters.count(id) ||
            !s.human_page_selections.count(id))
            return false;
        if ((raw == 61 || raw == 62) && !s.human_page_catalogs.count(id))
            return false;
        if(raw==61||raw==64) {
            const auto cursor=inspect_startup_world_human_catalogue_cursor(s,id);
            if(!cursor)return false;
        }
        if ((raw == 64 || raw == 73) && !s.equipment_page_catalogs.count(id))
            return false;
        if (raw == 64 || raw == 73) {
            const int phase = s.page_phases.at(id);
            const int selected = s.human_page_selections.at(id);
            if (phase < 0 || phase > (raw == 64 ? 4 : 3))
                return false;
            const auto &list = s.equipment_page_catalogs.at(id)[phase];
            if (selected < 0 ||
                (list.empty() ? selected != 0 : selected >= static_cast<int>(list.size())))
                return false;
        }
        if ((raw == 65 || raw == 66 || raw == 68 || raw == 69) && !s.human_equipment_choices.count(id))
            return false;
        return true;
    }
    // 首次Init不能接管孤立滚动载荷；只有本次Init可以创建该键。
    if (s.human_page_scroll.count(id))
        return false;
    auto &g = s.scene.world.world.ai.growth.at(human);
    if (raw == 60) {
        const auto stats =
            ref::derive_human_stats(g.definition, s.scene.world.world.ai.professions);
        if (!stats.candidate || (!ref::world_script_seen(s.scripts, 111) && !event(s, 111)))
            return false;
        g.derived = *stats.candidate;
        if (!synchronize_startup_world_human_capacity(s, human))
            return false;
    } else if (raw == 61 || raw == 62) {
        const auto profile = startup_world_human_profile(s, human);
        if (!profile)
            return false;
        for(std::size_t job=0;job<s.rules->jobs.size();++job)
            if(!s.scripts.professions.count(static_cast<int>(job)))return false;
        const auto list =
            ref::catalogue_human_professions(profile->sex, professions(s));
        if (!list || list->empty())
            return false;
        s.human_page_catalogs[id] = *list;
        if (raw == 62) {
            const auto selected = s.page_job_bindings.find(id);
            if (selected == s.page_job_bindings.end())
                return false;
            const auto chosen = std::find(list->begin(), list->end(), selected->second);
            if (chosen == list->end())
                return false;
            s.human_page_selections[id] = static_cast<int>(chosen - list->begin());
            const auto display = ref::preview_human_profession_change(
                g.definition, s.scene.world.world.ai.professions, selected->second);
            if (!display)
                return false;
            s.human_attribute_display = {display->before, display->after, display->difference};
        }
    } else if (raw == 63) {
        if (!s.page_job_bindings.count(id))
            return false;
        const auto draw = s.scene.random.draw(5);
        if (draw.error != ref::WorldRandomError::none)
            return false;
        constexpr const char *messages[]{"谢谢", "我要加油!", "转机", "好开心", "合适吗？"};
        s.human_gift_messages[id] = messages[draw.ticket];
    } else if (raw == 64 || raw == 73) {
        if (raw == 64 && !ref::world_script_seen(s.scripts, 99) && !event(s, 99))
            return false;
        std::vector<ref::HumanManagementEquipment> definitions;
        for (const auto &d : s.rules->equipment) {
            const int slot = d.shop.kind == 1 ? 0 : d.shop.kind == 3 ? 3 : d.shop.type == 2 ? 1 : 2;
            const auto e = equipment(s, slot, d.shop.id);
            if (!e)
                return false;
            definitions.push_back(*e);
        }
        for (int slot = 0; slot < 4; ++slot) {
            const auto list = ref::catalogue_human_equipment(slot, definitions);
            if (!list)
                return false;
            s.equipment_page_catalogs[id][slot] = *list;
        }
        if (raw == 64) {
            for (const auto &item : s.rules->items) {
                const auto stock = s.items.find(item.identity);
                if (stock == s.items.end() || stock->second.inventory < 0)
                    return false;
                if (stock->second.inventory > 0)
                    s.equipment_page_catalogs[id][4].push_back(item.identity);
            }
        }
        if (raw == 73) {
            const auto choice = s.human_equipment_choices.find(id);
            if (choice == s.human_equipment_choices.end() || choice->second[0] < 0 ||
                choice->second[0] > 3)
                return false;
            const auto &list = s.equipment_page_catalogs.at(id)[choice->second[0]];
            const auto selected = std::find(list.begin(), list.end(), choice->second[1]);
            if (selected == list.end())
                return false;
            s.page_phases[id] = choice->second[0];
            s.human_page_selections[id] = static_cast<int>(selected - list.begin());
        }
    }
    s.human_pages_initialized.insert(id);
    s.page_phases.try_emplace(id, 0);
    s.page_counters.try_emplace(id, 0);
    s.human_page_selections.try_emplace(id, 0);
    if(raw==61||raw==64) {
        auto first=s.human_page_scroll.emplace(id,0).first;
        const int phase=s.page_phases.at(id),selected=s.human_page_selections.at(id);
        if(selected<0||phase<0||phase>(raw==61?1:4))return false;
        const auto count=raw==61?s.human_page_catalogs.at(id).size():s.equipment_page_catalogs.at(id)[phase].size();
        if(count?std::size_t(selected)>=count:selected!=0)return false;
        // 回复入口可预选屏外道具；首轮按原窗口规则修正，不能将合法前缀判为坏载荷。
        const int visible=raw==61?5:4;
        if(selected>=visible)first->second=selected-visible+1;
    }
    if (raw == 60 || raw == 70) {
        // Init可压入首次说明111，栈顶已不是60；完成Init的本页仍须进入Start阶段。
        // 不能靠框架仅更新back()把已初始化却被遮住的60遗留为生命周期0。
        for (auto &page : s.scripts.pages)
            if (page.id == id && page.lifecycle == 0)
                page.lifecycle = 1;
    }
    return raw != 60 || valid_startup_world_human_detail_context(s, id);
}
bool denial(State &s, ref::HumanManagementDenial reason, const StartupWorldJob *job = nullptr) {
    using D = ref::HumanManagementDenial;
    if (reason == D::same_profession20)
        return job && event(s, 20, job->name);
    if (reason == D::insufficient_points12)
        return event(s, 12);
    if (reason == D::insufficient_medals27)
        return job && event(s, 27, std::to_string(job->required_medals));
    return reason == D::none;
}
std::optional<std::uint64_t> live_parent(const State &s, std::uint64_t child, int raw, int human) {
    const auto binding = s.human_page_parents.find(child);
    if (binding == s.human_page_parents.end() || s.human_page_answers.count(binding->second))
        return {};
    const auto parent = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                     [&](const auto &p) { return p.id == binding->second; });
    const auto current = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                      [&](const auto &p) { return p.id == child; });
    const auto person = s.page_human_bindings.find(binding->second);
    if (parent == s.scripts.pages.end() || current == s.scripts.pages.end() || parent >= current ||
        parent->lifecycle == 4 || parent->kind != ref::WorldScriptPageKind::raw_page ||
        parent->legacy_page != raw || person == s.page_human_bindings.end() ||
        person->second != human)
        return {};
    return parent->id;
}
bool commit_gift(State &s, std::uint64_t parent, int human) {
    const auto selected = s.human_equipment_choices.find(parent);
    if (selected == s.human_equipment_choices.end())
        return false;
    const auto choice = selected->second;
    const auto target = equipment(s, choice[0], choice[1]);
    if (!target)
        return false;
    auto &g = s.scene.world.world.ai.growth.at(human);
    const auto &shop = s.shop_humans.at(human);
    int old_grade{};
    if (shop.equipment[choice[0]]) {
        const auto old = equipment(s, choice[0], *shop.equipment[choice[0]]);
        if (!old)
            return false;
        old_grade = old->grade;
    }
    ref::HumanEquipmentGiftInput input;
    input.definition = g.definition;
    input.professions = s.scene.world.world.ai.professions;
    for (std::size_t n = 0; n < 4; ++n)
        input.equipment_ids[n] = shop.equipment[n].value_or(-1);
    input.equipment_cooldowns = shop.reselect;
    input.target = *target;
    input.old_grade = old_grade;
    input.profession_affinity = s.rules->jobs.at(g.definition.current_profession).gift_profile;
    input.money = s.scene.world.world.ai.accounting.funds();
    input.satisfaction = shop.satisfaction;
    input.medals = s.human_calendar.at(human).celebrations;
    input.pending_completion = s.scene.world.world.ai.pending_completion;
    const auto result = ref::prepare_human_equipment_gift(input);
    if (!result.candidate)
        return false;
    const auto &c = *result.candidate;
    auto scripts = startup_world_runtime_scripts(s);
    if (!scripts.finance)
        return false;
    const auto month = scripts.finance->month;
    const auto expense =
        static_cast<std::int64_t>(scripts.finance->monthly_totals.at(month)[2][1]) + c.cash_charge;
    if (expense > std::numeric_limits<int>::max())
        return false;
    scripts.finance->cash = c.money;
    scripts.finance->monthly_totals.at(month)[2][1] = static_cast<int>(expense);
    if (!write_startup_world_runtime_scripts(s, scripts))
        return false;
    s.catalog.at({choice[0] == 0 ? 1 : choice[0] == 3 ? 3 : 2, choice[1]}).free_purchases = c.stock;
    if (!reward(s, human, c.reward))
        return false;
    g.definition = c.definition;
    g.derived = c.final_stats;
    if (!synchronize_startup_world_human_capacity(s, human))
        return false;
    auto &current = s.shop_humans.at(human);
    for (std::size_t n = 0; n < 4; ++n)
        current.equipment[n] = c.equipment_ids[n] == -1 ? std::optional<int>{} : c.equipment_ids[n];
    current.reselect = c.equipment_cooldowns;
    const auto text = s.scene.random.draw(2);
    if (text.error != ref::WorldRandomError::none)
        return false;
    constexpr const char *messages[3][2]{
        {"好开心", "谢谢"}, {"好开心", "谢谢"}, {"太好了!", "感动了!"}};
    const auto gift = open(s, 66, human);
    if (!gift)
        return false;
    s.human_gift_scores[*gift] = c.evaluation;
    s.human_gift_messages[*gift] = messages[c.dialogue_band][text.ticket];
    s.human_equipment_choices[*gift] = choice;
    if (c.equipment_display) {
        s.equipment_attribute_display = *c.equipment_display;
        const auto display=open(s,68,human);
        if (!display)
            return false;
        // 原结果头保存实际变更槽及新装备，不由恢复后的当前装备猜测。
        s.human_equipment_choices[*display]=choice;
    }

    if (!reward_page(s, human, c.reward))
        return false;
    for (const auto id : s.scene.world.world.ai.human_order) {
        auto &actor = s.scene.world.world.ai.battle.actors.at(id);
        if (actor.definition != human)
            continue;
        const int weapon = current.equipment[0].value_or(-1);
        s.shop_actors.at(id).weapon = weapon;
        s.actor_metadata.at(id).weapon = weapon;
        break; // 原j()只同步首个同定义实例，不广播装备。
    }
    return clear_startup_world_item_notices(s);
}

// 普通道具在父64直接确认：先评价/结果，再消耗库存；全过程仍在Owner候选内。
bool commit_item(State &s, std::uint64_t parent, int human, int item) {
    const auto source = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                     [&](const auto &d) { return d.identity == item; });
    const auto stock = s.items.find(item);
    const auto catalog = s.catalog.find({0, item});
    if (source == s.rules->items.end() || stock == s.items.end() || catalog == s.catalog.end() ||
        catalog->second.inventory != stock->second.inventory)
        return false;
    auto &growth = s.scene.world.world.ai.growth.at(human);
    const auto &affinities = s.rules->jobs.at(growth.definition.current_profession).item_affinity;
    if (source->category < 0 || static_cast<std::size_t>(source->category) >= affinities.size())
        return false;
    const auto result = ref::prepare_human_item_gift(
        {growth.definition, s.scene.world.world.ai.professions, stock->second.inventory,
         affinities[source->category], source->difficulty, source->effect, source->attribute_amount,
         source->spell, s.shop_humans.at(human).satisfaction,
         s.human_calendar.at(human).celebrations, s.scene.world.world.ai.pending_completion});
    if (!result.candidate || !reward(s, human, result.candidate->reward))
        return false;
    const auto &c = *result.candidate;
    growth.definition = c.definition;
    if (c.final_stats) {
        growth.derived = *c.final_stats;
        if (!synchronize_startup_world_human_capacity(s, human))
            return false;
    }
    const auto text = s.scene.random.draw(2);
    if (text.error != ref::WorldRandomError::none)
        return false;
    const auto gift = open(s, 66, human);
    if (!gift)
        return false;
    constexpr const char *messages[3][2]{
        {"好开心", "谢谢"}, {"好开心", "谢谢"}, {"太好了!", "感动了!"}};
    s.human_gift_scores[*gift] = c.evaluation;
    s.human_gift_messages[*gift] = messages[c.dialogue_band][text.ticket];
    s.human_equipment_choices[*gift] = {4, item};
    if (c.attribute_display) {
        const auto &d = *c.attribute_display;
        s.human_attribute_display = {d.before, d.after, d.difference};
        const auto display = open(s, 69, human);
        if (!display)
            return false;
        s.human_equipment_choices[*display] = {4, item};
    }
    if (!reward_page(s, human, c.reward))
        return false;
    s.items.at(item).inventory = c.stock;
    s.catalog.at({0, item}).inventory = c.stock;
    auto &list = s.equipment_page_catalogs.at(parent)[4];
    if (!c.stock)
        list.erase(std::remove(list.begin(), list.end(), item), list.end());
    auto &selection = s.human_page_selections.at(parent);
    if (selection >= static_cast<int>(list.size()))
        selection = 0;
    // Steam非空目录在本次消费后即修窗；空目录沿既有维护归零策略，区别原保留旧游标。
    auto &first=s.human_page_scroll.at(parent);
    if(list.empty())first=0;
    else {
        if(first>selection)first=selection;else if(first+4<=selection)first=selection-3;
        first=std::min(first,std::max(0,static_cast<int>(list.size())-4));
    }
    // 原j()末尾同步首个同定义实例武器，即使道具不改变装备也执行同一收尾。
    for (const auto id : s.scene.world.world.ai.human_order) {
        const auto &actor = s.scene.world.world.ai.battle.actors.at(id);
        if (actor.definition == human) {
            const int weapon = s.shop_humans.at(human).equipment[0].value_or(-1);
            s.shop_actors.at(id).weapon = weapon;
            s.actor_metadata.at(id).weapon = weapon;
            break;
        }
    }
    return clear_startup_world_item_notices(s);
}

bool item_recovery(State &s, std::uint64_t page, int counter) {
    const auto choice = s.human_equipment_choices.find(page);
    if (choice == s.human_equipment_choices.end())
        return false;
    if (choice->second[0] != 4)
        return choice->second[0] >= 0 && choice->second[0] < 4;
    const auto item = std::find_if(s.rules->items.begin(), s.rules->items.end(),
                                   [&](const auto &d) { return d.identity == choice->second[1]; });
    if (item == s.rules->items.end())
        return false;
    if (item->recovery <= 0 || counter < 75)
        return true;
    auto &world = s.scene.world.world;
    for (const auto id : world.ai.human_order) {
        const auto found = world.ai.battle.actors.find(id);
        if (found == world.ai.battle.actors.end())
            return false;
        auto &actor = found->second;
        if (actor.definition != s.page_human_bindings.at(page))
            continue;
        if (counter == 75) {
            if (actor.control.action == 7 && !actor.rescue) {
                const auto hp = ref::prepare_hp_assignment(actor.hp, 0);
                const auto metadata = world.actors.find(id);
                if (!hp.candidate || metadata == world.actors.end())
                    return false;
                const auto restored = ref::prepare_actor_baseline_restore(
                    actor.control, actor.baseline, actor.kind == ref::ActorKind::human,
                    metadata->second.monster_mode);
                if (!restored)
                    return false;
                actor.state_counter = 900;
                actor.hp = *hp.candidate;
                actor.control = restored->control;
                if (restored->clear_encounter)
                    actor.encounter.reset();
            }
            const auto hp = ref::prepare_hp_change(actor.hp, item->recovery, actor.capacity);
            if (!hp.candidate)
                return false;
            actor.hp = *hp.candidate;
        }
        const auto animation = ref::advance_hp_animation(actor.hp);
        if (!animation.candidate)
            return false;
        actor.hp = *animation.candidate;
        break;
    }
    return true;
}

bool profession_animation(State &s, std::uint64_t id, int human, bool confirm) {
    const auto binding = s.page_job_bindings.find(id);
    const auto frame = s.page_counters.find(id);
    if (binding == s.page_job_bindings.end() || frame == s.page_counters.end() ||
        binding->second < 0 || binding->second >= static_cast<int>(s.rules->jobs.size()))
        return false;
    const auto plan = ref::human_profession_change_animation_plan(frame->second, confirm);
    if (!plan)
        return false;
    auto &growth = s.scene.world.world.ai.growth.at(human);
    if (plan->set_profession) {
        growth.definition.current_profession = binding->second;
        for (const auto actor : s.scene.world.world.ai.human_order) {
            const auto a = s.scene.world.world.ai.battle.actors.find(actor);
            const auto meta = s.actor_metadata.find(actor);
            if (a == s.scene.world.world.ai.battle.actors.end() || meta == s.actor_metadata.end())
                return false;
            if (a->second.definition == human && a->second.kind == ref::ActorKind::human)
                meta->second.profession = binding->second;
        }
    }
    if (plan->finish) {
        const auto stats =
            ref::derive_human_stats(growth.definition, s.scene.world.world.ai.professions);
        if (!stats.candidate)
            return false;
        growth.derived = *stats.candidate;
        if (!synchronize_startup_world_human_capacity(s, human) ||
            !refresh_startup_world_profession_economy(s) || !close(s, id))
            return false;
        growth.experience = 0;
        growth.pending.amount = 0; // L/N清零；O等待计数保留。
    }
    return true;
}

bool consume_display(State &s, std::uint64_t id, int raw, bool confirm) {
    auto &counter = s.page_counters.at(id);
    if (raw == 63)
        return profession_animation(s, id, s.page_human_bindings.at(id), confirm);
    if (raw == 68) {
        const auto result =
            ref::prepare_world_effort_display(counter, confirm, s.equipment_attribute_display[2]);
        if (!result)
            return false;
        counter = result->counter;
        return !result->closed || close(s, id);
    }
    if (raw == 66) {
        // 页面输入与真正更新分离：同一counter重复确认不能重复触发g(w)或s()。
        if (!confirm && !item_recovery(s, id, counter))
            return false;
        const int before = counter - 85;
        auto &phase = s.page_phases.at(id);
        if (counter >= 135 && phase == 0)
            phase = 1;
        if (confirm) {
            if (counter < 40)
                counter = 40;
            if (before >= 0) {
                if (before < 67)
                    counter += 66 - before;
                if (counter - 85 >= 67 && phase == 1)
                    return close(s, id);
            }
        }
    }
    if (raw == 69 && confirm) {
        if (counter < 39)
            counter = 39;
        else if (counter >= 45)
            return close(s, id);
    }
    return true;
}

bool resume_answer(State &s, std::uint64_t id, int raw, int human) {
    const auto answer = s.human_page_answers.find(id);
    if (answer == s.human_page_answers.end())
        return true;
    const int value = answer->second;
    s.human_page_answers.erase(answer);
    if (value != 0 && value != 1)
        return false;
    if (value == 1)
        return true;
    if (raw == 61)
        return close(s, id);
    return raw == 64 && commit_gift(s, id, human);
}
} // namespace
bool valid_startup_world_human_mastery_page(const State &s,const ref::WorldScriptPage &p) {
    const auto id=p.id;
    if(!s.rules||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=70||p.lifecycle<0||p.lifecycle>4)return false;
    const auto human=s.page_human_bindings.find(id),main=s.page_counters.find(id),secondary=s.page_secondary_counters.find(id),
               phase=s.page_phases.find(id),selection=s.human_page_selections.find(id);
    if(human==s.page_human_bindings.end()||!startup_world_human_details(s,human->second)||
       main==s.page_counters.end()||secondary==s.page_secondary_counters.end()||phase==s.page_phases.end()||
       selection==s.human_page_selections.end()||main->second<0||main->second==std::numeric_limits<int>::max()||
       secondary->second<0||secondary->second==std::numeric_limits<int>::max()||phase->second<0||phase->second>2||
       selection->second<0||selection->second>1||(phase->second<2&&selection->second!=0))return false;
    // 这些载荷属于其它人物页，原70没有父答案、职业选择目录或滚动窗口。
    if(s.human_page_parents.count(id)||s.human_page_answers.count(id)||s.human_page_catalogs.count(id)||
       s.equipment_page_catalogs.count(id)||s.page_job_bindings.count(id)||s.human_page_scroll.count(id)||
       s.human_equipment_choices.count(id))return false;
    if(!s.human_pages_initialized.count(id))
        return p.lifecycle==0&&main->second==0&&secondary->second==0&&phase->second==0&&selection->second==0;
    return p.lifecycle!=0;
}

bool synchronize_startup_world_human_capacity(State &s, int human) {
    auto &ai = s.scene.world.world.ai;
    const auto growth = ai.growth.find(human);
    if (growth == ai.growth.end())
        return false;
    const int capacity = growth->second.derived.combat[0];
    const auto update = [&](auto &actors) {
        for (auto &[id, actor] : actors) {
            (void)id;
            if (actor.kind == ref::ActorKind::human && actor.definition == human)
                actor.capacity = capacity;
        }
    };
    update(ai.battle.actors);
    update(ai.retired_actors);
    if (s.focus_actor.actor.kind == ref::ActorKind::human &&
        s.focus_actor.actor.definition == human)
        s.focus_actor.actor.capacity = capacity;
    return true;
}
std::optional<StartupHumanDetails> startup_world_human_details(const State &s, int id) {
    const auto profile = startup_world_human_profile(s, id);
    if (!profile)
        return {};
    const auto g = s.scene.world.world.ai.growth.find(id);
    const auto shop = s.shop_humans.find(id);
    const auto calendar = s.human_calendar.find(id);
    const auto home = s.human_homes.find(id);
    if (g == s.scene.world.world.ai.growth.end() || shop == s.shop_humans.end() ||
        calendar == s.human_calendar.end() || home == s.human_homes.end())
        return {};
    const int job = g->second.definition.current_profession;
    if (job < 0 || job >= static_cast<int>(s.rules->jobs.size()) ||
        job >= static_cast<int>(g->second.definition.profession_levels.size()) ||
        job >= static_cast<int>(s.scene.world.world.ai.professions.size()))
        return {};
    const int level = g->second.definition.profession_levels[job];
    const auto threshold =
        ref::human_growth_threshold(level, s.scene.world.world.ai.professions[job].difficulty);
    if (!threshold || g->second.experience < 0)
        return {};
    StartupHumanDetails result{id,
                               job,
                               level,
                               g->second.experience,
                               *threshold,
                               shop->second.satisfaction,
                               g->second.definition.legacy_u,
                               calendar->second.celebrations,
                               home->second[2] == 1,
                               g->second.derived.attributes,
                               g->second.derived.combat,
                               shop->second.equipment,
                               g->second.derived.available_spells,
                               {}, profile->name, profile->sex};
    for (const auto actor : s.scene.world.world.ai.human_order) {
        const auto found = s.scene.world.world.ai.battle.actors.find(actor);
        if (found == s.scene.world.world.ai.battle.actors.end())
            return {};
        if (found->second.definition == id) {
            result.live_actor = actor;
            break;
        }
    }
    return result;
}
std::optional<StartupHumanPresentationView>
inspect_startup_world_human_presentation(const State &s, std::uint64_t id) {
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [id](const auto &p) { return p.id == id; });
    if (!s.rules || page == s.scripts.pages.end() ||
        page->kind != ref::WorldScriptPageKind::raw_page || page->legacy_page != 60 ||
        page->lifecycle < 1 || page->lifecycle > 3 ||
        !s.human_pages_initialized.count(id) ||
        !valid_startup_world_human_detail_context(s, id))
        return {};
    const auto binding = s.page_human_bindings.find(id);
    const auto tab = s.page_phases.find(id);
    const auto frame = s.page_counters.find(id);
    const auto selection = s.human_page_selections.find(id);
    if (binding == s.page_human_bindings.end() || tab == s.page_phases.end() ||
        frame == s.page_counters.end() || selection == s.human_page_selections.end() ||
        tab->second < 0 || tab->second > 3 || frame->second < 0 || selection->second < 0 || selection->second > 3)
        return {};
    const auto details = startup_world_human_details(s, binding->second);
    if (!details)
        return {};
    StartupHumanPresentationView view{id, tab->second, frame->second, *details, {}};
    view.equipment_selection=selection->second;
    const auto &context = s.human_detail_contexts.find(id)->second;
    view.tracking_available = context.chase_mode == 1;
    if (context.actor)
        view.human.live_actor = context.actor;
    const auto &ai = s.scene.world.world.ai;
    // 镜头确认页保留显式W；仅无W绑定的目录详情才按名单取首个同定义实例。
    // 两条路径都不回退到退休实例或独立W。
    for (const auto actor_id : ai.human_order) {
        const auto actor = ai.battle.actors.find(actor_id);
        if (actor == ai.battle.actors.end() || !(actor->second.id == actor_id) ||
            actor->second.kind != ref::ActorKind::human)
            return {};
        const auto &live = actor->second;
        if (!view.human.live_actor || !(actor_id == *view.human.live_actor))
            continue;
        const auto growth = ai.growth.find(binding->second);
        if (growth == ai.growth.end() || growth->second.derived.combat[0] <= 0 ||
            live.control.state < 0 || live.definition != binding->second)
            return {};
        view.live = StartupHumanPresentationLive{actor_id, live.control.state,
                                                live.hp.displayed, live.hp.target,
                                                growth->second.derived.combat[0]};
        break;
    }
    if (view.human.live_actor && !view.live)
        return {};
    // 无实际实例仍返回定义肖像；不得补一条满HP血条。原HP可以超过新上限。
    return view;
}
std::optional<bool> startup_world_human_recovery_gift(const State &s,int human) {
    if(!s.rules||!startup_world_human_details(s,human))return {};
    const ref::BattleActorRecord *live{};
    for(const auto actor:s.scene.world.world.ai.human_order) {
        const auto found=s.scene.world.world.ai.battle.actors.find(actor);
        if(found==s.scene.world.world.ai.battle.actors.end()||!(found->second.id==actor)||
           found->second.kind!=ref::ActorKind::human)return {};
        if(found->second.definition==human){live=&found->second;break;}
    }
    if(!live)return false;
    bool recovery{};
    for(const auto &item:s.rules->items) {
        const auto stock=s.items.find(item.identity);
        if(stock==s.items.end()||stock->second.inventory<0)return {};
        recovery=recovery||(stock->second.inventory>0&&item.recovery>0);
    }
    const auto growth=s.scene.world.world.ai.growth.find(human);
    if(growth==s.scene.world.world.ai.growth.end()||growth->second.derived.combat[0]<=0||live->control.state<0)return {};
    return recovery&&(live->control.state==7||live->hp.target<growth->second.derived.combat[0]/2);
}
bool valid_startup_world_human_detail_context(const State &s, std::uint64_t id) {
    const auto page = std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                                   [id](const auto &p) { return p.id == id; });
    const auto context = s.human_detail_contexts.find(id);
    const auto binding = s.page_human_bindings.find(id);
    if (!s.rules || id == 0 || page == s.scripts.pages.end() ||
        std::count_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                      [id](const auto &p) { return p.id == id; }) != 1 ||
        page->kind != ref::WorldScriptPageKind::raw_page || page->legacy_page != 60 ||
        page->lifecycle < 0 || page->lifecycle > 4 ||
        context == s.human_detail_contexts.end() || binding == s.page_human_bindings.end() ||
        !startup_world_human_details(s, binding->second) ||
        (context->second.chase_mode != 0 && context->second.chase_mode != 1) ||
        (context->second.chase_mode == 0 && context->second.actor))
        return false;
    if (!s.human_pages_initialized.count(id) && page->lifecycle >= 1 && page->lifecycle <= 3)
        return false; // 已开始页缺标记是坏载荷，不能重新Init并重算/重播首次说明。
    if (s.human_pages_initialized.count(id)) {
        if (page->lifecycle == 0)
            return false;
        const auto phase = s.page_phases.find(id);
        const auto counter = s.page_counters.find(id);
        const auto selection = s.human_page_selections.find(id);
        if (phase == s.page_phases.end() || counter == s.page_counters.end() ||
            selection == s.human_page_selections.end() || phase->second < 0 || phase->second > 3 ||
            counter->second < 0 || counter->second == std::numeric_limits<int>::max() ||
            selection->second < 0 || selection->second > 3)
            return false;
    }
    // 来源1只来自实际35父页，或scene6确认后带W的新详情；来源不是可随意授予的标志。
    if (page->lifecycle != 4 && context->second.chase_mode == 1) {
        auto parent = page;
        while (parent != s.scripts.pages.begin()) {
            --parent;
            if (parent->lifecycle != 4) break;
        }
        if (parent == page || parent->lifecycle == 4) return false;
        if (context->second.actor) {
            if (parent->kind != ref::WorldScriptPageKind::scene) return false;
        } else {
            if (parent->kind != ref::WorldScriptPageKind::raw_page || parent->legacy_page != 35 ||
                !valid_startup_world_information_page(s, parent->id)) return false;
            const auto &list = s.information_page_data.find(parent->id)->second;
            if (list.lists.front()[list.selection] != binding->second) return false;
        }
    }
    if (!context->second.actor)
        return true;
    const auto &ai = s.scene.world.world.ai;
    const auto actor = ai.battle.actors.find(*context->second.actor);
    return actor != ai.battle.actors.end() && actor->second.id == *context->second.actor &&
           actor->second.kind == ref::ActorKind::human && actor->second.definition == binding->second &&
           std::find(ai.human_order.begin(), ai.human_order.end(), *context->second.actor) != ai.human_order.end() &&
           s.actor_metadata.count(*context->second.actor) != 0;
}

std::optional<std::uint64_t> append_startup_world_human_detail_page(
    State &s, int human, int chase_mode, std::optional<ref::CharacterId> actor) {
    const auto *parent = top(s);
    if (!s.rules || s.scene.framework_paused || s.scripts.page_mutations_locked ||
        !parent || parent->id == 0 ||
        std::count_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                      [parent](const auto &p) { return p.id == parent->id; }) != 1 ||
        (chase_mode != 0 && chase_mode != 1) || !startup_world_human_details(s, human))
        return {};
    const bool scene = parent->kind == ref::WorldScriptPageKind::scene;
    const bool directory = parent->kind == ref::WorldScriptPageKind::raw_page && parent->legacy_page == 35;
    // source0保留既有场景命令入口：原调用点夹具可在主场景生命周期0建立详情。
    // 新增追踪链只由已就绪35或真实scene6回调进入，不扩大其输入资格。
    if ((chase_mode == 0 && (parent->lifecycle < 0 || parent->lifecycle > 3)) ||
        (chase_mode == 1 && parent->lifecycle != 2 &&
         !(parent->lifecycle == 3 && s.scripts.executing_page == parent->id)))
        return {};
    if ((!scene && !directory) || (scene && s.scene.scene_state != 0) ||
        (chase_mode == 0 && (!scene || actor)) ||
        (chase_mode == 1 && ((scene && !actor) || (directory && actor))))
        return {};
    if (directory && !valid_startup_world_information_page(s, parent->id))
        return {};
    if (directory) {
        const auto &list = s.information_page_data.find(parent->id)->second;
        if (list.lists.front()[list.selection] != human) return {};
    }
    if (scene && chase_mode == 0) {
        const auto presence = s.human_presence.find(human);
        if (presence == s.human_presence.end() || presence->second == 0)
            return {};
    }
    if (actor) {
        const auto &ai = s.scene.world.world.ai;
        const auto found = ai.battle.actors.find(*actor);
        if (found == ai.battle.actors.end() || !(found->second.id == *actor) ||
            found->second.kind != ref::ActorKind::human || found->second.definition != human ||
            std::find(ai.human_order.begin(), ai.human_order.end(), *actor) == ai.human_order.end() ||
            !s.actor_metadata.count(*actor) || s.scripts.selected_actor != actor->value)
            return {};
    }
    // 原Push覆盖父页；底层script插页只负责位置/ID，不代替表单挂起。
    const auto parent_id = parent->id;
    for (auto &page : s.scripts.pages)
        if (page.id == parent_id)
            page.lifecycle = 3;
    const auto id = open(s, 60, human);
    if (!id || !s.human_detail_contexts.emplace(*id, StartupHumanDetailContext{chase_mode, actor}).second)
        return {};
    return id;
}

Error open_startup_world_human_page(State &s, int human) {
    const auto *p = top(s);
    const auto presence = s.human_presence.find(human);
    if (!s.rules || s.scene.framework_paused || s.scene.scene_state != 0 || !p ||
        p->kind != ref::WorldScriptPageKind::scene)
        return Error::invalid_page;
    if (presence == s.human_presence.end() || presence->second == 0 ||
        !startup_world_human_details(s, human))
        return Error::missing_source;
    auto next = s;
    next.scripts.executing_page = p->id;
    if (!append_startup_world_human_detail_page(next, human, 0))
        return Error::script_failed;
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}
bool startup_world_human_page_ready(const State &s, std::uint64_t id) {
    const auto *p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page ||
        !s.human_pages_initialized.count(id) || !s.page_human_bindings.count(id) ||
        !s.page_phases.count(id) || !s.page_counters.count(id) ||
        !s.human_page_selections.count(id))
        return false;
    const int raw = p->legacy_page;
    if(raw==70&&!valid_startup_world_human_mastery_page(s,*p))return false;
    if((raw==61||raw==64)&&!inspect_startup_world_human_catalogue_cursor(s,id))return false;
    if (raw == 60 && !valid_startup_world_human_detail_context(s, id))
        return false;
    if ((raw == 61 || raw == 62) && !s.human_page_catalogs.count(id))
        return false;
    if (raw == 64 || raw == 73) {
        const auto catalog = s.equipment_page_catalogs.find(id);
        const int phase = s.page_phases.at(id);
        if (catalog == s.equipment_page_catalogs.end() || phase < 0 || phase > (raw == 64 ? 4 : 3))
            return false;
    }
    return !((raw == 65 || raw == 66 || raw == 68 || raw == 69) && !s.human_equipment_choices.count(id)) &&
           !(raw == 66 && (!s.human_gift_scores.count(id) || !s.human_gift_messages.count(id)));
}
bool initialize_startup_world_human_pages(State &s) {
    if (s.scene.framework_paused)
        return true;
    std::vector<std::pair<std::uint64_t, int>> pending;
    for (const auto &p : s.scripts.pages) {
        if (p.lifecycle != 4 && p.kind == ref::WorldScriptPageKind::raw_page &&
            p.legacy_page == 60 && !valid_startup_world_human_detail_context(s, p.id))
            return false;
        if (p.lifecycle != 4 && p.kind == ref::WorldScriptPageKind::raw_page &&
            ((p.legacy_page >= 60 && p.legacy_page <= 66) ||
             (p.legacy_page == 68 || p.legacy_page == 69) || p.legacy_page == 70 ||
             p.legacy_page == 73) &&
            !s.human_pages_initialized.count(p.id))
            pending.emplace_back(p.id, p.legacy_page);
    }
    const auto executing = s.scripts.executing_page;
    for (const auto &[id, raw] : pending) {
        s.scripts.executing_page = id;
        if (!initialize(s, id, raw))
            return false;
    }
    s.scripts.executing_page = executing;
    return true;
}

std::optional<StartupHumanCatalogueCursor> inspect_startup_world_human_catalogue_cursor(
    const State &s,std::uint64_t id) {
    const auto p=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &v){return v.id==id;});
    const auto selection=s.human_page_selections.find(id),first=s.human_page_scroll.find(id),phase=s.page_phases.find(id),counter=s.page_counters.find(id);
    if(!s.rules||p==s.scripts.pages.end()||p->kind!=ref::WorldScriptPageKind::raw_page||p->lifecycle<0||p->lifecycle>3||
       (p->legacy_page!=61&&p->legacy_page!=64)||!s.human_pages_initialized.count(id)||!s.page_human_bindings.count(id)||
       selection==s.human_page_selections.end()||first==s.human_page_scroll.end()||phase==s.page_phases.end()||
       counter==s.page_counters.end()||counter->second<0||selection->second<0||first->second<0)return {};
    if(!startup_world_human_details(s,s.page_human_bindings.find(id)->second))return {};
    int count{},visible=p->legacy_page==61?5:4;
    if(p->legacy_page==61) {
        const auto list=s.human_page_catalogs.find(id);
        if(list==s.human_page_catalogs.end()||list->second.empty()||list->second.size()>std::size_t(std::numeric_limits<int>::max())||phase->second<0||phase->second>1)return {};
        for(const int job:list->second)
            if(job<0||std::size_t(job)>=s.rules->jobs.size()||!s.scripts.professions.count(job))return {};
        for(std::size_t job=0;job<s.rules->jobs.size();++job)
            if(!s.scripts.professions.count(static_cast<int>(job)))return {};
        count=static_cast<int>(list->second.size());
    } else {
        const auto lists=s.equipment_page_catalogs.find(id);
        if(lists==s.equipment_page_catalogs.end()||phase->second<0||phase->second>4)return {};
        const auto &list=lists->second[phase->second];if(list.size()>std::size_t(std::numeric_limits<int>::max()))return {};
        for(const int item:list) {
            if(phase->second<4) {if(!equipment(s,phase->second,item))return {};}
            else if(!s.items.count(item)||std::none_of(s.rules->items.begin(),s.rules->items.end(),
                [=](const auto &v){return v.identity==item;}))return {};
        }
        count=static_cast<int>(list.size());
    }
    if((count?selection->second>=count:selection->second!=0)||first->second>std::max(0,count-visible)||
       (count&&(selection->second<first->second||selection->second-first->second>=visible)))return {};
    return StartupHumanCatalogueCursor{selection->second,first->second,count,visible};
}
Error input_startup_world_human_catalogue_page(State &s,std::uint64_t id,const StartupHumanCatalogueInput &input) {
    const auto *page=top(s);
    if(!s.rules||s.scene.framework_paused||!page||page->id!=id||page->kind!=ref::WorldScriptPageKind::raw_page||
       (page->legacy_page!=61&&page->legacy_page!=64))return Error::invalid_page;
    auto next=s;next.scripts.executing_page=id;
    const int raw=page->legacy_page;
    if(!initialize(next,id,raw))return Error::missing_source;
    if(next.human_page_answers.count(id))return Error::invalid_page;
    const auto cursor=inspect_startup_world_human_catalogue_cursor(next,id);if(!cursor)return Error::missing_source;
    auto &selected=next.human_page_selections.at(id);
    auto &first=next.human_page_scroll.at(id);
    auto &phase=next.page_phases.at(id);
    if(input.scroll) {
        if(input.scroll->value==std::numeric_limits<int>::max())return Error::invalid_page;
        const auto proposed=input.scroll->absolute?std::int64_t(input.scroll->value):std::int64_t(first)+input.scroll->value;
        const int moved=static_cast<int>(std::clamp(proposed,std::int64_t(0),std::int64_t(std::max(0,cursor->count-cursor->visible))));
        selected+=moved-first;first=moved;
    }
    if(input.selection) {
        if(*input.selection<0||*input.selection>=cursor->count)return Error::invalid_page;
        selected=*input.selection;
    }
    int count=cursor->count;
    // 64先切最终页签，净变化才归零；上下随后使用新页签目录。
    if(raw==64) {
        const int old_phase=phase;
        if(input.left)phase=(phase+4)%5;
        if(input.right)phase=(phase+1)%5;
        if(phase!=old_phase) {
            selected=0;first=0;
            const auto switched=inspect_startup_world_human_catalogue_cursor(next,id);
            if(!switched)return Error::missing_source;
            count=switched->count;
        }
    }
    if(count) {
        if(input.up)selected=selected==0?count-1:selected-1;
        if(input.down)selected=selected==count-1?0:selected+1;
    } // 原空64目录跳过上下；仍允许切页、返回，不造非法取模。
    // 原同轮上下先合成再修窗，不能逐方向修窗使抵消输入改变旧scroll。
    if(selected<first)first=selected;else if(selected-first>=cursor->visible)first=selected-cursor->visible+1;
    if(raw==61) {
        if(input.left)phase=(phase+1)%2;
        if(input.right)phase=(phase+1)%2;
    }
    next.scripts.executing_page.reset();
    if(input.confirm||input.inspect_equipment||input.cancel) {
        const auto action=input.confirm?StartupHumanPageAction::confirm:input.cancel?
            StartupHumanPageAction::cancel:StartupHumanPageAction::inspect_equipment;
        const auto error=act_startup_world_human_page(next,id,action);
        if(error!=Error::none)return error;
    }
    s=std::move(next);return Error::none;
}
Error input_startup_world_human_detail_page(State &s,std::uint64_t id,const StartupHumanCatalogueInput &input) {
    const auto *page=top(s);
    if(!s.rules||s.scene.framework_paused||!page||page->id!=id||page->kind!=ref::WorldScriptPageKind::raw_page||
       page->legacy_page!=60||input.left||input.right||input.inspect_equipment)return Error::invalid_page;
    if(!startup_world_human_page_ready(s,id))return Error::missing_source;
    auto next=s;
    auto &phase=next.page_phases.find(id)->second;
    auto &chosen=next.human_page_selections.find(id)->second;
    const int old_phase=phase;
    if(input.scroll) {
        if(input.scroll->value==std::numeric_limits<int>::max())return Error::invalid_page;
        const auto requested=input.scroll->absolute?std::int64_t(input.scroll->value):std::int64_t(phase)+input.scroll->value;
        const int moved=static_cast<int>(std::clamp(requested,std::int64_t(0),std::int64_t(3)));
        // OnTouchVScrollBar先同步select；raw60的scroll与page轮末同值，无需第二份持久字段。
        chosen=std::clamp(chosen+moved-phase,0,3);phase=moved;
    }
    if(input.selection) {
        if(old_phase!=2||*input.selection<0||*input.selection>3)return Error::invalid_page;
        chosen=*input.selection;
    }
    // Steam1031E06F–E0AE：只在旧页1/3进入2时重定位；0直跳2保留触摸处理后的选择。
    if(phase==2&&old_phase!=2) {
        if(old_phase==1)chosen=0;
        else if(old_phase==3)chosen=3;
    }
    const int navigation_phase=phase;
    if(navigation_phase==2) {
        // 两个repeat共享本轮已进入的页2分支，不能逐动作重新分派到中途改变的page。
        if(input.up){if(chosen>0)--chosen;else phase=1;}
        if(input.down){if(chosen<3)++chosen;else phase=3;}
    } else {
        if(input.up)phase=(phase+3)%4;
        if(input.down)phase=(phase+1)%4;
        if(phase==2&&phase!=navigation_phase) {
            if(navigation_phase==1)chosen=0;
            else if(navigation_phase==3)chosen=3;
        }
    }
    next.scripts.executing_page.reset();
    if(input.confirm) {
        // 旧act(confirm)是维护关闭入口，保留；此桥明确使用原最终页的赠礼/转职/no-op路由。
        if(phase<=2) {
            const auto error=act_startup_world_human_page(next,id,phase==1?
                StartupHumanPageAction::professions:StartupHumanPageAction::gifts);
            if(error!=Error::none)return error;
        }
    } else if(input.cancel) {
        const auto error=act_startup_world_human_page(next,id,StartupHumanPageAction::cancel);
        if(error!=Error::none)return error;
    }
    s=std::move(next);return Error::none;
}
Error input_startup_world_human_mastery_page(State &s,std::uint64_t id,const StartupHumanCatalogueInput &input) {
    const auto *p=top(s);
    if(!p||p->id!=id||p->kind!=ref::WorldScriptPageKind::raw_page||p->legacy_page!=70||
       p->lifecycle!=2||s.scene.framework_paused||input.up||input.down||input.scroll||input.inspect_equipment)
        return Error::invalid_page;
    if(!valid_startup_world_human_mastery_page(s,*p))return Error::missing_source;
    const int phase=s.page_phases.at(id);
    if(input.selection&&phase!=2)return Error::invalid_page;
    if(input.selection&&(*input.selection<0||*input.selection>1))return Error::invalid_page;
    auto next=s;
    auto &selected=next.human_page_selections.at(id);
    if(input.selection)selected=*input.selection;
    if(phase==2&&input.left)selected=(selected+1)%2;
    if(phase==2&&input.right)selected=(selected+1)%2;
    // APK Back2与确认等价；Steam专属Update仅证确认，窗口不能据此推断Esc映射。
    if(input.confirm||input.cancel) {
        const auto error=act_startup_world_human_page(next,id,StartupHumanPageAction::confirm);
        if(error!=Error::none)return error;
    }
    s=std::move(next);return Error::none;
}
Error act_startup_world_human_page(State &s, std::uint64_t id, StartupHumanPageAction action,
                                   int selection) {
    using A = StartupHumanPageAction;
    const auto *p = top(s);
    if (!s.rules || s.scene.framework_paused || !p || p->id != id ||
        p->kind != ref::WorldScriptPageKind::raw_page ||
        !((p->legacy_page >= 60 && p->legacy_page <= 66) ||
          (p->legacy_page == 68 || p->legacy_page == 69) || p->legacy_page == 70 ||
          p->legacy_page == 73))
        return Error::invalid_page;
    if (action == A::track &&
        (p->legacy_page != 60 || p->lifecycle != 2 || s.scripts.page_mutations_locked ||
         !s.human_pages_initialized.count(id) ||
         !valid_startup_world_human_detail_context(s, id) ||
         s.human_detail_contexts.find(id)->second.chase_mode != 1))
        return Error::invalid_page;
    auto next = s;
    next.scripts.executing_page = id;
    const int raw = p->legacy_page;
    if (!initialize(next, id, raw))
        return Error::missing_source;
    const int human = next.page_human_bindings.at(id);
    if ((raw == 61 || raw == 64) && next.human_page_answers.count(id))
        return Error::invalid_page; // 等实际父页恢复轮消费答案，不能提前执行下一个输入。
    auto &chosen = next.human_page_selections.at(id);
    auto &phase = next.page_phases.at(id);
    auto &g = next.scene.world.world.ai.growth.at(human);
    const auto clear_profession_hints = [&] {
        for (const int job : next.human_page_catalogs.at(id))
            next.scripts.professions.at(job).pending_notice = false;
    };
    if (raw == 60) {
        if(action==A::scroll_relative||action==A::scroll_absolute) {
            StartupHumanCatalogueInput input;input.scroll=StartupHumanCatalogueInput::Scroll{action==A::scroll_absolute,selection};
            return input_startup_world_human_detail_page(s,id,input);
        } else if (action == A::track) {
            // Steam ProcChaseCommandFromInfoWindow按humans_原序取首个同定义W。
            // 没有W是业务提示137，不关闭详情，也不移动人物或镜头。
            const auto &ai = next.scene.world.world.ai;
            std::optional<ref::CharacterId> target;
            for (const auto actor_id : ai.human_order) {
                const auto actor = ai.battle.actors.find(actor_id);
                if (actor == ai.battle.actors.end() || !(actor->second.id == actor_id) ||
                    actor->second.kind != ref::ActorKind::human || !next.actor_metadata.count(actor_id))
                    return Error::missing_source;
                if (actor->second.definition == human) {
                    target = actor_id;
                    break;
                }
            }
            if (!target) {
                const auto details = startup_world_human_details(next, human);
                if (!details || !event(next, 137, details->name))
                    return Error::script_failed;
            } else {
                if (!activate_startup_world_main_page(next)) return Error::script_failed;
                next.scripts.selected_actor = target->value;
                next.scripts.selected_facility.reset();
                next.scripts.selection_mode = 1;
                next.scene.scene_state = 6;
                next.scene.scene_counter = 0;
            }
        } else if (action == A::view_tab) {
            if (selection < 0 || selection > 3)
                return Error::invalid_page;
            if(selection==2&&phase!=2) {
                if(phase==1)chosen=0;
                else if(phase==3)chosen=3;
            }
            phase = selection;
        } else if(action==A::select) {
            if(phase!=2||selection<0||selection>3)return Error::invalid_page;
            chosen=selection;
        } else if (action == A::previous || action == A::next) {
            if(phase==2) {
                if(action==A::previous) {if(chosen>0)--chosen;else phase=1;}
                else {if(chosen<3)++chosen;else phase=3;}
            } else {
                const int previous=phase;
                phase=(phase+(action==A::previous?3:1))%4;
                if(phase==2)chosen=previous==3?3:0;
            }
        } else if (action == A::cancel || action == A::confirm) {
            if (!close(next, id))
                return Error::script_failed;
        } else if (action == A::professions) {
            if ((!ref::world_script_seen(next.scripts, 112) && !event(next, 112)) ||
                !open(next, 61, human))
                return Error::script_failed;
        } else if (action == A::gifts) {
            std::optional<int> recovery_selection;
            if(phase==0) {
                const auto recovery=startup_world_human_recovery_gift(next,human);
                if(!recovery)return Error::missing_source;
                if(*recovery) {
                    int prefix{};
                    for(const auto &item:next.rules->items) {
                        if(item.recovery>0)break;
                        const auto stock=next.items.find(item.identity);
                        if(stock==next.items.end()||stock->second.inventory<0)return Error::missing_source;
                        if(stock->second.inventory>0)++prefix;
                    }
                    recovery_selection=prefix;
                }
            }
            const auto child=open(next,64,human);
            if(!child)
                return Error::script_failed;
            if(phase==2)next.page_phases[*child]=chosen;
            else if(recovery_selection) {
                next.page_phases[*child]=4;next.human_page_selections[*child]=*recovery_selection;
            }
        } else if (action == A::inspect_equipment) {
            if (selection < 0 || selection > 3)
                return Error::invalid_page;
            const auto child = open(next, 64, human);
            if (!child)
                return Error::script_failed;
            next.page_phases[*child] = selection;
        } else
            return Error::invalid_page;
    } else if (raw == 61 || raw == 62) {
        const auto &list = next.human_page_catalogs.at(id);
        if (chosen < 0 || chosen >= static_cast<int>(list.size()))
            return Error::missing_source;
        if(raw==61&&action==A::view_tab) {
            if(selection<0||selection>1)return Error::invalid_page;
            phase=selection; // 两统计页共享目录，原选择／scroll保持。
        } else if(raw==61&&(action==A::scroll_relative||action==A::scroll_absolute)) {
            StartupHumanCatalogueInput input;input.scroll=StartupHumanCatalogueInput::Scroll{action==A::scroll_absolute,selection};
            return input_startup_world_human_catalogue_page(s,id,input);
        } else if (action == A::previous || action == A::next || action == A::select) {
            const int size = static_cast<int>(list.size());
            const int target = action == A::select
                                   ? selection
                                   : (chosen + (action == A::previous ? size - 1 : 1)) % size;
            if (target < 0 || target >= size)
                return Error::invalid_page;
            chosen = target;
            if(raw==61) {
                auto &first=next.human_page_scroll.at(id);
                if(first>chosen)first=chosen;else if(first+5<=chosen)first=chosen-4;
            }
            if (raw == 62) {
                next.page_job_bindings[id] = list[chosen];
                const auto preview = ref::preview_human_profession_change(
                    g.definition, next.scene.world.world.ai.professions, list[chosen]);
                if (!preview)
                    return Error::missing_source;
                next.human_attribute_display = {preview->before, preview->after,
                                                preview->difference};
            }
        } else if (action == A::cancel) {
            if (raw == 61)
                clear_profession_hints();
            else {
                const auto parent = live_parent(next, id, 61, human);
                if (!parent)
                    return Error::missing_source;
                next.human_page_answers[*parent] = 1;
            }
            if (!close(next, id))
                return Error::script_failed;
        } else if (action == A::confirm) {
            const int job = list[chosen];
            const auto definitions = professions(next);
            const auto &target = definitions.at(job);
            const auto gate = ref::check_human_profession_change(
                raw == 61 ? ref::HumanProfessionEntry::catalogue61
                          : ref::HumanProfessionEntry::confirmation62,
                g.definition.current_profession, next.human_calendar.at(human).celebrations,
                next.village_points, target);
            if (gate.error != ref::HumanGrowthError::none)
                return Error::missing_source;
            if (gate.denial != ref::HumanManagementDenial::none) {
                if (!denial(next, gate.denial, &next.rules->jobs.at(job)))
                    return Error::script_failed;
            } else if (raw == 61) {
                clear_profession_hints();
                const auto child = open(next, 62, human);
                if (!child)
                    return Error::script_failed;
                next.page_job_bindings[*child] = job;
                next.human_page_parents[*child] = id;
            } else {
                const auto parent = live_parent(next, id, 61, human);
                const auto history = next.human_profession_changes.find(human);
                if (!parent || history == next.human_profession_changes.end() ||
                    history->second.size() != definitions.size())
                    return Error::missing_source;
                const auto change = ref::prepare_human_profession_change(
                    {g.definition, next.scene.world.world.ai.professions, target,
                     next.village_points, next.human_calendar.at(human).celebrations,
                     history->second.at(job), next.shop_humans.at(human).satisfaction,
                     next.scene.world.world.ai.pending_completion});
                if (!change.candidate)
                    return Error::missing_source;
                const auto &c = *change.candidate;
                next.village_points = c.village_points;
                const auto child = open(next, 63, human);
                if (!child)
                    return Error::script_failed;
                next.page_job_bindings[*child] = job;
                if (!reward(next, human, c.reward) || !reward_page(next, human, c.reward))
                    return Error::script_failed;
                next.human_profession_changes.at(human).at(job) = c.prior_changes;
                next.human_page_answers[*parent] = 0;
                if (!close(next, id))
                    return Error::script_failed;
            }
        } else
            return Error::invalid_page;
    } else if (raw == 64 || raw == 73) {
        if (phase < 0 || phase > (raw == 64 ? 4 : 3))
            return Error::missing_source;
        const auto &list = next.equipment_page_catalogs.at(id)[phase];
        if (action == A::equipment_slot && raw == 64) {
            if (selection < 0 || selection > 4)
                return Error::invalid_page;
            if(phase!=selection) {phase = selection;chosen = 0;next.human_page_scroll.at(id)=0;}
        } else if(raw==64&&(action==A::scroll_relative||action==A::scroll_absolute)) {
            StartupHumanCatalogueInput input;input.scroll=StartupHumanCatalogueInput::Scroll{action==A::scroll_absolute,selection};
            return input_startup_world_human_catalogue_page(s,id,input);
        } else if (action == A::previous || action == A::next || action == A::select) {
            if (list.empty())
                return Error::invalid_page;
            const int size = static_cast<int>(list.size());
            const int target = action == A::select
                                   ? selection
                                   : (chosen + (action == A::previous ? size - 1 : 1)) % size;
            if (target < 0 || target >= size)
                return Error::invalid_page;
            chosen = target;
            if(raw==64) {
                auto &first=next.human_page_scroll.at(id);
                if(first>chosen)first=chosen;else if(first+4<=chosen)first=chosen-3;
            }
        } else if (action == A::cancel || (action == A::confirm && raw == 73)) {
            if (raw == 64 && !clear_startup_world_item_notices(next))
                return Error::missing_source;
            if (!close(next, id))
                return Error::script_failed;
        } else if (action == A::confirm || action == A::inspect_equipment) {
            if (chosen < 0 || chosen >= static_cast<int>(list.size()))
                return Error::invalid_page;
            const int item = list[chosen];
            if (phase == 4) {
                if (action != A::confirm)
                    return Error::invalid_page;
                if (!commit_item(next, id, human, item))
                    return Error::missing_source;
                next.scripts.executing_page.reset();
                s = std::move(next);
                return Error::none;
            }
            const auto target = equipment(next, phase, item);
            const auto cost =
                target ? ref::human_equipment_gift_cost(*target) : std::optional<int>{};
            if (!cost)
                return Error::missing_source;
            if (action == A::confirm && *cost != -1 &&
                next.scene.world.world.ai.accounting.funds() < *cost) {
                if (!event(next, 11))
                    return Error::script_failed;
            } else {
                if (action == A::confirm)
                    next.catalog.at({phase == 0   ? 1
                                     : phase == 3 ? 3
                                                  : 2,
                                     item})
                        .newly_unlocked = false;
                const auto child = open(next, action == A::confirm ? 65 : 73, human);
                if (!child)
                    return Error::script_failed;
                next.human_equipment_choices[id] =
                    next.human_equipment_choices[*child] = {phase, item};
                next.page_phases[*child] = phase;
                next.human_page_parents[*child] = id;
            }
        } else
            return Error::invalid_page;
    } else if (raw == 65) {
        if (action != A::confirm && action != A::cancel)
            return Error::invalid_page;
        const auto parent = live_parent(next, id, 64, human);
        if (!parent || !next.human_equipment_choices.count(id) ||
            !next.human_equipment_choices.count(*parent) ||
            next.human_equipment_choices.at(id) != next.human_equipment_choices.at(*parent))
            return Error::missing_source;
        next.human_page_answers[*parent] = action == A::confirm ? 0 : 1;
        if (!close(next, id))
            return Error::script_failed;
    } else if (raw == 70) {
        if (phase < 0 || phase > 2)
            return Error::missing_source;
        if (action == A::previous || action == A::next || action == A::select) {
            if (phase != 2 || (action == A::select && (selection < 0 || selection > 1)))
                return Error::invalid_page;
            chosen = action == A::select ? selection : 1 - chosen;
        } else if (action == A::confirm || action == A::cancel) {
            auto &counter = next.page_counters.at(id);
            if (counter < 40)
                counter = 40;
            else if (phase < 2) {
                ++phase;
                counter = 0;
                if (phase == 2 && !ref::world_script_seen(next.scripts, 119) &&
                    !event(next, 119, next.rules->jobs.at(g.definition.current_profession).name))
                    return Error::script_failed;
            } else {
                const auto &job = next.rules->jobs.at(g.definition.current_profession);
                const auto mastery = ref::prepare_human_mastery_bonus(
                    g.definition, next.scene.world.world.ai.professions, job.mastery_attribute,
                    job.mastery_value);
                if (!mastery.candidate)
                    return Error::missing_source;
                g.definition = mastery.candidate->definition;
                g.derived = mastery.candidate->stats;
                if (!synchronize_startup_world_human_capacity(next, human))
                    return Error::missing_source;
                if ((chosen == 0 && !open(next, 61, human)) || !close(next, id))
                    return Error::script_failed;
            }
        } else
            return Error::invalid_page;
    } else {
        if (action != A::confirm)
            return Error::invalid_page;
        if (!consume_display(next, id, raw, true))
            return Error::missing_source;
    }
    next.scripts.executing_page.reset();
    s = std::move(next);
    return Error::none;
}

std::optional<State> update_startup_world_human_page(const State &s, std::uint64_t id) {
    const auto *p = top(s);
    if (!p || p->id != id || p->kind != ref::WorldScriptPageKind::raw_page)
        return {};
    if (s.scene.framework_paused)
        return s;
    auto next = s;
    next.scripts.executing_page = id;
    const int raw = p->legacy_page;
    if (!initialize(next, id, raw))
        return {};
    auto &counter = next.page_counters.at(id);
    if (counter < 0 || counter == std::numeric_limits<int>::max())
        return {};
    ++counter;
    if(raw==70) {
        counter%=std::numeric_limits<int>::max();
        auto &secondary=next.page_secondary_counters.at(id);
        secondary=(secondary+1)%std::numeric_limits<int>::max();
    }
    if (raw == 60 && !valid_startup_world_human_detail_context(next, id))
        return {};
    if (!resume_answer(next, id, raw, next.page_human_bindings.at(id)) ||
        !consume_display(next, id, raw, false))
        return {};
    next.scripts.executing_page.reset();
    return next;
}

StartupWorldResourceUsage startup_world_resource_usage(const State &s) {
    const auto &ai = s.scene.world.world.ai;
    StartupWorldResourceUsage r;
    r.live_actors = ai.battle.actors.size();
    r.retired_actors = ai.retired_actors.size();
    r.live_encounters = ai.encounters.size();
    r.retired_encounters = ai.retired_encounters.size();
    r.facilities = s.scene.world.world.facilities.size();
    r.pending_tasks = s.task_order.size();
    r.retained_tasks = s.tasks.size();
    r.pages = s.scripts.pages.size();
    // 只统计已列载荷容器的实例数，不含vector动态字节，也不是完整内存审计。
    r.page_payloads = s.page_counters.size() + s.page_secondary_counters.size() + s.task_page_ui.size() +
                      s.menu_page_data.size() + s.menu_page_positions.size() +
                      s.page_phases.size() + s.page_human_bindings.size() +
                      s.human_pages_initialized.size() + s.human_page_catalogs.size() +
                      s.equipment_page_catalogs.size() + s.human_page_selections.size() +s.human_page_scroll.size()+
                      s.page_job_bindings.size() + s.human_page_parents.size() +
                      s.human_detail_contexts.size() +
                      s.human_page_answers.size() + s.human_equipment_choices.size() +
                      s.human_gift_scores.size() + s.human_gift_messages.size() +
                      s.tax_page_residents.size() + s.tax_page_selection.size() +
                      s.tax_page_scroll.size() + s.activity_pages_initialized.size() +
                      s.activity_page_bindings.size() + s.activity_page_lists.size() +
                      s.activity_page_display_humans.size() + s.activity_page_parents.size() +
                      s.activity_page_answers.size() + s.activity_page_selections.size() +
                      s.activity_page_scroll.size() + s.facility_item_page_scroll.size() + s.facility_item_pages_initialized.size() +
                      s.facility_item_page_items.size() + s.facility_item_page_lists.size() +
                      s.facility_item_page_selections.size() + s.commerce_pages_initialized.size() +
                      s.commerce_page_data.size() + s.commerce_page_lists.size() +
                      s.facility_catalog_page_data.size() + s.facility_catalog_page_lists.size() +
                      s.facility_catalog_page_parents.size() + s.facility_catalog_pages_initialized.size() +
                      s.magic_pot_page_data.size() + s.magic_pot_page_lists.size() +
                      s.magic_pot_page_parents.size() + s.magic_pot_pages_initialized.size() +
                      s.facility_definition_page_bindings.size() + s.facility_page_bindings.size() +
                      s.facility_page_neighbours.size() + s.build_page_catalogs.size() +
                      s.residence_page_candidates.size() + s.facility_upgrade_initialized.size();
    r.sound_outputs = s.sound_requests.size();
    r.effects = s.visual_effects.size() + s.delayed_effects.size() + s.global_effects.size();
    r.continuations = s.scripts.continuations.size();
    return r;
}
} // namespace dungeon_village_prototype
