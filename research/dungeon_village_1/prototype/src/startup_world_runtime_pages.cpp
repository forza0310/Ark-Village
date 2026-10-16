#include "dungeon_village_prototype/startup_world_building.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "dungeon_village_prototype/startup_world_facility_items.hpp"
#include "dungeon_village_prototype/startup_world_facility_catalog.hpp"
#include "dungeon_village_prototype/startup_world_magic_pot.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_information.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"
#include "dungeon_village_prototype/startup_world_save.hpp"
#include "dungeon_village_prototype/startup_world_manual.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_runtime_tasks.hpp"
#include "dungeon_village_prototype/startup_world_tax.hpp"
#include "dungeon_village_prototype/startup_world_village_activity.hpp"
#include "dungeon_village_reference/world_gift_page.hpp"

#include <algorithm>
#include <set>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State = StartupWorldRuntimeState;
using Error = StartupWorldRuntimeError;
bool valid_reward_display(const State &s) {
    // 原n.aH共享旧/新/请求三行；两种奖励页都不能从当前人物反推或修补。
    for(std::size_t column=0;column<2;++column) {
        const int before=s.reward_display[0][column],after=s.reward_display[1][column],request=s.reward_display[2][column];
        if(before<0||before>100||after<0||after>100||request<0||
           after!=std::min<std::int64_t>(100,static_cast<std::int64_t>(before)+request))return false;
    }
    return true;
}
bool unlock_human_valid(const State &s, const ref::WorldScriptPage &p) {
    // b/g:3467的X仅含bv[f]；aq由到访排序读取，不在此页直接创建人物实例。
    return s.rules && s.human_calendar.count(p.legacy_f) &&
           std::any_of(s.rules->humans.begin(), s.rules->humans.end(),
                       [&](const auto &h) { return h.identity == p.legacy_f; });
}
std::optional<int> event_message_command(const State &s, const ref::WorldScriptPage &p) {
    const auto phase = s.page_phases.find(p.id);
    const int index = phase == s.page_phases.end() ? 0 : phase->second;
    if (p.message_commands.empty() || p.message_commands.size() != p.paragraphs.size() ||
        index < 0 || static_cast<std::size_t>(index) >= p.message_commands.size())
        return {};
    return p.message_commands.at(static_cast<std::size_t>(index));
}
bool consume_task_display(State &s, const ref::WorldScriptPage &page,
                          ref::WorldTaskDisplayAction action) {
    if (!s.rules||!page.monster_definition || *page.monster_definition < 0 ||
        *page.monster_definition >= static_cast<int>(s.rules->monsters.size()))
        return false;
    const auto count=s.page_counters.find(page.id);
    if(s.task_display_initialized.count(page.id)&&
       (count==s.page_counters.end()||count->second<0||count->second==std::numeric_limits<int>::max()))
        return false; // 已Init载荷损坏不能用[]重新造主钟。
    const ref::WorldTaskDisplayState display{
        page.legacy_page, s.task_display_initialized.count(page.id) != 0, s.task_display_table};
    const auto result = ref::prepare_world_task_display_page(
        display, {action, s.page_counters[page.id]}, s.scene.random);
    if (!result.candidate)
        return false;
    s.task_display_table = result.candidate->state.bd;
    s.scene.random = result.candidate->random;
    s.task_display_initialized.insert(page.id);
    if (result.candidate->closed) {
        const auto r =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), page.id);
        if (!r.candidate || !write_startup_world_runtime_scripts(s, r.candidate->state))
            return false;
    }
    return true;
}
ref::WorldGiftPageState gift(const State &s, const ref::WorldRuntimeAdapter<State> &adapter) {
    ref::WorldGiftPageState g;
    g.facility = adapter.facilities.read(s);
    g.village_points = s.village_points;
    g.facility_order = s.scene.world.facility_order;
    for (const auto &d : s.rules->facilities) {
        const auto value = [](const auto &map, int id) {
            const auto found = map.find(id);
            return found == map.end() ? 0 : static_cast<int>(found->second);
        };
        g.facility_unlocks.emplace(
            d.id, ref::WorldGiftFacilityDefinition{s.facility_presence.at(d.id),
                                                   value(s.facility_unlock_notices, d.id) != 0,
                                                   value(s.facility_unlock_counters, d.id),
                                                   value(s.facility_free_builds, d.id)});
    }
    return g;
}
bool write_gift(State &s, const ref::WorldGiftPageState &g,
                const ref::WorldRuntimeAdapter<State> &adapter) {
    if (!adapter.facilities.write(s, g.facility))
        return false;
    s.village_points = g.village_points;
    for (const auto &d : g.facility_unlocks) {
        s.facility_presence.at(d.first) = d.second.status;
        s.facility_unlock_notices[d.first] = d.second.pending_notice;
        s.facility_unlock_counters[d.first] = d.second.unlock_counter;
        s.facility_free_builds[d.first] = d.second.free_builds;
    }
    return true;
}
bool initialize_rank_page(State &s, std::uint64_t id) {
    if (s.page_counters.count(id))
        return s.page_phases.count(id)&&s.page_counters.at(id)>=0&&s.page_phases.at(id)>=0&&
               s.page_phases.at(id)<=4;
    const auto source=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &page){return page.id==id;});
    if(!s.rules||source==s.scripts.pages.end()||source->lifecycle!=0||s.page_phases.count(id)||
       s.page_secondary_counters.count(id)||s.rank_celebration_participants.count(id))return false;
    if (s.rank >= 5) {
        const auto script = ref::prepare_world_script(
            startup_world_runtime_catalog(), startup_world_runtime_scripts(s), {48, {}, {}});
        if (!script.candidate || !write_startup_world_runtime_scripts(s, script.candidate->state))
            return false;
        const auto closed =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
        return closed.candidate && write_startup_world_runtime_scripts(s, closed.candidate->state);
    }
    if (!refresh_startup_world_runtime_rank(s))
        return false;
    s.page_counters[id] = 0;
    s.page_phases[id] = 0;
    return true;
}
bool initialize_rank_celebration(State &s, std::uint64_t id) {
    const auto source=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &page){return page.id==id;});
    if (s.rank_celebration_participants.count(id))
        return source!=s.scripts.pages.end()&&source->lifecycle!=0&&valid_startup_world_town_progress_page(s,*source);
    if(!s.rules||source==s.scripts.pages.end()||source->lifecycle!=0||s.page_phases.count(id)||
       s.page_counters.count(id)||s.page_secondary_counters.count(id))return false;
    std::vector<int> definitions;
    for (const auto &h : s.rules->humans)
        if (!s.human_presence.count(h.identity))return false;
        else if (s.human_presence.at(h.identity) == 1)
            definitions.push_back(h.identity);
    const auto result = ref::prepare_world_rank_celebration(definitions, s.scene.random);
    if (!result)
        return false;
    s.rank_celebration_participants[id] = result->participants;
    s.scene.random = result->random;
    s.page_phases[id] = s.page_counters[id] = 0;
    s.page_secondary_counters[id] = 0;
    return true;
}
bool consume_rank_celebration(State &s, std::uint64_t id, bool confirm) {
    if (!initialize_rank_celebration(s, id))
        return false;
    auto &phase = s.page_phases[id];
    auto &counter = s.page_counters[id];
    if (phase < 0 || phase > 2)
        return false;
    if (phase == 0 && counter == 1 && !confirm)
        s.sound_requests.push_back({StartupAudioOperation::replace_bgm, 3});
    if (phase < 2 && counter >= (phase == 0 ? 135 : 20)) {
        ++phase;
        counter = 0;
    }
    if (confirm && phase == 2 && counter >= 140) {
        s.sound_requests.push_back({StartupAudioOperation::replace_bgm, s.active_task && s.task.encounter ? 2 : 1});
        const auto closed =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
        return closed.candidate && write_startup_world_runtime_scripts(s, closed.candidate->state);
    }
    return true;
}
ref::WorldAwardPageState award_projection(const State &s, std::uint64_t id) {
    ref::WorldAwardPageState a;
    a.medal_count = s.medal_count;
    const auto counter = s.page_counters.find(id);
    if (counter != s.page_counters.end())
        a.page_counter = counter->second;
    const auto rankings = s.award_rankings.find(id);
    if (rankings != s.award_rankings.end()) {
        a.initialized = true;
        a.ranked_definitions = rankings->second;
        a.announced = s.award_announced.at(id);
        a.termination_pending = s.award_termination_pending.at(id);
        const auto pending = s.award_pending_humans.find(id);
        if (pending != s.award_pending_humans.end())
            a.pending_award = pending->second;
    }
    for (const auto &h : s.rules->humans) {
        const auto &extra = s.human_calendar.at(h.identity);
        auto totals = extra.yearly_totals;
        totals[1] = s.scene.world.world.ai.battle.humans.at(h.identity).killed_stat1;
        totals[2] = s.scene.world.world.human_spending.at(h.identity);
        a.humans.push_back(
            {h.identity, s.human_presence.at(h.identity), totals, extra.contribution});
    }
    return a;
}
bool award_reward(State &s, int human) {
    auto &ai = s.scene.world.world.ai;
    const auto growth = ai.growth.find(human);
    if (growth == ai.growth.end() || !s.shop_humans.count(human) || !s.human_calendar.count(human))
        return false;
    const auto r = ref::prepare_human_reward(
        growth->second.definition, ai.professions, s.shop_humans.at(human).satisfaction,
        s.human_calendar.at(human).celebrations, ai.pending_completion, 10, 10, true);
    if (!r.candidate)
        return false;
    const auto &reward = *r.candidate;
    growth->second.definition = reward.definition;
    s.shop_humans.at(human).satisfaction = reward.satisfaction;
    s.human_calendar.at(human).celebrations = reward.celebrations;
    ai.pending_completion = reward.pending_completion;
    s.reward_display = reward.reward_display;
    if (reward.derived) {
        growth->second.derived = *reward.derived;
        if (!synchronize_startup_world_human_capacity(s, human))
            return false;
        s.effort_display = *reward.effort_display;
    }
    const auto event = ref::prepare_world_script(startup_world_runtime_catalog(),
                                                 startup_world_runtime_scripts(s), {58, {}, {}});
    if (!event.candidate || !write_startup_world_runtime_scripts(s, event.candidate->state))
        return false;
    for (const int raw : {88, 67}) {
        if (raw == 67 && !reward.effort_display)
            continue;
        ref::WorldScriptPage p;
        p.kind = ref::WorldScriptPageKind::raw_page;
        p.legacy_page = raw;
        if (raw == 67) {
            p.legacy_f = reward.reward_display[0][1];
            p.legacy_g = reward.reward_display[1][1];
        }
        const auto page = ref::prepare_world_script_page(startup_world_runtime_scripts(s), p);
        if (!page.candidate || !write_startup_world_runtime_scripts(s, page.candidate->state))
            return false;
        for (const auto &inserted : page.candidate->inserted_pages) {
            if(!s.page_human_bindings.emplace(inserted.id,human).second||
               !s.page_phases.emplace(inserted.id,0).second||!s.page_counters.emplace(inserted.id,0).second||
               !(raw==67?valid_startup_world_runtime_effort_result_page(s,inserted):
                         valid_startup_world_runtime_award_result_page(s,inserted)))return false;
        }
    }
    return true;
}
bool consume_award(State &s, std::uint64_t id, ref::WorldAwardAction action, int selection = 0) {
    const auto initialized = ref::prepare_world_award_page_initialization(award_projection(s, id));
    if (!initialized.candidate)
        return false;
    const auto result =
        ref::prepare_world_award_page(initialized.candidate->state, action, selection);
    if (!result.candidate)
        return false;
    const auto &a = result.candidate->state;
    s.medal_count = a.medal_count;
    s.award_rankings[id] = a.ranked_definitions;
    s.award_announced[id] = a.announced;
    s.award_termination_pending[id] = a.termination_pending;
    s.page_counters[id] = a.page_counter;
    if (a.pending_award)
        s.award_pending_humans[id] = *a.pending_award;
    else
        s.award_pending_humans.erase(id);
    for (const auto &h : a.humans)
        s.human_calendar.at(h.definition).contribution = h.contribution;
    for (const auto &effect : result.candidate->effects) {
        using Kind = ref::WorldAwardEffectKind;
        if (effect.kind == Kind::sound)
            s.sound_requests.push_back({StartupAudioOperation::replace_bgm, effect.value});
        else if (effect.kind == Kind::refresh)
            s.sound_requests.push_back({StartupAudioOperation::replace_bgm, s.active_task && s.task.encounter ? 2 : 1});
        else if (effect.kind == Kind::event) {
            const auto script = ref::prepare_world_script(
                startup_world_runtime_catalog(), startup_world_runtime_scripts(s),
                {effect.value,
                 effect.event_argument ? std::to_string(*effect.event_argument) : "",
                 {}});
            if (!script.candidate ||
                !write_startup_world_runtime_scripts(s, script.candidate->state))
                return false;
        } else if (effect.kind == Kind::reward) {
            if (!award_reward(s, effect.value))
                return false;
        } else if (effect.kind == Kind::close) {
            const auto closed =
                ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
            if (!closed.candidate ||
                !write_startup_world_runtime_scripts(s, closed.candidate->state))
                return false;
        }
        // termination_prompt由独立命令/窗口是非输入消费，不把普通确认当作终止。
    }
    return true;
}
bool consume_award_display(State &s, std::uint64_t id, bool confirm, int raw) {
    const auto binding = s.page_human_bindings.find(id);
    if (binding == s.page_human_bindings.end() || !s.human_calendar.count(binding->second))
        return false;
    bool close{};
    if (raw == 67) {
        const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
        if(page==s.scripts.pages.end()||page->lifecycle!=2||!valid_startup_world_runtime_effort_result_page(s,*page))return false;
        const auto r =
            ref::prepare_world_effort_display(s.page_counters.at(id), confirm, s.effort_display[2]);
        if (!r)
            return false;
        s.page_counters.at(id) = r->counter;
        close = r->closed;
    } else {
        const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &p){return p.id==id;});
        if(raw!=88||page==s.scripts.pages.end()||page->lifecycle!=2||
           !valid_startup_world_runtime_award_result_page(s,*page))return false;
        const auto r = ref::prepare_world_award_display({s.page_counters.at(id), s.page_phases.at(id)},
                                                        confirm, s.scene.random);
        if (!r)
            return false;
        s.page_counters.at(id) = r->state.counter;
        s.page_phases.at(id) = r->state.phase;
        s.scene.random = r->random;
        if (r->event) {
            const auto event =
                ref::prepare_world_script(startup_world_runtime_catalog(),
                                          startup_world_runtime_scripts(s), {*r->event, {}, {}});
            if (!event.candidate || !event.candidate->last_page ||
                !write_startup_world_runtime_scripts(s, event.candidate->state))
                return false;
            const auto page =
                std::find_if(s.scripts.pages.begin(), s.scripts.pages.end(),
                             [&](const auto &p) { return p.id == *event.candidate->last_page; });
            if (page == s.scripts.pages.end())
                return false;
            page->speaker_kind = 1;
            page->speaker_definition = binding->second;
            close = true;
        }
    }
    if (close) {
        const auto r = ref::prepare_world_script_close_page(startup_world_runtime_scripts(s), id);
        if (!r.candidate || !write_startup_world_runtime_scripts(s, r.candidate->state))
            return false;
    }
    return true;
}
} // namespace

