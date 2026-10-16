// Steam SubForm._draw2_4 VA1034AFD2–1034B395与APK b/g raw89的独立只读表达。
#include "ark/simulation/presentation/steam_monster_skin.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include "ark/simulation/combat/rules/combat_ai.hpp"
#include <algorithm>
#include <limits>

namespace ark::simulation {
namespace {
// StringUtil.Split(text," "): trim=true、保留空段；仅修剪U+0000..0020。
std::vector<std::string> statistic_parts(const std::string &text) {
    std::vector<std::string> result;
    if(text.empty())return result;
    std::size_t start{};
    for(;;) {
        const auto next=text.find(' ',start);
        auto part=text.substr(start,next==std::string::npos?next:next-start);
        // 不使用locale空白分类；UTF-8多字节首尾均不属于该ASCII集合。
        std::size_t first{};auto last=part.size();
        while(first<last&&static_cast<unsigned char>(part[first])<=0x20)++first;
        while(last>first&&static_cast<unsigned char>(part[last-1])<=0x20)--last;
        part=part.substr(first,last-first);
        result.push_back(std::move(part));
        if(next==std::string::npos)break;
        start=next+1;
    }
    return result;
}
}
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const SteamMonsterIntroductionInput &in,const SteamMonsterSkinOptions &o) {
    constexpr std::array<int,4> images{9,12,8,6},speeds{6,6,8,10};
    if(in.counter<0||in.counter==std::numeric_limits<int>::max()||in.body<0||in.body>3||
       in.variant<0||in.variant>=images[in.body]||in.shadow<0||in.shadow>3||
       in.name.empty()||in.name.size()>4096||!o.title_widths)return {};
    const auto outer=steam_startup_window_skin(
        {222,170,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto inner=steam_startup_box_skin({15,139,224,202},o.view_y);
    if(!outer||!inner)return {};
    SteamMonsterSkinPlan p;p.title=o.japanese?"新種ﾓﾝｽﾀｰ":"新型怪物";
    for(const auto &draw:outer->draws)p.operations.emplace_back(draw);
    for(const auto &draw:inner->draws)p.operations.emplace_back(draw);
    for(const auto &part:std::array<std::array<int,6>,3>{{
        {30,67,0,0,93,82},{109,67,1,0,91,82},{189,67,1,0,20,82}}})
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::residence_background,
            {part[0],part[1]},0,std::array<int,4>{part[2],part[3],part[4],part[5]}});
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,67,179,81},{148,148,148},true}});
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::shadow,{120,133},in.shadow,{}});
    p.operations.emplace_back(SteamMonsterBody{in.body*30+in.variant,in.body*4+2,
        (in.counter%(speeds[in.body]*4))/speeds[in.body],{120,132}});
    // 原RateConvert截限后进行整数乘除；主钟不受纯绘制推进。
    const int height=std::clamp(in.counter-5,0,35)*48/35;
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{0,158,240,height}});
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        in.name+(o.japanese?"が":""),{120,165,0,0},2,0,false,{92,51,31}});
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        o.japanese?"出現しました!!":"出现了!!",{120,187,0,0},2,0,false,{92,51,31}});
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
    if(in.counter>=40&&(in.counter-40)%25<15)
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
    // 原Touch2没有显式矩形，平台不得据此杜撰全屏点击区域。
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return p;
}
std::optional<SteamMonsterSkinPlan> steam_monster_introduction_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamMonsterSkinOptions &o) {
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
        [](const auto &p){return p.lifecycle!=4;});
    if(page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||
       !valid_startup_world_monster_intro_page(s,*page))return {};
    const auto d=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),
        [&](const auto &m){return m.identity==*page->monster_definition;});
    if(d==s.rules->monsters.end())return {};
    return steam_monster_introduction_skin(
        {s.page_counters.at(id),d->initial.body,d->initial.sprite_variant,d->shadow,d->name},o);
}
std::optional<SteamFacilityResource> steam_monster_resource(SteamMonsterAsset asset) {
    if(asset==SteamMonsterAsset::money)
        return SteamFacilityResource{"common2",1,0,"original/common2/monster_money.png","original/common2/monster_money.seb"};
    switch(asset) {
    case SteamMonsterAsset::aura0:return SteamFacilityResource{"effect",14,13,"original/effect/aura10.png","original/effect/aura10.seb"};
    case SteamMonsterAsset::aura1:return SteamFacilityResource{"effect",15,14,"original/effect/aura11.png","original/effect/aura11.seb"};
    case SteamMonsterAsset::aura2:return SteamFacilityResource{"effect",16,15,"original/effect/aura12.png","original/effect/aura12.seb"};
    case SteamMonsterAsset::aura3:return SteamFacilityResource{"effect",17,16,"original/effect/aura13.png","original/effect/aura13.seb"};
    case SteamMonsterAsset::smoke:return SteamFacilityResource{"effect",8,8,"original/effect/satSmoke02.png","original/effect/satSmoke02.seb"};
    default:break;
    }
    return {};
}
std::optional<SteamMonsterSkinPlan> steam_monster_detail_skin(
    const SteamMonsterDetailInput &in,const SteamMonsterSkinOptions &o) {
    constexpr std::array<int,4> images{9,12,8,6},speeds{6,6,8,10};
    if(in.counter<0||in.counter==std::numeric_limits<int>::max()||in.body<0||in.body>3||
       in.variant<0||in.variant>=images[in.body]||in.shadow<0||in.shadow>3||
       in.defeats<0||in.human_downs<0||in.level<1||in.level>10||in.experience<0||in.cash<0||in.points<0||
       in.name.empty()||in.name.size()>4096||in.description.size()>65536||!o.title_widths||
       (in.level>=2&&(!o.name_width||*o.name_width<0||*o.name_width>std::numeric_limits<int>::max()-49)))return {};
    const auto outer=steam_startup_window_skin(
        {220,168,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto inner=steam_startup_box_skin({17,74,219,184},o.view_y);
    if(!outer||!inner)return {};
    SteamMonsterSkinPlan p;p.title=o.japanese?"モンスター情報":"怪物情报";p.soft_labels={0,2};
    for(const auto &draw:outer->draws)p.operations.emplace_back(draw);
    for(const auto &draw:inner->draws)p.operations.emplace_back(draw);
    const auto text=[&](const std::string &value,int x,int y,int anchor=0,int size=0) {
        p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
            value,{x,y,0,0},anchor,size,false,{92,51,31}});
    };
    const auto number=[&](int value,int x,int y,int anchor,
                          SteamFacilityNumberKind kind=SteamFacilityNumberKind::number) {
        p.operations.emplace_back(SteamFacilityNumber{kind,SteamFacilityAsset::number05,value,{x,y},0,anchor,-1});
    };
    text(in.name,26,69,1);
    if(in.level>=2) {
        const int base=*o.name_width+29;
        p.operations.emplace_back(SteamFacilityImage{SteamFacilityAsset::level_label,{base+2,70},0,
            std::array<int,4>{0,0,17,10}});
        number(in.level,base+20,70,1);
    }
    // 该资源Steam图与APK不同；复用商会已出版的点数图，而非旧人物result_icon。
    p.operations.emplace_back(SteamCommerceImage{SteamCommerceAsset::point_unit,{o.japanese?142:166,67},4,{}});
    if(o.japanese) {text("支給",160,69,1);number(in.points,210,70,4);}
    else number(in.points,210,70,4,SteamFacilityNumberKind::plus_value);
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::residence_background,{23,89},0,
        std::array<int,4>{0,0,93,82}});
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::shadow,{70,158},in.shadow,{}});
    p.operations.emplace_back(SteamMonsterBody{in.body*30+in.variant,in.body*4+1,
        (in.counter%(speeds[in.body]*4))/speeds[in.body],{70,157}});
    // 原后备直接读静态literal，不复用不完整的LT结果，也不按非JP分支换成APK文字。
    const std::array<std::string,2> fallback{"倒した数…","倒された人数…"};
    const auto labels=o.statistic_labels.value_or(fallback);
    if(labels[0].size()>65536||labels[1].size()>65536)return {};
    const auto first=statistic_parts(labels[0]),second=statistic_parts(labels[1]);
    const bool split=!o.japanese&&first.size()>=2&&second.size()>=2;
    if(split) {text(first[0],125,98,0,10);text(first[1],125,112,0,10);}
    else text(fallback[0],125,95);
    p.operations.emplace_back(SteamInformationNumber{SteamInformationNumberKind::monster_count,
        in.defeats,{210,split?104:110},o.japanese});
    if(split) {text(second[0],125,132,0,10);text(second[1],125,146,0,10);}
    else text(fallback[1],125,129);
    p.operations.emplace_back(SteamInformationNumber{SteamInformationNumberKind::person_count,
        in.human_downs,{210,split?138:144},o.japanese});
    p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::experience_label,{42,178},0,{}});
    number(in.experience,95,177,4);
    p.operations.emplace_back(SteamMonsterImage{SteamMonsterAsset::money,{147,189},1});
    number(in.cash,200,177,4,SteamFacilityNumberKind::money);
    text(in.description,120,198,2);
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return p;
}
std::optional<SteamMonsterSkinPlan> steam_monster_detail_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamMonsterSkinOptions &o) {
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
        [](const auto &p){return p.lifecycle!=4;});
    if(page==s.scripts.pages.rend()||page->id!=id||page->lifecycle!=2||
       !valid_startup_world_monster_detail_page(s,*page))return {};
    const auto d=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),
        [&](const auto &m){return m.identity==*page->monster_definition;});
    const auto &ai=s.scene.world.world.ai;
    const auto current=ai.monster_growth.find(*page->monster_definition);
    const auto record=ai.battle.monsters.find(*page->monster_definition);
    if(d==s.rules->monsters.end()||current==ai.monster_growth.end()||record==ai.battle.monsters.end())return {};
    const auto &m=current->second;const bool boss=(d->flags&4U)!=0;
    const auto level=ref::prepare_monster_growth(0,m.growth,2,boss);
    const auto experience=ref::prepare_monster_growth(m.base_death_reward,m.growth,1,boss);
    const auto cash=ref::prepare_monster_growth(m.base_cash_reward,m.growth,1,boss);
    if(!level||!experience||!cash)return {};
    return steam_monster_detail_skin({s.page_counters.at(id),m.body,m.sprite_variant,d->shadow,
        m.growth,record->second.human_kills,*level,*experience,*cash,d->points_per_defeat,d->name,d->description},o);
}
std::optional<SteamMonsterSkinPlan> steam_monster_notice_skin(
    const SteamMonsterNoticeInput &in,const SteamMonsterSkinOptions &o) {
    const auto &m=in.monster;
    constexpr std::array<int,4> images{9,12,8,6},speeds{6,6,8,10};
    const bool boss=in.legacy_page==99;
    if((!boss&&in.legacy_page!=100)||m.counter<0||m.counter==std::numeric_limits<int>::max()||
       m.body<0||m.body>3||m.variant<0||m.variant>=images[m.body]||m.shadow<0||m.shadow>3||
       m.name.empty()||m.name.size()>4096||!o.title_widths)return {};
    const bool aura=boss&&(in.flags&4U)&&in.defeats>=1;
    if((boss&&(in.flags&4U)&&in.defeats<0)||(aura&&(in.aura_size<0||in.aura_size>3)))return {};
    if(!boss) {
        if(!in.outbreak)return {};
    }
    const auto outer=steam_startup_window_skin(
        {222,170,0,0,SteamStartupTextRole::message_title,0},o.view_y,o.title_widths);
    const auto inner=steam_startup_box_skin({15,139,224,202},o.view_y);
    if(!outer||!inner)return {};
    SteamMonsterSkinPlan p;
    p.title=boss?(o.japanese?"ボス出現！":"BOSS出现！"):
        (o.japanese?"ﾓﾝｽﾀｰ大量発生":"怪物大量出现");
    for(const auto &draw:outer->draws)p.operations.emplace_back(draw);
    for(const auto &draw:inner->draws)p.operations.emplace_back(draw);
    for(const auto &part:std::array<std::array<int,6>,3>{{
        {30,66,0,0,93,82},{109,66,1,0,91,82},{189,66,1,0,20,82}}})
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::residence_background,
            {part[0],part[1]},0,std::array<int,4>{part[2],part[3],part[4],part[5]}});
    p.operations.emplace_back(SteamStartupFrameDraw{StartupSkinRect{{30,66,179,81},{148,148,148},true}});
    const auto body=[&](int x,int y,int direction,int clock) {
        p.operations.emplace_back(SteamMonsterBody{m.body*30+m.variant,m.body*4+direction,
            (clock%(speeds[m.body]*4))/speeds[m.body],{x,y}});
    };
    if(boss) {
        // Steam BOSS_ANIME_DATA独立核值；本表是显示分段，不推进怪物AI或页主钟。
        constexpr std::array<std::array<int,6>,13> steps{{
            {0,0,2,120,120,5},{1,0,2,120,74,12},{0,0,2,74,74,5},
            {2,0,2,74,74,12},{0,0,2,74,74,10},{1,0,1,74,166,24},
            {0,0,1,166,166,5},{2,0,1,166,166,12},{0,0,1,166,166,10},
            {1,0,2,166,120,12},{0,0,2,120,120,5},{2,0,2,120,120,12},
            {0,0,2,120,120,5}}};
        const int time=m.counter%129;
        int index{},start{};
        while(index<12&&time>=start+steps[index][5])start+=steps[index++][5];
        const auto &step=steps[index];const int local=time-start;
        const int x=step[3]+(step[0]==1?local*(step[4]-step[3])/step[5]:0);
        int dy{};
        if(step[0]==2) {
            // GetParabolaValue(8,12,local)：每个float32阶段都舍入，随后向零转整数。
            const auto mul=[](float a,float b){volatile float v=a*b;return v;};
            const auto div=[](float a,float b){volatile float v=a/b;return v;};
            const auto add=[](float a,float b){volatile float v=a+b;return v;};
            const float velocity=div(16.F,5.F),acceleration=div(-16.F,30.F);
            const float curve=mul(mul(mul(float(local),acceleration),float(local+1)),0.5F);
            dy=-static_cast<int>(std::max(0.F,add(curve,mul(float(local),velocity))));
        }
        if(aura) {
            constexpr std::array<SteamMonsterAsset,4> assets{SteamMonsterAsset::aura0,
                SteamMonsterAsset::aura1,SteamMonsterAsset::aura2,SteamMonsterAsset::aura3};
            p.operations.emplace_back(SteamMonsterImage{assets[in.aura_size],{x,140+dy},(m.counter%4)/2});
        }
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::shadow,{x,138},m.shadow,{}});
        body(x,138+dy,step[2],step[0]==2?0:time);
        if(index==4||index==8||index==12)
            p.operations.emplace_back(SteamMonsterImage{SteamMonsterAsset::smoke,{x,142},local<3?0:local<6?1:2});
    } else {
        p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{30,66,180,82}});
        for(const auto &row:*in.outbreak) {
            if(row[7]==0)continue;
            const int x=row[0]/100;
            p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::shadow,{x,139},m.shadow,{}});
            // 外层世界步帧会被SetDrawParam用页主钟重算；不取行CNT或世界计数画身体。
            body(x,139,row[2]==1?2:1,m.counter);
        }
        p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
    }
    const int height=std::clamp(m.counter-5,0,35)*48/35;
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::push_intersect,{0,158,240,height}});
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        m.name+(o.japanese?"が":""),{120,164,0,0},2,0,false,{92,51,31}});
    p.operations.emplace_back(SteamHumanOverviewText{SteamHumanOverviewTextRole::result_message,
        boss?(o.japanese?"出現しました!!":"出现了!!"):
            (o.japanese?"大量発生しています!!":"正在大量出现!!"),{120,188,0,0},2,0,false,{92,51,31}});
    p.operations.emplace_back(SteamHumanClip{SteamHumanClipKind::pop,{}});
    if(m.counter>=40&&(m.counter-40)%25<15)
        p.operations.emplace_back(SteamHumanImage{SteamHumanAsset::continue_cursor,{212,204},0,{}});
    p.operations.emplace_back(SteamStartupTouch{2,0,{},{},2});
    return p;
}
std::optional<SteamMonsterSkinPlan> steam_monster_notice_skin(
    const StartupWorldRuntimeState &s,std::uint64_t id,const SteamMonsterSkinOptions &o) {
    const auto page=std::find_if(s.scripts.pages.rbegin(),s.scripts.pages.rend(),
        [](const auto &p){return p.lifecycle!=4;});
    if(!s.rules||page==s.scripts.pages.rend()||page->id!=id||!id||page->lifecycle!=2||
       page->kind!=ref::WorldScriptPageKind::raw_page||(page->legacy_page!=99&&page->legacy_page!=100)||
       !page->monster_definition||!s.task_display_initialized.count(id))return {};
    const auto count=s.page_counters.find(id);
    if(count==s.page_counters.end())return {};
    const auto d=std::find_if(s.rules->monsters.begin(),s.rules->monsters.end(),
        [&](const auto &m){return m.identity==*page->monster_definition;});
    const auto current=s.scene.world.world.ai.monster_growth.find(*page->monster_definition);
    if(d==s.rules->monsters.end()||current==s.scene.world.world.ai.monster_growth.end())return {};
    const auto &m=current->second;
    // Owner承接既有WorldTaskDisplayState的合法域；纯绘制的STATE!=0／DIRECT==1谓词不因此改写。
    if(page->legacy_page==100)for(const auto &row:s.task_display_table)
        if(row[2]<0||row[2]>1||row[3]<0||row[4]<0||row[7]<0||row[7]>2||row[8]<0)return {};
    SteamMonsterNoticeInput input;
    input.legacy_page=page->legacy_page;
    input.monster={count->second,m.body,m.sprite_variant,d->shadow,d->name};
    input.flags=d->flags;input.defeats=m.growth;input.aura_size=d->aura_size;
    if(page->legacy_page==100)input.outbreak=s.task_display_table;
    return steam_monster_notice_skin(input,o);
}
} // namespace ark::simulation
