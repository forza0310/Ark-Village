// Steam raw14：Init保存中，首次Update仅state0→1，下次由应用同步写档，state2才可退出。
// 证据rules/PERSISTENCE.md：10314BDF、10326427/103267C5、103264A1、10326458。
#include "dungeon_village_prototype/startup_world_save.hpp"
#include "dungeon_village_prototype/startup_world_menu.hpp"

#include <algorithm>
#include <limits>

namespace dungeon_village_prototype {
namespace {
using State=StartupWorldRuntimeState;
using Page=ref::WorldScriptPage;
using Error=StartupWorldRuntimeError;
const Page *find(const State &s,std::uint64_t id) {
    if(!id || id>=s.scripts.next_page_id)return nullptr;
    const Page *found{};
    for(const auto &page:s.scripts.pages)if(page.id==id) {
        if(!id || found)return nullptr;
        found=&page;
    }
    return found;
}
const Page *top(const State &s) {
    const auto p=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
                             [](const auto &v){return v.lifecycle!=4;});
    return p==s.scripts.pages.rend()?nullptr:&*p;
}
bool extra_payload(const State &s,std::uint64_t id) {
    // raw14只拥有counter/phase，不能借其它页载荷伪造可恢复绑定。
#define EXTRA(name) if(s.name.count(id))return true
    EXTRA(information_page_data);EXTRA(menu_page_data);EXTRA(menu_page_positions);
    EXTRA(page_secondary_counters);EXTRA(page_human_bindings);EXTRA(page_job_bindings);
    EXTRA(human_detail_contexts);EXTRA(human_pages_initialized);EXTRA(human_page_catalogs);
    EXTRA(equipment_page_catalogs);EXTRA(human_page_selections);EXTRA(human_page_parents);
    EXTRA(human_page_answers);EXTRA(human_equipment_choices);EXTRA(human_gift_scores);EXTRA(human_gift_messages);
    EXTRA(task_abort_questions);EXTRA(task_abort_answers);EXTRA(task_page_lists);EXTRA(task_recruitment_pages);
    EXTRA(task_extra_pages);EXTRA(task_page_predictions);EXTRA(task_page_acceleration);EXTRA(task_display_initialized);
    EXTRA(deadline_initialized);EXTRA(deadline_grades);EXTRA(deadline_returns);
    EXTRA(crew_summaries);EXTRA(exploration_summaries);EXTRA(facility_page_bindings);
    EXTRA(facility_definition_page_bindings);EXTRA(facility_page_neighbours);EXTRA(build_page_catalogs);
    EXTRA(residence_page_candidates);EXTRA(facility_upgrade_initialized);EXTRA(award_rankings);
    EXTRA(award_announced);EXTRA(award_termination_pending);EXTRA(award_pending_humans);
    EXTRA(rank_celebration_participants);EXTRA(tax_page_residents);EXTRA(tax_page_selection);EXTRA(tax_page_scroll);
    EXTRA(activity_pages_initialized);EXTRA(activity_page_bindings);EXTRA(activity_page_lists);
    EXTRA(activity_page_display_humans);EXTRA(activity_page_parents);EXTRA(activity_page_answers);
    EXTRA(activity_page_selections);EXTRA(activity_page_scroll);EXTRA(facility_item_pages_initialized);
    EXTRA(facility_item_page_items);EXTRA(facility_item_page_lists);EXTRA(facility_item_page_selections);
    EXTRA(commerce_pages_initialized);EXTRA(commerce_page_data);EXTRA(commerce_page_lists);
    EXTRA(facility_catalog_pages_initialized);EXTRA(facility_catalog_page_data);
    EXTRA(facility_catalog_page_lists);EXTRA(facility_catalog_page_parents);
    EXTRA(magic_pot_pages_initialized);EXTRA(magic_pot_page_data);EXTRA(magic_pot_page_lists);EXTRA(magic_pot_page_parents);
#undef EXTRA
    return false;
}
bool clean_metadata(const Page &p) {
    return p.kind==ref::WorldScriptPageKind::raw_page && p.legacy_page==14 &&
        (p.source_record==3 || p.source_record==10) && p.legacy_tag==20 &&
        p.replacement.empty() && p.speaker_kind==0 && p.speaker_definition==-1 &&
        p.legacy_r==0 && p.legacy_s==0 && p.legacy_t==0 && p.legacy_g==0 && p.legacy_l==0 &&
        p.message_commands.empty() && !p.task_identity && !p.task_definition &&
        !p.monster_definition && !p.facility_definition && p.title=="保存" &&
        p.paragraphs.size()==1 && p.paragraphs.front().size()<=1024;
}
bool live_context(const State &s,const Page &p) {
    if(p.lifecycle==4)return true;
    const Page *scene{};
    bool reached{};
    for(const auto &entry:s.scripts.pages) {
        if(entry.id==p.id) { reached=true;continue; }
        if(entry.lifecycle==4)continue;
        if(reached || scene || entry.kind!=ref::WorldScriptPageKind::scene || entry.lifecycle!=3)return false;
        scene=&entry;
    }
    return reached && scene && !s.scene.top_is_main;
}
bool callable(const State &s,std::uint64_t id) {
    const auto *p=top(s);
    return p && p->id==id && p->lifecycle==2 && valid_startup_world_save_page(s,id) &&
           (!s.scripts.executing_page || *s.scripts.executing_page==id);
}
std::string failure_text(const std::string &error) {
    std::string text="保存失败";
    if(error.empty())return text;
    text+="：";
    auto count=std::min(error.size(),std::size_t(1024)-text.size());
    // 不把UTF8后续字节保留成末尾半字符；错误是展示文本，不执行/解释其中内容。
    if(count<error.size())while(count>0 && (static_cast<unsigned char>(error[count])&0xc0U)==0x80U)--count;
    text.append(error,0,count);
    return text;
}
}
bool valid_startup_world_save_page(const State &s,std::uint64_t id) {
    const auto *p=find(s,id);
    if(!s.rules || !p || p->lifecycle<0 || p->lifecycle>4 || !clean_metadata(*p) ||
       extra_payload(s,id) || !live_context(s,*p) || (p->legacy_f==1 && s.save_marker!=1))return false;
    const auto phase=s.page_phases.find(id),counter=s.page_counters.find(id);
    if(phase==s.page_phases.end() && counter==s.page_counters.end()) {
        if(p->lifecycle==0)return p->legacy_f==-1 && p->paragraphs.front()=="保存中";
        return p->lifecycle==4 && (p->legacy_f==0 || p->legacy_f==1) &&
            (p->legacy_f==1?p->paragraphs.front()=="保存完成":p->paragraphs.front().rfind("保存失败",0)==0);
    }
    if(phase==s.page_phases.end() || counter==s.page_counters.end() || counter->second<0 ||
       phase->second<0 || phase->second>2)return false;
    if(p->lifecycle==0 || (p->lifecycle==4 && phase->second!=2))return false;
    if(phase->second<2)return p->legacy_f==-1 && p->paragraphs.front()=="保存中" && counter->second==phase->second;
    return counter->second>=2 && (p->legacy_f==0 || p->legacy_f==1) &&
        (p->legacy_f==1?p->paragraphs.front()=="保存完成":p->paragraphs.front().rfind("保存失败",0)==0);
}
Error open_startup_world_save_page(State &s) {
    const auto *source=top(s);
    if(!source || s.scene.framework_paused || !s.scripts.executing_page ||
       *s.scripts.executing_page!=source->id ||
       (!startup_world_menu_callback(s,3) && !startup_world_menu_callback(s,10)))return Error::invalid_page;
    if(source->legacy_page==10) {
        const auto view=inspect_startup_world_menu_page(s,source->id);
        if(!view || view->selection<0 || static_cast<std::size_t>(view->selection)>=view->tags.size() ||
           view->tags[static_cast<std::size_t>(view->selection)]!=20)return Error::invalid_page;
    }
    auto next=s;
    Page page;page.kind=ref::WorldScriptPageKind::raw_page;page.legacy_page=14;
    page.source_record=source->legacy_page;page.legacy_tag=20;page.legacy_f=-1;
    page.title="保存";page.paragraphs={"保存中"};
    const auto opened=ref::prepare_world_script_page(startup_world_runtime_scripts(next),page);
    if(!opened.candidate || opened.candidate->inserted_pages.size()!=1 ||
       !write_startup_world_runtime_scripts(next,opened.candidate->state) ||
       !retire_startup_world_menu_pages(next))return Error::script_failed;
    next.scene.top_is_main=false;
    if(!valid_startup_world_save_page(next,opened.candidate->inserted_pages.front().id))return Error::missing_source;
    s=std::move(next);
    return Error::none;
}
bool initialize_startup_world_save_pages(State &s) {
    std::vector<std::uint64_t> pending;
    for(const auto &p:s.scripts.pages)if(p.kind==ref::WorldScriptPageKind::raw_page && p.legacy_page==14) {
        if(!valid_startup_world_save_page(s,p.id))return false;
        if(p.lifecycle==0)pending.push_back(p.id);
    }
    if(s.scene.framework_paused || pending.empty())return true;
    auto next=s;
    for(auto id:pending) {
        next.page_counters.emplace(id,0);next.page_phases.emplace(id,0);
        for(auto &p:next.scripts.pages)if(p.id==id)p.lifecycle=1;
        if(!valid_startup_world_save_page(next,id))return false;
    }
    s=std::move(next);
    return true;
}
std::optional<StartupWorldSavePageView> inspect_startup_world_save_page(const State &s,std::uint64_t id) {
    if(!valid_startup_world_save_page(s,id))return {};
    const auto *p=find(s,id);const auto phase=s.page_phases.find(id),counter=s.page_counters.find(id);
    if(p->lifecycle==0 || p->lifecycle==4 || phase==s.page_phases.end() || counter==s.page_counters.end())return {};
    return StartupWorldSavePageView{id,phase->second,counter->second,
        phase->second==2?std::optional<bool>(p->legacy_f==1):std::nullopt,p->paragraphs.front()};
}
std::optional<State> update_startup_world_save_page(const State &s,std::uint64_t id) {
    if(!callable(s,id))return {};
    if(s.scene.framework_paused)return s;
    const auto view=inspect_startup_world_save_page(s,id);
    if(!view || view->stage==1 || view->counter==std::numeric_limits<int>::max())return {};
    auto next=s;++next.page_counters.find(id)->second;
    if(view->stage==0)next.page_phases.find(id)->second=1;
    return next;
}
Error act_startup_world_save_page(State &s,std::uint64_t id) {
    if(s.scene.framework_paused || !callable(s,id))return Error::invalid_page;
    const auto view=inspect_startup_world_save_page(s,id);
    if(!view)return Error::missing_source;
    if(view->stage<2)return Error::none;
    auto next=s;
    const auto closed=ref::prepare_world_script_close_page(startup_world_runtime_scripts(next),id);
    if(!closed.candidate || !write_startup_world_runtime_scripts(next,closed.candidate->state))return Error::script_failed;
    s=std::move(next);
    return Error::none;
}
std::optional<State> complete_startup_world_save_page(const State &s,std::uint64_t id,bool saved,const std::string &error) {
    const auto view=inspect_startup_world_save_page(s,id);
    if(s.scene.framework_paused || !callable(s,id) || !view || view->stage!=1 || (saved&&!error.empty()))return {};
    auto next=s;
    next.page_phases.find(id)->second=2;++next.page_counters.find(id)->second;
    for(auto &p:next.scripts.pages)if(p.id==id) {
        p.legacy_f=saved?1:0;p.paragraphs={saved?"保存完成":failure_text(error)};
    }
    if(saved)next.save_marker=1;
    if(!valid_startup_world_save_page(next,id))return {};
    return next;
}
std::optional<State> prepare_startup_world_save_export(const State &s,std::uint64_t id) {
    const auto view=inspect_startup_world_save_page(s,id);
    if(s.scene.framework_paused || !callable(s,id) || !view || view->stage!=1 || s.scripts.executing_page ||
       !s.sound_requests.empty() || s.scene.scene_state!=0 || s.build_mode!=0 || s.build_definition ||
       s.build_anchor || s.build_moving_facility || s.scripts.pages.size()!=2 ||
       s.scripts.pages.front().kind!=ref::WorldScriptPageKind::scene || s.scripts.pages.front().lifecycle!=3 ||
       s.scripts.pages.back().id!=id)return {};
    auto next=s;
    next.scripts.pages.pop_back();next.page_counters.erase(id);next.page_phases.erase(id);
    next.scripts.pages.front().lifecycle=2;next.scene.top_is_main=true;
    next.confirm_input=false;next.cancel_input=false;next.menu_input=false;
    next.page_confirm_held=false;next.focus_held_input=0;next.save_marker=1;
    return next;
}
} // namespace dungeon_village_prototype