bool valid_startup_world_runtime_task_summary_ui_page(const State &s,const ref::WorldScriptPage &p) {
    const int raw=p.legacy_page;const auto id=p.id;
    if(!s.rules||id==0||p.kind!=ref::WorldScriptPageKind::raw_page||raw<31||raw>33||p.lifecycle<0||p.lifecycle>4)return false;
    const auto human=[&](int h){return h>=0&&s.human_calendar.count(h)&&s.scene.world.world.ai.battle.humans.count(h)&&
        std::any_of(s.rules->humans.begin(),s.rules->humans.end(),[=](const auto &v){return v.identity==h;});};
    const auto task_definition=[&](int d){return s.task_progress.definitions.count(d)&&
        std::any_of(s.rules->tasks.begin(),s.rules->tasks.end(),[=](const auto &v){return v.factory.identity==d;});};
    std::size_t count{};
    if(raw==31) {
        if(!p.task_identity||!p.task_definition||!task_definition(*p.task_definition))return false;
        const auto task=s.tasks.find(*p.task_identity);
        if(task==s.tasks.end()||task->first==0||task->second.identity!=task->first||task->second.definition!=*p.task_definition)return false;
        const auto list=s.crew_summaries.find(id);
        if(list!=s.crew_summaries.end()) {
            count=list->second.size();
            for(int h:list->second)if(!human(h))return false; // 原X允许重复，不能去重。
        } else if(p.lifecycle!=0||s.task_page_ui.count(id))return false;
    } else if(raw==32) {
        const auto summary=s.exploration_summaries.find(id);
        if(summary==s.exploration_summaries.end()||summary->second.legacy_page!=32||!task_definition(summary->second.definition))return false;
        const auto task=s.tasks.find(summary->second.task);
        if(task==s.tasks.end()||task->first==0||task->second.identity!=task->first||task->second.definition!=summary->second.definition)return false;
        if(bool(p.task_identity)!=bool(p.task_definition)||(p.task_identity&&
           (*p.task_identity!=summary->second.task||*p.task_definition!=summary->second.definition)))return false;
        count=summary->second.rewards.size();
        for(const auto &row:summary->second.rewards) {
            if(row[0]<0||row[0]>3||row[1]<0||!s.catalog.count({row[0],row[1]}))return false;
        }
    } else {
        if(p.task_definition&&!task_definition(*p.task_definition))return false;
        if(p.task_identity) {
            if(!p.task_definition)return false;
            const auto task=s.tasks.find(*p.task_identity);
            if(task==s.tasks.end()||task->first==0||task->second.identity!=task->first||task->second.definition!=*p.task_definition)return false;
        }
        for(int h:s.participants)if(!human(h))return false;
    }
    if(raw!=33&&((p.legacy_f!=-1&&!human(p.legacy_f))||(p.legacy_g!=-1&&!human(p.legacy_g))))return false;
    if(count>65536)return false;
    const auto ui=s.task_page_ui.find(id),end=s.task_page_ui.end();
    const auto frame=s.page_counters.find(id),phase=s.page_phases.find(id);
    if(s.page_secondary_counters.count(id))return false;
    if(ui==end) {
        return p.lifecycle==0&&frame==s.page_counters.end()&&phase==s.page_phases.end()&&
               (raw!=31||!s.crew_summaries.count(id))&&
               (raw!=33||(!s.deadline_initialized.count(id)&&!s.deadline_grades.count(id)&&!s.deadline_returns.count(id)));
    }
    const auto &v=ui->second;
    if(v.selection<0||v.first<0||v.tab!=0||!v.member_rows.empty()||v.parent||v.answer||
       frame==s.page_counters.end()||phase==s.page_phases.end()||frame->second<0||frame->second==std::numeric_limits<int>::max())return false;
    if(p.lifecycle==0&&(v.selection!=0||v.first!=0||frame->second!=0||phase->second!=0))return false;
    if(raw==31) {
        if(!s.crew_summaries.count(id)||phase->second!=0)return false;
        return count?(std::size_t(v.selection)<count&&v.first<=v.selection&&v.selection-v.first<5):
                     (v.selection==0&&v.first==0);
    }
    if(raw==32)return phase->second==0&&v.selection==0&&v.first<=std::max(0,int(count)-5);
    const auto grade=s.deadline_grades.find(id),answer=s.deadline_returns.find(id);
    return v.selection<=1&&v.first==0&&s.deadline_initialized.count(id)&&phase->second>=0&&phase->second<=1&&
           grade!=s.deadline_grades.end()&&grade->second>=0&&grade->second<=4&&
           answer!=s.deadline_returns.end()&&answer->second>=-1&&answer->second<=1&&
           (phase->second==0?answer->second==-1:answer->second>=0);
}

