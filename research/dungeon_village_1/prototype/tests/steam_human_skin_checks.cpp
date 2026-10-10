#include "dungeon_village_prototype/steam_human_skin.hpp"
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_persistence.hpp"
#include "support/world_fixture.hpp"
#include "dungeon_village_tools/sprite.hpp"
#include <raylib.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

using namespace dungeon_village_prototype;
namespace {
struct Checks {
    int count{};
    void operator()(bool ok,const char *why) {
        ++count;if(!ok)throw std::runtime_error(std::string("Steam人物皮肤：")+why);
    }
};
template<class T> std::vector<T> parts(const SteamHumanSkinPlan &plan) {
    std::vector<T> result;
    for(const auto &draw:plan.draws)if(const auto *value=std::get_if<T>(&draw))result.push_back(*value);
    return result;
}
std::vector<std::uint8_t> bytes(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary);
    if(!in)throw std::runtime_error("人物素材缺失："+path.string());
    return {std::istreambuf_iterator<char>(in),{}};
}
void power_checks(Checks &check) {
    const auto plan=steam_human_power_skin({75,121},1,0,SteamHumanHpInput{800,309,1000});
    check(plan.has_value(),"合法HP图元");
    const auto r=parts<StartupSkinRect>(*plan);
    const auto images=parts<SteamHumanImage>(*plan);
    check(r.size()==4 && r[0].rect==std::array<int,4>{64,95,22,6} && r[0].outline &&
          r[1].rect==std::array<int,4>{65,96,20,4} && r[1].outline &&
          r[2].rect==std::array<int,4>{66,97,14,2} && !r[2].outline &&
          r[3].rect==std::array<int,4>{80,97,4,2} && !r[3].outline,
          "NOW控制长度，DrawRect加1为半开边框，FillRect保留18总宽");
    check(r[0].rgb==std::array<int,3>{246,246,246} && r[1].rgb==std::array<int,3>{39,53,74} &&
          r[2].rgb==std::array<int,3>{83,255,0} && r[3].rgb==std::array<int,3>{68,100,104},
          "血条四层原颜色与顺序");
    check(images.size()==1 && images[0].asset==SteamHumanAsset::pinch && images[0].frame==0 &&
          images[0].position==std::array<int,2>{61,93},"AFTER整数30.9%仍触发危险帧0");
    check(parts<SteamHumanImage>(*steam_human_power_skin({75,121},1,0,SteamHumanHpInput{0,310,1000})).empty(),
          "NOW为0但AFTER31%不触发危险");
    const auto right=parts<SteamHumanImage>(*steam_human_power_skin({98,121},2,5,SteamHumanHpInput{80,30,100}));
    check(right.size()==1 && right[0].position==std::array<int,2>{103,93},"direction2原偏移5/-28");
    check(parts<SteamHumanImage>(*steam_human_power_skin({75,121},1,6,SteamHumanHpInput{80,30,100})).empty(),
          "12槽闪烁后半段不绘危险标");
    check(steam_human_power_skin({75,121},1,0,{})->draws.empty(),"无实例不补满HP或危险图标");
    for(const auto sample:std::array<std::array<int,2>,4>{{{{-1,0}},{{0,0}},{{100,18}},{{101,18}}}}) {
        const auto bar=parts<StartupSkinRect>(*steam_human_power_skin({75,121},1,6,SteamHumanHpInput{sample[0],0,100}));
        check(bar[2].rect[2]==sample[1],"越界HP按原端点夹限，不修改HP槽");
    }
    check(steam_human_power_skin({75,121},1,6,SteamHumanHpInput{std::numeric_limits<int>::max(),
              std::numeric_limits<int>::max(),100}).has_value(),"端点夹限和隐藏闪烁不进行无关溢出乘法");
    check(!steam_human_power_skin({75,121},1,-1,{}) &&
          !steam_human_power_skin({75,121},4,0,{}) &&
          !steam_human_power_skin({75,121},1,0,SteamHumanHpInput{0,0,0}) &&
          !steam_human_power_skin({std::numeric_limits<int>::min(),121},1,0,SteamHumanHpInput{0,0,100}),
          "坏方向/计数/最大HP/坐标显式拒绝");
}
void medal_bubble_checks(Checks &check) {
    check(steam_human_medal_skin(0)->draws.empty(),"0枚不绘");
    const auto four=parts<SteamHumanImage>(*steam_human_medal_skin(4));
    check(four.size()==4,"1至4只绘重复奖章");
    for(int i=0;i<4;++i)
        check(four[i].position==std::array<int,2>{103-10*i,70} &&
              four[i].crop==std::optional<std::array<int,4>>({14,0,14,14}),"mode6/index1向左排列，原14×14裁片");
    const auto five=parts<SteamHumanImage>(*steam_human_medal_skin(5));
    check(five.size()==3 && five[0].position==std::array<int,2>{83,70} &&
          five[1].asset==SteamHumanAsset::number05 && five[1].frame==16 &&
          five[1].position==std::array<int,2>{95,74} && five[2].frame==5 &&
          five[2].position==std::array<int,2>{105,74},"5枚切换乘号SEB16及右对齐单数字");
    const auto many=parts<SteamHumanImage>(*steam_human_medal_skin(123));
    check(many.size()==5 && many[0].position==std::array<int,2>{67,70} &&
          many[1].position==std::array<int,2>{79,74} && many[1].frame==16 &&
          many[2].position==std::array<int,2>{89,74} && many[2].frame==1 &&
          many[3].position==std::array<int,2>{97,74} && many[3].frame==2 &&
          many[4].position==std::array<int,2>{105,74} && many[4].frame==3,
          "多位数字8步宽，奖章宽14不参与数字宽度");
    const auto small=steam_human_down_bubble_skin(60),large=steam_human_down_bubble_skin(61);
    const auto a=parts<SteamHumanImage>(*small),b=parts<SteamHumanImage>(*large);
    check(a.size()==3 && a[0].crop==std::optional<std::array<int,4>>({6,0,60,21}) &&
          a[0].position==std::array<int,2>{45,79} && a[1].frame==0 && a[1].position==std::array<int,2>{39,79} &&
          a[2].frame==1 && a[2].position==std::array<int,2>{105,79},"W60保留中段尾部与两帽");
    check(b.size()==6 && b[0].crop==std::optional<std::array<int,4>>({6,0,27,21}) &&
          b[1].position==std::array<int,2>{72,79} && b[2].crop==std::optional<std::array<int,4>>({6,0,7,21}) &&
          b[2].position==std::array<int,2>{99,79} && b[3].crop==std::optional<std::array<int,4>>({33,0,5,21}) &&
          b[3].position==std::array<int,2>{73,79} && b[4].frame==0 && b[5].frame==1,
          "W61铺27/27/7后只补一次中心尾部，再绘端帽");
    const auto text=parts<SteamHumanText>(*large);
    check(text.size()==1 && text[0].position==std::array<int,2>{75,82} && text[0].anchor==2 &&
          text[0].rgb==std::array<int,3>{0,0,0},"倒下文字原中心锚及黑色，不改字体字号");
    check(!steam_human_medal_skin(-1) && !steam_human_down_bubble_skin(-1) &&
          !steam_human_down_bubble_skin(4097),"负奖章/宽度与维护输出预算拒绝");
}
void resources(Checks &check,const std::filesystem::path &source_root) {
    const auto assets=source_root.parent_path();
    for(const auto asset:{SteamHumanAsset::medal,SteamHumanAsset::pinch,SteamHumanAsset::bubble,SteamHumanAsset::number05}) {
        const auto resource=steam_human_resource(asset);
        check(resource.has_value(),"具名资源身份存在");
        Image image=LoadImage((assets/resource->published_image).string().c_str());
        check(image.data!=nullptr,"正式PNG可解码");
        if(resource->published_sprite) {
            const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(assets/resource->published_sprite));
            for(const auto &layer:seb.layers)for(const auto &p:layer.parts) {
                if(p.width<=0 || p.height<=0)continue;
                check(p.image_index==resource->image && p.source_x>=0 && p.source_y>=0 &&
                      p.source_x+p.width<=image.width && p.source_y+p.height<=image.height,
                      "实际SEB图片身份与裁片在正式PNG内");
                if(asset==SteamHumanAsset::number05 && p.frame==16)
                    check(p.source_x==61 && p.source_y==10 && p.width==9 && p.height==10,
                          "乘号原裁片61/10/9/10");
            }
        } else check(image.width>=28 && image.height>=14,"奖章index1裁片实际存在");
        UnloadImage(image);
    }
    const auto number=steam_human_resource(SteamHumanAsset::number05);
    check(bytes(assets/number->published_image)!=bytes(source_root/"common/number05.png"),
          "Steam数字PNG不静默替换为APK同名图");
    check(!steam_human_resource(static_cast<SteamHumanAsset>(99)),"未知资源枚举拒绝");
}
void owner_checks(Checks &check) {
    // 页面/HP/实例均为表现条件夹具，不宣称新局自然出现两名同定义冒险者。
    auto owner=test_support::page_fixture(60);
    auto &page=owner.scripts.pages.back();const auto id=page.id;
    page.lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.human_homes.at(1)[2]=1;
    owner.shop_humans.at(1).equipment[0].reset();
    check(steam_human_detail_skin(owner,id).has_value(),"初始化条件页60产生只读皮肤");
    auto plan=*steam_human_detail_skin(owner,id);
    check(plan.after_portrait.draws.empty() && !plan.portrait.weapon && !plan.portrait.shadow,
          "无实例保留定义肖像，但不补HP/危险/标题阴影");
    check(plan.portrait_position==std::array<int,2>{75,121} && plan.portrait.body.sprite==1 &&
          plan.portrait.body.frame==0,"frame0住宅肖像原锚与方向");
    owner.page_counters[id]=100;plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait_position==std::array<int,2>{75,121} && plan.portrait.body.frame==1,
          "位移100槽回原点，步行16槽仍取完整frame，不能一并清零");
    owner.page_counters[id]=50;plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait_position==std::array<int,2>{98,121} && plan.portrait.body.sprite==2,
          "第4段固定98/方向2");
    owner.human_homes.at(1)[2]=0;plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait_position==std::array<int,2>{86,121},"非居民肖像x减12");
    owner.scene.world.world.ai.growth.at(1).definition.current_profession=2;
    owner.shop_humans.at(1).equipment[0]=0;
    plan=*steam_human_detail_skin(owner,id);
    check(plan.portrait.body.image==owner.rules->jobs.at(2).sprites.at(owner.rules->humans.at(1).sex) &&
          plan.portrait.weapon.has_value(),"消费当前职业和当前主武器而非初始人物缓存");
    auto &ai=owner.scene.world.world.ai;
    ref::BattleActorRecord first;first.id={900};first.kind=ref::ActorKind::human;first.definition=1;
    first.control.state=0;first.hp.displayed=80;first.hp.target=90;first.capacity=999;
    auto second=first;second.id={901};second.hp.displayed=0;second.hp.target=0;
    ai.battle.actors.emplace(first.id,first);ai.battle.actors.emplace(second.id,second);
    ai.human_order={second.id,first.id};ai.growth.at(1).derived.combat[0]=100;
    owner.page_counters[id]=0;
    plan=*steam_human_detail_skin(owner,id);
    check(parts<StartupSkinRect>(plan.after_portrait)[2].rect[2]==0 &&
          parts<SteamHumanImage>(plan.after_portrait).size()==1,"按名单原序首实例读取HP，不按ID排序");
    ai.human_order={first.id,second.id};plan=*steam_human_detail_skin(owner,id);
    check(parts<StartupSkinRect>(plan.after_portrait)[2].rect[2]==14 &&
          parts<SteamHumanImage>(plan.after_portrait).empty(),"HP最大取共享定义100，不借旧实例capacity999");
    auto bound=owner;
    bound.actor_metadata.emplace(second.id,StartupWorldActorMetadata{1,3,0,{}});
    bound.human_detail_contexts.at(id)={1,second.id}; // 镜头确认来源的明确W条件，不宣称自然双实例。
    const auto bound_plan=steam_human_detail_skin(bound,id);
    check(bound_plan && parts<StartupSkinRect>(bound_plan->after_portrait)[2].rect[2]==0 &&
          parts<SteamHumanImage>(bound_plan->after_portrait).size()==1,
          "source1显式W优先于同定义首实例，不能误显示另一人的HP");
    ai.battle.actors.at(first.id).control.state=7;
    check(!steam_human_detail_skin(owner,id),"倒下缺实际测宽不猜字宽");
    plan=*steam_human_detail_skin(owner,id,60);
    check(plan.portrait_position==std::array<int,2>{75,121} && plan.portrait.body.sprite==13 &&
          plan.portrait.body.frame==0 && !plan.portrait.weapon &&
          parts<StartupSkinRect>(plan.after_portrait).empty() && parts<SteamHumanText>(plan.after_portrait).size()==1,
          "倒下走固定身体13/0和气泡，不复用普通位移/HP/武器");
    owner.scripts.pages.back().lifecycle=3;
    ref::WorldScriptPage child;child.id=owner.scripts.next_page_id++;child.kind=ref::WorldScriptPageKind::raw_page;
    child.legacy_page=61;child.lifecycle=1;owner.scripts.pages.push_back(child);
    const auto before=startup_world_state_digest(owner);
    check(steam_human_detail_skin(owner,id,60).has_value() && startup_world_state_digest(owner)==before,
          "被子页覆盖的父60生命周期3仍可只读绘制，不推进Owner/随机/输出");
    for(int missing=0;missing<5;++missing) {
        auto bad=owner;
        if(missing==0)bad.page_human_bindings.erase(id);
        if(missing==1)bad.page_phases.erase(id);
        if(missing==2)bad.page_counters.erase(id);
        if(missing==3)bad.human_page_selections.erase(id);
        if(missing==4)bad.human_pages_initialized.erase(id);
        check(!steam_human_detail_skin(bad,id,60),"已初始化但缺必需键显式拒绝，不抛map.at");
    }
}
}
// 复用既有visuals执行文件；本层只验表现边界，不重测人物成长/战斗/职业业务。
int check_steam_human_skin(const std::filesystem::path &source_root) {
    Checks check;power_checks(check);medal_bubble_checks(check);resources(check,source_root);owner_checks(check);
    return check.count;
}
