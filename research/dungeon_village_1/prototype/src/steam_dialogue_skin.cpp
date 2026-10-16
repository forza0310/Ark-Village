// Steam raw0：Draw VA103565AE–10356CB5，Update VA10327286–1032748E；不复刻原scratch实现。
#include "dungeon_village_prototype/steam_dialogue_skin.hpp"
#include <limits>

namespace dungeon_village_prototype {
namespace {
bool measured(int value) { return value>=0&&value<=4096; }
void frame(SteamDialogueSkinPlan &plan,const SteamStartupFramePlan &source) {
    for(const auto &draw:source.draws)plan.draws.emplace_back(draw);
}
void name(SteamDialogueSkinPlan &plan,const SteamDialogueSkinInput &in) {
    plan.draws.emplace_back(SteamDialogueWindowColor{});
    plan.draws.emplace_back(SteamDialogueNameDraw{in.speaker_name,{53,174+(in.english?1:0),120,11},34,0});
}
}
std::optional<SteamDialogueSkinPlan> steam_dialogue_skin(const SteamDialogueSkinInput &in) {
    if(in.width<=0||in.height<=0||in.width>65536||in.height>65536||in.main<0||
       in.main==std::numeric_limits<int>::max()||in.wait<0)return {};
    SteamDialogueSkinPlan plan;
    const bool centered=in.width>240||in.height>240;
    plan.origin={centered?(in.width-240)/2:0,centered?(in.height-240)/2:0};
    if(!in.on_top||in.until_active_hide||in.wait>0)return plan;
    const bool human=in.speaker_kind==1&&in.speaker_index>=0;
    const bool monster=in.speaker_kind==2;
    const bool fixed=in.speaker_kind==0&&(in.speaker_index==-1||in.speaker_index==2||in.speaker_index==3);
    if(in.title.size()>4096||!in.title_widths||!measured((*in.title_widths)[0])||
       !measured((*in.title_widths)[1])||((human||monster||fixed)&&in.speaker_name.size()>4096)||
       (human&&(!in.human||in.human->definition!=in.speaker_index||in.human->body_sprite<0||in.human->weapon< -1))||
       (monster&&(in.speaker_index<0||!in.monster_flag4)))return {};
    const auto window=steam_startup_window_skin({202,122,0,0,SteamStartupTextRole::message_title,0},in.view_y,in.title_widths);
    const auto box=steam_startup_box_skin({27,82,214,159},in.view_y);
    if(!window||!box)return {};
    plan.title=in.title;
    frame(plan,*window);frame(plan,*box);
    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,143,-1,0,0,{0,0,7,7},{174,169}});
    plan.draws.emplace_back(SteamDialogueWindowColor{});
    plan.draws.emplace_back(SteamDialogueBodyDraw{});
    if(in.english)plan.draws.emplace_back(SteamDialogueFont{SteamDialogueFontAction::set_size10});
    if(fixed) {
        const int image=in.speaker_index==-1?126:in.speaker_index==2?173:176;
        const int seb=in.speaker_index==-1?87:in.speaker_index==2?38:95;
        plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::common,image,seb,1,0,{},{193,189}});
        name(plan,in);
    } else if(human) {
        plan.draws.emplace_back(SteamDialogueHumanSet{in.speaker_index,0,in.human->body_sprite,0,2,in.main});
        plan.draws.emplace_back(SteamDialogueHumanDraw{{193,189},in.human->weapon,0});
        name(plan,in);
    } else if(monster) {
        plan.draws.emplace_back(SteamDialogueMonsterDraw{in.speaker_index,{*in.monster_flag4?188:177,188},2});
        name(plan,in);
    }
    if(in.english)plan.draws.emplace_back(SteamDialogueFont{SteamDialogueFontAction::reset});
    plan.draws.emplace_back(SteamStartupTouch{2,0,{},{},0});
    return plan;
}
std::optional<SteamDialogueUpdatePlan> steam_dialogue_update(const SteamDialogueUpdateInput &in) {
    if(in.paragraphs.empty()||in.paragraphs.size()>4096||in.paragraph<0||
       static_cast<std::size_t>(in.paragraph)>=in.paragraphs.size()||in.next_talk< -1)return {};
    for(const auto &text:in.paragraphs)if(text.size()>4096)return {};
    SteamDialogueUpdatePlan plan;plan.paragraph=in.paragraph;
    plan.requests={SteamDialogueUpdateRequest::frame,SteamDialogueUpdateRequest::confirm_pulse};
    bool confirmed=in.confirm_pulse;
    if(!confirmed) {
        plan.requests.push_back(SteamDialogueUpdateRequest::debug_gate);
        if(in.debug_enabled) {
            plan.requests.push_back(SteamDialogueUpdateRequest::debug_held);
            confirmed=in.debug_held;
        }
    }
    if(!confirmed)return plan;
    if(!in.ended_after_frame)return {};
    plan.requests.push_back(SteamDialogueUpdateRequest::is_end);
    if(!*in.ended_after_frame) {
        plan.requests.push_back(SteamDialogueUpdateRequest::flush);
        return plan;
    }
    ++plan.paragraph;
    if(static_cast<std::size_t>(plan.paragraph)<in.paragraphs.size()) {
        plan.next_text=in.paragraphs[plan.paragraph];
        plan.requests.push_back(SteamDialogueUpdateRequest::set_text);
    } else {
        plan.requests.push_back(in.next_talk==-1?SteamDialogueUpdateRequest::pop:
                                                   SteamDialogueUpdateRequest::unsupported_next_talk);
        plan.normal_return.reset();
    }
    return plan;
}
} // namespace dungeon_village_prototype