bool initialize_startup_world_runtime_task_summary_ui_page(State &s,std::uint64_t id) {
    const auto p=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[=](const auto &v){return v.id==id;});
    if(p==s.scripts.pages.end()||!valid_startup_world_runtime_task_summary_ui_page(s,*p))return false;
    if(s.task_page_ui.count(id))return true;
    if(p->lifecycle!=0)return false;
    if(p->legacy_page==31)return initialize_startup_world_runtime_task_result_page(s,id);
    if(p->legacy_page==33)return initialize_startup_world_runtime_deadline_page(s,id);
    auto next=s;
    next.task_page_ui.emplace(id,StartupTaskPageUi{});next.page_counters.emplace(id,0);next.page_phases.emplace(id,0);
    if(!valid_startup_world_runtime_task_summary_ui_page(next,*p))return false;
    s=std::move(next);return true;
}

StartupWorldTaskPageResult input_startup_world_runtime_task_summary_page(State &s,std::uint64_t id,const StartupTaskPageInput &input) {
    const auto p=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &v){return v.lifecycle!=4;});
    if(!s.rules||s.scene.framework_paused||p==s.scripts.pages.rend()||p->id!=id||p->kind!=ref::WorldScriptPageKind::raw_page||
       p->legacy_page<31||p->legacy_page>33||(p->lifecycle!=0&&p->lifecycle!=2)||input.inspect||input.track||input.depart)return {Error::invalid_page};
    const int raw=p->legacy_page;
    if(raw!=33&&(input.left||input.right))return {Error::invalid_page};
    if(raw==33&&(input.up||input.down||input.scroll||input.cancel))return {Error::invalid_page};
    if(raw==32&&input.selection)return {Error::invalid_page};
    auto next=s;
    if(!initialize_startup_world_runtime_task_summary_ui_page(next,id))return {Error::missing_source};
    auto page=std::find_if(next.scripts.pages.begin(),next.scripts.pages.end(),[=](const auto &v){return v.id==id;});
    page->lifecycle=2;
    auto &v=next.task_page_ui.at(id);
    if(raw==31) {
        const int count=static_cast<int>(next.crew_summaries.at(id).size());
        if(input.selection){if(*input.selection<0||*input.selection>=count)return {Error::invalid_page};v.selection=*input.selection;}
        if(input.scroll) {
            const auto proposed=input.scroll->absolute?std::int64_t(input.scroll->value):std::int64_t(v.first)+input.scroll->value;
            const int moved=static_cast<int>(std::clamp(proposed,std::int64_t(0),std::int64_t(std::max(0,count-5))));
            v.selection+=moved-v.first;v.first=moved;
        }
        if(count){if(input.up)v.selection=(v.selection+count-1)%count;if(input.down)v.selection=(v.selection+1)%count;}
        else if(input.up||input.down)return {Error::invalid_page};
        if(v.first>v.selection)v.first=v.selection;
        if(v.selection-v.first>=5)v.first=v.selection-4;
    } else if(raw==32) {
        const int modulus=std::max(0,int(next.exploration_summaries.at(id).rewards.size())-4);
        if(input.scroll) {
            const auto proposed=input.scroll->absolute?std::int64_t(input.scroll->value):std::int64_t(v.first)+input.scroll->value;
            v.first=static_cast<int>(std::clamp(proposed,std::int64_t(0),std::int64_t(std::max(0,modulus-1))));
        }
        if(modulus){if(input.up)v.first=(v.first+modulus-1)%modulus;if(input.down)v.first=(v.first+1)%modulus;}
    } else if(next.page_phases.at(id)==0) {
        if(input.selection){if(*input.selection<0||*input.selection>1)return {Error::invalid_page};v.selection=*input.selection;}
        if(input.left)v.selection=(v.selection+1)%2;
        if(input.right)v.selection=(v.selection+1)%2;
    } else if(input.selection)return {Error::invalid_page};
    if(!valid_startup_world_runtime_task_summary_ui_page(next,*page))return {Error::missing_source};
    StartupWorldTaskPageResult result;
    if(raw==33&&input.confirm) {
        result=act_startup_world_runtime_deadline_page(next,id,v.selection);
        if(result.error!=Error::none)return result;
    } else if(raw!=33&&(input.confirm||input.cancel)) {
        next.scripts.executing_page=id;
        const auto closed=ref::prepare_world_script_close_page(startup_world_runtime_scripts(next),id);
        if(!closed.candidate||!write_startup_world_runtime_scripts(next,closed.candidate->state))return {Error::script_failed};
        next.scripts.executing_page.reset();
    }
    s=std::move(next);return result;
}

bool valid_startup_world_runtime_award_result_page(const State &s,const ref::WorldScriptPage &p) {
    if(!s.rules||p.id==0||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=88||p.lifecycle<0||p.lifecycle>4)return false;
    const auto count=s.page_counters.find(p.id),phase=s.page_phases.find(p.id);
    const auto human=s.page_human_bindings.find(p.id);
    if(count==s.page_counters.end()||phase==s.page_phases.end()||count->second<0||
       count->second==std::numeric_limits<int>::max()||phase->second<0||phase->second>1||
       s.page_secondary_counters.count(p.id)||human==s.page_human_bindings.end()||
       !s.human_calendar.count(human->second)||!s.shop_humans.count(human->second)||
       !s.scene.world.world.ai.growth.count(human->second)||
       std::none_of(s.rules->humans.begin(),s.rules->humans.end(),[&](const auto &h){return h.identity==human->second;}))return false;
    if(p.lifecycle==0&&(count->second!=0||phase->second!=0))return false;
    // phase1主钟回绕后local可为负；不得以main>=135拒绝原模INT_MAX轨迹。
    return valid_reward_display(s);
}

