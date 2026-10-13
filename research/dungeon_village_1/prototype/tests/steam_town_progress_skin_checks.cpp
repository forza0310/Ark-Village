#include "dungeon_village_prototype/steam_town_progress_skin.hpp"
#include "dungeon_village_prototype/startup_world_runtime.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_commerce.hpp"
#include "support/world_fixture.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>

using namespace dungeon_village_prototype;
namespace {
struct Checks {
    int count{};
    void operator()(bool ok,const char *why) {++count;if(!ok)throw std::runtime_error(std::string("Steam进度皮肤：")+why);}
};
template<class T> std::vector<T> parts(const SteamTownProgressPlan &p) {
    std::vector<T> result;for(const auto &op:p.operations)if(const auto *v=std::get_if<T>(&op))result.push_back(*v);return result;
}
SteamTownProgressOptions options() {
    SteamTownProgressOptions o;o.title_widths=std::array<int,2>{60,60};o.condition_suffix_width=60;
    o.bottom_width=24;o.promoted_suffix_width=36;o.reward_number_width=24;
    o.bubble_widths={24,36,24,12};return o;
}
void conditions(Checks &c) {
    c(steam_town_conditions_cursor(48,0,true,false)==4&&steam_town_conditions_cursor(48,4,false,true)==0,
      "48五项循环，解释行不是四项目录");
    c(steam_town_conditions_cursor(48,0,true,true)==0&&steam_town_conditions_cursor(49,0,true,true)==0,
      "同轮up后down；49固定0");
    c(!steam_town_conditions_cursor(49,1,false,false)&&!steam_town_conditions_cursor(48,-1,true,false),"越界选择拒绝");
    SteamTownConditionsInput in;in.options=options();in.village_name="夹具村";in.selection=2;
    in.rows={{{0,5000,5000,true,{}},{2,1,2,false,{}},{3,0,35,false,"面包房"},{5,300,300,true,{}}}};
    const auto p=steam_town_conditions_skin(in);c(p.has_value(),"四种条件合法纯绘制");
    const auto numbers=parts<SteamFacilityNumber>(*p);const auto touches=parts<SteamStartupTouch>(*p);
    c(numbers.size()==7&&numbers[1].kind==SteamFacilityNumberKind::money&&numbers[1].position==std::array<int,2>{133,117}&&
      numbers[3].kind==SteamFacilityNumberKind::money&&numbers[5].kind==SteamFacilityNumberKind::number,
      "收入/居民原money helper，人气普通number；指定设施不造数值");
    c(touches.size()==4&&touches[1].rectangle==std::optional<std::array<int,4>>({2,133,232,16})&&touches[1].value==0x20002,
      "条件行保留实际11与宽热区");
    const auto images=parts<SteamTownImage>(*p);
    c(images[0].position==std::array<int,2>{147,69}&&images[4].position==std::array<int,2>{195,69}&&
      images.back().asset==SteamTownAsset::residence_icon&&images.back().position==std::array<int,2>{67,133},
      "原Draw_star起点与非日文住宅标签图");
    in.selection=0;const auto bottom=steam_town_conditions_skin(in);const auto controls=parts<SteamFacilityEffectTouch>(*bottom);
    c(controls.size()==1&&controls[0].touch.component==3&&controls[0].margin==std::array<int,4>{0,0,-10,0},
      "未达申请按钮原上margin负10");
    for(auto &r:in.rows)r.met=true;
    c(parts<SteamFacilityEffectTouch>(*steam_town_conditions_skin(in))[0].margin[2]==-20,"满足升级按钮上margin负20");
    in.raw_page=49;const auto readonly=steam_town_conditions_skin(in);
    c(readonly&&parts<SteamFacilityEffectTouch>(*readonly).empty()&&parts<SteamStartupTouch>(*readonly).size()==1,
      "49了解只有原组件4，不借48四行触摸");
    in.options.bottom_width.reset();c(!steam_town_conditions_skin(in),"实际测宽缺失拒绝，不能估计中文字宽");
}
void celebration(Checks &c) {
    SteamTownCelebrationInput in;in.rank=2;in.options=options();in.phase=1;in.frame=4;
    in.participants={{0,0,0,{287,143,3,0}},{1,0,0,{273,135,1,1}}};
    const auto blink=steam_town_celebration_skin(in);c(blink.has_value(),"50第二段");
    const auto stars=parts<SteamTownImage>(*blink);
    c(std::any_of(stars.begin(),stars.end(),[](const auto &i){return i.asset==SteamTownAsset::rank_star&&i.position==std::array<int,2>{107,172}&&i.frame==2;}),
      "新增末星恰第4帧闪2，不把整排星一起闪");
    in.phase=0;in.frame=35;const auto give=steam_town_celebration_skin(in);
    c(give&&parts<SteamTownImage>(*give)[1].asset==SteamTownAsset::award&&parts<SteamTownImage>(*give)[1].frame==2,
      "第一段30..59持星人物sp动作3→SEB72/frame2");
    in.phase=2;in.frame=30;in.frame2=7;const auto enter=steam_town_celebration_skin(in);
    const auto people=parts<SteamHumanPortrait>(*enter);
    c(people.size()==2&&people[0].position==std::array<int,2>{198,135}&&people[0].actor.body.sprite==2&&people[0].actor.body.frame==1&&
      people[1].position==std::array<int,2>{212,143},"原50后层先绘且位移只读X，步来自独立frame2");
    in.frame=70;const auto carry=steam_town_celebration_skin(in);const auto carry_images=parts<SteamTownImage>(*carry);
    c(std::any_of(carry_images.begin(),carry_images.end(),[](const auto &i){return i.asset==SteamTownAsset::president_carry&&i.position==std::array<int,2>{136,130}&&i.frame==0;})&&
      std::none_of(carry_images.begin(),carry_images.end(),[](const auto &i){return i.asset==SteamTownAsset::rank_star&&i.position==std::array<int,2>{131,130};}),
      "70切抬大臣，手中星退休");
    in.frame=85;const auto lifted=steam_town_celebration_skin(in);const auto lifted_people=parts<SteamHumanPortrait>(*lifted);
    c(lifted_people[0].actor.body.sprite==17&&lifted_people[0].position==std::array<int,2>{123,135}&&
      lifted_people[1].actor.body.sprite==19&&lifted_people[1].position==std::array<int,2>{136,143},
      "抬举动作8各方向横偏不同，不移动冻结X");
    in.frame=140;const auto end=steam_town_celebration_skin(in);c(end.has_value(),"140终段合法");
    c(parts<SteamHumanImage>(*end).back().asset==SteamHumanAsset::continue_cursor,"140才显示可继续箭头");
    in.frame=139;const auto before=steam_town_celebration_skin(in);const auto before_images=parts<SteamHumanImage>(*before);
    c(std::none_of(before_images.begin(),before_images.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;}),"139不提前显示可继续");
    in.participants.push_back(in.participants.front());c(!steam_town_celebration_skin(in),"重复冻结人物拒绝");
}
void gifts(Checks &c) {
    SteamTownGiftInput in;in.options=options();in.kind=1;in.value=100;in.frame=35;
    const auto fast=steam_town_gift_skin(in);c(fast.has_value(),"95点数未完成展开");
    const auto clips=parts<SteamTownClip>(*fast);
    c(clips.size()==2&&clips[0].kind==SteamTownClipKind::replace&&clips[0].rectangle==std::array<int,4>{0,152,240,49},
      "35展开49高度，SetClip替换而非交集");
    c(parts<SteamFacilityNumber>(*fast)[0].asset==SteamFacilityAsset::number11,"点数实际SEB20，不借SEB18同PNG");
    for(int kind:{0,1,3,4,9,10,11}) {
        in.kind=kind;in.value=kind==4?0:100;in.mapchip=0;in.name="夹具定义";in.activity_icon=0;in.frame=40;
        const auto p=steam_town_gift_skin(in);c(p.has_value(),"七种已维护领取原序计划");
        c(parts<SteamStartupTouch>(*p).back().component==2&&parts<SteamStartupTouch>(*p).back().option==2,
          "领取页只返回KEYCLICK，无规则消费或取消");
    }
    in.kind=11;in.activity_flags=0;const auto activity=parts<SteamTownText>(*steam_town_gift_skin(in));
    c(std::count_if(activity.begin(),activity.end(),[](const auto &t){return t.rectangle[1]==184;})==1&&activity.back().value=="可以开展了。",
      "活动bit8假只出一条可开展后缀，拒绝JADX误落流");
    in.activity_flags=8;const auto layout=parts<SteamTownText>(*steam_town_gift_skin(in));
    c(layout.back().value=="可实施了。","bit8真实施后缀");
    in.kind=2;c(!steam_town_gift_skin(in),"未维护人物奖励不伪装成功");
    in.kind=4;in.value=0;in.sex=2;c(!steam_town_gift_skin(in),"职业预览错误性别拒绝");
}
void owner(Checks &c) {
    auto s=test_support::page_fixture(48);const auto id=s.scripts.pages.back().id;auto o=options();
    c(!steam_town_conditions_skin(s,id,o),"Owner未初始化不靠查询补字段");
    c(initialize_startup_world_town_progress_pages(s),"实际初始化条件夹具");s.scripts.pages.back().lifecycle=2;
    const auto digest=startup_world_state_digest(s);const auto p=steam_town_conditions_skin(s,id,o);c(p.has_value(),"Owner条件只读桥");
    for(int i=0;i<8;++i)c(steam_town_conditions_skin(s,id,o)->operations.size()==p->operations.size()&&startup_world_state_digest(s)==digest,
      "重复查询资源规模不变，不清缓存/随机/输出");
    auto bad=s;bad.page_phases.erase(id);c(!steam_town_conditions_skin(bad,id,o),"已初始化缺选择拒绝");
    bad=s;bad.scripts.pages.back().lifecycle=4;c(!steam_town_conditions_skin(bad,id,o),"退休页不再保持绘制引用");
    bad=s;bad.page_phases[id]=5;c(!steam_town_conditions_skin(bad,id,o),"越界行不以map.at异常代替显式拒绝");
}
void source_gift_pages(Checks &c) {
    const auto initial=test_support::world_fixture();
    const auto giver=startup_world_human_details(initial,1);c(bool(giver),"94有真实giver定义");
    SteamTownGiftInput in;in.raw_page=93;in.kind=3;in.value=0;in.mapchip=0;
    in.name=initial.rules->facilities.front().name;in.frame=50;in.options=options();
    in.options.bubble_widths[3].reset();
    auto plan=steam_town_gift_skin(in);c(bool(plan),"93设施领取没有giver/气泡字宽依赖");
    c(parts<SteamHumanPortrait>(*plan).empty()&&std::none_of(plan->operations.begin(),plan->operations.end(),[](const auto &op){
        const auto *v=std::get_if<SteamTownText>(&op);return v&&v->value=="请";
    }),"93不同于94/95，不画175赠送演员或请气泡");
    const auto purchase_words=parts<SteamTownText>(*plan);
    c(purchase_words.back().value=="可以建设了。"&&purchase_words.back().rectangle[1]==192,"93建设开放三行文本保持末行192");
    const auto career=startup_title_actor_skin(giver->profession,giver->sex,-1,0,2,false);c(bool(career),"94giver皮肤源");
    for(int reward:{3,4,5,6,7,8}) {
        in.raw_page=94;in.kind=reward;in.options=options();
        in.giver=SteamTownGiftInput::Giver{giver->profession,giver->sex,giver->name};in.reward_icon.clear();
        if(reward==4) {in.value=0;in.sex=0;in.name=initial.rules->jobs.front().name;}
        else if(reward>=5&&reward<=7) {
            const int kind=reward-4;const auto d=std::find_if(initial.rules->equipment.begin(),initial.rules->equipment.end(),
                [=](const auto &v){return v.shop.kind==kind;});c(d!=initial.rules->equipment.end(),"94装备独立namespace源");
            in.value=d->shop.id;in.name=d->name;in.reward_icon=*startup_world_equipment_icon_draws(initial,kind,in.value);
        } else if(reward==8) {
            const auto &d=initial.rules->items.front();in.value=d.identity;in.name=d.name;
            in.reward_icon=*startup_world_item_icon_draws(initial,in.value);
        }
        plan=steam_town_gift_skin(in);c(bool(plan),"94六种已维护通知纯计划");
        const auto people=parts<SteamHumanPortrait>(*plan);
        c(!people.empty()&&people.back().position==std::array<int,2>{175,145}&&
          people.back().actor.body.image==career->body.image&&!people.back().actor.weapon,
          "94原m当前职业/性别赠送演员，无武器，不套95special演员");
        const auto words=parts<SteamTownText>(*plan);
        c(std::any_of(words.begin(),words.end(),[=](const auto &v){return v.value==(reward==4?"注目!":"请");}),
          "94职业独立注目句，其余通知请句");
        if(reward>=5) {
            const auto images=parts<SteamHumanImage>(*plan);const auto wanted=reward==5?SteamHumanAsset::weapon_icon:
                reward==6?SteamHumanAsset::armour_icon:reward==7?SteamHumanAsset::accessory_icon:SteamHumanAsset::item_icon;
            c(std::any_of(images.begin(),images.end(),[=](const auto &v){return v.asset==wanted;}),
              "94四奖励图标沿武器/防具/饰品/道具namespace，不按共享定义ID猜裁片");
        }
        auto broken=in;broken.giver.reset();c(!steam_town_gift_skin(broken),"94缺giver不能补95演员");
        if(reward==4) {
            broken=in;broken.options.bubble_widths[3].reset();c(!steam_town_gift_skin(broken),"94显示注目气泡必须测本句");
            broken.frame=39;c(bool(steam_town_gift_skin(broken)),"94未到40不依赖尚未显示的气泡测宽");
        }
        auto s=test_support::page_fixture(94);const auto id=s.scripts.pages.back().id;
        auto &page=s.scripts.pages.back();page.lifecycle=0;page.legacy_r=reward;page.legacy_s=in.value;page.legacy_t=in.sex;
        s.page_human_bindings[id]=1;
        c(initialize_startup_world_gift_notice_pages(s),"94真实通知Init，仅安装主钟");
        s.scripts.pages.back().lifecycle=2;s.page_counters[id]=50;
        const auto digest=startup_world_state_digest(s);const auto draws=s.scene.random.draws();
        const auto bound=steam_town_gift_skin(s,id,in.options);c(bool(bound),"94严格Owner桥六类型");
        c(startup_world_state_digest(s)==digest&&s.scene.random.draws()==draws,"94投影不领取/解锁/改库存或随机");
        auto bad=s;bad.page_human_bindings.erase(id);c(!steam_town_gift_skin(bad,id,in.options),"94缺冻结giver绑定拒绝");
        bad=s;bad.page_counters.erase(id);c(!steam_town_gift_skin(bad,id,in.options),"94缺main不能临时初始化");
        bad=s;bad.page_secondary_counters[id]=0;c(!steam_town_gift_skin(bad,id,in.options),"94误挂其它结果页副钟拒绝");
        if(reward==8)for(int repeat=0;repeat<3;++repeat)c(steam_town_gift_skin(s,id,in.options)->operations.size()==bound->operations.size()&&
            startup_world_state_digest(s)==digest,"94重复计划输出规模稳定，不消费通知");
    }
    auto purchase=test_support::page_fixture(93);const auto id=purchase.scripts.pages.back().id;
    const auto definition=std::find_if(purchase.rules->facilities.begin(),purchase.rules->facilities.end(),
        [&](const auto &d){const auto status=purchase.facility_presence.find(d.id);return status!=purchase.facility_presence.end()&&status->second==0;});
    c(definition!=purchase.rules->facilities.end(),"93未领取设施源");
    purchase.scripts.pages.back().lifecycle=0;
    purchase.scripts.pages.back().legacy_r=3;purchase.scripts.pages.back().legacy_s=definition->id;
    c(initialize_startup_world_commerce_pages(purchase),"93真实commerce Init安装唯一领取载荷");
    purchase.scripts.pages.back().lifecycle=2;purchase.page_counters[id]=50;
    const auto digest=startup_world_state_digest(purchase);const auto bound=steam_town_gift_skin(purchase,id,options());
    c(bound&&parts<SteamHumanPortrait>(*bound).empty()&&startup_world_state_digest(purchase)==digest,
      "93实际Owner领取前只读桥不解锁设施或造giver");
    purchase.commerce_page_data.erase(id);c(!steam_town_gift_skin(purchase,id,options()),"93已初始化缺领取payload拒绝");
}
void resources(Checks &c,const std::filesystem::path &source_root) {
    const auto root=source_root.parent_path();
    for(auto asset:{SteamTownAsset::star,SteamTownAsset::rank_star,SteamTownAsset::rank_frame,
        SteamTownAsset::award,SteamTownAsset::president_carry,SteamTownAsset::shadow,
        SteamTownAsset::celebration_background,SteamTownAsset::reward_background,SteamTownAsset::mini1,
        SteamTownAsset::mini2,SteamTownAsset::move_facility,SteamTownAsset::residence_icon,SteamTownAsset::activity_icon}) {
        const auto r=steam_town_resource(asset);c(r&&std::filesystem::is_regular_file(root/r->published_image),"既有出版图片存在，无新复制素材");
        if(r->published_sprite)c(std::filesystem::is_regular_file(root/r->published_sprite),"已核SEB路径存在");
    }
}
}
// 扩展现有visuals套件；规则、资金、随机消费和确认时点仍由原pages/gift套件负责。
int check_steam_town_progress_skin(const std::filesystem::path &source_root) {
    Checks c;conditions(c);celebration(c);gifts(c);owner(c);source_gift_pages(c);resources(c,source_root);return c.count;
}
