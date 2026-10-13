// Steam RankForm.Draw/_draw RVA202CC0/203660；框架/随机和持久纪录留给原调用者。
#include "dungeon_village_prototype/steam_record_skin.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

namespace dungeon_village_prototype {
std::optional<std::string> steam_record_value_text(int page,std::int64_t value,bool japanese) {
    if(page<0||page>1||value<0)return {};
    auto text=std::to_string(value);
    for(auto at=text.size();at>3;) {at-=3;text.insert(at,",");}
    if(page==1)text+="Ｇ";else if(japanese)text+="P";
    return text;
}
std::optional<SteamRecordSkinPlan> steam_record_skin(const SteamRecordSkinInput &in) {
    const auto value=steam_record_value_text(in.page,in.value,in.japanese);
    const auto measured=[](std::optional<int> n){return n&&*n>=0&&*n<=4096;};
    if(!value||in.view_width<=0||in.view_width>65536||in.view_height<=0||in.view_height>65536||
       in.frame<0||in.frame==std::numeric_limits<int>::max()||(in.japanese&&in.english)||
       !measured(in.topic_width)||!measured(in.value_width)||!in.village_width||
       !std::isfinite(*in.village_width)||*in.village_width<0||*in.village_width>4096||
       in.definition_count<0||in.definition_count>4096||in.frozen_definitions.size()>static_cast<std::size_t>(in.definition_count))return {};
    for(const auto *text:{&in.village,&in.labels.translated_title,&in.labels.topic})
        if(text->size()>4096)return {};
    if((in.english&&in.labels.english_title.size()>4096)||(!in.japanese&&in.labels.bottom.size()>4096))return {};
    std::set<int> seen;
    for(int id:in.frozen_definitions)if(id<0||id>=in.definition_count||!seen.insert(id).second)return {};
    SteamRecordSkinPlan plan;
    plan.origin={(in.view_width-240)/2,(in.view_height-240)/2+13};
    const auto suffix=" "+std::to_string(in.page+1)+"/2";
    // 源两窗的翻译调用不同；第二次不能去重，字体测宽由各次窗口消费者提供。
    plan.draws.emplace_back(SteamRecordWindow{{200,170,0,-12,SteamStartupTextRole::message_title,0},
        (in.english?in.labels.english_title:std::string("ハイスコア"))+suffix});
    plan.draws.emplace_back(SteamRecordWindow{{200,170,0,-12,SteamStartupTextRole::message_title,0},
        in.labels.translated_title+suffix});
    const int shift=(in.frame%20)/5;
    for(int side=0;side<2;++side) {
        const auto index=plan.draws.size();
        plan.draws.emplace_back(SteamManualArrowRequest{{side?193+shift:44-shift,42},side?0:3,in.arrow_selected[side]});
        plan.draws.emplace_back(SteamStartupTouch{1,side?18:16,{},index,0});
    }
    plan.draws.emplace_back(SteamRecordText{in.labels.topic,{float(120-*in.topic_width/2),59.F},{60,100,200}});
    plan.draws.emplace_back(StartupSkinDraw{StartupSkinPackage::event,9,-1,0,0,{0,0,180,80},{30,78}});
    const int start=120-(std::min(int(in.frozen_definitions.size()),5)-1)*13;
    for(std::size_t n=0;n<in.frozen_definitions.size();++n)
        plan.draws.emplace_back(SteamManualActorRequest{in.frozen_definitions[n],{start+26*int(n),153},
            0,(in.frame%16)/4,1,in.frame});
    plan.draws.emplace_back(SteamRecordText{in.village,{120.F-*in.village_width/2.F,166.F},{0,0,60}});
    plan.draws.emplace_back(SteamRecordText{in.japanese?"ﾊｲｽｺｱ":in.labels.bottom,
        {in.japanese?58.F:46.F,186.F},{60,100,200}});
    plan.draws.emplace_back(SteamRecordText{*value,{float((in.japanese?186:198)-*in.value_width),186.F},{200,100,60}});
    return plan;
}
std::optional<SteamRecordUpdatePlan> steam_record_update(const SteamRecordUpdateInput &in) {
    if(in.page<0||in.page>1||in.frame<0||in.frame==std::numeric_limits<int>::max())return {};
    SteamRecordUpdatePlan plan;
    plan.page=in.page;plan.frame=in.frame==std::numeric_limits<int>::max()-1?0:in.frame+1;
    if(!in.state_zero)return plan;
    plan.checked_keys[0]=true;
    if(in.left_repeat) {plan.page=1-in.page;return plan;}
    plan.checked_keys[1]=true;
    if(in.right_repeat) {plan.page=1-in.page;return plan;}
    plan.checked_keys[2]=true;plan.request_pop=in.back_soft_label;
    return plan;
}
} // namespace dungeon_village_prototype