bool valid_startup_world_runtime_effort_result_page(const State &s,const ref::WorldScriptPage &p) {
    if(!s.rules||p.id==0||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=67||p.lifecycle<0||p.lifecycle>4||
       p.legacy_f<0||p.legacy_f>100||p.legacy_g<0||p.legacy_g>100||p.legacy_g/10<=p.legacy_f/10)return false;
    const auto count=s.page_counters.find(p.id),phase=s.page_phases.find(p.id);
    const auto human=s.page_human_bindings.find(p.id);
    if(count==s.page_counters.end()||phase==s.page_phases.end()||count->second<0||
       count->second==std::numeric_limits<int>::max()||phase->second!=0||s.page_secondary_counters.count(p.id)||
       human==s.page_human_bindings.end()||!s.human_calendar.count(human->second)||
       !s.shop_humans.count(human->second)||!s.scene.world.world.ai.growth.count(human->second)||
       std::none_of(s.rules->humans.begin(),s.rules->humans.end(),[&](const auto &h){return h.identity==human->second;}))return false;
    if(p.lifecycle==0&&count->second!=0)return false;
    // as[0]共享显示不重算人物当前值；原派生只上封9999，差额须在宽整数中核对。
    for(std::size_t column=0;column<4;++column)
        if(s.effort_display[0][column]>9999||s.effort_display[1][column]>9999||
           static_cast<std::int64_t>(s.effort_display[1][column])-s.effort_display[0][column]!=s.effort_display[2][column])return false;
    return true;
}

bool valid_startup_world_runtime_residence_result_page(const State &s,const ref::WorldScriptPage &p) {
    if(!s.rules||p.id==0||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=96||
       p.lifecycle<0||p.lifecycle>4)return false;
    const auto count=s.page_counters.find(p.id),phase=s.page_phases.find(p.id),second=s.page_secondary_counters.find(p.id);
    const auto human=s.page_human_bindings.find(p.id);
    if(count==s.page_counters.end()||phase==s.page_phases.end()||second==s.page_secondary_counters.end()||
       count->second<0||count->second==std::numeric_limits<int>::max()||
       second->second<0||second->second==std::numeric_limits<int>::max()||phase->second<0||phase->second>1||
       human==s.page_human_bindings.end()||!s.human_calendar.count(human->second)||
       !s.shop_humans.count(human->second)||!s.scene.world.world.ai.growth.count(human->second)||
       std::none_of(s.rules->humans.begin(),s.rules->humans.end(),[&](const auto &h){return h.identity==human->second;})||
       std::none_of(s.rules->facilities.begin(),s.rules->facilities.end(),[&](const auto &d){return d.id==p.legacy_f&&d.kind==12;}))return false;
    if(p.lifecycle==0&&(count->second!=0||second->second!=0||phase->second!=0))return false;
    return valid_reward_display(s);
}

bool valid_startup_world_runtime_task_victory_page(const State &s,const ref::WorldScriptPage &p) {
    if(!s.rules||p.id==0||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=30||
       p.lifecycle<0||p.lifecycle>4||p.legacy_f<0)return false;
    const auto summary=s.exploration_summaries.find(p.id);
    const auto count=s.page_counters.find(p.id),phase=s.page_phases.find(p.id),second=s.page_secondary_counters.find(p.id);
    if(summary==s.exploration_summaries.end()||summary->second.legacy_page!=30||
       !summary->second.rewards.empty()||count==s.page_counters.end()||phase==s.page_phases.end()||
       second==s.page_secondary_counters.end()||count->second<0||count->second==std::numeric_limits<int>::max()||
       second->second<0||second->second==std::numeric_limits<int>::max()||phase->second<0||phase->second>1)return false;
    const auto task=s.tasks.find(summary->second.task);
    if(task==s.tasks.end()||task->first==0||task->second.identity!=task->first||
       task->second.definition!=summary->second.definition||
       std::none_of(s.rules->tasks.begin(),s.rules->tasks.end(),[&](const auto &d){return d.factory.identity==summary->second.definition;}))return false;
    // 探索原producer仅在summary保存aa/q；战斗producer另有page绑定，不能反造缺失optional。
    if(bool(p.task_identity)!=bool(p.task_definition)||
       (p.task_identity&&(*p.task_identity!=task->first||*p.task_definition!=task->second.definition)))return false;
    if(p.lifecycle==0&&(count->second!=0||second->second!=0||phase->second!=0))return false;
    return true;
}

bool valid_startup_world_clear_page(const State &s,const ref::WorldScriptPage &p) {
    if(!s.rules||p.id==0||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=17||p.lifecycle<0||p.lifecycle>4)return false;
    const auto stage=s.page_phases.find(p.id),counter=s.page_counters.find(p.id),main=s.page_secondary_counters.find(p.id);
    if(p.lifecycle==0)return stage==s.page_phases.end()&&counter==s.page_counters.end()&&main==s.page_secondary_counters.end();
    if(stage==s.page_phases.end()||counter==s.page_counters.end()||main==s.page_secondary_counters.end()||
       counter->second<0||counter->second==std::numeric_limits<int>::max()||
       main->second<0||main->second==std::numeric_limits<int>::max())return false;
    if(p.lifecycle==4)return stage->second==7&&counter->second==0;
    return stage->second>=0&&stage->second<=6;
}
bool valid_startup_world_gift_notice_page(const State &s,const ref::WorldScriptPage &p) {
    const auto id=p.id;
    if(!s.rules||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=94||p.lifecycle<0||p.lifecycle>4||
       p.legacy_r<3||p.legacy_r>8||p.legacy_s<0)return false;
    const auto giver=s.page_human_bindings.find(id),main=s.page_counters.find(id);
    if(giver==s.page_human_bindings.end()||!startup_world_human_details(s,giver->second))return false;
    // 原94只有主钟。拒绝误挂其它页的演出字段属于维护安全校验，不新增原版收益资格。
    if(s.page_phases.count(id)||s.page_secondary_counters.count(id))return false;
    if(p.lifecycle==0)return main==s.page_counters.end();
    return main!=s.page_counters.end()&&main->second>=0&&main->second<std::numeric_limits<int>::max();
}
bool initialize_startup_world_gift_notice_pages(State &s) {
    for(auto &p:s.scripts.pages)if(p.kind==ref::WorldScriptPageKind::raw_page&&p.legacy_page==94) {
        if(!valid_startup_world_gift_notice_page(s,p))return false;
        if(p.lifecycle==0&&!s.scene.framework_paused) {s.page_counters.emplace(p.id,0);p.lifecycle=1;}
    }
    return true;
}
bool valid_startup_world_town_progress_page(const State &s,const ref::WorldScriptPage &p) {
    const int raw=p.legacy_page;const auto id=p.id;
    if(!s.rules||p.kind!=ref::WorldScriptPageKind::raw_page||
       (raw!=48&&raw!=49&&raw!=50&&raw!=95)||s.rank<0||s.rank>5)return false;
    const auto count=s.page_counters.find(id),phase=s.page_phases.find(id),second=s.page_secondary_counters.find(id);
    const auto records=s.rank_celebration_participants.find(id);
    if(p.lifecycle==4&&(raw==48||raw==49)&&s.rank==5&&count==s.page_counters.end()&&phase==s.page_phases.end())
        return second==s.page_secondary_counters.end()&&records==s.rank_celebration_participants.end();
    if(p.lifecycle==0) {
        if(count!=s.page_counters.end()||phase!=s.page_phases.end()||second!=s.page_secondary_counters.end()||
           records!=s.rank_celebration_participants.end())return false;
    } else {
        if(count==s.page_counters.end()||count->second<0)return false;
        if(raw==48||raw==49) {
            if(phase==s.page_phases.end()||phase->second<0||phase->second>(raw==48?4:0)||
               second!=s.page_secondary_counters.end()||records!=s.rank_celebration_participants.end())return false;
        } else if(raw==50) {
            if(phase==s.page_phases.end()||phase->second<0||phase->second>2||
               count->second==std::numeric_limits<int>::max()||second==s.page_secondary_counters.end()||
               second->second<0||second->second==std::numeric_limits<int>::max()||records==s.rank_celebration_participants.end()||
               records->second.size()>10)return false;
            std::set<int> seen;
            for(const auto &r:records->second) {
                if(!seen.insert(r[0]).second||!s.human_presence.count(r[0])||!s.scene.world.world.ai.growth.count(r[0])||
                   r[3]<0||r[3]>3||r[4]<0||r[4]>1)return false;
            }
        } else if(phase!=s.page_phases.end()||second!=s.page_secondary_counters.end()||records!=s.rank_celebration_participants.end())return false;
    }
    if(raw==95) {
        // r/s属于奖励页命名空间，不能从标题或当前开放状态猜绑定。
        const int kind=p.legacy_r,id_value=p.legacy_s;
        if(kind==11)return s.scripts.activities.count(id_value)!=0;
        if(kind==3)return s.facility_presence.count(id_value)!=0;
        if(kind==4)return s.scripts.professions.count(id_value)!=0;
        if(kind==0||kind==1||kind==9||kind==10)return id_value>=0;
        return false;
    }
    return true;
}
bool initialize_startup_world_town_progress_pages(State &s) {
    std::vector<ref::WorldScriptPage> pending;
    for(const auto &p:s.scripts.pages)
        if(p.lifecycle!=4&&p.kind==ref::WorldScriptPageKind::raw_page&&
           (p.legacy_page==48||p.legacy_page==49||p.legacy_page==50||p.legacy_page==95))pending.push_back(p);
    const auto executing=s.scripts.executing_page;
    for(const auto &p:pending) {
        if(!valid_startup_world_town_progress_page(s,p))return false;
        if(s.scene.framework_paused)continue;
        s.scripts.executing_page=p.id;
        if(p.lifecycle==0) {
            if(p.legacy_page==48||p.legacy_page==49) {if(!initialize_rank_page(s,p.id))return false;}
            else if(p.legacy_page==50) {if(!initialize_rank_celebration(s,p.id))return false;}
            else s.page_counters.emplace(p.id,0);
            const auto page=std::find_if(s.scripts.pages.begin(),s.scripts.pages.end(),[&](const auto &value){return value.id==p.id;});
            if(page==s.scripts.pages.end())return false;
            if(page->lifecycle!=4)page->lifecycle=1;
        }
    }
    s.scripts.executing_page=executing;
    return true;
}

