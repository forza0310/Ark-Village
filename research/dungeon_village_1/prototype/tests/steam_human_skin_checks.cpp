#include "dungeon_village_prototype/steam_human_skin.hpp"
#include "dungeon_village_prototype/startup.hpp"
#include "dungeon_village_prototype/startup_world_human.hpp"
#include "dungeon_village_prototype/startup_world_building.hpp"
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
template<class T> std::vector<T> parts(const SteamHumanOverviewPlan &plan) {
    std::vector<T> result;
    for(const auto &op:plan.operations)if(const auto *value=std::get_if<T>(&op))result.push_back(*value);
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
    for(const auto asset:{SteamHumanAsset::medal,SteamHumanAsset::pinch,SteamHumanAsset::bubble,SteamHumanAsset::number05,
        SteamHumanAsset::portrait_background,SteamHumanAsset::home_background,SteamHumanAsset::experience_bar,
        SteamHumanAsset::experience_label,SteamHumanAsset::experience_remaining,SteamHumanAsset::smile,
        SteamHumanAsset::combat_icon,SteamHumanAsset::equipment_background,SteamHumanAsset::weapon_icon,
        SteamHumanAsset::armour_icon,SteamHumanAsset::accessory_icon,SteamHumanAsset::magic_icon,SteamHumanAsset::catalogue_new,
        SteamHumanAsset::catalogue_get,SteamHumanAsset::gift_tab,
        SteamHumanAsset::ceremony_background,SteamHumanAsset::curtain,SteamHumanAsset::continue_cursor,SteamHumanAsset::result_icon,
        SteamHumanAsset::president,SteamHumanAsset::gift_box,SteamHumanAsset::gift_face,SteamHumanAsset::item_icon,
        SteamHumanAsset::gift_flash,SteamHumanAsset::recovery_effect,SteamHumanAsset::recovery_number}) {
        const auto resource=steam_human_resource(asset);
        check(resource.has_value(),"具名资源身份存在");
        Image image=LoadImage((assets/resource->published_image).string().c_str());
        check(image.data!=nullptr,"正式PNG可解码");
        if(resource->published_sprite) {
            const auto seb=dungeon_village_tools::parse_legacy_seb(bytes(assets/resource->published_sprite));
            for(const auto &layer:seb.layers)for(const auto &p:layer.parts) {
                if(p.width<=0 || p.height<=0)continue;
                if(asset==SteamHumanAsset::president&&(p.frame==10||p.frame==11)) {
                    // 原PNG宽75，未消费帧10/11指向x76；不改原资源或认证未消费帧。
                    check(p.image_index==resource->image&&p.source_x==76&&p.width==13&&image.width==75&&
                          p.source_x+p.width>image.width,"原总统SEB10/11越界事实保留，66只消费1/9，不将异常改成通过");
                    continue;
                }
                check(p.image_index==resource->image && p.source_x>=0 && p.source_y>=0 &&
                      p.source_x+p.width<=image.width && p.source_y+p.height<=image.height,
                      "实际SEB图片身份与裁片在正式PNG内");
                if(asset==SteamHumanAsset::number05 && p.frame==16)
                    check(p.source_x==61 && p.source_y==10 && p.width==9 && p.height==10,
                          "乘号原裁片61/10/9/10");
            }
        } else if(asset==SteamHumanAsset::medal)check(image.width>=28 && image.height>=14,"奖章index1裁片实际存在");
        UnloadImage(image);
    }
    const auto number=steam_human_resource(SteamHumanAsset::number05);
    check(bytes(assets/number->published_image)!=bytes(source_root/"common/number05.png"),
          "Steam数字PNG不静默替换为APK同名图");
    check(!steam_human_resource(static_cast<SteamHumanAsset>(99)),"未知资源枚举拒绝");
}
void overview_checks(Checks &check) {
    // 概览边界夹具；不认证自然等级10、住宅或恢复道具取得。
    auto owner=test_support::page_fixture(60);
    const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.shop_humans.at(1).equipment={0,{},{},{}};
    for(auto &stock:owner.items)stock.second.inventory=0;
    SteamHumanOverviewOptions options;
    options.title_widths=std::array<int,2>{80,80};options.profession_width=52;
    options.bottom_widths={48,96};options.down_text_width=36;
    auto plan=steam_human_overview_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{0,2}&&!plan->recovery_gift,"概览source0左空右返回且无W不补回复资格");
    const auto texts=parts<SteamHumanOverviewText>(*plan);
    check(std::all_of(texts.begin(),texts.end(),[](const auto &t){return t.rgb==std::array<int,3>{92,51,31};}),
          "概览正文读取固定SC_WINDOW_BLACK92/51/31，不借其它窗口棕色");
    const auto job=std::find_if(texts.begin(),texts.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession;});
    check(job!=texts.end()&&job->font_size==9&&job->rectangle[1]==88,"职业首次测宽52选择原9号字体");
    options.profession_width=53;
    check(!steam_human_overview_skin(owner,id,options),"首次53时必须提供原改字号前第二次测宽");
    options.profession_repeat_width=52;plan=steam_human_overview_skin(owner,id,options);
    auto labels=parts<SteamHumanOverviewText>(*plan);
    check(std::find_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession;})->font_size==8,
          "第二次52用8号，不自行按缩小字体重新试排");
    options.profession_repeat_width=53;plan=steam_human_overview_skin(owner,id,options);
    labels=parts<SteamHumanOverviewText>(*plan);
    check(std::find_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession;})->font_size==7,
          "两次53进入7号分支");
    const auto bars=parts<SteamHumanOverviewScroll>(*plan);
    check(bars.size()==1&&bars[0].rectangles[1].rect==std::array<int,4>{220,69,5,32}&&
        bars[0].thumb_touch==std::array<int,4>{221,69,5,31},"概览仍保留四页比例与独立注册矩形");
    auto &growth=owner.scene.world.world.ai.growth.at(1);
    const int profession=growth.definition.current_profession;
    check(startup_world_human_details(owner,1)->threshold==12,"夹具原初级经验门槛12");
    growth.experience=1;plan=steam_human_overview_skin(owner,id,options);
    const auto partial=parts<SteamHumanImage>(*plan);
    check(std::any_of(partial.begin(),partial.end(),[](const auto &i){return i.asset==SteamHumanAsset::experience_bar&&
        i.crop==std::optional<std::array<int,4>>({2,5,6,3});}),"经验1/12截断为6而非舍入7");
    const auto remaining=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(remaining.begin(),remaining.end(),[](const auto &n){return n.position==std::array<int,2>{201,110}&&n.value==11;}),
          "尚需经验读取12减1，不显示当前经验或总经验");
    growth.definition.profession_levels[profession]=10;
    growth.experience=0;plan=steam_human_overview_skin(owner,id,options);
    const auto images=parts<SteamHumanImage>(*plan);
    check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::experience_bar&&
        i.crop==std::optional<std::array<int,4>>({2,5,79,3});}),"等级恰10经验条强制79");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    check(numbers.size()==6&&std::none_of(numbers.begin(),numbers.end(),[](const auto &n){return n.position==std::array<int,2>{201,110};}),
          "大师仅满意/努力和四战斗缓存六数，不绘等级/剩余经验数");
    const auto recovery=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.recovery>0;});
    check(recovery!=owner.rules->items.end(),"源定义确有回复道具");
    owner.items.at(recovery->identity).inventory=1;
    ref::BattleActorRecord actor;actor.id={900};actor.kind=ref::ActorKind::human;actor.definition=1;
    actor.control.state=0;actor.hp.displayed=80;actor.hp.target=50;actor.capacity=999;
    owner.scene.world.world.ai.battle.actors.emplace(actor.id,actor);
    owner.scene.world.world.ai.human_order={actor.id};growth.derived.combat[0]=100;
    const auto combat=parts<SteamFacilityNumber>(*steam_human_overview_skin(owner,id,options));
    check(std::any_of(combat.begin(),combat.end(),[](const auto &n){return n.position==std::array<int,2>{203,128}&&n.value==100;}),
          "概览HP读取共享最大缓存100，不用实例显示80/目标50/旧capacity999");
    check(!steam_human_overview_skin(owner,id,options)->recovery_gift,"恰一半目标HP不切回复，不用显示HP或旧capacity");
    owner.scene.world.world.ai.battle.actors.at(actor.id).hp.target=49;
    check(steam_human_overview_skin(owner,id,options)->recovery_gift,"有库存且目标HP低于一半才切回复");
    owner.items.at(recovery->identity).inventory=0;
    owner.scene.world.world.ai.battle.actors.at(actor.id).control.state=7;
    check(!steam_human_overview_skin(owner,id,options)->recovery_gift,"倒下仍要求持有回复道具");
    owner.items.at(recovery->identity).inventory=1;
    check(steam_human_overview_skin(owner,id,options)->recovery_gift,"倒下且有道具进入回复提示");
    const auto before=startup_world_state_digest(owner);
    const auto size=steam_human_overview_skin(owner,id,options)->operations.size();
    for(int repeat=0;repeat<24;++repeat) {
        const auto same=steam_human_overview_skin(owner,id,options);
        check(same&&same->operations.size()==size&&startup_world_state_digest(owner)==before,"重复只读概览输出规模稳定，不改Owner/HP/随机/声音");
    }
    auto bad=owner;bad.items.erase(recovery->identity);
    check(!steam_human_overview_skin(bad,id,options),"缺必需库存依赖显式拒绝");
    bad=owner;bad.shop_humans.at(1).equipment[1]=std::numeric_limits<int>::max();
    check(!steam_human_overview_skin(bad,id,options),"错误当前防具引用不作为空槽容错");
    bad=owner;bad.page_phases[id]=1;
    check(!steam_human_overview_skin(bad,id,options),"页1不冒认本批概览皮肤");
    bad=owner;bad.scripts.pages.back().lifecycle=4;
    check(!steam_human_overview_skin(bad,id,options),"退休概览不能继续出图");
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
void attributes_checks(Checks &check) {
    // 缓存字段／无W为表现条件，不认证自然培养到这些属性。
    auto owner=test_support::page_fixture(60);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=1;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    auto &growth=owner.scene.world.world.ai.growth.at(1);
    growth.derived.attributes={11,22,33,44,55,66};growth.derived.combat={101,202,303,404};
    growth.definition.extra={7,7,7,7,7,7}; // 与缓存故意不同，绘制不能重算或再加一遍。
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{80,80};
    options.profession_width=52;options.profession_change_width=24;
    const auto plan=steam_human_attributes_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{0,2}&&!plan->recovery_gift,"六属性保留source0空左软标签，不改成转职");
    constexpr std::array<std::array<int,3>,6> expected{{{29,134,0},{29,153,16},{29,172,32},
        {121,134,48},{121,153,64},{121,172,96}}};
    int row{};
    for(std::size_t n=0;n<plan->operations.size();++n) {
        const auto *t=std::get_if<SteamHumanOverviewText>(&plan->operations[n]);
        if(!t||t->role!=SteamHumanOverviewTextRole::attribute)continue;
        check(row<6&&n>=3&&n+1<plan->operations.size(),"每属性均有完整五图元层序");
        const auto &e=expected[row];
        const auto *fill=std::get_if<SteamStartupFrameDraw>(&plan->operations[n-3]);
        const auto *line=std::get_if<SteamStartupFrameDraw>(&plan->operations[n-2]);
        const auto *image=std::get_if<SteamHumanImage>(&plan->operations[n-1]);
        const auto *value=std::get_if<SteamFacilityNumber>(&plan->operations[n+1]);
        check(fill&&line&&std::get<StartupSkinRect>(*fill).rect==std::array<int,4>{e[0]-4,e[1]-2,93,20}&&
            !std::get<StartupSkinRect>(*fill).outline&&std::get<StartupSkinRect>(*fill).rgb==std::array<int,3>{208,242,249}&&
            std::get<StartupSkinRect>(*line).rect==std::get<StartupSkinRect>(*fill).rect&&
            std::get<StartupSkinRect>(*line).outline&&std::get<StartupSkinRect>(*line).rgb==std::array<int,3>{149,206,217},
            "先原FillRect再DrawRect半开93×20，不双加1");
        check(image&&image->asset==SteamHumanAsset::combat_icon&&image->position==std::array<int,2>{e[0],e[1]}&&
            image->crop==std::optional<std::array<int,4>>({e[2],16,16,16}),"mode7直接裁图，运气原x96，不套SEB98");
        check(t->argument==row&&t->rectangle==std::array<int,4>{e[0]+18,e[1]+2,0,0}&&t->font_size==0&&t->rgb==std::array<int,3>{0,100,255}&&
            value&&value->value==11*(row+1)&&value->position==std::array<int,2>{e[0]+83,e[1]+3}&&
            value->asset==SteamFacilityAsset::number08,"六字段原序读共享缓存，默认字号和SEB15右锚");
        ++row;
    }
    check(row==6,"两列各三属性，不遗漏或追加战斗值");
    const auto bars=parts<SteamHumanOverviewScroll>(*plan);
    check(bars.size()==1&&bars[0].rectangles[1].rect==std::array<int,4>{220,100,5,32}&&
        bars[0].thumb_touch==std::array<int,4>{221,100,5,31},"页1保持四页蓝条比例与原有符号截断位置");
    const auto texts=parts<SteamHumanOverviewText>(*plan);
    check(std::count_if(texts.begin(),texts.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::profession_change;})==1&&
        std::none_of(texts.begin(),texts.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::gift;}),"页1唯一底部转职，不绘概览赠礼");
    const auto before=startup_world_state_digest(owner);
    for(int repeat=0;repeat<24;++repeat)
        check(steam_human_attributes_skin(owner,id,options)->operations.size()==plan->operations.size()&&
            startup_world_state_digest(owner)==before,"重复六属性输出规模稳定，不重算缓存/随机/输出");
    auto missing=options;missing.profession_change_width.reset();
    check(!steam_human_attributes_skin(owner,id,missing),"缺底部测宽显式拒绝");
    auto local=owner;local.items.clear();
    check(steam_human_attributes_skin(local,id,options).has_value(),"页1不查不消费页0回复库存");
    local=owner;local.page_phases[id]=2;
    check(!steam_human_attributes_skin(local,id,options),"页2仍显式拒绝，不冒认完整四页");
    local=owner;local.scripts.pages.back().lifecycle=4;
    check(!steam_human_attributes_skin(local,id,options),"退休六属性页不继续出图");
}
void equipment_magic_checks(Checks &check) {
    auto owner=test_support::page_fixture(60);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=2;owner.page_counters[id]=0;
    owner.human_page_selections[id]=2;owner.human_pages_initialized.insert(id);
    owner.shop_humans.at(1).equipment={0,{},{},{}};
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{80,80};options.profession_width=52;
    options.equipment_hint_width=96;options.magic_hint_width=120;
    auto plan=steam_human_information_skin(owner,id,options);
    check(plan.has_value(),"四装备读取已有选择2，不退回默认武器");
    const auto rows=parts<SteamHumanEquipmentRow>(*plan);
    check(rows.size()==4,"四行均保留Touch10原请求");
    for(int row=0;row<4;++row)
        check(rows[row].touch.component==10&&rows[row].touch.value==(0x20000|row)&&
              rows[row].touch.rectangle==std::optional<std::array<int,4>>({5,128+17*row,225,18})&&
              rows[row].touch.option==0&&rows[row].margin==std::array<int,4>{0,-20,0,0},"普通装备行无marker且保持负margin");
    auto labels=parts<SteamHumanOverviewText>(*plan);
    check(std::count_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::equipment_empty&&!t.layout;})==3,
          "三空槽普通DrawString，不套有装备名的TextLayout");
    check(parts<SteamStartupTouch>(*plan).empty(),"空装备底文false/false不注册确认条");
    const auto blue=parts<SteamHumanOverviewScroll>(*plan);
    check(blue[0].rectangles[1].rect==std::array<int,4>{220,132,5,32},"装备页四页比例蓝块top132");
    owner.human_page_selections[id]=0;plan=steam_human_information_skin(owner,id,options);
    const auto defs=std::find_if(owner.rules->equipment.begin(),owner.rules->equipment.end(),[](const auto &d){return d.shop.kind==1&&d.shop.id==0;});
    check(defs!=owner.rules->equipment.end()&&defs->shop.combat[1]>0,"原短剑确有攻击加值");
    const auto values=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(values.begin(),values.end(),[&](const auto &n){return n.kind==SteamFacilityNumberKind::plus_value&&
          n.asset==SteamFacilityAsset::number05&&n.value==defs->shop.combat[1]&&n.position==std::array<int,2>{108,205};}),
          "所选武器底文用装备定义加值/SEB12，不用人物战斗缓存");
    owner.page_phases[id]=3;owner.scene.world.world.ai.growth.at(1).derived.available_spells={false,false,false,false};
    plan=steam_human_information_skin(owner,id,options);
    labels=parts<SteamHumanOverviewText>(*plan);
    check(std::count_if(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::magic_empty&&t.anchor==34&&t.rgb==std::array<int,3>{0,100,255};})==4&&
          parts<SteamStartupTouch>(*plan).empty(),"未学四格蓝色---且不创建赠礼/转职触摸");
    check(parts<SteamHumanOverviewScroll>(*plan)[0].rectangles[1].rect==std::array<int,4>{220,164,5,32},"魔法页蓝块top164");
    for(int mask=1;mask<16;++mask) {
        auto &spells=owner.scene.world.world.ai.growth.at(1).derived.available_spells;
        int count{};for(int i=0;i<4;++i){spells[i]=(mask&(1<<i))!=0;if(spells[i])++count;}
        plan=steam_human_information_skin(owner,id,options);const auto images=parts<SteamHumanImage>(*plan);
        int found{};for(const auto &img:images)if(img.asset==SteamHumanAsset::magic_icon) {
            check(img.crop&&(*img.crop)[1]==0&&(*img.crop)[2]==16&&(*img.crop)[3]==16&&(*img.crop)[0]!=48,
                  "回复魔法使用index4而非第四列index3");++found;
        }
        labels=parts<SteamHumanOverviewText>(*plan);
        check(found==count&&std::any_of(labels.begin(),labels.end(),[=](const auto &t){return t.role==SteamHumanOverviewTextRole::magic_status&&
              t.argument==count&&t.rectangle==std::array<int,4>{120,204,0,0};}),"魔法所有组合按四可用位及计数正文，不消费永久学习位代替");
    }
    const auto before=startup_world_state_digest(owner);
    const auto size=plan->operations.size();
    for(int i=0;i<24;++i)check(steam_human_information_skin(owner,id,options)->operations.size()==size&&
        startup_world_state_digest(owner)==before,"完整四页计划无Owner/随机/声音写入且输出稳定");
    owner.human_page_selections[id]=4;
    check(!steam_human_information_skin(owner,id,options),"选择4严格拒绝，不映射到某个合法槽");
}
void equipment_detail_checks(Checks &check) {
    // 展示目录条件，未认证真实玩家已经开放第二武器。
    auto owner=test_support::page_fixture(73);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.equipment_page_catalogs[id][0]={0,1};owner.human_equipment_choices[id]={0,0};
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{90,90};
    const auto plan=steam_human_equipment_detail_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{2,2}&&plan->title=="装备信息 1/2","原73双返回及真实目录序号");
    const auto arrows=parts<SteamFacilityArrowTouch>(*plan);
    check(arrows.size()==2&&arrows[0].position==std::array<int,2>{51,77}&&arrows[0].frame==3&&arrows[0].value==16&&
          arrows[1].position==std::array<int,2>{189,77}&&arrows[1].frame==0&&arrows[1].value==18,
          "TitleArrow170/125原y77，注册SEB请求后委托真实按钮纹理");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(numbers.begin(),numbers.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::money&&
          n.asset==SteamFacilityAsset::number08&&n.position==std::array<int,2>{189,92}&&n.value==400;}),
          "短剑原基础价400而非赠礼600或render_image50");
    const auto text=parts<SteamHumanOverviewText>(*plan);
    check(std::any_of(text.begin(),text.end(),[](const auto &t){return t.value=="--"&&t.rectangle==std::array<int,4>{173,119,0,0}&&
          t.anchor==34&&t.rgb==std::array<int,3>{92,51,31};}),"非正HP原--173/119居中，不猜---184/115");
    const auto before=startup_world_state_digest(owner);
    for(int i=0;i<12;++i)check(steam_human_equipment_detail_skin(owner,id,options)->operations.size()==plan->operations.size()&&
        startup_world_state_digest(owner)==before,"73读取不收费/装配/清NEW/抽随机，输出稳定");
    owner.human_page_selections[id]=1;
    const auto second=steam_human_equipment_detail_skin(owner,id,options);
    const auto names=parts<SteamHumanOverviewText>(*second);
    const auto expected=std::find_if(owner.rules->equipment.begin(),owner.rules->equipment.end(),[](const auto &d){return d.shop.kind==1&&d.shop.id==1;});
    check(second->title=="装备信息 2/2"&&std::any_of(names.begin(),names.end(),[&](const auto &t){return t.role==SteamHumanOverviewTextRole::equipment_name&&t.value==expected->name;}),
          "73切目录读取当前选择1，不固定初始choice0");
    owner.equipment_page_catalogs[id][0]={0};owner.human_page_selections[id]=0;
    check(parts<SteamFacilityArrowTouch>(*steam_human_equipment_detail_skin(owner,id,options)).empty(),"原单元素目录没有TitleArrow");
    owner.human_page_selections[id]=1;
    check(!steam_human_equipment_detail_skin(owner,id,options),"73越界目录选择显式拒绝");
    owner.human_page_selections[id]=0;owner.human_equipment_choices.erase(id);
    check(!steam_human_equipment_detail_skin(owner,id,options),"73缺初始化来源拒绝而非容错补默认");
}
void gift_catalogue_checks(Checks &check) {
    // 本层只造目录/库存/价格的定向条件；不认证玩家自然已经解锁六种武器。
    auto owner=test_support::page_fixture(64);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_pages_initialized.insert(id);owner.human_page_selections[id]=1;owner.human_page_scroll[id]=1;
    owner.equipment_page_catalogs[id][0]={0,1,2,3,4,5};owner.human_equipment_choices[id]={0,1};
    owner.shop_humans.at(1).equipment[0]=0;
    for(int definition:owner.equipment_page_catalogs[id][0]) {
        auto &stock=owner.catalog.at({1,definition});stock.status=1;stock.free_purchases=0;stock.newly_unlocked=false;
    }
    owner.catalog.at({1,1}).free_purchases=2;owner.catalog.at({1,1}).newly_unlocked=true;
    SteamHumanOverviewOptions o;o.title_widths=std::array<int,2>{96,96};
    auto plan=steam_human_gift_catalogue_skin(owner,id,o);
    check(plan&&plan->soft_labels==std::array<int,2>{7,2}&&plan->title=="赠送礼物 1/5","64装备目录五页序号及真实软标签");
    const auto rows=parts<SteamHumanEquipmentRow>(*plan);
    const auto scroll=parts<SteamHumanCatalogueScroll>(*plan);
    check(rows.size()==4&&rows[0].touch.component==11&&rows[0].touch.value==(0x20000|1)&&
          rows[0].touch.rectangle==std::optional<std::array<int,4>>{{3,95,231,16}}&&rows[0].margin==std::array<int,4>{20,-20,0,0},
          "64四行从Owner first1开始，Touch11与装备详情Touch10不混");
    check(scroll.size()==1&&scroll[0].rectangles[1].rect==std::array<int,4>{220,103,5,74}&&
          scroll[0].arguments==std::array<int,3>{6,4,0x20000}&&scroll[0].touch.component==12&&scroll[0].track_touch.component==25,
          "64滚动真实first1/count6/visible4，组件12/25携带原附加参数");
    const auto values=parts<SteamFacilityNumber>(*plan);
    const auto images=parts<SteamHumanImage>(*plan);
    check(std::any_of(values.begin(),values.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::count&&
          n.asset==SteamFacilityAsset::number05&&n.value==2&&n.position==std::array<int,2>{210,98};}),
          "免费两份沿原count数量单位，不显示价格-1或物品乘号");
    check(std::any_of(values.begin(),values.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::plus_value&&
          n.value==3&&n.asset==SteamFacilityAsset::number05&&n.position==std::array<int,2>{106,200};})&&
          std::any_of(values.begin(),values.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::plus_value&&
          n.value==0&&n.asset==SteamFacilityAsset::number08&&n.position==std::array<int,2>{197,200};}),
          "铁剑8减短剑5显示+3，同精度显示+0灰而不省略");
    check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::catalogue_get&&
          i.position==std::array<int,2>{84,70};})&&std::any_of(images.begin(),images.end(),[](const auto &i){return
          i.asset==SteamHumanAsset::catalogue_get&&i.position==std::array<int,2>{10,99};}),
          "非日文页签GET消费Steam宽度57，行GET独立读取共享提示");
    auto jp=o;jp.japanese=true;
    const auto jp_images=parts<SteamHumanImage>(*steam_human_gift_catalogue_skin(owner,id,jp));
    check(std::any_of(jp_images.begin(),jp_images.end(),[](const auto &i){return i.asset==SteamHumanAsset::catalogue_get&&
          i.position==std::array<int,2>{73,70};}),"日文页签GET消费独立宽度46");
    o.catalogue_arrow_count=1;plan=steam_human_gift_catalogue_skin(owner,id,o);
    const auto arrow_rows=parts<SteamHumanEquipmentRow>(*plan);
    check(arrow_rows.size()==6&&arrow_rows[4].touch.component==23&&arrow_rows[4].touch.value==17&&
          arrow_rows[4].touch.image_draw.has_value()&&arrow_rows[4].margin==std::array<int,4>{40,40,45,0}&&
          arrow_rows[5].touch.value==19&&arrow_rows[5].margin==std::array<int,4>{40,40,-15,60},
          "udArrowCnt非0是Touch23图片箭头及不同Margin，不猜矩形向上向下");
    o.catalogue_arrow_count=0;owner.equipment_page_catalogs[id][0]={0};
    owner.human_page_selections[id]=owner.human_page_scroll[id]=0;owner.catalog.at({1,0}).free_purchases=0;
    owner.scene.world.world.ai.accounting=ref::PeriodAccounting(0);
    plan=steam_human_gift_catalogue_skin(owner,id,o);
    auto prices=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(prices.begin(),prices.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::money&&
          n.value==600&&n.asset==SteamFacilityAsset::number12;}),"短剑基础400赠礼600，现金不足红色金额");
    auto labels=parts<SteamHumanOverviewText>(*plan);
    check(std::any_of(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::gift_current_equipment;})&&
          std::none_of(prices.begin(),prices.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::plus_value;}),
          "同装备只显示当前装备中，不伪造属性差额");
    owner.scene.world.world.ai.accounting=ref::PeriodAccounting(600);owner.shop_humans.at(1).equipment[0]=1;
    plan=steam_human_gift_catalogue_skin(owner,id,o);prices=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(prices.begin(),prices.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::money&&
          n.value==600&&n.asset==SteamFacilityAsset::number08;})&&std::any_of(prices.begin(),prices.end(),[](const auto &n){
          return n.kind==SteamFacilityNumberKind::minus_value&&n.value==3&&n.asset==SteamFacilityAsset::number12;}),
          "现金恰600可支付，短剑降级差额-3用minus而不画负金额");
    const auto item=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &d){return d.effect==7;});
    check(item!=owner.rules->items.end(),"固定表有素材普通道具");
    owner.page_phases[id]=4;owner.equipment_page_catalogs[id][4]={item->identity};
    owner.items.at(item->identity).inventory=3;owner.items.at(item->identity).newly_unlocked=true;
    plan=steam_human_gift_catalogue_skin(owner,id,o);check(plan&&plan->soft_labels==std::array<int,2>{0,2},"64普通道具无装备详情软按钮");
    const auto item_rows=parts<SteamHumanEquipmentRow>(*plan);prices=parts<SteamFacilityNumber>(*plan);
    check(item_rows[0].touch.rectangle==std::optional<std::array<int,4>>{{23,95,191,16}}&&
          item_rows[0].touch.option==16&&item_rows[0].margin==std::array<int,4>{20,0,0,0}&&
          std::any_of(prices.begin(),prices.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::multi_value&&n.value==3;}),
          "普通道具Touch11 flag16与独立Margin，库存用multi乘号，不用免费count");
    const auto before=startup_world_state_digest(owner);
    const auto size=plan->operations.size();
    for(int i=0;i<12;++i)check(steam_human_gift_catalogue_skin(owner,id,o)->operations.size()==size&&
        startup_world_state_digest(owner)==before,"64重复投影资源规模稳定，不消费NEW/库存/资金/随机/输出");
    auto bad=owner;bad.human_page_scroll.erase(id);check(!steam_human_gift_catalogue_skin(bad,id,o),"64缺独立scroll显式拒绝");
    bad=owner;bad.items.at(item->identity).inventory=1000;
    check(!steam_human_gift_catalogue_skin(bad,id,o),"共享库存维护合法域1..999，原Draw自身不校验库存上限");
    bad=owner;bad.equipment_page_catalogs[id][4].push_back(9999);
    check(!steam_human_gift_catalogue_skin(bad,id,o),"屏外坏目录定义显式拒绝");
    owner.equipment_page_catalogs[id][4].clear();plan=steam_human_gift_catalogue_skin(owner,id,o);
    check(plan&&parts<SteamHumanEquipmentRow>(*plan).empty()&&parts<SteamFacilityNumber>(*plan).empty()&&
          parts<SteamHumanCatalogueScroll>(*plan)[0].arguments==std::array<int,3>{0,4,0x20000},"空目录保留窗口/滚动但不读选择或展示道具效果");
    labels=parts<SteamHumanOverviewText>(*plan);
    check(std::any_of(labels.begin(),labels.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::gift_catalogue_empty&&
          t.rectangle==std::array<int,4>{120,97,0,0}&&t.anchor==2;}),"原空目录真实文字层120/97居中");
    owner.scripts.pages.back().lifecycle=4;check(!steam_human_gift_catalogue_skin(owner,id,o),"退休64不再输出目录/道具引用");
    StartupWorldItem explanation;SteamHumanOverviewOptions widths;
    widths.item_explain_widths={20,30,70};
    for(int effect=0;effect<=9;++effect) {
        explanation.effect=effect;explanation.category=effect==6?11:0;explanation.spell=3;
        const auto explain=steam_human_item_explain_skin(explanation,widths);check(explain.has_value(),"普通道具10种效果说明完整分支");
        check(parts<SteamFacilityNumber>(*explain).empty(),"道具底说明不读取增长/回复数值或价格");
        if(effect<6) {
            const auto icons=parts<SteamFacilityImage>(*explain);
            const auto text=parts<SteamHumanOverviewText>(*explain);
            check(icons[0].position==std::array<int,2>{86,197}&&icons[0].crop==
                std::optional<std::array<int,4>>{{16*(effect==5?6:effect),16,16,16}}&&
                text[0].rgb==std::array<int,3>{0,100,255}&&text[1].anchor==4,
                "属性说明真实测宽+18居中，运图跳第5列，名字蓝suffix棕");
        } else if(effect==9) {
            const auto magic=parts<SteamHumanImage>(*explain);
            const auto rich=parts<SteamHumanLearningText>(*explain);
            check(magic[0].position==std::array<int,2>{76,197}&&magic[0].crop==std::optional<std::array<int,4>>{{64,0,16,16}}&&
                  rich.size()==1&&rich[0].spell==3&&rich[0].name_rgb==std::array<int,3>{0,101,255}&&
                  rich[0].suffix_rgb==std::array<int,3>{92,51,31}&&rich[0].position==std::array<int,2>{164,199},
                  "回复魔法图索引4，学习富文本只名称0065FF，不误套属性蓝100");
        }
    }
    explanation.effect=6;explanation.category=0;
    const auto trade=parts<SteamHumanOverviewText>(*steam_human_item_explain_skin(explanation,{}));
    check(trade[0].value=="在商店高价交易"&&!trade[0].layout&&trade[0].anchor==2,"effect6非category11走商店说明不画双培养文案");
    explanation.effect=5;check(!steam_human_item_explain_skin(explanation,{}),"属性说明缺实际测宽拒绝，不用字符数估宽");
    widths.english=true;
    check(parts<SteamFacilityImage>(*steam_human_item_explain_skin(explanation,widths))[0].position==std::array<int,2>{85,197},"英语属性总宽额外2像素");
    explanation.effect=9;explanation.spell=4;check(!steam_human_item_explain_skin(explanation,widths),"坏魔法槽拒绝");
    explanation.effect=10;check(!steam_human_item_explain_skin(explanation,widths),"坏effect拒绝");
}
void gift_confirmation_checks(Checks &check) {
    auto owner=test_support::page_fixture(64);const auto parent=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=3;owner.scripts.executing_page.reset();
    owner.page_human_bindings[parent]=1;owner.page_phases[parent]=0;owner.page_counters[parent]=0;
    owner.human_page_selections[parent]=0;owner.human_pages_initialized.insert(parent);
    owner.equipment_page_catalogs[parent][0]={1};owner.human_equipment_choices[parent]={0,1};
    ref::WorldScriptPage child;child.id=owner.scripts.next_page_id++;child.kind=ref::WorldScriptPageKind::raw_page;
    child.legacy_page=65;child.lifecycle=1;owner.scripts.pages.push_back(child);const auto id=child.id;
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=0;owner.human_pages_initialized.insert(id);
    owner.human_equipment_choices[id]={0,1};owner.human_page_parents[id]=parent;
    owner.shop_humans.at(1).equipment[0]=0;owner.scene.world.world.ai.growth.at(1).derived.combat={100,999,888,777};
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{48,48};
    options.comparison_name_widths={20,20};options.gift_accept_width=48;
    auto plan=steam_human_gift_confirmation_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{0,2}&&plan->title=="赠送礼物","65固定赠礼标题，左空右返回");
    owner.scripts.pages[owner.scripts.pages.size()-2].lifecycle=2;
    check(steam_human_gift_confirmation_skin(owner,id,options).has_value(),"真实通用open的父64仍life2可绘制，不伪定必须life3");
    const auto numbers=parts<SteamFacilityNumber>(*plan);
    check(numbers.size()==4&&numbers[0].value==5&&numbers[0].asset==SteamFacilityAsset::number08&&
          numbers[1].value==8&&numbers[1].asset==SteamFacilityAsset::number05&&numbers[2].value==0&&
          numbers[3].value==0&&numbers[3].asset==SteamFacilityAsset::number08,"短剑5与铁剑8为装备定义参数，不用人物缓存999；升值12同值15");
    check(std::none_of(numbers.begin(),numbers.end(),[](const auto &n){return n.kind!=SteamFacilityNumberKind::number;}),
          "确认65没有价格/库存/加号，不提前报价或兑现");
    const auto images=parts<SteamFacilityImage>(*plan);
    check(std::count_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamFacilityAsset::title_arrows;})==3&&
          images[0].frame==3&&images[1].frame==1&&images[2].frame==1,"counter0三箭头首项frame3，其余1");
    auto width=options;width.comparison_name_widths[0]=std::numeric_limits<int>::max();
    check(steam_human_gift_confirmation_skin(owner,id,width).has_value(),"名称测宽先Clamp90，不能int+20溢出");
    owner.shop_humans.at(1).equipment[0]=1;owner.human_equipment_choices[parent]=owner.human_equipment_choices[id]={0,0};
    owner.equipment_page_catalogs[parent][0]={0};owner.page_counters[id]=16;
    plan=steam_human_gift_confirmation_skin(owner,id,options);const auto decreased=parts<SteamFacilityNumber>(*plan);
    check(decreased[1].value==5&&decreased[1].asset==SteamFacilityAsset::number12,"下降用原SEB19而非普通/红字体数字");
    const auto before=startup_world_state_digest(owner);const auto size=plan->operations.size();
    for(int i=0;i<12;++i)check(steam_human_gift_confirmation_skin(owner,id,options)->operations.size()==size&&
        startup_world_state_digest(owner)==before,"65重复绘制不装配/扣款/答案/声音/随机变更");
    auto bad=owner;bad.page_human_bindings.erase(parent);
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65父绑定缺失显式拒绝");
    bad=owner;bad.equipment_page_catalogs.erase(parent);
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65父目录缺载荷不能从候选装备补默认");
    bad=owner;bad.scripts.pages[bad.scripts.pages.size()-2].lifecycle=4;
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65退休父64不能冒认有效候选来源");
    bad=owner;bad.page_phases.erase(id);
    check(!steam_human_gift_confirmation_skin(bad,id,options),"已初始化65缺phase不能靠绘制补键");
    bad=owner;bad.human_equipment_choices.at(parent)[1]=1;
    check(!steam_human_gift_confirmation_skin(bad,id,options),"65错误父候选拒绝");
    auto missing=options;missing.comparison_name_widths[0].reset();
    check(!steam_human_gift_confirmation_skin(owner,id,missing),"缺实际字号10测宽拒绝");
}
StartupWorldRuntimeState profession_fixture(int raw) {
    auto owner=test_support::page_fixture(61);const auto parent=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=2;owner.scripts.executing_page.reset();
    owner.page_human_bindings[parent]=1;owner.page_phases[parent]=0;owner.page_counters[parent]=0;
    owner.human_page_selections[parent]=0;owner.human_pages_initialized.insert(parent);owner.human_page_catalogs[parent]={0,1};
    ref::WorldScriptPage page;page.id=owner.scripts.next_page_id++;page.kind=ref::WorldScriptPageKind::raw_page;
    page.legacy_page=raw;page.lifecycle=1;owner.scripts.pages.push_back(page);
    owner.page_human_bindings[page.id]=1;owner.page_phases[page.id]=0;owner.page_counters[page.id]=0;
    owner.human_page_selections[page.id]=1;owner.human_pages_initialized.insert(page.id);
    owner.human_page_catalogs[page.id]={0,1};owner.page_job_bindings[page.id]=1;owner.human_page_parents[page.id]=parent;
    owner.human_gift_messages[page.id]="谢谢";
    // 仅表现条件，不声称真实人物已转职、升级或领取奖励。
    owner.scene.world.world.ai.growth.at(1).definition.current_profession=0;
    owner.scene.world.world.ai.growth.at(1).definition.profession_levels[1]=10;
    owner.human_attribute_display={std::array<int,6>{100,100,100,100,100,100},
        std::array<int,6>{99,100,101,100,100,100},std::array<int,6>{-1,0,1,0,0,0}};
    owner.reward_display={std::array<int,2>{999,9},std::array<int,2>{999,10},std::array<int,2>{5,1}};
    owner.village_points=1000;
    return owner;
}
void shared_character_clock_checks(Checks &check) {
    const auto initial=test_support::world_fixture();
    check(steam_human_shared_character_frame(initial)==std::optional<int>{0},
          "新维护世界共享人物计数零初值，不借窗口帧或页面计数");
    auto owner=profession_fixture(66);const auto id=owner.scripts.pages.back().id;
    owner.human_equipment_choices[id]={0,1};owner.human_gift_scores[id]=24;
    SteamHumanOverviewOptions options;
    // 源action0按人物步速4分四幅；结果页局部counter不是该时钟。
    for(const auto sample:std::array<std::array<int,2>,8>{{
            {{0,0}},{{3,0}},{{4,1}},{{7,1}},{{8,2}},{{12,3}},{{15,3}},{{16,0}}}}) {
        owner.entry_updates=sample[0];
        options.character_frame=steam_human_shared_character_frame(owner);
        for(int counter:{0,77,197}) {
            const auto plan=steam_human_reward_skin(owner,1,1,counter,options);
            check(plan.has_value(),"既有Owner共享计数可以显式接入奖励纯计划");
            const auto portraits=parts<SteamHumanPortrait>(*plan);
            check(portraits.size()==1&&portraits[0].actor.body.frame==sample[1],
                  "共享计数决定身体幅，独立奖励counter变化不重置身体");
        }
    }
    owner.entry_updates=12;owner.page_counters[id]=0;
    const auto before=startup_world_state_digest(owner);
    for(int i=0;i<12;++i)
        check(steam_human_shared_character_frame(owner)==std::optional<int>{12}&&
              startup_world_state_digest(owner)==before,
              "重复共享计数查询不推进Owner／随机／页面或一次性输出");
    const auto updated=update_startup_world_human_page(owner,id);
    check(updated&&updated->page_counters.at(id)==1&&
          steam_human_shared_character_frame(*updated)==std::optional<int>{12},
          "实际66模态更新推进局部计数，保持UserData共享计数");
    owner=*updated;
    check(act_startup_world_human_page(owner,id,StartupHumanPageAction::confirm)==
              StartupWorldRuntimeError::none&&owner.page_counters.at(id)==40&&
          steam_human_shared_character_frame(owner)==std::optional<int>{12},
          "66早确认快进到40不推进或清零共享人物计数");
    owner.scene.framework_paused=true;
    const auto paused=update_startup_world_human_page(owner,id);
    check(paused&&paused->page_counters.at(id)==40&&
          steam_human_shared_character_frame(*paused)==std::optional<int>{12},
          "框架暂停保持局部及共享计数，不由查询补更新");
    owner.entry_updates=std::numeric_limits<int>::max()-1;
    check(steam_human_shared_character_frame(owner)==
              std::optional<int>{std::numeric_limits<int>::max()-1},
          "共享计数最大合法值可读取，绕回由既有世界入口负责");
    for(int invalid:{-1,std::numeric_limits<int>::max()}) {
        owner.entry_updates=invalid;
        check(!steam_human_shared_character_frame(owner),
              "负值及INT_MAX共享计数显式拒绝，不取模补合法值");
    }
}
void profession_skin_checks(Checks &check) {
    auto owner=profession_fixture(62);const auto id=owner.scripts.pages.back().id;
    SteamHumanOverviewOptions o;o.title_widths=std::array<int,2>{80,80};o.profession_change_width=48;
    o.insufficient_points_width=72;o.message_width=24;o.character_frame=4;
    const auto preview=steam_human_profession_preview_skin(owner,id,o);
    check(preview&&preview->soft_labels==std::array<int,2>{0,2},"62右返回，读取真实父61与目录");
    const auto values=parts<SteamFacilityNumber>(*preview);
    const auto negative=std::find_if(values.begin(),values.end(),[](const auto &v){return v.parameter==0;});
    check(negative!=values.end()&&negative->kind==SteamFacilityNumberKind::minus_value&&negative->asset==SteamFacilityAsset::number12&&
          negative->value==1&&negative->position==std::array<int,2>{112,137},"62只有at[2]差额，负差SEB19正幅值右112/137");
    check(std::count_if(values.begin(),values.end(),[](const auto &v){return v.kind==SteamFacilityNumberKind::plus_value&&v.value==0;})==4,
          "62零差也画+0，不额外画before/after数值");
    const auto minus=steam_facility_number_draws(*negative,8);
    check(minus&&minus->back().frame==15&&minus->back().position==std::array<int,2>{96,137},"原minus先数字再减号，符号右锚减8*(位数+1)");
    auto bad_number=*negative;bad_number.value=-1;
    check(!steam_facility_number_draws(bad_number,8),"minus不自动abs负输入");
    bad_number=*negative;bad_number.kind=SteamFacilityNumberKind::plus_value;
    check(!steam_facility_number_draws(bad_number,8),"SEB19仍拒绝未证plus消费者");
    const auto portraits=parts<SteamHumanPortrait>(*preview);
    check(portraits.size()==4&&portraits[0].clip==std::optional<std::array<int,4>>({38,68,15,14})&&portraits[0].image_clip&&
          portraits[0].position==std::array<int,2>{46,89}&&std::none_of(portraits.begin(),portraits.end(),[](const auto &p){return p.actor.weapon.has_value();}),
          "62 clip/ImageClip并存且四身体不带武器");
    const auto images=parts<SteamFacilityImage>(*preview);
    check(std::any_of(images.begin(),images.end(),[](const auto &p){return p.asset==SteamFacilityAsset::maximum&&p.position==std::array<int,2>{171,118};}),
          "目标职业读取已有等级10并绘MAX，不固定预览等级1");
    auto no_points=owner;no_points.village_points=-1;
    const auto denied=steam_human_profession_preview_skin(no_points,id,o);
    const auto denied_images=parts<SteamHumanImage>(*denied);
    const auto denied_touches=parts<SteamStartupTouch>(*denied);
    check(std::none_of(denied_touches.begin(),denied_touches.end(),[](const auto &t){return t.component==4;})&&
          std::any_of(denied_images.begin(),denied_images.end(),[](const auto &i){return i.asset==SteamHumanAsset::result_icon;}),
          "不足点显示原result icon，不伪造支付热区");
    auto bad=owner;bad.human_page_parents.erase(id);
    check(!steam_human_profession_preview_skin(bad,id,o),"62缺父来源拒绝");
    bad=owner;bad.human_attribute_display[2][0]=2;
    check(!steam_human_profession_preview_skin(bad,id,o),"冻结差额失配拒绝，不绘制重算");
    const auto before=startup_world_state_digest(owner);
    for(int i=0;i<12;++i)check(steam_human_profession_preview_skin(owner,id,o)->operations.size()==preview->operations.size()&&
        startup_world_state_digest(owner)==before,"62只读稳定，不扣点/勋章/NEW/随机");
    for(const auto sample:std::vector<std::array<int,3>>{{0,0,999},{1,9,999},{1,10,999},{1,47,999},{1,57,999},{1,67,999},{1,77,999}}) {
        const auto plan=steam_human_reward_skin(owner,1,sample[0],sample[1],o);
        const auto nums=parts<SteamFacilityNumber>(*plan);
        check(nums[0].value==sample[2],"满意封顶主值不由请求量5增加到1004");
        if(sample[0]&&sample[1]>=10)check(std::any_of(nums.begin(),nums.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::plus_value&&n.value==5;}),
            "封顶仍显示原请求+5而不是实际差0");
        const auto imgs=parts<SteamHumanImage>(*plan);
        check(std::any_of(imgs.begin(),imgs.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;})==(sample[0]==1&&sample[1]>=77),
            "共用奖励箭头只page1满77");
    }
    auto missing=o;missing.character_frame.reset();
    check(!steam_human_reward_skin(owner,1,1,77,missing),"共享人物表现帧缺失拒绝，不用结果counter补默认");
    owner=profession_fixture(63);owner.human_page_selections[id]=0;
    for(const auto sample:std::vector<std::array<int,2>>{{25,-1},{26,1},{30,2},{40,3},{46,4},{52,3},{54,3},{55,3},{70,2},{80,1},{84,-1},{109,-1},{110,-1},{196,-1},{197,-1}}) {
        owner.page_counters[id]=sample[0];
        const auto plan=steam_human_profession_result_skin(owner,id,o);
        check(plan&&plan->soft_labels==std::array<int,2>{0,0},"63只读演出无软返回");
        const auto imgs=parts<SteamHumanImage>(*plan);
        check(std::count_if(imgs.begin(),imgs.end(),[&](const auto &i){return i.asset==SteamHumanAsset::curtain&&i.frame>0;})==(sample[1]>=0?1:0),
            "63幕布边界及退休，无额外帧");
        if(sample[1]>=0)check(std::any_of(imgs.begin(),imgs.end(),[&](const auto &i){return i.asset==SteamHumanAsset::curtain&&i.frame==sample[1];}),
            "63幕布40起局部12槽交替3/4");
        check(std::any_of(imgs.begin(),imgs.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;})==(sample[0]==197),
            "63满197才有继续箭头，196无箭头");
        const auto actor=parts<SteamHumanPortrait>(*plan).front();
        check(actor.actor.body.sprite==22&&actor.actor.body.frame==0&&!actor.actor.weapon,"63action4/dir2固定human22/frame0无武器");
        check(actor.actor.body.image==owner.rules->jobs[sample[0]<55?0:1].sprites[0],"63前55当前职业，55起绑定目标，无持久旧职业字段");
    }
    const auto result_before=startup_world_state_digest(owner);
    const auto result_size=steam_human_profession_result_skin(owner,id,o)->operations.size();
    for(int i=0;i<12;++i)check(steam_human_profession_result_skin(owner,id,o)->operations.size()==result_size&&
        startup_world_state_digest(owner)==result_before,"63纯绘制不改职业/重算/发声/重抽冻结文案");
    owner.human_gift_messages.erase(id);
    check(!steam_human_profession_result_skin(owner,id,o),"63缺冻结文案拒绝，不由绘制随机生成");
}
void gift_result_skin_checks(Checks &check) {
    auto owner=profession_fixture(66);const auto id=owner.scripts.pages.back().id;
    owner.human_page_selections[id]=0;owner.human_equipment_choices[id]={0,1};owner.human_gift_scores[id]=24;
    SteamHumanOverviewOptions o;o.title_widths=std::array<int,2>{48,48};o.message_width=24;o.character_frame=0;
    for(int frame:{0,9,10,13,16,23,25,34,35,44,45,47,48,53,54,60,74,75,84,85,161,162}) {
        owner.page_counters[id]=frame;owner.page_phases[id]=frame>=135?1:0;
        const auto plan=steam_human_gift_result_skin(owner,id,o);
        check(plan&&plan->soft_labels==std::array<int,2>{0,0},"66礼物只读结果、无返回软标签");
        const auto images=parts<SteamHumanImage>(*plan);
        const auto people=parts<SteamHumanPortrait>(*plan);
        const auto president=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::president;});
        check(president!=images.end()&&president->frame==(frame>=10&&frame<20?9:1),"spChara2是总统，10..19行动3/direction1=frame9");
        check(people.front().actor.body.sprite==(frame>=25&&frame<35?25:1)&&people.front().actor.body.frame==0&&
              !people.front().actor.weapon,"上部25..34为buy01 SEB25/frame0，35无基础动作残留且无武器");
        const auto box=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::gift_box;});
        check(box!=images.end()&&box->frame==(frame<45?0:1),"45才开礼盒，不靠PNG数猜时序");
        if(frame==0)check(box->position==std::array<int,2>{126,137},"礼盒13前初始126/137");
        if(frame==13)check(box->position==std::array<int,2>{126,137},"13起抛物线局部0");
        if(frame==16)check(box->position==std::array<int,2>{125,134},"礼盒局部3：单精度抛物线向零截断3，横整数125");
        if(frame==23)check(box->position==std::array<int,2>{120,131},"礼盒局部10顶点6，横120");
        if(frame>=35)check(box->position==std::array<int,2>{114,137},"20槽抛物线结束114/137");
        const auto icon=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::weapon_icon;});
        check((icon!=images.end())==(frame>=45&&frame<75),"商品只45..74出现");
        if(frame==45)check(icon->position==std::array<int,2>{105,117},"商品45起105/117");
        if(frame==60)check(icon->position==std::array<int,2>{105,92},"60夹到92而非继续飞出");
        check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::gift_flash;})==(frame>=45&&frame<54),
              "dropItem00仅三个3槽帧，54消失，无绘制发声");
        if(frame>=75)check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::gift_face&&i.frame==1&&i.position==std::array<int,2>{63,157};}),
              "低评价使用icon_nico00 frame1固定y157，不使用满意smile00");
        check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;})==(frame==162),
              "66奖励page1局部77（总162）才有继续箭头，不混Owner提前确认门槛");
    }
    const auto before=startup_world_state_digest(owner);
    const auto size=steam_human_gift_result_skin(owner,id,o)->operations.size();
    for(int i=0;i<12;++i)check(steam_human_gift_result_skin(owner,id,o)->operations.size()==size&&startup_world_state_digest(owner)==before,
        "66重复绘制不扣款/发声/随机/奖励/装配/HP更新");
    owner.page_counters[id]=79;owner.page_phases[id]=0;
    for(int score:{24,25,69,70}) {
        owner.human_gift_scores[id]=score;
        const auto images=parts<SteamHumanImage>(*steam_human_gift_result_skin(owner,id,o));
        check(std::any_of(images.begin(),images.end(),[=](const auto &i){return i.asset==SteamHumanAsset::gift_face&&
            i.frame==(score<25?1:score<70?0:2)&&i.position==std::array<int,2>{63,score<25?157:156};}),
            "评价24/25/69/70三档真实帧及中高档8槽跳动");
    }
    const auto ordinary=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.recovery==0&&i.effect>=0&&i.effect<6;});
    check(ordinary!=owner.rules->items.end(),"固定表有普通属性道具");
    owner.human_equipment_choices[id]={4,ordinary->identity};owner.page_counters[id]=45;
    auto ordinary_plan=steam_human_gift_result_skin(owner,id,o);
    const auto ordinary_images=parts<SteamHumanImage>(*ordinary_plan);
    check(std::any_of(ordinary_images.begin(),ordinary_images.end(),[](const auto &i){return i.asset==SteamHumanAsset::item_icon&&i.position==std::array<int,2>{106,117};}),
          "普通道具沿真实图标裁片，商品锚106/117不同装备105/117");
    const auto learning=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.effect==9;});
    check(learning!=owner.rules->items.end(),"固定表有魔法学习道具");
    owner.human_equipment_choices[id]={4,learning->identity};owner.page_counters[id]=75;
    const auto learned=parts<SteamHumanOverviewText>(*steam_human_gift_result_skin(owner,id,o));
    check(std::any_of(learned.begin(),learned.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::result_message&&t.value.find("习得")!=std::string::npos;}),
          "学习道具气泡取魔法学习文案，不直接用冻结普通评价");
    const auto recovery=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &i){return i.recovery>0;});
    check(recovery!=owner.rules->items.end(),"固定表有真实回复道具");
    owner.human_equipment_choices[id]={4,recovery->identity};owner.page_counters[id]=75;owner.page_phases[id]=0;
    owner.scene.world.world.ai.human_order.clear();
    auto plan=steam_human_gift_result_skin(owner,id,o);
    const auto rectangles=[&](const SteamHumanOverviewPlan &p) {
        std::vector<StartupSkinRect> result;
        for(const auto &op:p.operations)if(const auto *f=std::get_if<SteamStartupFrameDraw>(&op))
            if(const auto *r=std::get_if<StartupSkinRect>(f))result.push_back(*r);
        return result;
    };
    auto rects=rectangles(*plan);
    check(std::any_of(rects.begin(),rects.end(),[](const auto &r){return r.rect==std::array<int,4>{97,115,18,2}&&r.rgb==std::array<int,3>{83,255,0};}),
          "66无W原特殊恢复展示满18，不修改60无W不画条");
    for(int frame:{75,78,79,82,83,86,87,90,91,114,115}) {
        owner.page_counters[id]=frame;plan=steam_human_gift_result_skin(owner,id,o);
        const auto images=parts<SteamHumanImage>(*plan);
        const auto effect=std::find_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::recovery_effect;});
        check((effect!=images.end())==(frame<91),"回复effect按4/8/12/16四门槛结束，不循环");
        if(effect!=images.end())check(effect->frame==(frame-75)/4,"回复四帧门槛向右独立");
        check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::recovery_number;})==(frame<115),
              "回复数字SEB18仅75..114，115改HP回复气泡");
    }
    owner.page_counters[id]=75;
    ref::BattleActorRecord actor;actor.id={510};actor.definition=1;actor.kind=ref::ActorKind::human;
    actor.hp.displayed=actor.hp.target=0;owner.scene.world.world.ai.battle.actors[actor.id]=actor;
    owner.scene.world.world.ai.human_order.push_back(actor.id);
    rects=rectangles(*steam_human_gift_result_skin(owner,id,o));
    check(std::any_of(rects.begin(),rects.end(),[](const auto &r){return r.rect==std::array<int,4>{97,115,0,2}&&r.rgb==std::array<int,3>{83,255,0};}),
          "真实实例HP0取NOW空条，不用缺W的满条fallback");
    owner.human_gift_scores.erase(id);
    check(!steam_human_gift_result_skin(owner,id,o),"66缺评价冻结载荷拒绝，不在绘制重评礼物");
    owner.human_gift_scores[id]=30;owner.scripts.pages.back().lifecycle=4;
    check(!steam_human_gift_result_skin(owner,id,o),"66退休页面不再输出人物/礼盒/HP表现引用");
}
void attribute_result_checks(Checks &check) {
    // 只构造已兑现的表现载荷；不认证夹具是自然赠礼／成长所得。
    SteamHumanOverviewOptions o;o.title_widths=std::array<int,2>{48,48};o.result_name_width=40;o.result_suffix_width=24;
    auto owner=profession_fixture(68);const auto id=owner.scripts.pages.back().id;
    owner.human_page_selections[id]=0;owner.human_equipment_choices[id]={0,1};
    owner.shop_humans.at(1).equipment[0].reset(); // 现装为空仍消费冻结newID，不能由现装补头。
    owner.equipment_attribute_display={std::array<int,4>{100,100,100,100},
        std::array<int,4>{130,70,100,115},std::array<int,4>{30,-30,0,15}};
    // Steam门槛独立表：frame、首行主值、差额数量、继续箭头。
    for(const auto sample:std::vector<std::array<int,4>>{{0,100,0,0},{1,100,0,0},{2,100,1,0},
        {7,100,1,0},{8,100,2,0},{13,100,2,0},{14,100,2,0},{19,100,2,0},{20,100,3,0},
        {41,100,3,0},{42,100,3,0},{51,100,3,0},{52,101,3,0},{53,102,3,0},
        {59,114,3,0},{66,128,3,0},{67,130,3,0},{72,130,3,0},{73,130,3,1},
        {87,130,3,1},{88,130,3,0},{97,130,3,0},{98,130,3,1}}) {
        owner.page_counters[id]=sample[0];const auto plan=steam_human_attribute_result_skin(owner,id,o);
        check(plan&&plan->title=="装备变更"&&plan->soft_labels==std::array<int,2>{0,0},"68纯计划有正确标题与软标签");
        const auto values=parts<SteamFacilityNumber>(*plan);
        check(values.front().value==sample[1]&&values.front().asset==SteamFacilityAsset::number08&&values.front().position==std::array<int,2>{138,101},
            "68主值SEB15右138及52/67插值门槛");
        check(std::count_if(values.begin(),values.end(),[](const auto &n){return n.kind!=SteamFacilityNumberKind::number;})==sample[2],
            "68四行错开6槽，零差行不画加零");
        const auto images=parts<SteamHumanImage>(*plan);
        check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor&&i.frame==0&&i.position==std::array<int,2>{208,187};})==bool(sample[3]),
            "68满73箭头25槽前15可见，局部而非全局取余");
        check(std::count_if(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::combat_icon;})==4,"68mode8四个combat图标");
    }
    owner.page_counters[id]=67;auto plan=steam_human_attribute_result_skin(owner,id,o);
    const auto values=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(values.begin(),values.end(),[](const auto &n){return n.parameter==1&&n.kind==SteamFacilityNumberKind::minus_value&&n.asset==SteamFacilityAsset::number12&&n.value==30&&n.position==std::array<int,2>{202,124};}),
        "68负差SEB19 minus正幅值，与SEB12正差分开");
    auto texts=parts<SteamHumanOverviewText>(*plan);auto images=parts<SteamHumanImage>(*plan);
    const auto chosen=std::find_if(owner.rules->equipment.begin(),owner.rules->equipment.end(),[](const auto &e){return e.shop.kind==1&&e.shop.id==1;});
    check(chosen!=owner.rules->equipment.end()&&texts[0].value==chosen->name&&texts[0].rectangle==std::array<int,4>{95,70,0,0}&&texts[0].font_size==11&&
        texts[1].value=="装备"&&texts[1].rectangle==std::array<int,4>{165,70,0,0}&&texts[1].anchor==4,
        "68非JP在11号字测宽，头名取冻结newID而非当前装备");
    check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::weapon_icon&&i.position==std::array<int,2>{75,67};}),"68头图标按实际整组宽90居中");
    check(texts[2].role==SteamHumanOverviewTextRole::result_message&&texts[2].rectangle==std::array<int,4>{120,198,0,0}&&texts[2].anchor==2,"68说明底注中心120/198");
    auto jp=o;jp.japanese=true;jp.result_suffix_width.reset();
    const auto jp_text=parts<SteamHumanOverviewText>(*steam_human_attribute_result_skin(owner,id,jp));
    check(jp_text[0].rectangle[0]==89&&jp_text[0].font_size==0&&jp_text[1].rectangle[0]==135&&jp_text[1].anchor==0,"68JP固定nameWidth+62及后缀左锚减36，不需测后缀");
    for(int slot=0;slot<4;++slot) {
        const auto found=std::find_if(owner.rules->equipment.begin(),owner.rules->equipment.end(),[=](const auto &e){return e.shop.kind==(slot==0?1:slot==3?3:2)&&(e.shop.kind!=2||(e.shop.type==2?1:2)==slot);});
        check(found!=owner.rules->equipment.end(),"固定表包含四槽装备");
        owner.human_equipment_choices[id]={slot,found->shop.id};const auto selected=steam_human_attribute_result_skin(owner,id,o);
        const auto head=parts<SteamHumanImage>(*selected);
        check(head[1].asset==(slot==0?SteamHumanAsset::weapon_icon:slot==3?SteamHumanAsset::accessory_icon:SteamHumanAsset::armour_icon)&&
            parts<SteamHumanOverviewText>(*selected).front().value==found->name,"68四槽图标／名称来自真实choice命名空间");
    }
    const auto digest=startup_world_state_digest(owner);const auto size=steam_human_attribute_result_skin(owner,id,o)->operations.size();
    for(int i=0;i<12;++i)check(steam_human_attribute_result_skin(owner,id,o)->operations.size()==size&&startup_world_state_digest(owner)==digest,"68重复查询规模稳定，不装配／奖励／随机／声音写入");
    auto bad=owner;bad.human_equipment_choices.erase(id);check(!steam_human_attribute_result_skin(bad,id,o),"68缺冻结来源拒绝，不拿当前装备补头");
    for(const auto choice:std::vector<std::array<int,2>>{{-1,0},{4,0},{0,9999}}) {
        bad=owner;bad.human_equipment_choices[id]=choice;check(!steam_human_attribute_result_skin(bad,id,o),"68坏slot／来源ID拒绝");
    }
    const auto armour=std::find_if(owner.rules->equipment.begin(),owner.rules->equipment.end(),[](const auto &e){return e.shop.kind==2;});
    check(armour!=owner.rules->equipment.end(),"固定表包含真实防具");
    bad=owner;bad.human_equipment_choices[id]={armour->shop.type==2?2:1,armour->shop.id};
    check(!steam_human_attribute_result_skin(bad,id,o),"68防具真实分类与slot不符拒绝");
    bad=owner;bad.equipment_attribute_display[2][0]=29;check(!steam_human_attribute_result_skin(bad,id,o),"68冻结差额失配拒绝");
    bad=owner;bad.equipment_attribute_display[0][0]=0;bad.equipment_attribute_display[1][0]=std::numeric_limits<int>::min();bad.equipment_attribute_display[2][0]=std::numeric_limits<int>::min();
    check(!steam_human_attribute_result_skin(bad,id,o),"68 INT_MIN差额不能伪造正幅值，显式拒绝");
    bad=owner;bad.scripts.pages.back().lifecycle=4;check(!steam_human_attribute_result_skin(bad,id,o),"68退休页拒绝");
    auto missing=o;missing.result_name_width.reset();check(!steam_human_attribute_result_skin(owner,id,missing),"68缺实际名字测宽拒绝");
    missing=o;missing.result_suffix_width.reset();check(!steam_human_attribute_result_skin(owner,id,missing),"68非JP缺后缀测宽拒绝");

    owner=profession_fixture(69);const auto item=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &d){return d.effect>=0&&d.effect<6;});
    check(item!=owner.rules->items.end(),"固定表有真实六属性道具");
    owner.human_page_selections[id]=0;owner.human_equipment_choices[id]={4,item->identity};
    owner.human_attribute_display={std::array<int,6>{100,100,100,100,100,100},
        std::array<int,6>{130,130,130,130,130,130},std::array<int,6>{30,30,30,30,30,30}};
    owner.human_attribute_display[1][item->effect]=70;owner.human_attribute_display[2][item->effect]=-30;
    // 100→70的signed余差令终点夹回24；helper返回before100，39才由调用者显示after70。
    for(const auto sample:std::vector<std::array<int,3>>{{1,100,0},{2,100,0},{13,100,0},{14,100,0},
        {23,100,0},{24,100,0},{25,100,0},{31,100,0},{38,100,0},{39,70,0},{44,70,0},
        {45,70,1},{59,70,1},{60,70,0},{69,70,0},{70,70,1}}) {
        owner.page_counters[id]=sample[0];plan=steam_human_attribute_result_skin(owner,id,o);
        check(plan&&plan->title=="能力上升", "69只读道具结果标题");
        const auto numbers=parts<SteamFacilityNumber>(*plan);
        check(std::count_if(numbers.begin(),numbers.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::number;})==6&&
            std::any_of(numbers.begin(),numbers.end(),[&](const auto &n){return n.kind==SteamFacilityNumberKind::number&&n.parameter==item->effect&&n.value==sample[1];}),"69六主值24/39门槛，effect行负差保持旧值至39再下降");
        check(std::count_if(numbers.begin(),numbers.end(),[](const auto &n){return n.kind!=SteamFacilityNumberKind::number;})==(sample[0]>=2?1:0),"69只effect非零行出差额，其余合法非零不误拒绝或显示");
        const auto imgs=parts<SteamHumanImage>(*plan);
        check(std::any_of(imgs.begin(),imgs.end(),[](const auto &i){return i.asset==SteamHumanAsset::continue_cursor;})==bool(sample[2]),"69满45箭头局部25槽前15可见");
        const auto attrs=parts<SteamHumanOverviewText>(*plan);
        check(std::count_if(attrs.begin(),attrs.end(),[](const auto &t){return t.role==SteamHumanOverviewTextRole::attribute&&t.rectangle==std::array<int,4>{49,91+17*t.argument,0,0}&&t.rgb==std::array<int,3>{0,100,255};})==6,
            "69mode7六图标及蓝属性名49/y-2");
    }
    texts=parts<SteamHumanOverviewText>(*plan);images=parts<SteamHumanImage>(*plan);
    check(texts[0].value==item->name&&texts[0].rectangle[0]==96&&texts[1].value=="赠送"&&texts[1].rectangle[0]==164&&texts[1].anchor==4,"69nameWidth+24+后缀宽，不用装备26偏移");
    check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamHumanAsset::item_icon&&i.position==std::array<int,2>{76,67};}),"69使用真实item图标而非effect图或装备图");
    const auto minus=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(minus.begin(),minus.end(),[&](const auto &n){return n.parameter==item->effect&&n.kind==SteamFacilityNumberKind::minus_value&&n.asset==SteamFacilityAsset::number12&&n.value==30&&n.position==std::array<int,2>{202,93+17*item->effect};}),"69负差同样SEB19 minus正幅值右202");
    const auto item_digest=startup_world_state_digest(owner);const auto item_size=plan->operations.size();
    for(int i=0;i<12;++i)check(steam_human_attribute_result_skin(owner,id,o)->operations.size()==item_size&&startup_world_state_digest(owner)==item_digest,"69重复查询规模稳定，不消费库存／奖励／随机／声音");
    owner.human_attribute_display[1][item->effect]=100;owner.human_attribute_display[2][item->effect]=0;
    const auto zero=parts<SteamFacilityNumber>(*steam_human_attribute_result_skin(owner,id,o));
    check(std::none_of(zero.begin(),zero.end(),[](const auto &n){return n.kind!=SteamFacilityNumberKind::number;}),"69真实effect零差不显示+0，其余非零仍不出差额");
    owner.human_attribute_display[1][item->effect]=130;owner.human_attribute_display[2][item->effect]=30;
    const auto plus=parts<SteamFacilityNumber>(*steam_human_attribute_result_skin(owner,id,o));
    check(std::any_of(plus.begin(),plus.end(),[&](const auto &n){return n.parameter==item->effect&&n.kind==SteamFacilityNumberKind::plus_value&&n.asset==SteamFacilityAsset::number05&&n.value==30&&n.position==std::array<int,2>{202,93+17*item->effect};}),"69正差SEB12 plus正幅值右202");
    bad=owner;bad.human_equipment_choices.erase(id);check(!steam_human_attribute_result_skin(bad,id,o),"69缺item来源拒绝");
    bad=owner;bad.human_equipment_choices[id]={0,item->identity};check(!steam_human_attribute_result_skin(bad,id,o),"69非道具slot拒绝");
    bad=owner;bad.human_equipment_choices[id]={4,9999};check(!steam_human_attribute_result_skin(bad,id,o),"69坏item引用拒绝");
    const auto other=std::find_if(owner.rules->items.begin(),owner.rules->items.end(),[](const auto &d){return d.effect>=6;});
    check(other!=owner.rules->items.end(),"固定表包含非六属性道具");
    bad=owner;bad.human_equipment_choices[id]={4,other->identity};check(!steam_human_attribute_result_skin(bad,id,o),"69非六属性道具拒绝，不混66回复／学习");
    bad=owner;bad.human_attribute_display[2][0]=31;check(!steam_human_attribute_result_skin(bad,id,o),"69任一行冻结三元失配拒绝");
    bad=owner;bad.scripts.pages.back().lifecycle=4;check(!steam_human_attribute_result_skin(bad,id,o),"69退休页拒绝");
    bad=owner;bad.scripts.pages.back().legacy_page=70;check(!steam_human_attribute_result_skin(bad,id,o),"70不冒认为本入口支持");
    // Steam实参8/12的冻结float32截断oracle，不以被测helper生成期望。
    constexpr std::array<int,13> bounce{0,2,4,6,7,8,8,7,6,4,2,0,0};
    for(int raw:{68,69}) {
        auto animated=profession_fixture(raw);animated.human_page_selections[id]=0;
        animated.human_equipment_choices[id]=raw==68?std::array<int,2>{0,1}:std::array<int,2>{4,item->identity};
        animated.equipment_attribute_display={std::array<int,4>{10,10,10,10},std::array<int,4>{13,13,13,13},std::array<int,4>{3,3,3,3}};
        animated.human_attribute_display={std::array<int,6>{10,10,10,10,10,10},std::array<int,6>{13,13,13,13,13,13},std::array<int,6>{3,3,3,3,3,3}};
        const int effect=raw==68?0:item->effect,baseline=raw==68?101:93+17*effect;
        for(int local=0;local<13;++local) {
            animated.page_counters[id]=2+local;
            const auto numbers=parts<SteamFacilityNumber>(*steam_human_attribute_result_skin(animated,id,o));
            check(std::any_of(numbers.begin(),numbers.end(),[&](const auto &n){return n.parameter==effect&&n.kind==SteamFacilityNumberKind::plus_value&&n.position==std::array<int,2>{202,baseline-bounce[local]};}),"68/69跳动8/12单精度冻结位置oracle");
        }
        // Steam RVA2A4620与APK一致：差3提前六槽收口，下降／±1保留旧值至调用者末门槛。
        for(const auto sample:std::vector<std::array<int,4>>{{10,13,0,11},{10,13,1,11},{10,13,3,12},{10,13,6,13},
            {13,10,0,13},{13,10,3,13},{13,10,6,13},{13,10,14,13},{13,10,15,10},
            {10,11,0,10},{10,11,14,10},{10,11,15,11},
            {11,10,0,11},{11,10,14,11},{11,10,15,10},{10,10,0,10},{10,10,15,10}}) {
            animated.page_counters[id]=(raw==68?52:24)+sample[2];
            if(raw==68) {
                animated.equipment_attribute_display[0][effect]=sample[0];animated.equipment_attribute_display[1][effect]=sample[1];animated.equipment_attribute_display[2][effect]=sample[1]-sample[0];
            } else {
                animated.human_attribute_display[0][effect]=sample[0];animated.human_attribute_display[1][effect]=sample[1];animated.human_attribute_display[2][effect]=sample[1]-sample[0];
            }
            const auto numbers=parts<SteamFacilityNumber>(*steam_human_attribute_result_skin(animated,id,o));
            check(std::any_of(numbers.begin(),numbers.end(),[&](const auto &n){return n.parameter==effect&&n.kind==SteamFacilityNumberKind::number&&n.value==sample[3];}),"68/69countAnime差3/下降及±1保持旧值至末门槛/零冻结oracle");
        }
    }
}
void profession_catalogue_checks(Checks &check) {
    // 目录规模／满级／工资展示条件夹具，不认证自然开放与职业成长。
    auto owner=test_support::page_fixture(61);const auto id=owner.scripts.pages.back().id;
    owner.scripts.pages.back().lifecycle=1;owner.scripts.executing_page.reset();
    owner.page_human_bindings[id]=1;owner.page_phases[id]=0;owner.page_counters[id]=0;
    owner.human_page_selections[id]=2;owner.human_page_scroll[id]=1;owner.human_pages_initialized.insert(id);
    owner.human_page_catalogs[id].clear();
    const auto profile=startup_world_human_details(owner,1);check(profile.has_value(),"职业目录夹具有共享人物性别");
    for(int job=0;job<10;++job) {
        const auto &definition=owner.rules->jobs[job];
        if(definition.script_extra!=-1&&definition.script_extra!=profile->sex)continue;
        owner.human_page_catalogs[id].push_back(job);owner.scripts.professions.at(job).status=1;
    }
    check(owner.human_page_catalogs[id].size()>=6,"目录五行之外有独立滚动证据");
    auto &growth=owner.scene.world.world.ai.growth.at(1).definition;
    growth.current_profession=0;growth.profession_levels[0]=2;growth.profession_levels[1]=10;
    owner.scripts.professions.at(1).pending_notice=true;owner.village_points=0;
    SteamHumanOverviewOptions options;options.title_widths=std::array<int,2>{72,72};
    auto plan=steam_human_profession_catalogue_skin(owner,id,options);
    check(plan&&plan->soft_labels==std::array<int,2>{0,2}&&plan->title=="职业一览 1/2","61真实两页显示资格，左空右返回");
    const auto rows=parts<SteamHumanEquipmentRow>(*plan);
    const auto portraits=parts<SteamHumanPortrait>(*plan);
    check(rows.size()==5&&rows.front().touch.component==11&&rows.front().touch.value==(0x20000|1)&&
          rows.front().touch.rectangle==std::optional<std::array<int,4>>{{3,95,231,18}}&&
          rows.front().margin==std::array<int,4>{0,-20,0,0},"61用Owner first1，不从selection2重派生窗口；Touch11全目录下标");
    const auto scrolls=parts<SteamHumanCatalogueScroll>(*plan);
    check(scrolls.size()==1&&scrolls.front().touch.component==12&&scrolls.front().touch.value==0x40000&&
          scrolls.front().arguments==std::array<int,3>{static_cast<int>(owner.human_page_catalogs[id].size()),5,0x20000}&&
          scrolls.front().touch.rectangle==std::optional<std::array<int,4>>{{221,85,3,110}}&&
          scrolls.front().track_touch.component==25&&scrolls.front().track_touch.option==4,
          "61五行滚动请求保留12/25，原宽4映射基矩形宽3，非raw60四页滚动条");
    const auto backgrounds=parts<SteamStartupFrameDraw>(*plan);
    check(std::any_of(backgrounds.begin(),backgrounds.end(),[](const auto &v){const auto *r=std::get_if<StartupSkinRect>(&v);
          return r&&r->rect==std::array<int,4>{23,114,191,16}&&r->rgb==std::array<int,3>{255,153,55};}),
          "61选中行原SC_WINDOW_SELECT，不误用滚动first-touch橙246/129/0");
    check(portraits.size()==6&&portraits.front().position==std::array<int,2>{37,90}&&
          portraits.front().clip==std::optional<std::array<int,4>>{{29,68,15,14}}&&!portraits.front().image_clip&&
          portraits[1].position==std::array<int,2>{37,118},"61顶及行clip2，身体不带武器，不误套62 ImageClip");
    const auto assets=parts<SteamHumanImage>(*plan);
    const auto images=parts<SteamFacilityImage>(*plan);
    check(std::any_of(assets.begin(),assets.end(),[](const auto &i){return i.asset==SteamHumanAsset::catalogue_new&&i.position==std::array<int,2>{10,99};}),
          "61目录NEW来自共享职业提示，实际first1首行y97");
    check(std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamFacilityAsset::maximum&&i.position==std::array<int,2>{138,99};})&&
          std::any_of(images.begin(),images.end(),[](const auto &i){return i.asset==SteamFacilityAsset::finger&&i.position==std::array<int,2>{21,124};}),
          "61等级10用MAX，原选中手指在第二可见行21/y116+8");
    auto numbers=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(numbers.begin(),numbers.end(),[](const auto &n){return n.asset==SteamFacilityAsset::number12&&n.position==std::array<int,2>{212,98};}),
          "61点数不足为红SEB19费用，不扣点也不补现金符号");
    owner.page_phases[id]=1;owner.human_page_selections[id]=0;owner.human_page_scroll[id]=0;
    for(auto &status:owner.scripts.humans)status.second.status=0;
    owner.scripts.humans.at(0).status=owner.scripts.humans.at(1).status=1;
    owner.scene.world.world.ai.growth.at(0).definition.current_profession=0;
    owner.scene.world.world.ai.human_order.clear();
    plan=steam_human_profession_catalogue_skin(owner,id,options);check(plan.has_value(),"61页1在没有场上实例时仍统计共享人物");
    numbers=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(numbers.begin(),numbers.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::money&&
          n.asset==SteamFacilityAsset::number08&&n.value==166&&n.position==std::array<int,2>{212,98};}),
          "原职业0工资150..300，二级展示166；不套账本十整160");
    check(std::any_of(numbers.begin(),numbers.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::comma_number&&
          n.asset==SteamFacilityAsset::number05&&n.value==2&&n.position==std::array<int,2>{153,98};}),
          "开放定义0/1共两人即使无W，人数沿count4而非活跃列表");
    options.japanese=true;
    const auto jp_numbers=parts<SteamFacilityNumber>(*steam_human_profession_catalogue_skin(owner,id,options));
    check(std::any_of(jp_numbers.begin(),jp_numbers.end(),[](const auto &n){return n.kind==SteamFacilityNumberKind::person_count&&
          n.value==2&&n.position==std::array<int,2>{153,98};}),"JP保留count4具名请求，dx-10与人数单位由共用数字后端一次消费");
    options.japanese=false;
    for(int type:{0,1,7}) {
        const auto found=std::find_if(owner.human_page_catalogs[id].begin(),owner.human_page_catalogs[id].end(),[&](int j){return owner.rules->jobs[j].type==type;});
        check(found!=owner.human_page_catalogs[id].end(),"已核职业目录含销售/饮食/其他经济类别");
        owner.human_page_selections[id]=static_cast<int>(found-owner.human_page_catalogs[id].begin());
        owner.human_page_scroll[id]=std::max(0,owner.human_page_selections[id]-4);
        plan=steam_human_profession_catalogue_skin(owner,id,options);check(plan.has_value(),"经济奖励类别有独立原描述");
        const auto footer=parts<SteamFacilityImage>(*plan);
        check(std::any_of(footer.begin(),footer.end(),[](const auto &i){return i.asset==SteamFacilityAsset::facility_icon;})==(type==0||type==1),
              "销售/饮食经济bonus才画设施类别图，不按mastery_attribute映射");
        const auto text=parts<SteamHumanOverviewText>(*plan);
        check(std::any_of(text.begin(),text.end(),[&](const auto &t){return t.role==SteamHumanOverviewTextRole::profession_bonus_description&&
              !t.value.empty()&&(type==0||type==1||t.value==owner.rules->jobs[*found].bonus_description);}),
              "其它职业经济说明取列22，不复用大师奖励");
    }
    const auto digest=startup_world_state_digest(owner);
    const auto size=plan->operations.size();
    for(int i=0;i<12;++i)check(steam_human_profession_catalogue_skin(owner,id,options)->operations.size()==size&&
        startup_world_state_digest(owner)==digest,"61重复投影不增资源/随机/输出，不清共享NEW");
    auto bad=owner;bad.human_page_scroll.erase(id);check(!steam_human_profession_catalogue_skin(bad,id,options),"61缺scroll显式拒绝");
    bad=owner;bad.human_page_catalogs[id].back()=999;check(!steam_human_profession_catalogue_skin(bad,id,options),"61屏外非法目录引用也拒绝");
    bad=owner;bad.scripts.pages.back().lifecycle=4;check(!steam_human_profession_catalogue_skin(bad,id,options),"61退休页无目录表现引用");
}
}
namespace {
void residence_result_checks(Checks &check,const std::filesystem::path &source_root) {
    const auto &facilities=startup_world_rules().facilities;
    const auto home=std::find_if(facilities.begin(),facilities.end(),[](const auto &d){return d.kind==12;});
    check(home!=facilities.end(),"真实目录住宅定义");
    SteamHumanResidenceResultInput in;in.mapchip=home->display_id;
    in.human={0,1,0,0,0,"住宅条件",{{{95,90},{100,100},{20,15}}}};
    SteamHumanOverviewOptions o;o.title_widths=std::array<int,2>{48,48};
    auto plan=steam_human_residence_result_skin(in,o);check(bool(plan),"96第一段不需未消费的共享人物钟");
    check(plan->title=="自宅完成"&&plan->soft_labels==std::array<int,2>{0,0},"96无返回软标签");
    const auto touch=parts<SteamStartupTouch>(*plan);
    check(touch.size()==1&&touch[0].component==2&&!touch[0].rectangle&&touch[0].option==2,"96原全域Touch2无显式按钮");
    for(const auto sample:std::array<std::array<int,2>,4>{{{5,0},{6,1},{39,46},{40,48}}}) {
        in.counter=sample[0];plan=steam_human_residence_result_skin(in,o);
        const auto clip=parts<SteamHumanClip>(*plan);const auto text=parts<SteamHumanOverviewText>(*plan);
        check(clip.size()==2&&clip[0].kind==SteamHumanClipKind::push_intersect&&
            clip[0].rectangle==std::array<int,4>{0,158,240,sample[1]}&&clip[1].kind==SteamHumanClipKind::pop,
            "96底文5..40展开、顶部不裁且成对释放");
        check(text.size()==2&&text[0].rich&&!text[1].rich&&text[0].rectangle==std::array<int,4>{120,165,0,0}&&
              text[1].rectangle==std::array<int,4>{120,187,0,0},"96原DrawString2与普通DrawString资格、基点分开");
    }
    for(const auto sample:std::array<std::array<int,2>,5>{{{49,0},{50,1},{64,1},{65,0},{75,1}}}) {
        in.counter=sample[0];plan=steam_human_residence_result_skin(in,o);
        const auto images=parts<SteamHumanImage>(*plan);
        check(std::count_if(images.begin(),images.end(),[](const auto &v){return v.asset==SteamHumanAsset::continue_cursor;})==sample[1],
            "96首段箭头50开始并用原main模25，不是确认40或偏移后钟");
    }
    for(const auto sample:std::array<std::array<int,3>,7>{{{9,2,142},{10,10,142},{11,10,140},{12,10,139},{17,10,134},{25,10,142},{26,2,142}}}) {
        in.secondary=sample[0];plan=steam_human_residence_result_skin(in,o);
        const auto actors=parts<SteamHumanPortrait>(*plan);const auto imgs=parts<SteamHumanImage>(*plan);
        check(actors.size()==1&&actors[0].actor.body.sprite==sample[1]&&actors[0].actor.body.frame==0&&
            actors[0].position==std::array<int,2>{175,sample[2]}&&!actors[0].actor.weapon,
            "96顶人物无武器、副钟26槽、float32跳高独立oracle");
        check(std::any_of(imgs.begin(),imgs.end(),[](const auto &v){return v.asset==SteamHumanAsset::shadow&&v.position==std::array<int,2>{175,142};}),
            "96固定阴影不跟身体跳");
    }
    const auto map=parts<SteamFacilityMapchip2>(*plan);
    check(map.size()==1&&map[0].position==std::array<int,2>{120,113}&&map[0].orientation==0,"96调用Mapchip2固定锚和朝向");
    bool border=false;for(const auto &op:plan->operations)if(const auto *d=std::get_if<SteamStartupFrameDraw>(&op))
        if(const auto *r=std::get_if<StartupSkinRect>(d))border|=r->outline&&r->rect==std::array<int,4>{30,66,180,82}&&r->rgb==std::array<int,3>{148,148,148};
    check(border,"原DrawRect179/81转换半开180/82，不双加边界");
    const auto map_at=std::find_if(plan->operations.begin(),plan->operations.end(),[](const auto &v){return std::holds_alternative<SteamFacilityMapchip2>(v);});
    check(map_at!=plan->operations.end()&&std::holds_alternative<SteamHumanImage>(*(map_at+1))&&
        std::get<SteamHumanImage>(*(map_at+1)).asset==SteamHumanAsset::shadow&&std::holds_alternative<SteamHumanPortrait>(*(map_at+2)),
        "Mapchip2后先阴影再人物，不重排层次");
    in.phase=1;in.counter=77;check(!steam_human_residence_result_skin(in,o),"96第二段缺共享钟显式拒绝");
    o.character_frame=12;plan=steam_human_residence_result_skin(in,o);check(bool(plan),"96复用mentalUp已冻结奖励");
    const auto actors=parts<SteamHumanPortrait>(*plan);
    check(actors.size()==2&&actors[0].position==std::array<int,2>{62,181}&&actors[0].actor.weapon&&
        actors[0].actor.body.frame==3&&!actors[1].actor.weapon,"底人物共享钟12帧3、顶人物副钟独立，无武器资格不同");
    const auto values=parts<SteamFacilityNumber>(*plan);
    check(std::any_of(values.begin(),values.end(),[](const auto &v){return v.kind==SteamFacilityNumberKind::plus_value&&v.value==20;})&&
        std::any_of(values.begin(),values.end(),[](const auto &v){return v.kind==SteamFacilityNumberKind::plus_value&&v.value==15;}),
        "封顶后仍显示原请求20/15，不用实际差5/10");
    auto bad=in;bad.secondary=-1;check(!steam_human_residence_result_skin(bad,o),"96负副钟拒绝");
    bad=in;bad.mapchip=999;check(!steam_human_residence_result_skin(bad,o),"96坏住宅图绑定拒绝");
    bad=in;bad.human.rewards[0][0]=-1;check(!steam_human_residence_result_skin(bad,o),"96坏冻结值拒绝");
    for(int i=0;i<8;++i)check(steam_human_residence_result_skin(in,o)->operations.size()==plan->operations.size(),"96重复投影规模稳定");
    for(const auto a:{SteamHumanAsset::residence_background,SteamHumanAsset::shadow}) {
        const auto r=steam_human_resource(a);check(bool(r),"96资源具名映射");
        const auto root=source_root.parent_path();auto img=LoadImage((root/r->published_image).string().c_str());
        check(img.data!=nullptr&&img.width==(a==SteamHumanAsset::shadow?123:93)&&img.height==(a==SteamHumanAsset::shadow?3:82),"96复用原PNG尺寸");UnloadImage(img);
    }
    // 旧Owner mentalUp入口与纯DTO只在取数边界分离，主动画断言仍由既有profession/gift负责。
    const auto owner=test_support::world_fixture();const auto h=startup_world_human_details(owner,1);check(bool(h),"真实人物reward投影");
    SteamHumanRewardInput reward{h->profession,h->sex,h->equipment[0].value_or(-1),1,77,h->name,owner.reward_display};
    const auto old=steam_human_reward_skin(owner,1,1,77,o),pure=steam_human_reward_skin(reward,o);
    check(old&&pure&&old->operations.size()==pure->operations.size()&&parts<SteamFacilityNumber>(*old).size()==parts<SteamFacilityNumber>(*pure).size(),
        "旧入口仍复用同mentalUp输出结构");
    auto bound=test_support::page_fixture(96);const auto id=bound.scripts.pages.back().id;
    bound.scripts.pages.back().lifecycle=2;bound.scripts.pages.back().legacy_f=home->id;
    bound.page_human_bindings[id]=1;bound.page_counters[id]=77;bound.page_phases[id]=1;bound.page_secondary_counters[id]=18;
    bound.reward_display=in.human.rewards;
    check(valid_startup_world_runtime_residence_result_page(bound,bound.scripts.pages.back()),"96明确已完成住宅演出的绑定夹具");
    const auto digest=startup_world_state_digest(bound);
    o.character_frame=steam_human_shared_character_frame(bound);
    const auto result=steam_human_residence_result_skin(bound,id,o);check(bool(result),"96真实Owner字段严格桥");
    for(int i=0;i<8;++i)check(steam_human_residence_result_skin(bound,id,o)->operations.size()==result->operations.size()&&
        startup_world_state_digest(bound)==digest,"96只读桥不改字段、奖励、随机或一次输出");
    auto broken=bound;broken.page_secondary_counters.erase(id);check(!steam_human_residence_result_skin(broken,id,o),"96缺副钟不能代用main");
    broken=bound;broken.scripts.pages.back().legacy_f=33;check(!steam_human_residence_result_skin(broken,id,o),"96不是任意设施定义");
    broken=bound;broken.page_human_bindings[id]=999;check(!steam_human_residence_result_skin(broken,id,o),"96错人物显式拒绝");
    broken=bound;broken.scripts.pages.back().lifecycle=4;check(!steam_human_residence_result_skin(broken,id,o),"96退休后不能再绘制");
}
}
// 复用既有visuals执行文件；本层只验表现边界，不重测人物成长/战斗/职业业务。
int check_steam_human_skin(const std::filesystem::path &source_root) {
    Checks check;power_checks(check);medal_bubble_checks(check);resources(check,source_root);owner_checks(check);overview_checks(check);attributes_checks(check);equipment_magic_checks(check);equipment_detail_checks(check);gift_confirmation_checks(check);profession_skin_checks(check);gift_result_skin_checks(check);
    gift_catalogue_checks(check);profession_catalogue_checks(check);attribute_result_checks(check);shared_character_clock_checks(check);
    residence_result_checks(check,source_root);
    return check.count;
}
