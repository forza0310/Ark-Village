#include "ark/simulation/presentation/steam_town_progress_skin.hpp"
#include "ark/simulation/world/startup_world_runtime.hpp"
#include "ark/simulation/persistence/startup_world_persistence.hpp"
#include "support/world_fixture.hpp"
#include <algorithm>
#include <filesystem>
#include <stdexcept>

using namespace ark::simulation;
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
void resources(Checks &c,const std::filesystem::path &source_root) {
    const auto root=source_root;
    for(auto asset:{SteamTownAsset::star,SteamTownAsset::rank_star,SteamTownAsset::rank_frame,
        SteamTownAsset::award,SteamTownAsset::president_carry,SteamTownAsset::shadow,
        SteamTownAsset::celebration_background,SteamTownAsset::reward_background,SteamTownAsset::mini1,
        SteamTownAsset::mini2,SteamTownAsset::move_facility,SteamTownAsset::residence_icon,SteamTownAsset::activity_icon}) {
        const auto r=steam_town_resource(asset);c(r&&std::filesystem::is_regular_file(root/std::filesystem::path(r->published_image).lexically_relative("original")),"既有出版图片存在，无新复制素材");
        if(r->published_sprite)c(std::filesystem::is_regular_file(root/std::filesystem::path(r->published_sprite).lexically_relative("original")),"已核SEB路径存在");
    }
}
}
// 扩展现有visuals套件；规则、资金、随机消费和确认时点仍由原pages/gift套件负责。
int check_steam_town_progress_skin(const std::filesystem::path &source_root) {
    Checks c;conditions(c);celebration(c);gifts(c);owner(c);resources(c,source_root);return c.count;
}