Error act_startup_world_runtime_award_page(State &state, std::uint64_t id,
                                           ref::WorldAwardAction action, int selection) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (state.scene.framework_paused || top == state.scripts.pages.rend() || top->id != id ||
        top->kind != ref::WorldScriptPageKind::raw_page || top->legacy_page != 87)
        return Error::invalid_page;
    auto next = state;
    next.scripts.executing_page = id;
    auto &counter = next.page_counters[id];
    if (counter == std::numeric_limits<int>::max())
        return Error::missing_source;
    ++counter;
    if (!consume_award(next, id, action, selection))
        return Error::missing_source;
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}

namespace {
bool valid_monster_page(const State &s,const ref::WorldScriptPage &p,int raw) {
    if(!s.rules||p.id==0||p.kind!=ref::WorldScriptPageKind::raw_page||p.legacy_page!=raw||
       p.lifecycle<0||p.lifecycle>4||!p.monster_definition||*p.monster_definition<0||
       s.page_phases.count(p.id)||s.page_secondary_counters.count(p.id)||
       std::count_if(s.scripts.pages.begin(),s.scripts.pages.end(),[&](const auto &v){return v.id==p.id;})!=1||
       std::count_if(s.rules->monsters.begin(),s.rules->monsters.end(),
           [&](const auto &d){return d.identity==*p.monster_definition;})!=1)return false;
    const auto count=s.page_counters.find(p.id);
    if(p.lifecycle==0)return count==s.page_counters.end();
    return count!=s.page_counters.end()&&count->second>=0&&count->second<std::numeric_limits<int>::max();
}
} // namespace
bool valid_startup_world_monster_intro_page(const State &s,const ref::WorldScriptPage &p) {
    return valid_monster_page(s,p,89);
}
bool valid_startup_world_monster_detail_page(const State &s,const ref::WorldScriptPage &p) {
    if(!valid_monster_page(s,p,92)||p.task_identity||p.task_definition||p.facility_definition)return false;
    const auto &ai=s.scene.world.world.ai;
    const auto growth=ai.monster_growth.find(*p.monster_definition);
    const auto battle=ai.battle.monsters.find(*p.monster_definition);
    return growth!=ai.monster_growth.end()&&battle!=ai.battle.monsters.end()&&
        growth->second.growth>=0&&battle->second.human_kills>=0;
}
Error open_startup_world_monster_detail_page(State &state,ref::CharacterId actor) {
    const auto top=std::find_if(state.scripts.pages.rbegin(),state.scripts.pages.rend(),
        [](const auto &p){return p.lifecycle!=4;});
    if(!state.rules||state.scene.framework_paused||state.scene.scene_state!=0||
       top==state.scripts.pages.rend()||top->kind!=ref::WorldScriptPageKind::scene)return Error::invalid_page;
    const auto &ai=state.scene.world.world.ai;
    const auto found=ai.battle.actors.find(actor);
    if(found==ai.battle.actors.end()||!(found->second.id==actor)||found->second.kind!=ref::ActorKind::monster||
       std::count(ai.monster_order.begin(),ai.monster_order.end(),actor)!=1)return Error::missing_source;
    if(std::count_if(state.rules->monsters.begin(),state.rules->monsters.end(),
        [&](const auto &m){return m.identity==found->second.definition;})!=1||
       !ai.monster_growth.count(found->second.definition)||!ai.battle.monsters.count(found->second.definition))
        return Error::missing_source;
    auto next=state;next.scripts.executing_page=top->id;
    for(auto &p:next.scripts.pages)if(p.id==top->id)p.lifecycle=3;
    ref::WorldScriptPage page;page.kind=ref::WorldScriptPageKind::raw_page;page.legacy_page=92;
    page.monster_definition=found->second.definition;
    const auto created=ref::prepare_world_script_page(startup_world_runtime_scripts(next),page);
    if(!created.candidate||!write_startup_world_runtime_scripts(next,created.candidate->state))return Error::script_failed;
    const auto inserted=std::find_if(next.scripts.pages.rbegin(),next.scripts.pages.rend(),
        [](const auto &p){return p.lifecycle!=4;});
    if(inserted==next.scripts.pages.rend()||!valid_startup_world_monster_detail_page(next,*inserted))return Error::missing_source;
    next.scripts.executing_page.reset();state=std::move(next);return Error::none;
}
Error acknowledge_startup_world_runtime_page(State &state, std::uint64_t id) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    // 暂停冻结页回调；通用确认与专用管理动作遵守同一框架资格。
    if (state.scene.framework_paused || top == state.scripts.pages.rend() || top->id != id ||
        top->kind == ref::WorldScriptPageKind::scene)
        return Error::invalid_page;
    if (top->kind == ref::WorldScriptPageKind::raw_page && (top->legacy_page == 58||top->legacy_page==17))
        return Error::invalid_page; // 58由逐帧跟踪收尾；17只能由应用Owner计分，均不能通用确认关闭。
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==89&&
       (top->lifecycle!=2||!valid_startup_world_monster_intro_page(state,*top)))return Error::missing_source;
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==92&&
       (top->lifecycle!=2||!valid_startup_world_monster_detail_page(state,*top)))return Error::missing_source;
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==30&&
       (top->lifecycle!=2||!valid_startup_world_runtime_task_victory_page(state,*top)))
        return Error::missing_source;
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==96&&
       (top->lifecycle!=2||!valid_startup_world_runtime_residence_result_page(state,*top)))
        return Error::missing_source;
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==67&&
       (top->lifecycle!=2||!valid_startup_world_runtime_effort_result_page(state,*top)))
        return Error::missing_source;
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==88&&
       (top->lifecycle!=2||!valid_startup_world_runtime_award_result_page(state,*top)))
        return Error::missing_source;
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 14)
        return act_startup_world_save_page(state,id);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 13)
        return input_startup_world_manual_page(state,id,{true,false,false,false});
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 3 || top->legacy_page == 4 || top->legacy_page == 7 || top->legacy_page == 10)) {
        StartupWorldMenuInput input;
        input.confirm = true;
        return input_startup_world_menu_page(state, id, input);
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 9 || (top->legacy_page >= 34 && top->legacy_page <= 40))) {
        StartupInformationInput input;
        input.confirm = true;
        return input_startup_world_information_page(state, id, input);
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page >= 41 && top->legacy_page <= 47)
        return act_startup_world_magic_pot_page(state, id, StartupMagicPotAction::confirm);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 72 || top->legacy_page == 79 || top->legacy_page == 82))
        return act_startup_world_facility_catalog_page(state, id, StartupFacilityCatalogAction::confirm);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page >= 51 &&
        top->legacy_page <= 54)
        return act_startup_world_village_activity_page(state, id,
                                                       StartupVillageActivityAction::confirm);
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page>=31&&top->legacy_page<=33) {
        StartupTaskPageInput input;input.confirm=true;
        return input_startup_world_runtime_task_summary_page(state,id,input).error;
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 48)
        return input_startup_world_runtime_rank_page(state,id,StartupTownRankInput{false,false,true,false,{}});
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 90)
        return act_startup_world_tax_page(state, id, StartupWorldTaxAction::confirm);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 98)
        return Error::invalid_page;
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page >= 75 &&
        top->legacy_page <= 77)
        return act_startup_world_facility_item_page(state, id, StartupFacilityItemAction::confirm);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        ((top->legacy_page >= 83 && top->legacy_page <= 86) || top->legacy_page == 93))
        return act_startup_world_commerce_page(state, id, StartupCommerceAction::confirm);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        ((top->legacy_page >= 60 && top->legacy_page <= 66) || top->legacy_page == 68 ||
         top->legacy_page == 69 || top->legacy_page == 70 || top->legacy_page == 73))
        return act_startup_world_human_page(state, id, StartupHumanPageAction::confirm);
    auto next = state;
    next.scripts.executing_page = id;
    if (top->kind == ref::WorldScriptPageKind::raw_page) {
        const auto adapter = startup_world_runtime_adapter();
        if (top->legacy_page == 11) {
            // b/g:11367：确认只快进到40或关闭；不再次执行奖励/指令22续体。
            if (!event_message_command(next, *top))
                return Error::missing_source;
            if (next.page_counters[id] < 40)
                next.page_counters[id] = 40;
            else {
                const auto closed =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
                if (!closed.candidate ||
                    !write_startup_world_runtime_scripts(next, closed.candidate->state))
                    return Error::script_failed;
            }
        } else if (top->legacy_page == 59) {
            if (!unlock_human_valid(next, *top))
                return Error::missing_source;
            // b/g:4879、aM={60,70}：早确认不快进，满70才写aq10并关闭。
            if (next.page_counters[id] >= 70) {
                next.human_calendar.at(top->legacy_f).absent_months = 10;
                const auto closed =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
                if (!closed.candidate ||
                    !write_startup_world_runtime_scripts(next, closed.candidate->state))
                    return Error::script_failed;
            }
        } else if (top->legacy_page == 99 || top->legacy_page == 100) {
            if (!consume_task_display(next, *top, ref::WorldTaskDisplayAction::confirm))
                return Error::missing_source;
        } else if (top->legacy_page == 50) {
            if (!consume_rank_celebration(next, id, true))
                return Error::missing_source;
        } else if (top->legacy_page == 81) {
            if (!consume_startup_world_facility_upgrade(next, id, true))
                return Error::missing_source;
        } else if (top->legacy_page == 96) {
            auto &phase = next.page_phases.at(id);
            auto &counter = next.page_counters.at(id);
            if (phase == 0) {
                if (counter < 40)
                    counter = 40;
                else {
                    phase = 1;
                    counter = 0;
                }
            } else if (phase == 1) {
                if (counter < 77)
                    counter = 77;
                else {
                    const auto closed = ref::prepare_world_script_close_page(
                        startup_world_runtime_scripts(next), id);
                    if (!closed.candidate ||
                        !write_startup_world_runtime_scripts(next, closed.candidate->state))
                        return Error::script_failed;
                }
            } else
                return Error::missing_source;
        } else if (top->legacy_page == 67 || top->legacy_page == 88) {
            if (!consume_award_display(next, id, true, top->legacy_page))
                return Error::missing_source;
        } else if (top->legacy_page == 49) {
            if (!valid_startup_world_town_progress_page(next,*top)||!initialize_rank_page(next,id))
                return Error::missing_source;
            if (next.rank < 5) {
                // b/g.g L6e/L1a8：确认置u8后关闭，页49绝不走页48的晋级消费者。
                next.scripts.user_flags |= 8;
                const auto result =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
                if (!result.candidate ||
                    !write_startup_world_runtime_scripts(next, result.candidate->state))
                    return Error::script_failed;
            }
        } else if (top->legacy_page == 94 || top->legacy_page == 95) {
            if(top->legacy_page==94&&(top->lifecycle==0||!valid_startup_world_gift_notice_page(next,*top)))return Error::missing_source;
            if(top->legacy_page==95&&(top->lifecycle==0||!valid_startup_world_town_progress_page(next,*top)))return Error::missing_source;
            const auto result = ref::prepare_world_gift_page(
                gift(next, adapter), {id, next.page_counters[id], true}, adapter.catalog);
            if (!result.candidate || !write_gift(next, result.candidate->state, adapter))
                return Error::script_failed;
            next.page_counters[id] = result.candidate->counter;
        } else if (top->legacy_page == 92) {
            const auto closed=ref::prepare_world_script_close_page(startup_world_runtime_scripts(next),id);
            if(!closed.candidate||!write_startup_world_runtime_scripts(next,closed.candidate->state))return Error::script_failed;
        } else if (top->legacy_page == 89) {
            // b/g.g:6003：早确认仅快进；40之后关闭，不再重复执行介绍脚本。
            if (next.page_counters.at(id) < 40)
                next.page_counters.at(id) = 40;
            else {
                const auto result =
                    ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
                if (!result.candidate ||
                    !write_startup_world_runtime_scripts(next, result.candidate->state))
                    return Error::script_failed;
            }
        } else if (top->legacy_page == 30 && next.page_counters[id] < 40) {
            next.page_counters[id] = 40;
        } else if (top->legacy_page == 30 && next.page_phases[id] == 0) {
            next.page_phases[id] = 1;
            next.page_counters[id] = 0;
        } else if (top->legacy_page == 30) {
            // 阶段2已经提交奖励；成果页只展示与关闭，绝不再支付一次。
            if (!next.exploration_summaries.count(id))
                return Error::missing_source;
            const auto result =
                ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
            if (!result.candidate ||
                !write_startup_world_runtime_scripts(next, result.candidate->state))
                return Error::script_failed;
        } else
            return Error::missing_source; // 任务结果/税收等页有独立域动作，不能只关页。
    } else {
        const auto result =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
        if (!result.candidate ||
            !write_startup_world_runtime_scripts(next, result.candidate->state))
            return Error::script_failed;
    }
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}

Error cancel_startup_world_runtime_page(State &state, std::uint64_t id) {
    const auto catalogue = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                       [](const auto &p) { return p.lifecycle != 4; });
    if(catalogue!=state.scripts.pages.rend()&&catalogue->id==id&&catalogue->kind==ref::WorldScriptPageKind::raw_page&&
       catalogue->legacy_page==17)return Error::invalid_page;
    if(catalogue!=state.scripts.pages.rend()&&catalogue->id==id&&catalogue->kind==ref::WorldScriptPageKind::raw_page&&
       (catalogue->legacy_page==31||catalogue->legacy_page==32)) {
        StartupTaskPageInput input;input.cancel=true;
        return input_startup_world_runtime_task_summary_page(state,id,input).error;
    }
    if (catalogue != state.scripts.pages.rend() && catalogue->id == id &&
        catalogue->kind == ref::WorldScriptPageKind::raw_page &&
        (catalogue->legacy_page == 3 || catalogue->legacy_page == 4 ||
         catalogue->legacy_page == 7 || catalogue->legacy_page == 10)) {
        StartupWorldMenuInput input;
        input.cancel = true;
        return input_startup_world_menu_page(state, id, input);
    }
    if (catalogue != state.scripts.pages.rend() && catalogue->id == id &&
        catalogue->kind == ref::WorldScriptPageKind::raw_page && catalogue->legacy_page == 58)
        return Error::invalid_page;
    if (catalogue != state.scripts.pages.rend() && catalogue->id == id &&
        catalogue->kind == ref::WorldScriptPageKind::raw_page && catalogue->legacy_page == 14)
        return act_startup_world_save_page(state,id);
    if (catalogue != state.scripts.pages.rend() && catalogue->id == id &&
        catalogue->kind == ref::WorldScriptPageKind::raw_page && catalogue->legacy_page == 13)
        return input_startup_world_manual_page(state,id,{false,false,false,true});
    if(catalogue!=state.scripts.pages.rend()&&catalogue->id==id&&
       catalogue->kind==ref::WorldScriptPageKind::raw_page&&catalogue->legacy_page==92)
        return acknowledge_startup_world_runtime_page(state,id); // 原确认与soft2都立即Pop，无等待门槛。
    if (catalogue != state.scripts.pages.rend() && catalogue->id == id &&
        catalogue->kind == ref::WorldScriptPageKind::raw_page &&
        (catalogue->legacy_page == 9 || (catalogue->legacy_page >= 34 && catalogue->legacy_page <= 40))) {
        StartupInformationInput input;
        input.cancel = true;
        return input_startup_world_information_page(state, id, input);
    }
    if (catalogue != state.scripts.pages.rend() && catalogue->id == id &&
        catalogue->kind == ref::WorldScriptPageKind::raw_page &&
        catalogue->legacy_page >= 41 && catalogue->legacy_page <= 47)
        return act_startup_world_magic_pot_page(state, id, StartupMagicPotAction::cancel);
    if (catalogue != state.scripts.pages.rend() && catalogue->id == id &&
        catalogue->kind == ref::WorldScriptPageKind::raw_page &&
        (catalogue->legacy_page == 72 || catalogue->legacy_page == 79 || catalogue->legacy_page == 82))
        return act_startup_world_facility_catalog_page(state, id, StartupFacilityCatalogAction::cancel);
    const auto activity = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                       [](const auto &p) { return p.lifecycle != 4; });
    if (activity != state.scripts.pages.rend() && activity->id == id &&
        activity->kind == ref::WorldScriptPageKind::raw_page && activity->legacy_page >= 51 &&
        activity->legacy_page <= 54)
        return act_startup_world_village_activity_page(state, id,
                                                       StartupVillageActivityAction::cancel);
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top != state.scripts.pages.rend() && top->id == id &&
        top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page >= 75 &&
        top->legacy_page <= 77)
        return act_startup_world_facility_item_page(state, id, StartupFacilityItemAction::cancel);
    if (top != state.scripts.pages.rend() && top->id == id &&
        top->kind == ref::WorldScriptPageKind::raw_page &&
        ((top->legacy_page >= 83 && top->legacy_page <= 86) || top->legacy_page == 93))
        return act_startup_world_commerce_page(state, id, StartupCommerceAction::cancel);
    if (top != state.scripts.pages.rend() && top->id == id &&
        top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 60 || top->legacy_page == 61 || top->legacy_page == 62 ||
         top->legacy_page == 64 || top->legacy_page == 65 || top->legacy_page == 70 ||
         top->legacy_page == 73))
        return act_startup_world_human_page(state, id, StartupHumanPageAction::cancel);
    if (top != state.scripts.pages.rend() && top->id == id &&
        top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 48)
        return act_startup_world_runtime_rank_page(state, id, 0, true);
    return Error::invalid_page;
}

Error act_startup_world_runtime_rank_page(State &state, std::uint64_t id, int selection,
                                          bool cancel) {
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (state.scene.framework_paused || top == state.scripts.pages.rend() || top->id != id ||
        top->kind != ref::WorldScriptPageKind::raw_page || top->legacy_page != 48)
        return Error::invalid_page;
    auto next = state;
    next.scripts.executing_page = id;
    if(top->lifecycle==0||!valid_startup_world_town_progress_page(next,*top)||selection<0||selection>4)
        return Error::missing_source;
    next.page_phases.at(id)=selection;
    const auto close = [&]() {
        const auto result =
            ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), id);
        return result.candidate &&
               write_startup_world_runtime_scripts(next, result.candidate->state);
    };
    if (next.rank < 5) {
        if (cancel) {
            if (!close())
                return Error::script_failed;
        } else {
            const auto terms = ref::fixed_calendar_task_rank_terms();
            const auto result = ref::prepare_world_rank_promotion(
                next.rank, terms.at(next.rank), next.rank_met, selection, top->legacy_f == 1,
                next.scripts.village_name);
            if (!result)
                return Error::missing_source;
            const auto invoke = [&](const ref::WorldScriptInput &input) {
                const auto script = ref::prepare_world_script(
                    startup_world_runtime_catalog(), startup_world_runtime_scripts(next), input);
                return script.candidate &&
                       write_startup_world_runtime_scripts(next, script.candidate->state);
            };
            if (result->mark_user_flag)
                next.scripts.user_flags |= 8;
            for (const auto &input : result->before_promotion)
                if (!invoke(input))
                    return Error::script_failed;
            if (result->promoted) {
                next.rank = result->rank;
                next.rank_history.at(next.rank - 1) = {next.scene.calendar.year,
                                                       next.scene.calendar.month};
                if (std::any_of(next.rules->facilities.begin(), next.rules->facilities.end(),
                                [&](const auto &d) { return d.unlock_rank == next.rank; }))
                    next.scripts.notices.push_back({18, -1, 80, "", "有了新的设施"});
                for (const auto &activity : next.rules->activities) {
                    if (activity.parameters[6] != next.rank)
                        continue;
                    auto &definition = next.scripts.activities.at(activity.identity);
                    if (definition.status == 0)
                        definition.pending_notice = true;
                    definition.status = 1;
                    next.scripts.notices.push_back(
                        {6, -1, 80, activity.name,
                         "举办活动追加 <co=0064FF>" + activity.name + "</co> "});
                }
                ref::WorldScriptPage celebration;
                celebration.kind = ref::WorldScriptPageKind::raw_page;
                celebration.legacy_page = 50;
                const auto page = ref::prepare_world_script_page(
                    startup_world_runtime_scripts(next), celebration);
                if (!page.candidate ||
                    !write_startup_world_runtime_scripts(next, page.candidate->state))
                    return Error::script_failed;
                for (const auto &input : result->after_promotion)
                    if (!invoke(input))
                        return Error::script_failed;
                if (!close())
                    return Error::script_failed;
            }
        }
    }
    next.scripts.executing_page.reset();
    state = std::move(next);
    return Error::none;
}

Error input_startup_world_runtime_rank_page(State &s,std::uint64_t id,const StartupTownRankInput &input) {
    const auto p=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),[](const auto &page){return page.lifecycle!=4;});
    if(s.scene.framework_paused||p==s.scripts.pages.rend()||p->id!=id||p->legacy_page!=48||
       p->lifecycle==0||!valid_startup_world_town_progress_page(s,*p))return Error::invalid_page;
    auto next=s;auto &selected=next.page_phases.at(id);
    if(input.selection) {if(*input.selection<0||*input.selection>4)return Error::invalid_page;selected=*input.selection;}
    if(input.up)selected=(selected+4)%5;
    if(input.down)selected=(selected+1)%5;
    if(input.confirm||input.cancel) {
        const auto error=act_startup_world_runtime_rank_page(next,id,selected,!input.confirm&&input.cancel);
        if(error!=Error::none)return error;
    }
    s=std::move(next);return Error::none;
}

std::optional<State> update_startup_world_runtime_page(const State &state) {
    const auto notice=std::find_if(state.scripts.pages.rbegin(),state.scripts.pages.rend(),
        [](const auto &p){return p.lifecycle!=4;});
    if(notice!=state.scripts.pages.rend()&&notice->kind==ref::WorldScriptPageKind::raw_page&&
       (notice->legacy_page==99||notice->legacy_page==100)&&state.task_display_initialized.count(notice->id)) {
        const auto count=state.page_counters.find(notice->id);
        if(count==state.page_counters.end()||count->second<0||count->second==std::numeric_limits<int>::max())return {};
    }
    if (state.scene.framework_paused) {
        const auto page = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                      [](const auto &p) { return p.lifecycle != 4; });
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page==21&&!valid_startup_world_build_menu(state,*page))return {};
        if (page != state.scripts.pages.rend() && page->kind == ref::WorldScriptPageKind::raw_page &&
            page->legacy_page == 14 && !inspect_startup_world_save_page(state,page->id))
            return {};
        if (page != state.scripts.pages.rend() && page->kind == ref::WorldScriptPageKind::raw_page &&
            page->legacy_page == 13 && !inspect_startup_world_manual_page(state,page->id))
            return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page==30&&!valid_startup_world_runtime_task_victory_page(state,*page))return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page==96&&!valid_startup_world_runtime_residence_result_page(state,*page))return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page==67&&!valid_startup_world_runtime_effort_result_page(state,*page))return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page==88&&!valid_startup_world_runtime_award_result_page(state,*page))return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page==89&&!valid_startup_world_monster_intro_page(state,*page))return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page==92&&!valid_startup_world_monster_detail_page(state,*page))return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page>=25&&page->legacy_page<=28&&!valid_startup_world_runtime_task_ui_page(state,*page))return {};
        if(page!=state.scripts.pages.rend()&&page->kind==ref::WorldScriptPageKind::raw_page&&
           page->legacy_page>=31&&page->legacy_page<=33&&!valid_startup_world_runtime_task_summary_ui_page(state,*page))return {};
        if (page != state.scripts.pages.rend() && page->kind == ref::WorldScriptPageKind::raw_page &&
            (page->legacy_page == 3 || page->legacy_page == 4 || page->legacy_page == 7 ||
             page->legacy_page == 10) && !valid_startup_world_menu_page(state, page->id))
            return {};
        return state;
    }
    const auto top = std::find_if(state.scripts.pages.rbegin(), state.scripts.pages.rend(),
                                  [](const auto &p) { return p.lifecycle != 4; });
    if (top == state.scripts.pages.rend() || top->kind == ref::WorldScriptPageKind::scene)
        return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==21)
        return update_startup_world_build_menu(state,top->id);
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==17)return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==89&&
       (top->lifecycle!=2||!valid_startup_world_monster_intro_page(state,*top)))return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==92&&
       (top->lifecycle!=2||!valid_startup_world_monster_detail_page(state,*top)))return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==30&&
       (top->lifecycle!=2||!valid_startup_world_runtime_task_victory_page(state,*top)))return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==96&&
       (top->lifecycle!=2||!valid_startup_world_runtime_residence_result_page(state,*top)))return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==67&&
       (top->lifecycle!=2||!valid_startup_world_runtime_effort_result_page(state,*top)))return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==88&&
       (top->lifecycle!=2||!valid_startup_world_runtime_award_result_page(state,*top)))return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&(top->legacy_page==25||top->legacy_page==26))
        return update_startup_world_runtime_task_roster_page(state,top->id);
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page>=31&&top->legacy_page<=33&&
       !valid_startup_world_runtime_task_summary_ui_page(state,*top))return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&(top->legacy_page==31||top->legacy_page==32)) {
        auto next=state;
        if(!initialize_startup_world_runtime_task_summary_ui_page(next,top->id))return {};
        auto &frame=next.page_counters.at(top->id);
        frame=(frame+1)%std::numeric_limits<int>::max();
        std::find_if(next.scripts.pages.begin(),next.scripts.pages.end(),[&](const auto &v){return v.id==top->id;})->lifecycle=2;
        return next;
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 14)
        return update_startup_world_save_page(state,top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 13)
        return update_startup_world_manual_page(state,top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 3 || top->legacy_page == 4 || top->legacy_page == 7 || top->legacy_page == 10))
        return update_startup_world_menu_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 9 || (top->legacy_page >= 34 && top->legacy_page <= 40)))
        return update_startup_world_information_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 58)
        return update_startup_world_runtime_task_tracking_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 33)
        return update_startup_world_runtime_deadline_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 74 &&
        !valid_startup_world_facility_page(state, *top))
        return {};
    auto next = state;
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 97) {
        // b/g.b:6172：实际更新清R并m()；不是玩家确认页，不再执行奖励脚本。
        const auto adapter = startup_world_runtime_adapter();
        const auto result =
            ref::prepare_world_popularity_unlock_page(adapter.popularity.read(next), top->id);
        if (!result.candidate || !adapter.popularity.write(next, result.candidate->state))
            return {};
        return next;
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 24 || top->legacy_page == 28))
        return update_startup_world_runtime_task_page(state, top->id, state.page_confirm_held);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 90 || top->legacy_page == 98))
        return update_startup_world_tax_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page >= 51 &&
        top->legacy_page <= 54)
        return update_startup_world_village_activity_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page >= 75 &&
        top->legacy_page <= 77)
        return prepare_startup_world_facility_item_page(state);
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page >= 41 && top->legacy_page <= 47)
        return update_startup_world_magic_pot_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 72 || top->legacy_page == 79 || top->legacy_page == 82))
        return update_startup_world_facility_catalog_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        ((top->legacy_page >= 83 && top->legacy_page <= 86) || top->legacy_page == 93))
        return update_startup_world_commerce_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        ((top->legacy_page >= 60 && top->legacy_page <= 66) || top->legacy_page == 68 ||
         top->legacy_page == 69 || top->legacy_page == 70 || top->legacy_page == 73))
        return update_startup_world_human_page(state, top->id);
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 48 || top->legacy_page == 49)) {
        if (!initialize_rank_page(next, top->id))
            return {};
        if (next.rank >= 5)
            return next;
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page==48||top->legacy_page==49||top->legacy_page==50||top->legacy_page==95)&&
        !valid_startup_world_town_progress_page(next,*top))
        return {};
    if(top->kind==ref::WorldScriptPageKind::raw_page&&top->legacy_page==94&&
       !valid_startup_world_gift_notice_page(next,*top))return {};
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 81) {
        // raw81 自己校验并推进双计数；不得先用通用 [] 隐式补齐缺失的主计数。
        if (!consume_startup_world_facility_upgrade(next, top->id, false))
            return {};
        return next;
    }
    auto &counter = top->kind==ref::WorldScriptPageKind::raw_page&&(top->legacy_page==89||top->legacy_page==92)
        ? next.page_counters.at(top->id):next.page_counters[top->id];
    if (counter == std::numeric_limits<int>::max())
        return {};
    ++counter;
    if(top->legacy_page==27||top->legacy_page==67||top->legacy_page==88||top->legacy_page==89||top->legacy_page==92||top->legacy_page==94||top->legacy_page==99||top->legacy_page==100)counter%=std::numeric_limits<int>::max();
    if(top->legacy_page==50||top->legacy_page==30||top->legacy_page==96) {
        auto &secondary=next.page_secondary_counters.at(top->id);
        counter%=std::numeric_limits<int>::max();
        secondary=(secondary+1)%std::numeric_limits<int>::max();
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 50 &&
        !consume_rank_celebration(next, top->id, false))
        return {};
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 59) {
        if (!unlock_human_valid(next, *top))
            return {};
        if (counter == 1)
            next.sound_requests.push_back({StartupAudioOperation::jingle, 5});
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 96) {
        if (counter == 1 && next.page_phases.at(top->id) == 0)
            next.sound_requests.push_back({StartupAudioOperation::jingle, 5});
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 11) {
        const auto command = event_message_command(next, *top);
        if (!command)
            return {};
        // b/g:11368：首轮0播放4、1播放6，其余代码没有此音效。
        if (counter == 1 && (*command == 0 || *command == 1))
            next.sound_requests.push_back({StartupAudioOperation::jingle, *command == 0 ? 4 : 6});
    }
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 99 || top->legacy_page == 100) &&
        !consume_task_display(next, *top, ref::WorldTaskDisplayAction::update))
        return {};
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 87 &&
        !consume_award(next, top->id, ref::WorldAwardAction::update))
        return {};
    if (top->kind == ref::WorldScriptPageKind::raw_page &&
        (top->legacy_page == 67 || top->legacy_page == 88) &&
        !consume_award_display(next, top->id, false, top->legacy_page))
        return {};
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 30 && counter == 1 &&
        next.page_phases[top->id] == 0)
        next.sound_requests.push_back({StartupAudioOperation::jingle, 4});
    if (top->kind == ref::WorldScriptPageKind::raw_page && top->legacy_page == 16) {
        // d/a opcode7→g.a(L)，b/g.b先--L再走f--；至少一次更新，不接受确认跳过。
        if (counter >= std::max(1, top->legacy_l)) {
            const auto result =
                ref::prepare_world_script_close_page(startup_world_runtime_scripts(next), top->id);
            if (!result.candidate ||
                !write_startup_world_runtime_scripts(next, result.candidate->state))
                return {};
        }
    } else if (top->kind == ref::WorldScriptPageKind::raw_page &&
               (top->legacy_page == 56 || top->legacy_page == 57)) {
        ref::WorldScriptCameraFocusInput input;
        input.page = top->id;
        input.camera = next.camera;
        input.previous_camera = next.previous_camera;
        input.previous_velocity = next.camera_velocity;
        if (top->legacy_page == 57 && !next.task_order.empty()) {
            const auto task = next.tasks.find(next.task_order.front());
            if (task == next.tasks.end())
                return {};
            if (task->second.facility) {
                input.first_task_facility_view =
                    startup_world_runtime_facility_target(next, *task->second.facility);
                if (!input.first_task_facility_view)
                    return {};
            }
        } else if (top->legacy_page == 56 && !next.scene.world.world.ai.monster_order.empty()) {
            const auto id = next.scene.world.world.ai.monster_order.front();
            const auto meta = next.actor_metadata.find(id);
            if (meta == next.actor_metadata.end())
                return {};
            input.first_monster_cached_view = {static_cast<float>(meta->second.cached_view.x),
                                               static_cast<float>(meta->second.cached_view.y)};
        }
        const auto result =
            ref::prepare_world_script_camera_focus(startup_world_runtime_scripts(next), input);
        if (!result.candidate ||
            !write_startup_world_runtime_scripts(next, result.candidate->state))
            return {};
        next.camera = result.candidate->camera;
        next.previous_camera = result.candidate->previous_camera;
        next.camera_velocity = result.candidate->velocity;
    } else if (top->kind == ref::WorldScriptPageKind::raw_page &&
               (top->legacy_page == 94 || top->legacy_page == 95)) {
        const auto adapter = startup_world_runtime_adapter();
        const auto result = ref::prepare_world_gift_page(
            gift(next, adapter), {top->id, counter, false}, adapter.catalog);
        if (!result.candidate || !write_gift(next, result.candidate->state, adapter))
            return {};
        if (result.candidate->sound)
            next.sound_requests.push_back({StartupAudioOperation::jingle, *result.candidate->sound});
    }
    return next;
}
} // namespace dungeon_village_prototype
